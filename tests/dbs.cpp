#include "dbs.h"

using namespace qbit;
using namespace qbit::tests;

bool DbContainerCreate::execute() {

	try {
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

TransactionPtr DbEntityContainerCreate::createTx0() {
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

	SKey lKey0(lSeed0);
	lKey0.create();
	
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TxCoinBase::as(TransactionFactory::create(Transaction::COINBASE));
	lTx->initialize(lPKey0, 10);

	return lTx;
}

TransactionPtr DbEntityContainerCreate::createTx1(uint256 tx0) {
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

	SKey lKey0(lSeed0);
	lKey0.create();
	
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxSpendPtr lTx = TxSpend::as(TransactionFactory::create(Transaction::SPEND));

	lTx->in().resize(1); // add input
	lTx->in()[0].out().setHash(tx0);
	lTx->in()[0].out().setIndex(0);

	if (!lTx->initIn(lTx->in()[0], lPKey0, lKey0)) { error_ = "Signing failed"; return nullptr; }

	lTx->out().resize(1); // add out
	lTx->out()[0].setAmount(5);

	if (!lTx->initOut(lTx->out()[0], lPKey0)) { error_ = "Destination failed"; return nullptr; }

	return lTx;
}

bool DbEntityContainerCreate::execute() {

	try {
		db::DbEntityContainer<uint256, Transaction> lTxContainer("/tmp/db_tx");
		lTxContainer.open();

		//
		// create & check
		TransactionPtr lTx0 = createTx0();
		TransactionPtr lTx1 = createTx1(lTx0->hash());

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
		db::DbEntityContainer<uint256, Block> lBlockContainer("/tmp/db_block");
		lBlockContainer.open();

		//
		// create & check
		TransactionPtr lTx0 = createTx0();
		TransactionPtr lTx1 = createTx1(lTx0->hash());

		BlockPtr lBlock = Block::create();
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
