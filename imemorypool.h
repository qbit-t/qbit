// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IMEMORY_POOL_H
#define QBIT_IMEMORY_POOL_H

#include "transaction.h"
#include "block.h"
#include "blockcontext.h"
#include "transactioncontext.h"

#include "iconsensus.h"
#include "itransactionstore.h"

#include <memory>

namespace qbit {

class IMemoryPool {
public:
	IMemoryPool() {}

	virtual bool close() { throw qbit::exception("NOT_IMPL", "IMemoryPool::close - not implemented."); }

	virtual qunit_t estimateFeeRateByLimit(TransactionContextPtr /* ctx */, qunit_t /* max qunit\byte */) { throw qbit::exception("NOT_IMPL", "IMemoryPool::estimateFeeRateByLimit - not implemented."); }
	virtual qunit_t estimateFeeRateByBlock(TransactionContextPtr /* ctx */, uint32_t /* target block */) { throw qbit::exception("NOT_IMPL", "IMemoryPool::estimateFeeRateByBlock - not implemented."); }

	virtual ITransactionStorePtr persistentStore() { throw qbit::exception("NOT_IMPL", "IMemoryPool::persistentStore - not implemented."); }
	virtual TransactionContextPtr pushTransaction(TransactionPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::pushTransaction - not implemented."); }

	virtual BlockContextPtr beginBlock(BlockPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::beginBlock - not implemented."); }
	virtual void commit(BlockContextPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::commit - not implemented."); }
};

typedef std::shared_ptr<IMemoryPool> IMemoryPoolPtr;

} // qbit

#endif
