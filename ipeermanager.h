// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IPEER_MANAGER_H
#define QBIT_IPEER_MANAGER_H

#include "iconsensus.h"
#include "isettings.h"
#include "imemorypool.h"
#include "ipeer.h"
#include "state.h"

namespace qbit {

class IPeerManager {
public:
	IPeerManager() {}

	virtual bool exists(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::exists - not implemented."); }
	virtual bool existsActive(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::existsActive - not implemented."); }

	virtual bool updatePeerState(IPeerPtr /*peer*/, State& /*state*/, IPeer::UpdatePeerResult& /*peerResult*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::updatePeerState - not implemented."); }
	virtual bool updatePeerLatency(IPeerPtr /*peer*/, uint32_t /*latency*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::updatePeerLatency - not implemented."); }

	virtual void postpone(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::postpone - not implemented."); }
	virtual void ban(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::ban - not implemented."); }

	virtual void run() { throw qbit::exception("NOT_IMPL", "IPeerManager::run - not implemented."); }
	virtual void stop() { throw qbit::exception("NOT_IMPL", "IPeerManager::stop - not implemented."); }

	virtual bool addPeer(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::addPeer - not implemented."); }
	virtual void deactivatePeer(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::deactivatePeer - not implemented."); }
	virtual void removePeer(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::removePeer - not implemented."); }

	virtual IConsensusPtr consensus() { throw qbit::exception("NOT_IMPL", "IPeerManager::consensus - not implemented."); }	
	virtual ISettingsPtr settings() { throw qbit::exception("NOT_IMPL", "IPeerManager::settings - not implemented."); }	

	virtual void activePeers(std::vector<std::string>& /*peers container*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::activePeers - not implemented."); }

	virtual boost::asio::io_context& getContext(int /*id*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::getContext - not implemented."); }
	virtual int getContextId() { throw qbit::exception("NOT_IMPL", "IPeerManager::getContextId - not implemented."); }
};

typedef std::shared_ptr<IPeerManager> IPeerManagerPtr;

} // qbit

#endif
