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

TransactionPtr TransactionStore::locateMempoolTransaction(const uint256& tx) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[locateMempoolTransaction]: try to load transaction from mempool ") + 
		strprintf("%s/%s#", 
			tx.toHex(), chain_.toHex().substr(0, 10)));

	IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate(chain_);
	if (lMempool) {
		//
		return lMempool->locateTransaction(tx);
	} else {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[locateMempoolTransaction]: mempool was not found ") +
			strprintf("%s/%s#",tx.toHex(), chain_.toHex().substr(0, 10)));
		return nullptr;
	}

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
			(!approxHeight ? TransactionContext::STORE_REINDEX : (processWallet ? TransactionContext::STORE_COMMIT : TransactionContext::STORE_PUSH)),
			ctx->block()->time());
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlockTransactions]: processing tx ") +
			strprintf("%s/%s/%s#", lCtx->tx()->id().toHex(), ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));

		// check presence
		uint256 lTxIdUtxo;
		db::DbMultiContainerShared<uint256 /*tx*/, uint256 /*utxo*/>::Iterator lTxUtxo = txUtxo_.find(lCtx->tx()->id());
		if (approxHeight && lTxUtxo.valid() && lTxUtxo.first(lTxIdUtxo) && lTxIdUtxo == lCtx->tx()->id() /* double check */) {
			//
			Transaction::UnlinkedOut lUtxo;
			if (utxo_.read(*lTxUtxo, lUtxo) || ltxo_.read(*lTxUtxo, lUtxo)) {
				gLog().write(Log::GENERAL_ERROR,
					strprintf("[processBlockTransactions/error]: transaction ALREADY processed %s/%s/%s#",
						lCtx->tx()->id().toHex(), ctx->block()->hash().toHex(), chain_.toHex().substr(0, 10)));
				lHasErrors = true;
				lBlockCtx->addError(lCtx->tx()->id(), "E_TX_ALREADY_PROCESSED - Transaction is already processed");
			}
		}

		// check rules
		if (!lHasErrors) {
			if (!lProcessor.process(lCtx)) {
				//
				bool lHandled = false;
				if ((lCtx->errorsContains("UNKNOWN_REFTX") || lCtx->errorsContains("INVALID_UTXO")) && /*lCtx->errorsContains("0000000000#") &&*/ !approxHeight) {
					lHandled = true; // skipped and will NOT be indexed
				} else  {
					lHasErrors = true;
					lBlockCtx->addErrors(lCtx->tx()->id(), lCtx->errors());
				}

				for (std::list<std::string>::iterator lErr = lCtx->errors().begin(); lErr != lCtx->errors().end(); lErr++) {
					gLog().write(Log::GENERAL_ERROR, strprintf("[processBlockTransactions/error]: %s / %s",
						(lHandled ? "SKIPPED" : "elevated"), (*lErr)));
				}

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
					gLog().write(Log::GENERAL_ERROR, std::string("[processBlockTransactions/push/error]: Block with utxo inconsistency - ") + 
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
					gLog().write(Log::GENERAL_ERROR, std::string("[processBlockTransactions/pop/error]: Block with utxo inconsistency - ") + 
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
					gLog().write(Log::GENERAL_ERROR, std::string("[processBlockTransactions/wallet/push/error]: Block with utxo inconsistency - ") + 
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
					gLog().write(Log::GENERAL_ERROR, std::string("[processBlockTransactions/wallet/pop/error]: Block with utxo inconsistency - ") + 
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

					std::string lEntityName = lTx->entityName();
					std::transform(lEntityName.begin(), lEntityName.end(), lEntityName.begin(), ::tolower);
					entitiesIdx_.remove(lEntityName);

					// iterate
					db::DbMultiContainerShared<std::string /*short name*/, uint256 /*utxo*/>::Transaction lEntityUtxoTransaction = entityUtxo_.transaction();
					for (db::DbMultiContainerShared<std::string /*short name*/, uint256 /*utxo*/>::Iterator lEntityUtxo = entityUtxo_.find(lTx->entityName()); lEntityUtxo.valid(); ++lEntityUtxo) {
						lEntityUtxoTransaction.remove(lEntityUtxo);
					}

					lEntityUtxoTransaction.commit();

					db::DbMultiContainerShared<uint256 /*entity*/, uint256 /*shard*/>::Transaction lShardsTransaction = shards_.transaction();
					for (db::DbMultiContainerShared<uint256 /*entity*/, uint256 /*shard*/>::Iterator lShards = shards_.find(lTx->id()); lShards.valid(); ++lShards) {
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

		//
		// mark processed
		blockUtxoIdx_.write(lHash, TxBlockAction(TxBlockAction::UNDEFINED, uint256(), nullptr));

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
		uint64_t lHeight;
		if (blockMap_.read(lIndex.block(), lHeight)) {
			if (!lIndex.index()) coinbase = true;
			else coinbase = false;
			height = lHeight;
			block = lIndex.block();
			index = lIndex.index();

			uint64_t lFirst;
			db::DbContainer<uint64_t, uint256>::Iterator lHead = heightMap_.last();
			if (lHead.valid() && lHead.first(lFirst) && lFirst >= height) { 
				confirms = (lFirst - height) + 1; 
			} else confirms = 0;

			return true;
		}
	}

	return false;	
}

bool TransactionStore::blockHeight(const uint256& block, uint64_t& height) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	uint64_t lHeight;
	if (blockMap_.read(block, lHeight)) {
		height = lHeight;
		return true;
	}

	return false;
}

bool TransactionStore::blockHeaderHeight(const uint256& block, uint64_t& height, BlockHeader& header) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	uint64_t lHeight;
	if (blockMap_.read(block, lHeight)) {
		height = lHeight;
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

bool TransactionStore::isHeaderReachable(const uint256& from, const uint256& to, uint64_t& diff, uint64_t limit) {
	//
	BlockHeader lHeader;
	uint256 lHash = from;

	//
	uint256 lNull = BlockHeader().hash();

	//
	bool lTraced = true;
	while (headers_.read(lHash, lHeader)) {
		// reached
		bool lReached = (lHash == to);
		// check block data
		if (blockExists(lHash)) {
			// check
			if (lReached) break; // we found hard link
			// push
			lHash = lHeader.prev();
		} else {
			lTraced = false;
			break;
		}

		if (lHash == lNull) {
			lTraced = false;
			break;
		}

		if (diff++ >= limit) {
			lTraced = false;
			break;
		}
	}

	if (!lTraced) return false;
	if (lHash == to) return true;

	return false;
}

bool TransactionStore::isRootExists(const uint256& lastRoot, const uint256& newRoot, uint256& commonRoot, uint64_t& depth, uint64_t limit) {
	//
	BlockHeader lHeaderLeft, lHeaderRight;
	uint256 lHashLeft = lastRoot;
	uint256 lHashRight = newRoot;

	//
	uint256 lNull = BlockHeader().hash();

	//
	std::set<uint256> lThread;

	//
	bool lTraced = false;
	while (headers_.read(lHashLeft, lHeaderLeft) && headers_.read(lHashRight, lHeaderRight)) {
		// check
		if (!lThread.insert(lHashLeft).second){
			//
			commonRoot = lHashLeft;
			lTraced = true;
			break;
		}

		if (!lThread.insert(lHashRight).second) {
			//
			commonRoot = lHashRight;
			lTraced = true;
			break;
		}

		// check
		if (lHashLeft == lHashRight) {
			commonRoot = lHashLeft;
			lTraced = true;
			break;
		}

		// push
		lHashLeft = lHeaderLeft.prev();
		lHashRight = lHeaderRight.prev();

		if (lHashLeft == lNull || lHashRight == lNull) {
			break;
		}

		if (depth++ >= limit) {
			break;
		}
	}

	return lTraced;	
}

void TransactionStore::invalidateHeightMap() {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);

	//
	heightMap_.close();
	blockMap_.close();

	//
	rmpath((settings_->dataPath() + "/" + chain_.toHex() + std::string("/height_map")).c_str());
	rmpath((settings_->dataPath() + "/" + chain_.toHex() + std::string("/block_map")).c_str());

	//
	heightMap_.open();
	blockMap_.open();
}

bool TransactionStore::resyncHeight() {
	//
	BlockHeader lHeader;
	uint256 lHash = lastBlock_;
	uint64_t lIndex = 0;

	//
	uint256 lNull = BlockHeader().hash();

	//
	std::string lSeqPath = strprintf("%s/%s/s-%d", settings_->dataPath(), chain_.toHex(), qbit::getMicroseconds());
	db::DbListContainer<uint256> lSeqHeaders(lSeqPath);
	if (mkpath(lSeqPath.c_str(), 0777)) return false;
	lSeqHeaders.open();

	//
	size_t lCount = 0;
	std::list<uint256> lSeq;
	while (lHash != lNull && headers_.read(lHash, lHeader)) {
		// push
		lSeqHeaders.write(lHash);
		//
		if (!(++lCount % 1000))
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/info]: loading ") + 
					strprintf("id = %d, prev_block = %s, block = %s, chain = %s#", 
						lCount, lHash.toHex(), lHeader.prev().toHex(), chain_.toHex().substr(0, 10)));
		//
		lHash = lHeader.prev();
	}

	//
	invalidateHeightMap();

	//
	if (lHash != lNull && !lastBlock_.isNull()) {
		/*
		if (lHash == lHeader.hash()) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/error]: block data is MISSING for ") + 
					strprintf("block = %s, chain = %s#", 
						lHash.toHex(), chain_.toHex().substr(0, 10)));
		} else 
		*/
		{
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/error]: chain is BROKEN on ") + 
					strprintf("prev_block = %s, block = %s, chain = %s#", 
						lHash.toHex(), lHeader.hash().toHex(), chain_.toHex().substr(0, 10)));
		}

		lSeqHeaders.close();
		rmpath(lSeqPath.c_str());

		return false;
	}

	//
	{
		boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
		for (db::DbListContainer<uint256>::Iterator lIter = lSeqHeaders.last(); lIter.valid(); lIter--) {
			//
			heightMap_.write(++lIndex, *lIter);
			blockMap_.write(*lIter, lIndex);

			if (!(--lCount % 1000))
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/info]: writing ") + 
						strprintf("id = %d, block = %s, chain = %s#", 
							lCount, (*lIter).toHex(), chain_.toHex().substr(0, 10)));
		}
	}

	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight]: current ") + strprintf("height = %d, block = %s, chain = %s#", lIndex, lastBlock_.toHex(), chain_.toHex().substr(0, 10)));

	lSeqHeaders.close();
	rmpath(lSeqPath.c_str());

	return true;
}

