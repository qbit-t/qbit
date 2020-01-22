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

private:
	BlockPtr block_;
	std::list<_poolEntry> poolEntries_;
	std::map<uint256, TransactionContextPtr> txs_;
	size_t height_;
	// errors
	std::list<std::string> errors_;
};

} // qbit

#endif
