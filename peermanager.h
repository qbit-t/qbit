// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_PEERMANAGER_H
#define QBIT_PEERMANAGER_H

#include "ipeermanager.h"
#include "iconsensusmanager.h"
#include "log/log.h"
#include "db/containers.h"
#include "mkpath.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/chrono.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/range/algorithm.hpp>

namespace qbit {

class PeerManager: public IPeerManager, public std::enable_shared_from_this<PeerManager> {
public:
	PeerManager(ISettingsPtr settings, IConsensusManagerPtr consensusManager, IMemoryPoolManagerPtr mempoolManager) : 
		settings_(settings), consensusManager_(consensusManager), mempoolManager_(mempoolManager),
		peersContainer_(settings->dataPath() + "/peers") {

		// prepare pool contexts
		for (std::size_t lIdx = 0; lIdx < settings_->threadPoolSize(); ++lIdx) {
			IOContextPtr lContext(new boost::asio::io_context());
			contexts_.push_back(lContext);
			work_.push_back(boost::asio::make_work_guard(*lContext));

			peers_[lIdx] = PeersMap();
			active_[lIdx] = PeersSet();
			quarantine_[lIdx] = PeersSet();
			banned_[lIdx] = PeersSet();
			inactive_[lIdx] = PeersSet();

			contextMutex_.push_back(new boost::recursive_mutex());
		}
	}

