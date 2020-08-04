#include "wallet.h"
#include "../mkpath.h"

using namespace qbit;
using namespace qbit::tests;

bool WalletQbitCreateSpend::execute() {
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
	TransactionContextPtr lCtx = wallet_->createTxCoinBase(QBIT);
	// 2. check
	TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_, entityStore_) << 
		TxCoinBaseVerify::instance() 	<< 
		TxSpendVerify::instance() 		<< 
		TxSpendOutVerify::instance() 	<< TxAssetTypeVerify::instance() <<  // or
		TxBalanceVerify::instance();

	if (!lProcessor.process(lCtx)) {
		if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
		wallet_.reset();
		return false;
	}
	// 3. push
	store_->pushTransaction(lCtx);

	// 4. check
	if (wallet_->balance() != QBIT) {
		error_ = "Incorrect balance (0)";
		wallet_.reset();
		return false;
	}

	//
	// QBIT spend
	//

	// 0. create bob's key
	SKey lBobKey(seedBob_); // init bob key
	lBobKey.create();
	PKey lBob = lBobKey.createPKey();

	// 1. create
	lCtx = wallet_->createTxSpend(TxAssetType::qbitAsset(), lBob, 1000);
	// 2. check
	if (!lProcessor.process(lCtx)) {
		if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
		wallet_.reset();
		return false;
	}
	// 3. push
	store_->pushTransaction(lCtx);

	// 4. check
	if (wallet_->balance() != (QBIT-1000-lCtx->fee()*QUNIT*1)) {
		error_ = "Incorrect balance (1)";
		wallet_.reset();
		return false;
	}	

	wallet_.reset();
	return true;
}

bool WalletQbitCreateSpendRollback::execute() {
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
	TransactionContextPtr lCtx = wallet_->createTxCoinBase(QBIT);
	// 2. check
	TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_, entityStore_) << 
		TxCoinBaseVerify::instance() 	<< 
		TxSpendVerify::instance() 		<< 
		TxSpendOutVerify::instance() 	<< TxAssetTypeVerify::instance() <<  // or
		TxBalanceVerify::instance();
	if (!lProcessor.process(lCtx)) {
		if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
		return false;
	}
	// 3. push
	store_->pushTransaction(lCtx);

	// 4. check
	if (wallet_->balance() != QBIT) {
		error_ = "Incorrect balance (0)";
		wallet_.reset();
		return false;
	}


	//
	// QBIT spend
	//

	// 0. create bob's key
	SKey lBobKey(seedBob_); // init bob key
	lBobKey.create();
	PKey lBob = lBobKey.createPKey();

	// 1. create
	lCtx = wallet_->createTxSpend(TxAssetType::qbitAsset(), lBob, 1000);
	// 2. check
	if (!lProcessor.process(lCtx)) {
		if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
		wallet_.reset();
		return false;
	}
	// 3. push
	store_->pushTransaction(lCtx);

	// 4. check
	if (wallet_->balance() != (QBIT-1000-lCtx->fee()*QUNIT*1)) {
		error_ = "Incorrect balance (1)";
		wallet_.reset();
		return false;
	}

	// 5. rollback
	wallet_->rollback(lCtx);

	// 6. check
	if (wallet_->balance() != QBIT) {
		error_ = "Incorrect balance (2)";
		wallet_.reset();
		return false;
	}

	wallet_.reset();
	return true;
}

