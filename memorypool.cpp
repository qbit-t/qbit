#include "memorypool.h"
#include "log/log.h"
#include "transactionvalidator.h"
#include "txblockbase.h"
#include "txbase.h"
#include "iconsensusmanager.h"
#include "ipeermanager.h"

#include <iostream>

using namespace qbit;

void MemoryPool::PoolStore::addLink(const uint256& from, const uint256& to) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storeMutex_);
	// add
	bool lFound = false;
	std::pair<std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator,
				std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator> lRange = forward_.equal_range(from);
	for (std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator lItem = lRange.first; lItem != lRange.second; lItem++) {
		if (lItem->second == to) { lFound = true; break; }
	}

	if (!lFound) forward_.insert(std::multimap<uint256 /*from*/, uint256 /*to*/>::value_type(from, to));

	lFound = false;
	lRange = reverse_.equal_range(to);

	for (std::multimap<uint256 /*to*/, uint256 /*from*/>::iterator lItem = lRange.first; lItem != lRange.second; lItem++) {
		if (lItem->second == from) { lFound = true; break; }
	}
	
	if (!lFound) reverse_.insert(std::multimap<uint256 /*to*/, uint256 /*from*/>::value_type(to, from));
}

bool MemoryPool::PoolStore::pushTransaction(TransactionContextPtr ctx) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storeMutex_);
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

	// remove from candidates
	candidateTx_.erase(ctx->tx()->id());
	// remove from postponed candidates
	postponedTx_.erase(ctx->tx()->id());
	// push to actual
	tx_[ctx->tx()->id()] = ctx;

	if (gLog().isEnabled(Log::POOL)) gLog().write(Log::STORE, std::string("[STAT(1)]: ") +
		strprintf("tx_ = %d, candidateTx_ = %d, postponedTx_ = %d, %s#", 
			tx_.size(), candidateTx_.size(), postponedTx_.size(), ctx->tx()->chain().toHex().substr(0, 10)));

	return true;
}

void MemoryPool::PoolStore::cleanUp(TransactionContextPtr ctx) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storeMutex_);
	//
	for (std::vector<Transaction::In>::iterator lIn = ctx->tx()->in().begin(); lIn != ctx->tx()->in().end(); lIn++) {
		uint256 lInHash = (*lIn).out().hash();
		usedUtxo_.erase(lInHash);
	}
}

void MemoryPool::PoolStore::remove(TransactionContextPtr ctx) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storeMutex_);
	//
	for (std::vector<Transaction::In>::iterator lIn = ctx->tx()->in().begin(); lIn != ctx->tx()->in().end(); lIn++) {
		uint256 lInHash = (*lIn).out().hash();
		usedUtxo_.erase(lInHash);
		freeUtxo_.erase(lInHash);
	}

	uint32_t lIndex = 0;
	for (std::vector<Transaction::Out>::iterator lOut = ctx->tx()->out().begin(); lOut != ctx->tx()->out().end(); lOut++, lIndex++) {
		Transaction::Link lLink;
		lLink.setChain(ctx->tx()->chain());
		lLink.setAsset(ctx->tx()->out()[lIndex].asset());
		lLink.setTx(ctx->tx()->id());
		lLink.setIndex(lIndex);

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			lLink // link
		);

		uint256 lOutHash = lUTXO->hash();
		usedUtxo_.erase(lOutHash);
		freeUtxo_.erase(lOutHash);
	}

	std::pair<std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator,
				std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator> lRange = reverse_.equal_range(ctx->tx()->id());
	for (std::multimap<uint256 /*to*/, uint256 /*from*/>::iterator lItem = lRange.first; lItem != lRange.second; lItem++) {
		//
		forward_.erase(lItem->second); // clean-up reverse
	}

	forward_.erase(ctx->tx()->id()); // clean-up direct
	reverse_.erase(ctx->tx()->id()); // clean-up direct
	tx_.erase(ctx->tx()->id());

	if (gLog().isEnabled(Log::POOL)) gLog().write(Log::STORE, std::string("[STAT(2)]: ") +
		strprintf("tx_ = %d, candidateTx_ = %d, postponedTx_ = %d, forward_ = %d, reverse_ = %d, usedUtxo_ = %d, freeUtxo_ = %d, ctx->out() = %d, %s#", 
			tx_.size(), candidateTx_.size(), postponedTx_.size(), forward_.size(), reverse_.size(), usedUtxo_.size(), freeUtxo_.size(), ctx->out().size(), ctx->tx()->chain().toHex().substr(0, 10)));
}

