// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TRANSACTION_STORE_MANAGER_H
#define QBIT_TRANSACTION_STORE_MANAGER_H

#include "itransactionstoremanager.h"
#include "transactionstore.h"
#include "iconsensusmanager.h"
#include "ishardingmanager.h"

namespace qbit {

class TransactionStoreManager: public ITransactionStoreManager, public std::enable_shared_from_this<TransactionStoreManager> {
public:
	TransactionStoreManager(ISettingsPtr settings) : settings_(settings) {}

	bool exists(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(storagesMutex_);
		return storages_.find(chain) != storages_.end();
	}

	ITransactionStorePtr locate(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(storagesMutex_);
		std::map<uint256, ITransactionStorePtr>::iterator lStore = storages_.find(chain);
		if (lStore != storages_.end()) return lStore->second;
		return nullptr;
	}

	bool add(ITransactionStorePtr store) {
		//
		boost::unique_lock<boost::mutex> lLock(storagesMutex_);
		if (storages_.find(store->chain()) == storages_.end()) {
			if (store->open()) {
				storages_[store->chain()] = store;

				return true;
			}			
		}

		return false;		
	}

	ITransactionStorePtr push(const uint256& chain) {
		//
		boost::unique_lock<boost::mutex> lLock(storagesMutex_);
		if (storages_.find(chain) == storages_.end()) {
			ITransactionStorePtr lStore = TransactionStore::instance(chain, settings_, shared_from_this());

			std::static_pointer_cast<TransactionStore>(lStore)->setWallet(wallet_);
			storages_[chain] = lStore;
			lStore->open();
			lStore->prepare();
			return lStore;
		}

		return nullptr;
	}

	ITransactionStorePtr create(const uint256& chain) {
		//
		boost::unique_lock<boost::mutex> lLock(storagesMutex_);
		if (storages_.find(chain) == storages_.end()) {
			ITransactionStorePtr lStore = TransactionStore::instance(chain, settings_, shared_from_this());

			std::static_pointer_cast<TransactionStore>(lStore)->setWallet(wallet_);
			storages_[chain] = lStore;
			lStore->open();
			return lStore;
		}

		return nullptr;
	}

	void pop(const uint256& chain) {
		//
		boost::unique_lock<boost::mutex> lLock(storagesMutex_);
		std::map<uint256, ITransactionStorePtr>::iterator lStore = storages_.find(chain);
		if (lStore != storages_.end()) {
			lStore->second->close();
			storages_.erase(chain);
		}
	}

	std::vector<ITransactionStorePtr> storages() {
		//
		boost::unique_lock<boost::mutex> lLock(storagesMutex_);
		std::vector<ITransactionStorePtr> lStores;

		for (std::map<uint256, ITransactionStorePtr>::iterator lStore = storages_.begin(); lStore != storages_.end(); lStore++) {
			if (lStore->second->isOpened()) lStores.push_back(lStore->second);
		}

		return lStores;
	}

	void stop() {
		//
		boost::unique_lock<boost::mutex> lLock(storagesMutex_);
		for (std::map<uint256, ITransactionStorePtr>::iterator lStore = storages_.begin(); lStore != storages_.end(); lStore++) {
			lStore->second->close();
		}
	}

	void pushChain(const uint256& chain, EntityPtr dapp) {
		//
		shardingManager_->push(chain, dapp);
	}

	static ITransactionStoreManagerPtr instance(ISettingsPtr settings) {
		return std::make_shared<TransactionStoreManager>(settings); 
	}

	void setWallet(IWalletPtr wallet) { wallet_ = wallet; }
	void setShardingManager(IShardingManagerPtr shardingManager) { shardingManager_ = shardingManager; }

	TransactionPtr locateTransaction(const uint256& tx, ITransactionStorePtr except) {
		return nullptr;
	}

private:
	boost::mutex storagesMutex_;
	std::map<uint256, ITransactionStorePtr> storages_;
	IWalletPtr wallet_;
	ISettingsPtr settings_;
	IShardingManagerPtr shardingManager_;
};

} // qbit

#endif
