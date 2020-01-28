// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_WALLET_H
#define QBIT_WALLET_H

#include "iwallet.h"
#include "isettings.h"
#include "imemorypool.h"
#include "ientitystore.h"

#include "key.h"
#include "db/containers.h"

#include <sstream>

namespace qbit {

//
// Heavy Wallet implementation
class Wallet: public IWallet {
public:
	Wallet(ISettingsPtr settings) : 
		settings_(settings),
		keys_(settings_->dataPath() + "/wallet/keys"), 
		utxo_(settings_->dataPath() + "/wallet/utxo"), 
		ltxo_(settings_->dataPath() + "/wallet/ltxo"), 
		assets_(settings_->dataPath() + "/wallet/assets"), 
		entities_(settings_->dataPath() + "/wallet/entities")
	{
		opened_ = false;
		useUtxoCache_ = false;
	}

	Wallet(ISettingsPtr settings, IMemoryPoolPtr mempool, IEntityStorePtr entityStore) : 
		settings_(settings), mempool_(mempool), entityStore_(entityStore),
		keys_(settings_->dataPath() + "/wallet/keys"), 
		utxo_(settings_->dataPath() + "/wallet/utxo"), 
		ltxo_(settings_->dataPath() + "/wallet/ltxo"), 
		assets_(settings_->dataPath() + "/wallet/assets"), 
		entities_(settings_->dataPath() + "/wallet/entities")
	{
		opened_ = false;
		useUtxoCache_ = false;
	}

	static IWalletPtr instance(ISettingsPtr settings, IMemoryPoolPtr mempool, IEntityStorePtr entityStore) {
		return std::make_shared<Wallet>(settings, mempool, entityStore); 
	}

	static IWalletPtr instance(ISettingsPtr settings) {
		return std::make_shared<Wallet>(settings); 
	}

	void setTransactionStore(ITransactionStorePtr persistentStore) { persistentStore_ = persistentStore; }
	void setStoreManager(ITransactionStoreManagerPtr persistentStoreManager) { persistentStoreManager_ = persistentStoreManager; }
	void setMemoryPoolManager(IMemoryPoolManagerPtr mempoolManager) { mempoolManager_ = mempoolManager; }
	void setEntityStore(IEntityStorePtr entityStore) { entityStore_ = entityStore; }

	// key management
	SKey createKey(const std::list<std::string>&);
	SKey findKey(const PKey& pkey);
	SKey firstKey();
	SKey changeKey();

	// utxo management
	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr);
	bool popUnlinkedOut(const uint256&, TransactionContextPtr);
	Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&);
	Transaction::UnlinkedOutPtr findUnlinkedOutByAsset(const uint256&, amount_t);
	std::list<Transaction::UnlinkedOutPtr> collectUnlinkedOutsByAsset(const uint256&, amount_t);

	// rollback tx
	bool rollback(TransactionContextPtr);	

	// balance
	inline amount_t balance() {
		return balance(TxAssetType::qbitAsset());
	}
	amount_t balance(const uint256&);

	void resetCache() {
		boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);

		utxoCache_.clear();
		assetsCache_.clear();
		entitiesCache_.clear();
	}

	bool prepareCache();	

	// clean-up assets utxo
	void cleanUp();
	void cleanUpData(); 

	// dump utxo by asset
	void dumpUnlinkedOutByAsset(const uint256&, std::stringstream&);

	// wallet management
	bool open();
	bool close();
	bool isOpened() { return opened_; }

	ITransactionStorePtr persistentStore() {
		if (persistentStore_) return persistentStore_;
		return persistentStoreManager_->locate(MainChain::id());
	} // main

	// create coinbase tx
	TransactionContextPtr createTxCoinBase(amount_t /*amount*/);

	// create spend tx
	TransactionContextPtr createTxSpend(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/, qunit_t /*fee limit*/, int32_t targetBlock = -1);
	TransactionContextPtr createTxSpend(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/);

	// create spend private tx
	TransactionContextPtr createTxSpendPrivate(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/, qunit_t /*fee limit*/, int32_t targetBlock = -1);
	TransactionContextPtr createTxSpendPrivate(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/);

	// create asset type
	TransactionContextPtr createTxAssetType(const PKey& /*dest*/, const std::string& /*short name*/, const std::string& /*long name*/, amount_t /*supply*/, amount_t /*scale*/, TxAssetType::Emission /*emission*/, qunit_t /*fee limit*/, int32_t targetBlock = -1);
	TransactionContextPtr createTxAssetType(const PKey& /*dest*/, const std::string& /*short name*/, const std::string& /*long name*/, amount_t /*supply*/, amount_t /*scale*/, TxAssetType::Emission /*emission*/);
	// create chunked type (for example total supply 1000, 10 chunks = 10 outputs by 100)
	TransactionContextPtr createTxAssetType(const PKey& /*dest*/, const std::string& /*short name*/, const std::string& /*long name*/, amount_t /*supply*/, amount_t /*scale*/, int /*chunks*/, TxAssetType::Emission /*emission*/, qunit_t /*fee limit*/, int32_t targetBlock = -1);
	TransactionContextPtr createTxAssetType(const PKey& /*dest*/, const std::string& /*short name*/, const std::string& /*long name*/, amount_t /*supply*/, amount_t /*scale*/, int /*chunks*/, TxAssetType::Emission /*emission*/);

	// create asset emission
	TransactionContextPtr createTxLimitedAssetEmission(const PKey& /*dest*/, const uint256& /*asset*/, qunit_t /*fee limit*/, int32_t targetBlock = -1);
	TransactionContextPtr createTxLimitedAssetEmission(const PKey& /*dest*/, const uint256& /*asset*/);

	IMemoryPoolManagerPtr mempoolManager() { return mempoolManager_; }
	ITransactionStoreManagerPtr storeManager() { return persistentStoreManager_; }

	IMemoryPoolPtr mempool() {
		// TODO: chain id?
		if (mempoolManager_ == nullptr) { 
			return mempool_; 
		}
		return mempoolManager_->locate(MainChain::id());
	}

	bool isUnlinkedOutUsed(const uint256& utxo);
	bool isUnlinkedOutExists(const uint256& utxo);

