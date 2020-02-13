#include "transactionstore.h"
#include "transactionvalidator.h"
#include "log/log.h"
#include "mkpath.h"

#include <iostream>

using namespace qbit;

TransactionPtr TransactionStore::locateTransaction(const uint256& tx) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[locateTransaction]: try to load transaction ") + 
		strprintf("%s/%s#", 
			tx.toHex(), chain_.toHex().substr(0, 10)));
	//
	TxBlockIndex lIdx;
	if (transactionsIdx_.read(tx, lIdx)) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[locateTransaction]: found index ") + 
			strprintf("block(%d/%s) for %s/%s#", 
				lIdx.index(), lIdx.block().toHex(), tx.toHex(), chain_.toHex().substr(0, 10)));

		BlockTransactionsPtr lTransactions = transactions_.read(lIdx.block());
		if (lTransactions) {
			if (lIdx.index() < lTransactions->transactions().size())
				return lTransactions->transactions()[lIdx.index()];
			else {
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[locateTransaction]: transaction index not found for ") + 
					strprintf("%s/%s/%s#", 
						tx.toHex(), lIdx.block().toHex(), chain_.toHex().substr(0, 10)));
			}
		} else {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[locateTransaction]: transactions not found for block ") + 
				strprintf("%s", lIdx.block().toHex()));
		}
	} else {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[locateTransaction]: transaction NOT FOUND - ") + 
			strprintf("%s/%s#", 
				tx.toHex(), chain_.toHex().substr(0, 10)));
	}

	return nullptr;
}

TransactionContextPtr TransactionStore::locateTransactionContext(const uint256& tx) {
	TransactionPtr lTx = locateTransaction(tx);
	if (lTx) return TransactionContext::instance(lTx);

	return nullptr;
}

