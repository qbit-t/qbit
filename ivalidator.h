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
	IValidator() {}

	virtual uint256 chain() { throw qbit::exception("NOT_IMPL", "IValidator::chain - not implemented."); }
	virtual void run() { throw qbit::exception("NOT_IMPL", "IValidator::run - not implemented."); }
	virtual void stop() { throw qbit::exception("NOT_IMPL", "IValidator::stop - not implemented."); }

	virtual bool checkBlockHeader(const NetworkBlockHeader& /*blockHeader*/) { throw qbit::exception("NOT_IMPL", "IValidator::checkBlockHeader - not implemented."); }
	virtual bool acceptBlockHeader(const NetworkBlockHeader& /*blockHeader*/) { throw qbit::exception("NOT_IMPL", "IValidator::acceptBlockHeader - not implemented."); }

	virtual IMemoryPoolPtr mempool() { throw qbit::exception("NOT_IMPL", "IValidator::mempool - not implemented."); }
};

typedef std::shared_ptr<IValidator> IValidatorPtr;

} // qbit

#endif
