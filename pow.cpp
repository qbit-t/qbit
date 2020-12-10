#include "pow.h"
#include "itransactionstore.h"

namespace qbit {

uint32_t getNextWorkRequired(ITransactionStorePtr store, BlockPtr current, uint64_t blockTime) {
	//
	bool fNegative;
	bool fOverflow;
	//
	uint32_t lBitsLimit = 553713663;
	arith_uint256 lTargetLimit;
	lTargetLimit.SetCompact(lBitsLimit, &fNegative, &fOverflow);

	// calculate time (5 blocks)
	uint64_t lBlocks = 24;
	uint64_t lIdx = 0;
	uint256 lBlockId = current->prev();
	uint64_t lBlockTime = current->time();
	arith_uint256 lTarget;
	uint64_t lTimeSpan = 0;
	uint32_t lBits = lBitsLimit;
	//
	std::cout << std::endl;
	while(lIdx++ < lBlocks) {
		//
		BlockHeader lPrev;
		if (store->blockHeader(lBlockId, lPrev)) {
    		//
			arith_uint256 lCurrentTarget;
			lCurrentTarget.SetCompact(lPrev.bits_, &fNegative, &fOverflow);

			if (!lTarget) {
				lTarget = lCurrentTarget;
				//lBlockTime = lPrev.time();
				lBlockId = lPrev.prev();
				lBits = lPrev.bits();
			} else {
				uint64_t lDelta = ((lBlockTime - lPrev.time()) * lCurrentTarget.getdouble()) / lTarget.getdouble();
				lTimeSpan += lDelta;
				std::cout << "\tDelta = " << lDelta << std::endl;
				lBlockTime = lPrev.time();
				lBlockId = lPrev.prev();
			}
		}
	}

	if (!lTimeSpan) return lBitsLimit;

	std::cout << "\nSpan = " << lTimeSpan << ", time = " << current->time() << std::endl;

	uint64_t lTargetTimespan = lIdx * blockTime;
	if (lTimeSpan < lTargetTimespan/3) lTimeSpan = lTargetTimespan/3;
	else if (lTimeSpan > lTargetTimespan*3) lTimeSpan = lTargetTimespan*3;
	//
	std::cout << "\nSpan new = " << lTimeSpan << " " << lTargetTimespan << std::endl;

	//
	lTarget.SetCompact(lBits, &fNegative, &fOverflow);
	// Retarget
	lTarget /= lTargetTimespan;
	lTarget *= lTimeSpan;

	if (lTarget > lTargetLimit) {
		lTarget = lTargetLimit;
	}

	if (!lTarget) {
		std::cout << "\nTarget is ZERO!\n";
		lTarget.SetCompact(lBits, &fNegative, &fOverflow);
	}

	std::cout << "\nTarget = " << lTarget.GetCompact() << std::endl;
	return lTarget.GetCompact();
}

}