	inline bool exists(const std::string& endpoint) {
		// traverse
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lChannel->first]);
			std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != lChannel->second.end()) return true;
		}

		return false;
	}

	inline bool existsBanned(const std::string& endpoint) {
		// traverse
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lChannel->first]);
			std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != lChannel->second.end() && banned_[lChannel->first].find(endpoint) != banned_[lChannel->first].end())
				return true;
		}

		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
			// check banned hosts
			std::string lHost = endpoint.substr(0, endpoint.find(":"));
			//
			if (lHost.length()) {
				if (bannedEndpoins_.find(lHost) != bannedEndpoins_.end()) return true;
			}
		}
		
		return false;
	}

	inline bool existsPostponed(const std::string& endpoint) {
		// traverse
		return false;
	}

	inline bool existsQuarantine(const std::string& endpoint) {
		// traverse
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lChannel->first]);
			std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != lChannel->second.end() && quarantine_[lChannel->first].find(endpoint) != quarantine_[lChannel->first].end())
				return true;
		}
		
		return false;
	}

	inline bool existsBannedOrQuarantine(const std::string& endpoint) {
		// traverse
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lChannel->first]);
			std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != lChannel->second.end() && banned_[lChannel->first].find(endpoint) != banned_[lChannel->first].end())
				return true;
			if (lPeerIter != lChannel->second.end() && quarantine_[lChannel->first].find(endpoint) != quarantine_[lChannel->first].end())
				return true;
		}
		
		return false;
	}

	inline bool existsActive(const std::string& endpoint) {
		// traverse
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lChannel->first]);
			std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != lChannel->second.end() && active_[lChannel->first].find(endpoint) != active_[lChannel->first].end())
				return true;
		}
		
		return false;
	}

	static IPeerManagerPtr instance(ISettingsPtr settings, IConsensusManagerPtr consensusManager, IMemoryPoolManagerPtr mempoolManager) { 
		return std::make_shared<PeerManager>(settings, consensusManager, mempoolManager); 
	}

	static IPeerManagerPtr instance(ISettingsPtr settings, IConsensusManagerPtr consensusManager) { 
		return std::make_shared<PeerManager>(settings, consensusManager, nullptr); 
	}

	bool updatePeerLatency(IPeerPtr peer, uint32_t latency) {
		//
		IPeerPtr lPeer = peer;

		// peer exists
		if (lPeer) {
			// check
			if (lPeer->status() == Peer::BANNED || lPeer->status() == Peer::QUARANTINE) return false;

			lPeer->setLatency(latency);
			update(lPeer);

			return true;
		}

		return false;
	}

	void updateMedianTime() {
		// traverse
		uint64_t lTime = 0;
		uint64_t lOwnTime = getMicroseconds();
		int lPeers = 0;

		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lChannel->first]);

			for (std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeer = lChannel->second.begin(); lPeer != lChannel->second.end(); lPeer++) {
				// only active peers
				if (lPeer->second->status() != Peer::ACTIVE) continue;
				// only NOT clients
				if (lPeer->second->state()->client()) continue;
				// time diff shoud not be greater than 100 blocks * 5000 ms
				if (lPeer->second->time() > lOwnTime && lPeer->second->time() - lOwnTime > 100 * 5000) continue;
				else if (lPeer->second->time() < lOwnTime && lOwnTime - lPeer->second->time() > 100 * 5000) continue;
				// otherwise
				lTime += lPeer->second->time() + (getMicroseconds() - lPeer->second->timestamp());
				lPeers++;
			}
		}

		// own time
		lTime += getMicroseconds();
		lPeers++;

		// median microseconds
		uint64_t lMedianMicroseconds = lTime / lPeers;
		//consensusManager_->setMedianMicroseconds(lMedianMicroseconds);
		qbit::gMedianMicroseconds = lMedianMicroseconds;

		// median time
		uint64_t lMedianTime = lMedianMicroseconds / 100000;
		uint64_t lLastDigit = lMedianTime % 10;

		lMedianTime = lMedianTime / 10;
		if (lLastDigit >= 5) lMedianTime++;

		//consensusManager_->setMedianTime(lMedianTime); // seconds
		qbit::gMedianTime = lMedianTime;
	}

	void deactivatePeer(IPeerPtr peer) {
		//
		IPeerPtr lPeer = peer;
		if (lPeer) {
			if (lPeer->isOutbound()) pending(lPeer); // just move to pending state
			else deactivate(lPeer); // deactivate if inbound (will be removed)
		}
	}

	void removePeer(IPeerPtr peer) {
		if (peer) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[peer->contextId()]);

			peer->release();

			peers_[peer->contextId()].erase(peer->key());
			active_[peer->contextId()].erase(peer->key());
			inactive_[peer->contextId()].erase(peer->key());	
			quarantine_[peer->contextId()].erase(peer->key());	
			banned_[peer->contextId()].erase(peer->key());	

			gLog().write(Log::NET, std::string("[peerManager]: peer ") + strprintf("%s/%s deleted", peer->key(), peer->addressId().toHex()));
		} else {
			if (gLog().isEnabled(Log::NET)) 
				gLog().write(Log::NET, std::string("[peerManager]: peer was NOT FOUND! "));
		}
	}

	void removePeer(const std::string& endpoint) {
		//
		IPeerPtr lPeer = locate(endpoint);
		if (lPeer) {
			if (lPeer->type() == IPeer::Type::EXPLICIT) {
				// check all
				for (int lIdx = 0; lIdx < contexts_.size(); lIdx++) {
					boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lIdx]);
					explicit_[lIdx].erase(lPeer->key());
				}

				//
				/*
				{
					//
					std::vector<std::string> lParts;
					boost::split(lParts, lPeer->key(), boost::is_any_of(":"), boost::token_compress_on);

					//
					bool lResult = false;
					if (lParts.size() > 1) {
						boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
						explicitEndpoins_.erase(lParts[0]);
					}
				}
				*/

				peersContainer_.remove(lPeer->key()); // remove from storage
			}

			removePeer(lPeer);
		}
	}

	bool updatePeerState(IPeerPtr peer, StatePtr state, IPeer::UpdatePeerResult& peerResult, bool force) {
		// forward
		peerResult = IPeer::SUCCESSED;

		// sanity check
		StatePtr lOwnState = consensusManager()->currentState();
		if (lOwnState->address().id() == state->address().id()) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: self connection ") + lOwnState->address().id().toHex() + " / " + state->address().id().toHex());
			ban(peer);
			peerResult = IPeer::BAN;
			return false;
		}

		//
		IPeerPtr lPeer = peer;

		// check
		if (state->client() && lOwnState->nodeOrFullNode()) {
			// WARNING: clients_.size() is relatively thread safe
			if (clients_.size() > settings_->clientSessionsLimit()) {
				peerResult = IPeer::SESSIONS_EXCEEDED;
				return false;
			}
		}

		// peer exists
		if (exists(lPeer->key())) {
			// check
			if (lPeer->status() == IPeer::BANNED) return false;

			// sanity for new key
			if (lPeer->status() != IPeer::UNDEFINED && 
						lPeer->address().valid() && lPeer->address().id() != state->address().id()) {
				if (gLog().isEnabled(Log::NET)) 
					gLog().write(Log::NET, strprintf("[peerManager]: quarantine peer with old id = %s, new id = %s, key = %s", 
						lPeer->address().id().toHex(), state->address().id().toHex(), lPeer->key()));
				quarantine(lPeer);
				return false;
			}

			// check new state
			if (state->valid()) {
				lPeer->setState(state);

				// check for quarantine
				if (lPeer->onQuarantine())
					return false;

				// update state and data
				bool lResult = update(lPeer, force);
				if (!lResult) peerResult = IPeer::EXISTS;
				return lResult;
			} else {
				ban(lPeer);
				return false;
			}
		// peer is new - inbound only
		} else {
			if (state->valid()) {
				lPeer->setState(state);
				bool lResult = activate(lPeer, force);
				if (!lResult) peerResult = IPeer::EXISTS;
				return lResult;
			} else {
				return false;
			}
		}

		return true;
	}

	IConsensusManagerPtr consensusManager() { return consensusManager_; }
	ISettingsPtr settings() { return settings_; }
	IMemoryPoolManagerPtr memoryPoolManager() { return mempoolManager_; }

	IPeerPtr addPeerInternal(const std::string& endpoint, IPeer::Type type) {
		//
		std::vector<std::string> lParts;
		boost::split(lParts, endpoint, boost::is_any_of(":"), boost::token_compress_on);

		std::string lEndpoint;
		if (lParts.size() >= 2) {
			//
			if (lParts[0] == "0.0.0.0" || lParts[0] == "127.0.0.1" && !qbit::gTestNet) return nullptr;
			//
			lEndpoint = strprintf("%s:%s", lParts[0], lParts[1]);
			//
			if (locate(lEndpoint) == nullptr) {
				//
				if (lParts.size() >= 2) {
					IPeerPtr lPeer = Peer::instance(getContextId(), lEndpoint, std::static_pointer_cast<IPeerManager>(shared_from_this()));
					
					if (lParts.size() > 2 && lParts[2] == "e") lPeer->setType(IPeer::Type::EXPLICIT);
					else lPeer->setType(type);
					add(lPeer);

					return lPeer;
				}
			}
		}

		return nullptr;
	}

	IPeerPtr addPeer(const std::string& endpoint) {
		//
		return addPeerInternal(endpoint, IPeer::Type::IMPLICIT);
	}

	IPeerPtr addPeerExplicit(const std::string& endpoint) {
		//
		return addPeerInternal(endpoint, IPeer::Type::EXPLICIT);
	}

	IPeerPtr locate(const std::string& endpoint) {
		// traverse
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lChannel->first]);
			std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != lChannel->second.end()) return lPeerIter->second;
		}

		return nullptr;
	}	

	void broadcastState(StatePtr state) {
		//
		std::set<uint160> lClients;
		std::map<uint160, std::set<std::string>> lPeerIdx;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
			lClients = clients_;
			lPeerIdx = peerIdx_;
		}

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, 
			strprintf("[peerManager]: broadcasting state - %s", state->toStringShort()));

		// traverse peers
		for (std::set<uint160>::iterator lClient = lClients.begin(); lClient != lClients.end(); lClient++) {
			std::map<uint160, std::set<std::string>>::iterator lPeerIter = lPeerIdx.find(*lClient);
			if (lPeerIter != lPeerIdx.end()) {
				for (std::set<std::string>::iterator lKey = lPeerIter->second.begin(); lKey != lPeerIter->second.end(); lKey++) {
					IPeerPtr lPeer = locate(*lKey);
					if (lPeer) {
						std::map<std::string, uint512>::iterator lPeerStateSent = peerStateSent_.find(*lKey);
						if (lPeerStateSent == peerStateSent_.end() || lPeerStateSent->second != state->signature()) {
							if (!lPeer->state()->daemon()) lPeer->broadcastState(state);
							if (lPeerStateSent == peerStateSent_.end()) peerStateSent_[*lKey] = state->signature();
							else lPeerStateSent->second = state->signature();
						}
					}
				}
			}
		}
	}

	void notifyTransaction(TransactionContextPtr ctx) {
		//
		std::set<uint160> lClients;
		std::map<uint160, std::set<std::string>> lPeerIdx;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
			lClients = clients_;
			lPeerIdx = peerIdx_;
		}

		if (gLog().isEnabled(Log::NET)) 
			gLog().write(Log::NET, std::string("[peerManager]: try broadcast ") + strprintf("tx %s for %d addresses", ctx->tx()->id().toHex(), ctx->addresses().size()));

		// traverse addresses for tx
		std::set<PKey>& lOutAddresses = ctx->outAddresses(); // ref out
		std::set<PKey>& lInAddresses = ctx->addresses(); // ref in
		for (std::set<PKey>::iterator lAddress = lOutAddresses.begin(); lAddress != lOutAddresses.end(); lAddress++) {
			//
			uint160 lAddressId = lAddress->id();
			//
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: looking for client ") + strprintf("%s...", lAddressId.toHex()));
			if (lClients.find(lAddressId) != lClients.end() && lInAddresses.find(*lAddress) == lInAddresses.end()) {
				//
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: found client ") + strprintf("%s", lAddressId.toHex()));

				std::map<uint160, std::set<std::string>>::iterator lPeerIter = lPeerIdx.find(lAddressId);
				if (lPeerIter != lPeerIdx.end()) {
					//
					for (std::set<std::string>::iterator lKey = lPeerIter->second.begin(); lKey != lPeerIter->second.end(); lKey++) {
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: found client index ") + strprintf("%s/%s", lAddressId.toHex(), *lKey));

						IPeerPtr lPeer = locate(*lKey);
						if (lPeer) {
							lPeer->broadcastTransaction(ctx); // every direct transaction should be delivered (value)
						} else {
							if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: peer NOT found for ") + strprintf("%s/%s", lAddressId.toHex(), *lKey));
						}
					}
				}
			}	
		}

		// traverse peers
		for (std::set<uint160>::iterator lClient = lClients.begin(); lClient != lClients.end(); lClient++) {
			std::map<uint160, std::set<std::string>>::iterator lPeerIter = lPeerIdx.find(*lClient);
			if (lPeerIter != lPeerIdx.end()) {
				for (std::set<std::string>::iterator lKey = lPeerIter->second.begin(); lKey != lPeerIter->second.end(); lKey++) {
					IPeerPtr lPeer = locate(*lKey);
					if (lPeer) {
						std::map<std::string, IPeerExtensionPtr> lExtensions = lPeer->extensions();
						for (std::map<std::string, IPeerExtensionPtr>::iterator lExtension = lExtensions.begin(); lExtension != lExtensions.end(); lExtension++) {
							if (gLog().isEnabled(Log::NET)) 
								gLog().write(Log::NET, std::string("[peerManager]: processing extension ") + strprintf("tx %s for peer %s", ctx->tx()->id().toHex(), *lKey));
							lExtension->second->processTransaction(ctx); // every direct transaction should be delivered (value)
						}
					} else {
						if (gLog().isEnabled(Log::NET)) 
							gLog().write(Log::NET, std::string("[peerManager]: ") + strprintf("peer extension was NOT FOUND %s for %s", *lKey, ctx->tx()->id().toHex()));					
					}
				}
			}
		}
	}

	void suspend() {
		//
		paused_ = true;

		//
		std::list<IPeerPtr> lPeersToRemove;
		for (int lIdx = 0; lIdx < contexts_.size(); lIdx++) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lIdx]);
			for (std::set<std::string /*endpoint*/>::iterator lPeer = active_[lIdx].begin(); lPeer != active_[lIdx].end(); lPeer++) {
				PeersMap::iterator lPeerPtr = peers_[lIdx].find(*lPeer);
				if (lPeerPtr != peers_[lIdx].end() && lPeerPtr->second->isOutbound() && !lPeerPtr->second->state()->client()) {
					//
					lPeersToRemove.push_back(lPeerPtr->second);
				}
			}
		}

		//
		for (std::list<IPeerPtr>::iterator lPeer = lPeersToRemove.begin(); lPeer != lPeersToRemove.end(); lPeer++) {
			//
			deactivatePeer(*lPeer);
			(*lPeer)->close();
		}
	}

	void resume() {
		paused_ = false;
	}

	void poll() {
		//
		for (std::vector<IOContextPtr>::iterator lCtx = contexts_.begin(); lCtx != contexts_.end(); lCtx++)	{
			//
			long lEvents = 0;
			int lCount = 0;
			do {
				(*lCtx)->reset();
				//run all ready handlers
				lEvents = (*lCtx)->poll();

			} while(lEvents > 0 && lCount++ < 10); // up to 10 events
		}
	}

	void run() {
		// open container
		openPeersContainer();

		// load peers
		gLog().write(Log::INFO, std::string("[peerManager]: loading peers..."));
		if (!gLightDaemon && !explicitPeersOnly_) {
			for (db::DbContainer<std::string /*endpoint*/, Peer::PersistentState>::Iterator lState = peersContainer_.begin(); lState.valid(); ++lState) {
				// process state
				std::string lKey;
				Peer::PersistentState lPersistentState;
				if (lState.first(lKey) && lState.second(lPersistentState)) {
					// make peer
					gLog().write(Log::INFO, std::string("[peerManager]: try to load ") + lKey);
					IPeerPtr lPeer = addPeerInternal(lKey, lPersistentState.type());
					if (lPeer) {
						// previous state
						lPeer->setState(State::instance(lPersistentState.state()));

						// check if address id is actual
						if (!lPeer->state()->address().valid()) continue;

						// process status
						switch(lPersistentState.status()) {
							case IPeer::QUARANTINE: quarantine(lPeer); break; // quarantine starts from beginning
							case IPeer::BANNED: ban(lPeer); break;
							// NOTICE: statuses below is for real processing only
							// case IPeer::ACTIVE: activate(lPeer); break;
							// case IPeer::PENDING_STATE: pending(lPeer); break;
							// case IPeer::POSTPONED: postpone(lPeer); break;
						}
					}
				}
			}
		}

		// create timers
		gLog().write(Log::INFO, std::string("[peerManager]: starting timers..."));
		int lId = 0;
		for (std::vector<IOContextPtr>::iterator lCtx = contexts_.begin(); lCtx != contexts_.end(); lCtx++, lId++) {
			TimerPtr lTimer(new boost::asio::steady_timer(
					getContext(lId), 
					boost::asio::chrono::seconds(settings_->isClient() ? 1 : 20))); // for node - time to warm-up (validator & sharding managers)
			lTimer->async_wait(boost::bind(&PeerManager::touch, shared_from_this(), lId, lTimer, boost::asio::placeholders::error));			
			timers_.push_back(lTimer);
		}

		// create a pool of threads to run all of the io_contexts
		gLog().write(Log::INFO, std::string("[peerManager]: starting contexts..."));
		for (std::vector<IOContextPtr>::iterator lCtx = contexts_.begin(); lCtx != contexts_.end(); lCtx++)	{
			boost::shared_ptr<boost::thread> lThread(
				new boost::thread(
					boost::bind(&PeerManager::processor, shared_from_this(), *lCtx)));
			threads_.push_back(lThread);
		}

		if (!settings_->isClient()) {
			// wait for all threads in the pool to exit
			for (std::vector<boost::shared_ptr<boost::thread> >::iterator lThread = threads_.begin(); lThread != threads_.end(); lThread++)
				(*lThread)->join();
		}
	}

	void stop() {
		gLog().write(Log::INFO, std::string("[peerManager]: stopping contexts..."));
		for (std::vector<IOContextPtr>::iterator lCtx = contexts_.begin(); lCtx != contexts_.end(); lCtx++)
			(*lCtx)->stop();

		if (!settings_->isClient()) {
			boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));

			// stop validators
			gLog().write(Log::INFO, std::string("[peerManager]: stopping validators..."));
			consensusManager_->validatorManager()->stop();

			// stop storages
			gLog().write(Log::INFO, std::string("[peerManager]: stopping storages..."));
			consensusManager_->storeManager()->stop();
		}

		// stop storages
		gLog().write(Log::INFO, std::string("[peerManager]: closing wallet..."));
		consensusManager_->wallet()->close();		
	}

	boost::asio::io_context& getContext(int id) {
		return *contexts_[id];
	}

	int getContextId() {
		// lock peers - use the same mutex as for the peers
		boost::unique_lock<boost::recursive_mutex> lLock(nextContextMutex_);

		int lId = nextContext_++;
		
		if (nextContext_ == contexts_.size()) nextContext_ = 0;
		return lId;
	}

	void activePeers(std::vector<std::string>& peers) {
		//
		for (int lIdx = 0; lIdx < contexts_.size(); lIdx++) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lIdx]);
			for (std::set<std::string /*endpoint*/>::iterator lPeer = active_[lIdx].begin(); lPeer != active_[lIdx].end(); lPeer++) {
				PeersMap::iterator lPeerPtr = peers_[lIdx].find(*lPeer);
				if (lPeerPtr != peers_[lIdx].end() && lPeerPtr->second->isOutbound() && !lPeerPtr->second->state()->client()) 
					peers.push_back(*lPeer);
			}

			for (std::set<std::string /*endpoint*/>::iterator lPeer = explicit_[lIdx].begin(); lPeer != explicit_[lIdx].end(); lPeer++) {
				PeersMap::iterator lPeerPtr = peers_[lIdx].find(*lPeer);
				if (lPeerPtr != peers_[lIdx].end()) 
					peers.push_back((*lPeer) + ":e"); // explicit mark
			}
		}
	}

	void explicitPeers(std::list<IPeerPtr>& peers) {
		//
		for (int lIdx = 0; lIdx < contexts_.size(); lIdx++) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lIdx]);
			for (std::set<std::string /*endpoint*/>::iterator lPeer = explicit_[lIdx].begin(); lPeer != explicit_[lIdx].end(); lPeer++) {
				PeersMap::iterator lPeerPtr = peers_[lIdx].find(*lPeer);
				if (lPeerPtr != peers_[lIdx].end()) 
					peers.push_back(lPeerPtr->second);
			}
		}
	}

	bool existsExplicit(const std::string& key) {
		//
		// NOTICE: qbit protocol changes
		// - in case of full nodes client connectivity and node services is mandatory
		//
		const uint64_t CHANGE_PROTO_TIME_0 = 1614422850; // in seconds since 1614163650 + 3 days
		if (qbit::getTime() > CHANGE_PROTO_TIME_0) {
			//
			std::vector<std::string> lParts;
			boost::split(lParts, key, boost::is_any_of(":"), boost::token_compress_on);

			//
			bool lResult = false;
			if (lParts.size() > 1) {
				//
				boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
				std::set<std::string>::iterator lEndpoint = explicitEndpoins_.find(lParts[0]);
				if (lEndpoint != explicitEndpoins_.end()) {
					// just potencially reachable address
					lResult = true;
				}
			}

			return lResult;	
		}

		return true;
	}

	static void registerPeerExtension(const std::string& dappName, PeerExtensionCreatorPtr extension) {
		//
		gPeerExtensions.insert(std::map<std::string /*dapp name*/, PeerExtensionCreatorPtr>::value_type(
			dappName,
			extension));
	}

	PeerExtensionCreatorPtr locateExtensionCreator(const std::string& dappName) {
		//
		std::map<std::string /*dapp name*/, PeerExtensionCreatorPtr>::iterator lItem = gPeerExtensions.find(dappName);
		if (lItem != gPeerExtensions.end())
			return lItem->second;

		return nullptr;
	}

	void allPeers(std::list<IPeerPtr>& peers) {
		//
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[lChannel->first]);
			for (std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeer = lChannel->second.begin(); lPeer != lChannel->second.end(); lPeer++) {
				peers.push_back(lPeer->second);
			}
		}
	}

	void useExplicitPeersOnly() { explicitPeersOnly_ = true; }
	bool explicitPeersOnly() { return explicitPeersOnly_; }