bool MemoryPool::PoolStore::pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr ctx) {
	//
	uint256 lHash = utxo->out().hash();
	if (gLog().isEnabled(Log::POOL)) gLog().write(Log::STORE, std::string("[PoolStore::pushUnlinkedOut]: try to push ") +
		strprintf("utxo = %s, tx = %s, ctx = %s", 
			lHash.toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex()));

	boost::unique_lock<boost::recursive_mutex> lLock(storeMutex_);
	//
	if (freeUtxo_.find(lHash) == freeUtxo_.end() && 
		(!pool_->persistentStore()->isUnlinkedOutUsed(lHash) || 
			(pool_->persistentMainStore() && !pool_->persistentMainStore()->isUnlinkedOutUsed(lHash)))) {
		freeUtxo_.insert(std::map<uint256 /*utxo*/, Transaction::UnlinkedOutPtr>::value_type(lHash, utxo));

		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[PoolStore::pushUnlinkedOut]: PUSHED ") +
			strprintf("utxo = %s, tx = %s, ctx = %s", 
				lHash.toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex()));

		if (gLog().isEnabled(Log::POOL)) gLog().write(Log::STORE, std::string("[STAT(3)]: ") +
			strprintf("tx_ = %d, candidateTx_ = %d, postponedTx_ = %d, forward_ = %d, reverse_ = %d, usedUtxo_ = %d, freeUtxo_ = %d, %s#", 
				tx_.size(), candidateTx_.size(), postponedTx_.size(), forward_.size(), reverse_.size(), usedUtxo_.size(), freeUtxo_.size(), ctx->tx()->chain().toHex().substr(0, 10)));

		return true;
	}

	return false;
}

bool MemoryPool::PoolStore::popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx) {
	//
	if (gLog().isEnabled(Log::POOL)) gLog().write(Log::STORE, std::string("[PoolStore::popUnlinkedOut]: try to pop ") +
		strprintf("utxo = %s, tx = ?, ctx = %s", utxo.toHex(), ctx->tx()->hash().toHex()));

	if (gLog().isEnabled(Log::POOL)) gLog().write(Log::STORE, std::string("[STAT(4)]: ") +
		strprintf("tx_ = %d, candidateTx_ = %d, postponedTx_ = %d, forward_ = %d, reverse_ = %d, usedUtxo_ = %d, freeUtxo_ = %d, %s#", 
			tx_.size(), candidateTx_.size(), postponedTx_.size(), forward_.size(), reverse_.size(), usedUtxo_.size(), freeUtxo_.size(), ctx->tx()->chain().toHex().substr(0, 10)));

	//
	bool lExists = false;
	Transaction::UnlinkedOutPtr lFreeUtxoPtr;
	{
		boost::unique_lock<boost::recursive_mutex> lLock(storeMutex_);
		std::map<uint256 /*utxo*/, Transaction::UnlinkedOutPtr>::iterator lFreeUtxo = freeUtxo_.find(utxo);
		lExists = lFreeUtxo != freeUtxo_.end();
		if (lExists) { lFreeUtxoPtr = lFreeUtxo->second; freeUtxo_.erase(lFreeUtxo); }
	}

	if (lExists) {
		// case for mem-pool tx's only
		// usedUtxo_.insert(std::map<uint256, bool>::value_type(utxo, true));

		// add/update link: from -> to
		addLink(lFreeUtxoPtr->out().tx(), ctx->tx()->id());
		// in case of cross-links
		ctx->pushCrossLink(lFreeUtxoPtr->out().tx());

		{
			boost::unique_lock<boost::recursive_mutex> lLock(storeMutex_);

			if (usedUtxo_.find(utxo) == usedUtxo_.end()) {
				usedUtxo_.insert(std::map<uint256, bool>::value_type(utxo, true));
			} else {
				if (ctx->tx()->isValue(lFreeUtxoPtr)) {
					ctx->addError("E_OUT_USED|Output is already used.");
					return false;
				}
			}

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[PoolStore::popUnlinkedOut]: POPPED ") +
				strprintf("utxo = %s, tx = ?, ctx = %s", utxo.toHex(), ctx->tx()->id().toHex()));					

			return true;
		}
	}

	// just check for existence
	// usedUtxo_ already has all used and processed inputs
	Transaction::UnlinkedOutPtr lUtxo = pool_->persistentStore()->findUnlinkedOut(utxo);
	if (!lUtxo && pool_->persistentMainStore()) {
		lUtxo = pool_->persistentMainStore()->findUnlinkedOut(utxo);
	}

	if (lUtxo) {
		// add/update link: from -> to
		addLink(lUtxo->out().tx(), ctx->tx()->id());
	}

	if (!lUtxo) {
		if (pool_->chain() != MainChain::id() && pool_->wallet()->mempoolManager()->popUnlinkedOut(utxo, ctx, pool_)) {
			// add/update link: from -> to
			for (std::set<uint256>::iterator lLink = ctx->crossLinks().begin(); lLink != ctx->crossLinks().end(); lLink++) {
				addLink(*lLink, ctx->tx()->id());
			}
			ctx->clearCrossLinks(); // processed

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[PoolStore::popUnlinkedOut]: POPPED in MAIN ") +
				strprintf("utxo = %s, tx = ?, ctx = %s", utxo.toHex(), ctx->tx()->id().toHex()));					

			// we good?
			return true;
		}
	}

	/* NOTICE: in case if UTXO was NOT found - we should go process here, because of a) linked utxo is not there for the moment, b) if used utxo is invalid - further processing does not allow to be tx become commited */
	return true; // lUtxo != nullptr
}

