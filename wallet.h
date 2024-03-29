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
		space_(std::make_shared<db::DbContainerSpace>(settings_->dataPath() + "/wallet/data")),
		keys_(settings_->dataPath() + "/wallet/keys"),
		utxo_("wallet_utxo", space_),
		ltxo_("wallet_ltxo", space_),
		assets_("wallet_assets", space_),
		assetEntities_("wallet_asset_entities", space_),
		pendingtxs_("wallet_pending_txs", space_),
		balance_("balance", space_),
		pendingUtxo_("pending_utxo", space_)
	{
		opened_ = false;
	}

	Wallet(ISettingsPtr settings, IMemoryPoolPtr mempool, IEntityStorePtr entityStore) : 
		settings_(settings), mempool_(mempool), entityStore_(entityStore),
		space_(std::make_shared<db::DbContainerSpace>(settings_->dataPath() + "/wallet/data")),
		keys_(settings_->dataPath() + "/wallet/keys"),
		utxo_("wallet_utxo", space_),
		ltxo_("wallet_ltxo", space_),
		assets_("wallet_assets", space_),
		assetEntities_("wallet_asset_entities", space_),
		pendingtxs_("wallet_pending_txs", space_),
		balance_("balance", space_),
		pendingUtxo_("pending_utxo", space_)
	{
		opened_ = false;
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
	SKeyPtr createKey(const std::list<std::string>&);
	SKeyPtr findKey(const PKey& pkey);
	SKeyPtr removeKey(const PKey& pkey);
	SKeyPtr firstKey();
	SKeyPtr changeKey();
	void removeAllKeys();
	void allKeys(std::list<SKeyPtr>& keys);

	// utxo management
	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr);
	bool popUnlinkedOut(const uint256&, TransactionContextPtr);
	bool tryRemoveUnlinkedOut(const uint256&);
	bool tryRevertUnlinkedOut(const uint256&);
	Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&);
	Transaction::UnlinkedOutPtr findLinkedOut(const uint256&);
	Transaction::UnlinkedOutPtr findUnlinkedOutByAsset(const uint256&, amount_t);
	void collectUnlinkedOutsByAsset(const uint256&, amount_t, bool, std::list<Transaction::UnlinkedOutPtr>&);

	// rollback tx
	bool rollback(TransactionContextPtr);

	// balance
	inline amount_t balance() {
		return balance(TxAssetType::qbitAsset(), false);
	}
	amount_t balance(const uint256&, bool /*rescan*/);

	inline amount_t pendingBalance() {
		return pendingBalance(TxAssetType::qbitAsset());
	}
	amount_t pendingBalance(const uint256&);
	void balance(const uint256& /*asset*/, amount_t& /*pending*/, amount_t& /*actual*/, bool /*rescan*/);

	void resetCache() {
	}

	bool prepareCache();	

	// clean-up assets utxo
	void cleanUp();
	void cleanUpData(); 

	// dump utxo by asset
	void dumpUnlinkedOutByAsset(const uint256&, std::stringstream&);

	// wallet management
	bool open(const std::string&);
	bool close();
	bool isOpened() { return opened_; }

	ITransactionStorePtr persistentStore() {
		if (persistentStore_) return persistentStore_;
		return persistentStoreManager_->locate(MainChain::id());
	} // main

	IEntityStorePtr entityStore() { return entityStore_; }

	// create coinbase tx
	TransactionContextPtr createTxCoinBase(amount_t /*amount*/);

	// create generic base tx
	TransactionContextPtr createTxBase(const uint256& /*chain*/, amount_t /*amount*/);

	// create generic base tx
	TransactionContextPtr createTxBlockBase(const BlockHeader& /*block header*/, amount_t /*amount*/);

	// create spend tx
	TransactionContextPtr createTxSpend(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/, qunit_t /*fee limit*/, int32_t targetBlock = -1);
	TransactionContextPtr createTxSpend(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/);
	TransactionContextPtr createTxSpend(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/, bool /*aggregate*/);

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

	// create dapp tx
	TransactionContextPtr createTxDApp(const PKey& /*dest*/, const std::string& /*short name*/, const std::string& /*long name*/, TxDApp::Sharding /*sharding*/, Transaction::Type /*instances*/);

	// create dapp tx
	TransactionContextPtr createTxShard(const PKey& /*dest*/, const std::string& /*dapp name*/, const std::string& /*short name*/, const std::string& /*long name*/);

	// create tx fee
	TransactionContextPtr createTxFee(const PKey& /*dest*/, amount_t /*amount*/);	
	TransactionContextPtr createTxFeeLockedChange(const PKey& /*dest*/, amount_t /*amount*/, amount_t /*locked*/, uint64_t /*height*/);

	//
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

	void removePendingTransaction(const uint256& tx) {
		pendingtxs_.remove(tx);
	}

	void collectPendingTransactions(std::list<TransactionPtr>& list) {
		//
		if (!opened_) return;
		//
		db::DbEntityContainerShared<uint256 /*tx*/, Transaction /*data*/>::Transaction lDbTx = pendingtxs_.transaction();
		for (db::DbEntityContainerShared<uint256 /*tx*/, Transaction /*data*/>::Iterator lTx = pendingtxs_.begin(); lTx.valid(); lTx++) {
			//
			TransactionPtr lTxPtr = *lTx;
			if (lTxPtr) {
				list.push_back(lTxPtr);
			} else {
				uint256 lId;
				if (lTx.first(lId)) {
					lDbTx.remove(lId);
				}
			}
		}

		lDbTx.commit();
	}

	void writePendingTransaction(const uint256& id, TransactionPtr tx) {
		if (!opened_) return;
		pendingtxs_.write(id, tx);
	}

	amount_t fillInputs(TxSpendPtr /*tx*/, const uint256& /*asset*/, amount_t /*amount*/, bool /*aggregate*/, std::list<Transaction::UnlinkedOutPtr>& /*utxos*/);
	TransactionContextPtr aggregateCoinbaseTxs();

