// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ITRANSACTION_VALIDATOR_H
#define QBIT_ITRANSACTION_VALIDATOR_H

#include "transaction.h"
#include "transactioncontext.h"
#include "itransactionstore.h"
#include "ientitystore.h"
#include "iwallet.h"
#include "txassettype.h"
#include <list>

namespace qbit {

class TransactionAction {
public:
	enum Result {
		NONE		= 0x00,
		ERROR		= 0x01,
		SUCCESS		= 0x02,
		CONTINUE	= 0x03
	};
public:
	TransactionAction() {}

	virtual Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr, IEntityStorePtr) { return Result::ERROR; }
};

typedef std::shared_ptr<TransactionAction> TransactionActionPtr;

class TransactionProcessor {
public:
	TransactionProcessor(ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) : 
		store_(store), wallet_(wallet), entityStore_(entityStore) {}

	void add(TransactionActionPtr action) { actions_.push_back(action); }
	bool process(TransactionContextPtr);

	TransactionProcessor& operator << (TransactionActionPtr action)
	{
		add(action);
		return *this;
	}	

private:
	ITransactionStorePtr store_;
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
	std::list<TransactionActionPtr> actions_;
};

} // qbit

#endif