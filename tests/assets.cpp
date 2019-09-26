#include "assets.h"

using namespace qbit;
using namespace qbit::tests;

//
//
uint256 TxAssetVerify::create_qbitTx(BlockPtr block) {
	// 0
	// make pair (pubkey, key)
	SKey lKey0 = wallet_->createKey(seedMy_);
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TransactionHelper::to<TxCoinBase>(TransactionFactory::create(Transaction::COINBASE));
	lTx->addIn();
	Transaction::UnlinkedOutPtr lUTXO = lTx->addOut(lKey0, lPKey0, TxAssetType::qbitAsset(), 10);
	// id() = hash()
	lUTXO->out().setTx(lTx->id());

	store_->pushTransaction(TransactionContext::instance(lTx));
	store_->pushUnlinkedOut(lUTXO, nullptr);

	block->append(lTx);

	wallet_->pushUnlinkedOut(lUTXO, nullptr);
	return lUTXO->hash(); 
}

TransactionPtr TxAssetVerify::create_assetTypeTx(uint256 utxo, BlockPtr block) {

	// mine
	SKey lKey0 = wallet_->createKey(seedMy_); // just re-init
	PKey lPKey0 = lKey0.createPKey();

	// 1
	// create transaction
	TxAssetTypePtr lAssetTypeTx = TransactionHelper::to<TxAssetType>(TransactionFactory::create(Transaction::ASSET_TYPE));

	lAssetTypeTx->setShortName("TOKEN");
	lAssetTypeTx->setLongName("Very first token");
	lAssetTypeTx->setSupply(100);
	lAssetTypeTx->setScale(1);
	lAssetTypeTx->setEmission(TxAssetType::LIMITED);

	// 2
	// add emission out
	Transaction::UnlinkedOutPtr lUTXO0 = lAssetTypeTx->addLimitedOut(lKey0, lPKey0, 100); // full

	// 3
	// find qbit UTXO for fee
	Transaction::UnlinkedOutPtr lUTXO = wallet_->findUnlinkedOut(utxo);
	lAssetTypeTx->addIn(lKey0, lUTXO);

	// 4
	// add qbit change
	Transaction::UnlinkedOutPtr lUTXO1 = lAssetTypeTx->addOut(lKey0, lPKey0 /*change*/, TxAssetType::qbitAsset(), 9);

	// 5
	// add qbit fee
	Transaction::UnlinkedOutPtr lUTXO2 = lAssetTypeTx->addFeeOut(lKey0, TxAssetType::qbitAsset(), 1); // to miner

	// 6 finalize&check
	if (!lAssetTypeTx->finalize(lKey0)) {
		error_ = "AssetType finalization failed";
		return nullptr;
	}

	store_->pushTransaction(TransactionContext::instance(lAssetTypeTx));

	store_->pushUnlinkedOut(lUTXO1, nullptr); // add utxo
	store_->pushUnlinkedOut(lUTXO2, nullptr); // add utxo

	block->append(lAssetTypeTx);

	return lAssetTypeTx;
}

