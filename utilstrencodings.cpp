// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "utilstrencodings.h"

#include "tinyformat.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <limits>

static const std::string CHARS_ALPHA_NUM = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static const std::string SAFE_CHARS[] =
{
	CHARS_ALPHA_NUM + " .,;-_/:?@()", // SAFE_CHARS_DEFAULT
	CHARS_ALPHA_NUM + " .,;-_?@", // SAFE_CHARS_UA_COMMENT
	CHARS_ALPHA_NUM + ".-_", // SAFE_CHARS_FILENAME
	CHARS_ALPHA_NUM + "!*'();:@&=+$,/?#[]-_.~%", // SAFE_CHARS_URI
};

std::string SanitizeString(const std::string& str, int rule)
{
	std::string strResult;
	for (std::string::size_type i = 0; i < str.size(); i++)
	{
		if (SAFE_CHARS[rule].find(str[i]) != std::string::npos)
			strResult.push_back(str[i]);
	}
	return strResult;
}

const signed char p_util_hexdigit[256] =
{ -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, };

signed char HexDigit(char c)
{
	return p_util_hexdigit[(unsigned char)c];
}

bool IsHex(const std::string& str)
{
	for(std::string::const_iterator it(str.begin()); it != str.end(); ++it)
	{
		if (HexDigit(*it) < 0)
			return false;
	}
	return (str.size() > 0) && (str.size()%2 == 0);
}

bool IsHexNumber(const std::string& str)
{
	size_t starting_location = 0;
	if (str.size() > 2 && *str.begin() == '0' && *(str.begin()+1) == 'x') {
		starting_location = 2;
	}
	for (const char c : str.substr(starting_location)) {
		if (HexDigit(c) < 0) return false;
	}
	// Return false for empty string or "0x".
	return (str.size() > starting_location);
}

std::vector<unsigned char> ParseHex(const char* psz)
{
	// convert hex dump to vector
	std::vector<unsigned char> vch;
	while (true)
	{
		while (IsSpace(*psz))
			psz++;
		signed char c = HexDigit(*psz++);
		if (c == (signed char)-1)
			break;
		unsigned char n = (c << 4);
		c = HexDigit(*psz++);
		if (c == (signed char)-1)
			break;
		n |= c;
		vch.push_back(n);
	}
	return vch;
}

std::vector<unsigned char> ParseHex(const std::string& str)
{
	return ParseHex(str.c_str());
}

void SplitHostPort(std::string in, int &portOut, std::string &hostOut) {
	size_t colon = in.find_last_of(':');
	// if a : is found, and it either follows a [...], or no other : is in the string, treat it as port separator
	bool fHaveColon = colon != in.npos;
	bool fBracketed = fHaveColon && (in[0]=='[' && in[colon-1]==']'); // if there is a colon, and in[0]=='[', colon is not 0, so in[colon-1] is safe
	bool fMultiColon = fHaveColon && (in.find_last_of(':',colon-1) != in.npos);
	if (fHaveColon && (colon==0 || fBracketed || !fMultiColon)) {
		int32_t n;
		if (ParseInt32(in.substr(colon + 1), &n) && n > 0 && n < 0x10000) {
			in = in.substr(0, colon);
			portOut = n;
		}
	}
	if (in.size()>0 && in[0] == '[' && in[in.size()-1] == ']')
		hostOut = in.substr(1, in.size()-2);
	else
		hostOut = in;
}

std::string EncodeBase64(const unsigned char* pch, size_t len)
{
	static const char *pbase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	std::string str;
	str.reserve(((len + 2) / 3) * 4);
	ConvertBits<8, 6, true>([&](int v) { str += pbase64[v]; }, pch, pch + len);
	while (str.size() % 4) str += '=';
	return str;
}

std::string EncodeBase64(const std::string& str)
{
	return EncodeBase64((const unsigned char*)str.c_str(), str.size());
}

std::vector<unsigned char> DecodeBase64(const char* p, bool* pf_invalid)
{
	static const int decode64_table[256] =
	{
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1,
		-1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
		29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
		49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	};

	const char* e = p;
	std::vector<uint8_t> val;
	val.reserve(strlen(p));
	while (*p != 0) {
		int x = decode64_table[(unsigned char)*p];
		if (x == -1) break;
		val.push_back(x);
		++p;
	}

	std::vector<unsigned char> ret;
	ret.reserve((val.size() * 3) / 4);
	bool valid = ConvertBits<6, 8, false>([&](unsigned char c) { ret.push_back(c); }, val.begin(), val.end());

	const char* q = p;
	while (valid && *p != 0) {
		if (*p != '=') {
			valid = false;
			break;
		}
		++p;
	}
	valid = valid && (p - e) % 4 == 0 && p - q < 4;
	if (pf_invalid) *pf_invalid = !valid;

	return ret;
}

std::string DecodeBase64(const std::string& str, bool* pf_invalid)
{
	std::vector<unsigned char> vchRet = DecodeBase64(str.c_str(), pf_invalid);
	return std::string((const char*)vchRet.data(), vchRet.size());
}

