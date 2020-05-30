// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IPEER_MANAGER_H
#define QBIT_IPEER_MANAGER_H

#include "iconsensusmanager.h"
#include "isettings.h"
#include "imemorypool.h"
#include "ipeer.h"
#include "state.h"

namespace qbit {

class IPeerManager;
typedef std::shared_ptr<IPeerManager> IPeerManagerPtr;

class PeerExtensionCreator {
public:
	PeerExtensionCreator() {}
	virtual IPeerExtensionPtr create(const State::DAppInstance& /*dapp*/, IPeerPtr /*peer*/, IPeerManagerPtr /*peerManager*/) { return nullptr; }
};
typedef std::shared_ptr<PeerExtensionCreator> PeerExtensionCreatorPtr;

class IPeerManager {
public:
	IPeerManager() {}

	virtual bool exists(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::exists - not implemented."); }
	virtual bool existsActive(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::existsActive - not implemented."); }
	virtual bool existsBanned(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::existsBanned - not implemented."); }
	virtual bool existsPostponed(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::existsPostponed - not implemented."); }

	virtual bool updatePeerState(IPeerPtr /*peer*/, State& /*state*/, IPeer::UpdatePeerResult& /*peerResult*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::updatePeerState - not implemented."); }
	virtual bool updatePeerLatency(IPeerPtr /*peer*/, uint32_t /*latency*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::updatePeerLatency - not implemented."); }

	virtual void postpone(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::postpone - not implemented."); }
	virtual void ban(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::ban - not implemented."); }

	virtual void run() { throw qbit::exception("NOT_IMPL", "IPeerManager::run - not implemented."); }
	virtual void stop() { throw qbit::exception("NOT_IMPL", "IPeerManager::stop - not implemented."); }

	virtual void updateMedianTime() { throw qbit::exception("NOT_IMPL", "IPeerManager::updateMedianTime - not implemented."); }

	virtual IPeerPtr addPeer(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::addPeer - not implemented."); }
	virtual IPeerPtr addPeerExplicit(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::addPeerExplicit - not implemented."); }
	virtual void deactivatePeer(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::deactivatePeer - not implemented."); }
	virtual void removePeer(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::removePeer - not implemented."); }

	virtual uint32_t clients() { throw qbit::exception("NOT_IMPL", "IPeerManager::clients - not implemented."); }

	virtual IConsensusManagerPtr consensusManager() { throw qbit::exception("NOT_IMPL", "IPeerManager::consensusManager - not implemented."); }	
	virtual IMemoryPoolManagerPtr memoryPoolManager() { throw qbit::exception("NOT_IMPL", "IPeerManager::memoryPoolManager - not implemented."); }	
	virtual ISettingsPtr settings() { throw qbit::exception("NOT_IMPL", "IPeerManager::settings - not implemented."); }	

	virtual void activePeers(std::vector<std::string>& /*peers container*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::activePeers - not implemented."); }

	virtual void allPeers(std::list<IPeerPtr>& /*peers*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::allPeers - not implemented."); }

	virtual boost::asio::io_context& getContext(int /*id*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::getContext - not implemented."); }
	virtual int getContextId() { throw qbit::exception("NOT_IMPL", "IPeerManager::getContextId - not implemented."); }

	virtual void incPeersCount() { throw qbit::exception("NOT_IMPL", "IPeerManager::incPeersCount - not implemented."); }
	virtual void decPeersCount() { throw qbit::exception("NOT_IMPL", "IPeerManager::decPeersCount - not implemented."); }
	virtual int peersCount() { throw qbit::exception("NOT_IMPL", "IPeerManager::peersCount - not implemented."); }

	virtual PeerExtensionCreatorPtr locateExtensionCreator(const std::string& /*dappName*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::locateExtensionCreator - not implemented."); }

	// client facade
	virtual void notifyTransaction(TransactionContextPtr /*ctx*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::notifyTransaction - not implemented."); }
	virtual void broadcastState(StatePtr /*state*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::broadcastStateToClients - not implemented."); }
};

//
typedef std::map<std::string/*dapp name*/, PeerExtensionCreatorPtr> PeerExtensions;
extern PeerExtensions gPeerExtensions; // TODO: init on startup

} // qbit

#endif