bool TxAssetVerify::execute() {

	//
	//
	Transaction::registerTransactionType(Transaction::ASSET_TYPE, TxAssetTypeCreator::instance());

	BlockPtr lBlock = Block::instance();

	//
	// create & check
	uint256 utxo = create_qbitTx(lBlock);
	TransactionPtr lTx1 = create_assetTypeTx(utxo, lBlock);

	if (lTx1 != nullptr) {
		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_, entityStore_) << 
			TxCoinBaseVerify::instance() 	<< 
			TxSpendVerify::instance() 		<< 
			TxSpendOutVerify::instance() 	<< TxAssetTypeVerify::instance() <<  // or
			TxBalanceVerify::instance();
		
		TransactionContextPtr lCtx = TransactionContext::instance(lTx1);
		if (!lProcessor.process(lCtx)) {
			if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
			return false;
		}

		if (lCtx->fee() != 1) {
			error_ = "Fee is incorrect";
			return false;
		}
	} else { return false; }

	if (wallet_->balance() != 9) {
		error_ = "Incorrect wallet balance";
		return false;		
	}

	
	//
	// serialize
	DataStream lStream(SER_NETWORK, 0);
	Block::Serializer::serialize<DataStream>(lStream, lBlock);	

	BlockPtr lBlock2 = Block::Deserializer::deserialize<DataStream>(lStream);

	// recreate store
	store_ = std::make_shared<TxStoreA>(); 
	wallet_ = std::make_shared<TxWalletA>();
	entityStore_ = std::make_shared<EntityStoreA>();

	store_->pushBlock(lBlock2); // extract block for verification

	bool lFound = false;
	for(TransactionsContainer::iterator lIter = lBlock2->transactions().begin(); lIter != lBlock2->transactions().end(); lIter++) {

		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_, entityStore_) << 
			TxCoinBaseVerify::instance() 	<< 
			TxSpendVerify::instance() 		<< 
			TxSpendOutVerify::instance() 	<< TxAssetTypeVerify::instance() <<  // or
			TxBalanceVerify::instance();
		
		//std::cout << std::endl << (*lIter)->toString() << std::endl;

		TransactionContextPtr lCtx = TransactionContext::instance(*lIter);
		if (!lProcessor.process(lCtx)) {
			if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
			return false;
		}

		if ((*lIter)->id() == lTx1->id()) lFound = true;

		if (lFound && lCtx->fee() != 1) {
			error_ = "Fee is incorrect";
			return false;
		}
	}

	if (!lFound) {
		error_ = "Tx was not found";
		return false;		
	}

	// wallet_ was RECREATED, so keys was lost - balance MUST be zero
	if (wallet_->balance() != 0) {
		error_ = "Incorrect wallet balance";
		return false;	
	}

	if (entityStore_->locateEntity(lTx1->id()) == nullptr) {
		error_ = "Entity not found";
		return false;	
	}


	//
	//
	// recreate store
	store_ = std::make_shared<TxStoreA>(); 
	wallet_ = std::make_shared<TxWalletA>();
	entityStore_ = std::make_shared<EntityStoreA>();

	// fill address
	// receiver address
	wallet_->createKey(seedMy_);

	// re-process block
	store_->pushBlock(lBlock2); // extract block for verification

	lFound = false;
	for(TransactionsContainer::iterator lIter = lBlock2->transactions().begin(); lIter != lBlock2->transactions().end(); lIter++) {

		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_, entityStore_) << 
			TxCoinBaseVerify::instance() 	<< 
			TxSpendVerify::instance() 		<< 
			TxSpendOutVerify::instance() 	<< TxAssetTypeVerify::instance() <<  // or
			TxBalanceVerify::instance();
		
		//std::cout << std::endl << (*lIter)->toString() << std::endl;

		TransactionContextPtr lCtx = TransactionContext::instance(*lIter);
		if (!lProcessor.process(lCtx)) {
			if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
			return false;
		}

		if ((*lIter)->id() == lTx1->id()) lFound = true;

		if (lFound && lCtx->fee() != 1) {
			error_ = "Fee is incorrect";
			return false;
		}
	}

	if (!lFound) {
		error_ = "Tx was not found";
		return false;		
	}

	if (wallet_->balance() != 9) {
		error_ = "Incorrect wallet balance";
		return false;	
	}

	if (wallet_->balance(lTx1->id()) != 0) {
		error_ = "Incorrect wallet balance by new asset";
		return false;	
	}

	if (entityStore_->locateEntity(lTx1->id()) == nullptr) {
		error_ = "Entity not found";
		return false;	
	}

	return true;
}

//
//
//
uint256 TxAssetEmissionVerify::create_qbitTx(BlockPtr block) {
	// 0
	// make pair (pubkey, key)
	SKey lKey0 = wallet_->createKey(seedMy_);
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TransactionHelper::to<TxCoinBase>(TransactionFactory::create(Transaction::COINBASE));
	lTx->addIn();
	Transaction::UnlinkedOutPtr lUTXO = lTx->addOut(lKey0, lPKey0, TxAssetType::qbitAsset(), 10);
	// id() = hash()
	lUTXO->out().setTx(lTx->id());

	store_->pushTransaction(TransactionContext::instance(lTx));
	store_->pushUnlinkedOut(lUTXO, nullptr);

	block->append(lTx);

	wallet_->pushUnlinkedOut(lUTXO, nullptr);
	return lUTXO->hash();
}

