#include "dbs.h"
#include "../mkpath.h"

using namespace qbit;
using namespace qbit::tests;

bool DbContainerCreate::execute() {

	try {
		rmpath("/tmp/db_test");

		db::DbContainer<std::string, std::string> lContainer("/tmp/db_test");
		lContainer.open();

		if (!lContainer.write("key100", "value100")) { error_ = "Write failed"; return false; }
		
		std::string lValue;
		if (!lContainer.read("key100", lValue)) {
			error_ = "Read failed"; return false;
		}

		if (lValue != "value100") {
			error_ = "Read value is wrong"; return false;
		}
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}

	return true;
}

TransactionPtr DbEntityContainerCreate::createTx0(uint256& utxo) {
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

	SKeyPtr lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0->createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TransactionHelper::to<TxCoinBase>(TransactionFactory::create(Transaction::COINBASE));
	lTx->addIn();
	unsigned char* asset0 = (unsigned char*)"01234567890123456789012345678901";
	Transaction::UnlinkedOutPtr lUTXO = lTx->addOut(*lKey0, lPKey0, uint256(asset0), 10);
	lUTXO->out().setTx(lTx->id());

	//std::cout << std::endl << lTx->toString() << std::endl;

	store_->pushTransaction(lTx);
	store_->pushUnlinkedOut(lUTXO, nullptr);
	wallet_->pushUnlinkedOut(lUTXO, nullptr);
	utxo = lUTXO->hash();
	return lTx;
}

TransactionPtr DbEntityContainerCreate::createTx1(uint256 utxo) {
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

	SKeyPtr lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0->createPKey();

	// 1.0
	// create transaction
	TxSpendPtr lTx = TransactionHelper::to<TxSpend>(TransactionFactory::create(Transaction::SPEND));

	Transaction::UnlinkedOutPtr lUTXO = wallet_->findUnlinkedOut(utxo);
	lTx->addIn(*lKey0, lUTXO);

	unsigned char* asset0 = (unsigned char*)"01234567890123456789012345678901";
	Transaction::UnlinkedOutPtr lUTXO1 = lTx->addOut(*lKey0, lPKey0, uint256(asset0), 10);
	lUTXO1->out().setTx(lTx->id());

	lTx->finalize(*lKey0); // bool

	store_->pushTransaction(lTx);
	store_->pushUnlinkedOut(lUTXO, nullptr);
	wallet_->pushUnlinkedOut(lUTXO, nullptr);

	return lTx;
}

bool DbEntityContainerCreate::execute() {

		//
		// create & check
		uint256 utxo;
		TransactionPtr lTx0 = createTx0(utxo);
		TransactionPtr lTx1 = createTx1(utxo);

	try {
		rmpath("/tmp/db_tx");
		db::DbEntityContainer<uint256, Transaction> lTxContainer("/tmp/db_tx");
		lTxContainer.open();

		if (!lTxContainer.write(lTx0->hash(), lTx0)) { error_ = "Tx0 write failed"; return false; }
		if (!lTxContainer.write(lTx1->hash(), lTx1)) { error_ = "Tx1 write failed"; return false; }
		
		TransactionPtr lTx00 = lTxContainer.read(lTx0->hash());
		if (lTx00 == nullptr) { error_ = "Tx00 read failed"; return false; }

		TransactionPtr lTx01 = lTxContainer.read(lTx1->hash());
		if (lTx01 == nullptr) { error_ = "Tx01 read failed"; return false; }

		if (lTx0->hash() != lTx00->hash()) { error_ = "Tx0 != Tx00"; return false; }
		if (lTx1->hash() != lTx01->hash()) { error_ = "Tx1 != Tx01"; return false; }

		//std::cout << std::endl << lTx00->toString() << std::endl;
		//std::cout << lTx01->toString() << std::endl;
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}
	
	try {
		rmpath("/tmp/db_block");

		db::DbEntityContainer<uint256, Block> lBlockContainer("/tmp/db_block");
		lBlockContainer.open();

		BlockPtr lBlock = Block::instance();
		lBlock->append(lTx0);
		lBlock->append(lTx1);

		if (!lBlockContainer.write(lBlock->hash(), lBlock)) { error_ = "Block write failed"; return false; }
		
		BlockPtr lBlock1 = lBlockContainer.read(lBlock->hash());
		if (lBlock1 == nullptr) { error_ = "Block1 read failed"; return false; }

		if (lBlock->hash() != lBlock1->hash()) { error_ = "Block != Block1"; return false; }

		//std::cout << std::endl << lBlock1->toString() << std::endl;
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}

	return true;
}

