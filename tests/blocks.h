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
	BlockCreate(): Unit("BlockCreate") {
		store_ = std::make_shared<TxStore>(); 
		wallet_ = std::make_shared<TxWallet>(); 
	}

	bool execute();

	TransactionPtr createTx0(uint256&);
	TransactionPtr createTx1(uint256);

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;	
};

}
}

#endif