// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_SHARDING_MANAGER_H
#define QBIT_SHARDING_MANAGER_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include "ishardingmanager.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/chrono.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace qbit {

class ShardingManager: public IShardingManager, public std::enable_shared_from_this<ShardingManager> {
public:
	ShardingManager(ISettingsPtr settings, IConsensusManagerPtr consensusManager, 
						ITransactionStoreManagerPtr storeManager, IValidatorManagerPtr validatorManager, IMemoryPoolManagerPtr mempoolManager): 
		settings_(settings), consensusManager_(consensusManager), storeManager_(storeManager), validatorManager_(validatorManager), mempoolManager_(mempoolManager),
		signals_(context_) {

		signals_.add(SIGINT);
		signals_.add(SIGTERM);
#if defined(SIGQUIT)
		signals_.add(SIGQUIT);
#endif
		signals_.async_wait(boost::bind(&ShardingManager::stop, this));
	}

	bool exists(const uint256& chain) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(shardsMutex_);
		return pendingShards_.find(chain) != pendingShards_.end() || activeShards_.find(chain) != activeShards_.end();
	}

	void push(const uint256& chain, EntityPtr dapp) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(shardsMutex_);
		if (!exists(chain)) {
			pendingShards_.insert(std::map<uint256, EntityPtr>::value_type(chain, dapp));
		}
	}

	void pop(const uint256& chain) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(shardsMutex_);
		if (pendingShards_.find(chain) != pendingShards_.end()) {
			pendingShards_.erase(chain);
		} else if (activeShards_.find(chain) != activeShards_.end()) {
			//
			validatorManager_->pop(chain);
			mempoolManager_->pop(chain);
			consensusManager_->pop(chain);
			storeManager_->pop(chain);

			activeShards_.erase(chain);
		}
	}

	void run() {
		if (!controller_) {
			timer_ = TimerPtr(new boost::asio::steady_timer(context_, boost::asio::chrono::seconds(10))); // first tick - 15 secs
			timer_->async_wait(boost::bind(&ShardingManager::touch, shared_from_this(), boost::asio::placeholders::error));			

			controller_ = boost::shared_ptr<boost::thread>(
						new boost::thread(
							boost::bind(&ShardingManager::controller, shared_from_this())));
		}
	}

	void stop() {
		gLog().write(Log::INFO, std::string("[sharding/stop]: stopping..."));
		context_.stop();
	}

	static IShardingManagerPtr instance(ISettingsPtr settings, IConsensusManagerPtr consensusManager, 
											ITransactionStoreManagerPtr storeManager, IValidatorManagerPtr validatorManager,
											IMemoryPoolManagerPtr mempoolManager) {
		return std::make_shared<ShardingManager>(settings, consensusManager, storeManager, validatorManager, mempoolManager);
	}

	static void registerStoreExtension(const std::string& dappName, TransactionStoreExtensionCreatorPtr extension) {
		//
		gStoreExtensions.insert(std::map<std::string /*dapp name*/, TransactionStoreExtensionCreatorPtr>::value_type(
			dappName,
			extension));
	}

