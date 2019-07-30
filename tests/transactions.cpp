#include "transactions.h"

using namespace qbit;
using namespace qbit::tests;

uint256 TxVerify::createTx0() {
	// 0
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));

	SKey lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TxCoinBase::as(TransactionFactory::create(Transaction::COINBASE));
	lTx->addIn();
	unsigned char* asset0 = (unsigned char*)"01234567890123456789012345678901";
	Transaction::UnlinkedOut lUTXO = lTx->addOut(lKey0, lPKey0, uint256(asset0), 10);
	lUTXO.out().setTx(lTx->id());

	//std::cout << std::endl << lTx->toString() << std::endl;

	store_->pushTransaction(lTx);
	store_->pushUnlinkedOut(lUTXO);
	return wallet_->pushUnlinkedOut(lUTXO);
}

TransactionPtr TxVerify::createTx1(uint256 utxo) {
	// 0
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));

	SKey lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxSpendPtr lTx = TxSpend::as(TransactionFactory::create(Transaction::SPEND));

	Transaction::UnlinkedOut lUTXO;
	wallet_->findUnlinkedOut(utxo, lUTXO);
	lTx->addIn(lKey0, lUTXO);

	unsigned char* asset0 = (unsigned char*)"01234567890123456789012345678901";
	Transaction::UnlinkedOut lUTXO1 = lTx->addOut(lKey0, lPKey0, uint256(asset0), 10);
	lUTXO1.out().setTx(lTx->id());

	lTx->finalize(lKey0); // bool

	store_->pushTransaction(lTx);
	store_->pushUnlinkedOut(lUTXO);
	wallet_->pushUnlinkedOut(lUTXO);

	return lTx;
}

bool TxVerify::execute() {

	//
	// create & check
	uint256 utxo = createTx0();
	TransactionPtr lTx1 = createTx1(utxo);

	//std::cout << std::endl << lTx1->toString() << std::endl;

	if (lTx1 != nullptr) {
		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_) << 
			TxCoinBaseVerifyPush::instance() << TxSpendVerify::instance() << 
			TxSpendOutVerify::instance() << TxSpendBalanceVerify::instance();
		
		TransactionContextPtr lCtx = TransactionContext::instance(lTx1);
		if (!lProcessor.process(lCtx)) {
			if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
			return false;
		}

		//
		// serialize
		DataStream lStream(SER_NETWORK, 0);
		Transaction::Serializer::serialize<DataStream>(lStream, lTx1);

		//
		// deserialize&check
		TransactionPtr lTxDeserialized = Transaction::Deserializer::deserialize<DataStream>(lStream);

		TransactionContextPtr lCtx1 = TransactionContext::instance(lTxDeserialized);
		if (!lProcessor.process(lCtx1)) {
			if ((*(lCtx1->errors().begin())).find("INVALID_UTXO") == std::string::npos) {
				error_ = *(lCtx1->errors().begin());
				return false;
			} else {
				return true; // expected - UTXO is already linked
			}
		} else {
			return false;
		}
	} else {
		return false;
	}

	// log
	//std::cout << "\ntx: " << lStream.size() << "/" << HexStr(lStream.begin(), lStream.end()) << std::endl;

	return true;
}

//
//
//
uint256 TxVerifyPrivate::createTx0() {
	// 0
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));

	SKey lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TxCoinBase::as(TransactionFactory::create(Transaction::COINBASE));
	lTx->addIn();
	unsigned char asset00[] = { "0123456789012345678901234567890123654208352765429837659287635928673592863759286" };
	uint256 asset0 = Hash(asset00, asset00 + sizeof(asset00));
	Transaction::UnlinkedOut lUTXO = lTx->addOut(lKey0, lPKey0, asset0, 10);
	lUTXO.out().setTx(lTx->id());

	//std::cout << std::endl << lTx->toString() << std::endl;

	store_->pushTransaction(lTx);
	store_->pushUnlinkedOut(lUTXO);
	return wallet_->pushUnlinkedOut(lUTXO);
}

TransactionPtr TxVerifyPrivate::createTx1(uint256 utxo) {
	// 0
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));

	SKey lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxSpendPtr lTx = TxSpend::as(TransactionFactory::create(Transaction::SPEND_PRIVATE));

	Transaction::UnlinkedOut lUTXO;
	wallet_->findUnlinkedOut(utxo, lUTXO);
	lTx->addIn(lKey0, lUTXO);

	unsigned char asset00[] = { "0123456789012345678901234567890123654208352765429837659287635928673592863759286" };
	uint256 asset0 = Hash(asset00, asset00 + sizeof(asset00));
	//std::cout << "\n" << asset0.toHex() << "\n";
	Transaction::UnlinkedOut lUTXO1 = lTx->addOut(lKey0, lPKey0, asset0, 10);
	lUTXO1.out().setTx(lTx->id());

	if (!lTx->finalize(lKey0)) { // bool
		error_ = "Tx finalization failed";
		return nullptr;
	}

	store_->pushTransaction(lTx);
	store_->pushUnlinkedOut(lUTXO);
	wallet_->pushUnlinkedOut(lUTXO);

	//std::cout << std::endl << lTx->toString() << std::endl;

	return lTx;
}

