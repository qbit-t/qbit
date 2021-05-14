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

class IWallet;
typedef std::shared_ptr<IWallet> IWalletPtr;

class IMemoryPool {
public:
	IMemoryPool() {}

	virtual bool close() { throw qbit::exception("NOT_IMPL", "IMemoryPool::close - not implemented."); }
	virtual uint256 chain() { throw qbit::exception("NOT_IMPL", "IMemoryPool::chain - not implemented."); }	

	virtual qunit_t estimateFeeRateByLimit(TransactionContextPtr /* ctx */, qunit_t /* max qunit\byte */) { throw qbit::exception("NOT_IMPL", "IMemoryPool::estimateFeeRateByLimit - not implemented."); }
	virtual qunit_t estimateFeeRateByBlock(TransactionContextPtr /* ctx */, uint32_t /* target block */) { throw qbit::exception("NOT_IMPL", "IMemoryPool::estimateFeeRateByBlock - not implemented."); }

	virtual bool isUnlinkedOutUsed(const uint256&) { throw qbit::exception("NOT_IMPL", "IMemoryPool::isUnlinkedOutUsed - not implemented."); }
	virtual bool isUnlinkedOutExists(const uint256&) { throw qbit::exception("NOT_IMPL", "IMemoryPool::isUnlinkedOutExists - not implemented."); }

	virtual bool isTransactionExists(const uint256&) { throw qbit::exception("NOT_IMPL", "IMemoryPool::isTransactionExists - not implemented."); }
	virtual TransactionPtr locateTransaction(const uint256&) { throw qbit::exception("NOT_IMPL", "IMemoryPool::locateTransaction - not implemented."); }
	virtual TransactionContextPtr locateTransactionContext(const uint256& /*tx*/) { throw qbit::exception("NOT_IMPL", "IMemoryPool::locateTransactionContext - not implemented."); }

	virtual void setMainStore(ITransactionStorePtr /*store*/) { throw qbit::exception("NOT_IMPL", "IMemoryPool::setMainStore - not implemented."); }

	virtual IEntityStorePtr entityStore() { throw qbit::exception("NOT_IMPL", "IMemoryPool::entityStore - not implemented."); }
	virtual EntityPtr locateEntity(const std::string&) { throw qbit::exception("NOT_IMPL", "IMemoryPool::locateEntity - not implemented."); }
	virtual ITransactionStorePtr persistentMainStore() { throw qbit::exception("NOT_IMPL", "IMemoryPool::persistentMainStore - not implemented."); }
	virtual ITransactionStorePtr persistentStore() { throw qbit::exception("NOT_IMPL", "IMemoryPool::persistentStore - not implemented."); }
	virtual TransactionContextPtr pushTransaction(TransactionPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::pushTransaction - not implemented."); }
	virtual bool pushTransaction(TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::pushTransaction - not implemented."); }
	virtual IWalletPtr wallet() { throw qbit::exception("NOT_IMPL", "IMemoryPool::wallet - not implemented."); }
	virtual IConsensusPtr consensus() { throw qbit::exception("NOT_IMPL", "IMemoryPool::consensus - not implemented."); }

	virtual BlockContextPtr beginBlock(BlockPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::beginBlock - not implemented."); }
	virtual void commit(BlockContextPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::commit - not implemented."); }

	virtual void removeTransactions(BlockPtr) { throw qbit::exception("NOT_IMPL", "IMemoryPool::removeTransactions - not implemented."); }
	virtual void removeTransaction(const uint256&) { throw qbit::exception("NOT_IMPL", "IMemoryPool::removeTransaction - not implemented."); }
	virtual void pushConfirmedBlock(const NetworkBlockHeader& /*blockHeader*/) { throw qbit::exception("NOT_IMPL", "IMemoryPool::pushConfirmedBlock - not implemented."); }
	virtual bool popUnlinkedOut(const uint256& /*utxo*/, TransactionContextPtr /*ctx*/) { throw qbit::exception("NOT_IMPL", "IMemoryPool::popUnlinkedOut - not implemented."); }	

	virtual void processCandidates() { throw qbit::exception("NOT_IMPL", "IMemoryPool::processCandidates - not implemented."); }	

	virtual void selectTransactions(std::list<uint256>& /*txs*/, uint64_t& /*total*/, size_t /*limit*/) { throw qbit::exception("NOT_IMPL", "IMemoryPool::selectTransactions - not implemented."); }

	virtual void statistics(size_t& /*txs*/, size_t& /*candidates*/, size_t& /*postponed*/) { throw qbit::exception("NOT_IMPL", "IMemoryPool::statistics - not implemented."); }
};

typedef std::shared_ptr<IMemoryPool> IMemoryPoolPtr;

} // qbit

#endif