bool TransactionStore::resyncHeight(const uint256& to) {
	//
	BlockHeader lHeader;
	uint256 lHash = lastBlock_;
	uint32_t lPartialTree = 0;

	//
	IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate(chain_);
	lPartialTree = (lMempool ? lMempool->consensus()->partialTreeThreshold() : 10) * 10; // ten's size of partial tree

	//
	std::list<uint256> lSeq;
	while (headers_.read(lHash, lHeader)) {
		// push
		lSeq.push_back(lHash);

		// check
		if (lHash == to) break;

		// next
		lHash = lHeader.prev();
	}

	if (lHash != to && !lastBlock_.isNull()) {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/partial/error]: chain is BROKEN on ") + 
				strprintf("prev_block = %s, block = %s, chain = %s#", 
					lHash.toHex(), lHeader.hash().toHex(), chain_.toHex().substr(0, 10)));

		return false;
	}

	//
	bool lFull = false;
	uint64_t lLastIndex = 0;
	//
	if (/*lSeq.size() <= lPartialTree &&*/ lSeq.size()) {
		// partial resync
		boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
		// locate last index
		uint64_t lHeight;
		uint256 lBlock;
		if (blockMap_.read(*lSeq.rbegin(), lHeight)) {
			// "to" found
			lLastIndex = lHeight; // "to" index
			// clean-up
			std::list<uint256>::reverse_iterator lIter = ++(lSeq.rbegin()); // next from "to"
			for (; lIter != lSeq.rend(); lIter++) {
				if (blockMap_.read(*lIter, lHeight)) { 
					heightMap_.remove(lHeight);
					blockMap_.remove(*lIter);
				}
			}
			// build up
			lIter = ++(lSeq.rbegin()); // next from "to"
			for (; lIter != lSeq.rend(); lIter++) {
				uint256 lHeaderId;
				if (heightMap_.read(++lLastIndex, lHeaderId)) {
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/partial/warning]: height map is not clean, replacing for ") + 
							strprintf("index = %d, block = %s, to = %s, last = %s, chain = %s#", 
								lLastIndex, (*lIter).toHex(), to.toHex(), lastBlock_.toHex(), chain_.toHex().substr(0, 10)));
					heightMap_.remove(lLastIndex);
				}
				//
				heightMap_.write(lLastIndex, *lIter);
				//
				uint64_t lHeightId;
				if (blockMap_.read(*lIter, lHeightId)) {
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/partial/warning]: block map is not clean, replacing for ") + 
							strprintf("block = %s, to = %s, last = %s, chain = %s#", 
								(*lIter).toHex(), to.toHex(), lastBlock_.toHex(), chain_.toHex().substr(0, 10)));
					blockMap_.remove(*lIter);
				}
				//
				blockMap_.write(*lIter, lLastIndex);
			}
			// check
			uint64_t lLastHeight;
			uint256 lLastBlock;
			db::DbContainer<uint64_t, uint256>::Iterator lTop = heightMap_.last();
			if (lTop.valid() && lTop.first(lLastHeight) && lTop.second(lLastBlock) && lLastBlock != lastBlock_) {
				// full resync
				lFull = true;
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/partial/error]: top is not last, making FULL resync for ") + 
						strprintf("to = %s, last = %s, lastHeight = %d / %s, chain = %s#", 
							to.toHex(), lastBlock_.toHex(), lLastHeight, lLastBlock.toHex(), chain_.toHex().substr(0, 10)));
			}
		} else {
			// full resync
			lFull = true;
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/partial/error]: root was not found, making FULL resync for ") + 
					strprintf("to = %s, chain = %s#", 
						to.toHex(), chain_.toHex().substr(0, 10)));
		}
	} else if (false /*lSeq.size() > lPartialTree*/) {
		lFull = true;
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/partial/error]: subtree is LARGE, making FULL resync for ") + 
				strprintf("to = %s, delta = %d, chain = %s#", 
					to.toHex(), lSeq.size(), chain_.toHex().substr(0, 10)));
	}

	//
	if (lFull) return resyncHeight();
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[resyncHeight/partial]: current ") + strprintf("height = %d, block = %s, chain = %s#", lLastIndex, lastBlock_.toHex(), chain_.toHex().substr(0, 10)));

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
		ltxo_.remove(utxo); // sanity
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
void TransactionStore::removeBlocks(const uint256& from, const uint256& to, bool removeData, uint64_t limit) {
	// remove indexed data
	uint256 lHash = from;
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
		strprintf("removing blocks data [%s-%s]/%s#", from.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));		
	//
	uint64_t lLimit = 0;
	BlockHeader lHeader;
	while(lHash != to && headers_.read(lHash, lHeader) && (!limit || lLimit++ < limit)) {
		// begin transaction
		db::DbMultiContainerShared<uint256 /*block*/, TxBlockAction /*utxo action*/>::Transaction lBlockIdxTransaction = blockUtxoIdx_.transaction();
		// prepare reverse queue
		std::list<TxBlockAction> lQueue;
		std::set<uint256> lTouchedTxs;
		uint256 lNullUtxo;
		// iterate
		for (db::DbMultiContainerShared<uint256 /*block*/, TxBlockAction /*utxo action*/>::Iterator lUtxo = blockUtxoIdx_.find(lHash); lUtxo.valid(); ++lUtxo) {
			// check & push
			if ((*lUtxo).utxo() != lNullUtxo) lQueue.push_back(*lUtxo);
			// mark to remove
			lBlockIdxTransaction.remove(lUtxo);
		}
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
			strprintf("removing block data for %d/%s/%s#", lQueue.size(), lHash.toHex(), chain_.toHex().substr(0, 10)));		
		// reverse traverse
		for (std::list<TxBlockAction>::reverse_iterator lUtxo = lQueue.rbegin(); lUtxo != lQueue.rend(); lUtxo++) {
			// reconstruct utxo
			TxBlockAction lAction = *lUtxo;
			if (lAction.action() == TxBlockAction::PUSH) {
				// remove pushed utxo
				Transaction::UnlinkedOut lUtxoObj;
				if (utxo_.read(lAction.utxo(), lUtxoObj)) {
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
						strprintf("remove utxo = %s, tx = %s/%s/%s#",
							lAction.utxo().toHex(), lUtxoObj.out().tx().toHex(), lHash.toHex(),
							chain_.toHex().substr(0, 10)));

					utxo_.remove(lAction.utxo());
					ltxo_.remove(lAction.utxo()); // sanity
					utxoBlock_.remove(lAction.utxo()); // just for push
					addressAssetUtxoIdx_.remove(lUtxoObj.address().id(), lUtxoObj.out().asset(), lAction.utxo());
					wallet_->tryRemoveUnlinkedOut(lAction.utxo()); // try wallet
					lTouchedTxs.insert(lUtxoObj.out().tx()); // mark as visited
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
					wallet_->tryRevertUnlinkedOut(lAction.utxo()); // try wallet
					lTouchedTxs.insert(lUtxoObj.out().tx()); // mark as visited

					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
						strprintf("remove/add ltxo/utxo = %s, tx = %s/%s/%s#",
							lAction.utxo().toHex(),
							lUtxoObj.out().tx().toHex(), lHash.toHex(),
							chain_.toHex().substr(0, 10)));
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

		// clear transactions
		BlockTransactionsPtr lTransactions = transactions_.read(lHash);
		if (lTransactions) {
			for(TransactionsContainer::iterator lTx = lTransactions->transactions().begin(); lTx != lTransactions->transactions().end(); lTx++) {
				//
				TxBlockIndex lIndex;
				if (transactionsIdx_.read((*lTx)->id(), lIndex) && lIndex.block() != lHash) {
					// if transaction NOT belongs to the current block;
					// that can happened in the process of consensus making - tx can be added by the two miners in the same time
					// so conflict may occure and tx index should stay intact
					continue;
				}

				//
				if (extension_) extension_->removeTransaction(*lTx);

				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[removeBlocks]: ") +
					strprintf("removing tx index %s/%s/%s#", (*lTx)->id().toHex(), lHash.toHex(), chain_.toHex().substr(0, 10)));

				transactionsIdx_.remove((*lTx)->id());

				// iterate
				db::DbMultiContainerShared<uint256 /*tx*/, uint256 /*utxo*/>::Transaction lTxUtxoTransaction = txUtxo_.transaction();
				for (db::DbMultiContainerShared<uint256 /*tx*/, uint256 /*utxo*/>::Iterator lTxUtxo = txUtxo_.find((*lTx)->id()); lTxUtxo.valid(); ++lTxUtxo) {
					lTxUtxoTransaction.remove(lTxUtxo);

					// double check for the "lost" utxo's
					if (lTouchedTxs.find((*lTx)->id()) == lTouchedTxs.end()) {
						utxo_.remove(*lTxUtxo);
						ltxo_.remove(*lTxUtxo);
					}
				}

				lTxUtxoTransaction.commit();			

				if ((*lTx)->isEntity() && (*lTx)->entityName() != Entity::emptyName()) {
					entities_.remove((*lTx)->entityName());
					
					std::string lEntityName = (*lTx)->entityName();
					std::transform(lEntityName.begin(), lEntityName.end(), lEntityName.begin(), ::tolower);
					entitiesIdx_.remove(lEntityName);

					// iterate
					db::DbMultiContainerShared<std::string /*short name*/, uint256 /*utxo*/>::Transaction lEntityUtxoTransaction = entityUtxo_.transaction();
					for (db::DbMultiContainerShared<std::string /*short name*/, uint256 /*utxo*/>::Iterator lEntityUtxo = entityUtxo_.find((*lTx)->entityName()); lEntityUtxo.valid(); ++lEntityUtxo) {
						lEntityUtxoTransaction.remove(lEntityUtxo);
					}

					lEntityUtxoTransaction.commit();

					db::DbMultiContainerShared<uint256 /*entity*/, uint256 /*shard*/>::Transaction lShardsTransaction = shards_.transaction();
					for (db::DbMultiContainerShared<uint256 /*entity*/, uint256 /*shard*/>::Iterator lShards = shards_.find((*lTx)->id()); lShards.valid(); ++lShards) {
						shardEntities_.remove(*lShards);
						lShardsTransaction.remove(lShards);
					}

					lShardsTransaction.commit();
				}
			}
		}

		// commit changes
		lBlockIdxTransaction.commit();

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
	for (db::DbMultiContainerShared<std::string /*short name*/, uint256 /*utxo*/>::Iterator lUtxo = entityUtxo_.find(name); lUtxo.valid(); ++lUtxo) {
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
		for (db::DbMultiContainerShared<uint256 /*entity*/, uint256 /*shard*/>::Iterator lShard = shards_.find(lDAppTx); lShard.valid(); ++lShard) {
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
		for (db::DbMultiContainerShared<uint256 /*entity*/, uint256 /*shard*/>::Iterator lShard = shards_.find(lDAppTx); lShard.valid(); ++lShard) {
			//
			uint32_t lCount = 0;
			uint32_t lMainCount = 0;
			shardEntities_.read(*lShard, lMainCount);

			//
			ITransactionStorePtr lStore = storeManager()->locate(*lShard);
			if (lStore) {
				//
				if (lStore->entityStore()->entityCount(lCount)) {
					result[*lShard] = lMainCount + lCount;
				} else {
					result[*lShard] = lMainCount;
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

#define ERROR_REASON_TX_DATA_MISSING 1
#define ERROR_REASON_INTEGRITY_ERROR 2
#define ERROR_REASON_EXTENDED_CHECK_NOT_PASSED 3
#define ERROR_REASON_TX_INTEGRITY_ERROR 4

//
// interval [..)
bool TransactionStore::processBlocks(const uint256& from, const uint256& to, IMemoryPoolPtr pool, uint256& last, int& errorReason) {
	//
	std::list<uint256> lHeadersSeq;
	uint256 lHash = from;
	uint256 lPrev = from;

	//
	if (!wallet_->mempoolManager()) return false;
	IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate(chain_);

	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks]: ") +
		strprintf("processing blocks data [%s-%s]/%s#", from.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));		
	//
	bool lDone = false;
	bool lFound = false;
	BlockHeader lHeader;
	while(!lDone && headers_.read(lHash, lHeader)) {
		//
		lFound = lFound ? lFound : lHash == to;
		//
		if (lFound) { 
			// ONLY in case that the block is indexed, otherwise - go forward
			if (blockIndexed(lHash)) {
				lDone = true;
			} else {
				//
				lHeadersSeq.push_back(lHash);
				// next
				lPrev = lHash;
				lHash = lHeader.prev();				
			}

			// TODO: postpone for now
			/*
			uint64_t lHeight;
			if (!blockHeight(lHash, lHeight)) {
				lHeadersSeq.push_back(lHeader); // re-process "to" in case of fast and cross clean-ups 
			}
			*/
		} else {
			//
			lHeadersSeq.push_back(lHash);
			// next
			lPrev = lHash;
			lHash = lHeader.prev();
		}
	}

	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks]: processing depth = ") +
		strprintf("%d, %s-%s/%s#", lHeadersSeq.size(), from.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));

	// traverse
	for (std::list<uint256>::reverse_iterator lBlockId = lHeadersSeq.rbegin(); lBlockId != lHeadersSeq.rend(); lBlockId++) {
		//
		uint256 lBlockHash = (*lBlockId);
		BlockHeader lBlock;
		headers_.read(lBlockHash, lBlock); // load header
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks]: processing block ") +
			strprintf("%s/%s#", lBlockHash.toHex(), chain_.toHex().substr(0, 10)));

		// check sequence consistency
		bool lExtended = true;
		if (lMempool && !lMempool->consensus()->checkSequenceConsistency(lBlock, lExtended)) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks/error]: check basic sequence consistency FAILED ") +
				strprintf("block = %s, prev = %s, chain = %s#", lBlockHash.toHex(), lBlock.prev().toHex(), chain_.toHex().substr(0, 10)));
			errorReason = ERROR_REASON_INTEGRITY_ERROR;
			last = lBlockHash;
			return false;
		} else if (!lExtended && !settings_->reindex()) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks/error]: check extended sequence consistency FAILED ") +
				strprintf("block = %s, prev = %s, chain = %s#", lBlockHash.toHex(), lBlock.prev().toHex(), chain_.toHex().substr(0, 10)));
			errorReason = ERROR_REASON_EXTENDED_CHECK_NOT_PASSED;
			last = lBlockHash;
			return false;
		}

		//
		BlockContextPtr lBlockCtx = BlockContext::instance(Block::instance(lBlock)); // just header without transactions attached
		BlockTransactionsPtr lTransactions = transactions_.read(lBlockHash);
		if (!lTransactions) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[processBlocks/error]: transaction DATA is ABSENT for ") +
				strprintf("block = %s, prev = %s, chain = %s#", lBlockHash.toHex(), lBlock.prev().toHex(), chain_.toHex().substr(0, 10)));
			errorReason = ERROR_REASON_TX_DATA_MISSING;
			last = lBlock.prev(); // suppose, that the previous one is good enouht
			return false;
		}

		lBlockCtx->block()->append(lTransactions); // reconstruct block consistensy

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
			// mark processed
			blockUtxoIdx_.write(lBlockHash, TxBlockAction(TxBlockAction::UNDEFINED, uint256(), nullptr));

			//
			lBlockCtx->block()->compact(); // compact txs to just ids
			if (pool) pool->removeTransactions(lBlockCtx->block());
		} else {
			//
			lBlockCtx->block()->compact(); // compact txs to just ids
			if (pool) pool->removeTransactions(lBlockCtx->block());
			last = lBlock.prev(); // suppose, that the previous one is good enouht
			errorReason = ERROR_REASON_TX_INTEGRITY_ERROR;
			return false;
		}
	}

	return true;
}