bool TransactionStore::processBlockTransactions(ITransactionStorePtr store, BlockContextPtr ctx, BlockTransactionsPtr transactions, size_t approxHeight, bool processWallet) {
	//
	// make context block store and wallet store
	ITransactionStorePtr lTransactionStore = store;
	IWalletPtr lWallet = TxWalletStore::instance(wallet_, std::static_pointer_cast<ITransactionStore>(shared_from_this()));
	BlockContextPtr lBlockCtx = ctx;

	// check merkle root
	std::list<uint256> lHashes;
	transactions->transactionsHashes(lHashes);

	uint256 lRoot = ctx->calculateMerkleRoot(lHashes);
	if (ctx->block()->blockHeader().root() != lRoot) {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlockTransactions]: merkle root is NOT VALID ") +
			strprintf("header = %s, transactions = %s, %s/%s#", 
				ctx->block()->blockHeader().root().toHex(), lRoot.toHex(), ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));
		return false;
	}

	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlockTransactions]: ") +
			strprintf("merkle root = %s, %s/%s#", 
				lRoot.toHex(), ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));

	//
	// process block under local context block&wallet
	bool lHasErrors = false;
	// 
	amount_t lCoinBaseAmount = 0;
	amount_t lFeeAmount = 0;
	//
	TransactionProcessor lProcessor = TransactionProcessor::general(lTransactionStore, lWallet, std::static_pointer_cast<IEntityStore>(shared_from_this()));
	for(TransactionsContainer::iterator lTx = transactions->transactions().begin(); lTx != transactions->transactions().end(); lTx++) {
		TransactionContextPtr lCtx = TransactionContext::instance(*lTx, 
			(processWallet ? TransactionContext::STORE_COMMIT : TransactionContext::STORE_PUSH));
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlockTransactions]: processing tx ") +
			strprintf("%s/%s/%s#", lCtx->tx()->id().toHex(), ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));
		//
		if (!lProcessor.process(lCtx)) {
			lHasErrors = true;
			for (std::list<std::string>::iterator lErr = lCtx->errors().begin(); lErr != lCtx->errors().end(); lErr++) {
				gLog().write(Log::ERROR, std::string("[processBlockTransactions/error]: ") + (*lErr));
			}

			lBlockCtx->addErrors(lCtx->errors());
		} else {
			lTransactionStore->pushTransaction(lCtx);

			// if coinbase
			if (lCtx->tx()->type() == Transaction::COINBASE) {
				if (!lCoinBaseAmount) {
					lCoinBaseAmount = lCtx->amount();
				} else {
					lHasErrors = true;
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlockTransactions]: COINBASE already exists ") +
						strprintf("%s/%s/%s#", lCtx->tx()->id().toHex(), ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));
					lBlockCtx->addError(std::string("COINBASE already exists ") +
						strprintf("%s/%s/%s#", lCtx->tx()->id().toHex(), ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));
				}
			} else {
				lFeeAmount += lCtx->fee();
			}
		}
	}

	//
	// check and commit
	if (!lHasErrors) {
		//
		ctx->setCoinbaseAmount(lCoinBaseAmount);
		ctx->setFee(lFeeAmount);

		// check coinbase/fee consistence
		IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate(chain_);
		if (lMempool) {
			if (approxHeight && !lMempool->consensus()->checkBalance(lCoinBaseAmount, lFeeAmount, approxHeight)) {
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlockTransactions]: coinbase AMOUNT/FEE is not consistent ") +
					strprintf("amount = %d, fee = %d, %s/%s#", lCoinBaseAmount, lFeeAmount, ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));
				return false;
			}
		} else {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlockTransactions]: mempool was not found ") +
				strprintf("%s/%s#", ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));
			return false;
		}

		//
		// commit transaction store changes
		TxBlockStorePtr lBlockStore = std::static_pointer_cast<TxBlockStore>(lTransactionStore);
		for (std::list<TxBlockAction>::iterator lAction = lBlockStore->actions().begin(); lAction != lBlockStore->actions().end(); lAction++) {
			if (lAction->action() == TxBlockAction::PUSH) {
				if (!pushUnlinkedOut(lAction->utxoPtr(), lAction->ctx())) {
					gLog().write(Log::ERROR, std::string("[processBlockTransactions/push/error]: Block with utxo inconsistency - ") + 
						strprintf("push: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));

					lBlockCtx->addError("Block with utxo inconsistency - " +
						strprintf("pop: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));
					return false;
				}
			} else if (lAction->action() == TxBlockAction::POP) {
				if (!popUnlinkedOut(lAction->utxo(), lAction->ctx())) {
					gLog().write(Log::ERROR, std::string("[processBlockTransactions/pop/error]: Block with utxo inconsistency - ") + 
						strprintf("pop: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));

					lBlockCtx->addError("Block with utxo inconsistency - " +
						strprintf("pop: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));
					return false;
				}
			}
		}

		//
		// commit local wallet changes
		TxWalletStorePtr lWalletStore = std::static_pointer_cast<TxWalletStore>(lWallet);
		for (std::list<TxBlockAction>::iterator lAction = lWalletStore->actions().begin(); lAction != lWalletStore->actions().end(); lAction++) {
			//
			if ((processWallet || lAction->ctx()->tx()->type() == Transaction::COINBASE) && 
				lAction->action() == TxBlockAction::PUSH) {
				if (!wallet_->pushUnlinkedOut(lAction->utxoPtr(), lAction->ctx())) {
					gLog().write(Log::ERROR, std::string("[processBlockTransactions/wallet/push/error]: Block with utxo inconsistency - ") + 
						strprintf("push: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));

					lBlockCtx->addError("Block/pushWallet with utxo inconsistency - " +
						strprintf("push: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));							
					return false;					
				}
			}

			if (processWallet && lAction->action() == TxBlockAction::POP) {
				if (!wallet_->popUnlinkedOut(lAction->utxo(), lAction->ctx())) {
					gLog().write(Log::ERROR, std::string("[processBlockTransactions/wallet/pop/error]: Block with utxo inconsistency - ") + 
						strprintf("pop: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));

					lBlockCtx->addError("Block/popWallet with utxo inconsistency - " +
						strprintf("pop: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));
					return false;
				}
			}
		}

		return true;
	}

	return false;
}

bool TransactionStore::processBlock(BlockContextPtr ctx, size_t& height, bool processWallet) {
	//
	size_t lHeight = top();
	BlockTransactionsPtr lTransactions = ctx->block()->blockTransactions();
	ITransactionStorePtr lTransactionStore = TxBlockStore::instance(std::static_pointer_cast<ITransactionStore>(shared_from_this()));
	//
	if (processBlockTransactions(lTransactionStore, ctx, lTransactions, lHeight+1 /*next*/, processWallet)) {
		//
		// --- NO CHANGES to the block header or block data ---
		//

		//
		// write block data
		uint256 lHash = ctx->block()->hash();
		BlockHeader lHeader = ctx->block()->blockHeader();
		headers_.write(lHash, lHeader);
		transactions_.write(lHash, lTransactions);

		//
		// indexes
		//

		//
		// tx to block|index
		uint32_t lIndex = 0;
		for(TransactionsContainer::iterator lTx = ctx->block()->transactions().begin(); lTx != ctx->block()->transactions().end(); lTx++) {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlock]: writing index ") +
				strprintf("%s/block(%d/%s)/%s#", (*lTx)->id().toHex(), lIndex, lHash.toHex(), chain_.toHex().substr(0, 10)));
			//
			transactionsIdx_.write((*lTx)->id(), TxBlockIndex(lHash, lIndex++));
		}

		//
		// block to utxo
		TxBlockStorePtr lIndexBlockStore = std::static_pointer_cast<TxBlockStore>(lTransactionStore);
		for (std::list<TxBlockAction>::iterator lAction = lIndexBlockStore->actions().begin(); lAction != lIndexBlockStore->actions().end(); lAction++) {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlock]: writing block action ") +
				strprintf("%s - %s/%s/%s#", lAction->utxo().toHex(), (lAction->action() == TxBlockAction::PUSH?"PUSH":"POP"), lHash.toHex(), chain_.toHex().substr(0, 10)));

			blockUtxoIdx_.write(lHash, *lAction);
			if (lAction->action() == TxBlockAction::PUSH) utxoBlock_.write(lAction->utxo(), lHash);
			else utxoBlock_.remove(lAction->utxo());
		}

		// mark as last
		writeLastBlock(lHash);

		// new height
		height = pushNewHeight(lHash);

		// ok
		return true;
	}

	return false;
}

bool TransactionStore::transactionHeight(const uint256& tx, size_t& height, bool& coinbase) {
	TxBlockIndex lIndex;
	if (transactionsIdx_.read(tx, lIndex)) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
		std::map<uint256, size_t>::iterator lHeight = blockMap_.find(lIndex.block());
		if (lHeight != blockMap_.end()) {
			if (!lIndex.index()) coinbase = true;
			else coinbase = false;
			height = lHeight->second;
			return true;
		}
	}

	return false;
}

bool TransactionStore::blockHeight(const uint256& block, size_t& height) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	std::map<uint256, size_t>::iterator lHeight = blockMap_.find(block);
	if (lHeight != blockMap_.end()) {
		height = lHeight->second;
		return true;
	}

	return false;
}

bool TransactionStore::blockHeaderHeight(const uint256& block, size_t& height, BlockHeader& header) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	std::map<uint256, size_t>::iterator lHeight = blockMap_.find(block);
	if (lHeight != blockMap_.end()) {
		height = lHeight->second;
		return blockHeader(block, header);
	}

	return false;
}

bool TransactionStore::setLastBlock(const uint256& block) {
	BlockHeader lHeader;
	if (headers_.read(block, lHeader)) {
		writeLastBlock(block);
		return true;
	}

	return false;
}

bool TransactionStore::resyncHeight() {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	//
	heightMap_.clear();

	BlockHeader lHeader;
	uint256 lHash = lastBlock_;
	size_t lIndex = 0;

	std::list<uint256> lSeq;
	while (headers_.read(lHash, lHeader)) {
		lSeq.push_back(lHash);
		lHash = lHeader.prev();
	}

	for (std::list<uint256>::reverse_iterator lIter = lSeq.rbegin(); lIter != lSeq.rend(); lIter++) {
		heightMap_.insert(std::map<size_t, uint256>::value_type(++lIndex, *lIter));
		blockMap_.insert(std::map<uint256, size_t>::value_type(*lIter, lIndex));

		//if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight]: add ") + 
		//	strprintf("height = %d, block = %s, chain = %s#", lIndex, (*lIter).toHex(), chain_.toHex().substr(0, 10)));
	}

	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight]: current ") + strprintf("height = %d, block = %s, chain = %s#", lIndex, lastBlock_.toHex(), chain_.toHex().substr(0, 10)));

	return true;
}

size_t TransactionStore::calcHeight(const uint256& from) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);

	//
	BlockHeader lHeader;
	uint256 lHash = from;
	size_t lIndex = 0;

	while (headers_.read(lHash, lHeader)) {
		lIndex++;
		lHash = lHeader.prev();
	}

	return lIndex;
}

//
// [..)
void TransactionStore::removeBlocks(const uint256& from, const uint256& to, bool removeData) {
	// remove indexed data
	uint256 lHash = from;
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
		strprintf("removing blocks data [%s-%s]/%s#", from.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));		
	//
	BlockHeader lHeader;
	while(lHash != to && headers_.read(lHash, lHeader)) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
			strprintf("removing block data for %s/%s#", lHash.toHex(), chain_.toHex().substr(0, 10)));		
		// begin transaction
		db::DbMultiContainer<uint256 /*block*/, TxBlockAction /*utxo action*/>::Transaction lBlockIdxTransaction = blockUtxoIdx_.transaction();
		// prepare reverse queue
		std::list<TxBlockAction> lQueue;		
		// iterate
		for (db::DbMultiContainer<uint256 /*block*/, TxBlockAction /*utxo action*/>::Iterator lUtxo = blockUtxoIdx_.find(lHash); lUtxo.valid(); ++lUtxo) {
			lQueue.push_back(*lUtxo);
			// mark to remove
			lBlockIdxTransaction.remove(lUtxo);
		}
		// reverse traverse
		for (std::list<TxBlockAction>::reverse_iterator lUtxo = lQueue.rbegin(); lUtxo != lQueue.rend(); lUtxo++) {
			// reconstruct utxo
			TxBlockAction lAction = *lUtxo;
			if (lAction.action() == TxBlockAction::PUSH) {
				// remove pushed utxo
				Transaction::UnlinkedOut lUtxoObj;
				if (utxo_.read(lAction.utxo(), lUtxoObj)) {
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
						strprintf("remove utxo = %s, tx = %s/%s#", lAction.utxo().toHex(), lUtxoObj.out().tx().toHex(), chain_.toHex().substr(0, 10)));

					utxo_.remove(lAction.utxo());
					utxoBlock_.remove(lAction.utxo()); // just for push
					addressAssetUtxoIdx_.remove(lUtxoObj.address().id(), lUtxoObj.out().asset(), lAction.utxo());
				} else {
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
						strprintf("utxo was NOT FOUND - %s/%s#", lAction.utxo().toHex(), chain_.toHex().substr(0, 10)));
				}
			} else {
				Transaction::UnlinkedOut lUtxoObj;
				if (ltxo_.read(lAction.utxo(), lUtxoObj)) {
					utxo_.write(lAction.utxo(), lUtxoObj);
					addressAssetUtxoIdx_.write(lUtxoObj.address().id(), lUtxoObj.out().asset(), lAction.utxo(), lUtxoObj.out().tx());
					utxoBlock_.write(lAction.utxo(), lHash);
					ltxo_.remove(lAction.utxo());

					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
						strprintf("remove/add ltxo/utxo = %s, tx = %s/%s#", lAction.utxo().toHex(), lUtxoObj.out().tx().toHex(), chain_.toHex().substr(0, 10)));
				} else {
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
						strprintf("ltxo was NOT FOUND - %s/%s#", lAction.utxo().toHex(), chain_.toHex().substr(0, 10)));
				}
			}
		}

		// commit changes
		lBlockIdxTransaction.commit();

		// clear transactions
		BlockTransactionsPtr lTransactions = transactions_.read(lHash);
		if (lTransactions) {
			for(TransactionsContainer::iterator lTx = lTransactions->transactions().begin(); lTx != lTransactions->transactions().end(); lTx++) {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
					strprintf("removing tx index %s/%s/%s#", (*lTx)->id().toHex(), lHash.toHex(), chain_.toHex().substr(0, 10)));

				transactionsIdx_.remove((*lTx)->id());

				if ((*lTx)->isEntity() && (*lTx)->entityName() == Entity::emptyName()) {
					entities_.remove((*lTx)->entityName());
				}
			}
		}

		// remove data
		if (removeData) {
			headers_.remove(lHash);
			transactions_.remove(lHash);
		}

		// next
		lHash = lHeader.prev();
	}	
}

