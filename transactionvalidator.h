// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ITRANSACTION_VALIDATOR_H
#define QBIT_ITRANSACTION_VALIDATOR_H

#include "transaction.h"
#include "itransactionstore.h"

namespace qbit {

class TransactionValidator {
public:
	TransactionValidator(ITransactionStorePtr store) : store_(store) {}

	bool validate(TransactionPtr);

private:
	ITransactionStorePtr store_;
};

} // qbit

#endif