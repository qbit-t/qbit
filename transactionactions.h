// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TRANSACTION_ACTIONS_H
#define QBIT_TRANSACTION_ACTIONS_H

#include "transactionvalidator.h"

namespace qbit {

class TxCoinBaseVerify: public TransactionAction {
public:
	TxCoinBaseVerify() {}
	static TransactionActionPtr instance() { return std::make_shared<TxCoinBaseVerify>(); }

	TransactionAction::Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr, IEntityStorePtr);
};

class TxSpendVerify: public TransactionAction {
public:
	TxSpendVerify() {}
	static TransactionActionPtr instance() { return std::make_shared<TxSpendVerify>(); }

	TransactionAction::Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr, IEntityStorePtr);
};

class TxSpendOutVerify: public TransactionAction {
public:
	TxSpendOutVerify() {}
	static TransactionActionPtr instance() { return std::make_shared<TxSpendOutVerify>(); }

	TransactionAction::Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr, IEntityStorePtr);
};

class TxBalanceVerify: public TransactionAction {
public:
	TxBalanceVerify() {}
	static TransactionActionPtr instance() { return std::make_shared<TxBalanceVerify>(); }

	TransactionAction::Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr, IEntityStorePtr);
};

class TxAssetTypeVerify: public TransactionAction {
public:
	TxAssetTypeVerify() {}
	static TransactionActionPtr instance() { return std::make_shared<TxAssetTypeVerify>(); }

	TransactionAction::Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr, IEntityStorePtr);
};

} // qbit

#endif