TransactionPtr MemoryPool::PoolStore::locateTransaction(const uint256& hash) {
	{
		boost::unique_lock<boost::recursive_mutex> lLock(storeMutex_);
		std::map<uint256 /*id*/, TransactionContextPtr /*tx*/>::iterator lTx = tx_.find(hash);
		if (lTx != tx_.end()) return lTx->second->tx();
	}

	TransactionContextPtr lCtx = pool_->wallet()->mempoolManager()->locateTransactionContext(hash);
	if (lCtx) return lCtx->tx();

	TransactionPtr lTxPtr = pool_->persistentStore()->locateTransaction(hash);
	if (lTxPtr == nullptr && pool_->persistentMainStore()) lTxPtr = pool_->persistentMainStore()->locateTransaction(hash);
	return lTxPtr;
}

TransactionContextPtr MemoryPool::PoolStore::locateTransactionContext(const uint256& hash) {
	boost::unique_lock<boost::recursive_mutex> lLock(storeMutex_);
	std::map<uint256 /*id*/, TransactionContextPtr /*tx*/>::iterator lTx = tx_.find(hash);
	if (lTx != tx_.end()) return lTx->second;

	return nullptr;
}

//
// MemoryPool
//

bool MemoryPool::popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx) {
	//
	return poolStore_->popUnlinkedOut(utxo, ctx);
}

bool MemoryPool::isUnlinkedOutUsed(const uint256& utxo) {
	//
	return poolStore_->isUnlinkedOutUsed(utxo);
}

bool MemoryPool::isUnlinkedOutExists(const uint256& utxo) {
	//
	return poolStore_->isUnlinkedOutExists(utxo);
}

TransactionContextPtr MemoryPool::pushTransaction(TransactionPtr tx) {
	//
	TransactionContextPtr lCtx = TransactionContext::instance(tx);
	if (pushTransaction(lCtx))
		return lCtx;
	return nullptr;
}

void MemoryPool::processCandidates() {
	//
	PoolStorePtr lPool = PoolStore::toStore(poolStore_);
	//
	std::list<TransactionContextPtr> lCandidates;
	lPool->candidates(lCandidates);

	if (lCandidates.size()) {
		if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[processCandidates]: re-processing tx candidates..."));

		for (std::list<TransactionContextPtr>::iterator lCandidate = lCandidates.begin(); lCandidate != lCandidates.end(); lCandidate++) {
			//
			if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[processCandidates]: re-process transaction ") + 
					strprintf("%s/%s#", (*lCandidate)->tx()->id().toHex(), (*lCandidate)->tx()->chain().toHex().substr(0, 10)));
			// clean-up
			(*lCandidate)->errors().clear();
			//
			int lReporocessed = (*lCandidate)->incrementReprocessed();
			// try to push
			pushTransaction(*lCandidate);
			// check
			if ((*lCandidate)->errors().size()) {
				for (std::list<std::string>::iterator lErr = (*lCandidate)->errors().begin(); lErr != (*lCandidate)->errors().end(); lErr++) {
					if (gLog().isEnabled(Log::GENERAL_ERROR)) gLog().write(Log::GENERAL_ERROR, std::string("[processCandidates/error]: transaction re-processing ERROR - ") + (*lErr) +
							strprintf(" -> %d/%s/%s#", lReporocessed, (*lCandidate)->tx()->id().toHex(), (*lCandidate)->tx()->chain().toHex().substr(0, 10)));
				}
			}
			// check for timeout
			if ((*lCandidate)->reprocessTimedout()) {
				if (gLog().isEnabled(Log::GENERAL_ERROR)) gLog().write(Log::GENERAL_ERROR, std::string("[processCandidates]: transaction re-processing SKIPPED due to wait timeout") +
						strprintf(" -> %s/%s#", (*lCandidate)->tx()->id().toHex(), (*lCandidate)->tx()->chain().toHex().substr(0, 10)));					
				lPool->removeCandidate(*lCandidate);
			}
		}
	}
}

