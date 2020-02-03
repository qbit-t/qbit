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
		enum Type { UNDEFINED = 0, PUSH = 1, POP = 2 };
	public:
		TxBlockAction() {}
		TxBlockAction(TxBlockAction::Type action, const uint256& utxo, TransactionContextPtr ctx) :
			action_(action), utxo_(utxo), utxoPtr_(nullptr), ctx_(ctx) {}

		TxBlockAction(TxBlockAction::Type action, const uint256& utxo, Transaction::UnlinkedOutPtr utxoPtr, TransactionContextPtr ctx) :
			action_(action), utxo_(utxo), utxoPtr_(utxoPtr), ctx_(ctx) {}

		inline uint256& utxo() { return utxo_; }
		inline Transaction::UnlinkedOutPtr utxoPtr() { return utxoPtr_; }
		inline TxBlockAction::Type action() { return action_; }
		inline TransactionContextPtr ctx() { return ctx_; }

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			if (ser_action.ForRead()) {
				char lAction;
				s >> lAction;
				s >> utxo_;
				action_ = (Type)lAction;
			} else {
				char lAction = (char)action_;
				s << lAction;
				s << utxo_;
			}
		}

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

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxBlockStore::pushUnlinkedOut]: try to push ") +
				strprintf("utxo = %s, tx = %s", 
					utxo->hash().toHex(), utxo->out().tx().toHex()));

			if (utxo_.find(lId) == utxo_.end() && !persistentStore_->isUnlinkedOutUsed(lId)) {
				utxo_[lId] = utxo;
				actions_.push_back(TxBlockAction(TxBlockAction::PUSH, lId, utxo, ctx));

				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxBlockStore::pushUnlinkedOut]: PUSHED ") +
					strprintf("utxo = %s, tx = %s", 
						utxo->hash().toHex(), utxo->out().tx().toHex()));

				return true;
			}

			return false;
		}

		inline bool popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxBlockStore::popUnlinkedOut]: try to pop ") +
				strprintf("utxo = %s, tx = %s",	utxo.toHex(), ctx->tx()->hash().toHex()));

			if (utxo_.erase(utxo) || persistentStore_->findUnlinkedOut(utxo) != nullptr /*|| persistentStore_->isUnlinkedOutUsed(utxo)*/) {
				actions_.push_back(TxBlockAction(TxBlockAction::POP, utxo, ctx));
	
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxBlockStore::popUnlinkedOut]: POPPED ") +
					strprintf("utxo = %s, tx = %s", 
						utxo.toHex(), ctx->tx()->hash().toHex()));
	
				return true;
			}

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
			//
			uint256 lId = utxo->hash();
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxWalletStore::pushUnlinkedOut]: try to push ") +
				strprintf("utxo = %s, tx = %s", 
					utxo->hash().toHex(), utxo->out().tx().toHex()));

			if (utxo_.find(lId) == utxo_.end() /*&& !persistentStore_->isUnlinkedOutUsed(lId)*/) {
				utxo_[lId] = utxo;
				actions_.push_back(TxBlockAction(TxBlockAction::PUSH, lId, utxo, ctx));

				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxWalletStore::pushUnlinkedOut]: PUSHED ") +
					strprintf("utxo = %s, tx = %s", 
						utxo->hash().toHex(), utxo->out().tx().toHex()));

				return true;
			}

			return false;
		}

		inline bool popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxWalletStore::popUnlinkedOut]: try to pop ") +
				strprintf("utxo = %s, tx = %s", 
					utxo.toHex(), ctx->tx()->hash().toHex()));

			// just add action - wallet will process correcly
			{
				actions_.push_back(TxBlockAction(TxBlockAction::POP, utxo, ctx));

				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxWalletStore::popUnlinkedOut]: POPPED ") +
					strprintf("utxo = %s, tx = %s", 
						utxo.toHex(), ctx->tx()->hash().toHex()));

				return true;
			}

			return false;
		}
		
		inline void addLink(const uint256& /*from*/, const uint256& /*to*/) {}

		inline std::list<TxBlockAction>& actions() { return actions_; } 

		inline SKey findKey(const PKey& pkey) { 
			return persistentWallet_->findKey(pkey); 
		}

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
	TransactionStore(const uint256& chain, ISettingsPtr settings) : 
		chain_(chain),
		settings_(settings),
		opened_(false),
		workingSettings_(settings_->dataPath() + "/" + chain.toHex() + "/settings"),
		headers_(settings_->dataPath() + "/" + chain.toHex() + "/headers"), 
		transactions_(settings_->dataPath() + "/" + chain.toHex() + "/transactions"), 
		utxo_(settings_->dataPath() + "/" + chain.toHex() + "/utxo"), 
		ltxo_(settings_->dataPath() + "/" + chain.toHex() + "/ltxo"), 
		entities_(settings_->dataPath() + "/" + chain.toHex() + "/entities"), 
		transactionsIdx_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/transactions"), 
		addressAssetUtxoIdx_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/utxo"),
		blockUtxoIdx_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/blockutxo"),
		utxoBlock_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/utxoblock")
	{}

	// stub
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
	bool commitBlock(BlockContextPtr, size_t&); // from mempool->beginBlock()
	bool setLastBlock(const uint256& /*block*/);
	void saveBlock(BlockPtr); // just save block
	void reindexFull(const uint256&, IMemoryPoolPtr /*pool*/); // rescan and re-fill indexes and local wallet
	bool reindex(const uint256&, const uint256&, IMemoryPoolPtr /*pool*/); // rescan and re-fill indexes and local wallet

	IEntityStorePtr entityStore() {
		return std::static_pointer_cast<IEntityStore>(shared_from_this());
	}

	uint256 chain() { return chain_; }	

	// store management
	bool open();
	bool close();
	bool isOpened() { return opened_; }	

	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr);
	bool popUnlinkedOut(const uint256&, TransactionContextPtr);
	void addLink(const uint256& /*from*/, const uint256& /*to*/);
	bool isUnlinkedOutUsed(const uint256&);
	bool isUnlinkedOutExists(const uint256&);

	bool enqueueBlock(const NetworkBlockHeader& /*block*/);
	void dequeueBlock(const uint256& /*block*/);
	bool firstEnqueuedBlock(NetworkBlockHeader& /*block*/);

	Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&);

	bool resyncHeight();

	void erase(const uint256& /*from*/, const uint256& /*to*/); // erase indexes, reverse order (native)
	void remove(const uint256& /*from*/, const uint256& /*to*/); // remove indexes with data (headers, transactions), reverse order (native)
	bool processBlocks(const uint256& /*from*/, const uint256& /*to*/, std::list<BlockContextPtr>& /*ctxs*/);	// re\process blocks, forward order

	BlockPtr block(size_t /*height*/);
	BlockPtr block(const uint256& /*id*/);
	bool blockExists(const uint256& /*id*/);

	size_t currentHeight(BlockHeader&);
	BlockHeader currentBlockHeader();

	bool transactionHeight(const uint256& /*tx*/, size_t& /*height*/, bool& /*coinbase*/);
	bool blockHeight(const uint256& /*block*/, size_t& /*height*/);

	static ITransactionStorePtr instance(const uint256& chain, ISettingsPtr settings) {
		return std::make_shared<TransactionStore>(chain, settings); 
	}	

	static ITransactionStorePtr instance(ISettingsPtr settings) {
		return std::make_shared<TransactionStore>(MainChain::id(), settings); 
	}	

