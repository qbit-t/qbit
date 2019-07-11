// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_TRANSACTIONS_H
#define QBIT_UNITTEST_TRANSACTIONS_H

#include <string>
#include <list>

#include "unittest.h"
#include "../itransactionstore.h"
#include "../transactionvalidator.h"

namespace qbit {
namespace tests {

class TxStore: public ITransactionStore {
public:
	TxStore() {}

	TransactionPtr locateTransaction(uint256 tx) 
	{
		return txs_[tx]; 
	}

	void pushTransaction(TransactionPtr tx) {

		txs_[tx->hash()] = tx;
	}

	std::map<uint256, TransactionPtr> txs_;
};

class TxVerify: public Unit {
public:
	TxVerify(): Unit("TxVerify") { store_ = std::make_shared<TxStore>(); }

	uint256 createTx0();
	TransactionPtr createTx1(uint256);

	bool execute();

	ITransactionStorePtr store_; 
};

}
}

#endif