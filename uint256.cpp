#include "uint256.h"

#include "utilstrencodings.h"

#include <stdio.h>
#include <string.h>

template <unsigned int BITS>
base_blob<BITS>::base_blob(const std::vector<unsigned char>& vch)
{
    assert(vch.size() == sizeof(data));
    memcpy(data, &vch[0], sizeof(data));
}

template <unsigned int BITS>
base_blob<BITS>::base_blob(unsigned char* vch)
{
    set(vch);
}

template <unsigned int BITS>
void base_blob<BITS>::set(unsigned char *vch) {
    memcpy(data, vch, sizeof(data));   
}

template <unsigned int BITS>
std::string base_blob<BITS>::toHex() const
{
    char psz[sizeof(data) * 2 + 1];
    for (unsigned int i = 0; i < sizeof(data); i++)
        sprintf(psz + i * 2, "%02x", data[sizeof(data) - i - 1]);
    return std::string(psz, psz + sizeof(data) * 2);
}

template <unsigned int BITS>
void base_blob<BITS>::setHex(const char* psz)
{
    memset(data, 0, sizeof(data));

    // skip leading spaces
    while (isspace(*psz))
        psz++;

    // skip 0x
    if (psz[0] == '0' && tolower(psz[1]) == 'x')
        psz += 2;

    // hex string to uint
    const char* pbegin = psz;
    while (::HexDigit(*psz) != -1)
        psz++;
    psz--;
    unsigned char* p1 = (unsigned char*)data;
    unsigned char* pend = p1 + WIDTH;
    while (psz >= pbegin && p1 < pend) {
        *p1 = ::HexDigit(*psz--);
        if (psz >= pbegin) {
            *p1 |= ((unsigned char)::HexDigit(*psz--) << 4);
            p1++;
        }
    }
}

template <unsigned int BITS>
base_blob<BITS>& base_blob<BITS>::operator<<=(unsigned int shift)
{
    base_blob<BITS> a(*this);
    for (int i = 0; i < WIDTH; i++)
        data[i] = 0;
    int k = shift / 8;
    shift = shift % 8;
    for (int i = 0; i < WIDTH; i++) {
        if (i + k + 1 < WIDTH && shift != 0)
            data[i + k + 1] |= (a.data[i] >> (8 - shift));
        if (i + k < WIDTH)
            data[i + k] |= (a.data[i] << shift);
    }
    return *this;
}

template <unsigned int BITS>
base_blob<BITS>& base_blob<BITS>::operator>>=(unsigned int shift)
{
    base_blob<BITS> a(*this);
    for (int i = 0; i < WIDTH; i++)
        data[i] = 0;
    int k = shift / 8;
    shift = shift % 8;
    for (int i = 0; i < WIDTH; i++) {
        if (i - k - 1 >= 0 && shift != 0)
            data[i - k - 1] |= (a.data[i] << (8 - shift));
        if (i - k >= 0)
            data[i - k] |= (a.data[i] >> shift);
    }
    return *this;
}

template <unsigned int BITS>
void base_blob<BITS>::setHex(const std::string& str)
{
    setHex(str.c_str());
}

template <unsigned int BITS>
std::string base_blob<BITS>::toString() const
{
    return (toHex());
}

// Explicit instantiations for base_blob<160>
template base_blob<160>::base_blob(const std::vector<unsigned char>&);
template base_blob<160>::base_blob(unsigned char*);
template std::string base_blob<160>::toHex() const;
template std::string base_blob<160>::toString() const;
template void base_blob<160>::setHex(const char*);
template void base_blob<160>::setHex(const std::string&);
template void base_blob<160>::set(unsigned char*);

// Explicit instantiations for base_blob<256>
template base_blob<256>::base_blob(const std::vector<unsigned char>&);
template base_blob<256>::base_blob(unsigned char*);
template std::string base_blob<256>::toHex() const;
template std::string base_blob<256>::toString() const;
template void base_blob<256>::setHex(const char*);
template void base_blob<256>::setHex(const std::string&);
template void base_blob<256>::set(unsigned char*);

// Explicit instantiations for base_blob<512>
template base_blob<512>::base_blob(const std::vector<unsigned char>&);
template base_blob<512>::base_blob(unsigned char*);
template std::string base_blob<512>::toHex() const;
template std::string base_blob<512>::toString() const;
template void base_blob<512>::setHex(const char*);
template void base_blob<512>::setHex(const std::string&);
template void base_blob<512>::set(unsigned char*);

uint256& uint256::SetCompact(uint32_t nCompact, bool *pfNegative, bool *pfOverflow)
{
    int nSize = nCompact >> 24;
    uint32_t nWord = nCompact & 0x007fffff;
    if (nSize <= 3) {
        nWord >>= 8 * (3 - nSize);
        *this = nWord;
    } else {
        *this = nWord;
        *this <<= 8 * (nSize - 3);
    }
    if (pfNegative)
        *pfNegative = nWord != 0 && (nCompact & 0x00800000) != 0;
    if (pfOverflow)
        *pfOverflow = nWord != 0 && ((nSize > 34) ||
                                     (nWord > 0xff && nSize > 33) ||
                                     (nWord > 0xffff && nSize > 32));
    return *this;
}

uint32_t uint256::GetCompact(bool fNegative) const
{
    return 0;
}