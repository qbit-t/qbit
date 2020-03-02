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
#include "itransactionstoreextension.h"

#include <memory>

namespace qbit {

class IMemoryPool;
typedef std::shared_ptr<IMemoryPool> IMemoryPoolPtr;

class ITransactionStoreManager;
typedef std::shared_ptr<ITransactionStoreManager> ITransactionStoreManagerPtr;

class ITransactionStoreExtension;
typedef std::shared_ptr<ITransactionStoreExtension> ITransactionStoreExtensionPtr;

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
	virtual bool commitBlock(BlockContextPtr, uint64_t&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::commitBlock - not implemented."); }

	virtual void saveBlock(BlockPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::saveBlock - not implemented."); }
	virtual void saveBlockHeader(const BlockHeader& /*header*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::saveBlockHeader - not implemented."); }
	virtual void reindexFull(const uint256&, IMemoryPoolPtr /*pool*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::reindexFull - not implemented."); }
	virtual bool reindex(const uint256&, const uint256&, IMemoryPoolPtr /*pool*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::reindex - not implemented."); }

	// main entry
	virtual TransactionContextPtr pushTransaction(TransactionPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::pushTransaction - not implemented."); }

	virtual bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::pushUnlinkedOut - not implemented."); }
	virtual bool popUnlinkedOut(const uint256&, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "ITransactionStore::popUnlinkedOut - not implemented."); }
	virtual void addLink(const uint256& /*from*/, const uint256& /*to*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::addLink - not implemented."); }

	virtual bool enumUnlinkedOuts(const uint256& /*tx*/, std::vector<Transaction::UnlinkedOutPtr>& /*outs*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::enumUnlinkedOuts - not implemented."); }

	virtual Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::findUnlinkedOut - not implemented."); }
	virtual Transaction::UnlinkedOutPtr findLinkedOut(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::findLinkedOut - not implemented."); }
	virtual bool isUnlinkedOutUsed(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::isUnlinkedOutUsed - not implemented."); }
	virtual bool isUnlinkedOutExists(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::isUnlinkedOutExists - not implemented."); }
	virtual bool isLinkedOutExists(const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::isLinkedOutExists - not implemented."); }

	virtual IEntityStorePtr entityStore() { throw qbit::exception("NOT_IMPL", "ITransactionStore::entityStore - not implemented."); }
	virtual ITransactionStoreManagerPtr storeManager() { throw qbit::exception("NOT_IMPL", "ITransactionStore::storeManager - Not implemented."); }	

	virtual bool resyncHeight() { throw qbit::exception("NOT_IMPL", "ITransactionStore::resyncHeight - not implemented."); }

	virtual void erase(const uint256&, const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::erase - not implemented."); }
	virtual void remove(const uint256&, const uint256&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::remove - not implemented."); }
	virtual bool processBlocks(const uint256& /*from*/, const uint256& /*to*/, std::list<BlockContextPtr>& /*ctxs*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::processBlocks - not implemented."); }
	virtual bool setLastBlock(const uint256& /*block*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::setLastBlock - not implemented."); }

	virtual uint64_t currentHeight(BlockHeader&) { throw qbit::exception("NOT_IMPL", "ITransactionStore::currentHeight - not implemented."); }
	virtual BlockHeader currentBlockHeader() { throw qbit::exception("NOT_IMPL", "ITransactionStore::currentBlockHeader - not implemented."); }

	virtual BlockPtr block(uint64_t /*height*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::block - not implemented."); }
	virtual BlockPtr block(const uint256& /*id*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::block - not implemented."); }
	virtual bool blockHeader(const uint256& /*id*/, BlockHeader& /*header*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::blockHeader - not implemented."); }

	virtual bool blockExists(const uint256& /*block*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::blockExists - not implemented."); }
	virtual bool enqueueBlock(const NetworkBlockHeader& /*block*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::enqueueBlock - not implemented."); }
	virtual void dequeueBlock(const uint256& /*block*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::dequeueBlock - not implemented."); }

	virtual bool transactionHeight(const uint256& /*tx*/, uint64_t& /*height*/, uint64_t& /*confirms*/, bool& /*coinbase*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::transactionHeight - not implemented."); }
	virtual bool transactionInfo(const uint256& /*tx*/, uint256& /*block*/, uint64_t& /*height*/, uint64_t& /*confirms*/, uint32_t& /*index*/, bool& /*coinbase*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::transactionInfo - not implemented."); }
	virtual bool blockHeight(const uint256& /*block*/, uint64_t& /*height*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::blockHeight - not implemented."); }
	virtual bool blockHeaderHeight(const uint256& /*block*/, uint64_t& /*height*/, BlockHeader& /*header*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::blockHeaderHeight - not implemented."); }

	virtual bool firstEnqueuedBlock(NetworkBlockHeader& /*block*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::firstEnqueuedBlock - not implemented."); }

	virtual void setExtension(ITransactionStoreExtensionPtr /*extension*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::setExtension - not implemented."); }
	virtual ITransactionStoreExtensionPtr extension() { throw qbit::exception("NOT_IMPL", "ITransactionStore::extension - not implemented."); }
};

typedef std::shared_ptr<ITransactionStore> ITransactionStorePtr;

} // qbit

#endif
