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

	// last resort
	if (chain_ != MainChain::id()) return storeManager_->locate(MainChain::id())->locateTransaction(tx);
	return nullptr;
}

TransactionContextPtr TransactionStore::locateTransactionContext(const uint256& tx) {
	TransactionPtr lTx = locateTransaction(tx);
	if (lTx) return TransactionContext::instance(lTx);

	return nullptr;
}

bool TransactionStore::processBlockTransactions(ITransactionStorePtr store, IEntityStorePtr entityStore, BlockContextPtr ctx, BlockTransactionsPtr transactions, uint64_t approxHeight, bool processWallet) {
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
	TransactionProcessor lProcessor = TransactionProcessor::general(lTransactionStore, lWallet, entityStore);
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

			lBlockCtx->addErrors(lCtx->tx()->id(), lCtx->errors());
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
					lBlockCtx->addError(lCtx->tx()->id(), std::string("COINBASE already exists ") +
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
		std::list<TxBlockAction>::iterator lAction;
		bool lAbort = false;
		for (lAction = lBlockStore->actions().begin(); lAction != lBlockStore->actions().end(); lAction++) {
			if (lAction->action() == TxBlockAction::PUSH) {
				if (!pushUnlinkedOut(lAction->utxoPtr(), lAction->ctx())) {
					gLog().write(Log::ERROR, std::string("[processBlockTransactions/push/error]: Block with utxo inconsistency - ") + 
						strprintf("push: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));

					lBlockCtx->addError(lAction->ctx()->tx()->id(), "Block with utxo inconsistency - " +
						strprintf("pop: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));
					lAbort = true;
					break;
				}
			} else if (lAction->action() == TxBlockAction::POP) {
				if (!popUnlinkedOut(lAction->utxo(), lAction->ctx())) {
					gLog().write(Log::ERROR, std::string("[processBlockTransactions/pop/error]: Block with utxo inconsistency - ") + 
						strprintf("pop: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));

					lBlockCtx->addError(lAction->ctx()->tx()->id(), "Block with utxo inconsistency - " +
						strprintf("pop: utxo = %s, tx = %s, block = %s", 
							lAction->utxo().toHex(), lAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));
					lAbort = true;
					break;
				}
			}
		}

		if (lAbort) {
			// try to undo
			while ((--lAction) != lBlockStore->actions().end()) {
				if (lAction->action() == TxBlockAction::PUSH) {
					revertUtxo(lAction->utxoPtr()->hash());
				} else if (lAction->action() == TxBlockAction::POP) {
					revertLtxo(lAction->utxo(), uint256() /*empty*/);
				}
			}

			return false;
		}

		//
		// commit local wallet changes
		TxWalletStorePtr lWalletStore = std::static_pointer_cast<TxWalletStore>(lWallet);
		std::list<TxBlockAction>::iterator lWalletAction;
		for (lWalletAction = lWalletStore->actions().begin(); lWalletAction != lWalletStore->actions().end(); lWalletAction++) {
			//
			if ((processWallet || lWalletAction->ctx()->tx()->type() == Transaction::COINBASE) && 
				lWalletAction->action() == TxBlockAction::PUSH) {
				if (!wallet_->pushUnlinkedOut(lWalletAction->utxoPtr(), lWalletAction->ctx())) {
					gLog().write(Log::ERROR, std::string("[processBlockTransactions/wallet/push/error]: Block with utxo inconsistency - ") + 
						strprintf("push: utxo = %s, tx = %s, block = %s", 
							lWalletAction->utxo().toHex(), lWalletAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));

					lBlockCtx->addError(lWalletAction->ctx()->tx()->id(), "Block/pushWallet with utxo inconsistency - " +
						strprintf("push: utxo = %s, tx = %s, block = %s", 
							lWalletAction->utxo().toHex(), lWalletAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));							
					lAbort = true;
					break;
				}
			}

			if (processWallet && lWalletAction->action() == TxBlockAction::POP) {
				if (!wallet_->popUnlinkedOut(lWalletAction->utxo(), lWalletAction->ctx())) {
					gLog().write(Log::ERROR, std::string("[processBlockTransactions/wallet/pop/error]: Block with utxo inconsistency - ") + 
						strprintf("pop: utxo = %s, tx = %s, block = %s", 
							lWalletAction->utxo().toHex(), lWalletAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));

					lBlockCtx->addError(lWalletAction->ctx()->tx()->id(), "Block/popWallet with utxo inconsistency - " +
						strprintf("pop: utxo = %s, tx = %s, block = %s", 
							lWalletAction->utxo().toHex(), lWalletAction->ctx()->tx()->id().toHex(), ctx->block()->hash().toHex()));
					lAbort = true;
					break;
				}
			}
		}

		if (lAbort) {
			// try to undo
			while ((--lWalletAction) != lWalletStore->actions().end()) {
				if ((processWallet || lWalletAction->ctx()->tx()->type() == Transaction::COINBASE) && 
																lWalletAction->action() == TxBlockAction::PUSH) {
					wallet_->rollback(lWalletAction->ctx());
				}

				if (processWallet && lWalletAction->action() == TxBlockAction::POP) {
					wallet_->rollback(lWalletAction->ctx());
				}
			}

			return false;
		}

		//
		// commit entity changes
		TxEntityStorePtr lEntityStore = std::static_pointer_cast<TxEntityStore>(entityStore);
		std::list<TransactionContextPtr>::iterator lEntityAction;
		for (lEntityAction = lEntityStore->actions().begin(); lEntityAction != lEntityStore->actions().end(); lEntityAction++) {
			if (!pushEntity((*lEntityAction)->tx()->id(), *lEntityAction)) {
				lBlockCtx->addError((*lEntityAction)->tx()->id(), "Block/pushEntity inconsistency" +
					strprintf(" tx = %s, block = %s", 
						(*lEntityAction)->tx()->id().toHex(), ctx->block()->hash().toHex()));
				lAbort = true;
				break;
			}
		}

		if (lAbort) {
			// try to undo
			while ((--lEntityAction) != lEntityStore->actions().end()) {
				//
				TransactionPtr lTx = (*lEntityAction)->tx();
				//
				if (lTx->isEntity() && lTx->entityName() != Entity::emptyName()) {
					entities_.remove(lTx->entityName());
					
					// iterate
					db::DbMultiContainer<std::string /*short name*/, uint256 /*utxo*/>::Transaction lEntityUtxoTransaction = entityUtxo_.transaction();
					for (db::DbMultiContainer<std::string /*short name*/, uint256 /*utxo*/>::Iterator lEntityUtxo = entityUtxo_.find(lTx->entityName()); lEntityUtxo.valid(); ++lEntityUtxo) {
						lEntityUtxoTransaction.remove(lEntityUtxo);
					}

					lEntityUtxoTransaction.commit();

					db::DbMultiContainer<uint256 /*entity*/, uint256 /*shard*/>::Transaction lShardsTransaction = shards_.transaction();
					for (db::DbMultiContainer<uint256 /*entity*/, uint256 /*shard*/>::Iterator lShards = shards_.find(lTx->id()); lShards.valid(); ++lShards) {
						shardEntities_.remove(*lShards);
						lShardsTransaction.remove(lShards);
					}

					lShardsTransaction.commit();
				}

				if (extension_) {
					extension_->removeTransaction(lTx);
				}
			}
		}

		return true;
	}

	return false;
}

bool TransactionStore::processBlock(BlockContextPtr ctx, uint64_t& height, bool processWallet) {
	//
	uint64_t lHeight = top();
	BlockTransactionsPtr lTransactions = ctx->block()->blockTransactions();
	ITransactionStorePtr lTransactionStore = TxBlockStore::instance(std::static_pointer_cast<ITransactionStore>(shared_from_this()));
	IEntityStorePtr lEntityStore = TxEntityStore::instance(std::static_pointer_cast<IEntityStore>(shared_from_this()), std::static_pointer_cast<ITransactionStore>(shared_from_this()));
	//
	if (processBlockTransactions(lTransactionStore, lEntityStore, ctx, lTransactions, lHeight+1 /*next*/, processWallet)) {
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

bool TransactionStore::transactionHeight(const uint256& tx, uint64_t& height, uint64_t& confirms, bool& coinbase) {
	uint256 lBlock;
	uint32_t lIndex;
	return transactionInfo(tx, lBlock, height, confirms, lIndex, coinbase);
}

bool TransactionStore::transactionInfo(const uint256& tx, uint256& block, uint64_t& height, uint64_t& confirms, uint32_t& index, bool& coinbase) {
	TxBlockIndex lIndex;
	if (transactionsIdx_.read(tx, lIndex)) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
		std::map<uint256, uint64_t>::iterator lHeight = blockMap_.find(lIndex.block());

		if (lHeight != blockMap_.end()) {
			if (!lIndex.index()) coinbase = true;
			else coinbase = false;
			height = lHeight->second;
			block = lIndex.block();
			index = lIndex.index();

			std::map<uint64_t, uint256>::reverse_iterator lHead = heightMap_.rbegin();
			if (lHead != heightMap_.rend() && lHead->first >= height) { 
				confirms = (lHead->first - height) + 1; 
			} else confirms = 0;

			return true;
		}
	}

	return false;	
}

bool TransactionStore::blockHeight(const uint256& block, uint64_t& height) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	std::map<uint256, uint64_t>::iterator lHeight = blockMap_.find(block);
	if (lHeight != blockMap_.end()) {
		height = lHeight->second;
		return true;
	}

	return false;
}

bool TransactionStore::blockHeaderHeight(const uint256& block, uint64_t& height, BlockHeader& header) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	std::map<uint256, uint64_t>::iterator lHeight = blockMap_.find(block);
	if (lHeight != blockMap_.end()) {
		height = lHeight->second;
		return blockHeader(block, header);
	}

	return false;
}

bool TransactionStore::setLastBlock(const uint256& block) {
	//
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
	BlockHeader lHeader;
	uint256 lHash = lastBlock_;
	uint64_t lIndex = 0;

	//
	uint256 lNull = BlockHeader().hash();

	//
	std::list<uint256> lSeq;
	while (lHash != lNull && headers_.read(lHash, lHeader)) {
		// check block data
		if (blockExists(lHash)) {
			// push
			lSeq.push_back(lHash);
			lHash = lHeader.prev();
		} else {
			break;
		}
	}

	if (lHash != lNull && !lastBlock_.isNull()) {
		if (lHash == lHeader.hash()) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/error]: block data is MISSING for ") + 
					strprintf("block = %s, chain = %s#", 
						lHash.toHex(), lHeader.hash().toHex(), chain_.toHex().substr(0, 10)));
		} else {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/error]: chain is BROKEN on ") + 
					strprintf("prev_block = %s, block = %s, chain = %s#", 
						lHash.toHex(), lHeader.hash().toHex(), chain_.toHex().substr(0, 10)));
		}

		return false;
	}

	//
	heightMap_.clear();
	blockMap_.clear();

	for (std::list<uint256>::reverse_iterator lIter = lSeq.rbegin(); lIter != lSeq.rend(); lIter++) {
		heightMap_.insert(std::map<uint64_t, uint256>::value_type(++lIndex, *lIter));
		blockMap_.insert(std::map<uint256, uint64_t>::value_type(*lIter, lIndex));
	}

	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight]: current ") + strprintf("height = %d, block = %s, chain = %s#", lIndex, lastBlock_.toHex(), chain_.toHex().substr(0, 10)));

	return true;
}