bool MemoryPool::pushTransaction(TransactionContextPtr ctx) {
	// skip until reindexing; NOTICE: to allow normal processing - restart without '-reindex' switch needed
	if ((consensus_->settings()->reindex() || consensus_->settings()->reindexShard() == chain_) &&
			consensus_->chainState() != IConsensus::SYNCHRONIZED) {
		// declined
		return true;
	}
	//
	if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[pushTransaction]: ") + 
		strprintf("PUSHING transaction: %s/%s#",
			ctx->tx()->hash().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));
	//
	{
		// prelimiary check and store
		PoolStorePtr lPool = PoolStore::toStore(poolStore_);
		if (!lPool->tryPushTransaction(ctx)) {
			if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[pushTransaction]: ") + 
				strprintf("transaction is ALREADY PROCESSING: %s/%s#",
					ctx->tx()->hash().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));
			return false; // already exists
		}

		// check tx extra allowance
		if (!persistentStore_->isAllowed(ctx)) {
			if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[pushTransaction]: ") + 
				strprintf("transaction DECLINED: %s/%s#",
					ctx->tx()->hash().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));

			ctx->addError("Transaction was DECLINED by store extension rules");
			lPool->removeCandidate(ctx); // if any
			return true; // declined
		}

		// 0. set context
		ctx->setContext(TransactionContext::MEMPOOL_COMMIT);

		// 1. check
		if (poolStore_->locateTransaction(ctx->tx()->id())) { 
			if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[pushTransaction]: ") + 
				strprintf("transaction is ALREADY processed: %s/%s#",
					ctx->tx()->hash().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));
			lPool->removeCandidate(ctx); // if any
			return false; // already exists
		}

		// 2. check using processor
		TransactionProcessor lProcessor = TransactionProcessor::general(poolStore_, wallet_, poolEntityStore_);
		if (!lProcessor.process(ctx)) {
			// if tx has references to the missing tx (not yet arrived)
			if (ctx->errorsContains("UNKNOWN_REFTX")) {
				lPool->postponeCandidate(ctx);
			} else {
				lPool->removeCandidate(ctx);
			}

			return true; // check ctx->errors() for details
		}

		// 3. push transaction to the pool store
		poolStore_->pushTransaction(ctx);

		// 4. try to locate functional parent links
		std::list<uint256> lLinks;
		if (/*!ctx->errors().size() &&*/ persistentStore_->locateParents(ctx, lLinks)) {
			//
			boost::unique_lock<boost::recursive_mutex> lLock(mempoolMutex_);
			for (std::list<uint256>::iterator lParent = lLinks.begin(); lParent != lLinks.end(); lParent++) {
				threads_[*lParent].insert(ctx->tx()->id());
				reverseThreads_[ctx->tx()->id()].insert(*lParent);
			}
		}

		//
		{
			boost::unique_lock<boost::recursive_mutex> lLock(mempoolMutex_);
			// 4. add tx to pool map
			if (!ctx->errors().size()) {
				if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[pushTransaction]: ") + 
					strprintf("transaction PUSHED: rate = %d/%s/%s#",
						ctx->feeRate(), ctx->tx()->hash().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));
				//
				std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::iterator
					lEntry = map_.insert(std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::value_type(ctx->feeRate(), ctx->tx()->id()));
				//
				reverseMap_.insert(
					std::map<uint256 /*tx*/, std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::iterator>::value_type
					(
						ctx->tx()->id(),
						lEntry
					)
				);
			} else {
				if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[pushTransaction]: ") + 
					strprintf("transaction NOT PUSHED: rate = %d/%s/%s#",
						ctx->feeRate(), ctx->tx()->hash().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));			
			}

			//
			if (gLog().isEnabled(Log::POOL)) gLog().write(Log::STORE, std::string("[STAT(5)]: ") +
				strprintf("map_ = %d, reverseMap_ = %d, threads_ = %d, reverseThreads_ = %d, confirmedBlocks_ = %d, %s#", 
					map_.size(), reverseMap_.size(), threads_.size(), reverseThreads_.size(), confirmedBlocks_.size(), chain_.toHex().substr(0, 10)));
		}
	}

	return true;
}

bool MemoryPool::isTransactionExists(const uint256& tx) {
	return poolStore_->locateTransactionContext(tx) != nullptr;
}

TransactionPtr MemoryPool::locateTransaction(const uint256& tx) {
	TransactionContextPtr lCtx = poolStore_->locateTransactionContext(tx);
	if (lCtx) return lCtx->tx();
	return nullptr;
}	

TransactionContextPtr MemoryPool::locateTransactionContext(const uint256& tx) {
	TransactionContextPtr lCtx = poolStore_->locateTransactionContext(tx);
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

bool MemoryPool::traverseLeft(TransactionContextPtr root, TxTree& tree, bool& hasExternalOuts) {
	//
	uint256 lId = root->tx()->id();
	TransactionContextPtr lRoot = root;

	std::pair<std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator,
				std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator> lRange = reverse_.equal_range(lId);
	if (lRange.first == lRange.second) {
		if (!(persistentStore_->locateTransactionContext(lId) || 
					(persistentMainStore_ && persistentMainStore_->locateTransactionContext(lId)))) {
			hasExternalOuts = root->tx()->hasOuterIns();
			return false;
		}
	}

	for (std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator lTo = lRange.first; lTo != lRange.second; lTo++) {
		TransactionContextPtr lTx = poolStore_->locateTransactionContext(lTo->second); // TODO: duplicates?
		if (lTx) {
			int lPriority = tree.push(lRoot->tx()->id(), lTx, TxTree::UP);
			if (lPriority < 1000) { // sanity recursion deep
				if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[traverseLeft]: add tx ") + 
						strprintf("%d/%s/%s#", lPriority, lTx->tx()->id().toHex(), chain_.toHex().substr(0, 10)));
				if (!traverseLeft(lTx, tree, hasExternalOuts)) // back-traverse from the upper level
					return false;
			} else {
				return false; // too deep
			}
		} else if (!(persistentStore_->locateTransactionContext(lTo->second) || 
					(persistentMainStore_ && persistentMainStore_->locateTransactionContext(lTo->second)))) {
			hasExternalOuts = root->tx()->hasOuterIns();
			return false;
		}
	}

	return true;
}

bool MemoryPool::traverseRight(TransactionContextPtr root, TxTree& tree) {
	//
	uint256 lId = root->tx()->id();
	TransactionContextPtr lRoot = root;

	std::pair<std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator,
				std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator> lRange = forward_.equal_range(lId);

	for (std::multimap<uint256 /*from*/, uint256 /*to*/>::iterator lTo = lRange.first; lTo != lRange.second; lTo++) {
		TransactionContextPtr lTx = poolStore_->locateTransactionContext(lTo->second); // TODO: duplicates?
		if (lTx) {
			int lPriority = tree.push(lRoot->tx()->id(), lTx, TxTree::DOWN);
			if (lPriority < 1000) { // sanity recursion deep
				if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[traverseRight]: add tx ") + 
						strprintf("%d/%s/%s#", lPriority, lTx->tx()->id().toHex(), chain_.toHex().substr(0, 10)));
				//traverseLeft(lTx, tree); // back-traverse from the upper level
				traverseRight(lTx, tree); // forward travese, recursion!
			} else {
				return false; // too deep
			}
		}
	}

	return true;
}