private:
	TransactionStoreExtensionCreatorPtr locateStoreExtension(const std::string& dappName) {
		//
		std::map<std::string /*dapp name*/, TransactionStoreExtensionCreatorPtr>::iterator lExtension = gStoreExtensions.find(dappName);
		if (lExtension != gStoreExtensions.end()) {
			return lExtension->second;
		}

		return nullptr;
	}

	void controller() {
		// log
		gLog().write(Log::INFO, std::string("[sharding]: starting..."));

		try {
			context_.run();
		} 
		catch(boost::system::system_error& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[sharding]: context error -> ") + ex.what());
		}

		// log
		gLog().write(Log::INFO, std::string("[sharding]: stopped"));
	}

	void touch(const boost::system::error_code& error) {
		// life control
		if(!error && !settings_->qbitOnly()) {
			//
			size_t lActiveShardsCount = 0;
			std::map<uint256/*shard-chain*/, EntityPtr> lPendingShards;
			{
				boost::unique_lock<boost::recursive_mutex> lLock(shardsMutex_);
				lPendingShards = pendingShards_;
				lActiveShardsCount = activeShards_.size();
			}
			//
			if (gLog().isEnabled(Log::SHARDING)) gLog().write(Log::SHARDING, std::string("[touch]: looking for pending shards..."));
			for (std::map<uint256/*shard-chain*/, EntityPtr>::iterator lShard = lPendingShards.begin(); lShard != lPendingShards.end(); lShard++) {
				uint64_t lHeight;
				uint64_t lConfirms;
				bool lCoinbase;
				if (gLog().isEnabled(Log::SHARDING)) gLog().write(Log::SHARDING, std::string("[touch]: try to start ") + strprintf("%s", lShard->first.toHex()));
				if (storeManager_->locate(MainChain::id())->transactionHeight(lShard->first, lHeight, lConfirms, lCoinbase)) {
					//
					if (lConfirms > consensusManager_->locate(MainChain::id())->maturity()) {
						// DApp can contains several shards (or sub-chains); minumumDAppNodes - number of nodes, that supports given shard
						// if this number less than minumumDAppNodes - setup shard
						// or this node is full node
						TxDAppPtr lDApp = TransactionHelper::to<TxDApp>(lShard->second);
						if (settings_->isFullNode() || (
								settings_->isNode() && (
									lDApp->sharding() == TxDApp::Sharding::STATIC || (
										consensusManager_->chainSupportPeersCount(lShard->first) < settings_->minDAppNodes() && 
										lActiveShardsCount < settings_->maxShardsByNode())))) {
							uint256 lChain = lShard->first;
							//
							{
								boost::unique_lock<boost::recursive_mutex> lLock(shardsMutex_);
								activeShards_.insert(std::map<uint256, EntityPtr>::value_type(lShard->first, lShard->second));
								pendingShards_.erase(lShard->first);
							}

							gLog().write(Log::SHARDING, std::string("[touch]: starting shard ") + strprintf("%s", lShard->first.toHex()));

							ITransactionStorePtr lStore = storeManager_->push(lChain);
							TransactionStoreExtensionCreatorPtr lCreator = locateStoreExtension(lDApp->entityName());
							if (lCreator) lStore->setExtension(lCreator->create(settings_, lStore));

							IConsensusPtr lConsensus = consensusManager_->push(lChain, lDApp);
							if (lConsensus) lConsensus->setDApp(lDApp->entityName());
							mempoolManager_->push(lChain);
							validatorManager_->push(lChain, lDApp);

							//
							StatePtr lCurrentState = consensusManager_->currentState();
							consensusManager_->broadcastState(lCurrentState, lCurrentState->addressId());
						}
					}
				}
			}
		}

		timer_->expires_at(timer_->expiry() + boost::asio::chrono::milliseconds(2000));
		timer_->async_wait(boost::bind(&ShardingManager::touch, shared_from_this(), boost::asio::placeholders::error));
	}

private:
	// controller
	boost::shared_ptr<boost::thread> controller_;
	// context
	boost::asio::io_context context_;
	// signals
	boost::asio::signal_set signals_;
	// timer
	typedef std::shared_ptr<boost::asio::steady_timer> TimerPtr;
	TimerPtr timer_;
	// pending shards
	std::map<uint256/*shard-chain*/, EntityPtr> pendingShards_;
	// active shards
	std::map<uint256/*shard-chain*/, EntityPtr> activeShards_;
	// shards mutex
	boost::recursive_mutex shardsMutex_;

	// settings
	ISettingsPtr settings_;
	
	// managers
	IConsensusManagerPtr consensusManager_;
	ITransactionStoreManagerPtr storeManager_;
	IValidatorManagerPtr validatorManager_;
	IMemoryPoolManagerPtr mempoolManager_;
};

} // qbit

#endif