bool TxVerifyPrivate::execute() {

	//
	// create & check
	uint256 utxo = createTx0();
	TransactionPtr lTx1 = createTx1(utxo);

	//std::cout << std::endl << lTx1->toString() << std::endl;

	if (lTx1 != nullptr) {
		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_) << 
			TxCoinBaseVerifyPush::instance() << TxSpendVerify::instance() << 
			TxSpendOutVerify::instance() << TxSpendBalanceVerify::instance();
		
		uint32_t lBegin = now();
		TransactionContextPtr lCtx = TransactionContext::instance(lTx1);
		if (!lProcessor.process(lCtx)) {
			if (lCtx->errors().size()) error_ = *lCtx->errors().begin();
			return false;
		}
		std::cout << "Verify - " << (now() - lBegin) << "mc / ";

		//
		// serialize
		DataStream lStream(SER_NETWORK, 0);
		Transaction::Serializer::serialize<DataStream>(lStream, lTx1);
		//std::cout << "\ntx: " << lStream.size() << "/" << HexStr(lStream.begin(), lStream.end()) << std::endl;

		//
		// deserialize&check
		TransactionPtr lTxDeserialized = Transaction::Deserializer::deserialize<DataStream>(lStream);

		if (!lTxDeserialized) { error_ = "Deserialization failed"; return false; }
		//std::cout << std::endl << lTxDeserialized->toString() << std::endl;

		TransactionContextPtr lCtx1 = TransactionContext::instance(lTxDeserialized);
		if (!lProcessor.process(lCtx1)) {
			if ((*(lCtx1->errors().begin())).find("INVALID_UTXO") == std::string::npos) {
				error_ = *(lCtx1->errors().begin());
				return false;
			} else {
				return true; // expected - UTXO is already linked
			}
		} else {
			return false;
		}
	} else {
		return false;
	}

	return true;
}

//
//
//
uint256 TxVerifyFee::createTx0(BlockPtr block) {
	// 0
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));
	SKey lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TxCoinBase::as(TransactionFactory::create(Transaction::COINBASE));
	lTx->addIn();
	Transaction::UnlinkedOut lUTXO = lTx->addOut(lKey0, lPKey0, TxAssetType::qbitAsset(), 10);
	lUTXO.out().setTx(lTx->id());

	store_->pushTransaction(lTx);
	store_->pushUnlinkedOut(lUTXO);

	block->append(lTx);

	return wallet_->pushUnlinkedOut(lUTXO);
}

TransactionPtr TxVerifyFee::createTx1(uint256 utxo, BlockPtr block) {

	// mine
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));
	SKey lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0.createPKey();

	// receiver address
	std::list<std::string> lSeed1;
	lSeed1.push_back(std::string("fitness"));
	lSeed1.push_back(std::string("exchange"));
	lSeed1.push_back(std::string("glance"));
	lSeed1.push_back(std::string("diamond"));
	lSeed1.push_back(std::string("crystal"));
	lSeed1.push_back(std::string("cinnamon"));
	lSeed1.push_back(std::string("border"));
	lSeed1.push_back(std::string("arrange"));
	lSeed1.push_back(std::string("attitude"));
	lSeed1.push_back(std::string("between"));
	lSeed1.push_back(std::string("broccoli"));
	lSeed1.push_back(std::string("cannon"));
	lSeed1.push_back(std::string("crane"));
	lSeed1.push_back(std::string("double"));
	lSeed1.push_back(std::string("eyebrow"));
	lSeed1.push_back(std::string("frequent"));
	lSeed1.push_back(std::string("gravity"));
	lSeed1.push_back(std::string("hidden"));
	lSeed1.push_back(std::string("identify"));
	lSeed1.push_back(std::string("innocent"));
	lSeed1.push_back(std::string("jealous"));
	lSeed1.push_back(std::string("language"));
	lSeed1.push_back(std::string("leopard"));
	lSeed1.push_back(std::string("cannabis"));
	SKey lKey1 = wallet_->createKey(lSeed1);
	PKey lPKey1 = lKey1.createPKey();

	// 1.0
	// create transaction
	TxSpendPtr lTx = TxSpend::as(TransactionFactory::create(Transaction::SPEND));

	Transaction::UnlinkedOut lUTXO;
	wallet_->findUnlinkedOut(utxo, lUTXO);
	lTx->addIn(lKey0, lUTXO);

	Transaction::UnlinkedOut lUTXO1 = lTx->addOut(lKey0, lPKey1 /*to receiver*/, TxAssetType::qbitAsset(), 9);
	lUTXO1.out().setTx(lTx->id());

	Transaction::UnlinkedOut lUTXO2 = lTx->addFeeOut(lKey0, TxAssetType::qbitAsset(), 1); // to miner
	lUTXO2.out().setTx(lTx->id());

	lTx->finalize(lKey0); // bool

	store_->pushTransaction(lTx);

	store_->pushUnlinkedOut(lUTXO1); // add utxo
	store_->pushUnlinkedOut(lUTXO2); // add utxo

	block->append(lTx);

	return lTx;
}

