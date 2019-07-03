// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONTEXT_H
#define CONTEXT_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../allocator.h"

#include "secp256k1/include/secp256k1.h"
#include <vector>
#include "uint256.h"
#include "crypto/sha256.h"

#define MESSAGE_START_SIZE 4

namespace quark {

class Context
{
public:
	typedef unsigned char MessageStartChars[MESSAGE_START_SIZE];

	enum Base58Type
	{
		PUBKEY_ADDRESS,
		SCRIPT_ADDRESS,
		SCRIPT_ADDRESS2,
		SECRET_KEY,
		EXT_PUBLIC_KEY,
		EXT_SECRET_KEY,
		MAX_BASE58_TYPES
	};

	Context();
	~Context();

	secp256k1_context* signatureContext();
	secp256k1_context* noneContext();
	secp256k1_scratch_space* signatureScratch() { return scratch_; }

	const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes_[type]; }

private:
	void initialize();

private:
	secp256k1_context* none_ = NULL;
	secp256k1_context* context_ = NULL;
	secp256k1_scratch_space* scratch_ = NULL;

	std::vector<unsigned char> base58Prefixes_[MAX_BASE58_TYPES];
	MessageStartChars messageTag_;
};

} // quark

#endif // CONTEXT_H
