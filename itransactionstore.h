// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ITRANSACTION_STORE_H
#define QBIT_ITRANSACTION_STORE_H

#include "transaction.h"
#include "block.h"
#include "blockcontext.h"
#include "transactioncontext.h"
#include "ientitystore.h"

#include <memory>

namespace qbit {

class ITransactionStore {
public:
	ITransactionStore() {}

	virtual bool open() { throw qbit::exception("NOT_IMPL", "ITransactionStore::open - not implemented."); }
	virtual bool close() { throw qbit::exception("NOT_IMPL", "ITransactionStore::close - not implemented."); }
	virtual bool isOpened() { throw qbit::exception("NOT_IMPL", "ITransactionStore::isOpened - not implemented."); }

	virtual TransactionPtr locateTransaction(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::locateTransaction - not implemented."); }
	virtual TransactionContextPtr locateTransactionContext(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::locateTransactionContext - not implemented."); }
	virtual bool pushTransaction(TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::pushTransaction - not implemented."); }
	virtual BlockContextPtr pushBlock(BlockPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::pushBlock - not implemented."); }
	virtual bool commitBlock(BlockContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::commitBlock - not implemented."); }

	// main entry
	virtual TransactionContextPtr pushTransaction(TransactionPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::pushTransaction - not implemented."); }

	virtual bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::pushUnlinkedOut - not implemented."); }
	virtual bool popUnlinkedOut(const uint256&, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::popUnlinkedOut - not implemented."); }
	virtual void addLink(const uint256& /*from*/, const uint256& /*to*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::addLink - not implemented."); }

	virtual Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::findUnlinkedOut - not implemented."); }
	virtual bool isUnlinkedOutUsed(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::isUnlinkedOutUsed - not implemented."); }
	virtual bool isUnlinkedOutExists(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::isUnlinkedOutExists - not implemented."); }

	virtual IEntityStorePtr entityStore() { throw qbit::exception("NOT_IMPL", "ITransactionStore::entityStore - not implemented."); }

	virtual bool rollbackToHeight(size_t) { throw qbit::exception("NOT_IMPL", "ITransactionStore::rollbackToHeight - not implemented."); }
	virtual bool resyncHeight() { throw qbit::exception("NOT_IMPL", "ITransactionStore::resyncHeight - not implemented."); }

	virtual size_t currentHeight() { throw qbit::exception("NOT_IMPL", "ITransactionStore::currentHeight - not implemented."); }
	virtual BlockHeader currentBlockHeader() { throw qbit::exception("NOT_IMPL", "ITransactionStore::currentBlockHeader - not implemented."); }
};

typedef std::shared_ptr<ITransactionStore> ITransactionStorePtr;

} // qbit

#endif
