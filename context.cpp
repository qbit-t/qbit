#include "context.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include "include/secp256k1.h"
#include "include/secp256k1_ecdh.h"
#include "include/secp256k1_schnorrsig.h"
#include "include/secp256k1_generator.h"
#include "include/secp256k1_rangeproof.h"
#include "libsecp256k1-config.h"

#include "log/log.h"

#include <boost/thread.hpp>

#include "hash_impl.h"
#include "num_impl.h"
#include "field_impl.h"
#include "group_impl.h"
#include "scalar_impl.h"

using namespace qbit;

#include <signal.h>

Context::Context() {
}

Context::~Context() {
	if (context_) {
		secp256k1_context_destroy(context_);
		secp256k1_context_destroy(none_);
	}

	if (scratch_) {
		secp256k1_scratch_space_destroy(scratch_);
	}
}

secp256k1_context* Context::noneContext() {
	initialize();
	return none_;
}

secp256k1_context* Context::signatureContext() {
	initialize();
	return context_;
}

ContextPtr Context::instance() { 
	//
	static boost::thread_specific_ptr<ContextPtr> tContext;
	if (!tContext.get()) {
		tContext.reset(new ContextPtr(new Context()));
		return ContextPtr(*tContext.get()); 
	}

	return ContextPtr(*tContext.get());
} 

secp256k1_scratch_space* Context::signatureScratch() {
	if (!scratch_) scratch_ = secp256k1_scratch_space_create(context_, 1024 * 1024 * 1024); // magic digits
	return scratch_; 
}

void Context::initialize()
{
	if (!context_) {
		none_ = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
		context_ = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
	}
}

bool Context::createCommitment(qbit::vector<unsigned char>& out, const uint256& blind, amount_t amount) {
	secp256k1_pedersen_commitment lCommit;
	unsigned char lSerialized[33];
	if (secp256k1_pedersen_commit(signatureContext(), &lCommit, blind.begin(), amount, secp256k1_generator_h)) {
		secp256k1_pedersen_commitment_serialize(signatureContext(), lSerialized, &lCommit);
		out.insert(out.end(), lSerialized, lSerialized + sizeof(lSerialized));

		return true;
	}

	return false;
}

bool Context::createRangeProof(qbit::vector<unsigned char>& out, const qbit::vector<unsigned char>& commit, const uint256& blind, const uint256& nonce, amount_t amount) {
	unsigned char lProof[5134 + 1];
	size_t lLen = 5134;
	secp256k1_pedersen_commitment lCommit;

	if (secp256k1_pedersen_commitment_parse(signatureContext(), &lCommit, &commit[0]))
		if (secp256k1_rangeproof_sign(signatureContext(), lProof, &lLen, 0, &lCommit, blind.begin(), nonce.begin(), 0, 0, amount, NULL, 0, NULL, 0, secp256k1_generator_h)) {
			out.insert(out.end(), lProof, lProof + lLen);
			return true;
		}

	return false;
}

bool Context::verifyRangeProof(const unsigned char* commit, const unsigned char* proof, size_t size) {
	uint64_t lMinValue;
	uint64_t lMaxValue;
	secp256k1_pedersen_commitment lCommit;
	if (secp256k1_pedersen_commitment_parse(signatureContext(), &lCommit, commit))
		if (secp256k1_rangeproof_verify(signatureContext(), &lMinValue, &lMaxValue, &lCommit, proof, size, NULL, 0, secp256k1_generator_h))
		return true;

	return false;
}

bool Context::rewindRangeProof(uint64_t* vout, uint256& blind, const uint256& nonce, const unsigned char* commit, const unsigned char* proof, size_t size) {
	uint64_t lMinValue;
	uint64_t lMaxValue;

	secp256k1_pedersen_commitment lCommit;
	if (secp256k1_pedersen_commitment_parse(signatureContext(), &lCommit, commit))
		if (secp256k1_rangeproof_rewind(signatureContext(), blind.begin(), vout, NULL, NULL, nonce.begin(), &lMinValue, &lMaxValue, &lCommit, proof, size, NULL, 0, secp256k1_generator_h))
			return true;
	return false;
}

bool Context::verifyTally(const std::list<std::vector<unsigned char>>& in, const std::list<std::vector<unsigned char>>& out) {
	std::vector<secp256k1_pedersen_commitment> lIn;
	std::vector<secp256k1_pedersen_commitment> lOut;

	std::vector<secp256k1_pedersen_commitment*> lInPtr;
	std::vector<secp256k1_pedersen_commitment*> lOutPtr;

	lIn.resize(in.size());
	lOut.resize(out.size());

	lInPtr.resize(in.size());
	lOutPtr.resize(out.size());

	int lIdx = 0;
	for (std::list<std::vector<unsigned char>>::const_iterator lInIter = in.begin(); lInIter != in.end(); lInIter++, lIdx++) {
		if (secp256k1_pedersen_commitment_parse(signatureContext(), &lIn[lIdx], &(*lInIter)[0])) {
			lInPtr[lIdx] = &lIn[lIdx];
		} else return false;
	}

	lIdx = 0;
	for (std::list<std::vector<unsigned char>>::const_iterator lOutIter = out.begin(); lOutIter != out.end(); lOutIter++, lIdx++) {
		if (secp256k1_pedersen_commitment_parse(signatureContext(), &lOut[lIdx], &(*lOutIter)[0])) {
			lOutPtr[lIdx] = &lOut[lIdx];
		} else return false;
	}

	if (!secp256k1_pedersen_verify_tally(signatureContext(), &lInPtr[0], lInPtr.size(), &lOutPtr[0], lOutPtr.size())) return false;
	return true;
}


bool Math::add(uint256& c, const uint256& a, const uint256& b) {
	int lOverflow;
	secp256k1_scalar lA; secp256k1_scalar_set_b32(&lA, a.begin(), &lOverflow); if (lOverflow) return 0;
	secp256k1_scalar lB; secp256k1_scalar_set_b32(&lB, b.begin(), &lOverflow); if (lOverflow) return 0;
	secp256k1_scalar lC;

	secp256k1_scalar_add(&lC, &lA, &lB);

	unsigned char lRawC[32];
	secp256k1_scalar_get_b32(lRawC, &lC);

	c.set(lRawC);

	return 1;
}

bool Math::mul(uint256& c, const uint256& a, const uint256& b) {
	int lOverflow;
	secp256k1_scalar lA; secp256k1_scalar_set_b32(&lA, a.begin(), &lOverflow); if (lOverflow) return 0;
	secp256k1_scalar lB; secp256k1_scalar_set_b32(&lB, b.begin(), &lOverflow); if (lOverflow) return 0;
	secp256k1_scalar lC;

	secp256k1_scalar_mul(&lC, &lA, &lB);

	unsigned char lRawC[32];
	secp256k1_scalar_get_b32(lRawC, &lC);

	c.set(lRawC);

	return 1;
}

bool Math::neg(uint256& c, const uint256& a) {
	int lOverflow;
	secp256k1_scalar lA; secp256k1_scalar_set_b32(&lA, a.begin(), &lOverflow); if (lOverflow) return 0;
	secp256k1_scalar lC;

	secp256k1_scalar_negate(&lC, &lA);

	unsigned char lRawC[32];
	secp256k1_scalar_get_b32(lRawC, &lC);

	c.set(lRawC);

	return 1;
}
