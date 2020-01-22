// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IMEMORY_POOL_MANAGER_H
#define QBIT_IMEMORY_POOL_MANAGER_H

namespace qbit {
	class IMemoryPool;
	typedef std::shared_ptr<IMemoryPool> IMemoryPoolPtr;	
}

#include "iconsensus.h"
#include "isettings.h"
#include "imemorypool.h"
#include "itransactionstore.h"

namespace qbit {

class IMemoryPoolManager {
public:
	IMemoryPoolManager() {}

	virtual bool exists(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IMemoryPoolManager::exists - not implemented."); }
	virtual IMemoryPoolPtr locate(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IMemoryPoolManager::locate - not implemented."); }

	virtual bool add(IMemoryPoolPtr /*pool*/) { throw qbit::exception("NOT_IMPL", "IMemoryPoolManager::add - not implemented."); }
	virtual void push(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IMemoryPoolManager::push - not implemented."); }
	virtual void pop(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IMemoryPoolManager::pop - not implemented."); }	
	virtual std::vector<IMemoryPoolPtr> pools() { throw qbit::exception("NOT_IMPL", "IMemoryPoolManager::pools - not implemented."); }
};

typedef std::shared_ptr<IMemoryPoolManager> IMemoryPoolManagerPtr;

} // qbit

#endif