private:
	bool processBlockTransactions(ITransactionStorePtr /*tempStore*/, BlockContextPtr /*block*/, BlockTransactionsPtr /*transactions*/, size_t /*approxHeight*/, bool /*processWallet*/);
	bool processBlock(BlockContextPtr, size_t& /*new height*/, bool /*processWallet*/);
	void removeBlocks(const uint256& /*from*/, const uint256& /*to*/, bool /*removeData*/);
	void writeLastBlock(const uint256&);
	size_t pushNewHeight(const uint256&);
	size_t top();
	size_t calcHeight(const uint256&);

private:
	// chain id
	uint256 chain_;
	// various settings, command line args & config file
	ISettingsPtr settings_;
	// local wallet
	IWalletPtr wallet_;
	// flag
	bool opened_;
	// last block
	uint256 lastBlock_;

	//
	// main data
	//

	// persistent settings
	db::DbContainer<std::string /*name*/, std::string /*data*/> workingSettings_;
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

	// tx-to-block|index
	db::DbContainer<uint256 /*tx*/, TxBlockIndex /*block|index*/> transactionsIdx_;
	// address|asset|utxo -> tx
	db::DbThreeKeyContainer<uint160 /*address*/, uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/> addressAssetUtxoIdx_;
	// block | utxo
	db::DbMultiContainer<uint256 /*block*/, TxBlockAction /*utxo action*/> blockUtxoIdx_;
	// utxo | block
	db::DbContainer<uint256 /*utxo*/, uint256 /*block*/> utxoBlock_;

	//
	boost::recursive_mutex storageCommitMutex_;

	//
	boost::recursive_mutex storageMutex_;
	std::map<size_t, uint256> heightMap_;
	std::map<uint256, size_t> blockMap_;

	//
	std::map<uint256, NetworkBlockHeader> blocksQueue_;
};

} // qbit

#endif