//
// interval [..)
void TransactionStore::remove(const uint256& from, const uint256& to) {
}

//
// interval [..)
void TransactionStore::erase(const uint256& from, const uint256& to) {
}

//
// interval [..)
bool TransactionStore::processBlocks(const uint256& from, const uint256& to, std::list<BlockContextPtr>& ctxs) {
	//
	std::list<BlockHeader> lHeadersSeq;
	uint256 lHash = from;
	IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate(chain_);
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks]: ") +
		strprintf("processing blocks data [%s-%s]/%s#", from.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));		
	//
	BlockHeader lHeader;
	while(lHash != to && headers_.read(lHash, lHeader)) {
		// check sequence consistency
		if (lMempool && !lMempool->consensus()->checkSequenceConsistency(lHeader)) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks]: check sequence consistency FAILED ") +
				strprintf("block = %s, prev = %s, chain = %s#", lHash.toHex(), lHeader.prev().toHex(), chain_.toHex().substr(0, 10)));
			return false;
		}

		//
		lHeadersSeq.push_back(lHeader);
		// next
		lHash = lHeader.prev();
	}

	// traverse
	for (std::list<BlockHeader>::reverse_iterator lBlock = lHeadersSeq.rbegin(); lBlock != lHeadersSeq.rend(); lBlock++) {
		//
		BlockContextPtr lBlockCtx = BlockContext::instance(Block::instance(*lBlock)); // just header without transactions attached
		uint256 lBlockHash = lBlockCtx->block()->hash();
		BlockTransactionsPtr lTransactions = transactions_.read(lBlockHash);
		lBlockCtx->block()->append(lTransactions); // reconstruct block consistensy
		ctxs.push_back(lBlockCtx);

		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks]: processing block ") +
			strprintf("%s/%s#", lBlockHash.toHex(), chain_.toHex().substr(0, 10)));

		//
		// process txs
		ITransactionStorePtr lTransactionStore = TxBlockStore::instance(std::static_pointer_cast<ITransactionStore>(shared_from_this()));
		if (processBlockTransactions(lTransactionStore, lBlockCtx, lTransactions, 0 /*just re-sync*/, true /*process wallet*/)) {
			//
			// tx to block|index
			uint32_t lIndex = 0;
			for(TransactionsContainer::iterator lTx = lTransactions->transactions().begin(); lTx != lTransactions->transactions().end(); lTx++) {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks]: writing index ") +
					strprintf("%s/block(%d/%s)/%s#", (*lTx)->id().toHex(), lIndex, lBlockHash.toHex(), chain_.toHex().substr(0, 10)));
				//
				transactionsIdx_.write((*lTx)->id(), TxBlockIndex(lBlockHash, lIndex++));
			}

			//
			// block to utxo
			TxBlockStorePtr lIndexBlockStore = std::static_pointer_cast<TxBlockStore>(lTransactionStore);
			for (std::list<TxBlockAction>::iterator lAction = lIndexBlockStore->actions().begin(); lAction != lIndexBlockStore->actions().end(); lAction++) {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks]: writing block action ") +
					strprintf("%s - %s/%s/%s#", lAction->utxo().toHex(), (lAction->action() == TxBlockAction::PUSH?"PUSH":"POP"), lBlockHash.toHex(), chain_.toHex().substr(0, 10)));

				blockUtxoIdx_.write(lBlockHash, *lAction);
				if (lAction->action() == TxBlockAction::PUSH) utxoBlock_.write(lAction->utxo(), lBlockHash);
				else utxoBlock_.remove(lAction->utxo());
			}
		} else {
			return false;
		}
	}

	return true;
}

