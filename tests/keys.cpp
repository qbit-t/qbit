#include "keys.h"
#include "../random.h"

using namespace qbit;
using namespace qbit::tests;

bool RandomTest::execute() {
	uint256 r0 = Random::generate();
	uint256 r1 = Random::generate();

	if (r0 == r1) { error_ = "Digits are identical"; return false; }
	return true;
}

bool CreateKeySignAndVerify::execute() {
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));

	SKey lKey0(lSeed0);
	lKey0.create();
	
	PKey lPKey0 = lKey0.createPKey();

	// 1.0 sign & verify
	std::string lMessage = "this is a first message of mine!";
	uint256 lHash = Hash(lMessage.begin(), lMessage.end());
	std::vector<unsigned char> lSig;
	if (lKey0.sign(lHash, lSig)) {
		if (lPKey0.verify(lHash, lSig)) {
			return true;
		} else { error_ = "Signature was not verified"; }
	} else { error_ = "Signature was not made"; }

	return false;
}

#include "include/secp256k1.h"
#include "include/secp256k1_ecdh.h"
#include "include/secp256k1_schnorrsig.h"
#include "include/secp256k1_generator.h"
#include "include/secp256k1_rangeproof.h"
#include "libsecp256k1-config.h"
//#include "scalar_impl.h"
//#include "ecmult_impl.h"

#include "hash_impl.h"
#include "num_impl.h"
#include "field_impl.h"
#include "group_impl.h"
#include "scalar_impl.h"

