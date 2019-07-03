#include "context.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include "secp256k1/include/secp256k1.h"
#include "include/secp256k1_schnorrsig.h"

using namespace quark;

Context::Context()
{
	messageTag_[0] = 0xa1;
	messageTag_[1] = 0xb1;
	messageTag_[2] = 0xc1;
	messageTag_[3] = 0xd1;

	base58Prefixes_[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0x1a);
	base58Prefixes_[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,0x1b);
	base58Prefixes_[SECRET_KEY] 	= std::vector<unsigned char>(1,0x1c);

	//base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x02)(0xfa)(0xca)(0xfd).convert_to_container<std::vector<unsigned char> >();
	//base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x02)(0xfa)(0xc3)(0x98).convert_to_container<std::vector<unsigned char> >();
}

Context::~Context()
{
	if (context_)
	{
		secp256k1_scratch_space_destroy(scratch_);
		secp256k1_context_destroy(context_);
		secp256k1_context_destroy(none_);
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

void Context::initialize()
{
	if (!context_)
	{
		none_ = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
		context_ = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
		scratch_ = secp256k1_scratch_space_create(context_, 1024 * 1024 * 1024); // magic digits
	}
}