size_t TransactionStore::currentHeight(BlockHeader& block) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	if (heightMap_.size()) {
		std::map<size_t, uint256>::reverse_iterator lHeader = heightMap_.rbegin();
		if (headers_.read(lHeader->second, block)) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[currentHeight]: ") + strprintf("%d/%s/%s#", lHeader->first, block.hash().toHex(), chain_.toHex().substr(0, 10)));
			return lHeader->first;
		}
	}

	return 0;
}

size_t TransactionStore::top() {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	if (heightMap_.size()) {
		std::map<size_t, uint256>::reverse_iterator lHeader = heightMap_.rbegin();
		return lHeader->first;
	}

	return 0;
}

size_t TransactionStore::pushNewHeight(const uint256& block) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	size_t lHeight = 1;
	if (heightMap_.size()) {
		lHeight = heightMap_.rbegin()->first+1;
	}

	std::pair<std::map<size_t, uint256>::iterator, bool> lNewHeight = 
		heightMap_.insert(std::map<size_t, uint256>::value_type(lHeight, block));
	blockMap_.insert(std::map<uint256, size_t>::value_type(block, lHeight));

	return lNewHeight.first->first;
}

BlockHeader TransactionStore::currentBlockHeader() {
	//
	uint256 lHash = lastBlock_;

	BlockHeader lHeader;
	if (headers_.read(lHash, lHeader)) {
		return lHeader;
	}

	return BlockHeader();
}

