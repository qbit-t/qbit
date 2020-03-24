// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_MEMORY_POOL_H
#define QBIT_MEMORY_POOL_H

#include "iconsensus.h"
#include "imemorypool.h"
#include "itransactionstore.h"
#include "iwallet.h"
#include "ientitystore.h"

#include "log/log.h"

#include <memory>

namespace qbit {

class MemoryPool: public IMemoryPool, public INetworkBlockHandlerWithCoinBase, public std::enable_shared_from_this<MemoryPool> {
public:
	class TxTree {
	public:
		class TxInfo {
		public:
			TxInfo(int priority, TransactionContextPtr ctx) : priority_(priority), ctx_(ctx) {}

			inline TransactionContextPtr ctx() { return ctx_; }
			inline size_t priority() { return priority_; }
			inline void changePriority(int priority) { priority_ = priority; }

		private:
			TransactionContextPtr ctx_;
			size_t priority_;
		};

	public:
		TxTree() : size_(0) {}

		enum Direction {
			UP = -1,
			DOWN = 1,
			NEUTRAL = 0
		};

		inline int push(const uint256& context, TransactionContextPtr candidate, Direction direction) {
			//
			int lPriority = 0;
			std::map<uint256 /*tx*/, TxInfo>::iterator lContext = txs_.find(context);
			if (lContext != txs_.end()) {
				lPriority = lContext->second.priority(); // extract context node priority
			}

			if (direction == TxTree::NEUTRAL) neutral_ = candidate->tx()->id();
			lPriority += (int)direction;

			//
			std::map<uint256 /*tx*/, TxInfo>::iterator lInfo = txs_.find(candidate->tx()->id());
			if (lInfo != txs_.end()) {
				//
				if (lInfo->second.priority() != lPriority) {
					//
					std::pair<std::multimap<int, TransactionContextPtr>::iterator,
								std::multimap<int, TransactionContextPtr>::iterator> lRange = queue_.equal_range(lInfo->second.priority());
					for (std::multimap<int, TransactionContextPtr>::iterator lItem = lRange.first; lItem != lRange.second; lItem++) {
						//
						if (lItem->second->tx()->id() == candidate->tx()->id()) {
							queue_.erase(lItem);
							break; 
						}
					}

					queue_.insert(std::multimap<int /*priority*/, TransactionContextPtr>::value_type(lPriority, candidate));
				}
			} else {
				queue_.insert(std::multimap<int /*priority*/, TransactionContextPtr>::value_type(lPriority, candidate));
			}

			txs_.insert(std::map<uint256 /*tx*/, TxInfo>::value_type(candidate->tx()->id(), TxInfo(lPriority, candidate)));

			return lPriority;
		}

		inline void merge(const TxTree& other) {
			//
			if (!txs_.size()) {
				txs_.insert(const_cast<TxTree&>(other).transactions().begin(), const_cast<TxTree&>(other).transactions().end());
				queue_.insert(const_cast<TxTree&>(other).queue().begin(), const_cast<TxTree&>(other).queue().end());
			} else {
				//
				int lNeutralPriority = 0;
				std::map<uint256 /*tx*/, TxInfo>::iterator lNeutral = txs_.find(const_cast<TxTree&>(other).neutral());
				if (lNeutral != txs_.end()) {
					//
					lNeutralPriority = lNeutral->second.priority();
				}

				for (std::map<uint256 /*tx*/, TxInfo>::iterator lInfo = const_cast<TxTree&>(other).transactions().begin(); 
															lInfo != const_cast<TxTree&>(other).transactions().end(); lInfo++) {
					std::map<uint256 /*tx*/, TxInfo>::iterator lOurInfo = txs_.find(lInfo->first);
					if (lOurInfo == txs_.end()) {
						//
						txs_.insert(std::map<uint256 /*tx*/, TxInfo>::value_type(
							lInfo->first, 
							TxInfo(lInfo->second.priority() + lNeutralPriority, lInfo->second.ctx())));
						queue_.insert(std::multimap<int /*priority*/, TransactionContextPtr>::value_type(
							lInfo->second.priority() + lNeutralPriority, 
							lInfo->second.ctx()));
					} else if ((lOurInfo->second.priority() < 0 && lOurInfo->second.priority() > lNeutralPriority + lInfo->second.priority()) ||
								(lOurInfo->second.priority() > 0 && lOurInfo->second.priority() < lNeutralPriority + lInfo->second.priority())) {
						//
						std::pair<std::multimap<int, TransactionContextPtr>::iterator,
									std::multimap<int, TransactionContextPtr>::iterator> lRange = queue_.equal_range(lOurInfo->second.priority());
						for (std::multimap<int, TransactionContextPtr>::iterator lItem = lRange.first; lItem != lRange.second; lItem++) {
							//
							if (lItem->second->tx()->id() == lInfo->second.ctx()->tx()->id()) { queue_.erase(lItem); break; }
						}

						txs_.insert(std::map<uint256 /*tx*/, TxInfo>::value_type(
							lInfo->second.ctx()->tx()->id(), 
							TxInfo(lInfo->second.priority() + lNeutralPriority, lInfo->second.ctx())));
						queue_.insert(std::multimap<int /*priority*/, TransactionContextPtr>::value_type(
							lInfo->second.priority() + lNeutralPriority, 
							lInfo->second.ctx()));
					}
				}
			}
		}