void MemoryPool::processLocalBlockBaseTx(const uint256& block, const uint256& chain) {
	//
	ITransactionStorePtr lStore = wallet_->storeManager()->locate(chain);
	if (lStore) {
		uint64_t lHeight;
		BlockHeader lBlockHeader;

		if (lStore->blockHeaderHeight(block, lHeight, lBlockHeader)) {
			BlockHeader lCurrentBlockHeader;
			uint64_t lCurrentHeight = lStore->currentHeight(lCurrentBlockHeader);

			uint64_t lConfirms = 0;
			if (lCurrentHeight > lHeight) lConfirms = lCurrentHeight - lHeight;

			// load block and extract ...base (coinbase\base) transaction
			BlockPtr lBlock = lStore->block(block);
			TransactionPtr lTx = *lBlock->transactions().begin();

			// make network block header
			NetworkBlockHeader lNetworkBlockHeader(lBlockHeader, lHeight, lConfirms);

			// push for check
			handleReply(lNetworkBlockHeader, lTx);
		} else {
			if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: block WAS NOT found ") + 
					strprintf("%s/%s#", block.toHex(), chain.toHex().substr(0, 10)));			
		}
	}
}

BlockContextPtr MemoryPool::beginBlock(BlockPtr block) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mempoolMutex_);
	//
	// create context
	BlockContextPtr lCtx = BlockContext::instance(block);

	//
	// transactions tree to commit
	TxTree lTree;

	//
	// local copy
	PoolStorePtr lPool = PoolStore::toStore(poolStore_);
	forward_ = lPool->forward();
	reverse_ = lPool->reverse();

	//
	// traverse
	for (std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::reverse_iterator lEntry = map_.rbegin(); lEntry != map_.rend();) {
		//
		TransactionContextPtr lTx = poolStore_->locateTransactionContext(lEntry->second);
		if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: try to process tx ") + strprintf("%s/%s#", lEntry->second.toHex(), chain_.toHex().substr(0, 10)));

		if (lTx == nullptr) {
			gLog().write(Log::POOL, std::string("[fillBlock]: tx is absent ") + strprintf("%s/%s#", lEntry->second.toHex(), chain_.toHex().substr(0, 10)));
			reverseMap_.erase(lEntry->second);
			map_.erase(std::next(lEntry).base()); // remove from pool if there is no such tx
			continue;
		}

		// if tx ready: tx from another shard/chain, that claiming collected fee
		if (lTx->tx()->type() == Transaction::BLOCKBASE) {
			//
			TxBlockBasePtr lBlockBaseTx = TransactionHelper::to<TxBlockBase>(lTx->tx());
			//
			ConfirmedBlockInfo lBlockInfo;
			uint256 lBlockHash = lBlockBaseTx->blockHeader().hash();
			if (locateConfirmedBlock(lBlockHash, lBlockInfo)) {
				if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: confirmation found for ") + 
						strprintf("tx = %s, %d/%s/%s#", lTx->tx()->id().toHex(), lBlockInfo.blockHeader().confirms(), lBlockHash.toHex(), lBlockInfo.blockHeader().blockHeader().chain().toHex().substr(0, 10)));

				if (lBlockInfo.blockHeader().confirms() >= consensus_->coinbaseMaturity()) {
					// check base tx amount and blockbase amounts
					// 1. extract amount from _base_ and address
					if (lBlockInfo.base()->out().size()) {
						//
						PKey lBaseKey;
						amount_t lBaseAmount;
						TxBasePtr lBaseTx = TransactionHelper::to<TxBase>(lBlockInfo.base());
						if (lBaseTx->extractAddressAndAmount(lBaseKey, lBaseAmount)) {
							// 2. extract amount and address from blockbase
							PKey lBlockBaseKey;
							amount_t lBlockBaseAmount;
							if (lBlockBaseTx->extractAddressAndAmount(lBlockBaseKey, lBlockBaseAmount)) {
								// 3. check address and amount
								if (lBaseKey.id() == lBlockBaseKey.id() && lBaseAmount == lBlockBaseAmount) {
									// we good
									if (!consensus_->settings()->isMiner()) {
										if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: clean-up blockbase transaction - ") + 
												strprintf("%s/%s#", lBlockBaseTx->id().toHex(), lBlockInfo.base()->chain().toHex().substr(0, 10)));
										// remove request
										removeConfirmedBlock(lBlockHash);
										// remove from pool
										removeTx(lEntry->second);
										reverseMap_.erase(lEntry->second);
										map_.erase(std::next(lEntry).base());
										continue;
									} else {
										if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: approving blockbase transaction - ") + 
												strprintf("%s/%s#", lBlockBaseTx->id().toHex(), lBlockInfo.base()->chain().toHex().substr(0, 10)));
										removeConfirmedBlock(lBlockHash);
									}
								} else {
									if (gLog().isEnabled(Log::GENERAL_ERROR)) gLog().write(Log::GENERAL_ERROR, std::string("[fillBlock]: amounts/addresses are NOT EQUALS - ") + 
											strprintf("base(%d/%s), blockbase(%d/%s)", lBaseAmount, lBaseKey.id().toHex(), lBlockBaseAmount, lBlockBaseKey.id().toHex()));
									// remove request
									removeConfirmedBlock(lBlockHash);
									// remove from pool
									removeTx(lEntry->second);
									reverseMap_.erase(lEntry->second);
									map_.erase(std::next(lEntry).base());
									continue;
								}
							} else {
								if (gLog().isEnabled(Log::GENERAL_ERROR)) gLog().write(Log::GENERAL_ERROR, std::string("[fillBlock]: amount for blockbase tx IN UNDEFINED - ") + 
										strprintf("%s/%s#", lBlockBaseTx->id().toHex(), lBlockInfo.base()->chain().toHex().substr(0, 10)));
								// remove request
								removeConfirmedBlock(lBlockHash);
								// remove from pool
								removeTx(lEntry->second);
								reverseMap_.erase(lEntry->second);
								map_.erase(std::next(lEntry).base());
								continue;
							}
						} else {
							if (gLog().isEnabled(Log::GENERAL_ERROR)) gLog().write(Log::GENERAL_ERROR, std::string("[fillBlock]: amount for base tx IN UNDEFINED - ") + 
									strprintf("%s/%s/%s#", lBlockInfo.base()->id().toHex(), lBlockHash.toHex(), lBlockInfo.base()->chain().toHex().substr(0, 10)));
							// remove request
							removeConfirmedBlock(lBlockHash);
							// remove from pool
							removeTx(lEntry->second);
							reverseMap_.erase(lEntry->second);
							map_.erase(std::next(lEntry).base());
							continue;
						}
					}

				} else {
					if (gLog().isEnabled(Log::WARNING)) gLog().write(Log::WARNING, std::string("[fillBlock]: corresponding block is NOT MATURE for ") + 
							strprintf("tx = %s, %d/%s/%s#", lTx->tx()->id().toHex(), lBlockInfo.blockHeader().confirms(), lBlockHash.toHex(), lBlockInfo.blockHeader().blockHeader().chain().toHex().substr(0, 10)));
					// remove request
					removeConfirmedBlock(lBlockHash);
					// make new request
					if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: acquiring block info for ") + 
							strprintf("tx = %s, %s/%s#", lTx->tx()->id().toHex(), lBlockHash.toHex(), lBlockBaseTx->blockHeader().chain().toHex().substr(0, 10)));

					// local node does not support source chain
					if (!consensus_->consensusManager()->locate(lBlockBaseTx->blockHeader().chain())) {
						consensus_->consensusManager()->acquireBlockHeaderWithCoinbase(lBlockHash, lBlockBaseTx->blockHeader().chain(), shared_from_this());
					} else {
						processLocalBlockBaseTx(lBlockHash, lBlockBaseTx->blockHeader().chain());
					}

					lEntry++;
					continue;
				}

			} else {
				// check creation time; we check block time in context of _current_ consensus, because source chain may not exists
				uint64_t lCurrentTime = consensus_->currentTime();
				if (lCurrentTime > lBlockBaseTx->blockHeader().time() && 
					((lCurrentTime - lBlockBaseTx->blockHeader().time()) / (consensus_->blockTime() / 1000)) > consensus_->coinbaseMaturity()) {
					//
					if (gLog().isEnabled(Log::WARNING)) gLog().write(Log::WARNING, std::string("[fillBlock]: remove ORPHAN transaction ") + 
							strprintf("%s, %s/%s#", lTx->tx()->id().toHex(), lBlockHash.toHex(), lBlockBaseTx->blockHeader().chain().toHex().substr(0, 10)));
					// remove from pool
					removeTx(lEntry->second);
					reverseMap_.erase(lEntry->second);
					map_.erase(std::next(lEntry).base());
					continue;
				}

				// acquire bock header, make new request
				if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: acquiring block info for ") + 
						strprintf("tx = %s, ct = %d, bt = %d, maturity = %d, %s/%s#", 
							lTx->tx()->id().toHex(), 
							lCurrentTime,
							lBlockBaseTx->blockHeader().time(),
							((lCurrentTime - lBlockBaseTx->blockHeader().time()) / (consensus_->blockTime() / 1000)),
							lBlockHash.toHex(), lBlockBaseTx->blockHeader().chain().toHex().substr(0, 10)));

				// local node does not support source chain
				if (!consensus_->consensusManager()->locate(lBlockBaseTx->blockHeader().chain())) {
					consensus_->consensusManager()->acquireBlockHeaderWithCoinbase(lBlockHash, lBlockBaseTx->blockHeader().chain(), shared_from_this());
				} else {
					processLocalBlockBaseTx(lBlockHash, lBlockBaseTx->blockHeader().chain());
				}

				lEntry++;
				continue;
			}
		}

		// partial tree
		TxTree lPartialTree;
		int lPriority = lPartialTree.push(uint256(), lTx, TxTree::NEUTRAL); // just put current tx

		if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: add tx ") + 
				strprintf("%d/%s/%s#", lPriority, lTx->tx()->id().toHex(), chain_.toHex().substr(0, 10)));

		// traverse left
		bool lHasExternalOuts;
		if (!traverseLeft(lTx, lPartialTree, lHasExternalOuts)) {
			if (lTx->tx()->hasOuterIns() || lHasExternalOuts) {
				if (gLog().isEnabled(Log::WARNING)) gLog().write(Log::WARNING, std::string("[fillBlock]: transaction has outer inputs, POSTPONING tx ") + 
						strprintf("%s/%s#", lTx->tx()->id().toHex(), chain_.toHex().substr(0, 10)));
				lEntry++; 
				continue;
			}

			if (lTx->tx()->type() != Transaction::BLOCKBASE) {
				gLog().write(Log::GENERAL_ERROR, std::string("[fillBlock]: partial tree has broken for -> \n") + lTx->tx()->toString());
				removeTx(lEntry->second);
				reverseMap_.erase(lEntry->second);
				map_.erase(std::next(lEntry).base()); // remove from pool if there is no such tx
				continue;
			}
		}

		// traverse right
		if (!traverseRight(lTx, lPartialTree)) {
			gLog().write(Log::GENERAL_ERROR, std::string("[fillBlock]: partial tree too deep for -> \n") + lTx->tx()->toString());
			removeTx(lEntry->second);
			reverseMap_.erase(lEntry->second);
			map_.erase(std::next(lEntry).base()); // remove from pool if there is no such tx
			continue;
		}

		// merge trees
		lTree.merge(lPartialTree);

		if (lTree.size() > consensus_->maxBlockSize()) { // sanity, it should be Ok, even if lLimitReached == true
			if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: block limit reached by size ") +
				strprintf("%d/%d/%s#", lTree.size(), consensus_->maxBlockSize(), chain_.toHex().substr(0, 10)));
			break; 
		}

		lEntry++;
	}

	//
	amount_t lFee = 0;
	//
	uint32_t lSize = 0;
	//
	std::set<uint256> lCheck;
	//
	// we have appropriate index - fill the block
	for (std::multimap<int /*priority*/, TransactionContextPtr>::iterator lItem = lTree.queue().begin(); lItem != lTree.queue().end(); lItem++) {
		// 
		lSize += lItem->second->size(); // size is cached in tx context, double check
		if (lSize > consensus_->maxBlockSize()) {
			if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: block limit reached by size ") +
				strprintf("%d/%d/%s#", lSize, consensus_->maxBlockSize(), chain_.toHex().substr(0, 10)));
			break;
		} else {
			//
			if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: push tx ") + 
					strprintf("%d/%s/%s#", lItem->first, lItem->second->tx()->id().toHex(), chain_.toHex().substr(0, 10)));

			if (!lCheck.insert(lItem->second->tx()->id()).second) {
				if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[fillBlock]: try push DOUBLED tx ") + 
						strprintf("%d/%s/%s#", lItem->first, lItem->second->tx()->id().toHex(), chain_.toHex().substr(0, 10)));
			} else { 
				lFee += lItem->second->fee(); 
				lCtx->block()->append(lItem->second->tx()); // push to the block
			}
		}
	}

	// set fee
	lCtx->setFee(lFee);

	// set chain
	lCtx->block()->setChain(chain_);

	// set origin
	lCtx->block()->setOrigin(consensus_->mainKey()->createPKey());

	return lCtx;
}