bool TransactionStore::commitBlock(BlockContextPtr ctx, size_t& height) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageCommitMutex_);
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[commitBlock]: COMMITING block ") + 
		strprintf("%s/%s#", ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));

	BlockHeader lCurrentHeader = currentBlockHeader();
	if (lCurrentHeader.hash() != ctx->block()->blockHeader().prev()) {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[commitBlock]: WRONG commit sequence: ") + 
			strprintf("current = %s, proposed = %s, prev = %s/%s#", lCurrentHeader.hash().toHex(),
				ctx->block()->hash().toHex(), ctx->block()->blockHeader().prev().toHex(), chain_.toHex().substr(0, 10)));
		return false;
	}

	bool lResult = processBlock(ctx, height, false);

	//
	if (lResult)
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[commitBlock]: block COMMITED - ") + 
			strprintf("%s/%s#", ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));
	else
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[commitBlock]: block SKIPPED - ") + 
			strprintf("%s/%s#", ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));

	return lResult;	
}

BlockContextPtr TransactionStore::pushBlock(BlockPtr block) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageCommitMutex_);
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushBlock]: PUSHING block ") + 
		strprintf("%s/%s#", block->hash().toHex(), chain_.toHex().substr(0, 10)));

	BlockHeader lCurrentHeader = currentBlockHeader();
	if (lCurrentHeader.hash() != block->blockHeader().prev()) {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushBlock]: WRONG push sequence: ") + 
			strprintf("current = %s, proposed = %s, prev = %s/%s#", lCurrentHeader.hash().toHex(),
				block->hash().toHex(), block->blockHeader().prev().toHex(), chain_.toHex().substr(0, 10)));
		// remove
		dequeueBlock(block->hash());
		return nullptr;
	}

	//
	bool lResult;
	size_t lHeight;
	BlockContextPtr lBlockCtx = BlockContext::instance(block);	
	lResult = processBlock(lBlockCtx, lHeight, true);
	if (lResult) {
		lBlockCtx->setHeight(lHeight);
	}

	// remove
	dequeueBlock(lBlockCtx->block()->hash());

	//
	if (lResult) 
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushBlock]: block PUSHED ") + 
			strprintf("%s/%s#", block->hash().toHex(), chain_.toHex().substr(0, 10)));
	else
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushBlock]: block SKIPPED ") + 
			strprintf("%s/%s#", block->hash().toHex(), chain_.toHex().substr(0, 10)));

	return lBlockCtx;
}