		inline std::map<uint256 /*tx*/, TxInfo>& transactions() { return txs_; }
		inline std::multimap<int /*priority*/, TransactionContextPtr>& queue() { return queue_; }
		inline uint256 neutral() { return neutral_; }

		inline size_t size() { return size_; }

	private:
		size_t size_;
		std::map<uint256 /*tx*/, TxInfo> txs_;
		std::multimap<int /*priority*/, TransactionContextPtr> queue_;
		uint256 neutral_;
	};

	class PoolEntityStore: public IEntityStore {
	public:
		PoolEntityStore(IMemoryPoolPtr pool) : pool_(pool) {}

		EntityPtr locateEntity(const uint256& entity) {
			return pool_->entityStore()->locateEntity(entity);
		}

		EntityPtr locateEntity(const std::string& name) {
			return pool_->entityStore()->locateEntity(name);
		}

		bool pushEntity(const uint256& entity, TransactionContextPtr ctx) {
			return pool_->entityStore()->locateEntity(entity) == nullptr;
		}

		inline static IEntityStorePtr instance(IMemoryPoolPtr pool) {
			return std::make_shared<PoolEntityStore>(pool); 
		}		

	private:
		IMemoryPoolPtr pool_;
	};

	class PoolStore: public ITransactionStore {
	public:
		PoolStore(IMemoryPoolPtr pool) : pool_(pool) {}

		TransactionPtr locateTransaction(const uint256&);
		TransactionContextPtr locateTransactionContext(const uint256&);

		bool pushTransaction(TransactionContextPtr); // pool
		BlockContextPtr pushBlock(BlockPtr) { return nullptr; } // TODO: stub

		bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr);
		bool popUnlinkedOut(const uint256&, TransactionContextPtr);
		void addLink(const uint256& /*from*/, const uint256& /*to*/);

		Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&) {} // TODO: stub

		inline static ITransactionStorePtr instance(IMemoryPoolPtr pool) {
			return std::make_shared<PoolStore>(pool); 
		}

		std::multimap<uint256 /*from*/, uint256 /*to*/>& forward() { return forward_; }
		std::multimap<uint256 /*to*/, uint256 /*from*/>& reverse() { return reverse_; }

		void cleanUp(TransactionContextPtr);
		void remove(TransactionContextPtr);

		bool isUnlinkedOutUsed(const uint256& utxo) {
			return usedUtxo_.find(utxo) != usedUtxo_.end();
		}

		bool isUnlinkedOutExists(const uint256& utxo) {
			return freeUtxo_.find(utxo) != freeUtxo_.end() /*|| usedUtxo_.find(utxo) != usedUtxo_.end()*/;
		}

		bool transactionHeight(const uint256& tx, uint64_t& height, uint64_t& confirms, bool& coinbase) {
			if (tx_.find(tx) != tx_.end()) return false;

			if (!pool_->persistentStore()->transactionHeight(tx, height, confirms, coinbase)) {
				if (pool_->persistentMainStore()) return pool_->persistentStore()->transactionHeight(tx, height, confirms, coinbase);
				else return false;
			}
			
			return true;
		}

		static std::shared_ptr<PoolStore> toStore(ITransactionStorePtr store) { return std::static_pointer_cast<PoolStore>(store); }

		uint256 chain() { return pool_->chain(); }
		ITransactionStoreManagerPtr storeManager() { return pool_->wallet()->storeManager(); }		

	private:
		IMemoryPoolPtr pool_;

		// tx tree
		std::map<uint256 /*id*/, TransactionContextPtr /*tx*/> tx_;
		std::multimap<uint256 /*from*/, uint256 /*to*/> forward_;
		std::multimap<uint256 /*to*/, uint256 /*from*/> reverse_;

		// used utxo
		std::map<uint256 /*utxo*/, bool> usedUtxo_;
		// free utxo
		std::map<uint256 /*utxo*/, Transaction::UnlinkedOutPtr> freeUtxo_;
	};
	typedef std::shared_ptr<PoolStore> PoolStorePtr;

public:
	class ConfirmedBlockInfo {
	public:
		ConfirmedBlockInfo() {}
		ConfirmedBlockInfo(const NetworkBlockHeader& blockHeader, TransactionPtr base) : blockHeader_(blockHeader), base_(base) {}

		NetworkBlockHeader& blockHeader() { return blockHeader_; }
		TransactionPtr base() { return base_; }

	private:
		NetworkBlockHeader blockHeader_;
		TransactionPtr base_;
	};