bool WalletAssetCreateAndSpend::execute() {
	//
	Transaction::registerTransactionType(Transaction::ASSET_TYPE, TxAssetTypeCreator::instance());
	Transaction::registerTransactionType(Transaction::ASSET_EMISSION, TxAssetEmissionCreator::instance());

	// clean up
	rmpath(settings_->dataPath().c_str());

	// prepare wallet
	if (!wallet_->open("")) {
		error_  = "Wallet open failed.";
		wallet_.reset();
		return false;
	}

	//
	// Create key
	//
	SKeyPtr lMyKey = wallet_->createKey(seedMy_);
	PKey lMyPubKey = lMyKey->createPKey();

	//
	// QBIT coinbase
	//

	// 1. create
	TransactionContextPtr lCtx = wallet_->createTxCoinBase(QBIT);
	// 2. check
	TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_, entityStore_) << 
		TxCoinBaseVerify::instance() 	<< 
		TxSpendVerify::instance() 		<< 
		TxSpendOutVerify::instance() 	<< TxAssetTypeVerify::instance() <<  // or
		TxBalanceVerify::instance();

	if (!lProcessor.process(lCtx)) {
		if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
		wallet_.reset();
		return false;
	}
	// 3. push
	store_->pushTransaction(lCtx);

	// 4. check
	if (wallet_->balance() != QBIT) {
		error_ = "Incorrect balance (QBIT)";
		wallet_.reset();
		return false;
	}

	//
	// TOKEN asset type create
	//

	// 1. create, total supply = supply * chunks * scale (chunks = number of unlinked outputs)
	lCtx = wallet_->createTxAssetType(lMyPubKey, "tkn", "Token", 100 /*supply chunk*/, 10 /*scale*/, 10 /*supply chunks*/, TxAssetType::LIMITED);
	// 2. check
	if (!lProcessor.process(lCtx)) {
		if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
		wallet_.reset();
		return false;
	}
	// 3. push
	store_->pushTransaction(lCtx);

	// 4. TOKEN id
	uint256 TOKEN = lCtx->tx()->hash();

	// 5. check
	if (wallet_->balance(TOKEN) != 0) {
		error_ = "Incorrect balance (TOKEN type)";
		wallet_.reset();
		return false;
	}

	//
	// TOKEN emission create
	//

	// 1. create
	lCtx = wallet_->createTxLimitedAssetEmission(lMyPubKey, TOKEN);
	// 2. check
	if (!lProcessor.process(lCtx)) {
		if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
		wallet_.reset();
		return false;
	}
	// 3. push
	store_->pushTransaction(lCtx);

	// 4. check
	if (wallet_->balance(TOKEN) != 100 /*chunk*/ * 10 /*scale*/) {
		error_ = "Incorrect balance (TOKEN emission)";
		wallet_.reset();
		return false;
	}

	//
	// TOKEN spend
	//

	// 0. create bob's key
	SKey lBobKey(seedBob_); // init bob key
	lBobKey.create();
	PKey lBob = lBobKey.createPKey();

	// 1. create
	lCtx = wallet_->createTxSpend(TOKEN, lBob, 10 * 10);
	// 2. check
	if (!lProcessor.process(lCtx)) {
		if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
		wallet_.reset();
		return false;
	}
	// 3. push
	store_->pushTransaction(lCtx);

	// 4. check
	if (wallet_->balance(TOKEN) != 90 /*chunk*/ * 10 /*scale*/) {
		error_ = "Incorrect balance (TOKEN spend)";
		wallet_.reset();
		return false;
	}

	amount_t lQBIT = wallet_->balance();
	amount_t lTOKEN = wallet_->balance(TOKEN);

	//
	// TOKEN rollback
	//

	// 1. create
	lCtx = wallet_->createTxSpend(TOKEN, lBob, 10 * 10);
	// 2. check
	if (!lProcessor.process(lCtx)) {
		if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
		wallet_.reset();
		return false;
	}
	// 3. push
	store_->pushTransaction(lCtx);

	// 4. rollback
	wallet_->rollback(lCtx);

	// 5. check - balance unchanged
	if (wallet_->balance(TOKEN) != lTOKEN) {
		error_ = "Incorrect balance (TOKEN spend)";
		wallet_.reset();
		return false;
	}

	// 6. check - balance unchanged
	if (wallet_->balance() != lQBIT) {
		error_ = "Incorrect balance (QBIT spend)";
		wallet_.reset();
		return false;
	}

	wallet_.reset();
	return true;
}
