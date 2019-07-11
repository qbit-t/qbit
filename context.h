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

#include <memory>

#define MESSAGE_START_SIZE 4

namespace qbit {

class Context;
typedef std::shared_ptr<Context> ContextPtr;

class Context
{
public:
	Context();
	~Context();

	secp256k1_context* signatureContext();
	secp256k1_context* noneContext();
	secp256k1_scratch_space* signatureScratch();

	static ContextPtr instance();

private:
	void initialize();

private:
	secp256k1_context* none_ = NULL;
	secp256k1_context* context_ = NULL;
	secp256k1_scratch_space* scratch_ = NULL;
};

} // qbit

#endif // CONTEXT_H
