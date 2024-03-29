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

class TransactionProcessor;
typedef std::shared_ptr<TransactionProcessor> TransactionProcessorPtr;

class TransactionAction {
public:
	enum Result {
		NONE			= 0x00,
		GENERAL_ERROR	= 0x01,
		SUCCESS			= 0x02,
		CONTINUE		= 0x03
	};
public:
	TransactionAction() {}
	~TransactionAction() { processor_.reset(); }

	void setTransactionProcessor(TransactionProcessorPtr);

	virtual Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr, IEntityStorePtr) { return Result::GENERAL_ERROR; }

protected:
	TransactionProcessorPtr processor_;
};

typedef std::shared_ptr<TransactionAction> TransactionActionPtr;

//
typedef std::list<TransactionActionPtr> TransactionActions;
extern TransactionActions gTransactionActions; // TODO: init on startup

class TransactionProcessor: public std::enable_shared_from_this<TransactionProcessor> {
public:
	TransactionProcessor(ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) : 
		store_(store), wallet_(wallet), entityStore_(entityStore) {}

	void add(TransactionActionPtr action) { 
		actions_.push_back(action); 
	}

	bool process(TransactionContextPtr);

	TransactionProcessor& operator << (TransactionActionPtr action)
	{
		add(action);
		return *this;
	}

	static TransactionProcessor general(ITransactionStorePtr, IWalletPtr, IEntityStorePtr);
	static void registerTransactionAction(TransactionActionPtr);

private:
	ITransactionStorePtr store_;
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
	std::list<TransactionActionPtr> actions_;
};

} // qbit

#endif