bool DbContainerIterator::execute() {

	try {
		db::DbContainer<std::string, std::string> lContainer("/tmp/db_test");
		lContainer.open();

		for (int lIdx = 101; lIdx < 200; lIdx++) {
			std::stringstream lKey;
			std::stringstream lValue;

			lKey << "key" << lIdx;
			lValue << "value" << lIdx;

			lContainer.write(lKey.str(), lValue.str());		
		}
		
		db::DbContainer<std::string, std::string>::Iterator lIter = lContainer.find("key111");
		if(!(lIter.valid() && (*lIter) == "value111")) {
			error_ = "data != value111"; return false; 
		}
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}

	return true;
}

bool DbMultiContainerIterator::execute() {

	try {
		rmpath("/tmp/db_multi_string");

		db::DbMultiContainer<std::string, std::string> lContainer("/tmp/db_multi_string");
		lContainer.open();

		for (int lIdx = 0; lIdx < 10; lIdx++) {
			std::stringstream lKey;
			std::stringstream lValue;

			lKey << "key";
			lValue << "value_s" << lIdx;
			lContainer.write(lKey.str(), lValue.str());

			std::stringstream lKey2;
			std::stringstream lValue2;

			lKey2 << "key2";
			lValue2 << "value_ss" << lIdx;
			lContainer.write(lKey2.str(), lValue2.str());
		}

		int lCount = 0;
		for (db::DbMultiContainer<std::string, std::string>::Iterator lIter = lContainer.find("key"); lIter.valid(); lIter++, lCount++) {
			//std::cout << (*lIter) << std::endl;
			std::string lVal = (*lIter);
			
			bool lFound = false;
			for (int lIdx = 0; lIdx < 10; lIdx++) {
				std::stringstream lValue;
				lValue << "value_s" << lIdx;

				if (lVal == lValue.str()) { lFound = true; }
			}

			if (!lFound) { error_ = "(str)lVal != lValue"; return false; }
		}

		if (lCount != 10) { error_ = "(str)Count != 10"; return false; }
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}

	try {
		rmpath("/tmp/db_multi_int");

		db::DbMultiContainer<int, std::string> lContainer("/tmp/db_multi_int");
		lContainer.open();

		for (int lIdx = 0; lIdx < 10; lIdx++) {
			std::stringstream lValue;
			std::stringstream lValue2;

			lValue << "value_i" << lIdx;
			lValue2 << "value_ii" << lIdx;

			lContainer.write(16, lValue.str());		
			lContainer.write(15, lValue2.str());		
		}

		int lCount = 0;
		for (db::DbMultiContainer<int, std::string>::Iterator lIter = lContainer.find(16); lIter.valid(); ++lIter, lCount++) {
			//std::cout << (*lIter) << std::endl;
			std::string lVal = (*lIter);
			
			bool lFound = false;
			for (int lIdx = 0; lIdx < 10; lIdx++) {
				std::stringstream lValue;
				lValue << "value_i" << lIdx;

				if (lVal == lValue.str()) { lFound = true; }
			}

			if (!lFound) { error_ = "(int)lVal != lValue"; return false; }
		}

		if (lCount != 10) { error_ = "(int)Count != 10"; return false; }
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}

	return true;
}

bool DbContainerTransaction::execute() {

	try {
		db::DbContainer<std::string, std::string> lContainer("/tmp/db_test");
		lContainer.open();

		db::DbContainer<std::string, std::string>::Transaction lTransaction = lContainer.transaction();

		for (int lIdx = 201; lIdx < 300; lIdx++) {
			std::stringstream lKey;
			std::stringstream lValue;

			lKey << "key" << lIdx;
			lValue << "value" << lIdx;

			lTransaction.write(lKey.str(), lValue.str());
		}

		if(!lTransaction.commit()) { error_ = "Commit failed"; return false; }
		
		db::DbContainer<std::string, std::string>::Iterator lIter = lContainer.find("key222");
		if(!(lIter.valid() && (*lIter) == "value222")) {
			error_ = "data != value222"; return false; 
		}
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}

	return true;
}

bool DbEntityContainerTransaction::execute() {

	try {
		rmpath("/tmp/db_tx_transaction");

		db::DbEntityContainer<uint256, Transaction> lTxContainer("/tmp/db_tx_transaction");
		lTxContainer.open();

		//
		// create & check
		uint256 utxo;
		TransactionPtr lTx0 = createTx0(utxo);
		TransactionPtr lTx1 = createTx1(utxo);

		db::DbEntityContainer<uint256, Transaction>::Transaction lTransaction = lTxContainer.transaction();
		{
			lTransaction.write(lTx0->hash(), lTx0);
			lTransaction.write(lTx1->hash(), lTx1);

			if (!lTransaction.commit()) { error_ = "Commit failed"; return false; }
		}
		
		TransactionPtr lTx00 = lTxContainer.read(lTx0->hash());
		if (lTx00 == nullptr) { error_ = "Tx00 read failed"; return false; }

		TransactionPtr lTx01 = lTxContainer.read(lTx1->hash());
		if (lTx01 == nullptr) { error_ = "Tx01 read failed"; return false; }

		if (lTx0->hash() != lTx00->hash()) { error_ = "Tx0 != Tx00"; return false; }
		if (lTx1->hash() != lTx01->hash()) { error_ = "Tx1 != Tx01"; return false; }

		//std::cout << std::endl << lTx00->toString() << std::endl;
		//std::cout << lTx01->toString() << std::endl;
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}

	return true;
}

