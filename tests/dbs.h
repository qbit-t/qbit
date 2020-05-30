// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_DB_H
#define QBIT_UNITTEST_DB_H

#include <string>
#include <list>

#include "unittest.h"
#include "../db/db.h"
#include "../db/containers.h"
#include "transactions.h"

namespace qbit {
namespace tests {

class DbContainerCreate: public Unit {
public:
	DbContainerCreate(): Unit("DbContainerCreate") {}

	bool execute();
};

class DbEntityContainerCreate: public Unit {
public:
	DbEntityContainerCreate(): Unit("DbEntityContainerCreate") {
		store_ = std::make_shared<TxStore>(); 
		wallet_ = std::make_shared<TxWallet>(); 		
	}
	DbEntityContainerCreate(const std::string& name): Unit(name) {
		store_ = std::make_shared<TxStore>(); 
		wallet_ = std::make_shared<TxWallet>(); 		
	}

	TransactionPtr createTx0(uint256&);
	TransactionPtr createTx1(uint256);

	bool execute();

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;		
};

class DbContainerIterator: public Unit {
public:
	DbContainerIterator(): Unit("DbContainerIterator") {}

	bool execute();
};

class DbMultiContainerIterator: public Unit {
public:
	DbMultiContainerIterator(): Unit("DbMultiContainerIterator") {}

	bool execute();
};

class DbContainerTransaction: public Unit {
public:
	DbContainerTransaction(): Unit("DbContainerTransaction") {}

	bool execute();
};

class DbEntityContainerTransaction: public DbEntityContainerCreate {
public:
	DbEntityContainerTransaction(): DbEntityContainerCreate("DbEntityContainerTransaction") {}

	bool execute();
};

class DbMultiContainerTransaction: public Unit {
public:
	DbMultiContainerTransaction(): Unit("DbMultiContainerTransaction") {}

	bool execute();
};

class DbMultiContainerRemove: public Unit {
public:
	DbMultiContainerRemove(): Unit("DbMultiContainerRemove") {}

	bool execute();
};

class DbEntityContainerRemove: public DbEntityContainerCreate {
public:
	DbEntityContainerRemove(): DbEntityContainerCreate("DbEntityContainerRemove") {}

	bool execute();
};

class DbContainerRemove: public Unit {
public:
	DbContainerRemove(): Unit("DbContainerRemove") {}

	bool execute();
};

class DbContainerHash: public Unit {
public:
	DbContainerHash(): Unit("DbContainerHash") {}

	bool execute();
};



} // tests
} // qbit

#endif
