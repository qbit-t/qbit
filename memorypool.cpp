#include "memorypool.h"
#include "log/log.h"
#include "transactionvalidator.h"

#include <iostream>

using namespace qbit;

void MemoryPool::PoolStore::addLink(const uint256& from, const uint256& to) {
	// add
	forward_.insert(std::multimap<uint256 /*from*/, uint256 /*to*/>::value_type(from, to));
	reverse_.insert(std::map<uint256 /*to*/, uint256 /*from*/>::value_type(to, from));
}

bool MemoryPool::PoolStore::pushTransaction(TransactionContextPtr ctx) {
	//
	// extract ins and check possible double linking and check pure qbit tx
	for (std::vector<Transaction::In>::iterator lIn = ctx->tx()->in().begin(); lIn != ctx->tx()->in().end(); lIn++) {
		uint256 lInHash = (*lIn).out().hash();
		/*
		if (usedUtxo_.find(lInHash) == usedUtxo_.end()) {
			usedUtxo_.insert(std::map<uint256, bool>::value_type(lInHash, true));
		} else {
			if (ctx->tx()->isValue(Transaction::UnlinkedOut::instance((*lIn).out()))) {
				ctx->addError("E_OUT_USED|Output is already used.");
				return false;
			}
		}
		*/

		freeUtxo_.erase(lInHash);

		if ((*lIn).out().asset() == TxAssetType::qbitAsset()) ctx->setQbitTx(); // mark it
		else ctx->resetQbitTx(); // reset it
	}

	tx_[ctx->tx()->id()] = ctx;

	return true;
}

void MemoryPool::PoolStore::cleanUp(TransactionContextPtr ctx) {
	//
	for (std::vector<Transaction::In>::iterator lIn = ctx->tx()->in().begin(); lIn != ctx->tx()->in().end(); lIn++) {
		uint256 lInHash = (*lIn).out().hash();
		usedUtxo_.erase(lInHash);
	}
}

void MemoryPool::PoolStore::remove(TransactionContextPtr ctx) {
	//
	for (std::vector<Transaction::In>::iterator lIn = ctx->tx()->in().begin(); lIn != ctx->tx()->in().end(); lIn++) {
		uint256 lInHash = (*lIn).out().hash();
		usedUtxo_.erase(lInHash);
		freeUtxo_.erase(lInHash);
	}

	forward_.erase(ctx->tx()->id()); // clean-up
	reverse_.erase(ctx->tx()->id()); // clean-up
	tx_.erase(ctx->tx()->id());
}

bool MemoryPool::PoolStore::pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr) {
	uint256 lHash = utxo->out().hash();
	if (freeUtxo_.find(lHash) == freeUtxo_.end() && 
		(!pool_->persistentStore()->isUnlinkedOutUsed(lHash) || 
			(pool_->persistentMainStore() && !pool_->persistentMainStore()->isUnlinkedOutUsed(lHash)))) {
		freeUtxo_.insert(std::map<uint256 /*utxo*/, Transaction::UnlinkedOutPtr>::value_type(lHash, utxo));
		return true;
	}

	return false;
}

bool MemoryPool::PoolStore::popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx) {
	std::map<uint256 /*utxo*/, Transaction::UnlinkedOutPtr>::iterator lFreeUtxo = freeUtxo_.find(utxo);
	if (lFreeUtxo != freeUtxo_.end()) {
		// case for mem-pool tx's only
		// usedUtxo_.insert(std::map<uint256, bool>::value_type(utxo, true));

		freeUtxo_.erase(lFreeUtxo);

		if (usedUtxo_.find(utxo) == usedUtxo_.end()) {
			usedUtxo_.insert(std::map<uint256, bool>::value_type(utxo, true));
		} else {
			if (ctx->tx()->isValue(lFreeUtxo->second)) {
				ctx->addError("E_OUT_USED|Output is already used.");
				return false;
			}
		}

		return true;
	}

	// just check for existence
	// usedUtxo_ already has all used and processed inputs
	return pool_->persistentStore()->findUnlinkedOut(utxo) != nullptr ||
			(pool_->persistentMainStore() && pool_->persistentMainStore()->findUnlinkedOut(utxo) != nullptr);
}

