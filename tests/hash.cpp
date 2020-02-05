#include "hash.h"
#include "../hash/cuckoo.h"

using namespace qbit;
using namespace qbit::tests;

bool HashTest::execute() {
	uint256 hash;
	std::set<uint32_t> cycle;
	//bool result = FindCycle(hash, 27 /*edge bits*/, 42 /* proof size */, cycle);
	printf("sycel size %d\n", cycle.size());
	return true;
}