private:
	amount_t fillInputs(TxSpendPtr /*tx*/, const uint256& /*asset*/, amount_t /*amount*/);
	TransactionContextPtr makeTxSpend(Transaction::Type /*type*/, const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/, qunit_t /*fee limit*/, int32_t /*targetBlock*/);
	Transaction::UnlinkedOutPtr findUnlinkedOutByEntity(const uint256& /*entity*/);

	inline void cacheUtxo(Transaction::UnlinkedOutPtr utxo) {
		if (useUtxoCache_) utxoCache_[utxo->hash()] = utxo;
	}

	inline void removeUtxo(const uint256& hash) {
		if (useUtxoCache_) utxoCache_.erase(hash);
	}

	bool isUnlinkedOutExistsGlobal(const uint256& utxo) {
		//
		if (settings_->isNode() || settings_->isFullNode()) {
			if (!mempool()->isUnlinkedOutExists(utxo)) {
				if (persistentStoreManager_) {
					return persistentStoreManager_->locate(MainChain::id())->isUnlinkedOutExists(utxo);
				}

				return persistentStore_->isUnlinkedOutExists(utxo);
			}
		}

		return true;
	}

private:
	// various settings, command line args & config file
	ISettingsPtr settings_;
	// memory pool
	IMemoryPoolManagerPtr mempoolManager_;
	// main pool
	IMemoryPoolPtr mempool_;
	// entity store
	IEntityStorePtr entityStore_;
	// store manager
	ITransactionStoreManagerPtr persistentStoreManager_;
	// main store
	ITransactionStorePtr persistentStore_;
	// wallet keys
	db::DbContainer<uint160 /*id*/, SKey> keys_;
	// unlinked outs
	db::DbContainer<uint256 /*utxo*/, Transaction::UnlinkedOut /*data*/> utxo_;
	// linked outs
	db::DbContainer<uint256 /*utxo*/, Transaction::UnlinkedOut /*data*/> ltxo_;
	// unlinked outs by asset
	db::DbMultiContainer<uint256 /*asset*/, uint256 /*utxo*/> assets_;
	// unlinked outs by entity
	db::DbMultiContainer<uint256 /*entity*/, uint256 /*utxo*/> entities_;

	// cache
	// utxo/data
	std::map<uint256 /*utxo*/, Transaction::UnlinkedOutPtr /*data*/> utxoCache_; // optionally
	// asset/utxo/data
	std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>> assetsCache_;

	// entity/utxo/data
	std::map<uint256 /*entity*/, std::map<uint256 /*utxo*/, Transaction::UnlinkedOutPtr /*data*/>> entitiesCache_;

	// flag
	bool opened_;

	// use utxo cache (in case of slow db functionality)
	bool useUtxoCache_;

	//
	boost::recursive_mutex cacheMutex_;
};

} // qbit

#endif
