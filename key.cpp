#include <iostream>

#include "key.h"
#include "base58.h"

#include "include/secp256k1.h"
#include "include/secp256k1_ecdh.h"
#include "include/secp256k1_schnorrsig.h"
#include "libsecp256k1-config.h"
#include "scalar_impl.h"
#include "seedwords.h"

using namespace qbit;

SKey::SKey(ContextPtr context) {
	memset(vch_, 0, KEY_LEN);
	valid_ = false;
	context_ = context;
}

SKey::SKey(ContextPtr context, const std::list<std::string>& seed) {
	memset(vch_, 0, KEY_LEN);
	valid_ = false;
	context_ = context;

	for (std::list<std::string>::const_iterator lIter = seed.begin(); lIter != seed.end(); ++lIter) {
		seed_.push_back(Word(*lIter));
	}
}

SKey::SKey(const std::list<std::string>& seed) {
	memset(vch_, 0, KEY_LEN);
	valid_ = false;

	for (std::list<std::string>::const_iterator lIter = seed.begin(); lIter != seed.end(); lIter++) {
		seed_.push_back(Word(*lIter));
	}
}

bool SKey::check(const unsigned char *vch) {
	return true;
}

bool SKey::create() {
	//
	unsigned char lHash[KEY_BUF_LEN]; // max

	// if seed words specified
	if (!seed_.size()) {
		SeedPhrase lPhrase;
		std::list<std::string> lWords = lPhrase.generate();

		for (std::list<std::string>::iterator lWord = lWords.begin(); lWord != lWords.end(); lWord++) {
			seed_.push_back(Word(*lWord));
		}
	}

	std::basic_string<unsigned char> lPhrase;
	for(std::vector<Word>::iterator lItem = seed_.begin(); lItem != seed_.end(); lItem++) {
		lPhrase += lItem->word();
	}

	CSHA512 lHasher;
	lHasher.Write(lPhrase.c_str(), lPhrase.length());
	lHasher.Finalize(lHash);

	memcpy(vch_, lHash, KEY_LEN);
	valid_ = true;

	return valid_;
}

std::string SKey::toString() {
	std::vector<unsigned char> vch; vch.assign((unsigned char*)vch_, ((unsigned char*)vch_) + KEY_LEN);
	return EncodeBase58Check(vch);
}

bool SKey::sign(const uint256& hash /*data chunk hash*/, std::vector<unsigned char>& signature /*resulting signature*/) {
	unsigned char lSig64[64] = {0};
	secp256k1_schnorrsig lSig;
	int nonce_is_negated = 0;

	ContextPtr lContext = getContext();
	if (secp256k1_schnorrsig_sign(lContext->signatureContext(), &lSig, &nonce_is_negated, hash.begin(), vch_, NULL, NULL)) {
		if (secp256k1_schnorrsig_serialize(lContext->noneContext(), lSig64, &lSig)) {
			signature.insert(signature.end(), lSig64, lSig64+sizeof(lSig64));
			return true;
		}
	}

	return false;
}

bool SKey::sign(const uint256& hash /*data chunk hash*/, uint512& signature /*resulting signature*/) {
	secp256k1_schnorrsig lSig;
	int nonce_is_negated = 0;

	ContextPtr lContext = getContext();
	if (secp256k1_schnorrsig_sign(lContext->signatureContext(), &lSig, &nonce_is_negated, hash.begin(), vch_, NULL, NULL)) {
		if (secp256k1_schnorrsig_serialize(lContext->noneContext(), signature.begin(), &lSig)) {
			return true;
		}
	}

	return false;
}