private:
	void openPeersContainer() {
		// open container
		if (!gLightDaemon && !peersContainer_.opened()) {
			try {
				mkpath(std::string(settings_->dataPath() + "/peers").c_str(), 0777);

				gLog().write(Log::INFO, std::string("[peerManager]: opening peers container..."));
				peersContainer_.open();
			}
			catch(const std::exception& ex) {
				gLog().write(Log::GENERAL_ERROR, std::string("[peerManager/run]: ") + ex.what());
				return;
			}
		}
	}

	void processor(std::shared_ptr<boost::asio::io_context> ctx) {
		gLog().write(Log::INFO, std::string("[peerManager]: context run..."));
		while(true) {
			try {
				ctx->run();
				break;
			} 
			catch(qbit::exception& ex) {
				gLog().write(Log::GENERAL_ERROR, std::string("[peerManager]: qbit error -> ") + ex.what());
			}
			catch(boost::system::system_error& ex) {
				gLog().write(Log::GENERAL_ERROR, std::string("[peerManager]: context error -> ") + ex.what());
			}
			catch(std::runtime_error& ex) {
				gLog().write(Log::GENERAL_ERROR, std::string("[peerManager]: runtime error -> ") + ex.what());
			}
		}
		gLog().write(Log::INFO, std::string("[peerManager]: context stop."));
	}

	void touch(int id, std::shared_ptr<boost::asio::steady_timer> timer, const boost::system::error_code& error) {
		// touch active peers
		if (!paused_) {
			if (!error) {
				boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[id]);
				for (std::set<std::string /*endpoint*/>::iterator lPeer = inactive_[id].begin(); lPeer != inactive_[id].end(); lPeer++) {
					IPeerPtr lPeerPtr = peers_[id][*lPeer];
					if (lPeerPtr->status() != IPeer::POSTPONED || (lPeerPtr->status() == IPeer::POSTPONED && lPeerPtr->postponedTick()))
						lPeerPtr->connect();
				}

				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager/touch]: active count = ") + strprintf("%d", active_[id].size()));
				for (std::set<std::string /*endpoint*/>::iterator lPeer = active_[id].begin(); lPeer != active_[id].end(); lPeer++) {
					// TODO?: DAEMON - no need to support latency updates
					if (!peers_[id][*lPeer]->state()->daemon())	peers_[id][*lPeer]->ping();
				}
			} else {
				// log
				gLog().write(Log::GENERAL_ERROR, std::string("[peerManager/touch]: ") + error.message());
			}
		}

		timer->expires_at(timer->expiry() + (settings_->isDaemon() ? boost::asio::chrono::seconds(6) : boost::asio::chrono::seconds(2)));
		timer->async_wait(boost::bind(&PeerManager::touch, shared_from_this(), id, timer, boost::asio::placeholders::error));
	}

