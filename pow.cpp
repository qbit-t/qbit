#include "pow.h"
#include "itransactionstore.h"

namespace qbit {

uint32_t getNextWorkRequired(ITransactionStorePtr store, BlockPtr current) {
	uint32_t bits_limit = 553713663;
	BlockHeader lPrev, lPPrev;
	if(store->blockHeader(current->prev(), lPrev)) {
		if(store->blockHeader(lPrev.prev(), lPPrev)) {
			uint64_t block_found_time = lPrev.time_ - lPPrev.time_;
			uint64_t time_limit = 10;
			double time_shift = (double)block_found_time / (double) time_limit;
			bool fNegative;
    		bool fOverflow;
			arith_uint256 target;
			target.SetCompact(lPrev.bits_, &fNegative, &fOverflow);

			/*
			std::cout << "Was work " << bits_limit << " prev bits " << lPrev.bits_ << " " << fNegative << " " << fOverflow << std::endl ;
			std::cout << "Next work " << target.GetCompact() << std::endl;
			std::cout << "block found time " << block_found_time << std::endl;
			std::cout << "time limit " << time_limit << std::endl;
			std::cout << "time shift " << time_shift << std::endl;
			*/

			if(time_shift < 0.5) {
				target /= 2;
			} else if(time_shift > 2) {
				target *= 2;
			}

			//target *= block_found_time;
			//target /= time_limit;
			//target *= time_shift;

			/*
			std::cout << "Was work " << bits_limit << " prev bits " << lPrev.bits_ << " " << fNegative << " " << fOverflow << std::endl ;
			std::cout << "Next work " << target.GetCompact() << std::endl;
			*/
			return target.GetCompact();
		}
	}
	return bits_limit;
}

}
