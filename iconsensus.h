// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ICONSENSUS_H
#define QBIT_ICONSENSUS_H

#include "transaction.h"
#include "block.h"
#include "transactioncontext.h"
#include "state.h"

#include "synchronizationjob.h"
#include "isettings.h"
#include "ivalidatormanager.h"
#include "itransactionstore.h"
#include "imemorypool.h"
#include "timestamp.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/chrono.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <memory>

namespace qbit {

class IConsensusManager;
typedef std::shared_ptr<IConsensusManager> IConsensusManagerPtr;

class IConsensus {
public:
	enum ChainState {
		UNDEFINED = 0,
		NOT_SYNCHRONIZED = 1,
		SYNCHRONIZING = 2,
		INDEXING = 3, 
		SYNCHRONIZED = 4
	};

public:
	IConsensus() {}

	virtual uint256 chain() { throw qbit::exception("NOT_IMPL", "IConsensus::chain - not implemented."); }	
	virtual bool close() { throw qbit::exception("NOT_IMPL", "IConsensus::close - not implemented."); }	

	virtual size_t maxBlockSize() { throw qbit::exception("NOT_IMPL", "IConsensus::maxBlockSize - not implemented."); }
	virtual uint64_t currentTime() { throw qbit::exception("NOT_IMPL", "IConsensus::currentTime - not implemented."); }
	virtual StatePtr currentState() { throw qbit::exception("NOT_IMPL", "IConsensus::currentState - not implemented."); }
	virtual size_t quarantineTime() { throw qbit::exception("NOT_IMPL", "IConsensus::quarantineTime - not implemented."); }
	virtual size_t partialTreeThreshold() { throw qbit::exception("NOT_IMPL", "IConsensus::partialTreeThreshold - not implemented."); }

	virtual void pushPeer(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IConsensus::pushPeer - not implemented."); }
	virtual void popPeer(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IConsensus::popPeer - not implemented."); }

	virtual bool pushState(StatePtr /*state*/) { throw qbit::exception("NOT_IMPL", "IConsensus::pushState - not implemented."); }
	virtual bool popState(StatePtr /*state*/) { throw qbit::exception("NOT_IMPL", "IConsensus::popState - not implemented."); }
	virtual void broadcastState(StatePtr /*state*/, const uint160& /*except*/) { throw qbit::exception("NOT_IMPL", "IConsensus::broadcastState - not implemented."); }

	//virtual bool pushBlock(BlockPtr /*block*/) { throw qbit::exception("NOT_IMPL", "IConsensus::pushBlock - not implemented."); }
	virtual IValidator::BlockCheckResult pushBlockHeader(const NetworkBlockHeader& /*block*/, IValidatorPtr /*validator*/) { throw qbit::exception("NOT_IMPL", "IConsensus::pushBlockHeader - not implemented."); }
	virtual void broadcastBlockHeader(const NetworkBlockHeader& /*block*/, const uint160& /*except*/) { throw qbit::exception("NOT_IMPL", "IConsensus::broadcastBlockHeader - not implemented."); }
	virtual void broadcastBlockHeaderAndState(const NetworkBlockHeader& /*block*/, StatePtr /*state*/, const uint160& /*except*/) { throw qbit::exception("NOT_IMPL", "IConsensus::broadcastBlockHeaderAndState - not implemented."); }
	virtual bool acquireBlock(const NetworkBlockHeader& /*block*/) { throw qbit::exception("NOT_IMPL", "IConsensus::acquireBlock - not implemented."); }

	virtual bool broadcastTransaction(TransactionContextPtr /*ctx*/, const uint160& /*except*/) { throw qbit::exception("NOT_IMPL", "IConsensus::broadcastTransaction - not implemented."); }

	virtual SKey mainKey() { throw qbit::exception("NOT_IMPL", "IConsensus::mainKey - not implemented."); }

	virtual size_t blockTime() { throw qbit::exception("NOT_IMPL", "IConsensus::blockTime - not implemented."); }

	virtual ISettingsPtr settings() { throw qbit::exception("NOT_IMPL", "IConsensus::settings - not implemented."); }
	virtual void setValidatorManager(IValidatorManagerPtr /*validatorManager*/) { throw qbit::exception("NOT_IMPL", "IConsensus::setValidatorManager - not implemented."); }

	virtual bool isChainSynchronized() { throw qbit::exception("NOT_IMPL", "IConsensus::isChainSynchronized - not implemented."); }
	virtual uint64_t locateSynchronizedRoot(std::list<IPeerPtr>& /*peers*/, uint256& /*block*/) { throw qbit::exception("NOT_IMPL", "IConsensus::locateSynchronizedRoot - not implemented."); }

	virtual ChainState chainState() { throw qbit::exception("NOT_IMPL", "IConsensus::chainState - not implemented."); }
	virtual bool doSynchronize() { throw qbit::exception("NOT_IMPL", "IConsensus::doSynchrinize - not implemented."); }
	virtual void doIndex(const uint256& /*block*/) { throw qbit::exception("NOT_IMPL", "IConsensus::doIndex - not implemented."); }
	virtual bool doIndex(const uint256& /*block*/, const uint256& /*lastFoundBlock*/) { throw qbit::exception("NOT_IMPL", "IConsensus::doIndex - not implemented."); }
	virtual void toNonSynchronized() { throw qbit::exception("NOT_IMPL", "IConsensus::toNonSynchronized - not implemented."); }
	virtual void toSynchronizing() { throw qbit::exception("NOT_IMPL", "IConsensus::toSynchronizing - not implemented."); }

	virtual SynchronizationJobPtr lastJob() { throw qbit::exception("NOT_IMPL", "IConsensus::lastJob - not implemented."); }
	virtual ITransactionStorePtr store() { throw qbit::exception("NOT_IMPL", "IConsensus::store - not implemented."); }

	virtual std::string chainStateString() { throw qbit::exception("NOT_IMPL", "IConsensus::chainStateString - not implemented."); }
	virtual size_t maturity() { throw qbit::exception("NOT_IMPL", "IConsensus::maturity - not implemented."); }
	virtual size_t coinbaseMaturity() { throw qbit::exception("NOT_IMPL", "IConsensus::coinbaseMaturity - not implemented."); }

	virtual amount_t blockReward(uint64_t /*height*/) { throw qbit::exception("NOT_IMPL", "IConsensus::blockReward - not implemented."); }
	virtual bool checkBalance(amount_t /*coinbaseAmount*/, amount_t /*blockFee*/, uint64_t /*height*/) { throw qbit::exception("NOT_IMPL", "IConsensus::checkCoinbaseAmountAndFee - not implemented."); }
	virtual bool checkSequenceConsistency(const BlockHeader& /*block*/) { throw qbit::exception("NOT_IMPL", "IConsensus::checkSequenceConsistency - not implemented."); }

	virtual bool processPartialTreeHeaders(SynchronizationJobPtr /*job*/) { throw qbit::exception("NOT_IMPL", "IConsensus::processPartialTreeHeaders - not implemented."); }

	virtual bool isSimpleNetwork() { throw qbit::exception("NOT_IMPL", "IConsensus::isSimpleNetwork - not implemented."); }

	virtual void collectPeers(std::map<uint160, IPeerPtr>& /*peers*/) { throw qbit::exception("NOT_IMPL", "IConsensus::collectPeers - not implemented."); }

	virtual IConsensusManagerPtr consensusManager() { throw qbit::exception("NOT_IMPL", "IConsensus::consensusManager - not implemented."); }
};

typedef std::shared_ptr<IConsensus> IConsensusPtr;

} // qbit

#endif
