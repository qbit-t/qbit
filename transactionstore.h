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

	class TxEntityStore: public IEntityStore {
	public:
		TxEntityStore(IEntityStorePtr persistentEntityStore, ITransactionStorePtr persistentStore) : 
			persistentEntityStore_(persistentEntityStore), persistentStore_(persistentStore) {}

		static IEntityStorePtr instance(IEntityStorePtr persistentEntityStore, ITransactionStorePtr persistentStore) {
			return std::make_shared<TxEntityStore>(persistentEntityStore, persistentStore);
		}

		EntityPtr locateEntity(const uint256& entity) {
			//
			std::map<uint256, TransactionContextPtr>::iterator lEntity = entities_.find(entity);
			if (lEntity != entities_.end()) {
				return TransactionHelper::to<Entity>((lEntity->second)->tx());
			}

			return persistentEntityStore_->locateEntity(entity);
		}

		EntityPtr locateEntity(const std::string& name) {
			//
			std::map<std::string, uint256>::iterator lEntity = entityByName_.find(name);
			if (lEntity != entityByName_.end()) {
				return locateEntity(lEntity->second);
			}

			return nullptr;
		}

		bool pushEntity(const uint256& id, TransactionContextPtr ctx) {
			//
			if (ctx->tx()->isEntity() && ctx->tx()->entityName() == Entity::emptyName()) return true;

			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxEntityStore::pushEntity]: try to push entity ") +
				strprintf("'%s'/%s", ctx->tx()->entityName(), id.toHex()));

			if (persistentEntityStore_->locateEntity(id)) { 
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxEntityStore::pushEntity]: entity ALREADY EXISTS - ") +
					strprintf("'%s'/%s", ctx->tx()->entityName(), id.toHex()));
				return false;
			}

			if (entities_.find(id) == entities_.end()) {
				entities_.insert(std::map<uint256, TransactionContextPtr>::value_type(id, ctx));
				entityByName_.insert(std::map<std::string, uint256>::value_type(ctx->tx()->entityName(), ctx->tx()->id()));
				pushEntities_.push_back(ctx);
			}

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxEntityStore::pushEntity]: PUSHED entity ") +
				strprintf("'%s'/%s", ctx->tx()->entityName(), id.toHex()));

			return true;
		}

		inline std::list<TransactionContextPtr>& actions() { return pushEntities_; }		

	private:
		IEntityStorePtr persistentEntityStore_;
		ITransactionStorePtr persistentStore_;
		std::list<TransactionContextPtr> pushEntities_;
		std::map<std::string, uint256> entityByName_;
		std::map<uint256, TransactionContextPtr> entities_;
	};
	typedef std::shared_ptr<TxEntityStore> TxEntityStorePtr;

	class TxBlockStore: public ITransactionStore {
	public:
		TxBlockStore(ITransactionStorePtr persistentStore) : persistentStore_(persistentStore) {}

		static ITransactionStorePtr instance(ITransactionStorePtr persistentStore) {
			return std::make_shared<TxBlockStore>(persistentStore);
		}

		inline bool pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr ctx) {
			uint256 lId = utxo->hash();

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxBlockStore::pushUnlinkedOut]: try to push ") +
				strprintf("utxo = %s, tx = %s, ctx = %s", 
					lId.toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex()));

			if (utxo_.find(lId) == utxo_.end() && !persistentStore_->isUnlinkedOutUsed(lId)) {
				utxo_[lId] = utxo;
				actions_.push_back(TxBlockAction(TxBlockAction::PUSH, lId, utxo, ctx));

				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxBlockStore::pushUnlinkedOut]: PUSHED ") +
					strprintf("utxo = %s, tx = %s, ctx = %s", 
						lId.toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex()));

				return true;
			}

			return false;
		}

		inline bool popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxBlockStore::popUnlinkedOut]: try to pop ") +
				strprintf("utxo = %s, tx = ?, ctx = %s", utxo.toHex(), ctx->tx()->hash().toHex()));

			// TODO: for debug reasons only - need optimization
			Transaction::UnlinkedOutPtr lUtxo;
			std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lUtxoPtr = utxo_.find(utxo);
			if (lUtxoPtr != utxo_.end()) lUtxo = lUtxoPtr->second;
			if (!lUtxo) lUtxo = persistentStore_->findUnlinkedOut(utxo);

			if (utxo_.erase(utxo) || lUtxo != nullptr) {
				actions_.push_back(TxBlockAction(TxBlockAction::POP, utxo, ctx));
	
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[TxBlockStore::popUnlinkedOut]: POPPED ") +
					strprintf("utxo = %s, tx = %s, ctx = %s", utxo.toHex(), lUtxo->out().tx().toHex(), ctx->tx()->id().toHex()));
	
				return true;
			} else if (!lUtxo && persistentStore_->chain() != MainChain::id()) {
				return persistentStore_->storeManager()->locate(MainChain::id())->findUnlinkedOut(utxo) != nullptr;
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

		uint256 chain() { return persistentStore_->chain(); }
		ITransactionStoreManagerPtr storeManager() { return persistentStore_->storeManager(); }

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
	TransactionStore(const uint256& chain, ISettingsPtr settings, ITransactionStoreManagerPtr storeManager) : 
		chain_(chain),
		settings_(settings),
		storeManager_(storeManager),
		opened_(false),
		workingSettings_(settings_->dataPath() + "/" + chain.toHex() + "/settings"),
		headers_(settings_->dataPath() + "/" + chain.toHex() + "/headers"), 
		transactions_(settings_->dataPath() + "/" + chain.toHex() + "/transactions"), 
		utxo_(settings_->dataPath() + "/" + chain.toHex() + "/utxo"), 
		ltxo_(settings_->dataPath() + "/" + chain.toHex() + "/ltxo"), 
		entities_(settings_->dataPath() + "/" + chain.toHex() + "/entities"), 
		transactionsIdx_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/transactions"), 
		addressAssetUtxoIdx_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/utxo"),
		blockUtxoIdx_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/block_utxo"),
		utxoBlock_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/utxo_block"),
		shards_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/shards"),
		entityUtxo_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/entity_utxo"),
		shardEntities_(settings_->dataPath() + "/" + chain.toHex() + "/indexes/shard_entities")
	{}

	// stub
	bool pushTransaction(TransactionContextPtr) {}
	TransactionContextPtr pushTransaction(TransactionPtr) { return nullptr; }

	//
	// IEntityStore implementation
	EntityPtr locateEntity(const uint256&);
	EntityPtr locateEntity(const std::string&);
	bool pushEntity(const uint256&, TransactionContextPtr);

	//
	// ITransactionStore implementation
	TransactionContextPtr locateTransactionContext(const uint256&);
	TransactionPtr locateTransaction(const uint256&);
	void setWallet(IWalletPtr wallet) { wallet_ = wallet; }
	
	BlockContextPtr pushBlock(BlockPtr); // sync blocks
	bool commitBlock(BlockContextPtr, uint64_t&); // from mempool->beginBlock()
	bool setLastBlock(const uint256& /*block*/);
	void saveBlock(BlockPtr); // just save block
	void saveBlockHeader(const BlockHeader& /*header*/);
	void reindexFull(const uint256&, IMemoryPoolPtr /*pool*/); // rescan and re-fill indexes and local wallet
	bool reindex(const uint256&, const uint256&, IMemoryPoolPtr /*pool*/); // rescan and re-fill indexes and local wallet

	IEntityStorePtr entityStore() {
		return std::static_pointer_cast<IEntityStore>(shared_from_this());
	}

	ITransactionStoreManagerPtr storeManager() { return storeManager_; }

	uint256 chain() { return chain_; }

	bool collectUtxoByEntityName(const std::string& /*name*/, std::vector<Transaction::UnlinkedOutPtr>& /*result*/);
	bool entityCountByShards(const std::string& /*name*/, std::map<uint32_t, uint256>& /*result*/);

	// store management
	bool open();
	bool close();
	bool isOpened() { return opened_; }	

	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr);
	bool popUnlinkedOut(const uint256&, TransactionContextPtr);
	void addLink(const uint256& /*from*/, const uint256& /*to*/);
	bool isUnlinkedOutUsed(const uint256&);
	bool isUnlinkedOutExists(const uint256&);
	bool isLinkedOutExists(const uint256&);

	bool enqueueBlock(const NetworkBlockHeader& /*block*/);
	void dequeueBlock(const uint256& /*block*/);
	bool firstEnqueuedBlock(NetworkBlockHeader& /*block*/);

	Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&);
	Transaction::UnlinkedOutPtr findLinkedOut(const uint256&);

	bool resyncHeight();

	void erase(const uint256& /*from*/, const uint256& /*to*/); // erase indexes, reverse order (native)
	void remove(const uint256& /*from*/, const uint256& /*to*/); // remove indexes with data (headers, transactions), reverse order (native)
	bool processBlocks(const uint256& /*from*/, const uint256& /*to*/, std::list<BlockContextPtr>& /*ctxs*/);	// re\process blocks, forward order

	BlockPtr block(uint64_t /*height*/);
	BlockPtr block(const uint256& /*id*/);
	bool blockExists(const uint256& /*id*/);
	bool blockHeader(const uint256& /*id*/, BlockHeader& /*header*/);
	bool blockHeaderHeight(const uint256& /*block*/, uint64_t& /*height*/, BlockHeader& /*header*/);	

	uint64_t currentHeight(BlockHeader&);
	BlockHeader currentBlockHeader();

	bool transactionHeight(const uint256& /*tx*/, uint64_t& /*height*/, uint64_t& /*confirms*/, bool& /*coinbase*/);
	bool transactionInfo(const uint256& /*tx*/, uint256& /*block*/, uint64_t& /*height*/, uint64_t& /*confirms*/, uint32_t& /*index*/, bool& /*coinbase*/);	
	bool blockHeight(const uint256& /*block*/, uint64_t& /*height*/);

	static ITransactionStorePtr instance(const uint256& chain, ISettingsPtr settings, ITransactionStoreManagerPtr storeManager) {
		return std::make_shared<TransactionStore>(chain, settings, storeManager); 
	}	

	static ITransactionStorePtr instance(ISettingsPtr settings, ITransactionStoreManagerPtr storeManager) {
		return std::make_shared<TransactionStore>(MainChain::id(), settings, storeManager); 
	}	

