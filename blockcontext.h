// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BLOCK_CONTEXT_H
#define QBIT_BLOCK_CONTEXT_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
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
	inline std::list<TransactionContextPtr>& txs() { return txs_; }

	template<typename _iterator>
	void insertReverseTransactions(_iterator begin, _iterator end) {
		while (begin != end) {
			std::pair<std::set<uint256>::iterator, bool> lResult = index_.insert((*begin)->tx()->id());
			if (lResult.second) {
				txs_.insert(txs_.begin(), *begin);
			}
			
			begin++; 
		}
	}

	template<typename _iterator>
	void insertTransactions(_iterator begin, _iterator end) {
		while (begin != end) {
			std::pair<std::set<uint256>::iterator, bool> lResult = index_.insert((*begin)->tx()->id());
			if (lResult.second) {
				txs_.insert(txs_.end(), *begin);
			}
			
			begin++; 
		}
	}

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
		std::list<uint256> lHashes;
		block_->transactionsHashes(lHashes);

		return calculateMerkleRoot(lHashes);
	}

	uint256 calculateMerkleRoot(std::list<uint256>& hashes);

private:
	uint256 calcTree(std::list<uint256>::iterator, std::list<uint256>::iterator);

private:
	BlockPtr block_;
	std::list<_poolEntry> poolEntries_;
	std::list<TransactionContextPtr> txs_;
	std::set<uint256> index_;
	size_t height_;
	amount_t coinbaseAmount_;
	amount_t fee_;

	// errors
	std::list<std::string> errors_;
};

} // qbit

#endif