uint64_t TransactionStore::currentHeight(BlockHeader& block) {
	//
	if (!opened_) return 0;
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	uint64_t lHeight;
	db::DbContainer<uint64_t, uint256>::Iterator lHeader = heightMap_.last();
	if (lHeader.valid() && lHeader.first(lHeight)) {
		// validate
		if (headers_.read(*lHeader, block)) {
			if ((*lHeader) == lastBlock_) {
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[currentHeight]: ") + strprintf("%d/%s/%s#", lHeight, block.hash().toHex(), chain_.toHex().substr(0, 10)));
				return lHeight;
			} else {
				// lastBlock_ was rolled
				if (!lastBlock_.isNull()) {
					db::DbContainer<uint256, uint64_t>::Iterator lBlockHeader = blockMap_.find(lastBlock_);
					if (lBlockHeader.valid() && lBlockHeader.second(lHeight)) {
						if (headers_.read(lastBlock_, block)) {
							if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[currentHeight/rolled]: ") + strprintf("%d/%s/%s#", lHeight, block.hash().toHex(), chain_.toHex().substr(0, 10)));
							return lHeight;
						}
					}		
				}
			}
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[currentHeight/undefined]: ") + strprintf("%d/%s/%s#", lHeight, block.hash().toHex(), chain_.toHex().substr(0, 10)));
			return lHeight;
		} else {
			db::DbContainer<uint256, uint64_t>::Iterator lBlockHeader = blockMap_.find(lastBlock_);
			if (lBlockHeader.valid() && lBlockHeader.second(lHeight)) {
				if (headers_.read(lastBlock_, block)) {
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[currentHeight/lastKnown]: ") + strprintf("%d/%s/%s#", lHeight, block.hash().toHex(), chain_.toHex().substr(0, 10)));
					return lHeight;
				}
			}
		}
	}

	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[currentHeight/missing]: ") + strprintf("%d/%s/%s#", 0, BlockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));
	return 0;
}