bool CreateKeySignAndVerifyMix::execute() {
	// key1
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));

	SKey lKey0(lSeed0);
	lKey0.create();
	
	PKey lPKey0 = lKey0.createPKey();

	// key2
	std::list<std::string> lSeed1;
	lSeed1.push_back(std::string("fitness"));
	lSeed1.push_back(std::string("exchange"));
	lSeed1.push_back(std::string("glance"));
	lSeed1.push_back(std::string("diamond"));
	lSeed1.push_back(std::string("crystal"));
	lSeed1.push_back(std::string("cinnamon"));
	lSeed1.push_back(std::string("border"));
	lSeed1.push_back(std::string("arrange"));
	lSeed1.push_back(std::string("attitude"));
	lSeed1.push_back(std::string("between"));
	lSeed1.push_back(std::string("broccoli"));
	lSeed1.push_back(std::string("cannon"));
	lSeed1.push_back(std::string("crane"));
	lSeed1.push_back(std::string("double"));
	lSeed1.push_back(std::string("eyebrow"));
	lSeed1.push_back(std::string("frequent"));
	lSeed1.push_back(std::string("gravity"));
	lSeed1.push_back(std::string("hidden"));
	lSeed1.push_back(std::string("identify"));
	lSeed1.push_back(std::string("innocent"));
	lSeed1.push_back(std::string("jealous"));
	lSeed1.push_back(std::string("language"));
	lSeed1.push_back(std::string("leopard"));
	lSeed1.push_back(std::string("jellyfish"));

	SKey lKey1(lSeed1);
	lKey1.create();
	
	PKey lPKey1 = lKey1.createPKey();

	// key3
	std::list<std::string> lSeed2;
	lSeed2.push_back(std::string("fitness"));
	lSeed2.push_back(std::string("exchange"));
	lSeed2.push_back(std::string("glance"));
	lSeed2.push_back(std::string("diamond"));
	lSeed2.push_back(std::string("crystal"));
	lSeed2.push_back(std::string("cinnamon"));
	lSeed2.push_back(std::string("border"));
	lSeed2.push_back(std::string("arrange"));
	lSeed2.push_back(std::string("attitude"));
	lSeed2.push_back(std::string("between"));
	lSeed2.push_back(std::string("broccoli"));
	lSeed2.push_back(std::string("cannon"));
	lSeed2.push_back(std::string("crane"));
	lSeed2.push_back(std::string("double"));
	lSeed2.push_back(std::string("eyebrow"));
	lSeed2.push_back(std::string("frequent"));
	lSeed2.push_back(std::string("gravity"));
	lSeed2.push_back(std::string("hidden"));
	lSeed2.push_back(std::string("identify"));
	lSeed2.push_back(std::string("innocent"));
	lSeed2.push_back(std::string("jealous"));
	lSeed2.push_back(std::string("language"));
	lSeed2.push_back(std::string("leopard"));
	lSeed2.push_back(std::string("homodrillo"));

	SKey lKey2(lSeed2);
	lKey2.create();
	
	PKey lPKey2 = lKey2.createPKey();

	// key4
	std::list<std::string> lSeed3;
	lSeed3.push_back(std::string("fitness"));
	lSeed3.push_back(std::string("exchange"));
	lSeed3.push_back(std::string("glance"));
	lSeed3.push_back(std::string("diamond"));
	lSeed3.push_back(std::string("crystal"));
	lSeed3.push_back(std::string("cinnamon"));
	lSeed3.push_back(std::string("border"));
	lSeed3.push_back(std::string("arrange"));
	lSeed3.push_back(std::string("attitude"));
	lSeed3.push_back(std::string("between"));
	lSeed3.push_back(std::string("broccoli"));
	lSeed3.push_back(std::string("cannon"));
	lSeed3.push_back(std::string("crane"));
	lSeed3.push_back(std::string("double"));
	lSeed3.push_back(std::string("eyebrow"));
	lSeed3.push_back(std::string("frequent"));
	lSeed3.push_back(std::string("gravity"));
	lSeed3.push_back(std::string("hidden"));
	lSeed3.push_back(std::string("identify"));
	lSeed3.push_back(std::string("innocent"));
	lSeed3.push_back(std::string("jealous"));
	lSeed3.push_back(std::string("language"));
	lSeed3.push_back(std::string("leopard"));
	lSeed3.push_back(std::string("coccodrillo"));

	SKey lKey3(lSeed3);
	lKey3.create();
	
	PKey lPKey3 = lKey3.createPKey();

	// key0 - me
	//
	//
	uint256 lSh00 = lKey0.shared(lPKey1); // in  - 64
	uint256 lSh01 = lKey0.shared(lPKey2); // in  - 100
	uint256 lSh11 = lKey0.shared(lPKey3); // out - 164 (neg)

	secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
	
	secp256k1_scalar lSh00S; secp256k1_scalar_set_b32(&lSh00S, lSh00.begin(), NULL); 
	secp256k1_scalar lSh01S; secp256k1_scalar_set_b32(&lSh01S, lSh01.begin(), NULL); 
	secp256k1_scalar lSh11S; secp256k1_scalar_set_b32(&lSh11S, lSh11.begin(), NULL); 
	secp256k1_scalar lShAll; secp256k1_scalar_set_int(&lShAll, 0);

	secp256k1_scalar_add(&lShAll, &lShAll, &lSh00S);
	secp256k1_scalar_add(&lShAll, &lShAll, &lSh01S);
	secp256k1_scalar_negate(&lSh11S, &lSh11S); // neg
	secp256k1_scalar_add(&lShAll, &lShAll, &lSh11S);

	unsigned char lBlAll[32];
	secp256k1_scalar_get_b32(lBlAll, &lShAll);

	unsigned char commit_proof[5134 + 1];
	size_t commit_proof_len = 5134;
	unsigned char ser_commit[33];

	uint64_t minv;
	uint64_t maxv;

	secp256k1_pedersen_commitment in_commit[2];
	secp256k1_pedersen_commitment out_commit[2];

	const secp256k1_pedersen_commitment *in_commit_ptr[2]; in_commit_ptr[0] = &in_commit[0]; in_commit_ptr[1] = &in_commit[1];
	const secp256k1_pedersen_commitment *out_commit_ptr[2]; out_commit_ptr[0] = &out_commit[0]; out_commit_ptr[1] = &out_commit[1];

	/*
	// 1st
	commit_proof_len = 5134;
	secp256k1_pedersen_commit(ctx, &in_commit[0], lSh00.begin(), (uint64_t)64, secp256k1_generator_h);
	secp256k1_pedersen_commitment_serialize(ctx, ser_commit, &in_commit[0]);
	secp256k1_rangeproof_sign(ctx, commit_proof, &commit_proof_len, 0, &in_commit[0], lSh00.begin(), lSh00.begin(), 0, 0, (uint64_t)64, NULL, 0, NULL, 0, secp256k1_generator_h);
	// --------- verify ------------
	if (!secp256k1_rangeproof_verify(ctx, &minv, &maxv, &in_commit[0], commit_proof, commit_proof_len, NULL, 0, secp256k1_generator_h))
	{ std::cout << "verify - err!\n"; }
	// --------- verify ------------
	std::cout << std::endl << "[commit] 33 : " << HexStr(ser_commit, ser_commit + sizeof(ser_commit)) << std::endl <<
		"[sign] " << commit_proof_len << " : " << HexStr(commit_proof, commit_proof + commit_proof_len) << std::endl;
	// [ser_commit, commit_proof] -> transaction

	// 2nd
	commit_proof_len = 5134;
	secp256k1_pedersen_commit(ctx, &in_commit[1], lSh01.begin(), (uint64_t)100, secp256k1_generator_h);
	secp256k1_pedersen_commitment_serialize(ctx, ser_commit, &in_commit[1]);
	secp256k1_rangeproof_sign(ctx, commit_proof, &commit_proof_len, 0, &in_commit[1], lSh01.begin(), lSh01.begin(), 0, 0, (uint64_t)100, NULL, 0, NULL, 0, secp256k1_generator_h);
	// --------- verify ------------
	if (!secp256k1_rangeproof_verify(ctx, &minv, &maxv, &in_commit[1], commit_proof, commit_proof_len, NULL, 0, secp256k1_generator_h))
	{ std::cout << "verify - err!\n"; }
	// --------- verify ------------
	std::cout << std::endl << "[commit] 33 : " << HexStr(ser_commit, ser_commit + sizeof(ser_commit)) << std::endl <<
		"[sign] " << commit_proof_len << " : " << HexStr(commit_proof, commit_proof + commit_proof_len) << std::endl;

	// 3rd
	commit_proof_len = 5134;
	secp256k1_pedersen_commit(ctx, &out_commit[0], lSh11.begin(), (uint64_t)164, secp256k1_generator_h);
	secp256k1_pedersen_commitment_serialize(ctx, ser_commit, &out_commit[0]);
	secp256k1_rangeproof_sign(ctx, commit_proof, &commit_proof_len, 0, &out_commit[0], lSh11.begin(), lSh11.begin(), 0, 0, (uint64_t)164, NULL, 0, NULL, 0, secp256k1_generator_h);
	// --------- verify ------------
	if (!secp256k1_rangeproof_verify(ctx, &minv, &maxv, &out_commit[0], commit_proof, commit_proof_len, NULL, 0, secp256k1_generator_h))
	{ std::cout << "verify - err!\n"; }
	// --------- verify ------------
	std::cout << std::endl << "[commit] 33 : " << HexStr(ser_commit, ser_commit + sizeof(ser_commit)) << std::endl <<
		"[sign] " << commit_proof_len << " : " << HexStr(commit_proof, commit_proof + commit_proof_len) << std::endl;

	// finale
	commit_proof_len = 5134;
	secp256k1_pedersen_commit(ctx, &out_commit[1], lBlAll, (uint64_t)0, secp256k1_generator_h);
	secp256k1_pedersen_commitment_serialize(ctx, ser_commit, &out_commit[1]);
	secp256k1_rangeproof_sign(ctx, commit_proof, &commit_proof_len, 0, &out_commit[1], lBlAll, lBlAll, 0, 0, (uint64_t)0, NULL, 0, NULL, 0, secp256k1_generator_h);
	// --------- verify ------------
	if (!secp256k1_rangeproof_verify(ctx, &minv, &maxv, &out_commit[1], commit_proof, commit_proof_len, NULL, 0, secp256k1_generator_h))
	{ std::cout << "verify - err!\n"; }
	// --------- verify ------------
	std::cout << std::endl << "[commit] 33 : " << HexStr(ser_commit, ser_commit + sizeof(ser_commit)) << std::endl <<
		"[sign] " << commit_proof_len << " : " << HexStr(commit_proof, commit_proof + commit_proof_len) << std::endl;

	// check balance
	if (secp256k1_pedersen_verify_tally(ctx, in_commit_ptr, 2, out_commit_ptr, 2)) 
	{
		std::cout << "OK!\n";
	}
	else std::cout << "ERRR!\n";
	*/

	// check range-proof

	/*
	secp256k1_pedersen_commitment commits[4];
	const secp256k1_pedersen_commitment *cptr[4];
	unsigned char blinds[32*4];
	const unsigned char *bptr[4];
	secp256k1_scalar s;
	uint64_t values[4];
	int64_t totalv;
	int i;
	int inputs;
	int outputs;
	int total;
	inputs = 2;
	outputs = 2;
	total = inputs + outputs;
	for (i = 0; i < 4; i++) {
		cptr[i] = &commits[i];
		bptr[i] = &blinds[i * 32];
	}
	totalv = 164;
	values[0] = 64;
	values[1] = 100;

	for (i = 0; i < outputs - 1; i++) {
		values[i + inputs] = 164;
		totalv -= values[i + inputs];
	}
	values[total - 1] = totalv;

	secp256k1_scalar_get_b32(&blinds[0 * 32], &lSh00S);
	secp256k1_scalar_get_b32(&blinds[1 * 32], &lSh01S);
	secp256k1_scalar_get_b32(&blinds[2 * 32], &lSh11S);

	unsigned char proof[5134 + 1];

	std::cout << std::endl;

	uint32_t lBegin = now();
	std::cout << "secp256k1_pedersen_commit ";

	if(!secp256k1_pedersen_blind_sum(ctx, &blinds[(total - 1) * 32], bptr, total - 1, inputs)) { std::cout << "sum - ERR!\n"; }
	for (i = 0; i < total; i++) {
		if (!secp256k1_pedersen_commit(ctx, &commits[i], &blinds[i * 32], values[i], secp256k1_generator_h)) { std::cout << "commit - ERR!\n"; }

		size_t len = 5134;
		if (!secp256k1_rangeproof_sign(ctx, proof, &len, 0, &commits[i], &blinds[i * 32], &blinds[i * 32], 0, 0, values[i], NULL, 0, NULL, 0, secp256k1_generator_h))
		{ std::cout << "sign - ERR!\n"; }

		//std::cout << "[sign] " << len << " : " << HexStr(proof, proof + len) << std::endl;

		unsigned char blindout[32];
		uint64_t vout;
		uint64_t vmin;
		uint64_t minv;
		uint64_t maxv;

		if( !secp256k1_rangeproof_rewind(ctx, blindout, &vout, NULL, NULL, &blinds[i * 32], &minv, &maxv, &commits[i], proof, len, NULL, 0, secp256k1_generator_h))
		{ std::cout << "unwind - ERR!\n"; }			

		std::cout << "[unwind] " << vout << std::endl;

	}
	
	std::cout << (now() - lBegin) << "mc" << std::endl;
	

	lBegin = now();
	std::cout << "secp256k1_pedersen_verify_tally ";
	if (!secp256k1_pedersen_verify_tally(ctx, cptr, inputs, &cptr[inputs], outputs)) { std::cout << "verify - ERR!\n"; }
	std::cout << (now() - lBegin) << "mc" << std::endl;

	*/

	//std::cout << std::endl;
	//std::cout << 
	//  lSh00.toString() << std::endl <<
	//  lSh01.toString() << std::endl <<
	//  lSh11.toString() << std::endl <<
	//  lSh12.toString() << std::endl;

	return true;
}