uint64_t TransactionStore::calcHeight(const uint256& from) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);

	//
	BlockHeader lHeader;
	uint256 lHash = from;
	uint64_t lIndex = 0;

	while (headers_.read(lHash, lHeader)) {
		lIndex++;
		lHash = lHeader.prev();
	}

	return lIndex;
}

bool TransactionStore::revertUtxo(const uint256& utxo) {
	//
	Transaction::UnlinkedOut lUtxoObj;
	if (utxo_.read(utxo, lUtxoObj)) {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[revertUtxo]: ") +
			strprintf("remove utxo = %s, tx = %s/%s#", utxo.toHex(), lUtxoObj.out().tx().toHex(), chain_.toHex().substr(0, 10)));

		utxo_.remove(utxo);
		utxoBlock_.remove(utxo); // just for push
		addressAssetUtxoIdx_.remove(lUtxoObj.address().id(), lUtxoObj.out().asset(), utxo);

		return true;
	}

	return false;
}

bool TransactionStore::revertLtxo(const uint256& ltxo, const uint256& hash) {
	//
	Transaction::UnlinkedOut lUtxoObj;
	if (ltxo_.read(ltxo, lUtxoObj)) {
		utxo_.write(ltxo, lUtxoObj);
		addressAssetUtxoIdx_.write(lUtxoObj.address().id(), lUtxoObj.out().asset(), ltxo, lUtxoObj.out().tx());
		if (!hash.isEmpty()) utxoBlock_.write(ltxo, hash);
		ltxo_.remove(ltxo);

		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[revertLtxo]: ") +
			strprintf("remove/add ltxo/utxo = %s, tx = %s/%s#", ltxo.toHex(), lUtxoObj.out().tx().toHex(), chain_.toHex().substr(0, 10)));

		return true;
	}

	return false;
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
					// try main chain 
					if (chain_ != MainChain::id()) {
						//
						ITransactionStorePtr lMainStore = storeManager_->locate(MainChain::id());
						if (!lMainStore->revertUtxo(lAction.utxo())) {
							if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
								strprintf("utxo was NOT FOUND - %s/%s#", lAction.utxo().toHex(), chain_.toHex().substr(0, 10)));
						}
					} else {
						if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
									strprintf("utxo was NOT FOUND - %s/%s#", lAction.utxo().toHex(), chain_.toHex().substr(0, 10)));						
					}
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
					// try main chain 
					if (chain_ != MainChain::id()) {
						//
						ITransactionStorePtr lMainStore = storeManager_->locate(MainChain::id());
						if (!lMainStore->revertLtxo(lAction.utxo(), lHash)) {
							if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
								strprintf("ltxo was NOT FOUND - %s/%s#", lAction.utxo().toHex(), chain_.toHex().substr(0, 10)));
						}
					} else {
						if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
							strprintf("ltxo was NOT FOUND - %s/%s#", lAction.utxo().toHex(), chain_.toHex().substr(0, 10)));
					}
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
				if (extension_) extension_->removeTransaction(*lTx);

				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
					strprintf("removing tx index %s/%s/%s#", (*lTx)->id().toHex(), lHash.toHex(), chain_.toHex().substr(0, 10)));

				transactionsIdx_.remove((*lTx)->id());

				// iterate
				db::DbMultiContainer<uint256 /*tx*/, uint256 /*utxo*/>::Transaction lTxUtxoTransaction = txUtxo_.transaction();
				for (db::DbMultiContainer<uint256 /*tx*/, uint256 /*utxo*/>::Iterator lTxUtxo = txUtxo_.find((*lTx)->id()); lTxUtxo.valid(); ++lTxUtxo) {
					lTxUtxoTransaction.remove(lTxUtxo);
				}

				lTxUtxoTransaction.commit();			

				if ((*lTx)->isEntity() && (*lTx)->entityName() != Entity::emptyName()) {
					entities_.remove((*lTx)->entityName());
					
					// iterate
					db::DbMultiContainer<std::string /*short name*/, uint256 /*utxo*/>::Transaction lEntityUtxoTransaction = entityUtxo_.transaction();
					for (db::DbMultiContainer<std::string /*short name*/, uint256 /*utxo*/>::Iterator lEntityUtxo = entityUtxo_.find((*lTx)->entityName()); lEntityUtxo.valid(); ++lEntityUtxo) {
						lEntityUtxoTransaction.remove(lEntityUtxo);
					}

					lEntityUtxoTransaction.commit();

					db::DbMultiContainer<uint256 /*entity*/, uint256 /*shard*/>::Transaction lShardsTransaction = shards_.transaction();
					for (db::DbMultiContainer<uint256 /*entity*/, uint256 /*shard*/>::Iterator lShards = shards_.find((*lTx)->id()); lShards.valid(); ++lShards) {
						shardEntities_.remove(*lShards);
						lShardsTransaction.remove(lShards);
					}

					lShardsTransaction.commit();
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
//
bool TransactionStore::collectUtxoByEntityName(const std::string& name, std::vector<Transaction::UnlinkedOutPtr>& result) {
	//
	bool lResult = false;
	std::map<uint32_t, Transaction::UnlinkedOutPtr> lUtxos;

	// iterate
	for (db::DbMultiContainer<std::string /*short name*/, uint256 /*utxo*/>::Iterator lUtxo = entityUtxo_.find(name); lUtxo.valid(); ++lUtxo) {
		Transaction::UnlinkedOut lUtxoObj;
		if (utxo_.read(*lUtxo, lUtxoObj)) {
			lUtxos[lUtxoObj.out().index()] = Transaction::UnlinkedOut::instance(lUtxoObj);
			lResult = true;
		}
	}

	// ordering
	for (std::map<uint32_t, Transaction::UnlinkedOutPtr>::iterator lItem = lUtxos.begin(); lItem != lUtxos.end(); lItem++) {
		result.push_back(lItem->second);
	}

	return lResult;
}

bool TransactionStore::entityCountByShards(const std::string& name, std::map<uint32_t, uint256>& result) {
	//
	bool lResult = false;

	uint256 lDAppTx;
	if(entities_.read(name, lDAppTx)) {
		//
		for (db::DbMultiContainer<uint256 /*entity*/, uint256 /*shard*/>::Iterator lShard = shards_.find(lDAppTx); lShard.valid(); ++lShard) {
			//
			uint32_t lCount = 0;
			shardEntities_.read(*lShard, lCount);
			result.insert(std::map<uint32_t, uint256>::value_type(lCount, *lShard));
			lResult = true;
		}
	}

	return lResult;
}

bool TransactionStore::entityCountByDApp(const std::string& name, std::map<uint256, uint32_t>& result) {
	//
	bool lResult = true;

	uint256 lDAppTx;
	if(entities_.read(name, lDAppTx)) {
		//
		for (db::DbMultiContainer<uint256 /*entity*/, uint256 /*shard*/>::Iterator lShard = shards_.find(lDAppTx); lShard.valid(); ++lShard) {
			//
			uint32_t lCount = 0;
			ITransactionStorePtr lStore = storeManager()->locate(*lShard);
			if (lStore) {
				//
				if (lStore->entityStore()->entityCount(lCount)) {
					result[*lShard] = lCount;
				} else {
					result[*lShard] = 0;
				}
			}
		}
	}

	return lResult;
}

bool TransactionStore::entityCount(uint32_t& result) {
	//
	return shardEntities_.read(chain_, result);
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
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks/error]: check sequence consistency FAILED ") +
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
		uint256 lBlockHash = (*lBlock).hash();
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks]: processing block ") +
			strprintf("%s/%s#", lBlockHash.toHex(), chain_.toHex().substr(0, 10)));

		BlockContextPtr lBlockCtx = BlockContext::instance(Block::instance(*lBlock)); // just header without transactions attached
		BlockTransactionsPtr lTransactions = transactions_.read(lBlockHash);
		if (!lTransactions) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks/error]: transaction DATA is ABSENT for ") +
				strprintf("block = %s, prev = %s, chain = %s#", lBlockHash.toHex(), (*lBlock).prev().toHex(), chain_.toHex().substr(0, 10)));
			return false;
		}

		lBlockCtx->block()->append(lTransactions); // reconstruct block consistensy
		ctxs.push_back(lBlockCtx);

		//
		// process txs
		ITransactionStorePtr lTransactionStore = TxBlockStore::instance(std::static_pointer_cast<ITransactionStore>(shared_from_this()));
		IEntityStorePtr lEntityStore = TxEntityStore::instance(std::static_pointer_cast<IEntityStore>(shared_from_this()), std::static_pointer_cast<ITransactionStore>(shared_from_this()));	
		if (processBlockTransactions(lTransactionStore, lEntityStore, lBlockCtx, lTransactions, 0 /*just re-sync*/, true /*process wallet*/)) {
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

			//
			lBlockCtx->block()->compact(); // compact txs to just ids
		} else {
			//
			lBlockCtx->block()->compact(); // compact txs to just ids
			return false;
		}
	}

	return true;
}

