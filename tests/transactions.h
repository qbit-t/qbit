// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_TRANSACTIONS_H
#define QBIT_UNITTEST_TRANSACTIONS_H

#include <string>
#include <list>

#include "unittest.h"
#include "../itransactionstore.h"
#include "../iwallet.h"
#include "../ientitystore.h"
#include "../transactionvalidator.h"
#include "../transactionactions.h"
#include "../block.h"

namespace qbit {
namespace tests {

class TxWallet: public IWallet {
public:
	TxWallet() {}

	SKey createKey(const std::list<std::string>& seed) {
		SKey lSKey(seed);
		lSKey.create();

		PKey lPKey = lSKey.createPKey();
		keys_[lPKey.id()] = lSKey;

		return lSKey;
	}

	SKey findKey(const PKey& pkey) {
		if (pkey.valid()) {
			std::map<uint160, SKey>::iterator lKey = keys_.find(pkey.id());
			if (lKey != keys_.end()) {
				return lKey->second;
			}
		}

		return SKey();
	}

	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr out, TransactionContextPtr ctx) {
		utxo_[out->hash()] = out;
		return true;
	}
	
	bool popUnlinkedOut(const uint256& hash, TransactionContextPtr ctx) {
		if (utxo_.find(hash) != utxo_.end()) {
			utxo_.erase(hash);
			return true;
		}
		return false;
	}

	Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256& hash) {
		std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lIter = utxo_.find(hash);
		if (lIter != utxo_.end()) {
			return lIter->second;
		}

		return nullptr;
	}

	amount_t balance() {
		// TODO: asset type matters: lOut.out().asset()
		amount_t lBalance = 0;
		for (std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lIter = utxo_.begin(); lIter != utxo_.end(); lIter++) {
			Transaction::UnlinkedOutPtr lOut = lIter->second;
			lBalance += lOut->amount();
		}

		return lBalance;
	}

	std::map<uint160, SKey> keys_;
	std::map<uint256, Transaction::UnlinkedOutPtr> utxo_;
};

class TxStore: public ITransactionStore {
public:
	TxStore() {}

	TransactionPtr locateTransaction(const uint256& tx) 
	{
		return txs_[tx]->tx(); 
	}

	TransactionContextPtr locateTransactionContext(const uint256& tx) {
		return txs_[tx]; 
	}	

	bool pushTransaction(TransactionContextPtr tx) {

		txs_[tx->tx()->hash()] = tx;
		return true;
	}

	TransactionContextPtr pushTransaction(TransactionPtr tx) {

		txs_[tx->hash()] = TransactionContext::instance(tx);
		return txs_[tx->hash()];
	}

	void addLink(const uint256& /*from*/, const uint256& /*to*/) { }

	BlockContextPtr pushBlock(BlockPtr block) {
		for(TransactionsContainer::iterator lIter = block->transactions().begin(); lIter != block->transactions().end(); lIter++) {
			txs_[(*lIter)->id()] = TransactionContext::instance(*lIter);
		}

		return BlockContext::instance(block);
	}	

	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr out, TransactionContextPtr) {
		utxo_[out->hash()] = out;
		return true;
	}
	
	bool popUnlinkedOut(const uint256& hash, TransactionContextPtr) {
		if (utxo_.find(hash) != utxo_.end()) {
			utxo_.erase(hash);
			return true;
		}
		return false;
	}

	Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256& hash) {
		std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lIter = utxo_.find(hash);
		if (lIter != utxo_.end()) {
			return lIter->second;
		}

		return nullptr;
	}

	std::map<uint256, TransactionContextPtr> txs_;
	std::map<uint256, Transaction::UnlinkedOutPtr> utxo_;
};

class EntityStore: public IEntityStore {
public:
	EntityStore() {}

	EntityPtr locateEntity(const uint256&) { return nullptr; }
	
	bool pushEntity(const uint256&, TransactionContextPtr wrapper) {

		return false;
	}
};

class TxVerify: public Unit {
public:
	TxVerify(): Unit("TxVerify") {
		store_ = std::make_shared<TxStore>(); 
		wallet_ = std::make_shared<TxWallet>(); 
		entityStore_ = std::make_shared<EntityStore>(); 
	}

	uint256 createTx0();
	TransactionPtr createTx1(uint256);

	bool execute();

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
};

class TxVerifyPrivate: public Unit {
public:
	TxVerifyPrivate(): Unit("TxVerifyPrivate") {
		store_ = std::make_shared<TxStore>(); 
		wallet_ = std::make_shared<TxWallet>();
		entityStore_ = std::make_shared<EntityStore>();		
	}

	uint256 createTx0();
	TransactionPtr createTx1(uint256);

	bool execute();

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
};

class TxVerifyFee: public Unit {
public:
	TxVerifyFee(): Unit("TxVerifyFee") {
		store_ = std::make_shared<TxStore>(); 
		wallet_ = std::make_shared<TxWallet>();
		entityStore_ = std::make_shared<EntityStore>();	
	}

	uint256 createTx0(BlockPtr);
	TransactionPtr createTx1(uint256, BlockPtr);

	bool execute();

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
};

class TxVerifyPrivateFee: public Unit {
public:
	TxVerifyPrivateFee(): Unit("TxVerifyPrivateFee") {
		store_ = std::make_shared<TxStore>(); 
		wallet_ = std::make_shared<TxWallet>();
		entityStore_ = std::make_shared<EntityStore>();
	}

	uint256 createTx0(BlockPtr);
	TransactionPtr createTx1(uint256, BlockPtr);

	bool execute();

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
};


}
}

#endif