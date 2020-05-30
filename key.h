// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_KEY_H
#define QBIT_KEY_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"

#include "uint256.h"
#include "crypto/sha256.h"
#include <list>
#include "hash.h"
#include "context.h"
#include "containers.h"
#include "utilstrencodings.h"
#include "serialize.h"

namespace qbit {

#define KEY_LEN 32
#define KEY_HASH_LEN 64
#define KEY_BUF_LEN 128
#define WORD_LEN 64

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

	~PKey() {
		if (context_) context_.reset();
	}

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

	std::string toString() const;
	std::string toString(unsigned int len) const;
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

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(vch_);
	}	

	friend inline bool operator < (const PKey& a, const PKey& b) {
		return a.id() < b.id();
	}

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

class SKey;
typedef std::shared_ptr<SKey> SKeyPtr;

class SKey {
public:
	class Word {
	public:
		Word() {}
		Word(const std::string& word) { 
			if (word.size() < WORD_LEN) memcpy(word_, word.c_str(), word.size()); 
			else memcpy(word_, word.c_str(), WORD_LEN-1);
		}

		template<typename Stream>
		void serialize(Stream& s) const
		{
			s.write((char*)word_, sizeof(word_));
		}

		template<typename Stream>
		void deserialize(Stream& s)
		{
			s.read((char*)word_, sizeof(word_));
		}

		std::basic_string<unsigned char> word() { return std::basic_string<unsigned char>(word_); }
		std::basic_string<char> wordA() { return std::basic_string<char>((char*)word_); }

	private:
		unsigned char word_[WORD_LEN] = {0};
	};
public:
	SKey() { valid_ = false; }
	SKey(ContextPtr);
	SKey(ContextPtr, const std::list<std::string>&);
	SKey(const std::list<std::string>&);

	~SKey() {
		if (context_) context_.reset();
	}

	const unsigned char* begin() const { return vch_; }
	const unsigned char* end() const { return vch_ + size(); }
	unsigned int size() const { return (valid_ ? KEY_LEN : 0); }

	const unsigned char& operator[](unsigned int pos) const { return vch_[pos]; }

	bool create();
	PKey createPKey();

	std::string toString() const;
	std::string toHex() { return HexStr(begin(), end()); }

	static SKeyPtr instance() {
		return std::make_shared<SKey>();
	}

	static SKeyPtr instance(const SKey& key) {
		return std::make_shared<SKey>(key);
	}

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

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(valid_);
		READWRITE(vch_);
		READWRITE(seed_);

		if (!ser_action.ForRead()) {
			if (vch_[0] != 0 && vch_[KEY_LEN-1] != 0)
				valid_ = true;
		}
	}

	std::vector<Word>& seed() { return seed_; }

private:
	bool check(const unsigned char *vch);
	inline ContextPtr getContext() { if (!context_) context_ = Context::instance(); return context_; }

private:
	ContextPtr context_;
	std::vector<Word> seed_;
	unsigned char vch_[KEY_LEN] = {0};
	bool valid_;

	PKey pkey_;
};

} // qbit

#endif