TransactionPtr TxAssetEmissionVerify::create_assetTypeTx(uint256 utxo, BlockPtr block, uint256& limitedOut, uint256& feeOut) {

	// mine
	SKey lKey0 = wallet_->createKey(seedMy_); // just re-init
	PKey lPKey0 = lKey0.createPKey();

	// 1
	// create transaction
	TxAssetTypePtr lAssetTypeTx = TransactionHelper::to<TxAssetType>(TransactionFactory::create(Transaction::ASSET_TYPE));

	lAssetTypeTx->setShortName("TOKEN");
	lAssetTypeTx->setLongName("Very first token");
	lAssetTypeTx->setSupply(100);
	lAssetTypeTx->setScale(1);
	lAssetTypeTx->setEmission(TxAssetType::LIMITED);

	// 2
	// add emission out
	Transaction::UnlinkedOutPtr lUTXO0 = lAssetTypeTx->addLimitedOut(lKey0, lPKey0, 100); // full

	// 3
	// find qbit UTXO for fee
	Transaction::UnlinkedOutPtr lUTXO = wallet_->findUnlinkedOut(utxo);
	lAssetTypeTx->addIn(lKey0, lUTXO);

	// 4
	// add qbit change
	Transaction::UnlinkedOutPtr lUTXO1 = lAssetTypeTx->addOut(lKey0, lPKey0 /*change*/, TxAssetType::qbitAsset(), 9);

	// 5
	// add qbit fee
	Transaction::UnlinkedOutPtr lUTXO2 = lAssetTypeTx->addFeeOut(lKey0, TxAssetType::qbitAsset(), 1); // to miner

	// 6 finalize&check
	if (!lAssetTypeTx->finalize(lKey0)) {
		error_ = "AssetType finalization failed";
		return nullptr;
	}

	limitedOut = lUTXO0->hash();
	feeOut = lUTXO1->hash();

	store_->pushTransaction(TransactionContext::instance(lAssetTypeTx));

	wallet_->pushUnlinkedOut(lUTXO0, nullptr); // add utxo
	wallet_->pushUnlinkedOut(lUTXO1, nullptr); // add utxo

	store_->pushUnlinkedOut(lUTXO0, nullptr); // add utxo
	store_->pushUnlinkedOut(lUTXO1, nullptr); // add utxo
	store_->pushUnlinkedOut(lUTXO2, nullptr); // add utxo

	block->append(lAssetTypeTx);

	return lAssetTypeTx;
}

TransactionPtr TxAssetEmissionVerify::create_assetEmissionTx(uint256 qbit_utxo, uint256 assetType_utxo, BlockPtr block) {

	// mine
	SKey lKey0 = wallet_->createKey(seedMy_); // just re-init
	PKey lPKey0 = lKey0.createPKey();

	// 1
	// create transaction
	TxAssetEmissionPtr lAssetEmissionTx = TransactionHelper::to<TxAssetEmission>(TransactionFactory::create(Transaction::ASSET_EMISSION));
	
	Transaction::UnlinkedOutPtr lAssetTypeUTXO = wallet_->findUnlinkedOut(assetType_utxo);
	if (!lAssetTypeUTXO) {
		error_ = "AssetType utxo not found";
		return nullptr;
	}

	Transaction::UnlinkedOutPtr lQbitUTXO = wallet_->findUnlinkedOut(qbit_utxo);	
	
	lAssetEmissionTx->addIn(lKey0, lQbitUTXO); // fee-in
	lAssetEmissionTx->addLimitedIn(lKey0, lAssetTypeUTXO); // asset-type

	// 2
	// add emission
	Transaction::UnlinkedOutPtr lUTXO0 = lAssetEmissionTx->addOut(lKey0, lPKey0, lAssetTypeUTXO->out().asset(), 100); // full

	// 4
	// add qbit change
	Transaction::UnlinkedOutPtr lUTXO1 = lAssetEmissionTx->addOut(lKey0, lPKey0 /*change*/, TxAssetType::qbitAsset(), 8);

	// 5
	// add qbit fee
	Transaction::UnlinkedOutPtr lUTXO2 = lAssetEmissionTx->addFeeOut(lKey0, TxAssetType::qbitAsset(), 1); // to miner

	// 6 finalize&check
	if (!lAssetEmissionTx->finalize(lKey0)) {
		error_ = "AssetEmission finalization failed";
		return nullptr;
	}

	store_->pushTransaction(TransactionContext::instance(lAssetEmissionTx));

	wallet_->pushUnlinkedOut(lUTXO0, nullptr); // add utxo
	wallet_->pushUnlinkedOut(lUTXO1, nullptr); // add utxo

	store_->pushUnlinkedOut(lUTXO0, nullptr); // add utxo
	store_->pushUnlinkedOut(lUTXO1, nullptr); // add utxo
	store_->pushUnlinkedOut(lUTXO2, nullptr); // add utxo

	block->append(lAssetEmissionTx);

	return lAssetEmissionTx;
}

bool TxAssetEmissionVerify::checkBlock(BlockPtr lBlock) {

	for(TransactionsContainer::iterator lIter = lBlock->transactions().begin(); lIter != lBlock->transactions().end(); lIter++) {
		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_, entityStore_) << 
			TxCoinBaseVerify::instance() 	<< 
			TxSpendVerify::instance() 		<< 
			TxSpendOutVerify::instance() 	<< TxAssetTypeVerify::instance() <<  // or
			TxBalanceVerify::instance();
		
		//std::cout << std::endl << (*lIter)->toString() << std::endl;

		TransactionContextPtr lCtx = TransactionContext::instance(*lIter);
		if (!lProcessor.process(lCtx)) {
			if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
			return false;
		}

		if (lCtx->fee() != 1 && (lCtx->tx()->type() == Transaction::SPEND || 
				lCtx->tx()->type() == Transaction::ASSET_TYPE || lCtx->tx()->type() == Transaction::ASSET_EMISSION)) {
			error_ = "Fee is incorrect";
			return false;
		}
	}

	return true;
}