uint64_t TransactionStore::currentHeight(BlockHeader& block) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	if (heightMap_.size()) {
		std::map<uint64_t, uint256>::reverse_iterator lHeader = heightMap_.rbegin();
		// validate
		if (headers_.read(lHeader->second, block)) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[currentHeight]: ") + strprintf("%d/%s/%s#", lHeader->first, block.hash().toHex(), chain_.toHex().substr(0, 10)));
			return lHeader->first;
		}
	}

	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[currentHeight/missing]: ") + strprintf("%d/%s/%s#", 0, BlockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));
	return 0;
}

uint64_t TransactionStore::top() {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	if (heightMap_.size()) {
		std::map<uint64_t, uint256>::reverse_iterator lHeader = heightMap_.rbegin();
		return lHeader->first;
	}

	return 0;
}

uint64_t TransactionStore::pushNewHeight(const uint256& block) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	uint64_t lHeight = 1;
	if (heightMap_.size()) {
		lHeight = heightMap_.rbegin()->first+1;
	}

	std::pair<std::map<uint64_t, uint256>::iterator, bool> lNewHeight = 
		heightMap_.insert(std::map<uint64_t, uint256>::value_type(lHeight, block));
	blockMap_.insert(std::map<uint256, uint64_t>::value_type(block, lHeight));

	return lNewHeight.first->first;
}

