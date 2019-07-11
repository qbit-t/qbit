// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ITRANSACTION_STORE_H
#define QBIT_ITRANSACTION_STORE_H

#include "transaction.h"
#include <memory>

namespace qbit {

class ITransactionStore {
public:
	ITransactionStore() {}

	virtual TransactionPtr locateTransaction(uint256) { return nullptr; }
	virtual void pushTransaction(TransactionPtr) {}
};

typedef std::shared_ptr<ITransactionStore> ITransactionStorePtr;

} // qbit

#endif