std::string EncodeBase32(const unsigned char* pch, size_t len)
{
	static const char *pbase32 = "abcdefghijklmnopqrstuvwxyz234567";

	std::string str;
	str.reserve(((len + 4) / 5) * 8);
	ConvertBits<8, 5, true>([&](int v) { str += pbase32[v]; }, pch, pch + len);
	while (str.size() % 8) str += '=';
	return str;
}

std::string EncodeBase32(const std::string& str)
{
	return EncodeBase32((const unsigned char*)str.c_str(), str.size());
}

std::vector<unsigned char> DecodeBase32(const char* p, bool* pf_invalid)
{
	static const int decode32_table[256] =
	{
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1,
		-1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1,  0,  1,  2,
		 3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
		23, 24, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	};

	const char* e = p;
	std::vector<uint8_t> val;
	val.reserve(strlen(p));
	while (*p != 0) {
		int x = decode32_table[(unsigned char)*p];
		if (x == -1) break;
		val.push_back(x);
		++p;
	}

	std::vector<unsigned char> ret;
	ret.reserve((val.size() * 5) / 8);
	bool valid = ConvertBits<5, 8, false>([&](unsigned char c) { ret.push_back(c); }, val.begin(), val.end());

	const char* q = p;
	while (valid && *p != 0) {
		if (*p != '=') {
			valid = false;
			break;
		}
		++p;
	}
	valid = valid && (p - e) % 8 == 0 && p - q < 8;
	if (pf_invalid) *pf_invalid = !valid;

	return ret;
}

std::string DecodeBase32(const std::string& str, bool* pf_invalid)
{
	std::vector<unsigned char> vchRet = DecodeBase32(str.c_str(), pf_invalid);
	return std::string((const char*)vchRet.data(), vchRet.size());
}

NODISCARD static bool ParsePrechecks(const std::string& str)
{
	if (str.empty()) // No empty string allowed
		return false;
	if (str.size() >= 1 && (IsSpace(str[0]) || IsSpace(str[str.size()-1]))) // No padding allowed
		return false;
	if (str.size() != strlen(str.c_str())) // No embedded NUL characters allowed
		return false;
	return true;
}

bool ParseInt32(const std::string& str, int32_t *out)
{
	if (!ParsePrechecks(str))
		return false;
	char *endp = nullptr;
	errno = 0; // strtol will not set errno if valid
	long int n = strtol(str.c_str(), &endp, 10);
	if(out) *out = (int32_t)n;
	// Note that strtol returns a *long int*, so even if strtol doesn't report an over/underflow
	// we still have to check that the returned value is within the range of an *int32_t*. On 64-bit
	// platforms the size of these types may be different.
	return endp && *endp == 0 && !errno &&
		n >= std::numeric_limits<int32_t>::min() &&
		n <= std::numeric_limits<int32_t>::max();
}

bool ParseInt64(const std::string& str, int64_t *out)
{
	if (!ParsePrechecks(str))
		return false;
	char *endp = nullptr;
	errno = 0; // strtoll will not set errno if valid
	long long int n = strtoll(str.c_str(), &endp, 10);
	if(out) *out = (int64_t)n;
	// Note that strtoll returns a *long long int*, so even if strtol doesn't report an over/underflow
	// we still have to check that the returned value is within the range of an *int64_t*.
	return endp && *endp == 0 && !errno &&
		n >= std::numeric_limits<int64_t>::min() &&
		n <= std::numeric_limits<int64_t>::max();
}

bool ParseUInt32(const std::string& str, uint32_t *out)
{
	if (!ParsePrechecks(str))
		return false;
	if (str.size() >= 1 && str[0] == '-') // Reject negative values, unfortunately strtoul accepts these by default if they fit in the range
		return false;
	char *endp = nullptr;
	errno = 0; // strtoul will not set errno if valid
	unsigned long int n = strtoul(str.c_str(), &endp, 10);
	if(out) *out = (uint32_t)n;
	// Note that strtoul returns a *unsigned long int*, so even if it doesn't report an over/underflow
	// we still have to check that the returned value is within the range of an *uint32_t*. On 64-bit
	// platforms the size of these types may be different.
	return endp && *endp == 0 && !errno &&
		n <= std::numeric_limits<uint32_t>::max();
}

bool ParseUInt64(const std::string& str, uint64_t *out)
{
	if (!ParsePrechecks(str))
		return false;
	if (str.size() >= 1 && str[0] == '-') // Reject negative values, unfortunately strtoull accepts these by default if they fit in the range
		return false;
	char *endp = nullptr;
	errno = 0; // strtoull will not set errno if valid
	unsigned long long int n = strtoull(str.c_str(), &endp, 10);
	if(out) *out = (uint64_t)n;
	// Note that strtoull returns a *unsigned long long int*, so even if it doesn't report an over/underflow
	// we still have to check that the returned value is within the range of an *uint64_t*.
	return endp && *endp == 0 && !errno &&
		n <= std::numeric_limits<uint64_t>::max();
}