uint64_t TransactionStore::top() {
	//
	if (!opened_) return 0;
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	uint64_t lHeight;
	db::DbContainer<uint64_t, uint256>::Iterator lHeader = heightMap_.last();
	if (lHeader.valid() && lHeader.first(lHeight)) {
		return lHeight;
	}

	return 0;
}

uint64_t TransactionStore::pushNewHeight(const uint256& block) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	uint64_t lHeight = 1, lTopHeight = 0;
	db::DbContainer<uint64_t, uint256>::Iterator lTop = heightMap_.last();
	if (lTop.valid() && lTop.first(lTopHeight)) {
		lHeight = lTopHeight + 1;
	}

	heightMap_.write(lHeight, block);
	blockMap_.write(block, lHeight);

	return lHeight;
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

void TransactionStore::writeLastReindex(const uint256& from, const uint256& to) {
	//
	if (!opened_) return;

	boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	workingSettings_.write("lastReindex", strprintf("%s|%s", from.toHex(), to.toHex()));
}

BlockPtr TransactionStore::block(uint64_t height) {
	//
	BlockHeader lHeader;
	uint256 lHash;
	bool lFound = false;
	{
		boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
		if (heightMap_.read(height, lHash)) {
			lFound = true;
		} else {
			return nullptr;
		}
	}

	if (lFound && headers_.read(lHash, lHeader)) {
		BlockTransactionsPtr lTransactions = transactions_.read(lHash);
		if (lTransactions)
			return Block::instance(lHeader, lTransactions);
	}

	return nullptr;
}