bool TxVerifyFee::execute() {

	BlockPtr lBlock = Block::create();

	//
	// create & check
	uint256 utxo = createTx0(lBlock);
	TransactionPtr lTx1 = createTx1(utxo, lBlock);

	if (lTx1 != nullptr) {
		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_) << 
			TxCoinBaseVerifyPush::instance() << TxSpendVerify::instance() << 
			TxSpendOutVerify::instance() << TxSpendBalanceVerify::instance();
		
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
	store_ = std::make_shared<TxStore>(); 
	wallet_ = std::make_shared<TxWallet>(); 

	store_->pushBlock(lBlock2); // extract block for verification

	bool lFound = false;
	for(TransactionsContainer::iterator lIter = lBlock2->transactions().begin(); lIter != lBlock2->transactions().end(); lIter++) {

		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_) << 
			TxCoinBaseVerifyPush::instance() << TxSpendVerify::instance() << 
			TxSpendOutVerify::instance() << TxSpendBalanceVerify::instance();
		
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

	//
	//
	// recreate store
	store_ = std::make_shared<TxStore>(); 
	wallet_ = std::make_shared<TxWallet>(); 

	// fill address
	// receiver address
	std::list<std::string> lSeed1;
	lSeed1.push_back(std::string("fitness"));
	lSeed1.push_back(std::string("exchange"));
	lSeed1.push_back(std::string("glance"));
	lSeed1.push_back(std::string("diamond"));
	lSeed1.push_back(std::string("crystal"));
	lSeed1.push_back(std::string("cinnamon"));
	lSeed1.push_back(std::string("border"));
	lSeed1.push_back(std::string("arrange"));
	lSeed1.push_back(std::string("attitude"));
	lSeed1.push_back(std::string("between"));
	lSeed1.push_back(std::string("broccoli"));
	lSeed1.push_back(std::string("cannon"));
	lSeed1.push_back(std::string("crane"));
	lSeed1.push_back(std::string("double"));
	lSeed1.push_back(std::string("eyebrow"));
	lSeed1.push_back(std::string("frequent"));
	lSeed1.push_back(std::string("gravity"));
	lSeed1.push_back(std::string("hidden"));
	lSeed1.push_back(std::string("identify"));
	lSeed1.push_back(std::string("innocent"));
	lSeed1.push_back(std::string("jealous"));
	lSeed1.push_back(std::string("language"));
	lSeed1.push_back(std::string("leopard"));
	lSeed1.push_back(std::string("cannabis"));
	wallet_->createKey(lSeed1);

	// re-process block
	store_->pushBlock(lBlock2); // extract block for verification

	lFound = false;
	for(TransactionsContainer::iterator lIter = lBlock2->transactions().begin(); lIter != lBlock2->transactions().end(); lIter++) {

		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_) << 
			TxCoinBaseVerifyPush::instance() << TxSpendVerify::instance() << 
			TxSpendOutVerify::instance() << TxSpendBalanceVerify::instance();
		
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

	return true;
}

//
//
//
uint256 TxVerifyPrivateFee::createTx0(BlockPtr block) {
	// 0
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));
	SKey lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TxCoinBase::as(TransactionFactory::create(Transaction::COINBASE));
	lTx->addIn();
	Transaction::UnlinkedOut lUTXO = lTx->addOut(lKey0, lPKey0, TxAssetType::qbitAsset(), 10);
	lUTXO.out().setTx(lTx->id());

	store_->pushTransaction(lTx);
	store_->pushUnlinkedOut(lUTXO);

	block->append(lTx);

	return wallet_->pushUnlinkedOut(lUTXO);
}

