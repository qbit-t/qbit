#include "pow.h"

namespace qbit {

uint32_t GetNextWorkRequired(ITransactionStorePtr store, BlockHeader &current) {
	uint32_t bits_limit = 553713663;
	BlockHeader lPrev, lPPrev;
	if(store->blockHeader(current.prev(), lPrev)) {
		if(store->blockHeader(lPrev.prev(), lPPrev)) {
			uint64_t block_found_time = lPrev.time_ - lPPrev.time_;
			uint64_t time_limit = 30;
			double time_shift = (double)block_found_time / (double) time_limit;
			bool fNegative;
    		bool fOverflow;
			arith_uint256 target;
			target.SetCompact(lPrev.bits_, &fNegative, &fOverflow);
			target *= time_shift;

			return target.GetCompact();
		}
	}
	return bits_limit;
}

}
