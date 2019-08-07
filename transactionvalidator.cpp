#include "transactionvalidator.h"
#include "vm/vm.h"
#include "log/log.h"

#include <iostream>

using namespace qbit;

bool TransactionProcessor::process(TransactionContextPtr tx) {
	try {
		TransactionAction::Result lResult = TransactionAction::CONTINUE;
		for(std::list<TransactionActionPtr>::iterator lAction = actions_.begin(); 
			lResult == TransactionAction::CONTINUE && lAction != actions_.end(); ++lAction) {
			lResult = (*lAction)->execute(tx, store_, wallet_, entityStore_);
		}

		if (lResult == TransactionAction::SUCCESS)
			return true;
		return false;
	}
	catch(std::exception& ex) {
		gLog().write(Log::ERROR, std::string("[TransactionProcessor]: ") + ex.what());
		tx->addError(ex.what());
	}

	return false;
}