bool DbMultiContainerTransaction::execute() {

	try {
		db::DbMultiContainer<std::string, std::string> lContainer("/tmp/db_multi_string");
		lContainer.open();

		db::DbMultiContainer<std::string, std::string>::Transaction lTransaction = lContainer.transaction();

		for (int lIdx = 10; lIdx < 20; lIdx++) {
			std::stringstream lKey;
			std::stringstream lValue;

			lKey << "key3";
			lValue << "value_s" << lIdx;
			lTransaction.write(lKey.str(), lValue.str());

			std::stringstream lKey2;
			std::stringstream lValue2;

			lKey2 << "key4";
			lValue2 << "value_ss" << lIdx;
			lTransaction.write(lKey2.str(), lValue2.str());
		}

		lTransaction.commit();

		int lCount = 0;
		for (db::DbMultiContainer<std::string, std::string>::Iterator lIter = lContainer.find("key3"); lIter.valid(); lIter++, lCount++) {
			//std::cout << (*lIter) << std::endl;
			std::string lVal = (*lIter);
			
			bool lFound = false;
			for (int lIdx = 10; lIdx < 20; lIdx++) {
				std::stringstream lValue;
				lValue << "value_s" << lIdx;

				if (lVal == lValue.str()) { lFound = true; }
			}

			if (!lFound) { error_ = "(str)lVal != lValue"; return false; }
		}

		if (lCount != 10) { error_ = "(str)Count != 10"; return false; }
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}

	return true;
}

bool DbMultiContainerRemove::execute() {

	try {
		db::DbMultiContainer<std::string, std::string> lContainer("/tmp/db_multi_string");
		lContainer.open();

		db::DbMultiContainer<std::string, std::string>::Transaction lTransaction = lContainer.transaction();

		int lCount = 0;
		for (db::DbMultiContainer<std::string, std::string>::Iterator lIter = lContainer.find("key3"); lIter.valid(); lIter++, lCount++) {
			lContainer.remove(lIter);			
		}

		if (lCount != 10) { error_ = "(str)Count != 10"; return false; }

		//
		// find* works well, because all key3* entries was erased 
		db::DbMultiContainer<std::string, std::string>::Iterator lIter = lContainer.find("key3");
		if (lIter.valid()) { error_ = "(str)Iter is valid"; return false; }
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}

	return true;
}

bool DbEntityContainerRemove::execute() {

	try {
		db::DbEntityContainer<uint256, Transaction> lTxContainer("/tmp/db_tx_transaction");
		lTxContainer.open();
		
		//
		// create & check
		uint256 utxo;
		TransactionPtr lTx0 = createTx0(utxo);
		TransactionPtr lTx1 = createTx1(utxo);

		db::DbEntityContainer<uint256, Transaction>::Transaction lTransaction = lTxContainer.transaction();
		{
			lTransaction.write(lTx0->hash(), lTx0);
			lTransaction.write(lTx1->hash(), lTx1);

			if (!lTransaction.commit()) { error_ = "Commit failed"; return false; }
		}		

		TransactionPtr lTx00 = lTxContainer.read(lTx0->hash());
		if (lTx00 == nullptr) { error_ = "Tx00 read failed"; return false; }

		if (!lTxContainer.remove(lTx00->hash())) { error_ = "Tx00 remove failed"; return false; }

		lTx00 = lTxContainer.read(lTx0->hash());
		if (lTx00 != nullptr) { error_ = "Tx00 exists"; return false; }
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}

	return true;
}

bool DbContainerRemove::execute() {

	try {
		db::DbContainer<std::string, std::string> lContainer("/tmp/db_test");
		lContainer.open();

		if (!lContainer.remove("key222")) { error_ = "Error deleting"; return false; }

		std::string lVal;
		if (lContainer.read("key222", lVal)) {
			error_ = "key222 exists"; return false; 
		}

		db::DbContainer<std::string, std::string>::Transaction lTransaction = lContainer.transaction();

		for (int lIdx = 201; lIdx < 300; lIdx++) {
			std::stringstream lKey;
			lKey << "key" << lIdx;

			lTransaction.remove(lKey.str());
		}

		if(!lTransaction.commit()) { error_ = "Commit failed"; return false; }
		
		for (int lIdx = 201; lIdx < 300; lIdx++) {
			std::stringstream lKey;
			lKey << "key" << lIdx;

			std::string lVal1;
			if (lContainer.read(lKey.str(), lVal1)) {
				error_ = lKey.str() + " exists"; return false; 
			}
		}
	}
	catch(const std::exception& ex) {
		error_ = ex.what();
		return false;
	}

	return true;
}
