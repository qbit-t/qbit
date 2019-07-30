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

	uint256 pushUnlinkedOut(const Transaction::UnlinkedOut& out) {
		utxo_[const_cast<Transaction::UnlinkedOut&>(out).hash()] = out;
		return const_cast<Transaction::UnlinkedOut&>(out).hash();
	}
	
	bool popUnlinkedOut(const uint256& hash) {
		if (utxo_.find(hash) != utxo_.end()) {
			utxo_.erase(hash);
			return true;
		}
		return false;
	}

	bool findUnlinkedOut(const uint256& hash, Transaction::UnlinkedOut& out) {
		std::map<uint256, Transaction::UnlinkedOut>::iterator lIter = utxo_.find(hash);
		if (lIter != utxo_.end()) {
			out = lIter->second;
			return true;
		}

		return false;
	}

	amount_t balance() {
		// TODO: asset type matters: lOut.out().asset()
		amount_t lBalance = 0;
		for (std::map<uint256, Transaction::UnlinkedOut>::iterator lIter = utxo_.begin(); lIter != utxo_.end(); lIter++) {
			Transaction::UnlinkedOut& lOut = lIter->second;
			lBalance += lOut.amount();
		}

		return lBalance;
	}

	std::map<uint160, SKey> keys_;
	std::map<uint256, Transaction::UnlinkedOut> utxo_;
};

class TxStore: public ITransactionStore {
public:
	TxStore() {}

	TransactionPtr locateTransaction(const uint256& tx) 
	{
		return txs_[tx]; 
	}

	void pushTransaction(TransactionPtr tx) {

		txs_[tx->hash()] = tx;
	}

	void pushBlock(BlockPtr block) {
		for(TransactionsContainer::iterator lIter = block->transactions().begin(); lIter != block->transactions().end(); lIter++) {
			txs_[(*lIter)->id()] = *lIter;
		}
	}	

	uint256 pushUnlinkedOut(const Transaction::UnlinkedOut& out) {
		utxo_[const_cast<Transaction::UnlinkedOut&>(out).hash()] = out;
		return const_cast<Transaction::UnlinkedOut&>(out).hash();
	}
	
	bool popUnlinkedOut(const uint256& hash) {
		if (utxo_.find(hash) != utxo_.end()) {
			utxo_.erase(hash);
			return true;
		}
		return false;
	}

	bool findUnlinkedOut(const uint256& hash, Transaction::UnlinkedOut& out) {
		std::map<uint256, Transaction::UnlinkedOut>::iterator lIter = utxo_.find(hash);
		if (lIter != utxo_.end()) {
			out = lIter->second;
			return true;
		}

		return false;
	}

	std::map<uint256, TransactionPtr> txs_;
	std::map<uint256, Transaction::UnlinkedOut> utxo_;
};

class TxVerify: public Unit {
public:
	TxVerify(): Unit("TxVerify") {
		store_ = std::make_shared<TxStore>(); 
		wallet_ = std::make_shared<TxWallet>(); 
	}

	uint256 createTx0();
	TransactionPtr createTx1(uint256);

	bool execute();

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
};

class TxVerifyPrivate: public Unit {
public:
	TxVerifyPrivate(): Unit("TxVerifyPrivate") {
		store_ = std::make_shared<TxStore>(); 
		wallet_ = std::make_shared<TxWallet>(); 
	}

	uint256 createTx0();
	TransactionPtr createTx1(uint256);

	bool execute();

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
};

class TxVerifyFee: public Unit {
public:
	TxVerifyFee(): Unit("TxVerifyFee") {
		store_ = std::make_shared<TxStore>(); 
		wallet_ = std::make_shared<TxWallet>(); 
	}

	uint256 createTx0(BlockPtr);
	TransactionPtr createTx1(uint256, BlockPtr);

	bool execute();

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
};

class TxVerifyPrivateFee: public Unit {
public:
	TxVerifyPrivateFee(): Unit("TxVerifyPrivateFee") {
		store_ = std::make_shared<TxStore>(); 
		wallet_ = std::make_shared<TxWallet>(); 
	}

	uint256 createTx0(BlockPtr);
	TransactionPtr createTx1(uint256, BlockPtr);

	bool execute();

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
};


}
}

#endif