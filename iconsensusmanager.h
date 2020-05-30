// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ICONSENSUS_MANAGER_H
#define QBIT_ICONSENSUS_MANAGER_H

#include "iconsensus.h"
#include "isettings.h"
#include "state.h"
#include "ivalidatormanager.h"
#include "entity.h"

namespace qbit {

class IConsensusManager {
public:
	IConsensusManager() {}

	virtual bool exists(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::exists - not implemented."); }
	virtual IConsensusPtr locate(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::locate - not implemented."); }

	virtual void pushPeerLatency(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::pushPeerLatency - not implemented."); }
	virtual bool pushPeer(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::pushPeer - not implemented."); }
	virtual void popPeer(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::popPeer - not implemented."); }	
	virtual bool peerExists(const uint160& /*peer*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::peerExists - not implemented."); }
	virtual StatePtr currentState() { throw qbit::exception("NOT_IMPL", "IConsensusManager::currentState - not implemented."); }
	virtual size_t quarantineTime() { throw qbit::exception("NOT_IMPL", "IConsensusManager::quarantineTime - not implemented."); }

	virtual bool pushState(StatePtr /*state*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::pushState - not implemented."); }
	virtual bool popState(StatePtr /*state*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::popState - not implemented."); }
	virtual void broadcastState(StatePtr /*state*/, const uint160& /*peer*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::broadcastState - not implemented."); }

	virtual bool add(IConsensusPtr /*consensus*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::add - not implemented."); }
	virtual IConsensusPtr push(const uint256& /*chain*/, EntityPtr /*dapp*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::push - not implemented."); }
	virtual void pop(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::pop - not implemented."); }
	virtual std::vector<IConsensusPtr> consensuses() { throw qbit::exception("NOT_IMPL", "IConsensusManager::pools - not implemented."); }

	virtual SKeyPtr mainKey() { throw qbit::exception("NOT_IMPL", "IConsensusManager::mainKey - not implemented."); }
	virtual PKey mainPKey() { throw qbit::exception("NOT_IMPL", "IConsensusManager::mainPKey - not implemented."); }
	virtual IValidator::BlockCheckResult pushBlockHeader(const NetworkBlockHeader& /*block*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::pushBlockHeader - not implemented."); }
	virtual void broadcastBlockHeader(const NetworkBlockHeader& /*block*/, const uint160& /*peer*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::broadcastBlockHeader - not implemented."); }
	virtual void broadcastTransaction(TransactionContextPtr /*ctx*/, const uint160& /*peer*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::broadcastTransaction - not implemented."); }
	virtual void broadcastBlockHeaderAndState(const NetworkBlockHeader& /*block*/, StatePtr /*state*/, const uint160& /*except*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::broadcastBlockHeaderAndState - not implemented."); }
	virtual TransactionContextPtr processTransaction(TransactionPtr) { throw qbit::exception("NOT_IMPL", "IConsensusManager::processTransaction - Not implemented."); }

	virtual IValidatorManagerPtr validatorManager() { throw qbit::exception("NOT_IMPL", "IConsensusManager::validatorManager - not implemented."); }
	virtual ITransactionStoreManagerPtr storeManager() { throw qbit::exception("NOT_IMPL", "IConsensusManager::storeManager - not implemented."); }
	virtual IWalletPtr wallet() { throw qbit::exception("NOT_IMPL", "IConsensusManager::wallet - not implemented."); }
	virtual IMemoryPoolManagerPtr mempoolManager() { throw qbit::exception("NOT_IMPL", "IConsensusManager::mempoolManager - not implemented."); }

	virtual size_t chainSupportPeersCount(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IConsensusManager::chainSupportPeersCount - not implemented."); }
	virtual const std::vector<State::DAppInstance>& dApps() const {	throw qbit::exception("NOT_IMPL", "IConsensusManager::dApps - not implemented."); }

	// open requests
	virtual void acquireBlockHeaderWithCoinbase(const uint256& /*block*/, const uint256& /*chain*/, INetworkBlockHandlerWithCoinBasePtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IConsensus::acquireBlockHeaderWithCoinbase - not implemented."); }
	virtual void broadcastStateToClients(StatePtr /*state*/) { throw qbit::exception("NOT_IMPL", "IConsensus::broadcastStateToClients - not implemented."); }
	virtual void broadcastAirdropRequest(const PKey& /*key*/, const uint160& /*except*/) { throw qbit::exception("NOT_IMPL", "IConsensus::broadcastAirdropRequest - not implemented."); }
};

typedef std::shared_ptr<IConsensusManager> IConsensusManagerPtr;

//
class ConsensusCreator {
public:
	ConsensusCreator() {}
	virtual IConsensusPtr create(const uint256& /*chain*/, IConsensusManagerPtr /*consensusManager*/, ISettingsPtr /*settings*/, IWalletPtr /*wallet*/, ITransactionStorePtr /*store*/) { return nullptr; }
};

typedef std::shared_ptr<ConsensusCreator> ConsensusCreatorPtr;

//
typedef std::map<std::string/*dapp name*/, ConsensusCreatorPtr> Consensuses;
extern Consensuses gConsensuses; // TODO: init on startup

} // qbit

#endif
