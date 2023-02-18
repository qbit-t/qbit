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
	virtual bool existsExplicit(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::existsExplicit - not implemented."); }
	virtual IPeerPtr locate(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::locate - not implemented."); }

	virtual bool updatePeerState(IPeerPtr /*peer*/, StatePtr /*state*/, IPeer::UpdatePeerResult& /*peerResult*/, bool force = false) { throw qbit::exception("NOT_IMPL", "IPeerManager::updatePeerState - not implemented."); }
	virtual bool updatePeerLatency(IPeerPtr /*peer*/, uint32_t /*latency*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::updatePeerLatency - not implemented."); }

	virtual void postpone(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::postpone - not implemented."); }
	virtual void ban(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::ban - not implemented."); }
	virtual void quarantine(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::quarantine - not implemented."); }
	virtual void release(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::ban - not implemented."); }
	virtual void handshaking(const uint160& /*id*/, IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::handshaking - not implemented."); }

	virtual void run() { throw qbit::exception("NOT_IMPL", "IPeerManager::run - not implemented."); }
	virtual void stop() { throw qbit::exception("NOT_IMPL", "IPeerManager::stop - not implemented."); }
	virtual void poll() { throw qbit::exception("NOT_IMPL", "IPeerManager::poll - not implemented."); }
	virtual void suspend() { throw qbit::exception("NOT_IMPL", "IPeerManager::suspend - not implemented."); }
	virtual void resume() { throw qbit::exception("NOT_IMPL", "IPeerManager::resume - not implemented."); }
	virtual void useExplicitPeersOnly() { throw qbit::exception("NOT_IMPL", "IPeerManager::useExplicitPeersOnly - not implemented."); }
	virtual bool explicitPeersOnly() { throw qbit::exception("NOT_IMPL", "IPeerManager::explicitPeersOnly - not implemented."); }

	virtual void updateMedianTime() { throw qbit::exception("NOT_IMPL", "IPeerManager::updateMedianTime - not implemented."); }

	virtual IPeerPtr addPeer(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::addPeer - not implemented."); }
	virtual IPeerPtr addPeerExplicit(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::addPeerExplicit - not implemented."); }
	virtual void deactivatePeer(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::deactivatePeer - not implemented."); }
	virtual void removePeer(IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::removePeer - not implemented."); }
	virtual void removePeer(const std::string& /*endpoint*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::removePeer - not implemented."); }

	virtual uint32_t clients() { throw qbit::exception("NOT_IMPL", "IPeerManager::clients - not implemented."); }

	virtual IConsensusManagerPtr consensusManager() { throw qbit::exception("NOT_IMPL", "IPeerManager::consensusManager - not implemented."); }	
	virtual IMemoryPoolManagerPtr memoryPoolManager() { throw qbit::exception("NOT_IMPL", "IPeerManager::memoryPoolManager - not implemented."); }	
	virtual ISettingsPtr settings() { throw qbit::exception("NOT_IMPL", "IPeerManager::settings - not implemented."); }	

	virtual void activePeers(std::vector<std::string>& /*peers container*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::activePeers - not implemented."); }

	virtual void allPeers(std::list<IPeerPtr>& /*peers*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::allPeers - not implemented."); }
	virtual void explicitPeers(std::list<IPeerPtr>& /*peers*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::explicitPeers - not implemented."); }

	virtual boost::asio::io_context& getContext(int /*id*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::getContext - not implemented."); }
	virtual int getContextId() { throw qbit::exception("NOT_IMPL", "IPeerManager::getContextId - not implemented."); }

	virtual void incPeersCount() { throw qbit::exception("NOT_IMPL", "IPeerManager::incPeersCount - not implemented."); }
	virtual void decPeersCount() { throw qbit::exception("NOT_IMPL", "IPeerManager::decPeersCount - not implemented."); }
	virtual int peersCount() { throw qbit::exception("NOT_IMPL", "IPeerManager::peersCount - not implemented."); }
	virtual int removalQueueLength() { throw qbit::exception("NOT_IMPL", "IPeerManager::removalQueueLength - not implemented."); }

	virtual bool protoEncryption() { throw qbit::exception("NOT_IMPL", "IPeerManager::protoEncryption - not implemented."); }
	virtual void setProtoEncryption(bool /*protoEncryption*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::setProtoEncryption - not implemented."); }

	virtual PeerExtensionCreatorPtr locateExtensionCreator(const std::string& /*dappName*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::locateExtensionCreator - not implemented."); }

	// client facade
	virtual void notifyTransaction(TransactionContextPtr /*ctx*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::notifyTransaction - not implemented."); }
	virtual void broadcastState(StatePtr /*state*/) { throw qbit::exception("NOT_IMPL", "IPeerManager::broadcastStateToClients - not implemented."); }

	//
	static bool extractAddressInfo(const std::string& key, std::string& ip, std::string& port, bool& ex, bool& v6) {
		//
		std::vector<std::string> lPartsV6;
		boost::split(lPartsV6, key, boost::is_any_of("[]"), boost::token_compress_on);
		// ipv6 semantic found [ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]:port:e
		if (lPartsV6.size() >= 2) {
			std::vector<std::string> lPortV6;
			boost::split(lPortV6, lPartsV6[2], boost::is_any_of(":"), boost::token_compress_on); // extract port and mark
			//
			ip = lPartsV6[1];
			port = lPortV6[1];
			v6 = true;
			//
			if (lPortV6.size() > 2 && lPortV6[2] == "e") ex = true;
			else ex = false;
			//
			return true;
		} else {
			std::vector<std::string> lParts;
			boost::split(lParts, key, boost::is_any_of(":"), boost::token_compress_on);

			std::string lEndpoint;
			// ipv4
			if (lParts.size() >= 2) {
				//
				ip = lParts[0];
				port = lParts[1];
				v6 = false;
				//
				if (lParts.size() > 2 && lParts[2] == "e") ex = true;
				else ex = false;
			}

			return true;
		}

		return false;
	}
};

//
typedef std::map<std::string/*dapp name*/, PeerExtensionCreatorPtr> PeerExtensions;
extern PeerExtensions gPeerExtensions; // TODO: init on startup

} // qbit

#endif