bool ParseDouble(const std::string& str, double *out)
{
	if (!ParsePrechecks(str))
		return false;
	if (str.size() >= 2 && str[0] == '0' && str[1] == 'x') // No hexadecimal floats allowed
		return false;
	std::istringstream text(str);
	text.imbue(std::locale::classic());
	double result;
	text >> result;
	if(out) *out = result;
	return text.eof() && !text.fail();
}

std::string FormatParagraph(const std::string& in, size_t width, size_t indent)
{
	std::stringstream out;
	size_t ptr = 0;
	size_t indented = 0;
	while (ptr < in.size())
	{
		size_t lineend = in.find_first_of('\n', ptr);
		if (lineend == std::string::npos) {
			lineend = in.size();
		}
		const size_t linelen = lineend - ptr;
		const size_t rem_width = width - indented;
		if (linelen <= rem_width) {
			out << in.substr(ptr, linelen + 1);
			ptr = lineend + 1;
			indented = 0;
		} else {
			size_t finalspace = in.find_last_of(" \n", ptr + rem_width);
			if (finalspace == std::string::npos || finalspace < ptr) {
				// No place to break; just include the entire word and move on
				finalspace = in.find_first_of("\n ", ptr);
				if (finalspace == std::string::npos) {
					// End of the string, just add it and break
					out << in.substr(ptr);
					break;
				}
			}
			out << in.substr(ptr, finalspace - ptr) << "\n";
			if (in[finalspace] == '\n') {
				indented = 0;
			} else if (indent) {
				out << std::string(indent, ' ');
				indented = indent;
			}
			ptr = finalspace + 1;
		}
	}
	return out.str();
}

std::string i64tostr(int64_t n)
{
	return strprintf("%d", n);
}

std::string itostr(int n)
{
	return strprintf("%d", n);
}

int64_t atoi64(const char* psz)
{
#ifdef _MSC_VER
	return _atoi64(psz);
#else
	return strtoll(psz, nullptr, 10);
#endif
}

int64_t atoi64(const std::string& str)
{
#ifdef _MSC_VER
	return _atoi64(str.c_str());
#else
	return strtoll(str.c_str(), nullptr, 10);
#endif
}

int atoi(const std::string& str)
{
	return atoi(str.c_str());
}

/** Upper bound for mantissa.
 * 10^18-1 is the largest arbitrary decimal that will fit in a signed 64-bit integer.
 * Larger integers cannot consist of arbitrary combinations of 0-9:
 *
 *   999999999999999999  1^18-1
 *  9223372036854775807  (1<<63)-1  (max int64_t)
 *  9999999999999999999  1^19-1     (would overflow)
 */
static const int64_t UPPER_BOUND = 1000000000000000000LL - 1LL;

/** Helper function for ParseFixedPoint */
static inline bool ProcessMantissaDigit(char ch, int64_t &mantissa, int &mantissa_tzeros)
{
	if(ch == '0')
		++mantissa_tzeros;
	else {
		for (int i=0; i<=mantissa_tzeros; ++i) {
			if (mantissa > (UPPER_BOUND / 10LL))
				return false; /* overflow */
			mantissa *= 10;
		}
		mantissa += ch - '0';
		mantissa_tzeros = 0;
	}
	return true;
}

bool ParseFixedPoint(const std::string &val, int decimals, int64_t *amount_out)
{
	int64_t mantissa = 0;
	int64_t exponent = 0;
	int mantissa_tzeros = 0;
	bool mantissa_sign = false;
	bool exponent_sign = false;
	int ptr = 0;
	int end = val.size();
	int point_ofs = 0;

	if (ptr < end && val[ptr] == '-') {
		mantissa_sign = true;
		++ptr;
	}
	if (ptr < end)
	{
		if (val[ptr] == '0') {
			/* pass single 0 */
			++ptr;
		} else if (val[ptr] >= '1' && val[ptr] <= '9') {
			while (ptr < end && IsDigit(val[ptr])) {
				if (!ProcessMantissaDigit(val[ptr], mantissa, mantissa_tzeros))
					return false; /* overflow */
				++ptr;
			}
		} else return false; /* missing expected digit */
	} else return false; /* empty string or loose '-' */
	if (ptr < end && val[ptr] == '.')
	{
		++ptr;
		if (ptr < end && IsDigit(val[ptr]))
		{
			while (ptr < end && IsDigit(val[ptr])) {
				if (!ProcessMantissaDigit(val[ptr], mantissa, mantissa_tzeros))
					return false; /* overflow */
				++ptr;
				++point_ofs;
			}
		} else return false; /* missing expected digit */
	}
	if (ptr < end && (val[ptr] == 'e' || val[ptr] == 'E'))
	{
		++ptr;
		if (ptr < end && val[ptr] == '+')
			++ptr;
		else if (ptr < end && val[ptr] == '-') {
			exponent_sign = true;
			++ptr;
		}
		if (ptr < end && IsDigit(val[ptr])) {
			while (ptr < end && IsDigit(val[ptr])) {
				if (exponent > (UPPER_BOUND / 10LL))
					return false; /* overflow */
				exponent = exponent * 10 + val[ptr] - '0';
				++ptr;
			}
		} else return false; /* missing expected digit */
	}
	if (ptr != end)
		return false; /* trailing garbage */

	/* finalize exponent */
	if (exponent_sign)
		exponent = -exponent;
	exponent = exponent - point_ofs + mantissa_tzeros;

	/* finalize mantissa */
	if (mantissa_sign)
		mantissa = -mantissa;

	/* convert to one 64-bit fixed-point value */
	exponent += decimals;
	if (exponent < 0)
		return false; /* cannot represent values smaller than 10^-decimals */
	if (exponent >= 18)
		return false; /* cannot represent values larger than or equal to 10^(18-decimals) */

	for (int i=0; i < exponent; ++i) {
		if (mantissa > (UPPER_BOUND / 10LL) || mantissa < -(UPPER_BOUND / 10LL))
			return false; /* overflow */
		mantissa *= 10;
	}
	if (mantissa > UPPER_BOUND || mantissa < -UPPER_BOUND)
		return false; /* overflow */

	if (amount_out)
		*amount_out = mantissa;

	return true;
}

