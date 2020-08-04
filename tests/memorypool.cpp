#include "memorypool.h"
#include "../mkpath.h"

using namespace qbit;
using namespace qbit::tests;

bool MemoryPoolQbitCreateSpend::execute() {
	//
	// 1. create qbit
	//

	// clean up
	rmpath(settings_->dataPath().c_str());

	// prepare wallet
	if (!wallet_->open("")) {
		error_  = "Wallet open failed.";
		return false;
	}

	//
	// Create key
	//
	wallet_->createKey(seedMy_);

	//
	// QBIT coinbase
	//

	// 1. create
	TransactionContextPtr lCtx = wallet_->createTxCoinBase(10000);
	// 2. check
	TransactionProcessor lProcessor = TransactionProcessor(persistentStore_, wallet_, entityStore_) << 
		TxCoinBaseVerify::instance() 	<< 
		TxSpendVerify::instance() 		<< 
		TxSpendOutVerify::instance() 	<< TxAssetTypeVerify::instance() <<  // or
		TxBalanceVerify::instance();

	if (!lProcessor.process(lCtx)) {
		if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
		mempool_->close();
		wallet_->close();
		return false;
	}
	// 3. push to persistent store
	persistentStore_->pushTransaction(lCtx);


	//
	// MEMORY POOL
	//

	// 1. Create 3
	// 1.1. create bob's key
	SKey lBobKey(seedBob_); // init bob key
	lBobKey.create();
	PKey lBob = lBobKey.createPKey();

	SKeyPtr lMyKey = wallet_->firstKey();
	PKey lMy = lMyKey->createPKey();

	// 1.2 create tx
	lCtx = wallet_->createTxSpend(TxAssetType::qbitAsset(), lBob, 1000);
	lCtx = mempool_->pushTransaction(lCtx->tx()); // re-create context
	if (lCtx->errors().size()) {
		error_ = *lCtx->errors().begin();
		mempool_->close();
		wallet_->close();
		return false;
	}

	// 1.2.1 send to self
	lCtx = wallet_->createTxSpend(TxAssetType::qbitAsset(), lMy, 8000);
	lCtx = mempool_->pushTransaction(lCtx->tx()); // re-create context
	if (lCtx->errors().size()) {
		error_ = *lCtx->errors().begin();
		mempool_->close();
		wallet_->close();
		return false;
	}

	// 1.3 create tx
	lCtx = wallet_->createTxSpend(TxAssetType::qbitAsset(), lBob, 1100);
	lCtx = mempool_->pushTransaction(lCtx->tx()); // re-create context
	if (lCtx->errors().size()) {
		error_ = *lCtx->errors().begin();
		mempool_->close();
		wallet_->close();
		return false;
	}

	// 1.4 create tx
	lCtx = wallet_->createTxSpend(TxAssetType::qbitAsset(), lBob, 1200);
	lCtx = mempool_->pushTransaction(lCtx->tx()); // re-create context
	if (lCtx->errors().size()) {
		error_ = *lCtx->errors().begin();
		mempool_->close();
		wallet_->close();
		return false;
	}

	// 2. compose block
	BlockPtr lBlock = Block::instance();
	BlockContextPtr lBlockCtx = mempool_->beginBlock(lBlock);
	mempool_->commit(lBlockCtx);

	//std::cout << "\n" << lBlock->toString() << "\n";

	if (lBlock->transactions().size() != 4) {
		error_ = "Tx count is invalid";
		mempool_->close();
		wallet_->close();
		return false;
	}

	mempool_->close();
	wallet_->close();
	return true;
}