void TransactionStore::writeLastBlock(const uint256& block) {
	if (!opened_) return;

	lastBlock_ = block;
	std::string lHex = block.toHex();
	workingSettings_.write("lastBlock", lHex);
}

BlockPtr TransactionStore::block(size_t height) {
	//
	BlockHeader lHeader;
	uint256 lHash;
	{
		boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
		std::map<size_t, uint256>::iterator lHeight = heightMap_.find(height);
		if (lHeight != heightMap_.end()) {
			lHash = lHeight->second;
		} else {
			return nullptr;
		}
	}

	if (headers_.read(lHash, lHeader)) {
		BlockTransactionsPtr lTransactions = transactions_.read(lHash);

		return Block::instance(lHeader, lTransactions);
	}

	return nullptr;
}

BlockPtr TransactionStore::block(const uint256& id) {
	//
	BlockHeader lHeader;
	uint256 lHash = id;

	if (headers_.read(lHash, lHeader)) {
		BlockTransactionsPtr lTransactions = transactions_.read(lHash);

		return Block::instance(lHeader, lTransactions);
	}

	return nullptr;	
}

bool TransactionStore::open() {
	//
	if (!opened_) { 
		try {
			gLog().write(Log::INFO, std::string("[open]: opening store for ") + strprintf("%s", chain_.toHex()));

			if (mkpath(std::string(settings_->dataPath() + "/" + chain_.toHex() + "/indexes").c_str(), 0777)) return false;

			workingSettings_.open();
			headers_.open();
			transactions_.open();
			utxo_.open();
			ltxo_.open();
			entities_.open();
			blockUtxoIdx_.open();
			utxoBlock_.open();
			transactionsIdx_.open();
			addressAssetUtxoIdx_.open();

			std::string lLastBlock;
			if (workingSettings_.read("lastBlock", lLastBlock)) {
				lastBlock_.setHex(lLastBlock);
			}

			gLog().write(Log::INFO, std::string("[open]: lastBlock = ") + strprintf("%s/%s#", lastBlock_.toHex(), chain_.toHex().substr(0, 10)));

			resyncHeight();

			opened_ = true;
		}
		catch(const std::exception& ex) {
			gLog().write(Log::ERROR, std::string("[open/error]: ") + ex.what());
			return false;
		}
	}

	return opened_;
}

bool TransactionStore::close() {
	//
	gLog().write(Log::INFO, std::string("[close]: closing storage for ") + strprintf("%s#...", chain_.toHex().substr(0, 10)));

	settings_.reset();
	wallet_.reset();
	
	workingSettings_.close();
	headers_.close();
	transactions_.close();
	utxo_.close();	
	ltxo_.close();	
	entities_.close();
	transactionsIdx_.close();
	addressAssetUtxoIdx_.close();
	blockUtxoIdx_.close();
	utxoBlock_.close();

	opened_ = false;

	return true;
}

