// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ITRANSACTION_STORE_MANAGER_H
#define QBIT_ITRANSACTION_STORE_MANAGER_H

#include "iconsensus.h"
#include "isettings.h"
#include "imemorypool.h"
#include "itransactionstore.h"

namespace qbit {

class ITransactionStoreManager {
public:
	ITransactionStoreManager() {}

	virtual bool exists(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "ITransactionStoreManager::exists - not implemented."); }
	virtual ITransactionStorePtr locate(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "ITransactionStoreManager::locate - not implemented."); }

	virtual bool add(ITransactionStorePtr /*store*/) { throw qbit::exception("NOT_IMPL", "ITransactionStoreManager::add - not implemented."); }
	virtual ITransactionStorePtr push(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "ITransactionStoreManager::push - not implemented."); }
	virtual void pop(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "ITransactionStoreManager::pop - not implemented."); }		
	virtual std::vector<ITransactionStorePtr> storages() { throw qbit::exception("NOT_IMPL", "ITransactionStoreManager::storages - not implemented."); }

	virtual void stop() { throw qbit::exception("NOT_IMPL", "ITransactionStoreManager::stop - not implemented."); }
};

typedef std::shared_ptr<ITransactionStoreManager> ITransactionStoreManagerPtr;

} // qbit

#endif