BlockHeader TransactionStore::currentBlockHeader() {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);

	uint256 lHash = lastBlock_;

	BlockHeader lHeader;
	if (headers_.read(lHash, lHeader)) {
		return lHeader;
	}

	return BlockHeader();
}

bool TransactionStore::commitBlock(BlockContextPtr ctx, uint64_t& height) {
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
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushBlock]: PUSHING block ") + 
		strprintf("%s/%s#", block->hash().toHex(), chain_.toHex().substr(0, 10)));

	boost::unique_lock<boost::recursive_mutex> lLock(storageCommitMutex_);

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
	uint64_t lHeight;
	BlockContextPtr lBlockCtx = BlockContext::instance(block);	
	{
		//
		lResult = processBlock(lBlockCtx, lHeight, true);
		if (lResult) {
			lBlockCtx->setHeight(lHeight);
		}
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
	//
	if (!opened_) return;

	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	lastBlock_ = block;
	std::string lHex = block.toHex();
	workingSettings_.write("lastBlock", lHex);
}

BlockPtr TransactionStore::block(uint64_t height) {
	//
	BlockHeader lHeader;
	uint256 lHash;
	{
		boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
		std::map<uint64_t, uint256>::iterator lHeight = heightMap_.find(height);
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

bool TransactionStore::blockHeader(uint64_t height, BlockHeader& header) {
	//
	uint256 lHash;
	{
		boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
		std::map<uint64_t, uint256>::iterator lHeight = heightMap_.find(height);
		if (lHeight != heightMap_.end()) {
			lHash = lHeight->second;
		} else {
			return false;
		}
	}

	return headers_.read(lHash, header);
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
			shards_.open();
			entityUtxo_.open();
			shardEntities_.open();
			txUtxo_.open();

			if (settings_->supportAirdrop()) {
				airDropAddressesTx_.open();
				airDropPeers_.open();
			}

			std::string lLastBlock;
			if (workingSettings_.read("lastBlock", lLastBlock)) {
				lastBlock_.setHex(lLastBlock);
			}

			gLog().write(Log::INFO, std::string("[open]: lastBlock = ") + strprintf("%s/%s#", lastBlock_.toHex(), chain_.toHex().substr(0, 10)));

			resyncHeight();

			// push shards
			for (db::DbMultiContainer<uint256 /*entity*/, uint256 /*shard*/>::Iterator lShard = shards_.begin(); lShard.valid(); ++lShard) {
				//
				uint256 lDAppId;
				gLog().write(Log::INFO, std::string("[open]: try to open shard ") + strprintf("%s/%s#", (*lShard).toHex(), chain_.toHex().substr(0, 10)));

				if (lShard.first(lDAppId) && storeManager_) {
					TransactionPtr lDApp = locateTransaction(lDAppId);
					if (lDApp) storeManager_->pushChain(*lShard, TransactionHelper::to<Entity>(lDApp));
					else {
						gLog().write(Log::INFO, std::string("[open/warning]: DApp tx was not found - ") + strprintf("%s/%s#", lDAppId.toHex(), chain_.toHex().substr(0, 10)));
					}
				}
			}

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

	wallet_.reset();
	storeManager_.reset();

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
	shards_.close();
	entityUtxo_.close();
	shardEntities_.close();
	txUtxo_.close();

	if (settings_->supportAirdrop()) {
		airDropAddressesTx_.close();
		airDropPeers_.close();
	}

	if (extension_) extension_->close();

	settings_.reset();
	opened_ = false;

	return true;
}

bool TransactionStore::enumUnlinkedOuts(const uint256& tx, std::vector<Transaction::UnlinkedOutPtr>& outs) {
	//
	bool lResult = false;
	std::map<uint32_t, Transaction::UnlinkedOutPtr> lUtxos;
	for (db::DbMultiContainer<uint256 /*tx*/, uint256 /*utxo*/>::Iterator lTxUtxo = txUtxo_.find(tx); lTxUtxo.valid(); ++lTxUtxo) {
		Transaction::UnlinkedOut lUtxo;
		if (utxo_.read(*lTxUtxo, lUtxo)) {
			lUtxos[lUtxo.out().index()] = Transaction::UnlinkedOut::instance(lUtxo);
			lResult = true;
		}
	}

	// ordering
	for (std::map<uint32_t, Transaction::UnlinkedOutPtr>::iterator lItem = lUtxos.begin(); lItem != lUtxos.end(); lItem++) {
		outs.push_back(lItem->second);
	}

	return lResult;
}

bool TransactionStore::pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr ctx) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushUnlinkedOut]: try to push ") +
		strprintf("utxo = %s, tx = %s, ctx = %s", 
			utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex()));

	uint256 lUtxo = utxo->hash();
	if (!isUnlinkedOutUsed(lUtxo) /*&& !findUnlinkedOut(lUtxo)*/) { // sensitive: in rare cases updates may occure
		// update utxo db
		utxo_.write(lUtxo, *utxo);
		txUtxo_.write(utxo->out().tx(), lUtxo);

		// update indexes
		addressAssetUtxoIdx_.write(utxo->address().id(), utxo->out().asset(), lUtxo, utxo->out().tx());
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushUnlinkedOut]: PUSHED ") +
			strprintf("utxo = %s, tx = %s, ctx = %s", 
				lUtxo.toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex()));

		// name -> utxo
		if (ctx->tx()->isEntity() && ctx->tx()->entityName() != Entity::emptyName()) {
			entityUtxo_.write(ctx->tx()->entityName(), lUtxo);
		}

		// forward
		if (extension_) extension_->pushUnlinkedOut(utxo, ctx);

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
		// forward
		if (extension_) extension_->popUnlinkedOut(utxo, ctx);
		return true;
	} else if (!lUtxo && chain_ != MainChain::id()) {
		return storeManager_->locate(MainChain::id())->popUnlinkedOut(utxo, ctx);
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
			uint64_t lHeight;
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

bool TransactionStore::isLinkedOutExists(const uint256& utxo) {
	//
	Transaction::UnlinkedOut lUtxo;
	if (ltxo_.read(utxo, lUtxo)) {
		return true;
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

Transaction::UnlinkedOutPtr TransactionStore::findLinkedOut(const uint256& utxo) {
	// 
	Transaction::UnlinkedOut lUtxo;
	if (ltxo_.read(utxo, lUtxo)) {
		return Transaction::UnlinkedOut::instance(lUtxo);
	}

	return nullptr;
}

EntityPtr TransactionStore::locateEntity(const uint256& entity) {
	TxBlockIndex lIndex;
	if (transactionsIdx_.read(entity, lIndex)) {
		BlockTransactionsPtr lTxs = transactions_.read(lIndex.block());

		if (lTxs && lIndex.index() < lTxs->transactions().size()) {
			// TODO: check?
			return TransactionHelper::to<Entity>(lTxs->transactions()[lIndex.index()]);
		}
	}

	return nullptr;
}

EntityPtr TransactionStore::locateEntity(const std::string& entity) {
	//		
	uint256 lEntityTx;
	if (entities_.read(entity, lEntityTx)) return locateEntity(lEntityTx);
	return nullptr;
}

void TransactionStore::selectUtxoByAddress(const PKey& address, std::vector<Transaction::NetworkUnlinkedOut>& utxo) {
	//
	for (db::DbThreeKeyContainer
			<uint160 /*address*/, 
				uint256 /*asset*/, 
				uint256 /*utxo*/, 
				uint256 /*tx*/>::Iterator lBundle = addressAssetUtxoIdx_.find(address.id()); lBundle.valid(); ++lBundle) {
		uint160 lAddress;
		uint256 lAsset;
		uint256 lUtxo;

		if (lBundle.first(lAddress, lAsset, lUtxo)) {
			Transaction::UnlinkedOut lOut;
			if (utxo_.read(lUtxo, lOut)) {
				//
				uint64_t lHeight = 0;
				uint64_t lConfirms = 0;
				bool lCoinbase = false;

				transactionHeight(lOut.out().tx(), lHeight, lConfirms, lCoinbase);
				utxo.push_back(Transaction::NetworkUnlinkedOut(lOut, lConfirms, lCoinbase));
			}
		}
	}
}

void TransactionStore::selectUtxoByAddressAndAsset(const PKey& address, const uint256& asset, std::vector<Transaction::NetworkUnlinkedOut>& utxo) {
	//
	uint160 lId = address.id();
	for (db::DbThreeKeyContainer
			<uint160 /*address*/, 
				uint256 /*asset*/, 
				uint256 /*utxo*/, 
				uint256 /*tx*/>::Iterator lBundle = addressAssetUtxoIdx_.find(lId, asset); lBundle.valid(); ++lBundle) {
		uint160 lAddress;
		uint256 lAsset;
		uint256 lUtxo;

		if (lBundle.first(lAddress, lAsset, lUtxo)) {
			Transaction::UnlinkedOut lOut;
			if (utxo_.read(lUtxo, lOut)) {
				//
				uint64_t lHeight = 0;
				uint64_t lConfirms = 0;
				bool lCoinbase = false;

				transactionHeight(lOut.out().tx(), lHeight, lConfirms, lCoinbase);
				utxo.push_back(Transaction::NetworkUnlinkedOut(lOut, lConfirms, lCoinbase));
			}
		}
	}
}

void TransactionStore::selectUtxoByTransaction(const uint256& tx, std::vector<Transaction::NetworkUnlinkedOut>& utxo) {
	//
	uint64_t lHeight = 0;
	uint64_t lConfirms = 0;
	bool lCoinbase = false;
	transactionHeight(tx, lHeight, lConfirms, lCoinbase);
	//
	std::map<uint32_t, Transaction::NetworkUnlinkedOut> lUtxos;
	db::DbMultiContainer<uint256 /*tx*/, uint256 /*utxo*/>::Iterator lTxRoot = txUtxo_.find(tx);
	for (; lTxRoot.valid(); ++lTxRoot) {
		Transaction::UnlinkedOut lOut;
		if (utxo_.read(*lTxRoot, lOut)) {
			//
			lUtxos[lOut.out().index()] = Transaction::NetworkUnlinkedOut(lOut, lConfirms, lCoinbase);
		}
	}

	// ordering
	for (std::map<uint32_t, Transaction::NetworkUnlinkedOut>::iterator lItem = lUtxos.begin(); lItem != lUtxos.end(); lItem++) {
		utxo.push_back(lItem->second);
	}
}

void TransactionStore::selectEntityNames(const std::string& name, std::vector<IEntityStore::EntityName>& names) {
	//
	db::DbContainer<std::string /*short name*/, uint256 /*tx*/>::Iterator lFrom = entities_.find(name);
	for (int lCount = 0; lFrom.valid() && names.size() < 6 && lCount < 100; ++lFrom, ++lCount) {
		std::string lName;
		if (lFrom.first(lName) && lName.find(name) != std::string::npos) 
			names.push_back(IEntityStore::EntityName(lName));
	}
}

bool TransactionStore::pushEntity(const uint256& id, TransactionContextPtr ctx) {
	//
	// forward entity processing to the extension anyway
	if (extension_) extension_->pushEntity(id, ctx);

	// filter unnamed entities
	if (ctx->tx()->isEntity() && ctx->tx()->entityName() == Entity::emptyName()) return true;

	// sanity check: skip if not entity
	if (!ctx->tx()->isEntity()) return true;

	uint256 lId;
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushEntity]: try to push entity ") +
		strprintf("'%s'/%s", ctx->tx()->entityName(), id.toHex()));

	if (entities_.read(ctx->tx()->entityName(), lId)) {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushEntity]: entity ALREADY EXISTS - ") +
			strprintf("'%s'/%s", ctx->tx()->entityName(), id.toHex()));
		return false;
	}

	entities_.write(ctx->tx()->entityName(), id);
	// coint own entities
	if (chain_ != MainChain::id()) {
		uint32_t lCount = 0;
		shardEntities_.read(chain_, lCount);
		shardEntities_.write(chain_, ++lCount);
	}

	if (ctx->tx()->type() == Transaction::SHARD && ctx->tx()->in().size()) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushEntity]: extracting DApp ") +
			strprintf("%s for '%s'/%s#", ctx->tx()->in().begin()->out().tx().toHex(), ctx->tx()->entityName(), id.toHex().substr(0, 10)));

		TransactionPtr lDAppTx = locateTransaction(ctx->tx()->in().begin()->out().tx());
		if (lDAppTx && lDAppTx->type() == Transaction::DAPP) {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushEntity]: pushing shard ") +
				strprintf("'%s'/%s# for '%s'", ctx->tx()->entityName(), id.toHex().substr(0, 10), lDAppTx->entityName()));

			shards_.write(lDAppTx->id(), id); // dapp -> shard

			if (storeManager_) storeManager_->pushChain(id, TransactionHelper::to<Entity>(lDAppTx));
		} else {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushEntity]: DApp was NOT FOUND ") +
				strprintf("%s for '%s'/%s#", ctx->tx()->in().begin()->out().tx().toHex(), ctx->tx()->entityName(), id.toHex().substr(0, 10)));
			return false;
		}
	} else if (ctx->tx()->type() >= Transaction::CUSTOM_00 && ctx->tx()->in().size() > 1) {
		//
		uint256 lShardId = (++(ctx->tx()->in().begin()))->out().tx(); // in[1]
		// custom entities
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushEntity]: extracting shard ") +
			strprintf("%s for '%s'/%s#", lShardId.toHex(), ctx->tx()->entityName(), id.toHex().substr(0, 10)));

		TransactionPtr lShardTx = locateTransaction(lShardId); // in[1]
		if (lShardTx && lShardTx->type() == Transaction::SHARD) {
			//
			uint32_t lCount = 0;
			shardEntities_.read(lShardId, lCount);
			shardEntities_.write(lShardId, ++lCount);

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushEntity]: registering custom entity ") +
				strprintf("[%d]/'%s'/%s# with shard '%s'", lCount, ctx->tx()->entityName(), id.toHex().substr(0, 10), lShardTx->entityName()));
		}
	}

	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushEntity]: PUSHED entity ") +
		strprintf("'%s'/%s", ctx->tx()->entityName(), id.toHex()));

	return true;
}