TransactionPtr MemoryPool::PoolStore::locateTransaction(const uint256& hash) {
	std::map<uint256 /*id*/, TransactionContextPtr /*tx*/>::iterator lTx = tx_.find(hash);
	if (lTx != tx_.end()) return lTx->second->tx();

	TransactionPtr lTxPtr = pool_->persistentStore()->locateTransaction(hash);
	if (lTxPtr == nullptr && pool_->persistentMainStore()) lTxPtr = pool_->persistentMainStore()->locateTransaction(hash);
	return lTxPtr;
}

TransactionContextPtr MemoryPool::PoolStore::locateTransactionContext(const uint256& hash) {
	std::map<uint256 /*id*/, TransactionContextPtr /*tx*/>::iterator lTx = tx_.find(hash);
	if (lTx != tx_.end()) return lTx->second;

	return nullptr;
}

//
// MemoryPool
//

bool MemoryPool::isUnlinkedOutUsed(const uint256& utxo) {
	return poolStore_->isUnlinkedOutUsed(utxo);
}

bool MemoryPool::isUnlinkedOutExists(const uint256& utxo) {
	return poolStore_->isUnlinkedOutExists(utxo);
}

TransactionContextPtr MemoryPool::pushTransaction(TransactionPtr tx) {
	//
	// 0. check
	if (poolStore_->locateTransactionContext(tx->id())) return nullptr; // already exists

	//
	// 1. create ctx
	TransactionContextPtr lCtx = TransactionContext::instance(tx);

	// 2. check using processor
	TransactionProcessor lProcessor = TransactionProcessor::general(poolStore_, wallet_, entityStore_);
	if (!lProcessor.process(lCtx))
		return lCtx; // check ctx->errors() for details

	// 3. push transaction to the pool store
	if (!poolStore_->pushTransaction(lCtx))
		return lCtx; // check ctx->errors() for details

	// 4. add tx to pool map
	if (!lCtx->errors().size()) {
		map_.insert(std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::value_type(lCtx->feeRate(), lCtx->tx()->id()));
	}

	// 5. check if "qbit"
	if (lCtx->qbitTx()) qbitTxs_[lCtx->tx()->id()] = lCtx;

	return lCtx;
}

qunit_t MemoryPool::estimateFeeRateByLimit(TransactionContextPtr ctx, qunit_t feeRateLimit) {
	//
	// 1. try top
	qunit_t lRate = getTop();
	if (lRate < feeRateLimit) return lRate;

	// TODO: need more accurate algo
	// 2. try to estimate?
	return feeRateLimit;
}

qunit_t MemoryPool::estimateFeeRateByBlock(TransactionContextPtr ctx, uint32_t targetBlock) {
	//
	// 1. get top rate
	qunit_t lRate = getTop();

	// 2. first block?
	if (targetBlock == 0) { // first block
		return lRate;
	}

	// TODO: need more heuristic algo
	if (targetBlock == 1) { // 0.85
		lRate /= 0.85;
	} else if (targetBlock == 2) { // 0.50
		lRate /= 0.50;
	} else if (targetBlock == 3) { // 0.25
		lRate /= 0.25;
	} else if (targetBlock >= 4) {
		return QUNIT;
	}

	if (!lRate) return QUNIT;
	return lRate;
}

bool MemoryPool::traverseLeft(TransactionContextPtr root, const TxTree& tree) {
	PoolStorePtr lPool = PoolStore::toStore(poolStore_);
	uint256 lId = root->tx()->id();

	std::map<uint256 /*to*/, uint256 /*from*/>::iterator lEdge;
	while((lEdge = lPool->reverse().find(lId)) != lPool->reverse().end()) {
		lId = lEdge->second;

		// try pool store
		TransactionContextPtr lTx = poolStore_->locateTransactionContext(lId);
		if (lTx) const_cast<TxTree&>(tree).add(lTx);
		else if (persistentStore_->locateTransactionContext(lId) || 
					(persistentMainStore_ && persistentMainStore_->locateTransactionContext(lId))) return true; // done
		else return false;
	}

	return false;
}

