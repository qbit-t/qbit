#include "hash.h"
#include "../block.h"
#include "../hash.h"
#include "../hash/cuckoo.h"

using namespace qbit;
using namespace qbit::tests;

bool HashTest::execute() {
	Block blk;
	blk.bits_ = 0x207fffff;
	std::set<uint32_t> cycle;
	for(int i=0; i<10000; i++)
	{
		blk.nonce_ = i;
		bool result = FindCycle(blk.hash(), 20 /*edge bits*/, 42 /* 42 proof size */, cycle);
		HashWriter lStream(SER_GETHASH, PROTOCOL_VERSION);
		std::vector<uint32_t> v(cycle.begin(), cycle.end());
		lStream << v;
		uint256 ch = lStream.GetHash();
		if (result) break;
	}
	printf("cycle size %d\n", cycle.size());

	return true;
}