PKey SKey::createPKey() {
	PKey lPKey;
	size_t lPKeyLen = 65;

	secp256k1_pubkey pubkey;
	ContextPtr lContext = getContext();
	if (!secp256k1_ec_pubkey_create(lContext->signatureContext(), &pubkey, vch_)) return PKey();
	if (secp256k1_ec_pubkey_serialize(lContext->signatureContext(), (unsigned char*)lPKey.begin(), &lPKeyLen, &pubkey, SECP256K1_EC_COMPRESSED)) {
		lPKey.setPackedSize(lPKeyLen);
		return lPKey;
	}

	return PKey(getContext());
}

uint256 SKey::shared(const PKey& other) {
	uint256 lShared;
	secp256k1_pubkey lPk;
	ContextPtr lContext = getContext();
	if (secp256k1_ec_pubkey_parse(lContext->signatureContext(), &lPk, other.begin(), other.size()))
		if (secp256k1_ecdh(lContext->signatureContext(), lShared.begin(), &lPk, vch_, NULL, NULL))
			return lShared;

	return uint256();
}

//
// PKey
std::string PKey::toString() {
	std::vector<unsigned char> vch; vch.assign((unsigned char*)vch_, ((unsigned char*)vch_) + size());
	return EncodeBase58Check(vch);
}

std::string PKey::toString(unsigned int len) {
	std::vector<unsigned char> vch; vch.assign((unsigned char*)vch_, ((unsigned char*)vch_) + len);
	return EncodeBase58Check(vch);
}

bool PKey::fromString(const std::string& str) {
	std::vector<unsigned char> vch;
	secp256k1_pubkey lPk;

	ContextPtr lContext = getContext();
	if (DecodeBase58Check(str.c_str(), vch)) {
		memcpy(vch_, &vch[0], vch.size());
		if (secp256k1_ec_pubkey_parse(lContext->signatureContext(), &lPk, begin(), size()))
			return true;
	}

	return false;
}

bool PKey::verify(const uint256& hash /*data chunk hash*/, std::vector<unsigned char>& signature /*resulting signature*/) {
	secp256k1_schnorrsig lSig;
	secp256k1_pubkey lPk;
	ContextPtr lContext = getContext();
	if (secp256k1_schnorrsig_parse(lContext->noneContext(), &lSig, (unsigned char*)&signature[0])) {
		if (secp256k1_ec_pubkey_parse(lContext->signatureContext(), &lPk, begin(), size())) {
			if (secp256k1_schnorrsig_verify(lContext->signatureContext(), &lSig, hash.begin(), &lPk)) {
				return true;
			}
		}
	}

	return false;
}

bool PKey::verify(const uint256& hash /*data chunk hash*/, const uint512& signature /*resulting signature*/) {
	secp256k1_schnorrsig lSig;
	secp256k1_pubkey lPk;
	ContextPtr lContext = getContext();
	if (secp256k1_schnorrsig_parse(lContext->noneContext(), &lSig, signature.begin())) {
		if (secp256k1_ec_pubkey_parse(lContext->signatureContext(), &lPk, begin(), size())) {
			if (secp256k1_schnorrsig_verify(lContext->signatureContext(), &lSig, hash.begin(), &lPk)) {
				return true;
			}
		}
	}

	return false;
}

std::vector<unsigned char> PKey::unpack() {
	secp256k1_pubkey lPk;
	ContextPtr lContext = getContext();
	if (secp256k1_ec_pubkey_parse(lContext->signatureContext(), &lPk, begin(), size())) {
		std::vector<unsigned char> lKey;
		lKey.insert(lKey.end(), lPk.data, lPk.data+sizeof(secp256k1_pubkey));
		return lKey;
	}

	return std::vector<unsigned char>();
}

bool PKey::pack(unsigned char* key) {
	size_t lPKeyLen = 65;
	ContextPtr lContext = getContext();
	if (secp256k1_ec_pubkey_serialize(lContext->signatureContext(), (unsigned char*)begin(), &lPKeyLen, (const secp256k1_pubkey*)key,  SECP256K1_EC_COMPRESSED))
	{
		setPackedSize(lPKeyLen);
		return true;
	}

	return false;
}
