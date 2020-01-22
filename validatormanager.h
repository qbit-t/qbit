// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_VALIDATOR_MANAGER_H
#define QBIT_VALIDATOR_MANAGER_H

#include "iconsensusmanager.h"
#include "iconsensus.h"
#include "isettings.h"
#include "imemorypool.h"
#include "imemorypoolmanager.h"
#include "itransactionstore.h"
#include "itransactionstoremanager.h"
#include "ivalidator.h"
#include "ivalidatormanager.h"
#include "validator.h"

namespace qbit {

class ValidatorManager: public IValidatorManager, public std::enable_shared_from_this<ValidatorManager> {
public:
	ValidatorManager(ISettingsPtr settings, IConsensusManagerPtr consensusManager, IMemoryPoolManagerPtr mempoolManager, ITransactionStoreManagerPtr storeManager) : 
		settings_(settings), consensusManager_(consensusManager), mempoolManager_(mempoolManager), storeManager_(storeManager) {}

	bool exists(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(validatorsMutex_);
		return validators_.find(chain) != validators_.end();
	}

	IValidatorPtr locate(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(validatorsMutex_);
		std::map<uint256, IValidatorPtr>::iterator lValidator = validators_.find(chain);
		if (lValidator != validators_.end()) return lValidator->second;
		return nullptr;
	}

	IValidatorPtr push(const uint256& chain) {
		//
		boost::unique_lock<boost::mutex> lLock(validatorsMutex_);
		if (validators_.find(chain) == validators_.end()) {
			IValidatorPtr lValidator = ValidatorFactory::create(chain, consensusManager_->locate(chain), mempoolManager_->locate(chain), storeManager_->locate(chain), nullptr);
			validators_[chain] = lValidator;
			return lValidator;
		}

		return nullptr;
	}

	bool pop(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(validatorsMutex_);
		std::map<uint256, IValidatorPtr>::iterator lValidator = validators_.find(chain);
		if (lValidator != validators_.end()) {
			lValidator->second->stop();
			validators_.erase(chain);
			return true;
		}

		return false;
	}

	std::vector<IValidatorPtr> validators() {
		//
		boost::unique_lock<boost::mutex> lLock(validatorsMutex_);
		std::vector<IValidatorPtr> lValidators;

		for (std::map<uint256, IValidatorPtr>::iterator lValidator = validators_.begin(); lValidator != validators_.end(); lValidator++) {
			lValidators.push_back(lValidator->second);
		}

		return lValidators;
	}

	static IValidatorManagerPtr instance(ISettingsPtr settings, IConsensusManagerPtr consensusManager, IMemoryPoolManagerPtr mempoolManager, ITransactionStoreManagerPtr storeManager) {
		return std::make_shared<ValidatorManager>(settings, consensusManager, mempoolManager, storeManager);
	}

	void run() {
		boost::unique_lock<boost::mutex> lLock(validatorsMutex_);

		for (std::map<uint256, IValidatorPtr>::iterator lValidator = validators_.begin(); lValidator != validators_.end(); lValidator++) {
			lValidator->second->run();
		}
	}

	void stop() {
		boost::unique_lock<boost::mutex> lLock(validatorsMutex_);

		for (std::map<uint256, IValidatorPtr>::iterator lValidator = validators_.begin(); lValidator != validators_.end(); lValidator++) {
			lValidator->second->stop();
		}
	}

	IMemoryPoolManagerPtr mempoolManager() { return mempoolManager_; }

private:
	boost::mutex validatorsMutex_;
	std::map<uint256, IValidatorPtr> validators_;
	ISettingsPtr settings_;
	IConsensusManagerPtr consensusManager_;
	IMemoryPoolManagerPtr mempoolManager_;
	ITransactionStoreManagerPtr storeManager_;
};

} // qbit

#endif