bool TransactionStore::pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr ctx) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushUnlinkedOut]: try to push ") +
		strprintf("utxo = %s, tx = %s, ctx = %s", 
			utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex()));

	uint256 lUtxo = utxo->hash();
	if (!isUnlinkedOutUsed(lUtxo) && !findUnlinkedOut(lUtxo)) {
		// update utxo db
		utxo_.write(lUtxo, *utxo);
		// update indexes
		addressAssetUtxoIdx_.write(utxo->address().id(), utxo->out().asset(), lUtxo, utxo->out().tx());
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushUnlinkedOut]: PUSHED ") +
			strprintf("utxo = %s, tx = %s, ctx = %s", 
				lUtxo.toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex()));
		return true;
	}

	return false;
}

bool TransactionStore::popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[popUnlinkedOut]: try to pop ") +
		strprintf("utxo = %s, tx = ?, ctx = %s", utxo.toHex(), ctx->tx()->hash().toHex()));

	Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(utxo); 
	if (!isUnlinkedOutUsed(utxo) && lUtxo) {
		// update utxo db
		utxo_.remove(utxo);
		// update ltxo db
		ltxo_.write(utxo, *lUtxo);
		// cleanup index
		addressAssetUtxoIdx_.remove(lUtxo->address().id(), lUtxo->out().asset(), utxo);
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[popUnlinkedOut]: POPPED ") +
			strprintf("utxo = %s, tx = %s, ctx = %s", 
				utxo.toHex(), lUtxo->out().tx().toHex(), ctx->tx()->id().toHex()));		
		return true;
	}

	return false;
}

void TransactionStore::addLink(const uint256& /*from*/, const uint256& /*to*/) {
	// stub
}

bool TransactionStore::isUnlinkedOutUsed(const uint256& ltxo) {
	// 
	Transaction::UnlinkedOut lUtxo;
	if (ltxo_.read(ltxo, lUtxo)) {
		return true;
	}

	return false;
}

bool TransactionStore::isUnlinkedOutExists(const uint256& utxo) {
	//
	Transaction::UnlinkedOut lUtxo;
	if (utxo_.read(utxo, lUtxo)) {

		uint256 lBlock;
		if (utxoBlock_.read(utxo, lBlock)) {
			size_t lHeight;
			if (!blockHeight(lBlock, lHeight)) 
				return false;
			return (lHeight > 0);
		} else {
			return false;
		}

		return false;
	}

	return false;
}

Transaction::UnlinkedOutPtr TransactionStore::findUnlinkedOut(const uint256& utxo) {
	// 
	Transaction::UnlinkedOut lUtxo;
	if (utxo_.read(utxo, lUtxo)) {
		return Transaction::UnlinkedOut::instance(lUtxo);
	}

	return nullptr;
}

EntityPtr TransactionStore::locateEntity(const uint256& entity) {
	TxBlockIndex lIndex;
	if (transactionsIdx_.read(entity, lIndex)) {
		BlockTransactionsPtr lTxs = transactions_.read(lIndex.block());

		if (lTxs && lTxs->transactions().size() < lIndex.index()) {
			// TODO: check?
			return TransactionHelper::to<Entity>(lTxs->transactions()[lIndex.index()]);
		}
	}

	return nullptr;
}

bool TransactionStore::pushEntity(const uint256& id, TransactionContextPtr ctx) {
	uint256 lId;
	if (ctx->tx()->isEntity() && ctx->tx()->entityName() == Entity::emptyName()) return true;
	if (!ctx->tx()->isEntity()) return false;
	if (entities_.read(ctx->tx()->entityName(), lId)) {
		return false;
	}

	entities_.write(ctx->tx()->entityName(), id);

	return true;
}

void TransactionStore::saveBlock(BlockPtr block) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[saveBlock]: saving block ") +
		strprintf("%s/%s#", block->hash().toHex(), chain_.toHex().substr(0, 10)));
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);	
	//
	// WARNING: should not ever be called from mining circle
	//
	uint256 lHash = block->hash();
	BlockHeader lHeader = block->blockHeader();
	
	// write block header
	headers_.write(lHash, lHeader);
	// write block transactions
	transactions_.write(lHash, block->blockTransactions());
	// remove
	dequeueBlock(lHash);
}

void TransactionStore::saveBlockHeader(const BlockHeader& header) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[saveBlockHeader]: saving block header ") +
		strprintf("%s/%s#", const_cast<BlockHeader&>(header).hash().toHex(), chain_.toHex().substr(0, 10)));
	//
	uint256 lHash = const_cast<BlockHeader&>(header).hash();
	// write block header
	headers_.write(lHash, header);
}

bool TransactionStore::blockExists(const uint256& id) {
	//
	BlockHeader lHeader;
	if (headers_.read(id, lHeader))
		return true;
	return false;
}

bool TransactionStore::blockHeader(const uint256& id, BlockHeader& header) {
	//
	if (headers_.read(id, header))
		return true;
	return false;
}

