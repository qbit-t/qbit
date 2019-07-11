// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_BLOCKS_H
#define QBIT_UNITTEST_BLOCKS_H

#include <string>
#include <list>

#include "unittest.h"
#include "transactions.h"
#include "../block.h"

namespace qbit {
namespace tests {

class BlockCreate: public Unit {
public:
	BlockCreate(): Unit("BlockCreate") {}

	bool execute();

	TransactionPtr createTx0();
	TransactionPtr createTx1(uint256);
};

}
}

#endif