public:
	void quarantine(IPeerPtr peer) {
		bool lPop = false;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
			std::map<uint160, std::set<std::string>>::iterator lPeerIter = peerIdx_.find(peer->addressId());
			if (lPeerIter != peerIdx_.end() && lPeerIter->second.find(peer->key()) != lPeerIter->second.end()) {
				lPeerIter->second.erase(peer->key());
				if (!lPeerIter->second.size()) peerIdx_.erase(peer->addressId());
				if (peer->state()->client()) {
					clients_.erase(peer->addressId());
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: removing client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));					
				}

				lPop = true;
			}
		}

		{
			if (peer->state()->client()) {
				removePeer(peer);
			} else {
				boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[peer->contextId()]);
				peer->toQuarantine(consensusManager_->currentState()->height() + consensusManager_->quarantineTime());
				active_[peer->contextId()].erase(peer->key());
				quarantine_[peer->contextId()].insert(peer->key());
				inactive_[peer->contextId()].erase(peer->key());
			}
		}

		// pop from consensus
		consensusManager_->popPeer(peer);

		// update peers db
		updatePeer(peer);

		//
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::INFO, std::string("[peerManager]: quarantine peer ") + peer->key());
	}

	void release(const std::string& endpoint) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
		bannedEndpoins_.erase(endpoint);
	}

	void ban(IPeerPtr peer) {
		bool lPop = false;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
			std::map<uint160, std::set<std::string>>::iterator lPeerIter = peerIdx_.find(peer->addressId());
			if (lPeerIter != peerIdx_.end() && lPeerIter->second.find(peer->key()) != lPeerIter->second.end()) {
				lPeerIter->second.erase(peer->key());
				if (!lPeerIter->second.size()) peerIdx_.erase(peer->addressId());
				if (peer->state()->client()) {
					clients_.erase(peer->addressId());
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: removing client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));					
				}

				lPop = true;
			}
		}

		{
			if (peer->state()->client()) {
				removePeer(peer);
			} else {
				{
					boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[peer->contextId()]);
					peer->toBan();
					quarantine_[peer->contextId()].erase(peer->key());
					active_[peer->contextId()].erase(peer->key());
					banned_[peer->contextId()].insert(peer->key());
					inactive_[peer->contextId()].erase(peer->key());
				}

				{
					boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
					std::vector<std::string> lParts;
					boost::split(lParts, peer->key(), boost::is_any_of(":"), boost::token_compress_on);

					if (lParts.size() == 2) {
						bannedEndpoins_.insert(lParts[0]); // push host
					}
				}
			}
		}

		// pop from consensus
		consensusManager_->popPeer(peer);

		// update peers db
		updatePeer(peer);

		//
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::INFO, std::string("[peerManager]: ban peer ") + peer->key());
	}

	void postpone(IPeerPtr peer) {
		peer->toPostponed();
	}

	bool activate(IPeerPtr peer, bool force = false) {
		uint160 lAddress = peer->addressId();
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
			std::map<uint160, std::set<std::string>>::iterator lPeerIndex = peerIdx_.find(lAddress);
			//
			if (lPeerIndex == peerIdx_.end() || (peer->state()->client() /*&& lPeerIndex->second != peer->key()*/)) {
				// ONLY in case if peer is CLIENT
				if (lPeerIndex != peerIdx_.end()) {
					// try to cleen-up
					std::set<std::string> lKeys = lPeerIndex->second; // copy
					for (std::set<std::string>::iterator lKey = lKeys.begin(); lKey != lKeys.end(); lKey++) {
						//
						if (locate(*lKey) == nullptr) lPeerIndex->second.erase(*lKey);
					}

					//
					/*
					IPeerPtr lPrevPeer = locate(lPeerIndex->second);
					if (lPrevPeer) removePeer(lPrevPeer);
					else {
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: peer is ABSENT ") + 
							strprintf("%s to change on %s", lPeerIndex->second, peer->key()));
					}

					peerIdx_.erase(lPeerIndex);
					*/
				}
				
				// add new one or another one
				peerIdx_[peer->addressId()].insert(peer->key());
				
				//
				if (peer->state()->client()) { 
					clients_.insert(peer->addressId());
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: adding client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));
					//
					for (std::vector<State::DAppInstance>::const_iterator 
							lInstance = peer->state()->dApps().begin(); lInstance != peer->state()->dApps().end(); lInstance++) {
						//
						if (!peer->extension(lInstance->name())) {
							PeerExtensionCreatorPtr lCreator = locateExtensionCreator(lInstance->name());
							if (lCreator) {
								peer->setExtension(lInstance->name(), lCreator->create(*lInstance, peer, shared_from_this()));
							}
						}
					}
				} else {
					//
					for (std::vector<State::BlockInfo>::iterator lInfo = peer->state()->infos().begin(); lInfo != peer->state()->infos().end(); lInfo++) {
						//
						if (lInfo->dApp().size() && lInfo->dApp() != "none") {
							if (!peer->extension(lInfo->dApp())) {
								//
								if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: adding peer extension ") + strprintf("%s/%s/%s", lInfo->dApp(), peer->key(), peer->addressId().toHex()));
								PeerExtensionCreatorPtr lCreator = locateExtensionCreator(lInfo->dApp());
								if (lCreator) {
									peer->setExtension(lInfo->dApp(), lCreator->create(State::DAppInstance(lInfo->dApp(), uint256()), peer, shared_from_this()));
								}
							}
						}
					}
				}
			} else {
				if (lPeerIndex != peerIdx_.end() && lPeerIndex->second.size() /*&& !force*/) {
					IPeerPtr lPeer = locate(*(lPeerIndex->second.begin()));
					if (lPeer) {
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: peer ALREADY exists - ") + 
								strprintf("new - %s/%s/%s, old - %s/%s/%s", 
									peer->key(), peer->statusString(), peer->addressId().toHex(), 
									lPeer->key(), lPeer->statusString(), lPeer->addressId().toHex()));

						lPeer->toActive();
						if (!consensusManager_->peerExists(lAddress) || settings_->isClient()) consensusManager_->pushPeer(lPeer);
						else consensusManager_->pushPeerLatency(lPeer);
						return false;
					}
				} /*else {
					if (lPeerIndex != peerIdx_.end() && force) {
						// deactivate old
						IPeerPtr lPeer = locate(*(lPeerIndex->second.begin()));
						if (lPeer && lPeer->key() != peer->key()) {
							deactivatePeer(lPeer);
							// add new one
							peerIdx_[peer->addressId()].insert(peer->key());
							//
							for (std::vector<State::BlockInfo>::iterator lInfo = peer->state()->infos().begin(); lInfo != peer->state()->infos().end(); lInfo++) {
								//
								if (lInfo->dApp().size() && lInfo->dApp() != "none") {
									if (!peer->extension(lInfo->dApp())) {
										//
										if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: adding peer extension ") + strprintf("%s/%s/%s", lInfo->dApp(), peer->key(), peer->addressId().toHex()));
										PeerExtensionCreatorPtr lCreator = locateExtensionCreator(lInfo->dApp());
										if (lCreator) {
											peer->setExtension(lInfo->dApp(), lCreator->create(State::DAppInstance(lInfo->dApp(), uint256()), peer, shared_from_this()));
										}
									}
								}
							}
						} else return false;
					}
				} */
			}
		}

		{
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[peer->contextId()]);
			peer->toActive();

			if (peers_[peer->contextId()].find(peer->key()) != peers_[peer->contextId()].end()) {
				peers_[peer->contextId()][peer->key()]->release();
				peers_[peer->contextId()].erase(peer->key());
			}

			peers_[peer->contextId()].insert(std::map<std::string /*endpoint*/, IPeerPtr>::value_type(peer->key(), peer));
			active_[peer->contextId()].insert(peer->key());
			inactive_[peer->contextId()].erase(peer->key());
		}

		// push to consensus
		if (!consensusManager_->pushPeer(peer)) {
			deactivatePeer(peer);
			return false;
		}

		// update peers db
		updatePeer(peer);

		//
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::INFO, std::string("[peerManager]: activate peer ") + peer->key());

		return true;
	}

	void add(IPeerPtr peer) {
		{
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[peer->contextId()]);
			if (peers_[peer->contextId()].find(peer->key()) != peers_[peer->contextId()].end()) {
				peers_[peer->contextId()][peer->key()]->release();
				peers_[peer->contextId()].erase(peer->key());
			}
			
			peers_[peer->contextId()].insert(std::map<std::string /*endpoint*/, IPeerPtr>::value_type(peer->key(), peer));
			inactive_[peer->contextId()].insert(peer->key());
			if (peer->type() == IPeer::Type::EXPLICIT) 
				explicit_[peer->contextId()].insert(peer->key());
		}

		if (peer->type() == IPeer::Type::EXPLICIT) {
			//
			std::vector<std::string> lParts;
			boost::split(lParts, peer->key(), boost::is_any_of(":"), boost::token_compress_on);

			if (lParts.size() > 1) {
				boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
				explicitEndpoins_.insert(lParts[0]);
				//
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: explicit peer added ") + peer->key() + " / " + lParts[0]);
			}
		}

		//
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: peer added ") + peer->key());

		// update peers db
		openPeersContainer(); // if not running
		updatePeer(peer);
	}

	void pending(IPeerPtr peer) {
		bool lPop = false;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
			std::map<uint160, std::set<std::string>>::iterator lPeerIter = peerIdx_.find(peer->addressId());
			if (lPeerIter != peerIdx_.end() && lPeerIter->second.find(peer->key()) != lPeerIter->second.end()) {
				lPeerIter->second.erase(peer->key());
				if (!lPeerIter->second.size()) peerIdx_.erase(peer->addressId());
				if (peer->state()->client()) { 
					clients_.erase(peer->addressId()); 
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: removing client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));
				}

				lPop = true;
			}
		}

		{
			if (peer->state()->client()) {
				removePeer(peer);
			} else {
				boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[peer->contextId()]);
				if (peer->status() == IPeer::BANNED) return;
				if (peer->status() != IPeer::POSTPONED) peer->toPendingState();

				if (peers_[peer->contextId()].find(peer->key()) != peers_[peer->contextId()].end()) {
					peers_[peer->contextId()][peer->key()]->release();
					peers_[peer->contextId()].erase(peer->key());
				}

				peers_[peer->contextId()].insert(std::map<std::string /*endpoint*/, IPeerPtr>::value_type(peer->key(), peer));
				inactive_[peer->contextId()].insert(peer->key());
				active_[peer->contextId()].erase(peer->key());
			}
		}

		// pop from consensus
		consensusManager_->popPeer(peer);

		// update peers db
		updatePeer(peer);

		//
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: pending state ") + peer->key());
	}

	bool update(IPeerPtr peer, bool force = false) {
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
			uint160 lAddress = peer->addressId();
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: updating - ") + strprintf("%s/%s", peer->key(), lAddress.toHex()));
			std::map<uint160, std::set<std::string>>::iterator lPeerIndex = peerIdx_.find(lAddress);
			//
			if (lPeerIndex == peerIdx_.end() || (peer->state()->client() /*&& lPeerIndex->second != peer->key()*/)) {
				if (lPeerIndex != peerIdx_.end()) {
					// try to cleen-up
					std::set<std::string> lKeys = lPeerIndex->second; // copy
					for (std::set<std::string>::iterator lKey = lKeys.begin(); lKey != lKeys.end(); lKey++) {
						//
						if (locate(*lKey) == nullptr) lPeerIndex->second.erase(*lKey);
					}

					/*
					IPeerPtr lPrevPeer = locate(lPeerIndex->second);
					if (lPrevPeer) removePeer(lPrevPeer);
					else {
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: peer is ABSENT ") + 
							strprintf("%s to change on %s", lPeerIndex->second, peer->key()));
					}

					peerIdx_.erase(lPeerIndex);
					*/
				}
				
				// add new one or another one
				peerIdx_[peer->addressId()].insert(peer->key());

				//
				if (peer->state()->client()) { 
					clients_.insert(peer->addressId());
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: adding client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));

					//
					for (std::vector<State::DAppInstance>::const_iterator 
							lInstance = peer->state()->dApps().begin(); lInstance != peer->state()->dApps().end(); lInstance++) {
						//
						if (!peer->extension(lInstance->name())) {
							PeerExtensionCreatorPtr lCreator = locateExtensionCreator(lInstance->name());
							if (lCreator) {
								peer->setExtension(lInstance->name(), lCreator->create(*lInstance, peer, shared_from_this()));
							}
						}
					}
				} else {
					//
					for (std::vector<State::BlockInfo>::iterator lInfo = peer->state()->infos().begin(); lInfo != peer->state()->infos().end(); lInfo++) {
						//
						if (lInfo->dApp().size() && lInfo->dApp() != "none") {
							if (!peer->extension(lInfo->dApp())) {
								//
								if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: adding peer extension ") + strprintf("%s/%s/%s", lInfo->dApp(), peer->key(), peer->addressId().toHex()));
								PeerExtensionCreatorPtr lCreator = locateExtensionCreator(lInfo->dApp());
								if (lCreator) {
									peer->setExtension(lInfo->dApp(), lCreator->create(State::DAppInstance(lInfo->dApp(), uint256()), peer, shared_from_this()));
								}
							}
						}
					}
				}
			} else {
				if (lPeerIndex != peerIdx_.end() && lPeerIndex->second.size() /*&& !force*/) {
					IPeerPtr lPeer = locate(*(lPeerIndex->second.begin()));
					if (lPeer) {
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: peer ALREADY exists - ") + 
								strprintf("new - %s/%s/%s, old - %s/%s/%s", 
									peer->key(), peer->statusString(), peer->addressId().toHex(), 
									lPeer->key(), lPeer->statusString(), lPeer->addressId().toHex()));

						lPeer->toActive();
						if (!consensusManager_->peerExists(lAddress) || settings_->isClient()) consensusManager_->pushPeer(lPeer);
						else consensusManager_->pushPeerLatency(lPeer);
						return false;
					}
				} /*else {
					if (lPeerIndex != peerIdx_.end() && force) {
						// deactivate old
						IPeerPtr lPeer = locate(*(lPeerIndex->second.begin()));
						if (lPeer && lPeer->key() != peer->key()) {
							deactivatePeer(lPeer);
							// add new one
							peerIdx_[peer->addressId()].insert(peer->key());
							//
							for (std::vector<State::BlockInfo>::iterator lInfo = peer->state()->infos().begin(); lInfo != peer->state()->infos().end(); lInfo++) {
								//
								if (lInfo->dApp().size() && lInfo->dApp() != "none") {
									if (!peer->extension(lInfo->dApp())) {
										//
										if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: adding peer extension ") + strprintf("%s/%s/%s", lInfo->dApp(), peer->key(), peer->addressId().toHex()));
										PeerExtensionCreatorPtr lCreator = locateExtensionCreator(lInfo->dApp());
										if (lCreator) {
											peer->setExtension(lInfo->dApp(), lCreator->create(State::DAppInstance(lInfo->dApp(), uint256()), peer, shared_from_this()));
										}
									}
								}
							}
						} else return false;
					}
				} */
			}
		}

		{
			boost::unique_lock<boost::recursive_mutex> lLock(contextMutex_[peer->contextId()]);
			peer->toActive();

			if (peers_[peer->contextId()].find(peer->key()) == peers_[peer->contextId()].end())
				peers_[peer->contextId()].insert(std::map<std::string /*endpoint*/, IPeerPtr>::value_type(peer->key(), peer));
			
			active_[peer->contextId()].insert(peer->key());
			quarantine_[peer->contextId()].erase(peer->key());
			banned_[peer->contextId()].erase(peer->key());
			inactive_[peer->contextId()].erase(peer->key());
		}

		// push to consensus
		if (!consensusManager_->pushPeer(peer)) {
			deactivatePeer(peer);
			return false;
		}

		// update peers db
		updatePeer(peer);

		//
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: update peer ") + peer->key());

		return true;
	}

	void deactivate(IPeerPtr peer) {
		bool lPop = false;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersIdxMutex_);
			std::map<uint160, std::set<std::string>>::iterator lPeerIter = peerIdx_.find(peer->addressId());
			if (lPeerIter != peerIdx_.end() && lPeerIter->second.find(peer->key()) != lPeerIter->second.end()) {
				lPeerIter->second.erase(peer->key());
				if (!lPeerIter->second.size()) peerIdx_.erase(peer->addressId());
				if (peer->state()->client()) {
					clients_.erase(peer->addressId());
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: removing client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));					
				}

				lPop = true;
			}
		}

		// pop from manager
		consensusManager_->popPeer(peer);

		// update peers db
		updatePeer(peer);

		// inbound connections only
		if (!peer->isOutbound() /*double check*/) removePeer(peer);

		//
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::INFO, std::string("[peerManager]: peer deactivated ") + peer->key());
	}

	void updatePeer(IPeerPtr peer) {
		if (!gLightDaemon && peer->isOutbound()) {
			peersContainer_.write(peer->key(), Peer::PersistentState(*peer->state(), peer->status(), peer->type()));
		}
	}

	uint32_t clients() { return clients_.size(); }

	void incPeersCount() { peersCount_++; }
	void decPeersCount() { peersCount_--; }
	int peersCount() { return peersCount_; }

