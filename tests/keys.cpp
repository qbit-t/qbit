#include "keys.h"

using namespace qbit;
using namespace qbit::tests;

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
