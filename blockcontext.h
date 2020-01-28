// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BLOCK_CONTEXT_H
#define QBIT_BLOCK_CONTEXT_H

#include "block.h"
#include "version.h"
#include "transactioncontext.h"

namespace qbit {

class BlockContext;
typedef std::shared_ptr<BlockContext> BlockContextPtr;

class BlockContext {
public:
	typedef std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::reverse_iterator _poolEntry;
public:
	explicit BlockContext() {}
	BlockContext(BlockPtr block) : block_(block) {}

	inline BlockPtr block() { return block_; }
	inline void addPoolEntry(_poolEntry entry) {
		poolEntries_.push_back(entry);
	}
	inline std::list<_poolEntry>& poolEntries() { return poolEntries_; }
	inline std::map<uint256, TransactionContextPtr>& txs() { return txs_; }

	inline static BlockContextPtr instance(BlockPtr block) { return std::make_shared<BlockContext>(block); }

	inline void addError(const std::string& error) { errors_.push_back(error); }
	inline void addErrors(const std::list<std::string>& errors) {
		errors_.insert(errors_.end(), errors.begin(), errors.end());
	}

	inline void setHeight(size_t height) { height_ = height; }
	inline size_t height() { return height_; }

	inline std::list<std::string>& errors() { return errors_; }

	inline void setCoinbaseAmount(amount_t amount) { coinbaseAmount_ = amount; }
	inline amount_t coinbaseAmount() { return coinbaseAmount_; }

	inline void setFee(amount_t fee) { coinbaseAmount_ = fee; }
	inline amount_t fee() { return fee_; }

	uint256 calculateMerkleRoot() {
		std::vector<uint256> lHashes;
		block_->transactionsHashes(lHashes);

		return calculateMerkleRoot(lHashes, mutated_);
	}

	uint256 calculateMerkleRoot(std::vector<uint256>& hashes, bool& mutated) {
		//
		bool lMutation = false;
		//
		while (hashes.size() > 1) {
			for (size_t lPos = 0; lPos + 1 < hashes.size(); lPos += 2) {
				if (hashes[lPos] == hashes[lPos + 1]) lMutation = true;
			}
			
			if (hashes.size() & 1) {
				hashes.push_back(hashes.back());
			}

			HashWriter lStream(SER_GETHASH, PROTOCOL_VERSION);
			lStream.write((char*)hashes[0].begin(), hashes.size() / 2);
			hashes[0] = lStream.hash();			

			hashes.resize(hashes.size() / 2);
		}

		mutated = lMutation;
		if (hashes.size() == 0) return uint256();

		return hashes[0];
	}

private:
	BlockPtr block_;
	std::list<_poolEntry> poolEntries_;
	std::map<uint256, TransactionContextPtr> txs_;
	size_t height_;
	amount_t coinbaseAmount_;
	amount_t fee_;
	bool mutated_ = false;

	// errors
	std::list<std::string> errors_;
};

} // qbit

#endif
