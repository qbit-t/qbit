#include "pow.h"
#include "itransactionstore.h"

namespace qbit {

uint32_t getNextWorkRequired(ITransactionStorePtr store, BlockPtr current, uint64_t blockTime) {
	//
	uint32_t lBitsLimit = 553713663;
	//
	BlockHeader lPrev, lPPrev;
	if(store->blockHeader(current->prev(), lPrev)) {
		if(store->blockHeader(lPrev.prev(), lPPrev)) {
			//
			uint64_t lBlockFoundTime = lPrev.time_ - lPPrev.time_;
			uint64_t lTimeLimit = blockTime;
			double lTimeShift = (double) lBlockFoundTime / (double) lTimeLimit;
			bool fNegative;
    		bool fOverflow;
    		//
			arith_uint256 lTarget;
			lTarget.SetCompact(lPrev.bits_, &fNegative, &fOverflow);

			/*
			std::cout << "Was work " << lPrev.bits_ << " " << fNegative << " " << fOverflow << std::endl;
			std::cout << "Next work " << lTarget.GetCompact() << std::endl;
			std::cout << "block found time " << lBlockFoundTime << std::endl;
			std::cout << "time limit " << lTimeLimit << std::endl;
			std::cout << "time shift " << lTimeShift << std::endl;
			*/

			uint32_t lDelta = 2;

			if(lTimeShift < 0.5) {
				lTarget /= lDelta;
			} else if(lTimeShift > 2.0) {
				lTarget *= lDelta;
			}

			if (!lTarget) {
				// std::cout << "Target is ZERO!" << std::endl;
				lTarget.SetCompact(lPrev.bits_, &fNegative, &fOverflow);
			}

			// std::cout << "Adjusted next work " << lTarget.GetCompact() << " " << lTarget.GetHex() << std::endl;
			return lTarget.GetCompact();
		}
	}

	return lBitsLimit;
}

}
