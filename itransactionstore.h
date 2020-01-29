// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ITRANSACTION_STORE_H
#define QBIT_ITRANSACTION_STORE_H

#include "transaction.h"
#include "block.h"
#include "blockcontext.h"
#include "transactioncontext.h"
#include "ientitystore.h"

#include <memory>

namespace qbit {

class IMemoryPool;
typedef std::shared_ptr<IMemoryPool> IMemoryPoolPtr;

class ITransactionStore {
public:
	ITransactionStore() {}

	virtual bool open() { throw qbit::exception("NOT_IMPL", "ITransactionStore::open - not implemented."); }
	virtual bool close() { throw qbit::exception("NOT_IMPL", "ITransactionStore::close - not implemented."); }
	virtual bool isOpened() { throw qbit::exception("NOT_IMPL", "ITransactionStore::isOpened - not implemented."); }
	virtual uint256 chain() { throw qbit::exception("NOT_IMPL", "ITransactionStore::chain - not implemented."); }

	virtual TransactionPtr locateTransaction(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::locateTransaction - not implemented."); }
	virtual TransactionContextPtr locateTransactionContext(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::locateTransactionContext - not implemented."); }
	virtual bool pushTransaction(TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::pushTransaction - not implemented."); }
	virtual BlockContextPtr pushBlock(BlockPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::pushBlock - not implemented."); }
	virtual bool commitBlock(BlockContextPtr, size_t&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::commitBlock - not implemented."); }

	virtual void saveBlock(BlockPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::saveBlock - not implemented."); }
	virtual void reindexFull(const uint256&, IMemoryPoolPtr /*pool*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::reindexFull - not implemented."); }
	virtual void reindex(const uint256&, const uint256&, IMemoryPoolPtr /*pool*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::reindex - not implemented."); }

	// main entry
	virtual TransactionContextPtr pushTransaction(TransactionPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::pushTransaction - not implemented."); }

	virtual bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::pushUnlinkedOut - not implemented."); }
	virtual bool popUnlinkedOut(const uint256&, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::popUnlinkedOut - not implemented."); }
	virtual void addLink(const uint256& /*from*/, const uint256& /*to*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::addLink - not implemented."); }

	virtual Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::findUnlinkedOut - not implemented."); }
	virtual bool isUnlinkedOutUsed(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::isUnlinkedOutUsed - not implemented."); }
	virtual bool isUnlinkedOutExists(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::isUnlinkedOutExists - not implemented."); }

	virtual IEntityStorePtr entityStore() { throw qbit::exception("NOT_IMPL", "ITransactionStore::entityStore - not implemented."); }

	virtual bool resyncHeight() { throw qbit::exception("NOT_IMPL", "ITransactionStore::resyncHeight - not implemented."); }

	virtual void erase(const uint256&, const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::erase - not implemented."); }
	virtual void remove(const uint256&, const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::remove - not implemented."); }
	virtual bool processBlocks(const uint256& /*from*/, const uint256& /*to*/, std::list<BlockContextPtr>& /*ctxs*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::processBlocks - not implemented."); }
	virtual bool setLastBlock(const uint256& /*block*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::setLastBlock - not implemented."); }

	virtual size_t currentHeight(BlockHeader&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::currentHeight - not implemented."); }
	virtual BlockHeader currentBlockHeader() { throw qbit::exception("NOT_IMPL", "ITransactionStore::currentBlockHeader - not implemented."); }

	virtual BlockPtr block(size_t /*height*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::block - not implemented."); }
	virtual BlockPtr block(const uint256& /*id*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::block - not implemented."); }

	virtual bool blockExists(const uint256& /*block*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::blockExists - not implemented."); }
	virtual bool enqueueBlock(const NetworkBlockHeader& /*block*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::enqueueBlock - not implemented."); }
	virtual void dequeueBlock(const uint256& /*block*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::dequeueBlock - not implemented."); }

	virtual bool transactionHeight(const uint256& /*tx*/, size_t& /*height*/, bool& /*coinbase*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::transactionHeight - not implemented."); }
	virtual bool blockHeight(const uint256& /*block*/, size_t& /*height*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::blockHeight - not implemented."); }

	virtual bool firstEnqueuedBlock(NetworkBlockHeader& /*block*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::firstEnqueuedBlock - not implemented."); }
};

typedef std::shared_ptr<ITransactionStore> ITransactionStorePtr;

} // qbit

#endif