bool TransactionStore::blockHeader(uint64_t height, BlockHeader& header) {
	//
	uint256 lHash;
	bool lFound = false;
	{
		boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
		if (heightMap_.read(height, lHash)) {
			lFound = true;
		} else {
			return false;
		}
	}

	return lFound && headers_.read(lHash, header);
}

BlockPtr TransactionStore::block(const uint256& id) {
	//
	BlockHeader lHeader;
	uint256 lHash = id;

	if (headers_.read(lHash, lHeader)) {
		BlockTransactionsPtr lTransactions = transactions_.read(lHash);
		if (lTransactions)
			return Block::instance(lHeader, lTransactions);
	}

	return nullptr;	
}

bool TransactionStore::open() {
	//
	if (!opened_) { 
		try {
			gLog().write(Log::INFO, std::string("[open]: opening store for ") + strprintf("%s", chain_.toHex()));

			if (mkpath(std::string(settings_->dataPath() + "/" + chain_.toHex() + "/data").c_str(), 0777)) return false;

			workingSettings_.open();
			workingSettings_.attach();

			headers_.open();
			headers_.attach();

			transactions_.open();
			transactions_.attach();

			utxo_.open();
			utxo_.attach();

			ltxo_.open();
			ltxo_.attach();

			entities_.open();
			entities_.attach();

			blockUtxoIdx_.open();
			blockUtxoIdx_.attach();

			utxoBlock_.open();
			utxoBlock_.attach();

			transactionsIdx_.open();
			transactionsIdx_.attach();

			entitiesIdx_.open();
			entitiesIdx_.attach();

			addressAssetUtxoIdx_.open();
			addressAssetUtxoIdx_.attach();

			shards_.open();
			shards_.attach();

			entityUtxo_.open();
			entityUtxo_.attach();

			shardEntities_.open();
			shardEntities_.attach();

			txUtxo_.open();
			txUtxo_.attach();

			blocksQueue_.open();

			if (settings_->supportAirdrop()) {
				airDropAddressesTx_.open();
				airDropAddressesTx_.attach();

				airDropPeers_.open();
				airDropPeers_.attach();
			}

			// finally - open space
			if (settings_->repairDb()) {
				gLog().write(Log::INFO, std::string("[open]: repairing store for ") + strprintf("%s", chain_.toHex()));
				space_->repair();
			}
			//
			space_->open();

			// very first read begins here
			std::string lLastBlock;
			if (workingSettings_.read("lastBlock", lLastBlock)) {
				lastBlock_.setHex(lLastBlock);
			}

			gLog().write(Log::INFO, std::string("[open]: lastBlock = ") + strprintf("%s/%s#", lastBlock_.toHex(), chain_.toHex().substr(0, 10)));

			// open height index
			heightMap_.open();
			blockMap_.open();

			// push shards
			for (db::DbMultiContainerShared<uint256 /*entity*/, uint256 /*shard*/>::Iterator lShard = shards_.begin(); lShard.valid(); ++lShard) {
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

			// post-open check
			BlockHeader lHeader;
			if (!currentHeight(lHeader) && !lastBlock_.isNull()) {
				// try to reconstruct height&block map
				resyncHeight();
			}
		}
		catch(const std::exception& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[open/error]: ") + ex.what());
			return false;
		}
	}

	return opened_;
}

void TransactionStore::prepare() {
	//
	if (opened_) {
		gLog().write(Log::INFO, std::string("[prepare]: preparing store for ") + strprintf("%s", chain_.toHex()));

		// check interrupted reindex and re-process
		std::string lLastReindex;
		if (workingSettings_.read("lastReindex", lLastReindex)) {
			std::vector<std::string> lParts;
			boost::split(lParts, lLastReindex, boost::is_any_of("|"), boost::token_compress_on);
			if (lParts.size() == 2) {
				//
				uint256 lFrom; lFrom.setHex(lParts[0]);
				uint256 lTo; lTo.setHex(lParts[1]);

				//
				if (lFrom != uint256() && lTo != uint256()) {
					// reprocess last reindex
					gLog().write(Log::INFO, strprintf("[prepare/warning]: last reindex remains from = %s, to = %s/%s#",
						lFrom.toHex(), lTo.toHex(), chain_.toHex().substr(0, 10)));

					//
					// NOTICE: due to new error procesing technique during reindexing we eventually can find out that
					// we need full chain analyzis
					//
					// reindex(lFrom, lTo, nullptr);
				}
			}
		}
	}
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
	entitiesIdx_.close();
	addressAssetUtxoIdx_.close();
	blockUtxoIdx_.close();
	utxoBlock_.close();
	shards_.close();
	entityUtxo_.close();
	shardEntities_.close();
	txUtxo_.close();
	heightMap_.close();
	blockMap_.close();

	blocksQueue_.close();

	if (settings_->supportAirdrop()) {
		airDropAddressesTx_.close();
		airDropPeers_.close();
	}

	space_->close();

	if (extension_) extension_->close();

	settings_.reset();
	opened_ = false;

	return true;
}

bool TransactionStore::enumUnlinkedOuts(const uint256& tx, std::vector<Transaction::UnlinkedOutPtr>& outs) {
	//
	bool lResult = false;
	std::map<uint32_t, Transaction::UnlinkedOutPtr> lUtxos;
	for (db::DbMultiContainerShared<uint256 /*tx*/, uint256 /*utxo*/>::Iterator lTxUtxo = txUtxo_.find(tx); lTxUtxo.valid(); ++lTxUtxo) {
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
	for (db::DbThreeKeyContainerShared
			<uint160 /*address*/, 
				uint256 /*asset*/, 
				uint256 /*utxo*/, 
				uint256 /*tx*/>::Iterator lBundle = addressAssetUtxoIdx_.find(address.id()); lBundle.valid(); ++lBundle) {
		uint160 lAddress;
		uint256 lAsset;
		uint256 lUtxo;

		if (lBundle.first(lAddress, lAsset, lUtxo)) {
			Transaction::UnlinkedOut lOut, lLOut;
			if (utxo_.read(lUtxo, lOut) && !ltxo_.read(lUtxo, lLOut)) {
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
	for (db::DbThreeKeyContainerShared
			<uint160 /*address*/, 
				uint256 /*asset*/, 
				uint256 /*utxo*/, 
				uint256 /*tx*/>::Iterator lBundle = addressAssetUtxoIdx_.find(lId, asset); lBundle.valid(); ++lBundle) {
		uint160 lAddress;
		uint256 lAsset;
		uint256 lUtxo;

		if (lBundle.first(lAddress, lAsset, lUtxo)) {
			Transaction::UnlinkedOut lOut, lLOut;
			if (utxo_.read(lUtxo, lOut) && !ltxo_.read(lUtxo, lLOut)) {
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

void TransactionStore::selectUtxoByRawAddressAndAsset(const PKey& address, const uint256& asset, std::vector<Transaction::NetworkUnlinkedOut>& utxo, int limit) {
	//
	std::map<uint64_t, Transaction::NetworkUnlinkedOut> lUtxos;
	//
	int lCount = 0;
	uint160 lId = address.id();
	for (db::DbThreeKeyContainerShared
			<uint160 /*address*/, 
				uint256 /*asset*/, 
				uint256 /*utxo*/, 
				uint256 /*tx*/>::Iterator lBundle = addressAssetUtxoIdx_.find(lId, asset); lBundle.valid() && (limit == -1 || lCount < limit); ++lBundle, lCount++) {
		uint160 lAddress;
		uint256 lAsset;
		uint256 lUtxo;

		if (lBundle.first(lAddress, lAsset, lUtxo)) {
			Transaction::UnlinkedOut lOut, lLOut;
			if (utxo_.read(lUtxo, lOut) && !ltxo_.read(lUtxo, lLOut)) {
				//
				lUtxos[lOut.amount()] = Transaction::NetworkUnlinkedOut(lOut, 0, 0);
			}
		}
	}

	// ordering
	for (std::map<uint64_t, Transaction::NetworkUnlinkedOut>::reverse_iterator lItem = lUtxos.rbegin(); lItem != lUtxos.rend(); lItem++) {
		utxo.push_back(lItem->second);
	}
}

void TransactionStore::selectUtxoByRawTransaction(const uint256& tx, std::vector<Transaction::NetworkUnlinkedOut>& utxo) {
	//
	std::map<uint32_t, Transaction::NetworkUnlinkedOut> lUtxos;
	db::DbMultiContainerShared<uint256 /*tx*/, uint256 /*utxo*/>::Iterator lTxRoot = txUtxo_.find(tx);
	for (; lTxRoot.valid(); ++lTxRoot) {
		Transaction::UnlinkedOut lOut;
		if (utxo_.read(*lTxRoot, lOut)) {
			//
			lUtxos[lOut.out().index()] = Transaction::NetworkUnlinkedOut(lOut, 0, 0);
		}
	}

	// ordering
	for (std::map<uint32_t, Transaction::NetworkUnlinkedOut>::iterator lItem = lUtxos.begin(); lItem != lUtxos.end(); lItem++) {
		utxo.push_back(lItem->second);
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
	db::DbMultiContainerShared<uint256 /*tx*/, uint256 /*utxo*/>::Iterator lTxRoot = txUtxo_.find(tx);
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
	// control
	std::set<std::string> lControl;

	// NOTICE: extra search is unneeded now, just use entitiesIdx_ for complete entity name search
	/*
	// try direct names
	db::DbContainer<std::string, uint256>::Iterator lFrom = entities_.find(name);
	for (int lCount = 0; lFrom.valid() && names.size() < 6 && lCount < 100; ++lFrom, ++lCount) {
		std::string lName;
		if (lFrom.first(lName) && lName.find(name) != std::string::npos && lControl.insert(lName).second) {
			names.push_back(IEntityStore::EntityName(lName));
		}
	}
	*/

	{
		// try entity names index
		std::string lArgName = name;
		std::transform(lArgName.begin(), lArgName.end(), lArgName.begin(), ::tolower);
		//
		db::DbContainerShared<std::string, std::string>::Iterator lFromIndex = entitiesIdx_.find(lArgName);
		for (int lCount = 0; lFromIndex.valid() && names.size() < 6 && lCount < 100; ++lFromIndex, ++lCount) {
			std::string lName;
			if (lFromIndex.first(lName) && lName.find(lArgName) != std::string::npos && lControl.insert(*lFromIndex).second) {
				names.push_back(IEntityStore::EntityName(*lFromIndex));
			}
		}
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

	// try regular index
	if (entities_.read(ctx->tx()->entityName(), lId) && lId != id) {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushEntity]: entity ALREADY EXISTS - ") +
			strprintf("'%s'/%s", ctx->tx()->entityName(), id.toHex()));
		return false;
	}

	// double
	if (lId == id) {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushEntity]: DOUBLE PUSHED entity ") +
			strprintf("'%s'/%s", ctx->tx()->entityName(), id.toHex()));
	}

	// try entity name full text index
	std::string lEntityName = ctx->tx()->entityName();
	std::transform(lEntityName.begin(), lEntityName.end(), lEntityName.begin(), ::tolower);

	std::string lOriginalEntityName;
	if (entitiesIdx_.read(lEntityName, lOriginalEntityName)) {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[pushEntity]: entity ALREADY EXISTS - ") +
			strprintf("'%s'/%s", ctx->tx()->entityName(), id.toHex()));
		return false;
	}

	// main entity entry
	entities_.write(ctx->tx()->entityName(), id);

	// entity name full text index
	entitiesIdx_.write(lEntityName, ctx->tx()->entityName());

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
	// boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
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
	// tx to block|index
	uint32_t lIndex = 0;
	for(TransactionsContainer::iterator lTx = block->transactions().begin(); lTx != block->transactions().end(); lTx++) {
		//
		//if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[saveBlock]: writing index ") +
		//	strprintf("%s/block(%d/%s)/%s#", (*lTx)->id().toHex(), lIndex, lHash.toHex(), chain_.toHex().substr(0, 10)));
		//
		transactionsIdx_.write((*lTx)->id(), TxBlockIndex(lHash, lIndex++));
	}
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

bool TransactionStore::blockIndexed(const uint256& id) {
	//
	db::DbMultiContainerShared<uint256 /*block*/, TxBlockAction /*utxo action*/>::Iterator lAction = blockUtxoIdx_.find(id);
	return lAction.valid();
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
	bool lSkip = settings_->skipMainChainCleanUp() && chain_ == MainChain::id();
	if (!lSkip) removeBlocks(from, BlockHeader().hash(), false, 0);

	//
	uint256 lLastBlock = lastBlock_;

	//
	// NOICE: This potencially will lead to clean-up wallet utxos
	//

	/*
	// reset connected wallet cache
	wallet_->resetCache();
	// remove wallet utxo-binding data
	wallet_->cleanUpData();
	*/

	// probably last block
	uint256 lLastPrev;
	// error reason if any
	int lErrorReason = 0;
	// process blocks
	if (!processBlocks(from, BlockHeader().hash(), pool, lLastPrev, lErrorReason)) {
		//
		// NOTICE: reset to NULL block and invalidate height map
		// we have consistency errors and let sync procedure will decide
		//
		BlockHeader lNull;
		setLastBlock(lNull.hash());
	} else {
		// new last
		setLastBlock(from);
		// NOTICE: heavy weight operation!!!
		// re-build height map
		resyncHeight();
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

	// params
	uint64_t /*lLastHeight = 0, lToHeight = 0,*/ lDestinationDiff = 0, lLastBlockDiff = 0, lFromDiff = 0, lLimit = (60/2)*60*24*3; // far check distance
	uint256 lCommonRoot = BlockHeader().hash();

	// clean-up from lastBlock_
	bool lCommonRootFound = true;
	bool lDestinationFound = false;

	// check for new chain switching
	if (to == lCommonRoot /*zero block*/) {
		//
		gLog().write(Log::STORE, std::string("[reindex]: performing FULL reindex for new chain ") + 
			strprintf("%s/%s/%s#", from.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));
	} else {
		// NOTICE: not relevant
		// WARNING: lastBlock AND to MUST be in current chain - otherwise all indexes will be invalidated
		/*
		if (!blockExists(lastBlock_) || !blockExists(to)) {
			//
			gLog().write(Log::STORE, std::string("[reindex]: partial reindex is NOT POSSIBLE for ") + 
				strprintf("%s/%s/%s#", lastBlock_.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));

			return false;
		}
		*/

		// check is block headers (our lastBlock and "from" has common root) are down-traceable (headers only)
		if ((lCommonRootFound = isRootExists(lastBlock_, from, lCommonRoot, lLastBlockDiff, lLimit))) {
			//
			gLog().write(Log::STORE, std::string("[reindex/warning]: limited clean-up with partial reindex is POSSIBLE for ") + 
				strprintf("depth = %d, lastBlock = %s, from = %s, root = %s/%s#",
					lLastBlockDiff, lastBlock_.toHex(), from.toHex(), lCommonRoot.toHex(), chain_.toHex().substr(0, 10)));
		} else {
			//
			gLog().write(Log::STORE, std::string("[reindex/warning]: removing OLD data is not possible, because NO common root is found for ") + 
				strprintf("depth = %d, lastBlock = %s, from = %s/%s#", lLastBlockDiff, lastBlock_.toHex(), from.toHex(), chain_.toHex().substr(0, 10)));
		}

		// check is "from" is traceable to "to" (headers and tx existence)
		if ((lDestinationFound = isHeaderReachable(from, to, lDestinationDiff, lLimit))) {
			//
			gLog().write(Log::STORE, std::string("[reindex/info]: from -> to is REACHABLE with ") + 
				strprintf("depth = %d, from = %s, to = %s/%s#",
					lDestinationDiff, from.toHex(), to.toHex(), chain_.toHex().substr(0, 10)));
		} else {
			//
			gLog().write(Log::STORE, std::string("[reindex/warning]: from -> to is NOT REACHABLE, using common root with ") + 
				strprintf("depth = %d, lastBlock = %s, from = %s, root = %s/%s#",
					lLastBlockDiff, lastBlock_.toHex(), from.toHex(), lCommonRoot.toHex(), chain_.toHex().substr(0, 10)));
		}
	}

	//
	bool lResult = true;

	// save prev_last
	uint256 lLastBlock = lastBlock_;
	//
	bool lResyncHeight = false;
	bool lRemoveTransactions = false;
	{
		//
		boost::unique_lock<boost::recursive_mutex> lLock(storageCommitMutex_);
		// synchronizing
		SynchronizingGuard lGuard(shared_from_this());
		// mark last reindex - as requested
		writeLastReindex(from, to);
		// remove old index (limited)
		if (lCommonRootFound) removeBlocks(lastBlock_, lCommonRoot, false, 0);
		// remove new index (NOTICE: if "from -!> to" and commot root found - use common root as destination)
		removeBlocks(from, (!lDestinationFound && lCommonRootFound ? lCommonRoot : to), false, 0); // in case of wrapped restarts (re-process blocks may occur)
		// probably last block
		uint256 lLastPrev;
		// error reason if any
		int lErrorReason = 0;
		// process blocks (NOTICE: if "from -!> to" and commot root found - use common root as destination)
		if (!(lResult = processBlocks(from, (!lDestinationFound && lCommonRootFound ? lCommonRoot : to), pool, lLastPrev, lErrorReason))) {
			/*
			//
			// NOTICE: reset to NULL block and invalidate height map
			// we have consistency errors and let sync procedure will decide
			//
			BlockHeader lNull;
			setLastBlock(lNull.hash());
			invalidateHeightMap();
			*/

			if (lErrorReason == ERROR_REASON_TX_DATA_MISSING ||
				lErrorReason == ERROR_REASON_EXTENDED_CHECK_NOT_PASSED ||
				lErrorReason == ERROR_REASON_INTEGRITY_ERROR /*||
				lErrorReason == ERROR_REASON_TX_INTEGRITY_ERROR*/) {
				//
				gLog().write(Log::STORE, std::string("[reindex/warning]: fallback to ") + 
					strprintf("block = %s/%s#",
						lLastPrev.toHex(), chain_.toHex().substr(0, 10)));
				// rollback
				setLastBlock(lLastPrev);
			} else {
				//
				gLog().write(Log::STORE, std::string("[reindex/error]: consistency errors occured, fallback to last known ") + 
					strprintf("block = %s/%s#",
						lLastPrev.toHex(), chain_.toHex().substr(0, 10)));
				// rollback
				setLastBlock(lLastPrev);
			}
		} else {
			//
			gLog().write(Log::STORE, std::string("[reindex]: blocks processed for ") + 
				strprintf("%s#", chain_.toHex().substr(0, 10)));
			// point to the last block
			setLastBlock(from);
			// resync height
			gLog().write(Log::STORE, std::string("[reindex]: resyncing height for ") + 
				strprintf("%s#", chain_.toHex().substr(0, 10)));
			//
			if (!resyncHeight(to)) {
				// rollback
				setLastBlock(lLastBlock);
				resyncHeight(to);
			}
		}
	}

	// umark last reindex
	writeLastReindex(uint256(), uint256());
	//
	gLog().write(Log::STORE, std::string("[reindex]: reindex FINISHED for ") + 
		strprintf("%s#", chain_.toHex().substr(0, 10)));

	return lResult;
}

bool TransactionStore::enqueueBlock(const NetworkBlockHeader& block) {
	//
	//boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	uint256 lHash = const_cast<NetworkBlockHeader&>(block).blockHeader().hash();

	NetworkBlockHeader lHeader;
	if (blocksQueue_.read(lHash, lHeader)) {
		return false;
	}

	blocksQueue_.write(lHash, block);

	return true;
}

void TransactionStore::dequeueBlock(const uint256& block) {
	//
	//boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	blocksQueue_.remove(block);
}

bool TransactionStore::firstEnqueuedBlock(NetworkBlockHeader& block) {
	//
	//boost::unique_lock<boost::recursive_mutex> lLock(storageMutex_);
	db::DbContainer<uint256, NetworkBlockHeader>::Iterator lFirst = blocksQueue_.begin();
	if (lFirst.valid()) {
		lFirst.second(block);
		return true;
	}

	return false;	
}

bool TransactionStore::airdropped(const uint160& address, const uint160& peer) {
	//
	if (settings_->supportAirdrop()) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(airdropMutex_);
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
			// mark intention
			airDropAddressesTx_.write(address, uint256());

			//
			// NOTICE: in case of airdrops relay requests we do not able to check against ip addresses; we need more secure algo
			//
			/*
			db::DbTwoKeyContainer<
				uint160,
				uint160,
				uint256>::Iterator lFrom = airDropPeers_.find(peer);
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

				if (++lCount > 20) { lAirdropped = true; break; }
			}

			if (lAirdropped) {
				//
				gLog().write(Log::STORE, std::string("[airdropped]: already AIRDROPPED to ") + 
					strprintf("address_id = %s, peer_id = %s", address.toHex(), peer.toHex()));
				return true;
			}
			*/

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
