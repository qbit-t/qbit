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

namespace qbit {
namespace tests {

class DbContainerCreate: public Unit {
public:
	DbContainerCreate(): Unit("DbContainerCreate") {}

	bool execute();
};

class DbEntityContainerCreate: public Unit {
public:
	DbEntityContainerCreate(): Unit("DbEntityContainerCreate") {}

	TransactionPtr createTx0();
	TransactionPtr createTx1(uint256);

	bool execute();
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

} // tests
} // qbit

#endif
