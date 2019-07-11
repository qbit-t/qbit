#include "context.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include "secp256k1/include/secp256k1.h"
#include "include/secp256k1_schnorrsig.h"

using namespace qbit;

Context::Context()
{
}

Context::~Context()
{
	if (context_) {
		secp256k1_context_destroy(context_);
		secp256k1_context_destroy(none_);
	}

	if (scratch_) {
		secp256k1_scratch_space_destroy(scratch_);
	}
}

secp256k1_context* Context::noneContext()
{
	initialize();
	return none_;
}

secp256k1_context* Context::signatureContext()
{
	initialize();
	return context_;
}

ContextPtr Context::instance() { return std::make_shared<Context>(); } 

secp256k1_scratch_space* Context::signatureScratch() 
{
	if (!scratch_) scratch_ = secp256k1_scratch_space_create(context_, 1024 * 1024 * 1024); // magic digits
	return scratch_; 
}

void Context::initialize()
{
	if (!context_)
	{
		none_ = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
		context_ = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
	}
}
