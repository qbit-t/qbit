// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_LIGHT_WALLET_H
#define QBIT_LIGHT_WALLET_H

#include "iwallet.h"
#include "isettings.h"

#include "key.h"
#include "db/containers.h"

#include "irequestprocessor.h"

#include <sstream>

namespace qbit {

//
// Light Wallet implementation, client side
class LightWallet: 
		public IWallet, 
		public ISelectUtxoByAddressHandler, 
		public ILoadTransactionHandler, 
		public std::enable_shared_from_this<LightWallet> {
public:
	class TxEntityStore: public IEntityStore {
	public:
		TxEntityStore() {}

		static IEntityStorePtr instance() {
			return std::make_shared<TxEntityStore>();
		}

		EntityPtr locateEntity(const uint256& entity) {
			return nullptr;
		}

		EntityPtr locateEntity(const std::string& name) {
			return nullptr;
		}

		bool pushEntity(const uint256& id, TransactionContextPtr ctx) {
			return true;
		}
	};
	typedef std::shared_ptr<TxEntityStore> TxEntityStorePtr;

	class TxBlockStore: public ITransactionStore {
	public:
		TxBlockStore() {}

		static ITransactionStorePtr instance() {
			return std::make_shared<TxBlockStore>();
		}

		inline bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) {
			return true;
		}

		inline bool popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx) {
			return true;
		}
		
		inline void addLink(const uint256& /*from*/, const uint256& /*to*/) {}

		inline bool pushTransaction(TransactionContextPtr ctx) {
			return true;
		}

		inline TransactionPtr locateTransaction(const uint256& id) {
			return nullptr;
		}

	};
	typedef std::shared_ptr<TxBlockStore> TxBlockStorePtr;

public:
	LightWallet(ISettingsPtr settings, IRequestProcessorPtr requestProcessor) : 
		settings_(settings),
		requestProcessor_(requestProcessor),
		keys_(settings_->dataPath() + "/wallet/keys"), 
		utxo_(settings_->dataPath() + "/wallet/utxo") {
		opened_ = false;
	}

	static IWalletPtr instance(ISettingsPtr settings, IRequestProcessorPtr requestProcessor) {
		return std::make_shared<LightWallet>(settings, requestProcessor); 
	}

	// key management
	SKey createKey(const std::list<std::string>&);
	SKey findKey(const PKey& pkey);
	SKey firstKey();
	SKey changeKey();

	// utxo management
	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr);
	bool popUnlinkedOut(const uint256&, TransactionContextPtr);
	void collectUnlinkedOutsByAsset(const uint256&, amount_t, std::list<Transaction::UnlinkedOutPtr>&);

	// 
	TransactionContextPtr processTransaction(TransactionPtr);

	//
	// handlers
	//

	// ISelectUtxoByAddressHandler
	void handleReply(const std::vector<Transaction::NetworkUnlinkedOut>&, const PKey&);
	// ILoadTransactionHandler
	void handleReply(TransactionPtr);
	// IReplyHandler
	void timeout() {
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[timeout]: request timeout"));

		status_ = IWallet::UNKNOWN;
		prepareCache();
	}

	// rollback tx
	bool rollback(TransactionContextPtr);

	// balance
	inline amount_t balance() {
		return balance(TxAssetType::qbitAsset());
	}
	amount_t balance(const uint256&);

	void resetCache() {
		boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
		assetsCache_.clear();
	}

	bool prepareCache();
	IWallet::Status status() { return status_; }

	void refetchTransactions();

	// clean-up assets utxo
	void cleanUp();
	void cleanUpData(); 

	// wallet management
	bool open();
	bool close();
	bool isOpened() { return opened_; }

	// create spend tx
	TransactionContextPtr createTxSpend(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/, qunit_t /*fee limit*/);
	TransactionContextPtr createTxSpend(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/);

	// create spend private tx
	TransactionContextPtr createTxSpendPrivate(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/, qunit_t /*fee limit*/);
	TransactionContextPtr createTxSpendPrivate(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/);

	// create tx fee
	TransactionContextPtr createTxFee(const PKey& /*dest*/, amount_t /*amount*/);	

	amount_t fillInputs(TxSpendPtr /*tx*/, const uint256& /*asset*/, amount_t /*amount*/, std::list<Transaction::UnlinkedOutPtr>& /*utxos*/);
	void removeUnlinkedOut(std::list<Transaction::UnlinkedOutPtr>&);
	void cacheUnlinkedOut(Transaction::UnlinkedOutPtr);

private:
	TransactionContextPtr makeTxSpend(Transaction::Type /*type*/, const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/, qunit_t /*fee limit*/);
	Transaction::NetworkUnlinkedOutPtr findNetworkUnlinkedOut(const uint256& /*out*/);
	void removeFromAssetsCache(Transaction::UnlinkedOutPtr);

private:
	// various settings, command line args & config file
	ISettingsPtr settings_;
	// Peer manager (facade)
	IRequestProcessorPtr requestProcessor_;
	// wallet keys
	db::DbContainer<uint160 /*id*/, SKey> keys_;
	// unlinked outs
	db::DbContainer<uint256 /*utxo*/, Transaction::NetworkUnlinkedOut /*data*/> utxo_;

	// cache
	// asset/utxo/data
	std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>> assetsCache_;
	//
	std::map<uint256 /*id*/, Transaction::NetworkUnlinkedOutPtr /*utxo*/> utxoCache_;

	// flag
	bool opened_;

	// wallet status
	IWallet::Status status_;

	// lock
	boost::recursive_mutex cacheMutex_;
};

} // qbit

#endif