void MemoryPool::selectTransactions(std::list<uint256>& txs, uint64_t& total, size_t limit) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mempoolMutex_);
	//
	total = map_.size();
	//
	for (std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::reverse_iterator lEntry = map_.rbegin(); lEntry != map_.rend() && txs.size() < limit; lEntry++) {
		//
		txs.push_back(lEntry->second);
	}
}

void MemoryPool::selectTransactions(const uint256& parent, std::list<TransactionContextPtr>& txs) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mempoolMutex_);
	//
	std::map<uint256 /*root*/, std::set<uint256 /*txs*/>>::iterator lRoot = threads_.find(parent);
	if (lRoot != threads_.end()) {
		for (std::set<uint256 /*txs*/>::iterator lTx = lRoot->second.begin(); lTx != lRoot->second.end(); lTx++) {
			//
			TransactionContextPtr lCtx = poolStore_->locateTransactionContext(*lTx);
			if (lCtx) txs.push_back(lCtx);
		}
	}
}

void MemoryPool::commit(BlockContextPtr ctx) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mempoolMutex_);
	for (std::list<BlockContext::_poolEntry>::iterator lEntry = ctx->poolEntries().begin(); lEntry != ctx->poolEntries().end(); lEntry++) {
		reverseMap_.erase((*lEntry)->second);
		map_.erase(std::next(*lEntry).base());
		//
		std::map<uint256 /*tx*/, std::set<uint256 /*root*/>>::iterator lRoot = reverseThreads_.find((*lEntry)->second);
		if (lRoot != reverseThreads_.end()) {
			for (std::set<uint256 /*roots*/>::iterator lTx = lRoot->second.begin(); lTx != lRoot->second.end(); lTx++) {
				//
				std::map<uint256 /*tx*/, std::set<uint256 /*root*/>>::iterator lTop = threads_.find(*lTx);
				if (lTop != threads_.end()) {
					lTop->second.erase((*lEntry)->second);
					if (!lTop->second.size()) threads_.erase(lTop);
				}
			}

			reverseThreads_.erase(lRoot);
		}
	}

	if (gLog().isEnabled(Log::POOL)) gLog().write(Log::STORE, std::string("[STAT(6)]: ") +
		strprintf("map_ = %d, reverseMap_ = %d, threads_ = %d, reverseThreads_ = %d, confirmedBlocks_ = %d, %s#", 
			map_.size(), reverseMap_.size(), threads_.size(), reverseThreads_.size(), confirmedBlocks_.size(), chain_.toHex().substr(0, 10)));
}