TransactionPtr TxVerifyPrivateFee::createTx1(uint256 utxo, BlockPtr block) {

	// mine
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));
	SKey lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0.createPKey();

	// receiver address
	std::list<std::string> lSeed1;
	lSeed1.push_back(std::string("fitness"));
	lSeed1.push_back(std::string("exchange"));
	lSeed1.push_back(std::string("glance"));
	lSeed1.push_back(std::string("diamond"));
	lSeed1.push_back(std::string("crystal"));
	lSeed1.push_back(std::string("cinnamon"));
	lSeed1.push_back(std::string("border"));
	lSeed1.push_back(std::string("arrange"));
	lSeed1.push_back(std::string("attitude"));
	lSeed1.push_back(std::string("between"));
	lSeed1.push_back(std::string("broccoli"));
	lSeed1.push_back(std::string("cannon"));
	lSeed1.push_back(std::string("crane"));
	lSeed1.push_back(std::string("double"));
	lSeed1.push_back(std::string("eyebrow"));
	lSeed1.push_back(std::string("frequent"));
	lSeed1.push_back(std::string("gravity"));
	lSeed1.push_back(std::string("hidden"));
	lSeed1.push_back(std::string("identify"));
	lSeed1.push_back(std::string("innocent"));
	lSeed1.push_back(std::string("jealous"));
	lSeed1.push_back(std::string("language"));
	lSeed1.push_back(std::string("leopard"));
	lSeed1.push_back(std::string("cannabis"));
	
	SKey lKey1(lSeed1);
	lKey1.create();
	PKey lPKey1 = lKey1.createPKey();


	// 1.0
	// create transaction
	TxSpendPrivatePtr lTx = TransactionHelper::to<TxSpendPrivate>(TransactionFactory::create(Transaction::SPEND_PRIVATE));

	Transaction::UnlinkedOut lUTXO;
	wallet_->findUnlinkedOut(utxo, lUTXO);
	lTx->addIn(lKey0, lUTXO);

	Transaction::UnlinkedOut lUTXO1 = lTx->addOut(lKey0, lPKey1 /*to receiver*/, TxAssetType::qbitAsset(), 5);
	lUTXO1.out().setTx(lTx->id());

	Transaction::UnlinkedOut lUTXO2 = lTx->addOut(lKey0, lPKey0 /*to self*/, TxAssetType::qbitAsset(), 4);
	lUTXO2.out().setTx(lTx->id());

	Transaction::UnlinkedOut lUTXO3 = lTx->addFeeOut(lKey0, TxAssetType::qbitAsset(), 1); // to miner
	lUTXO3.out().setTx(lTx->id());

	lTx->finalize(lKey0); // bool

	store_->pushTransaction(lTx);

	store_->pushUnlinkedOut(lUTXO1); // add utxo
	store_->pushUnlinkedOut(lUTXO2); // add utxo
	store_->pushUnlinkedOut(lUTXO3); // add utxo

	block->append(lTx);

	return lTx;
}

bool TxVerifyPrivateFee::execute() {

	BlockPtr lBlock = Block::create();

	//
	// create & check
	uint256 utxo = createTx0(lBlock);
	TransactionPtr lTx1 = createTx1(utxo, lBlock);

	if (lTx1 != nullptr) {
		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_) << 
			TxCoinBaseVerifyPush::instance() << TxSpendVerify::instance() << 
			TxSpendOutVerify::instance() << TxSpendBalanceVerify::instance();
		
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

	if (wallet_->balance() != 4) {
		error_ = "Incorrect wallet balance";
		return false;		
	}

	//
	// serialize
	DataStream lStream(SER_NETWORK, 0);
	Block::Serializer::serialize<DataStream>(lStream, lBlock);	

	BlockPtr lBlock2 = Block::Deserializer::deserialize<DataStream>(lStream);

	// recreate store
	store_ = std::make_shared<TxStore>(); 
	wallet_ = std::make_shared<TxWallet>(); 

	store_->pushBlock(lBlock2); // extract block for verification

	bool lFound = false;
	for(TransactionsContainer::iterator lIter = lBlock2->transactions().begin(); lIter != lBlock2->transactions().end(); lIter++) {

		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_) << 
			TxCoinBaseVerifyPush::instance() << TxSpendVerify::instance() << 
			TxSpendOutVerify::instance() << TxSpendBalanceVerify::instance();
		
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

	//
	//
	// recreate store
	store_ = std::make_shared<TxStore>(); 
	wallet_ = std::make_shared<TxWallet>(); 

	// fill address
	// receiver address
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));
	wallet_->createKey(lSeed0);

	// re-process block
	store_->pushBlock(lBlock2); // extract block for verification

	lFound = false;
	for(TransactionsContainer::iterator lIter = lBlock2->transactions().begin(); lIter != lBlock2->transactions().end(); lIter++) {

		TransactionProcessor lProcessor = TransactionProcessor(store_, wallet_) << 
			TxCoinBaseVerifyPush::instance() << TxSpendVerify::instance() << 
			TxSpendOutVerify::instance() << TxSpendBalanceVerify::instance();
		
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

	if (wallet_->balance() != 4) {
		error_ = "Incorrect wallet balance";
		return false;	
	}

	return true;
}
