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

// TODO: Concurrency!!
class MemoryPool: public IMemoryPool {
public:
	class TxTree {
	public:
		TxTree() : size_(0) {}

		inline std::map<uint256, TransactionContextPtr>& bundle() { return bundle_; }
		inline void add(TransactionContextPtr ctx) {
			if (bundle_.find(ctx->tx()->id()) == bundle_.end()) {
				bundle_[ctx->tx()->id()] = ctx; size_ += ctx->size(); 
			}
		}

		inline size_t size() { return size_; }

	private:
		size_t size_;
		std::map<uint256, TransactionContextPtr> bundle_;
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
		std::map<uint256 /*to*/, uint256 /*from*/>& reverse() { return reverse_; }

		void cleanUp(TransactionContextPtr);
		void remove(TransactionContextPtr);

		bool isUnlinkedOutUsed(const uint256& utxo) {
			return usedUtxo_.find(utxo) != usedUtxo_.end();
		}

		bool isUnlinkedOutExists(const uint256& utxo) {
			return freeUtxo_.find(utxo) != freeUtxo_.end() /*|| usedUtxo_.find(utxo) != usedUtxo_.end()*/;
		}

		static std::shared_ptr<PoolStore> toStore(ITransactionStorePtr store) { return std::static_pointer_cast<PoolStore>(store); }

	private:
		IMemoryPoolPtr pool_;

		// tx tree
		std::map<uint256 /*id*/, TransactionContextPtr /*tx*/> tx_;
		std::multimap<uint256 /*from*/, uint256 /*to*/> forward_;
		std::map<uint256 /*to*/, uint256 /*from*/> reverse_;

		// used utxo
		std::map<uint256 /*utxo*/, bool> usedUtxo_;
		// free utxo
		std::map<uint256 /*utxo*/, Transaction::UnlinkedOutPtr> freeUtxo_;
	};
	typedef std::shared_ptr<PoolStore> PoolStorePtr;

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

	uint256 chain() { return chain_; }	

	bool close() {
		consensus_.reset();
		persistentStore_.reset();
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

	//
	// wallet
	IWalletPtr wallet() { return wallet_; }

	//
	// consensus
	IConsensusPtr consensus() { return consensus_; }

private:
	inline qunit_t getTop() {
		std::multimap<qunit_t /*fee rate*/, uint256 /*tx*/>::reverse_iterator lBegin = map_.rbegin();
		if (lBegin != map_.rend()) {
			return lBegin->first; 
		}

		return QUNIT;
	}

	bool traverseLeft(TransactionContextPtr, const TxTree&);
	bool traverseRight(TransactionContextPtr, const TxTree&);

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