void MemoryPool::removeTransactions(BlockPtr block) {
	//
	if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[removeTransactions]: cleaning up mempool for ") + 
		strprintf("%s/%s#", block->blockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));
	//
	PoolStorePtr lPoolStore = PoolStore::toStore(poolStore_);
	PoolEntityStorePtr lPoolEntityStore = PoolEntityStore::toStore(poolEntityStore_);

	if (block->headers().size()) {
		for (HeadersContainer::iterator lTx = block->headers().begin(); lTx != block->headers().end(); lTx++) {
			//
			if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[removeTransactions]: try to remove tx ") + 
				strprintf("%s/%s/%s#", (*lTx).toHex(), block->blockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));

			// clean-up indexes
			{
				boost::unique_lock<boost::recursive_mutex> lLock(mempoolMutex_);
				std::map<uint256 /*tx*/, std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::iterator>::iterator
				lEntry = reverseMap_.find(*lTx);
				if (lEntry != reverseMap_.end()) {
					map_.erase(lEntry->second);
					reverseMap_.erase(lEntry);

					std::map<uint256 /*tx*/, std::set<uint256 /*root*/>>::iterator lRoot = reverseThreads_.find(lEntry->first);
					if (lRoot != reverseThreads_.end()) {
						for (std::set<uint256 /*roots*/>::iterator lInnerTx = lRoot->second.begin(); lInnerTx != lRoot->second.end(); lInnerTx++) {
							//
							std::map<uint256 /*tx*/, std::set<uint256 /*root*/>>::iterator lTop = threads_.find(*lInnerTx);
							if (lTop != threads_.end()) {
								lTop->second.erase(lEntry->first);
								if (!lTop->second.size()) threads_.erase(lTop);
							}
						}

						reverseThreads_.erase(lRoot);
					}

				}
			}

			//
			TransactionContextPtr lCtx = poolStore_->locateTransactionContext(*lTx);
			if (lCtx) {
				if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[removeTransactions]: remove tx ") + 
					strprintf("%s/%s/%s#", lCtx->tx()->hash().toHex(), block->blockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));	
				
				lPoolStore->remove(lCtx);
				lPoolEntityStore->removeEntity(lCtx->tx()->id());

				//
				// NOTICE: massive broadcast may occure - main chain ONLY
				if (chain_ == MainChain::id()) {
					//
					consensus_->consensusManager()->peerManager()->notifyTransaction(lCtx);
				}
			}
		}
	} else {
		for (TransactionsContainer::iterator lTx = block->transactions().begin(); lTx != block->transactions().end(); lTx++) {
			//
			if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[removeTransactions]: try to remove tx ") + 
				strprintf("%s/%s/%s#", (*lTx)->id().toHex(), block->blockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));	

			// clean-up indexes
			{
				boost::unique_lock<boost::recursive_mutex> lLock(mempoolMutex_);
				std::map<uint256 /*tx*/, std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::iterator>::iterator 
				lEntry = reverseMap_.find((*lTx)->id());
				if (lEntry != reverseMap_.end()) {
					map_.erase(lEntry->second);
					reverseMap_.erase(lEntry);

					std::map<uint256 /*tx*/, std::set<uint256 /*root*/>>::iterator lRoot = reverseThreads_.find(lEntry->first);
					if (lRoot != reverseThreads_.end()) {
						for (std::set<uint256 /*roots*/>::iterator lInnerTx = lRoot->second.begin(); lInnerTx != lRoot->second.end(); lInnerTx++) {
							//
							std::map<uint256 /*tx*/, std::set<uint256 /*root*/>>::iterator lTop = threads_.find(*lInnerTx);
							if (lTop != threads_.end()) {
								lTop->second.erase(lEntry->first);
								if (!lTop->second.size()) threads_.erase(lTop);
							}
						}

						reverseThreads_.erase(lRoot);
					}
				}
			}

			//
			TransactionContextPtr lCtx = poolStore_->locateTransactionContext((*lTx)->id());
			if (lCtx) {
				if (gLog().isEnabled(Log::POOL)) gLog().write(Log::POOL, std::string("[removeTransactions]: remove tx ") + 
					strprintf("%s/%s/%s#", lCtx->tx()->hash().toHex(), block->blockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));	
				
				lPoolStore->remove(lCtx);
				lPoolEntityStore->removeEntity(lCtx->tx()->id());

				//
				// NOTICE: massive broadcast may occure - main chain ONLY
				if (chain_ == MainChain::id()) {
					//
					consensus_->consensusManager()->peerManager()->notifyTransaction(lCtx);
				}
			}
		}
	}
}
