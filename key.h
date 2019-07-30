// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_KEY_H
#define QBIT_KEY_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../allocator.h"

#include "uint256.h"
#include "crypto/sha256.h"
#include <list>
#include "hash.h"
#include "context.h"
#include "containers.h"
#include "utilstrencodings.h"

namespace qbit {

#define KEY_LEN 32
#define KEY_HASH_LEN 64
#define KEY_BUF_LEN 128

// forward
class PKey;

class SKey {
public:
	SKey() { valid_ = false; }
	SKey(ContextPtr);
	SKey(ContextPtr, const std::list<std::string>&);
	SKey(const std::list<std::string>&);

	const unsigned char* begin() const { return vch_; }
	const unsigned char* end() const { return vch_ + size(); }
	unsigned int size() const { return (valid_ ? KEY_LEN : 0); }

	const unsigned char& operator[](unsigned int pos) const { return vch_[pos]; }

	bool create();
	PKey createPKey();

	std::string toString();

	template <typename T> void set(const T pbegin, const T pend)
	{
		if (pend - pbegin != KEY_LEN) {
			valid_ = false;
			return;
		}
		if (check(&pbegin[0])) {
			memcpy(vch_, (unsigned char*)&pbegin[0], KEY_LEN);
			valid_ = true;
		} else {
			valid_ = false;
		}
	}

	bool valid() { return valid_; }

	bool sign(const uint256& hash /*data chunk hash*/, std::vector<unsigned char>& signature /*resulting signature*/);
	bool sign(const uint256& hash /*data chunk hash*/, uint512& signature /*resulting signature*/);

	uint256 shared(const PKey& other);

	inline ContextPtr context() { return getContext(); } 

private:
	bool check(const unsigned char *vch);
	inline ContextPtr getContext() { if (!context_) context_ = Context::instance(); return context_; }

private:
	ContextPtr context_;
	std::list<std::basic_string<unsigned char>> seed_;
	unsigned char vch_[KEY_LEN] = {0};
	bool valid_;
};

#define PKEY_LEN 65

//
// TODO: PKey consists: 4 bytes - chain identity, 65 | 33 - pubkey data calculated from secret key
// chain identity - genesis block hash (sha256 first 32 bit)
//
class PKey
{
public:
	PKey() {}
	PKey(ContextPtr context) { context_ = context; }
	PKey(const std::string& str) { fromString(str); }

	template <typename T> PKey(const T pbegin, const T pend)
	{
		set(pbegin, pend);
	}

	PKey(const std::vector<unsigned char>& vch)
	{
		set(vch.begin(), vch.end());
	}

	void setContext(ContextPtr context) { context_ = context; }

	void setPackedSize(unsigned int size) { size_ = size; }
	unsigned int getPackedSize() { return size_; }

	unsigned int size() const { return length(vch_[0]); }
	const unsigned char* begin() const { return vch_; }
	const unsigned char* end() const { return vch_ + size(); }
	const unsigned char& operator[](unsigned int pos) const { return vch_[pos]; }

	std::string toString();
	std::string toString(unsigned int len);
	std::string toHex() { return HexStr(begin(), end()); }

	bool fromString(const std::string&);

	bool valid() const
	{
		return size() > 0;
	}

	static unsigned short miner() {
		return 0xFFFA;
	}

	uint256 hash();

	void invalidate()
	{
		vch_[0] = 0xFF;
	}

	qbit::vector<unsigned char> get()
	{
		qbit::vector<unsigned char> lKey;
		lKey.insert(lKey.end(), vch_, vch_+size());
		return lKey;
	}

	template <typename T> void set(const T pbegin, const T pend)
	{
		int len = pend == pbegin ? 0 : length(pbegin[0]);
		if (len && len == (pend - pbegin))
			memcpy(vch_, (unsigned char*)&pbegin[0], len);
		else
			invalidate();
	}

	uint160 id() const
	{
		return Hash160(vch_, vch_ + size());
	}

	bool verify(const uint256& hash /*data chunk hash*/, std::vector<unsigned char>& signature /*resulting signature*/);
	bool verify(const uint256& hash /*data chunk hash*/, const uint512& signature /*resulting signature*/);

	std::vector<unsigned char> unpack();
	bool pack(unsigned char*);

private:
	unsigned int static length(unsigned char chHeader)
	{
		if (chHeader == 2 || chHeader == 3)
			return 33;
		if (chHeader == 4 || chHeader == 6 || chHeader == 7)
			return 65;
		return 0;
	}

	inline ContextPtr getContext() { if (!context_) context_ = Context::instance(); return context_; }

	ContextPtr context_;
	unsigned char vch_[PKEY_LEN] = {0};
	unsigned int size_;
};

} // qbit

#endif
