// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IMEMORY_POOL_H
#define QBIT_IMEMORY_POOL_H

#include "transaction.h"
#include "block.h"
#include "transactioncontext.h"

#include <memory>

namespace qbit {

class IMemoryPool {
public:
	IMemoryPool() {}

	virtual qunit_t estimateFeeRateByLimit(TransactionContextPtr /* ctx */, qunit_t /* max qunit\byte */) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	virtual qunit_t estimateFeeRateByBlock(TransactionContextPtr /* ctx */, uint32_t /* target block */) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
};

typedef std::shared_ptr<IMemoryPool> IMemoryPoolPtr;

} // qbit

#endif
