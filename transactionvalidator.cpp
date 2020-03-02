#include "transactionvalidator.h"
#include "transactionactions.h"
#include "vm/vm.h"
#include "log/log.h"

#include <iostream>

using namespace qbit;

qbit::TransactionActions qbit::gTransactionActions;

void TransactionProcessor::registerTransactionAction(TransactionActionPtr action) {
	qbit::gTransactionActions.push_back(action);
}

bool TransactionProcessor::process(TransactionContextPtr tx) {
	try {
		TransactionAction::Result lResult = TransactionAction::CONTINUE;
		for(std::list<TransactionActionPtr>::iterator lAction = actions_.begin(); 
			lResult == TransactionAction::CONTINUE && lAction != actions_.end(); ++lAction) {
			lResult = (*lAction)->execute(tx, store_, wallet_, entityStore_);
		}

		if (lResult == TransactionAction::SUCCESS)
			return true;
		else {
			if (!tx->errors().size()) {
				tx->addError("Transaction processing failed.");
			}
		}

		return false;
	}
	catch(qbit::exception& ex) {
		gLog().write(Log::ERROR, std::string("[TransactionProcessor]: ") + ex.code() + " | " + ex.what());
		tx->addError(ex.code() + "|" + ex.what());
	}
	catch(std::exception& ex) {
		gLog().write(Log::ERROR, std::string("[TransactionProcessor]: ") + ex.what());
		tx->addError(ex.what());
	}

	return false;
}

TransactionProcessor TransactionProcessor::general(ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	// compose pipeline
	TransactionProcessor lProcessor(store, wallet, entityStore);
	lProcessor << 
		TxCoinBaseVerify::instance() 	<< 
		TxSpendVerify::instance() 		<< 
		TxSpendOutVerify::instance() 	<< TxAssetTypeVerify::instance(); // or

	// push additional actions
	for (TransactionActions::iterator lAction = gTransactionActions.begin(); lAction != gTransactionActions.end(); lAction++) {
		lProcessor << *lAction;
	}

	lProcessor << TxBalanceVerify::instance(); // finish

	return lProcessor;
}
