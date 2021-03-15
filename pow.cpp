#include "pow.h"
#include "itransactionstore.h"

namespace qbit {

uint32_t getNextWorkRequired(ITransactionStorePtr store, BlockPtr current, uint64_t blockTime) {
	//
	bool fNegative;
	bool fOverflow;

	// minimum difficulty 0xffff
	uint32_t lBitsLimit = 0x2100FFFF; // or 553713663
	arith_uint256 lTargetLimit;
	lTargetLimit.SetCompact(lBitsLimit, &fNegative, &fOverflow);

	// calculate time (24 blocks)
	uint64_t lBlocks = 24 * 4;
	uint64_t lIdx = 0;
	uint256 lBlockId = current->prev();
	uint64_t lBlockTime = current->time(), lLastBlockTime = current->time();
	arith_uint256 lTarget;
	uint32_t lTimeSpan = 0;
	uint32_t lBits = lBitsLimit;

	// calc total time with work done
	while(lIdx < lBlocks) {
		//
		BlockHeader lPrev;
		if (store->blockHeader(lBlockId, lPrev)) {
    		//
			arith_uint256 lCurrentTarget;
			lCurrentTarget.SetCompact(lPrev.bits_, &fNegative, &fOverflow);

			if (!lIdx) {
				if (!lTarget) lTarget = lCurrentTarget;
				lLastBlockTime = lPrev.time();
				lBlockTime = lPrev.time();
				lBlockId = lPrev.prev();
				lBits = lPrev.bits();
			} else {
				uint64_t lDelta = ((lBlockTime - lPrev.time()) * lCurrentTarget.getdouble()) / lTarget.getdouble();
				lTimeSpan += lDelta;
				// std::cout << "\tDelta = " << lDelta << std::endl;
				lBlockTime = lPrev.time();
				lBlockId = lPrev.prev();
			}
		} else break;

		lIdx++;
	}

	//
	// std::cout << "\nSpan = " << lTimeSpan << ", time = " << current->time() << ", mid = " << lTarget.GetCompact() << ", bits = " << lBits << std::endl;

	if (!lTimeSpan) return lBitsLimit;

	uint32_t lTargetTimespan = lIdx * blockTime;
	if (lTimeSpan < lTargetTimespan/3) lTimeSpan = lTargetTimespan/3;
	if (lTimeSpan > lTargetTimespan*6) lTimeSpan = lTargetTimespan*6;
	
	//
	// std::cout << "\nSpan new = " << lTimeSpan << " " << lTargetTimespan << " " << (double)lTimeSpan / (double)lTargetTimespan << std::endl;

	// get last block bits (difficulty)
	lTarget.SetCompact(lBits, &fNegative, &fOverflow);

	// check bounds
	if (lTarget.getdouble() * lTimeSpan / lTargetTimespan > lTargetLimit.getdouble()) {
		// default - minimum difficulty
		lTarget = lTargetLimit;
	} else {
		// retarget
		lTarget /= lTargetTimespan;
		lTarget *= lTimeSpan;

		if (lTarget > lTargetLimit || !lTarget) {
			lTarget = lTargetLimit;
		}
	}

	//
	uint32_t lCompactTarget = lTarget.GetCompact();

	//
	if (gLog().isEnabled(Log::VALIDATOR)) 
		gLog().write(Log::VALIDATOR, strprintf("[validator/miner/pow]: bits = %d, coeff = %f", lCompactTarget, (double)lTimeSpan / (double)lTargetTimespan));

	// std::cout << "\nTarget = " << lTarget.GetCompact() << std::endl;
	return lCompactTarget;
}

}
