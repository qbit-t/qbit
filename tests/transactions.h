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
		return keys_[pkey.id()];
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


}
}

#endif