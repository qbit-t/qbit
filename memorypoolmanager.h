// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_MEMORY_POOL_MANAGER_H
#define QBIT_MEMORY_POOL_MANAGER_H

#include "itransactionstoremanager.h"
#include "imemorypoolmanager.h"
#include "memorypool.h"
#include "iconsensusmanager.h"

namespace qbit {

class MemoryPoolManager: public IMemoryPoolManager, public std::enable_shared_from_this<MemoryPoolManager> {
public:
	MemoryPoolManager(ISettingsPtr settings) : settings_(settings) {}

	bool exists(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(poolsMutex_);
		return pools_.find(chain) != pools_.end();
	}
 
	IMemoryPoolPtr locate(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(poolsMutex_);
		std::map<uint256, IMemoryPoolPtr>::iterator lPool = pools_.find(chain);
		if (lPool != pools_.end()) return lPool->second;
		return nullptr;
	}

	bool add(IMemoryPoolPtr pool) {
		//
		boost::unique_lock<boost::mutex> lLock(poolsMutex_);
		if (pools_.find(pool->chain()) == pools_.end()) {
			if (pool->chain() != MainChain::id()) std::static_pointer_cast<MemoryPool>(pool)->setMainStore(storeManager_->locate(MainChain::id()));
			pools_[pool->chain()] = pool;
			return true;
		}

		return false;
	}

	void push(const uint256& chain) {
		//
		boost::unique_lock<boost::mutex> lLock(poolsMutex_);
		if (pools_.find(chain) == pools_.end()) {
			IMemoryPoolPtr lPool = MemoryPool::instance(chain, consensusManager_->locate(chain), storeManager_->locate(chain), entityStore_);
			std::static_pointer_cast<MemoryPool>(lPool)->setWallet(wallet_);
			if (chain != MainChain::id()) std::static_pointer_cast<MemoryPool>(lPool)->setMainStore(storeManager_->locate(MainChain::id()));
			pools_[chain] = lPool;
		}
	}

	void pop(const uint256& chain) {
		//
		boost::unique_lock<boost::mutex> lLock(poolsMutex_);
		std::map<uint256, IMemoryPoolPtr>::iterator lMempool = pools_.find(chain);
		if (lMempool != pools_.end()) {
			lMempool->second->close();
			pools_.erase(chain);
		}
	}

	std::vector<IMemoryPoolPtr> pools() {
		//
		boost::unique_lock<boost::mutex> lLock(poolsMutex_);
		std::vector<IMemoryPoolPtr> lPools;

		for (std::map<uint256, IMemoryPoolPtr>::iterator lPool = pools_.begin(); lPool != pools_.end(); lPool++) {
			lPools.push_back(lPool->second);
		}

		return lPools;
	}

	TransactionContextPtr locateTransactionContext(const uint256& tx) {
		//
		std::vector<IMemoryPoolPtr> lPools = pools();
		TransactionContextPtr lTx = nullptr;
		for (std::vector<IMemoryPoolPtr>::iterator lPool = lPools.begin(); lPool != lPools.end(); lPool++) {
			lTx = (*lPool)->locateTransactionContext(tx);
			if (lTx) break;
		}

		return lTx;
	}

	bool popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx, IMemoryPoolPtr except) {
		//
		std::vector<IMemoryPoolPtr> lPools = pools();
		bool lResult = false;
		for (std::vector<IMemoryPoolPtr>::iterator lPool = lPools.begin(); lPool != lPools.end(); lPool++) {
			if (except->chain() != (*lPool)->chain()) lResult |= (*lPool)->popUnlinkedOut(utxo, ctx);
		}

		return lResult;
	}

	static IMemoryPoolManagerPtr instance(ISettingsPtr settings) {
		return std::make_shared<MemoryPoolManager>(settings);
	}

	void setWallet(IWalletPtr wallet) { wallet_ = wallet; }
	void setStoreManager(ITransactionStoreManagerPtr storeManager) { storeManager_ = storeManager; }
	void setEntityStore(IEntityStorePtr entityStore) { entityStore_ = entityStore; }	
	void setConsensusManager(IConsensusManagerPtr consensusManager) { consensusManager_ = consensusManager; }	

private:
	boost::mutex poolsMutex_;
	std::map<uint256, IMemoryPoolPtr> pools_;
	IWalletPtr wallet_;
	IConsensusManagerPtr consensusManager_;
	IEntityStorePtr entityStore_;
	ITransactionStoreManagerPtr storeManager_;
	ISettingsPtr settings_;
};

} // qbit

#endif