void TransactionStore::reindexFull(const uint256& from, IMemoryPoolPtr pool) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageCommitMutex_);	
	//
	gLog().write(Log::STORE, std::string("[reindexFull]: starting FULL reindex for ") + 
		strprintf("%s#", chain_.toHex().substr(0, 10)));

	// remove index
	removeBlocks(from, uint256(), false);
	// reset connected wallet cache
	wallet_->resetCache();	
	// remove wallet utxo-binding data
	wallet_->cleanUpData();
	// process blocks
	std::list<BlockContextPtr> lContexts;
	if (!processBlocks(from, uint256(), lContexts)) {
		if (lContexts.size()) {
			std::list<BlockContextPtr>::reverse_iterator lLast = (++lContexts.rbegin());
			if (lLast != lContexts.rend()) {
				setLastBlock((*lLast)->block()->hash());
				// build height map
				resyncHeight();
			}
		}
	} else {
		// clean-up
		for (std::list<BlockContextPtr>::iterator lBlock = lContexts.begin(); lBlock != lContexts.end(); lBlock++) {
			pool->removeTransactions((*lBlock)->block());
		}
		// new last
		setLastBlock(from);
		// build height map
		resyncHeight();	
	}

	//
	gLog().write(Log::STORE, std::string("[reindexFull]: FULL reindex FINISHED for ") + 
		strprintf("%s#", chain_.toHex().substr(0, 10)));
}

bool TransactionStore::reindex(const uint256& from, const uint256& to, IMemoryPoolPtr pool) {
	//
	gLog().write(Log::STORE, std::string("[reindex]: STARTING reindex for ") + 
		strprintf("%s#", chain_.toHex().substr(0, 10)));

	// check for new chain switching
	if (to == BlockHeader().hash()) {
		//
		gLog().write(Log::STORE, std::string("[reindex]: performing FULL reindex for new chain ") + 
			strprintf("%s/%s/%s#", from.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));
	} else {
		// WARNING: lastBlock AND to MUST be in current chain - otherwise all indexes will be invalidated
		size_t lLastHeight, lToHeight;
		if (!blockHeight(lastBlock_, lLastHeight) || !blockHeight(to, lToHeight)) {
			//
			gLog().write(Log::STORE, std::string("[reindex]: partial reindex is NOT POSSIBLE for ") + 
				strprintf("%s/%s/%s#", lastBlock_.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));

			return false;
		}
	}

	{
		//
		boost::unique_lock<boost::recursive_mutex> lLock(storageCommitMutex_);
		// remove old index
		removeBlocks(lastBlock_, to, false);
		// remove new index
		removeBlocks(from, to, false); // in case of wrapped restarts (re-process blocks may occure)
		// process blocks
		std::list<BlockContextPtr> lContexts;
		if (!processBlocks(from, to, lContexts)) {
			if (lContexts.size()) {
				std::list<BlockContextPtr>::reverse_iterator lLast = (++lContexts.rbegin()); // prev good
				if (lLast != lContexts.rend()) {
					setLastBlock((*lLast)->block()->hash());
					// build height map
					resyncHeight();
				} else {
					// shift "to" to prev, try to handle
					BlockHeader lHeader;
					if (blockHeader(to, lHeader)) {
						setLastBlock(lHeader.prev());
						// build height map
						resyncHeight();
					}
				}
			}
		} else {
			// clean-up
			for (std::list<BlockContextPtr>::iterator lBlock = lContexts.begin(); lBlock != lContexts.end(); lBlock++) {
				pool->removeTransactions((*lBlock)->block());
			}

			setLastBlock(from);
			// build height map
			resyncHeight();
		}

		//
		gLog().write(Log::STORE, std::string("[reindex]: reindex FINISHED for ") + 
			strprintf("%s#", chain_.toHex().substr(0, 10)));
	}

	return true;
}

bool TransactionStore::enqueueBlock(const NetworkBlockHeader& block) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	uint256 lHash = const_cast<NetworkBlockHeader&>(block).blockHeader().hash();
	if (blocksQueue_.find(lHash) == blocksQueue_.end()) {
		blocksQueue_.insert(std::map<uint256, NetworkBlockHeader>::value_type(lHash, block));	
		return true;
	}

	return false;
}

void TransactionStore::dequeueBlock(const uint256& block) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	blocksQueue_.erase(block);	
}

bool TransactionStore::firstEnqueuedBlock(NetworkBlockHeader& block) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	if (blocksQueue_.size()) {
		block = blocksQueue_.begin()->second;
		return true;
	}

	return false;	
}