// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IVALIDATOR_MANAGER_H
#define QBIT_IVALIDATOR_MANAGER_H

#include "iconsensus.h"
#include "isettings.h"
#include "imemorypool.h"
#include "imemorypoolmanager.h"
#include "itransactionstore.h"
#include "itransactionstoremanager.h"
#include "ivalidator.h"

namespace qbit {

class IValidatorManager {
public:
	IValidatorManager() {}

	virtual bool exists(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IValidatorManager::exists - not implemented."); }
	virtual IValidatorPtr locate(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IValidatorManager::locate - not implemented."); }

	virtual IValidatorPtr push(const uint256& /*chain*/, EntityPtr /*dapp*/) { throw qbit::exception("NOT_IMPL", "IValidatorManager::push - not implemented."); }
	virtual bool pop(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IValidatorManager::pop - not implemented."); }
	virtual std::vector<IValidatorPtr> validators() { throw qbit::exception("NOT_IMPL", "IValidatorManager::validators - not implemented."); }

	virtual void run() { throw qbit::exception("NOT_IMPL", "IValidatorManager::run - not implemented."); }
	virtual void stop() { throw qbit::exception("NOT_IMPL", "IValidatorManager::stop - not implemented."); }

	virtual IMemoryPoolManagerPtr mempoolManager() { throw qbit::exception("NOT_IMPL", "IValidatorManager::mempoolManager - not implemented."); }
};

typedef std::shared_ptr<IValidatorManager> IValidatorManagerPtr;

} // qbit

#endif