void TransactionStore::saveBlock(BlockPtr block) {
	//
	/*
	if (synchronizing()) {
		gLog().write(Log::STORE, strprintf("[saveBlock]: skipped, REINDEXING %s", chain_.toHex().substr(0, 10)));
		return;
	}
	*/

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
	/*
	if (synchronizing()) {
		gLog().write(Log::STORE, strprintf("[saveBlockHeader]: skipped, REINDEXING %s", chain_.toHex().substr(0, 10)));
		return;
	}
	*/

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
	return transactions_.exists(id);
}

bool TransactionStore::blockHeader(const uint256& id, BlockHeader& header) {
	//
	if (headers_.read(id, header))
		return true;
	return false;
}

void TransactionStore::reindexFull(const uint256& from, IMemoryPoolPtr pool) {
	//
	if (synchronizing()) {
		gLog().write(Log::STORE, std::string("[reindexFull]: already SYNCHRONIZING ") + 
			strprintf("%s/%s#", from.toHex(), chain_.toHex().substr(0, 10)));
		return;
	}
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageCommitMutex_);
	// synchronizing
	SynchronizingGuard lGuard(shared_from_this());	
	//
	gLog().write(Log::STORE, std::string("[reindexFull]: starting FULL reindex for ") + 
		strprintf("%s#", chain_.toHex().substr(0, 10)));

	// remove index, DO WE need really this? - that is totally new chain
	removeBlocks(from, BlockHeader().hash(), false);

	// reset connected wallet cache
	wallet_->resetCache();	
	// remove wallet utxo-binding data
	wallet_->cleanUpData();
	// process blocks
	std::list<BlockContextPtr> lContexts;
	if (!processBlocks(from, BlockHeader().hash(), lContexts)) {
		if (lContexts.size()) {
			std::list<BlockContextPtr>::reverse_iterator lLast = (++lContexts.rbegin());
			if (lLast != lContexts.rend()) {
				uint256 lLastBlock = lastBlock_;
				setLastBlock((*lLast)->block()->hash());
				// build height map
				if (!resyncHeight()) {
					// try to rollback
					setLastBlock(lLastBlock);
					resyncHeight();
				}
			}
		}
	} else {
		// clean-up
		for (std::list<BlockContextPtr>::iterator lBlock = lContexts.begin(); lBlock != lContexts.end(); lBlock++) {
			pool->removeTransactions((*lBlock)->block());
		}
		//
		uint256 lLastBlock = lastBlock_;
		// new last
		setLastBlock(from);
		// build height map
		if (!resyncHeight()) {
			// try to rollback
			setLastBlock(lLastBlock);
			resyncHeight();
		}
	}

	//
	gLog().write(Log::STORE, std::string("[reindexFull]: FULL reindex FINISHED for ") + 
		strprintf("%s#", chain_.toHex().substr(0, 10)));
}

