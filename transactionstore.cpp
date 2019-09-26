#include "transactionstore.h"
#include "transactionvalidator.h"
#include "log/log.h"
#include "mkpath.h"

#include <iostream>

using namespace qbit;

TransactionPtr TransactionStore::locateTransaction(const uint256& tx) {
	//
	TxBlockIndex lIdx;
	if (transactionsIdx_.read(tx, lIdx)) {

		BlockTransactionsPtr lTransactions = transactions_.read(lIdx.block());
		if (lTransactions) {
			if (lIdx.index() < lTransactions->transactions().size())
				return lTransactions->transactions()[lIdx.index()];
		}
	}

	return nullptr;
}

TransactionContextPtr TransactionStore::locateTransactionContext(const uint256& tx) {
	TransactionPtr lTx = locateTransaction(tx);
	if (lTx) return TransactionContext::instance(lTx);

	return nullptr;
}

bool TransactionStore::processBlock(BlockContextPtr ctx, bool processWallet) {
	
	//
	// make context block store and wallet store
	ITransactionStorePtr lTransactionStore = TxBlockStore::instance(std::static_pointer_cast<ITransactionStore>(shared_from_this()));
	IWalletPtr lWallet = TxWalletStore::instance(wallet_, std::static_pointer_cast<ITransactionStore>(shared_from_this()));
	BlockContextPtr lBlockCtx = ctx;

	//
	// process block under local context block&wallet
	bool lHasErrors = false;
	TransactionProcessor lProcessor = TransactionProcessor::general(lTransactionStore, lWallet, std::static_pointer_cast<IEntityStore>(shared_from_this()));
	for(TransactionsContainer::iterator lTx = ctx->block()->transactions().begin(); lTx != ctx->block()->transactions().end(); lTx++) {
		TransactionContextPtr lCtx = TransactionContext::instance(*lTx);
		if (!lProcessor.process(lCtx)) {
			lHasErrors = true;
			for (std::list<std::string>::iterator lErr = lCtx->errors().begin(); lErr != lCtx->errors().end(); lErr++) {
				gLog().write(Log::STORE, std::string("[TransactionStore::pushBlock/error]: ") + (*lErr));
			}

			lBlockCtx->addErrors(lCtx->errors());
		} else {
			lTransactionStore->pushTransaction(lCtx);
		}
	}

	//
	// check and commit
	if (!lHasErrors) {
		//
		// commit transaction store changes
		TxBlockStorePtr lBlockStore = std::static_pointer_cast<TxBlockStore>(lTransactionStore);
		for (std::list<TxBlockAction>::iterator lAction = lBlockStore->actions().begin(); lAction != lBlockStore->actions().end(); lAction++) {
			if (lAction->action() == TxBlockAction::PUSH) {
				pushUnlinkedOut(lAction->utxoPtr(), nullptr);
			} else if (lAction->action() == TxBlockAction::POP) {
				if (!popUnlinkedOut(lAction->utxo(), nullptr)) {
					gLog().write(Log::STORE, std::string("[TransactionStore::pushBlock/error]: Block with utxo inconsistency - ") + lAction->utxo().toHex());

					lBlockCtx->addError("Block with utxo inconsistency - " + lAction->utxo().toHex());
					return false;
				}
			}
		}

		//
		// commit local wallet changes
		if (processWallet) {
			TxWalletStorePtr lWalletStore = std::static_pointer_cast<TxWalletStore>(lWallet);
			for (std::list<TxBlockAction>::iterator lAction = lWalletStore->actions().begin(); lAction != lWalletStore->actions().end(); lAction++) {
				if (lAction->action() == TxBlockAction::PUSH) {
					if (!wallet_->pushUnlinkedOut(lAction->utxoPtr(), lAction->ctx())) {
						gLog().write(Log::STORE, std::string("TransactionStore::pushBlock/wallet/error]: Block with utxo inconsistency - ") + lAction->utxo().toHex());

						lBlockCtx->addError("Block/pushWallet with utxo inconsistency - " + lAction->utxo().toHex());
						return false;					
					}
				} else if (lAction->action() == TxBlockAction::POP) {
					if (!wallet_->popUnlinkedOut(lAction->utxo(), lAction->ctx())) {
						gLog().write(Log::STORE, std::string("TransactionStore::pushBlock/wallet/error]: Block with utxo inconsistency - ") + lAction->utxo().toHex());

						lBlockCtx->addError("Block/popWallet with utxo inconsistency - " + lAction->utxo().toHex());
						return false;
					}
				}
			}
		}

		//
		// write block data
		uint256 lHash = ctx->block()->hash();
		BlockHeader lHeader = ctx->block()->blockHeader();
		headers_.write(lHash, lHeader);
		transactions_.write(lHash, ctx->block()->blockTransactions());

		//
		// indexes
		uint32_t lIndex = 0;
		for(TransactionsContainer::iterator lTx = ctx->block()->transactions().begin(); lTx != ctx->block()->transactions().end(); lTx++) {
			transactionsIdx_.write((*lTx)->id(), TxBlockIndex(lHash, lIndex++));
		}

		//
		// block time index
		timeIdx_.write(ctx->block()->time(), lHash);
	}

	return true;	
}

bool TransactionStore::commitBlock(BlockContextPtr ctx) {
	//
	return processBlock(ctx, false);
}

BlockContextPtr TransactionStore::pushBlock(BlockPtr block) {
	//
	BlockContextPtr lBlockCtx = BlockContext::instance(block); 
	processBlock(lBlockCtx, true);

	return lBlockCtx;
}

bool TransactionStore::open() {
	try {
		if (mkpath(std::string(settings_->dataPath() + "/" + chain_.toHex() + "/indexes").c_str(), 0777)) return false;

		headers_.open();
		transactions_.open();
		utxo_.open();
		ltxo_.open();
		entities_.open();
		blockUtxoIdx_.open();
		transactionsIdx_.open();
		addressAssetUtxoIdx_.open();
		entities_.open();
		timeIdx_.open();

		for (db::DbContainer<uint64_t /*time*/, uint256 /*block*/>::Iterator lTime = timeIdx_.begin(); lTime.valid(); ++lTime) {
			height_.push_back(*lTime);
		}

		opened_ = true;
	}
	catch(const std::exception& ex) {
		gLog().write(Log::ERROR, std::string("[TransactionStore::open]: ") + ex.what());
		return false;
	}

	return true;
}

bool TransactionStore::close() {
	settings_.reset();
	consensus_.reset();
	wallet_.reset();

	return true;
}

bool TransactionStore::pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr) {
	//
	uint256 lUtxo = utxo->hash();
	if (!isUnlinkedOutUsed(lUtxo) && !findUnlinkedOut(lUtxo)) {
		// update utxo db
		utxo_.write(lUtxo, *utxo);
		// update indexes
		addressAssetUtxoIdx_.write(utxo->address().id(), utxo->out().asset(), lUtxo, utxo->out().tx());

		return true;
	}

	return false;
}

bool TransactionStore::popUnlinkedOut(const uint256& utxo, TransactionContextPtr) {
	//
	Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(utxo); 
	if (!isUnlinkedOutUsed(utxo) && lUtxo) {
		// update utxo db
		utxo_.remove(utxo);
		// update ltxo db
		ltxo_.write(utxo, *lUtxo);
		// cleanup index
		addressAssetUtxoIdx_.remove(lUtxo->address().id(), lUtxo->out().asset(), utxo);

		return true;
	}

	return true;
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
