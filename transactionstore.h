// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TRANSACTION_STORE_H
#define QBIT_TRANSACTION_STORE_H

#include "itransactionstore.h"
#include "ientitystore.h"
#include "isettings.h"
#include "iconsensus.h"
#include "iwallet.h"
#include "db/containers.h"

namespace qbit {

class TransactionStore: public ITransactionStore, public IEntityStore, public std::enable_shared_from_this<TransactionStore> {
public:
	class TxBlockAction {
	public:
		enum Type { PUSH, POP };
	public:
		TxBlockAction(TxBlockAction::Type action, const uint256& utxo, TransactionContextPtr ctx) :
			action_(action), utxo_(utxo), utxoPtr_(nullptr), ctx_(ctx) {}

		TxBlockAction(TxBlockAction::Type action, const uint256& utxo, Transaction::UnlinkedOutPtr utxoPtr, TransactionContextPtr ctx) :
			action_(action), utxo_(utxo), utxoPtr_(utxoPtr), ctx_(ctx) {}

		inline uint256& utxo() { return utxo_; }
		inline Transaction::UnlinkedOutPtr utxoPtr() { return utxoPtr_; }
		inline TxBlockAction::Type action() { return action_; }
		inline TransactionContextPtr ctx() { return ctx_; }

	private:
		 TxBlockAction::Type action_;
		 uint256 utxo_;
		 Transaction::UnlinkedOutPtr utxoPtr_;
		 TransactionContextPtr ctx_;
	}; 

	class TxBlockStore: public ITransactionStore {
	public:
		TxBlockStore(ITransactionStorePtr persistentStore) : persistentStore_(persistentStore) {}

		static ITransactionStorePtr instance(ITransactionStorePtr persistentStore) {
			return std::make_shared<TxBlockStore>(persistentStore);
		}

		inline bool pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr ctx) {
			uint256 lId = utxo->hash();
			if (utxo_.find(lId) == utxo_.end() && !persistentStore_->isUnlinkedOutUsed(lId)) {
				utxo_[lId] = utxo;
				actions_.push_back(TxBlockAction(TxBlockAction::PUSH, lId, utxo, ctx));
			}

			return false;
		}

		inline bool popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx) {
			if (utxo_.erase(utxo) || persistentStore_->findUnlinkedOut(utxo) != nullptr)
				actions_.push_back(TxBlockAction(TxBlockAction::POP, utxo, ctx));
				return true;

			return false;
		}
		
		inline void addLink(const uint256& /*from*/, const uint256& /*to*/) {}

		inline std::list<TxBlockAction>& actions() { return actions_; }

		inline bool pushTransaction(TransactionContextPtr ctx) {
			txs_[ctx->tx()->id()] = ctx;
		}

		inline TransactionPtr locateTransaction(const uint256& id) {
			std::map<uint256, TransactionContextPtr>::iterator lTx = txs_.find(id);
			
			if (lTx != txs_.end()) {
				return lTx->second->tx();
			} 

			return persistentStore_->locateTransaction(id);
		}

	private:
		ITransactionStorePtr persistentStore_;
		std::list<TxBlockAction> actions_;
		std::map<uint256, Transaction::UnlinkedOutPtr> utxo_;
		std::map<uint256, TransactionContextPtr> txs_;
	};
	typedef std::shared_ptr<TxBlockStore> TxBlockStorePtr;

	class TxWalletStore: public IWallet {
	public:
		TxWalletStore(IWalletPtr persistentWallet, ITransactionStorePtr persistentStore) : 
			persistentWallet_(persistentWallet),
			persistentStore_(persistentStore) {}

		static IWalletPtr instance(IWalletPtr persistentWallet, ITransactionStorePtr persistentStore) {
			return std::make_shared<TxWalletStore>(persistentWallet, persistentStore);
		}

		inline bool pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr ctx) {
			uint256 lId = utxo->hash();
			if (utxo_.find(lId) == utxo_.end() && !persistentStore_->isUnlinkedOutUsed(lId)) {
				utxo_[lId] = utxo;
				actions_.push_back(TxBlockAction(TxBlockAction::PUSH, lId, utxo, ctx));
			}

			return false;
		}

		inline bool popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx) {
			if (utxo_.erase(utxo) || persistentWallet_->findUnlinkedOut(utxo) != nullptr)
				actions_.push_back(TxBlockAction(TxBlockAction::POP, utxo, ctx));
				return true;

			return false;
		}
		
		inline void addLink(const uint256& /*from*/, const uint256& /*to*/) {}

		inline std::list<TxBlockAction>& actions() { return actions_; } 

		inline SKey findKey(const PKey& pkey) { return persistentWallet_->findKey(pkey); }

	private:
		IWalletPtr persistentWallet_;
		ITransactionStorePtr persistentStore_;
		std::list<TxBlockAction> actions_;
		std::map<uint256, Transaction::UnlinkedOutPtr> utxo_;
	};
	typedef std::shared_ptr<TxWalletStore> TxWalletStorePtr;

	class TxBlockIndex {
	public:
		TxBlockIndex() {}
		TxBlockIndex(const uint256& block, uint32_t index) : block_(block), index_(index) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(block_);
			READWRITE(index_);
		}

		inline uint256& block() { return block_; }
		inline uint32_t index() { return index_; }

	private:
		uint256 block_;
		uint32_t index_;
	};

