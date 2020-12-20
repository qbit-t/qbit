#ifndef QBIT_COOCKOO_H
#define QBIT_COOCKOO_H

#include "blake2/blake2.h"
#include "../uint256.h"
#include <utility>
#include <set>

#define MAXPATHLEN 8192
#define EDGEBITS 20 //20
#define PROOFSIZE 40 //42

enum verify_code {
    POW_OK,
    POW_HEADER_LENGTH,
    POW_TOO_BIG,
    POW_TOO_SMALL,
    POW_NON_MATCHING,
    POW_BRANCH,
    POW_DEAD_END,
    POW_SHORT_CYCLE
};

#if defined(_MSC_VER) && (MSC_VER <= 1500) && !defined(CYBOZU_DEFINED_INTXX)
	#define CYBOZU_DEFINED_INTXX
	typedef __int64 int64_t;
	typedef unsigned __int64 uint64_t;
	typedef unsigned int uint32_t;
	typedef int int32_t;
	typedef unsigned short uint16_t;
	typedef short int16_t;
	typedef unsigned char uint8_t;
	typedef signed char int8_t;
#else
	#include <stdint.h>
#endif

// siphash uses a pair of 64-bit keys,
typedef struct {
    uint64_t k0;
    uint64_t k1;
} siphash_keys;


#define ROTL(x,b) (uint64_t)( ((x) << (b)) | ( (x) >> (64 - (b))) )
#define SIPROUND \
  do { \
    v0 += v1; v2 += v3; v1 = ROTL(v1,13); \
    v3 = ROTL(v3,16); v1 ^= v0; v3 ^= v2; \
    v0 = ROTL(v0,32); v2 += v1; v0 += v3; \
    v1 = ROTL(v1,17);   v3 = ROTL(v3,21); \
    v1 ^= v2; v3 ^= v0; v2 = ROTL(v2,32); \
  } while(0)

// inline uint16_t bswap_16(uint16_t x)
// {
//     return (x >> 8) | (x << 8);
// }

// inline uint32_t bswap_32(uint32_t x)
// {
//     return (((x & 0xff000000U) >> 24) | ((x & 0x00ff0000U) >>  8) |
//             ((x & 0x0000ff00U) <<  8) | ((x & 0x000000ffU) << 24));
// }

// inline uint64_t bswap_64(uint64_t x)
// {
//      return (((x & 0xff00000000000000ull) >> 56)
//           | ((x & 0x00ff000000000000ull) >> 40)
//           | ((x & 0x0000ff0000000000ull) >> 24)
//           | ((x & 0x000000ff00000000ull) >> 8)
//           | ((x & 0x00000000ff000000ull) << 8)
//           | ((x & 0x0000000000ff0000ull) << 24)
//           | ((x & 0x000000000000ff00ull) << 40)
//           | ((x & 0x00000000000000ffull) << 56));
// }

// inline uint64_t htole64(uint64_t host_64bits)
// {
//     return bswap_64(host_64bits);
// }

/** SipHash-2-4 */
class CSipHasher
{
private:
    uint64_t v[4];
    uint64_t tmp;
    int count;

public:
    /** Construct a SipHash calculator initialized with 128-bit key (k0, k1) */
    CSipHasher(uint64_t k0, uint64_t k1);
    /** Hash a 64-bit integer worth of data
     *  It is treated as if this was the little-endian interpretation of 8 bytes.
     *  This function can only be used when a multiple of 8 bytes have been written so far.
     */
    CSipHasher& Write(uint64_t data);
    /** Hash arbitrary bytes. */
    CSipHasher& Write(const unsigned char* data, size_t size);
    /** Compute the 64-bit SipHash-2-4 of the data written so far. The object remains untouched. */
    uint64_t Finalize() const;
};


// SipHash-2-4 specialized to precomputed key and 8 byte nonces
uint64_t siphash24(const siphash_keys* keys, const uint64_t nonce);

// convenience function for extracting siphash keys from header
void setKeys(const char* header, const uint32_t headerlen, siphash_keys* keys);

// generate edge endpoint in cuckoo graph without partition bit
uint32_t _sipnode(const siphash_keys* keys, uint32_t mask, uint32_t nonce, uint32_t uorv);

// generate edge endpoint in cuckoo graph without partition bit
uint32_t _sipnode(const CSipHasher* hasher, uint32_t mask, uint32_t nonce, uint32_t uorv);

// generate edge endpoint in cuckoo graph
uint32_t sipnode(const CSipHasher* hasher, uint32_t mask, uint32_t nonce, uint32_t uorv);

uint32_t sipnode(const siphash_keys* keys, uint32_t mask, uint32_t nonce, uint32_t uorv);

// Find proofsize-length cuckoo cycle in random graph
bool FindCycle(const uint256& hash, uint8_t edgeBits, uint8_t proofSize, std::set<uint32_t>& cycle);

// verify that cycle is valid in block hash generated graph
int VerifyCycle(const uint256& hash, uint8_t edgeBits, uint8_t proofSize, const std::vector<uint32_t>& cycle);

#endif