private:
	bool processBlockTransactions(ITransactionStorePtr /*tempStore*/, IEntityStorePtr /*tempEntityStore*/, BlockContextPtr /*block*/, BlockTransactionsPtr /*transactions*/, uint64_t /*approxHeight*/, bool /*processWallet*/);
	bool processBlock(BlockContextPtr, uint64_t& /*new height*/, bool /*processWallet*/);
	void removeBlocks(const uint256& /*from*/, const uint256& /*to*/, bool /*removeData*/);
	void writeLastBlock(const uint256&);
	uint64_t pushNewHeight(const uint256&);
	uint64_t top();
	uint64_t calcHeight(const uint256&);

private:
	// chain id
	uint256 chain_;
	// various settings, command line args & config file
	ISettingsPtr settings_;
	// local wallet
	IWalletPtr wallet_;
	// store manager
	ITransactionStoreManagerPtr storeManager_;
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
	db::DbContainer<std::string /*short name*/, uint256 /*tx*/> entities_;
	// entities utxo
	db::DbMultiContainer<std::string /*short name*/, uint256 /*utxo*/> entityUtxo_;
	// shards by entity
	db::DbMultiContainer<uint256 /*entity*/, uint256 /*shard*/> shards_;

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
	// shard | entities count
	db::DbContainer<uint256 /*shard*/, uint32_t /*entities_count*/> shardEntities_;

	//
	boost::recursive_mutex storageCommitMutex_;

	//
	boost::recursive_mutex storageMutex_;
	std::map<uint64_t, uint256> heightMap_;
	std::map<uint256, uint64_t> blockMap_;

	//
	std::map<uint256, NetworkBlockHeader> blocksQueue_;
};

} // qbit

#endif
