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

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/chrono.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace qbit {

class ValidatorManager: public IValidatorManager, public std::enable_shared_from_this<ValidatorManager> {
public:
	ValidatorManager(ISettingsPtr settings, IConsensusManagerPtr consensusManager, IMemoryPoolManagerPtr mempoolManager, ITransactionStoreManagerPtr storeManager) : 
		settings_(settings), consensusManager_(consensusManager), mempoolManager_(mempoolManager), storeManager_(storeManager),
		signals_(context_) {

		signals_.add(SIGINT);
		signals_.add(SIGTERM);
#if defined(SIGQUIT)
		signals_.add(SIGQUIT);
#endif
		signals_.async_wait(boost::bind(&ValidatorManager::stop, this));
	}

	static void registerValidator(const std::string& name, ValidatorCreatorPtr creator) {
		gValidators[name] = creator;
	}

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

	IValidatorPtr push(const uint256& chain, EntityPtr dapp) {
		//
		boost::unique_lock<boost::mutex> lLock(validatorsMutex_);
		if (validators_.find(chain) == validators_.end()) {
			IValidatorPtr lValidator = ValidatorFactory::create(chain, consensusManager_->locate(chain), mempoolManager_->locate(chain), storeManager_->locate(chain), dapp);
			if (lValidator) {
				validators_[chain] = lValidator;
				lValidator->run();
			} else {
				if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, 
						strprintf("[manager/push]: validator was NOT FOUND for %s/%s#", dapp->entityName(), chain.toHex().substr(0, 10)));
			}

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
		//
		bool lHasValidators = false;
		{
			boost::unique_lock<boost::mutex> lLock(validatorsMutex_);
			lHasValidators = validators_.size() > 0;

			for (std::map<uint256, IValidatorPtr>::iterator lValidator = validators_.begin(); lValidator != validators_.end(); lValidator++) {
				lValidator->second->run();
			}
		}

		//
		if (!controller_ && lHasValidators) {
			timer_ = TimerPtr(new boost::asio::steady_timer(context_, boost::asio::chrono::seconds(300))); // 5 mins since start
			timer_->async_wait(boost::bind(&ValidatorManager::touch, shared_from_this(), boost::asio::placeholders::error));			

			controller_ = boost::shared_ptr<boost::thread>(
						new boost::thread(
							boost::bind(&ValidatorManager::controller, shared_from_this())));
		}		
	}

	void stop() {
		gLog().write(Log::INFO, std::string("[validators/stop]: stopping..."));
		boost::unique_lock<boost::mutex> lLock(validatorsMutex_);
		for (std::map<uint256, IValidatorPtr>::iterator lValidator = validators_.begin(); lValidator != validators_.end(); lValidator++) {
			lValidator->second->stop();
		}

		context_.stop();
	}

	IMemoryPoolManagerPtr mempoolManager() { return mempoolManager_; }

private:
	void controller() {
		// log
		gLog().write(Log::INFO, std::string("[validators]: starting..."));

		try {
			context_.run();
		} 
		catch(qbit::exception& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[validators]: qbit error -> ") + ex.what());
		}
		catch(boost::system::system_error& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[validators]: context error -> ") + ex.what());
		}
		catch(std::runtime_error& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[validators]: runtime error -> ") + ex.what());
		}

		// log
		gLog().write(Log::INFO, std::string("[validators]: stopped"));
	}	

	void touch(const boost::system::error_code& error) {
		// life control
		if(!error) {
			//
			gLog().write(Log::INFO, std::string("[aggregate]: trying to aggregate..."));
			if (consensusManager_->locate(MainChain::id())->chainState() == IConsensus::SYNCHRONIZED) {
				//
				TransactionContextPtr lCtx = consensusManager_->wallet()->aggregateCoinbaseTxs();
				if (lCtx != nullptr && !lCtx->errors().size()) {
					// push to memory pool
					IMemoryPoolPtr lMempool = consensusManager_->wallet()->mempoolManager()->locate(MainChain::id()); // all spend txs - to the main chain
					if (lMempool) {
						//
						if (lMempool->pushTransaction(lCtx)) {
							// check for errors
							if (lCtx->errors().size()) {
								// rollback transaction
								consensusManager_->wallet()->rollback(lCtx);
								// error
								gLog().write(Log::GENERAL_ERROR, std::string("[aggregate/error]: E_TX_MEMORYPOOL - ") + *lCtx->errors().begin());
								lCtx = nullptr;
							} else if (!lMempool->consensus()->broadcastTransaction(lCtx, consensusManager_->wallet()->firstKey()->createPKey().id())) {
								// error
								gLog().write(Log::GENERAL_ERROR, std::string("[aggregate/error]: E_TX_NOT_BROADCASTED - Transaction is not broadcasted"));
								// rollback transaction
								consensusManager_->wallet()->rollback(lCtx);
								// reset
								lCtx = nullptr;
							} else {
								gLog().write(Log::GENERAL_ERROR, std::string("[aggregate]: ") + strprintf("tx = %s successed", lCtx->tx()->hash().toHex()));
							}
						} else {
							// error
							gLog().write(Log::GENERAL_ERROR, std::string("[aggregate/error]: E_TX_EXISTS - Transaction already exists"));
							// rollback transaction
							consensusManager_->wallet()->rollback(lCtx);
							// reset
							lCtx = nullptr;
						}
					}
				}
			}
		}

		timer_->expires_at(timer_->expiry() + boost::asio::chrono::milliseconds(1000*60*30)); // 30 minutes
		timer_->async_wait(boost::bind(&ValidatorManager::touch, shared_from_this(), boost::asio::placeholders::error));
	}

private:
	boost::mutex validatorsMutex_;
	std::map<uint256, IValidatorPtr> validators_;
	ISettingsPtr settings_;
	IConsensusManagerPtr consensusManager_;
	IMemoryPoolManagerPtr mempoolManager_;
	ITransactionStoreManagerPtr storeManager_;

	// controller
	boost::shared_ptr<boost::thread> controller_;
	// context
	boost::asio::io_context context_;
	// signals
	boost::asio::signal_set signals_;
	// timer
	typedef std::shared_ptr<boost::asio::steady_timer> TimerPtr;
	TimerPtr timer_;
};

} // qbit

#endif
