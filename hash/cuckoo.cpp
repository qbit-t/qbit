#include "cuckoo.h"
#include <stdlib.h>


uint64_t siphash24(const siphash_keys* keys, const uint64_t nonce)
{
    uint64_t v0 = keys->k0 ^ 0x736f6d6570736575ULL,
             v1 = keys->k1 ^ 0x646f72616e646f6dULL,
             v2 = keys->k0 ^ 0x6c7967656e657261ULL,
             v3 = keys->k1 ^ 0x7465646279746573ULL ^ nonce;
    SIPROUND;
    SIPROUND;
    v0 ^= nonce;
    v2 ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return (v0 ^ v1) ^ (v2 ^ v3);
}

// convenience function for extracting siphash keys from header
void setKeys(const char* header, const uint32_t headerlen, siphash_keys* keys)
{
    char hdrkey[32];
    // SHA256((unsigned char *)header, headerlen, (unsigned char *)hdrkey);
    blake2b((void*)hdrkey, sizeof(hdrkey), (const void*)header, headerlen, 0, 0);

    keys->k0 = htole64(((uint64_t*)hdrkey)[0]);
    keys->k1 = htole64(((uint64_t*)hdrkey)[1]);
}

// generate edge endpoint in cuckoo graph without partition bit
uint32_t _sipnode(const siphash_keys* keys, uint32_t mask, uint32_t nonce, uint32_t uorv)
{
    return siphash24(keys, 2 * nonce + uorv) & mask;
}

// generate edge endpoint in cuckoo graph without partition bit
uint32_t _sipnode(const CSipHasher* hasher, uint32_t mask, uint32_t nonce, uint32_t uorv)
{
    return CSipHasher(*hasher).Write(2 * nonce + uorv).Finalize() & mask;
}

// generate edge endpoint in cuckoo graph
uint32_t sipnode(const CSipHasher* hasher, uint32_t mask, uint32_t nonce, uint32_t uorv)
{
    uint32_t node = _sipnode(hasher, mask, nonce, uorv);

    return node << 1 | uorv;
}

uint32_t sipnode(const siphash_keys* keys, uint32_t mask, uint32_t nonce, uint32_t uorv)
{
    uint32_t node = _sipnode(keys, mask, nonce, uorv);

    return node << 1 | uorv;
}

const char* errstr[] = {
    "OK",
    "wrong header length",
    "nonce too big",
    "nonces not ascending",
    "endpoints don't match up",
    "branch in cycle",
    "cycle dead ends",
    "cycle too short"};


CSipHasher::CSipHasher(uint64_t k0, uint64_t k1)
{
    v[0] = 0x736f6d6570736575ULL ^ k0;
    v[1] = 0x646f72616e646f6dULL ^ k1;
    v[2] = 0x6c7967656e657261ULL ^ k0;
    v[3] = 0x7465646279746573ULL ^ k1;
    count = 0;
    tmp = 0;
}

CSipHasher& CSipHasher::Write(uint64_t data)
{
    uint64_t v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];

    //assert(count % 8 == 0);

    v3 ^= data;
    SIPROUND;
    SIPROUND;
    v0 ^= data;

    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;

    count += 8;
    return *this;
}

CSipHasher& CSipHasher::Write(const unsigned char* data, size_t size)
{
    uint64_t v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];
    uint64_t t = tmp;
    int c = count;

    while (size--) {
        t |= ((uint64_t)(*(data++))) << (8 * (c % 8));
        c++;
        if ((c & 7) == 0) {
            v3 ^= t;
            SIPROUND;
            SIPROUND;
            v0 ^= t;
            t = 0;
        }
    }

    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
    count = c;
    tmp = t;

    return *this;
}

uint64_t CSipHasher::Finalize() const
{
    uint64_t v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];

    uint64_t t = tmp | (((uint64_t)count) << 56);

    v3 ^= t;
    SIPROUND;
    SIPROUND;
    v0 ^= t;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}

class CuckooCtx
{
public:
    CSipHasher* m_hasher;
    siphash_keys m_keys;
    uint32_t m_difficulty;
    uint32_t* m_cuckoo;

    CuckooCtx(const char* header, const uint32_t headerlen, uint32_t difficulty, uint32_t nodesCount)
    {
        setKeys(header, headerlen, &m_keys);
        m_hasher = new CSipHasher(m_keys.k0, m_keys.k1);

        m_difficulty = difficulty;
        m_cuckoo = (uint32_t*)calloc(1 + nodesCount, sizeof(uint32_t));

        //assert(m_cuckoo != 0);
    }

    ~CuckooCtx()
    {
        free(m_cuckoo);
    }
};

int path(uint32_t* cuckoo, uint32_t u, uint32_t* us)
{
    int nu;
    for (nu = 0; u; u = cuckoo[u]) {
        if (++nu >= MAXPATHLEN) {
            //LogPrintf("nu is %d\n", nu);
            while (nu-- && us[nu] != u)
                ;
            //if (nu < 0)
            //    LogPrintf("maximum path length exceeded\n");
            //else
            //    LogPrintf("illegal % 4d-cycle\n", MAXPATHLEN - nu);
            exit(0);
        }
        us[nu] = u;
    }
    return nu;
}

typedef std::pair<uint32_t, uint32_t> edge;

void solution(CuckooCtx* ctx, uint32_t* us, int nu, uint32_t* vs, int nv, std::set<uint32_t>& nonces, const uint32_t edgeMask)
{
    //assert(nonces.empty());
    std::set<edge> cycle;

    unsigned n;
    cycle.insert(edge(*us, *vs));
    while (nu--) {
        cycle.insert(edge(us[(nu + 1) & ~1], us[nu | 1])); // u's in even position; v's in odd
    }
    while (nv--) {
        cycle.insert(edge(vs[nv | 1], vs[(nv + 1) & ~1])); // u's in odd position; v's in even
    }

    for (uint32_t nonce = n = 0; nonce < ctx->m_difficulty; nonce++) {
        edge e(sipnode(&ctx->m_keys, edgeMask, nonce, 0), sipnode(&ctx->m_keys, edgeMask, nonce, 1));
        if (cycle.find(e) != cycle.end()) {
            // LogPrintf("%x ", nonce);
            cycle.erase(e);
            nonces.insert(nonce);
        }
    }
    // LogPrintf("\n");
}