void Downcase(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](char c){return ToLower(c);});
}

std::string Capitalize(std::string str)
{
	if (str.empty()) return str;
	str[0] = ToUpper(str.front());
	return str;
}

unsigned char* UTFStringToLowerCase(unsigned char* pString)
{
unsigned char* p = pString;
unsigned char* pExtChar = 0;

if (pString && *pString) {
		while (*p) {
			if ((*p >= 0x41) && (*p <= 0x5a)) /* US ASCII */
				(*p) += 0x20;
			else if (*p > 0xc0) {
				pExtChar = p;
				p++;
				switch (*pExtChar) {
				case 0xc3: /* Latin 1 */
					if ((*p >= 0x80)
						&& (*p <= 0x9e)
						&& (*p != 0x97))
						(*p) += 0x20; /* US ASCII shift */
					break;
				case 0xc4: /* Latin ext */
					if (((*p >= 0x80)
						&& (*p <= 0xb7)
						&& (*p != 0xb0))
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					else if ((*p >= 0xb9)
						&& (*p <= 0xbe)
						&& (*p % 2)) /* Odd */
						(*p)++; /* Next char is lwr */
					else if (*p == 0xbf) {
						*pExtChar = 0xc5;
						(*p) = 0x80;
					}
					break;
				case 0xc5: /* Latin ext */
					if ((*p >= 0x81)
						&& (*p <= 0x88)
						&& (*p % 2)) /* Odd */
						(*p)++; /* Next char is lwr */
					else if ((*p >= 0x8a)
						&& (*p <= 0xb7)
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					else if (*p == 0xb8) {
						*pExtChar = 0xc3;
						(*p) = 0xbf;
					}
					else if ((*p >= 0xb9)
						&& (*p <= 0xbe)
						&& (*p % 2)) /* Odd */
						(*p)++; /* Next char is lwr */
					break;
				case 0xc6: /* Latin ext */
					switch (*p) {
					case 0x81:
						*pExtChar = 0xc9;
						(*p) = 0x93;
						break;
					case 0x86:
						*pExtChar = 0xc9;
						(*p) = 0x94;
						break;
					case 0x89:
						*pExtChar = 0xc9;
						(*p) = 0x96;
						break;
					case 0x8a:
						*pExtChar = 0xc9;
						(*p) = 0x97;
						break;
					case 0x8e:
						*pExtChar = 0xc9;
						(*p) = 0x98;
						break;
					case 0x8f:
						*pExtChar = 0xc9;
						(*p) = 0x99;
						break;
					case 0x90:
						*pExtChar = 0xc9;
						(*p) = 0x9b;
						break;
					case 0x93:
						*pExtChar = 0xc9;
						(*p) = 0xa0;
						break;
					case 0x94:
						*pExtChar = 0xc9;
						(*p) = 0xa3;
						break;
					case 0x96:
						*pExtChar = 0xc9;
						(*p) = 0xa9;
						break;
					case 0x97:
						*pExtChar = 0xc9;
						(*p) = 0xa8;
						break;
					case 0x9c:
						*pExtChar = 0xc9;
						(*p) = 0xaf;
						break;
					case 0x9d:
						*pExtChar = 0xc9;
						(*p) = 0xb2;
						break;
					case 0x9f:
						*pExtChar = 0xc9;
						(*p) = 0xb5;
						break;
					case 0xa9:
						*pExtChar = 0xca;
						(*p) = 0x83;
						break;
					case 0xae:
						*pExtChar = 0xca;
						(*p) = 0x88;
						break;
					case 0xb1:
						*pExtChar = 0xca;
						(*p) = 0x8a;
						break;
					case 0xb2:
						*pExtChar = 0xca;
						(*p) = 0x8b;
						break;
					case 0xb7:
						*pExtChar = 0xca;
						(*p) = 0x92;
						break;
					case 0x82:
					case 0x84:
					case 0x87:
					case 0x8b:
					case 0x91:
					case 0x98:
					case 0xa0:
					case 0xa2:
					case 0xa4:
					case 0xa7:
					case 0xac:
					case 0xaf:
					case 0xb3:
					case 0xb5:
					case 0xb8:
					case 0xbc:
						(*p)++; /* Next char is lwr */
						break;
					default:
						break;
					}
					break;
				case 0xc7: /* Latin ext */
					if (*p == 0x84)
						(*p) = 0x86;
					else if (*p == 0x85)
						(*p)++; /* Next char is lwr */
					else if (*p == 0x87)
						(*p) = 0x89;
					else if (*p == 0x88)
						(*p)++; /* Next char is lwr */
					else if (*p == 0x8a)
						(*p) = 0x8c;
					else if (*p == 0x8b)
						(*p)++; /* Next char is lwr */
					else if ((*p >= 0x8d)
						&& (*p <= 0x9c)
						&& (*p % 2)) /* Odd */
						(*p)++; /* Next char is lwr */
					else if ((*p >= 0x9e)
						&& (*p <= 0xaf)
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					else if (*p == 0xb1)
						(*p) = 0xb3;
					else if (*p == 0xb2)
						(*p)++; /* Next char is lwr */
					else if (*p == 0xb4)
						(*p)++; /* Next char is lwr */
					else if (*p == 0xb6) {
						*pExtChar = 0xc6;
						(*p) = 0x95;
					}
					else if (*p == 0xb7) {
						*pExtChar = 0xc6;
						(*p) = 0xbf;
					}
					else if ((*p >= 0xb8)
						&& (*p <= 0xbf)
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					break;
				case 0xc8: /* Latin ext */
					if ((*p >= 0x80)
						&& (*p <= 0x9f)
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					else if (*p == 0xa0) {
						*pExtChar = 0xc6;
						(*p) = 0x9e;
					}
					else if ((*p >= 0xa2)
						&& (*p <= 0xb3)
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					else if (*p == 0xbb)
						(*p)++; /* Next char is lwr */
					else if (*p == 0xbd) {
						*pExtChar = 0xc6;
						(*p) = 0x9a;
					}
					/* 0xba three byte small 0xe2 0xb1 0xa5 */
					/* 0xbe three byte small 0xe2 0xb1 0xa6 */
					break;
				case 0xc9: /* Latin ext */
					if (*p == 0x81)
						(*p)++; /* Next char is lwr */
					else if (*p == 0x83) {
						*pExtChar = 0xc6;
						(*p) = 0x80;
					}
					else if (*p == 0x84) {
						*pExtChar = 0xca;
						(*p) = 0x89;
					}
					else if (*p == 0x85) {
						*pExtChar = 0xca;
						(*p) = 0x8c;
					}
					else if ((*p >= 0x86)
						&& (*p <= 0x8f)
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					break;
				case 0xcd: /* Greek & Coptic */
					switch (*p) {
					case 0xb0:
					case 0xb2:
					case 0xb6:
						(*p)++; /* Next char is lwr */
						break;
					case 0xbf:
						*pExtChar = 0xcf;
						(*p) = 0xb3;
						break;
					default:
						break;
					}
					break;
				case 0xce: /* Greek & Coptic */
					if (*p == 0x86)
						(*p) = 0xac;
					else if (*p == 0x88)
						(*p) = 0xad;
					else if (*p == 0x89)
						(*p) = 0xae;
					else if (*p == 0x8a)
						(*p) = 0xaf;
					else if (*p == 0x8c) {
						*pExtChar = 0xcf;
						(*p) = 0x8c;
					}
					else if (*p == 0x8e) {
						*pExtChar = 0xcf;
						(*p) = 0x8d;
					}
					else if (*p == 0x8f) {
						*pExtChar = 0xcf;
						(*p) = 0x8e;
					}
					else if ((*p >= 0x91)
						&& (*p <= 0x9f))
						(*p) += 0x20; /* US ASCII shift */
					else if ((*p >= 0xa0)
						&& (*p <= 0xab)
						&& (*p != 0xa2)) {
						*pExtChar = 0xcf;
						(*p) -= 0x20;
					}
					break;
				case 0xcf: /* Greek & Coptic */
					if (*p == 0x8f)
						(*p) = 0x97;
					else if ((*p >= 0x98)
						&& (*p <= 0xaf)
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					else if (*p == 0xb4) {
						(*p) = 0x91;
					}
					else if (*p == 0xb7)
						(*p)++; /* Next char is lwr */
					else if (*p == 0xb9)
						(*p) = 0xb2;
					else if (*p == 0xba)
						(*p)++; /* Next char is lwr */
					else if (*p == 0xbd) {
						*pExtChar = 0xcd;
						(*p) = 0xbb;
					}
					else if (*p == 0xbe) {
						*pExtChar = 0xcd;
						(*p) = 0xbc;
					}
					else if (*p == 0xbf) {
						*pExtChar = 0xcd;
						(*p) = 0xbd;
					}
					break;
				case 0xd0: /* Cyrillic */
					if ((*p >= 0x80)
						&& (*p <= 0x8f)) {
						*pExtChar = 0xd1;
						(*p) += 0x10;
					}
					else if ((*p >= 0x90)
						&& (*p <= 0x9f))
						(*p) += 0x20; /* US ASCII shift */
					else if ((*p >= 0xa0)
						&& (*p <= 0xaf)) {
						*pExtChar = 0xd1;
						(*p) -= 0x20;
					}
					break;
				case 0xd1: /* Cyrillic supplement */
					if ((*p >= 0xa0)
						&& (*p <= 0xbf)
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					break;
				case 0xd2: /* Cyrillic supplement */
					if (*p == 0x80)
						(*p)++; /* Next char is lwr */
					else if ((*p >= 0x8a)
						&& (*p <= 0xbf)
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					break;
				case 0xd3: /* Cyrillic supplement */
					if (*p == 0x80)
						(*p) = 0x8f;
					else if ((*p >= 0x81)
						&& (*p <= 0x8e)
						&& (*p % 2)) /* Odd */
						(*p)++; /* Next char is lwr */
					else if ((*p >= 0x90)
						&& (*p <= 0xbf)
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					break;
				case 0xd4: /* Cyrillic supplement & Armenian */
					if ((*p >= 0x80)
						&& (*p <= 0xaf)
						&& (!(*p % 2))) /* Even */
						(*p)++; /* Next char is lwr */
					else if ((*p >= 0xb1)
						&& (*p <= 0xbf)) {
						*pExtChar = 0xd5;
						(*p) -= 0x10;
					}
					break;
				case 0xd5: /* Armenian */
					if ((*p >= 0x80)
						&& (*p <= 0x8f)) {
						(*p) += 0x30;
					}
					else if ((*p >= 0x90)
						&& (*p <= 0x96)) {
						*pExtChar = 0xd6;
						(*p) -= 0x10;
					}
					break;
				case 0xe1: /* Three byte code */
					pExtChar = p;
					p++;
					switch (*pExtChar) {
					case 0x82: /* Georgian asomtavruli */
						if ((*p >= 0xa0)
							&& (*p <= 0xbf)) {
							*pExtChar = 0x83;
							(*p) -= 0x10;
						}
						break;
					case 0x83: /* Georgian asomtavruli */
						if (((*p >= 0x80)
							&& (*p <= 0x85))
							|| (*p == 0x87)
							|| (*p == 0x8d))
							(*p) += 0x30;
						break;
					case 0x8e: /* Cherokee */
						if ((*p >= 0xa0)
							&& (*p <= 0xaf)) {
							*(p - 2) = 0xea;
							*pExtChar = 0xad;
							(*p) += 0x10;
						}
						else if ((*p >= 0xb0)
							&& (*p <= 0xbf)) {
							*(p - 2) = 0xea;
							*pExtChar = 0xae;
							(*p) -= 0x30;
						}
						break;
					case 0x8f: /* Cherokee */
						if ((*p >= 0x80)
							&& (*p <= 0xaf)) {
							*(p - 2) = 0xea;
							*pExtChar = 0xae;
							(*p) += 0x10;
						}
						else if ((*p >= 0xb0)
							&& (*p <= 0xb5)) {
							(*p) += 0x08;
						}
						/* 0xbe three byte small 0xe2 0xb1 0xa6 */
						break;
					case 0xb2: /* Georgian mtavruli */
						if (((*p >= 0x90)
							&& (*p <= 0xba))
							|| (*p == 0xbd)
							|| (*p == 0xbe)
							|| (*p == 0xbf))
							*pExtChar = 0x83;
						break;
					case 0xb8: /* Latin ext */
						if ((*p >= 0x80)
							&& (*p <= 0xbf)
							&& (!(*p % 2))) /* Even */
							(*p)++; /* Next char is lwr */
						break;
					case 0xb9: /* Latin ext */
						if ((*p >= 0x80)
							&& (*p <= 0xbf)
							&& (!(*p % 2))) /* Even */
							(*p)++; /* Next char is lwr */
						break;
					case 0xba: /* Latin ext */
						if ((*p >= 0x80)
							&& (*p <= 0x94)
							&& (!(*p % 2))) /* Even */
							(*p)++; /* Next char is lwr */
						else if ((*p >= 0xa0)
							&& (*p <= 0xbf)
							&& (!(*p % 2))) /* Even */
							(*p)++; /* Next char is lwr */
						/* 0x9e Two byte small 0xc3 0x9f */
						break;
					case 0xbb: /* Latin ext */
						if ((*p >= 0x80)
							&& (*p <= 0xbf)
							&& (!(*p % 2))) /* Even */
							(*p)++; /* Next char is lwr */
						break;
					case 0xbc: /* Greek ex */
						if ((*p >= 0x88)
							&& (*p <= 0x8f))
							(*p) -= 0x08;
						else if ((*p >= 0x98)
							&& (*p <= 0x9d))
							(*p) -= 0x08;
						else if ((*p >= 0xa8)
							&& (*p <= 0xaf))
							(*p) -= 0x08;
						else if ((*p >= 0xb8)
							&& (*p <= 0xbf))
							(*p) -= 0x08;
						break;
					case 0xbd: /* Greek ex */
						if ((*p >= 0x88)
							&& (*p <= 0x8d))
							(*p) -= 0x08;
						else if ((*p == 0x99)
							|| (*p == 0x9b)
							|| (*p == 0x9d)
							|| (*p == 0x9f))
							(*p) -= 0x08;
						else if ((*p >= 0xa8)
							&& (*p <= 0xaf))
							(*p) -= 0x08;
						break;
					case 0xbe: /* Greek ex */
						if ((*p >= 0x88)
							&& (*p <= 0x8f))
							(*p) -= 0x08;
						else if ((*p >= 0x98)
							&& (*p <= 0x9f))
							(*p) -= 0x08;
						else if ((*p >= 0xa8)
							&& (*p <= 0xaf))
							(*p) -= 0x08;
						else if ((*p >= 0xb8)
							&& (*p <= 0xb9))
							(*p) -= 0x08;
						else if ((*p >= 0xba)
							&& (*p <= 0xbb)) {
							*(p - 1) = 0xbd;
							(*p) -= 0x0a;
						}
						else if (*p == 0xbc)
							(*p) -= 0x09;
						break;
					case 0xbf: /* Greek ex */
						if ((*p >= 0x88)
							&& (*p <= 0x8b)) {
							*(p - 1) = 0xbd;
							(*p) += 0x2a;
						}
						else if (*p == 0x8c)
							(*p) -= 0x09;
						else if ((*p >= 0x98)
							&& (*p <= 0x99))
							(*p) -= 0x08;
						else if ((*p >= 0x9a)
							&& (*p <= 0x9b)) {
							*(p - 1) = 0xbd;
							(*p) += 0x1c;
						}
						else if ((*p >= 0xa8)
							&& (*p <= 0xa9))
							(*p) -= 0x08;
						else if ((*p >= 0xaa)
							&& (*p <= 0xab)) {
							*(p - 1) = 0xbd;
							(*p) += 0x10;
						}
						else if (*p == 0xac)
							(*p) -= 0x07;
						else if ((*p >= 0xb8)
							&& (*p <= 0xb9)) {
							*(p - 1) = 0xbd;
						}
						else if ((*p >= 0xba)
							&& (*p <= 0xbb)) {
							*(p - 1) = 0xbd;
							(*p) += 0x02;
						}
						else if (*p == 0xbc)
							(*p) -= 0x09;
						break;
					default:
						break;
					}
					break;
				case 0xe2: /* Three byte code */
					pExtChar = p;
					p++;
					switch (*pExtChar) {
					case 0xb0: /* Glagolitic */
						if ((*p >= 0x80)
							&& (*p <= 0x8f)) {
							(*p) += 0x30;
						}
						else if ((*p >= 0x90)
							&& (*p <= 0xae)) {
							*pExtChar = 0xb1;
							(*p) -= 0x10;
						}
						break;
					case 0xb1: /* Latin ext */
						switch (*p) {
						case 0xa0:
						case 0xa7:
						case 0xa9:
						case 0xab:
						case 0xb2:
						case 0xb5:
							(*p)++; /* Next char is lwr */
							break;
						case 0xa2: /* Two byte small 0xc9 0xab */
						case 0xa4: /* Two byte small 0xc9 0xbd */
						case 0xad: /* Two byte small 0xc9 0x91 */
						case 0xae: /* Two byte small 0xc9 0xb1 */
						case 0xaf: /* Two byte small 0xc9 0x90 */
						case 0xb0: /* Two byte small 0xc9 0x92 */
						case 0xbe: /* Two byte small 0xc8 0xbf */
						case 0xbf: /* Two byte small 0xc9 0x80 */
							break;
						case 0xa3:
							*(p - 2) = 0xe1;
							*(p - 1) = 0xb5;
							*(p) = 0xbd;
							break;
						default:
							break;
						}
						break;
					case 0xb2: /* Coptic */
						if ((*p >= 0x80)
							&& (*p <= 0xbf)
							&& (!(*p % 2))) /* Even */
							(*p)++; /* Next char is lwr */
						break;
					case 0xb3: /* Coptic */
						if (((*p >= 0x80)
							&& (*p <= 0xa3)
							&& (!(*p % 2))) /* Even */
							|| (*p == 0xab)
							|| (*p == 0xad)
							|| (*p == 0xb2))
							(*p)++; /* Next char is lwr */
						break;
					case 0xb4: /* Georgian nuskhuri */
						if (((*p >= 0x80)
							&& (*p <= 0xa5))
							|| (*p == 0xa7)
							|| (*p == 0xad)) {
							*(p - 2) = 0xe1;
							*(p - 1) = 0x83;
							(*p) += 0x10;
						}
						break;
					default:
						break;
					}
					break;
				case 0xea: /* Three byte code */
					pExtChar = p;
					p++;
					switch (*pExtChar) {
					case 0x99: /* Cyrillic */
						if ((*p >= 0x80)
							&& (*p <= 0xad)
							&& (!(*p % 2))) /* Even */
							(*p)++; /* Next char is lwr */
						break;
					case 0x9a: /* Cyrillic */
						if ((*p >= 0x80)
							&& (*p <= 0x9b)
							&& (!(*p % 2))) /* Even */
							(*p)++; /* Next char is lwr */
						break;
					case 0x9c: /* Latin ext */
						if ((((*p >= 0xa2)
							&& (*p <= 0xaf))
							|| ((*p >= 0xb2)
								&& (*p <= 0xbf)))
							&& (!(*p % 2))) /* Even */
							(*p)++; /* Next char is lwr */
						break;
					case 0x9d: /* Latin ext */
						if ((((*p >= 0x80)
							&& (*p <= 0xaf))
							&& (!(*p % 2))) /* Even */
							|| (*p == 0xb9)
							|| (*p == 0xbb)
							|| (*p == 0xbe))
							(*p)++; /* Next char is lwr */
						else if (*p == 0xbd) {
							*(p - 2) = 0xe1;
							*(p - 1) = 0xb5;
							*(p) = 0xb9;
						}
						break;
					case 0x9e: /* Latin ext */
						if (((((*p >= 0x80)
							&& (*p <= 0x87))
							|| ((*p >= 0x96)
								&& (*p <= 0xa9))
							|| ((*p >= 0xb4)
								&& (*p <= 0xbf)))
							&& (!(*p % 2))) /* Even */
							|| (*p == 0x8b)
							|| (*p == 0x90)
							|| (*p == 0x92))
							(*p)++; /* Next char is lwr */
						else if (*p == 0xb3) {
							*(p - 2) = 0xea;
							*(p - 1) = 0xad;
							*(p) = 0x93;
						}
						/* case 0x8d: // Two byte small 0xc9 0xa5 */
						/* case 0xaa: // Two byte small 0xc9 0xa6 */
						/* case 0xab: // Two byte small 0xc9 0x9c */
						/* case 0xac: // Two byte small 0xc9 0xa1 */
						/* case 0xad: // Two byte small 0xc9 0xac */
						/* case 0xae: // Two byte small 0xc9 0xaa */
						/* case 0xb0: // Two byte small 0xca 0x9e */
						/* case 0xb1: // Two byte small 0xca 0x87 */
						/* case 0xb2: // Two byte small 0xca 0x9d */
						break;
					case 0x9f: /* Latin ext */
						if ((*p == 0x82)
							|| (*p == 0x87)
							|| (*p == 0x89)
							|| (*p == 0xb5))
							(*p)++; /* Next char is lwr */
						else if (*p == 0x84) {
							*(p - 2) = 0xea;
							*(p - 1) = 0x9e;
							*(p) = 0x94;
						}
						else if (*p == 0x86) {
							*(p - 2) = 0xe1;
							*(p - 1) = 0xb6;
							*(p) = 0x8e;
						}
						/* case 0x85: // Two byte small 0xca 0x82 */
						break;
					default:
						break;
					}
					break;
				case 0xef: /* Three byte code */
					pExtChar = p;
					p++;
					switch (*pExtChar) {
					case 0xbc: /* Latin fullwidth */
						if ((*p >= 0xa1)
							&& (*p <= 0xba)) {
							*pExtChar = 0xbd;
							(*p) -= 0x20;
						}
						break;
					default:
						break;
					}
					break;
				case 0xf0: /* Four byte code */
					pExtChar = p;
					p++;
					switch (*pExtChar) {
					case 0x90:
						pExtChar = p;
						p++;
						switch (*pExtChar) {
						case 0x90: /* Deseret */
							if ((*p >= 0x80)
								&& (*p <= 0x97)) {
								(*p) += 0x28;
							}
							else if ((*p >= 0x98)
								&& (*p <= 0xa7)) {
								*pExtChar = 0x91;
								(*p) -= 0x18;
							}
							break;
						case 0x92: /* Osage  */
							if ((*p >= 0xb0)
								&& (*p <= 0xbf)) {
								*pExtChar = 0x93;
								(*p) -= 0x18;
							}
							break;
						case 0x93: /* Osage  */
							if ((*p >= 0x80)
								&& (*p <= 0x93))
								(*p) += 0x28;
							break;
						case 0xb2: /* Old hungarian */
							if ((*p >= 0x80)
								&& (*p <= 0xb2))
								*pExtChar = 0xb3;
							break;
						default:
							break;
						}
						break;
					case 0x91:
						pExtChar = p;
						p++;
						switch (*pExtChar) {
						case 0xa2: /* Warang citi */
							if ((*p >= 0xa0)
								&& (*p <= 0xbf)) {
								*pExtChar = 0xa3;
								(*p) -= 0x20;
							}
							break;
						default:
							break;
						}
						break;
					case 0x96:
						pExtChar = p;
						p++;
						switch (*pExtChar) {
						case 0xb9: /* Medefaidrin */
							if ((*p >= 0x80)
								&& (*p <= 0x9f)) {
								(*p) += 0x20;
							}
							break;
						default:
							break;
						}
						break;
					case 0x9E:
						pExtChar = p;
						p++;
						switch (*pExtChar) {
						case 0xA4: /* Adlam */
							if ((*p >= 0x80)
								&& (*p <= 0x9d))
								(*p) += 0x22;
							else if ((*p >= 0x9e)
								&& (*p <= 0xa1)) {
								*(pExtChar) = 0xa5;
								(*p) -= 0x1e;
							}
							break;
						default:
							break;
						}
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
				pExtChar = 0;
			}
			p++;
		}
	}
	return pString;
}
