// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ITRANSACTION_STORE_H
#define QBIT_ITRANSACTION_STORE_H

#include "transaction.h"
#include "block.h"
#include <memory>

namespace qbit {

class ITransactionStore {
public:
	ITransactionStore() {}

	virtual TransactionPtr locateTransaction(const uint256&) { return nullptr; }
	virtual void pushTransaction(TransactionPtr) {}
	virtual void pushBlock(BlockPtr) {}

	virtual uint256 pushUnlinkedOut(Transaction::UnlinkedOutPtr) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	virtual bool popUnlinkedOut(const uint256&) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	virtual Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
};

typedef std::shared_ptr<ITransactionStore> ITransactionStorePtr;

} // qbit

#endif
