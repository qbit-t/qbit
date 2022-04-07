// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IVALIDATOR_H
#define QBIT_IVALIDATOR_H

#include "block.h"
#include "imemorypool.h"

namespace qbit {

class IValidator {
public:
	enum BlockCheckResult {
		SUCCESS = 0,
		BROKEN_CHAIN = 1,
		ORIGIN_NOT_ALLOWED = 2,
		INTEGRITY_IS_INVALID = 3,
		ALREADY_PROCESSED = 4,
		VALIDATOR_ABSENT = 5,
		PEERS_IS_ABSENT = 6
	};

public:
	IValidator() {}

	virtual uint256 chain() { throw qbit::exception("NOT_IMPL", "IValidator::chain - not implemented."); }
	virtual void run() { throw qbit::exception("NOT_IMPL", "IValidator::run - not implemented."); }
	virtual void stop() { throw qbit::exception("NOT_IMPL", "IValidator::stop - not implemented."); }

	virtual BlockCheckResult checkBlockHeader(const NetworkBlockHeader& /*blockHeader*/) { throw qbit::exception("NOT_IMPL", "IValidator::checkBlockHeader - not implemented."); }
	virtual bool acceptBlockHeader(const NetworkBlockHeader& /*blockHeader*/) { throw qbit::exception("NOT_IMPL", "IValidator::acceptBlockHeader - not implemented."); }

	virtual IMemoryPoolPtr mempool() { throw qbit::exception("NOT_IMPL", "IValidator::mempool - not implemented."); }

	virtual bool reindexed() { throw qbit::exception("NOT_IMPL", "IValidator::reindexed - not implemented."); }
};

typedef std::shared_ptr<IValidator> IValidatorPtr;

//
class ValidatorCreator {
public:
	ValidatorCreator() {}
	virtual IValidatorPtr create(const uint256& /*chain*/, IConsensusPtr /*consensus*/, IMemoryPoolPtr /*mempool*/, ITransactionStorePtr /*store*/) { return nullptr; }
};

typedef std::shared_ptr<ValidatorCreator> ValidatorCreatorPtr;

//
typedef std::map<std::string/*dapp name*/, ValidatorCreatorPtr> Validators;
extern Validators gValidators; // TODO: init on startup

} // qbit

#endif