public:
	TransactionStore(const uint256& chain, ISettingsPtr settings, IConsensusPtr consensus) : 
		chain_(chain),
		settings_(settings),
		consensus_(consensus),
		opened_(false),
		headers_(settings_->dataPath() + "/" + chain.toHex() + "/headers"), 
		transactions_(settings_->dataPath() + "/" + chain.toHex() + "/transactions"), 
		utxo_(settings_->dataPath() + "/" + chain.toHex() + "/utxo"), 
		ltxo_(settings_->dataPath() + "/" + chain.toHex() + "/ltxo"), 
		entities_(settings_->dataPath() + "/" + chain.toHex() + "/entities"), 
		timeIdx_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/time"), 
		transactionsIdx_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/transactions"), 
		addressAssetUtxoIdx_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/utxo"),
		blockUtxoIdx_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/blocks")
	{}

	// stubs
	bool pushTransaction(TransactionContextPtr) {}
	TransactionContextPtr pushTransaction(TransactionPtr) { return nullptr; }

	//
	// IEntityStore implementation
	EntityPtr locateEntity(const uint256&);
	bool pushEntity(const uint256&, TransactionContextPtr);

	//
	// ITransactionStore implementation
	TransactionContextPtr locateTransactionContext(const uint256&);
	TransactionPtr locateTransaction(const uint256&);
	void setWallet(IWalletPtr wallet) { wallet_ = wallet; }
	
	BlockContextPtr pushBlock(BlockPtr); // sync blocks
	bool commitBlock(BlockContextPtr); // from mempool->beginBlock()

	IEntityStorePtr entityStore() {
		return std::static_pointer_cast<IEntityStore>(shared_from_this());
	}

	// store management
	bool open();
	bool close();
	bool isOpened() { return opened_; }	

	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr);
	bool popUnlinkedOut(const uint256&, TransactionContextPtr);
	void addLink(const uint256& /*from*/, const uint256& /*to*/);
	bool isUnlinkedOutUsed(const uint256&);
	bool isUnlinkedOutExists(const uint256&);

	Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&);

	bool rollbackToHeight(size_t);
	bool resyncHeight();

	size_t currentHeight();
	BlockHeader currentBlockHeader();

	static ITransactionStorePtr instance(const uint256& chain, ISettingsPtr settings, IConsensusPtr consensus) {
		return std::make_shared<TransactionStore>(chain, settings, consensus); 
	}	

	static ITransactionStorePtr instance(ISettingsPtr settings, IConsensusPtr consensus) {
		return std::make_shared<TransactionStore>(MainChain::id(), settings, consensus); 
	}	

private:
	bool processBlock(BlockContextPtr, bool /*processWallet*/);

private:
	// chain id
	uint256 chain_;
	// various settings, command line args & config file
	ISettingsPtr settings_;
	// consensus settings and processing
	IConsensusPtr consensus_;
	// local wallet
	IWalletPtr wallet_;
	// flag
	bool opened_;

	//
	// main data
	//

	// block headers
	db::DbContainer<uint256 /*block*/, BlockHeader /*data*/> headers_;
	// block transactions
	db::DbEntityContainer<uint256 /*block*/, BlockTransactions /*data*/> transactions_;
	// unlinked outs
	db::DbContainer<uint256 /*utxo*/, Transaction::UnlinkedOut /*data*/> utxo_;	
	// linked outs
	db::DbContainer<uint256 /*utxo*/, Transaction::UnlinkedOut /*data*/> ltxo_;	
	// entities
	db::DbContainer<std::string /*short name*/, uint256 /*tx*/> entities_;	 // TODO: remove if tx does not exists

	//
	// indexes
	//

	// time-to-block
	db::DbContainer<uint64_t /*time*/, uint256 /*block*/> timeIdx_;
	// tx-to-block|index
	db::DbContainer<uint256 /*tx*/, TxBlockIndex /*block|index*/> transactionsIdx_;
	// address|asset|utxo -> tx
	db::DbThreeKeyContainer<uint160 /*address*/, uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/> addressAssetUtxoIdx_;
	// block | utxo
	db::DbMultiContainer<uint256 /*block*/, uint256 /*utxo*/> blockUtxoIdx_; // TODO: implement	

	//
	// cache
	//
	std::vector<uint256> height_;
};

} // qbit

#endif