bool TxAssetEmissionVerify::execute() {

	//
	//
	Transaction::registerTransactionType(Transaction::ASSET_EMISSION, TxAssetEmissionCreator::instance());

	BlockPtr lBlock = Block::instance();

	//
	// create & check
	uint256 limitedOut, feeOut;
	uint256 utxo = create_qbitTx(lBlock);
	TransactionPtr lTx1 = create_assetTypeTx(utxo, lBlock, limitedOut, feeOut);
	TransactionPtr lTx2 = create_assetEmissionTx(feeOut, limitedOut, lBlock);

	if (!checkBlock(lBlock)) return false;

	if (wallet_->balance() != 8) {
		error_ = "Incorrect wallet balance";
		return false;		
	}

	//
	// re-create and re-check

	DataStream lStream(SER_NETWORK, CLIENT_VERSION);
	Block::Serializer::serialize<DataStream>(lStream, lBlock);	

	BlockPtr lBlock2 = Block::Deserializer::deserialize<DataStream>(lStream);

	store_ = std::make_shared<TxStoreA>(); 
	wallet_ = std::make_shared<TxWalletA>(); 
	entityStore_ = std::make_shared<EntityStoreA>();

	wallet_->createKey(seedMy_); // init my key

	store_->pushBlock(lBlock2); // extract block for verification

	if (!checkBlock(lBlock2)) return false;	

	if (wallet_->balance() != 8) {
		error_ = "Incorrect wallet balance";
		return false;		
	}

	if (wallet_->balance(lTx1->id()) != 100) { // new asset type
		error_ = "Incorrect wallet balance by new asset";
		return false;	
	}


	return true;
}

//
//
//
bool TxAssetEmissionSpend::execute() {

	//
	//
	BlockPtr lBlock = Block::instance();

	//
	// create & check
	uint256 limitedOut, feeOut;
	uint256 utxo = create_qbitTx(lBlock);
	TransactionPtr lTx1 = create_assetTypeTx(utxo, lBlock, limitedOut, feeOut);
	TransactionPtr lTx2 = create_assetEmissionTx(feeOut, limitedOut, lBlock);

	if (!checkBlock(lBlock)) return false;

	//
	// re-create and re-check

	DataStream lStream(SER_NETWORK, CLIENT_VERSION);
	Block::Serializer::serialize<DataStream>(lStream, lBlock);	

	BlockPtr lBlock2 = Block::Deserializer::deserialize<DataStream>(lStream);

	store_ = std::make_shared<TxStoreA>(); 
	wallet_ = std::make_shared<TxWalletA>(); 
	entityStore_ = std::make_shared<EntityStoreA>();
	
	SKey lKey0 = wallet_->createKey(seedMy_); // init my key
	PKey lPKey0 = lKey0.createPKey();
	store_->pushBlock(lBlock2); // extract block for verification

	SKey lKey1(seedBob_); // init bob key
	lKey1.create();
	PKey lPKey1 = lKey1.createPKey();

	if (!checkBlock(lBlock2)) return false;	

	// add spend tx
	TxSpendPtr lTxSpend = TransactionHelper::to<TxSpend>(TransactionFactory::create(Transaction::SPEND));
	Transaction::UnlinkedOutPtr lUTXO = wallet_->findUnlinkedOutByAsset(lTx1->id(), 100);
	Transaction::UnlinkedOutPtr lFee = wallet_->findUnlinkedOutByAsset(TxAssetType::qbitAsset(), 1);

	if (lUTXO == nullptr) {
		error_ = "UTXO not found";
		return false;
	}

	lTxSpend->addIn(lKey0, lUTXO);
	lTxSpend->addIn(lKey0, lFee);

	lTxSpend->addOut(lKey0, lPKey1 /*to receiver*/, lTx1->id(), 100);
	lTxSpend->addOut(lKey0, lPKey0 /*change*/, TxAssetType::qbitAsset(), lFee->amount() - 1);
	lTxSpend->addFeeOut(lKey0, TxAssetType::qbitAsset(), 1); // to miner

	lTxSpend->finalize(lKey0);

	lBlock2->append(lTxSpend);

	if (!checkBlock(lBlock2)) return false;	

	if (wallet_->balance() != 7) {
		error_ = "Incorrect wallet balance";
		return false;		
	}

	if (wallet_->balance(lTx1->id()) != 0) { // new asset type
		error_ = "Incorrect wallet balance asset";
		return false;	
	}

	return true;
}
