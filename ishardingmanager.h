// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ISHARDING_MANAGER_H
#define QBIT_ISHARDING_MANAGER_H

#include "isettings.h"
#include "iconsensusmanager.h"
#include "imemorypoolmanager.h"
#include "itransactionstoremanager.h"
#include "ivalidatormanager.h"

namespace qbit {

class IShardingManager {
public:
	IShardingManager() {}

	virtual bool exists(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IShardingManager::exists - not implemented."); }
	virtual void push(const uint256& /*chain*/, EntityPtr /*dapp*/) { throw qbit::exception("NOT_IMPL", "IShardingManager::push - not implemented."); }
	virtual void pop(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IShardingManager::pop - not implemented."); }		
	virtual void run() { throw qbit::exception("NOT_IMPL", "IShardingManager::run - not implemented."); }
	virtual void stop() { throw qbit::exception("NOT_IMPL", "IShardingManager::stop - not implemented."); }
};

typedef std::shared_ptr<IShardingManager> IShardingManagerPtr;

} // qbit

#endif