bool TransactionStore::reindex(const uint256& from, const uint256& to, IMemoryPoolPtr pool) {
	//
	if (synchronizing()) {
		gLog().write(Log::STORE, std::string("[reindex]: already SYNCHRONIZING ") + 
			strprintf("%s/%s/%s#", from.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));
		return false;
	}
	//
	gLog().write(Log::STORE, std::string("[reindex]: STARTING reindex for ") + 
		strprintf("from = %s, to = %s, %s#", from.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));

	// check for new chain switching
	if (to == BlockHeader().hash()) {
		//
		gLog().write(Log::STORE, std::string("[reindex]: performing FULL reindex for new chain ") + 
			strprintf("%s/%s/%s#", from.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));
	} else {
		// WARNING: lastBlock AND to MUST be in current chain - otherwise all indexes will be invalidated
		/*
		uint64_t lLastHeight, lToHeight;
		if (!blockHeight(lastBlock_, lLastHeight) || !blockHeight(to, lToHeight)) {
			//
			gLog().write(Log::STORE, std::string("[reindex]: partial reindex is NOT POSSIBLE for ") + 
				strprintf("%s/%s/%s#", lastBlock_.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));

			return false;
		}
		*/
	}

	//
	bool lResult = true;

	// save prev_last
	uint256 lLastBlock = lastBlock_;
	//
	bool lResyncHeight = false;
	bool lRemoveTransactions = false;
	std::list<BlockContextPtr> lContexts;
	{
		//
		boost::unique_lock<boost::recursive_mutex> lLock(storageCommitMutex_);
		// synchronizing
		SynchronizingGuard lGuard(shared_from_this());
		// remove old index
		removeBlocks(lastBlock_, to, false);
		// remove new index
		removeBlocks(from, to, false); // in case of wrapped restarts (re-process blocks may occur)
		// process blocks
		if (!(lResult = processBlocks(from, to, lContexts))) {
			if (lContexts.size()) {
				std::list<BlockContextPtr>::reverse_iterator lLast = (++lContexts.rbegin()); // prev good
				if (lLast != lContexts.rend()) {
					setLastBlock((*lLast)->block()->hash());
					// build height map
					if (!resyncHeight()) {
						// rollback
						setLastBlock(lLastBlock);
						resyncHeight();
					}
				} else {
					// shift "to" to prev, try to handle
					BlockHeader lHeader;
					if (blockHeader(to, lHeader)) {
						setLastBlock(lHeader.prev());
						// build height map
						if (!resyncHeight()) {
							// rollback
							setLastBlock(lLastBlock);
							resyncHeight();
						}
					}
				}
			}
		} else {
			// clean-up
			for (std::list<BlockContextPtr>::iterator lBlock = lContexts.begin(); lBlock != lContexts.end(); lBlock++) {
				pool->removeTransactions((*lBlock)->block());
			}
			// point to the last block
			setLastBlock(from);
			// build height map
			if (!resyncHeight()) {
				// rollback
				setLastBlock(lLastBlock);
				resyncHeight();
			}
		}
	}

	//
	gLog().write(Log::STORE, std::string("[reindex]: reindex FINISHED for ") + 
		strprintf("%s#", chain_.toHex().substr(0, 10)));

	return lResult;
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

bool TransactionStore::airdropped(const uint160& address, const uint160& peer) {
	//
	if (settings_->supportAirdrop()) {
		//
		gLog().write(Log::STORE, std::string("[airdropped]: try to check ") + 
			strprintf("address_id = %s, peer_id = %s", address.toHex(), peer.toHex()));
		//
		uint256 lTx;
		if (airDropAddressesTx_.read(address, lTx)) {
			//
			gLog().write(Log::STORE, std::string("[airdropped]: already AIRDROPPED to ") + 
				strprintf("address_id = %s, peer_id = %s", address.toHex(), peer.toHex()));
			return true;
		} else {
			//
			db::DbTwoKeyContainer<
				uint160 /*peer_key_id*/, 
				uint160 /*address_id*/, 
				uint256 /*tx*/>::Iterator lFrom = airDropPeers_.find(peer);
			//
			lFrom.setKey2Empty();
			//
			bool lAirdropped = false;
			for (int lCount = 0; lFrom.valid(); ++lFrom) {
				//
				uint160 lPeer;
				uint160 lAddress;
				if (lFrom.first(lPeer, lAddress)) {
					//
					if (address == lAddress) { lAirdropped = true; break; }
				}

				if (++lCount > 1 /*2 different addresses from 1 peer*/) { lAirdropped = true; break; }
			}

			if (lAirdropped) {
				//
				gLog().write(Log::STORE, std::string("[airdropped]: already AIRDROPPED to ") + 
					strprintf("address_id = %s, peer_id = %s", address.toHex(), peer.toHex()));
				return true;
			}

			return false;
		}
	}

	return true;
}

void TransactionStore::pushAirdropped(const uint160& address, const uint160& peer, const uint256& tx) {
	//
	airDropAddressesTx_.write(address, tx);
	airDropPeers_.write(peer, address, tx);
}
