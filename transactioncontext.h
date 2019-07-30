// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TRANSACTION_CONTEXT_H
#define QBIT_TRANSACTION_CONTEXT_H

#include "transaction.h"

namespace qbit {

class TransactionContext;
typedef std::shared_ptr<TransactionContext> TransactionContextPtr;

class TransactionContext {
public:
	typedef std::map<uint256, std::list<std::vector<unsigned char>>> _commitMap;
public:
	explicit TransactionContext() { fee_ = 0; }
	TransactionContext(TransactionPtr tx) : tx_(tx) { fee_ = 0; }

	inline TransactionPtr tx() { return tx_; }

	inline std::vector<PKey>& addresses() { return addresses_; }
	inline std::list<std::string>& errors() { return errors_; }
	inline void addAddress(const PKey& key) { addresses_.push_back(key); }
	inline void addError(const std::string& error) { errors_.push_back(error); }

	inline static TransactionContextPtr instance(TransactionPtr tx) { return std::make_shared<TransactionContext>(tx); }

	inline _commitMap& commitIn() { return commitIn_; }
	inline _commitMap& commitOut() { return commitOut_; }

	inline amount_t fee() { return fee_; }
	inline void addFee(amount_t fee) { fee_ += fee; }

private:
	TransactionPtr tx_;
	// in-addresses
	std::vector<PKey> addresses_;
	// errors
	std::list<std::string> errors_;
	// commit in group
	_commitMap commitIn_;
	// commit out group
	_commitMap commitOut_;
	// miner fee
	amount_t fee_;
};

} // qbit

#endif