public:
	MemoryPool(const uint256& chain, IConsensusPtr consensus, ITransactionStorePtr persistentStore, IEntityStorePtr entityStore) { 
		consensus_ = consensus;
		persistentStore_ = persistentStore; 
		entityStore_ = entityStore;
		chain_ = chain;

		poolStore_ = PoolStore::instance(std::shared_ptr<IMemoryPool>(this)); 
	}

	qunit_t estimateFeeRateByLimit(TransactionContextPtr /* ctx */, qunit_t /* max qunit\byte */);
	qunit_t estimateFeeRateByBlock(TransactionContextPtr /* ctx */, uint32_t /* target block */);

	virtual void setMainStore(ITransactionStorePtr store) { persistentMainStore_ = store; }

	inline IEntityStorePtr entityStore() { return entityStore_; }
	inline ITransactionStorePtr persistentMainStore() { return persistentMainStore_; }
	inline ITransactionStorePtr persistentStore() { return persistentStore_; }
	inline void setWallet(IWalletPtr wallet) { wallet_ = wallet; }

	inline bool isUnlinkedOutUsed(const uint256&);
	inline bool isUnlinkedOutExists(const uint256&);

	inline static IMemoryPoolPtr instance(IConsensusPtr consensus, ITransactionStorePtr persistentStore, IEntityStorePtr entityStore) {
		return std::make_shared<MemoryPool>(MainChain::id(), consensus, persistentStore, entityStore); 
	}

	inline static IMemoryPoolPtr instance(const uint256& chain, IConsensusPtr consensus, ITransactionStorePtr persistentStore, IEntityStorePtr entityStore) {
		return std::make_shared<MemoryPool>(chain, consensus, persistentStore, entityStore); 
	}

	void removeTransactions(BlockPtr);
	void removeTransaction(const uint256& tx) {
		boost::unique_lock<boost::recursive_mutex> lLock(mempoolMutex_);
		removeTx(tx);
	}

	uint256 chain() { return chain_; }	

	bool close() {
		consensus_.reset();
		persistentStore_.reset();
		persistentMainStore_.reset();
		poolStore_.reset();
		wallet_.reset();
		entityStore_.reset();

		return true;
	}

	//
	// compose block
	BlockContextPtr beginBlock(BlockPtr);

	//
	// commit changes to pool
	void commit(BlockContextPtr);

	//
	// main entry
	TransactionContextPtr pushTransaction(TransactionPtr);
	bool pushTransaction(TransactionContextPtr);
	bool isTransactionExists(const uint256&);
	TransactionPtr locateTransaction(const uint256&);
	TransactionContextPtr locateTransactionContext(const uint256&);
	bool popUnlinkedOut(const uint256&, TransactionContextPtr);

	//
	// wallet
	IWalletPtr wallet() { return wallet_; }

	//
	// consensus
	IConsensusPtr consensus() { return consensus_; }

	//
	// INetworkBlockHandlerWithCoinBase handler
	void handleReply(const NetworkBlockHeader& blockHeader, TransactionPtr base) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(confirmedBlocksMutex_);
		confirmedBlocks_.insert(std::map<uint256, ConfirmedBlockInfo>::value_type(
			const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().hash(), 
			ConfirmedBlockInfo(blockHeader, base)));
	}

	bool locateConfirmedBlock(const uint256& id, ConfirmedBlockInfo& block) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(confirmedBlocksMutex_);
		std::map<uint256, ConfirmedBlockInfo>::iterator lBlock = confirmedBlocks_.find(id);
		if (lBlock != confirmedBlocks_.end()) {
			block = lBlock->second; 
			return true; 
		}
		
		return false; 
	}

	void removeConfirmedBlock(const uint256& id) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(confirmedBlocksMutex_);
		confirmedBlocks_.erase(id);
	}

private:
	inline qunit_t getTop() {
		std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::reverse_iterator lBegin = map_.rbegin();
		if (lBegin != map_.rend()) {
			return lBegin->first; 
		}

		return QUNIT;
	}

	bool traverseLeft(TransactionContextPtr, TxTree&);
	bool traverseRight(TransactionContextPtr, TxTree&);

	void processLocalBlockBaseTx(const uint256& /*block*/, const uint256& /*chain*/);

	void removeTx(const uint256& tx) {
		PoolStorePtr lPoolStore = PoolStore::toStore(poolStore_);
		TransactionContextPtr lCtx = poolStore_->locateTransactionContext(tx);
		if (lCtx) {
			lPoolStore->remove(lCtx);
		}
	}

private:
	std::map<uint256, ConfirmedBlockInfo> confirmedBlocks_;
	boost::recursive_mutex confirmedBlocksMutex_;

private:
	IConsensusPtr consensus_;
	ITransactionStorePtr persistentMainStore_;
	ITransactionStorePtr persistentStore_;
	ITransactionStorePtr poolStore_;
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
	uint256 chain_;

	// weighted map
	std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/> map_;
	// qbit txs
	std::map<uint256, TransactionContextPtr> qbitTxs_;
	//
	boost::recursive_mutex mempoolMutex_;
};

} // qbit

#endif