bool MemoryPool::traverseRight(TransactionContextPtr root, const TxTree& tree) {
	PoolStorePtr lPool = PoolStore::toStore(poolStore_);
	uint256 lId = root->tx()->id();

	std::pair<std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator,
				std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator> lRange = lPool->forward().equal_range(lId);

	for (std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator lTo = lRange.first; lTo != lRange.second; lTo++) {
		TransactionContextPtr lTx = poolStore_->locateTransactionContext(lTo->second); // TODO: duplicates?
		if (lTx) {
			if (const_cast<TxTree&>(tree).size() + lTx->size() < consensus_->maxBlockSize()) {
				const_cast<TxTree&>(tree).add(lTx);
				traverseRight(lTx, tree);  // recursion!
			} else return true;
		} else return false;
	}

	return false;
}

BlockContextPtr MemoryPool::beginBlock(BlockPtr block) {
	//
	// create context
	BlockContextPtr lCtx = BlockContext::instance(block);

	//
	// current size
	size_t lSize = 0;

	//
	// Size limit reached
	bool lLimitReached = false;

	//
	// traverse
	for (std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::reverse_iterator lEntry = map_.rbegin(); !lLimitReached && lEntry != map_.rend();) {
		// try:
		// 1. pool
		// 2. chain 
		TransactionContextPtr lTx = poolStore_->locateTransactionContext(lEntry->second);
		if (lTx == nullptr) {
			gLog().write(Log::POOL, std::string("[fillBlock]: tx is absent ") + lEntry->second.toHex());
			map_.erase(std::next(lEntry).base()); // remove from pool if there is no such tx
			continue;
		}

		// partial tree
		TxTree lTree;
		lTree.add(lTx); // put current tx

		// traverse left
		if (!traverseLeft(lTx, lTree)) {
			gLog().write(Log::POOL, std::string("[fillBlock]: partial tree has broken for ") + lTx->tx()->toString());
			map_.erase(std::next(lEntry).base()); // remove from pool if there is no such tx
			continue;
		}

		// traverse right
		lLimitReached = traverseRight(lTx, lTree); // up to maxBlockSize

		// calc size
		lSize += lTree.size();

		if (lSize < consensus_->maxBlockSize()) { // sanity, it should be Ok, even if lLimitReached == true
			// fill index
			lCtx->txs().insert(lTree.bundle().begin(), lTree.bundle().end());
			// add pool entry
			lCtx->addPoolEntry(lEntry);
		}

		lEntry++;
	}

	//
	// we have appropriate index - fill the block
	for (std::map<uint256 /*tx*/, TransactionContextPtr /*obj*/>::iterator lItem = lCtx->txs().begin(); lItem != lCtx->txs().end(); lItem++) {
		lCtx->block()->append(lItem->second->tx()); // push to the block
	}

	// set adjusted time
	lCtx->block()->setTime(consensus_->currentTime());

	// set chain
	lCtx->block()->setChain(chain_);

	// set prev block
	lCtx->block()->setPrev(persistentStore_->currentBlockHeader().hash());

	return lCtx;
}

void MemoryPool::commit(BlockContextPtr ctx) {
	//
	PoolStorePtr lPoolStore = PoolStore::toStore(poolStore_);
	for (std::map<uint256 /*tx*/, TransactionContextPtr /*obj*/>::iterator lItem = ctx->txs().begin(); lItem != ctx->txs().end(); lItem++) {
		//lPoolStore->forward().erase(lItem->first); // clean-up
		//lPoolStore->reverse().erase(lItem->first); // clean-up
		//lPoolStore->cleanUp(lItem->second); // clean-up
		lPoolStore->remove(lItem->second);
	}

	for (std::list<BlockContext::_poolEntry>::iterator lEntry = ctx->poolEntries().begin(); lEntry != ctx->poolEntries().end(); lEntry++) {
		qbitTxs_.erase((*lEntry)->second);
		map_.erase(std::next(*lEntry).base());
	}
}

void MemoryPool::removeTransactions(BlockPtr block) {
	PoolStorePtr lPoolStore = PoolStore::toStore(poolStore_);
	for (TransactionsContainer::iterator lTx = block->transactions().begin(); lTx != block->transactions().end(); lTx++) {
		TransactionContextPtr lCtx = poolStore_->locateTransactionContext((*lTx)->id());
		if (lCtx) {
			lPoolStore->remove(lCtx);
		}
	}
}
