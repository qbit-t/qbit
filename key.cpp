#include <iostream>

#include "key.h"
#include "base58.h"

#include "include/secp256k1.h"
#include "include/secp256k1_schnorrsig.h"

using namespace quark;

SKey::SKey(Context* context)
{
	memset(vch_, 0, KEY_LEN);
	valid_ = false;
	context_ = context;
}

SKey::SKey(Context* context, std::list<std::string>& seed)
{
	memset(vch_, 0, KEY_LEN);
	valid_ = false;
	context_ = context;

	for (std::list<std::string>::iterator lIter = seed.begin(); lIter != seed.end(); lIter++)
	{
		seed_.push_back(std::basic_string<unsigned char>((unsigned char*)(*lIter).c_str()));
	}
}

bool SKey::check(const unsigned char *vch)
{
	return true;
}

bool SKey::create()
{
	// if seed words specified
	if (seed_.size())
	{
		unsigned char lHash[KEY_BUF_LEN]; // max

		std::basic_string<unsigned char> lPhrase;
		for(std::list<std::basic_string<unsigned char>>::iterator lItem = seed_.begin(); lItem != seed_.end(); lItem++)
		{
			lPhrase += *lItem;
		}

		CSHA512 lHasher;
		lHasher.Write(lPhrase.c_str(), lPhrase.length());
		lHasher.Finalize(lHash);

		memcpy(vch_, lHash, KEY_LEN);
		valid_ = true;
	}
	else // collect seed words
	{
	}

	return valid_;
}

std::string SKey::toString()
{
	std::vector<unsigned char> vch; vch.assign((unsigned char*)vch_, ((unsigned char*)vch_) + KEY_LEN);
	return EncodeBase58Check(vch);
}

bool SKey::sign(const uint256& hash /*data chunk hash*/, std::vector<unsigned char>& signature /*resulting signature*/) const
{
	unsigned char lSig64[64] = {0};
	secp256k1_schnorrsig lSig;
	int nonce_is_negated = 0;

	if (secp256k1_schnorrsig_sign(context_->signatureContext(), &lSig, &nonce_is_negated, hash.begin(), vch_, NULL, NULL)) {
		if (secp256k1_schnorrsig_serialize(context_->noneContext(), lSig64, &lSig)) {
			signature.insert(signature.end(), lSig64, lSig64+sizeof(lSig64));
			return true;
		}
	}

	return false;
}

PKey SKey::createPKey()
{
	PKey lPKey(context_);
	size_t lPKeyLen = 65;

	secp256k1_pubkey pubkey;
	if (!secp256k1_ec_pubkey_create(context_->signatureContext(), &pubkey, vch_)) return PKey();
	if (secp256k1_ec_pubkey_serialize(context_->signatureContext(), (unsigned char*)lPKey.begin(), &lPKeyLen, &pubkey, SECP256K1_EC_COMPRESSED))
	{
		lPKey.setPackedSize(lPKeyLen);
		return lPKey;
	}

	return PKey(context_);
}


// PKey
std::string PKey::toString()
{
	std::vector<unsigned char> vch; vch.assign((unsigned char*)vch_, ((unsigned char*)vch_) + size());
	return EncodeBase58Check(vch);
}

std::string PKey::toString(unsigned int len)
{
	std::vector<unsigned char> vch; vch.assign((unsigned char*)vch_, ((unsigned char*)vch_) + len);
	return EncodeBase58Check(vch);
}

bool PKey::verify(const uint256& hash /*data chunk hash*/, std::vector<unsigned char>& signature /*resulting signature*/) const
{
	secp256k1_schnorrsig lSig;
	secp256k1_pubkey lPk;
	if (secp256k1_schnorrsig_parse(context_->noneContext(), &lSig, (unsigned char*)&signature[0])) {
		if (secp256k1_ec_pubkey_parse(context_->signatureContext(), &lPk, begin(), size())) {
			if (secp256k1_schnorrsig_verify(context_->signatureContext(), &lSig, hash.begin(), &lPk)) {
				return true;
			}
		}
		else std::cout << "pkey fail\n";
	}

	return false;
}

std::vector<unsigned char> PKey::unpack()
{
	secp256k1_pubkey lPk;
	if (secp256k1_ec_pubkey_parse(context_->signatureContext(), &lPk, begin(), size())) {
		std::vector<unsigned char> lKey;
		lKey.insert(lKey.end(), lPk.data, lPk.data+sizeof(secp256k1_pubkey));
		return lKey;
	}

	return std::vector<unsigned char>();
}

bool PKey::pack(unsigned char* key)
{
	size_t lPKeyLen = 65;
	if (secp256k1_ec_pubkey_serialize(context_->signatureContext(), (unsigned char*)begin(), &lPKeyLen, (const secp256k1_pubkey*)key,  SECP256K1_EC_COMPRESSED))
	{
		setPackedSize(lPKeyLen);
		return true;
	}

	return false;
}