private:
	typedef std::map<std::string /*endpoint*/, IPeerPtr> PeersMap;
	typedef std::set<std::string /*endpoint*/> PeersSet;
	typedef std::shared_ptr<boost::asio::io_context> IOContextPtr;
	typedef std::shared_ptr<boost::asio::steady_timer> TimerPtr;
	typedef boost::asio::executor_work_guard<boost::asio::io_context::executor_type> IOContextWork;

	boost::recursive_mutex nextContextMutex_;
	std::map<int, PeersMap> peers_; // peers by context id's

	boost::ptr_vector<boost::recursive_mutex> contextMutex_;
	std::map<int, PeersSet> active_;
	std::map<int, PeersSet> quarantine_;
	std::map<int, PeersSet> banned_;
	std::map<int, PeersSet> inactive_;
	std::map<int, PeersSet> explicit_;	
	std::set<std::string> bannedEndpoins_;
	std::set<std::string> explicitEndpoins_;

	boost::recursive_mutex peersIdxMutex_;
	std::map<uint160, std::set<std::string>> peerIdx_;
	std::set<uint160> clients_;

	std::vector<TimerPtr> timers_;
	std::vector<IOContextPtr> contexts_;
	std::list<IOContextWork> work_;
	std::vector<boost::shared_ptr<boost::thread> > threads_;
	int nextContext_ = 0;

	int peersCount_ = 0;

	ISettingsPtr settings_;
	IConsensusManagerPtr consensusManager_;
	IMemoryPoolManagerPtr mempoolManager_;

	db::DbContainer<std::string /*endpoint*/, Peer::PersistentState> peersContainer_;
	std::map<std::string, uint512> peerStateSent_;

	bool paused_ = false;
	bool explicitPeersOnly_ = false;
};

}

#endif
