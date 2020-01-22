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
#include "iwallet.h"

#include <memory>

namespace qbit {

class IMemoryPool {
public:
	IMemoryPool() {}

	virtual bool close() { throw qbit::exception("NOT_IMPL", "IMemoryPool::close - not implemented."); }
	virtual uint256 chain() { throw qbit::exception("NOT_IMPL", "IMemoryPool::chain - not implemented."); }	

	virtual qunit_t estimateFeeRateByLimit(TransactionContextPtr /* ctx */, qunit_t /* max qunit\byte */) { throw qbit::exception("NOT_IMPL", "IMemoryPool::estimateFeeRateByLimit - not implemented."); }
	virtual qunit_t estimateFeeRateByBlock(TransactionContextPtr /* ctx */, uint32_t /* target block */) { throw qbit::exception("NOT_IMPL", "IMemoryPool::estimateFeeRateByBlock - not implemented."); }

	virtual bool isUnlinkedOutUsed(const uint256&) { throw qbit::exception("NOT_IMPL", "IMemoryPool::isUnlinkedOutUsed - not implemented."); }
	virtual bool isUnlinkedOutExists(const uint256&) { throw qbit::exception("NOT_IMPL", "IMemoryPool::isUnlinkedOutExists - not implemented."); }

	virtual void setMainStore(ITransactionStorePtr /*store*/) { throw qbit::exception("NOT_IMPL", "IMemoryPool::setMainStore - not implemented."); }

	virtual ITransactionStorePtr persistentMainStore() { throw qbit::exception("NOT_IMPL", "IMemoryPool::persistentMainStore - not implemented."); }
	virtual ITransactionStorePtr persistentStore() { throw qbit::exception("NOT_IMPL", "IMemoryPool::persistentStore - not implemented."); }
	virtual TransactionContextPtr pushTransaction(TransactionPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::pushTransaction - not implemented."); }
	virtual IWalletPtr wallet() { throw qbit::exception("NOT_IMPL", "IMemoryPool::wallet - not implemented."); }

	virtual BlockContextPtr beginBlock(BlockPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::beginBlock - not implemented."); }
	virtual void commit(BlockContextPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::commit - not implemented."); }

	virtual void removeTransactions(BlockPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::removeTransactions - not implemented."); }
};

typedef std::shared_ptr<IMemoryPool> IMemoryPoolPtr;

} // qbit

#endif