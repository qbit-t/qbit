// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ITRANSACTION_STORE_EXTENSION_H
#define QBIT_ITRANSACTION_STORE_EXTENSION_H

#include "transaction.h"
#include "block.h"
#include "blockcontext.h"
#include "transactioncontext.h"
#include "isettings.h"
#include "itransactionstore.h"

#include <memory>

namespace qbit {

class ITransactionStore;
typedef std::shared_ptr<ITransactionStore> ITransactionStorePtr;

class ITransactionStoreExtension {
public:
	ITransactionStoreExtension() {}

	virtual bool open() { throw qbit::exception("NOT_IMPL", "ITransactionStoreExtension::open - not implemented."); }
	virtual bool close() { throw qbit::exception("NOT_IMPL", "ITransactionStoreExtension::close - not implemented."); }

	virtual bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStoreExtension::pushUnlinkedOut - not implemented."); }
	virtual bool popUnlinkedOut(const uint256&, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStoreExtension::popUnlinkedOut - not implemented."); }

	virtual bool pushEntity(const uint256&, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStoreExtension::pushEntity - not implemented."); }
	virtual void removeTransaction(TransactionPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStoreExtension::removeTransaction - not implemented."); }

	virtual bool isAllowed(TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStoreExtension::isAllowed - not implemented."); }

	virtual bool locateParents(TransactionContextPtr /*root*/, std::list<uint256>& /*parents*/) { throw qbit::exception("NOT_IMPL", "ITransactionStoreExtension::locateParents - not implemeted."); }
};

typedef std::shared_ptr<ITransactionStoreExtension> ITransactionStoreExtensionPtr;

//
class TransactionStoreExtensionCreator {
public:
	TransactionStoreExtensionCreator() {}
	virtual ITransactionStoreExtensionPtr create(ISettingsPtr /*settings*/, ITransactionStorePtr /*store*/) { return nullptr; }
};

typedef std::shared_ptr<TransactionStoreExtensionCreator> TransactionStoreExtensionCreatorPtr;

//
typedef std::map<std::string/*dapp name*/, TransactionStoreExtensionCreatorPtr> TransactionStoreExtensions;
extern TransactionStoreExtensions gStoreExtensions; // TODO: init on startup

} // qbit

#endif