private:
	TransactionContextPtr makeTxSpend(Transaction::Type /*type*/, const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/, qunit_t /*fee limit*/, int32_t /*targetBlock*/, bool /*aggregate*/);
	Transaction::UnlinkedOutPtr findUnlinkedOutByEntity(const uint256& /*entity*/);
	amount_t fillCoinbaseInputs(TxSpendPtr /*tx*/);
	void collectCoinbaseUnlinkedOuts(std::list<Transaction::UnlinkedOutPtr>& /*list*/);

	inline void cacheUtxo(Transaction::UnlinkedOutPtr utxo) {
	}

	inline void removeUtxo(const uint256& hash) {
	}

	bool isUnlinkedOutExistsGlobal(const uint256& hash, Transaction::UnlinkedOut& utxo) {
		//
		if (!mempool()->isUnlinkedOutExists(hash)) {
			if (persistentStoreManager_) {
				return persistentStoreManager_->locate(utxo.out().chain())->isUnlinkedOutExists(hash);
			}

			return persistentStore_->isUnlinkedOutExists(hash);
		}

		return true;
	}

	bool isLinkedOutExistsGlobal(const uint256& hash, Transaction::UnlinkedOut& utxo) {
		//
		if (!mempool()->isUnlinkedOutExists(hash)) {
			if (persistentStoreManager_) {
				return persistentStoreManager_->locate(utxo.out().chain())->isLinkedOutExists(hash);
			}

			return persistentStore_->isLinkedOutExists(hash);
		}

		return true;
	}

	void collectUnlinkedOutsByAssetReverse(const uint256&, amount_t, std::list<Transaction::UnlinkedOutPtr>&);
	void collectUnlinkedOutsByAssetForward(const uint256&, amount_t, std::list<Transaction::UnlinkedOutPtr>&);

	void checkPendingUtxo();

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
	db::DbContainer<uint160 /*id*/, SKey> keys_; // separated container

	//
	// container space
	//
	db::DbContainerSpacePtr space_;

	// unlinked outs
	db::DbContainerShared<uint256 /*utxo*/, Transaction::UnlinkedOut /*data*/> utxo_;
	// linked outs
	db::DbContainerShared<uint256 /*utxo*/, Transaction::UnlinkedOut /*data*/> ltxo_;
	// unlinked outs by asset
	db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/> assets_;
	// unlinked outs by asset entity
	db::DbTwoKeyContainerShared<uint256 /*entity*/, uint256 /*utxo*/, uint256 /*tx*/> assetEntities_;
	// pending tx
	db::DbEntityContainerShared<uint256 /*tx*/, Transaction /*data*/> pendingtxs_;
	// pre-calc
	db::DbContainerShared<uint256 /*asset*/, amount_t /*balance*/> balance_;
	// pending utxo's
	db::DbContainerShared<uint256 /*utxo*/, uint256 /*transaction*/> pendingUtxo_;

	// flag
	bool opened_;

	// cached keys
	// TODO: consider to REMOVE key caching due to possible excessive memory usage
	std::map<uint160 /*id*/, SKeyPtr> keysCache_;

	// lock
	boost::recursive_mutex cacheMutex_;
	boost::recursive_mutex keyMutex_;
};

} // qbit

#endif
