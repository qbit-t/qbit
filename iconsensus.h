// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ICONSENSUS_H
#define QBIT_ICONSENSUS_H

#include "transaction.h"
#include "block.h"
#include "transactioncontext.h"

#include <memory>

namespace qbit {

class IConsensus {
public:
	IConsensus() {}

	virtual size_t getMaxBlockSize() { throw qbit::exception("NOT_IMPL", "IConsensus::getMaxBlockSize - not implemented."); }
};

typedef std::shared_ptr<IConsensus> IConsensusPtr;

} // qbit

#endif
