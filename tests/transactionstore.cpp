#include "transactionstore.h"
#include "../mkpath.h"

using namespace qbit;
using namespace qbit::tests;

bool StoreQbitCreateSpend::execute() {
	// clean up
	rmpath(settings_->dataPath().c_str());

	// prepare wallet
	if (!wallet_->open()) {
		error_  = "Wallet open failed.";
		return false;
	}

	// prepare store
	if (!store_->open()) {
		error_  = "Storage open failed.";
		return false;
	}

	// create bob's key
	SKey lBobKey(seedBob_); // init bob key
	lBobKey.create();
	PKey lBob = lBobKey.createPKey();

	//
	// Create key
	//
	wallet_->createKey(seedMy_);

	//
	// BLOCK(0)
	//
	{
		//
		// QBIT coinbase
		//

		// 1. create
		TransactionContextPtr lCtx = wallet_->createTxCoinBase(QBIT);

		// 2. create block
		BlockPtr lBlock = Block::instance();
		lBlock->append(lCtx->tx());

		// 3. process block
		BlockContextPtr lBlockCtx = mempool_->beginBlock(lBlock);
		mempool_->commit(lBlockCtx);
		size_t lHeight;
		store_->commitBlock(lBlockCtx, lHeight);

		if (lBlockCtx->errors().size()) {
			error_ = *(lBlockCtx->errors().begin()->second.begin());
			wallet_->close();
			store_->close();
			mempool_->close();
			return false;
		}
	}

	//
	// BLOCK(1)
	//
	{
		//
		// QBIT spend
		//

		// 1. create block
		BlockPtr lBlock = Block::instance();

		//std::stringstream s0;
		//wallet_->dumpUnlinkedOutByAsset(TxAssetType::qbitAsset(), s0);
		//std::cout << s0.str() << "\n";

		// 2. create tx
		for (int i = 0; i < 1; i++) {
			TransactionContextPtr lCtx = wallet_->createTxSpend(TxAssetType::qbitAsset(), lBob, 100);
			lCtx = mempool_->pushTransaction(lCtx->tx());
			
			if (lCtx->errors().size()) {
				error_ = *lCtx->errors().begin();
				wallet_->close();
				store_->close();
				mempool_->close();
				return false;
			}

			lBlock->append(lCtx->tx());
		}

		// 3. process block
		BlockContextPtr lBlockCtx = mempool_->beginBlock(lBlock);
		mempool_->commit(lBlockCtx);
		size_t lHeight;
		store_->commitBlock(lBlockCtx, lHeight);

		if (lBlockCtx->errors().size()) {
			error_ = *(lBlockCtx->errors().begin()->second.begin());
			wallet_->close();
			store_->close();
			mempool_->close();
			return false;
		}		
	}

	wallet_->close();
	store_->close();
	mempool_->close();

	return true;
}
