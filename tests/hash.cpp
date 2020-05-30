#include "hash.h"
#include "../block.h"
#include "../hash.h"
#include "../hash/cuckoo.h"

using namespace qbit;
using namespace qbit::tests;

bool HashTest::execute() {
	Block blk;
	//blk.bits_ = 0x207fffff;
	blk.bits_ = 0x207fffff;
	//blk.bits_ = 1;
	
	for(int i=0; i<10000; i++)
	{
		std::set<uint32_t> cycle;
		blk.nonce_ = i;
		bool result = FindCycle(blk.hash(), 20 /*edge bits*/, 42 /* 42 proof size */, cycle);
		if(cycle.size() == 0) continue;
		HashWriter lStream(SER_GETHASH, PROTOCOL_VERSION);
		std::vector<uint32_t> v(cycle.begin(), cycle.end());
		lStream << v;
		uint256 ch = lStream.GetHash();

		bool fNegative;
    	bool fOverflow;
		arith_uint256 target;
		target.SetCompact(blk.bits_, &fNegative, &fOverflow);
		target *= 10;
		printf("low target %d \n", target.GetCompact());
		arith_uint256 cycle_hash_arith = UintToArith256(ch);
		
		if (fNegative || target == 0 || fOverflow || target < cycle_hash_arith)
		{
			printf("%d %d %d %d %f\n", cycle.size(), fNegative, fOverflow, blk.bits_, target.getdouble()-cycle_hash_arith.getdouble());
			continue;
		}

		printf("cycle size %d %f\n", cycle.size(), target.getdouble()-cycle_hash_arith.getdouble());

		if (result) break;
	}
	//printf("cycle size %d\n", cycle.size());

	return true;
}
