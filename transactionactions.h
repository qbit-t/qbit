// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TRANSACTION_ACTIONS_H
#define QBIT_TRANSACTION_ACTIONS_H

#include "transactionvalidator.h"

namespace qbit {

class TxCoinBaseVerifyPush: public TransactionAction {
public:
	TxCoinBaseVerifyPush() {}
	static TransactionActionPtr instance() { return std::make_shared<TxCoinBaseVerifyPush>(); }

	TransactionAction::Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr);
};

class TxSpendVerify: public TransactionAction {
public:
	TxSpendVerify() {}
	static TransactionActionPtr instance() { return std::make_shared<TxSpendVerify>(); }

	TransactionAction::Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr);
};

class TxSpendOutVerify: public TransactionAction {
public:
	TxSpendOutVerify() {}
	static TransactionActionPtr instance() { return std::make_shared<TxSpendOutVerify>(); }

	TransactionAction::Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr);
};

class TxSpendBalanceVerify: public TransactionAction {
public:
	TxSpendBalanceVerify() {}
	static TransactionActionPtr instance() { return std::make_shared<TxSpendBalanceVerify>(); }

	TransactionAction::Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr);
};

} // qbit

#endif