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

			contextMutex_.push_back(new boost::mutex());
		}
	}

	inline bool exists(const std::string& endpoint) {
		// traverse
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::mutex> lLock(contextMutex_[lChannel->first]);
			std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != lChannel->second.end()) return true;
		}

		return false;
	}

	inline bool existsBanned(const std::string& endpoint) {
		// traverse
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::mutex> lLock(contextMutex_[lChannel->first]);
			std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != lChannel->second.end() && banned_[lChannel->first].find(endpoint) != banned_[lChannel->first].end())
				return true;
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
			boost::unique_lock<boost::mutex> lLock(contextMutex_[lChannel->first]);
			std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != lChannel->second.end() && quarantine_[lChannel->first].find(endpoint) != quarantine_[lChannel->first].end())
				return true;
		}
		
		return false;
	}

	inline bool existsBannedOrQuarantine(const std::string& endpoint) {
		// traverse
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::mutex> lLock(contextMutex_[lChannel->first]);
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
			boost::unique_lock<boost::mutex> lLock(contextMutex_[lChannel->first]);
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
		int lPeers = 0;

		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::mutex> lLock(contextMutex_[lChannel->first]);

			for (std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeer = lChannel->second.begin(); lPeer != lChannel->second.end(); lPeer++) {
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
			else deactivate(lPeer); // deactivate if inbound
		}
	}

	void removePeer(IPeerPtr peer) {
		if (peer) {
			boost::unique_lock<boost::mutex> lLock(contextMutex_[peer->contextId()]);

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

	bool updatePeerState(IPeerPtr peer, State& state, IPeer::UpdatePeerResult& peerResult) {
		// forward
		peerResult = IPeer::SUCCESSED;

		// sanity check
		StatePtr lOwnState = consensusManager()->currentState();
		if (lOwnState->address().id() == state.address().id()) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: self connection ") + lOwnState->address().id().toHex() + " / " + state.address().id().toHex());
			ban(peer);
			peerResult = IPeer::BAN;
			return false;
		}

		//
		IPeerPtr lPeer = peer;

		// check
		if (state.client() && lOwnState->nodeOrFullNode()) {
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
			if (lPeer->status() != IPeer::UNDEFINED && lPeer->address().id() != state.address().id()) {
				quarantine(lPeer);
				return false;
			}

			// check new state
			if (state.valid()) {
				lPeer->setState(state);

				// check for quarantine
				if (lPeer->onQuarantine())
					return false;

				// update state and data
				bool lResult = update(lPeer);
				if (!lResult) peerResult = IPeer::EXISTS;
				return lResult;
			} else {
				ban(lPeer);
				return false;
			}
		// peer is new - inbound only
		} else {
			if (state.valid()) {
				lPeer->setState(state);
				bool lResult = activate(lPeer);
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

	IPeerPtr addPeer(const std::string& endpoint) {
		if (locate(endpoint) == nullptr) {
			std::vector<std::string> lParts;
			boost::split(lParts, endpoint, boost::is_any_of(":"), boost::token_compress_on);

			if (lParts.size() == 2) {
				IPeerPtr lPeer = Peer::instance(getContextId(), endpoint, std::static_pointer_cast<IPeerManager>(shared_from_this()));
				add(lPeer);

				return lPeer;
			}
		}

		return nullptr;
	}

	IPeerPtr locate(const std::string& endpoint) {
		// traverse
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			boost::unique_lock<boost::mutex> lLock(contextMutex_[lChannel->first]);
			std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != lChannel->second.end()) return lPeerIter->second;
		}

		return nullptr;
	}	

	void notifyTransaction(TransactionContextPtr ctx) {
		//
		std::set<uint160> lClients;
		std::map<uint160, std::string> lPeerIdx;
		{
			boost::unique_lock<boost::mutex> lLock(peersIdxMutex_);
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

				std::map<uint160, std::string>::iterator lPeerIter = lPeerIdx.find(lAddressId);
				if (lPeerIter != lPeerIdx.end()) {
					//
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: found client index ") + strprintf("%s", lAddressId.toHex()));

					IPeerPtr lPeer = locate(lPeerIter->second);
					if (lPeer) {
						lPeer->broadcastTransaction(ctx); // every direct transaction should be delivered (value)
					} else {
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: peer NOT found for ") + strprintf("%s", lAddressId.toHex()));
					}
				}
			}	
		}

		// traverse peers
		for (std::set<uint160>::iterator lClient = lClients.begin(); lClient != lClients.end(); lClient++) {
			std::map<uint160, std::string>::iterator lPeerIter = lPeerIdx.find(*lClient);
			if (lPeerIter != lPeerIdx.end()) {
				IPeerPtr lPeer = locate(lPeerIter->second);
				if (lPeer) {
					std::map<std::string, IPeerExtensionPtr> lExtensions = lPeer->extensions();
					for (std::map<std::string, IPeerExtensionPtr>::iterator lExtension = lExtensions.begin(); lExtension != lExtensions.end(); lExtension++) {
						if (gLog().isEnabled(Log::NET)) 
							gLog().write(Log::NET, std::string("[peerManager]: processing extension ") + strprintf("tx %s for peer %s", ctx->tx()->id().toHex(), lPeerIter->second));
						lExtension->second->processTransaction(ctx); // every direct transaction should be delivered (value)
					}
				} else {
					if (gLog().isEnabled(Log::NET)) 
						gLog().write(Log::NET, std::string("[peerManager]: ") + strprintf("peer extension was NOT FOUND %s for %s", lPeerIter->second, ctx->tx()->id().toHex()));					
				}
			}
		}
	}

	void run() {
		// open container
		openPeersContainer();

		// load peers
		gLog().write(Log::INFO, std::string("[peerManager]: loading peers..."));
		for (db::DbContainer<std::string /*endpoint*/, Peer::PersistentState>::Iterator lState = peersContainer_.begin(); lState.valid(); ++lState) {
			// process state
			std::string lKey;
			Peer::PersistentState lPersistentState;
			if (lState.first(lKey) && lState.second(lPersistentState)) {
				// make peer
				IPeerPtr lPeer = addPeer(lKey);
				if (lPeer) {
					lPeer->setState(lPersistentState.state());

					// process status
					switch(lPersistentState.status()) {
						case IPeer::ACTIVE: activate(lPeer); break;
						case IPeer::QUARANTINE: quarantine(lPeer); break; // quarantine starts from beginning
						case IPeer::BANNED: ban(lPeer); break;
						case IPeer::PENDING_STATE: pending(lPeer); break;
						case IPeer::POSTPONED: postpone(lPeer); break;
					}
				}
			}
		}

		// create timers
		gLog().write(Log::INFO, std::string("[peerManager]: starting timers..."));
		int lId = 0;
		for (std::vector<IOContextPtr>::iterator lCtx = contexts_.begin(); lCtx != contexts_.end(); lCtx++, lId++) {
			TimerPtr lTimer(new boost::asio::steady_timer(getContext(lId), boost::asio::chrono::seconds(1)));
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
		boost::unique_lock<boost::mutex> lLock(nextContextMutex_);

		int lId = nextContext_++;
		
		if (nextContext_ == contexts_.size()) nextContext_ = 0;
		return lId;
	}

	void activePeers(std::vector<std::string>& peers) {
		//
		for (int lIdx = 0; lIdx < contexts_.size(); lIdx++) {
			boost::unique_lock<boost::mutex> lLock(contextMutex_[lIdx]);
			for (std::set<std::string /*endpoint*/>::iterator lPeer = active_[lIdx].begin(); lPeer != active_[lIdx].end(); lPeer++) {
				PeersMap::iterator lPeerPtr = peers_[lIdx].find(*lPeer);
				if (lPeerPtr != peers_[lIdx].end() && lPeerPtr->second->isOutbound() && !lPeerPtr->second->state().client()) 
					peers.push_back(*lPeer);
			}
		}
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
			boost::unique_lock<boost::mutex> lLock(contextMutex_[lChannel->first]);
			for (std::map<std::string /*endpoint*/, IPeerPtr>::iterator lPeer = lChannel->second.begin(); lPeer != lChannel->second.end(); lPeer++) {
				peers.push_back(lPeer->second);
			}
		}
	}

private:
	void openPeersContainer() {
		// open container
		if (!peersContainer_.opened()) {
			try {
				mkpath(std::string(settings_->dataPath() + "/peers").c_str(), 0777);

				gLog().write(Log::INFO, std::string("[peerManager]: opening peers container..."));
				peersContainer_.open();
			}
			catch(const std::exception& ex) {
				gLog().write(Log::ERROR, std::string("[peerManager/run]: ") + ex.what());
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
			catch(boost::system::system_error& ex) {
				gLog().write(Log::ERROR, std::string("[peerManager]: context error -> ") + ex.what());
			}
		}
		gLog().write(Log::INFO, std::string("[peerManager]: context stop."));
	}

	void touch(int id, std::shared_ptr<boost::asio::steady_timer> timer, const boost::system::error_code& error) {
		// touch active peers
		if(!error) {
			boost::unique_lock<boost::mutex> lLock(contextMutex_[id]);
			for (std::set<std::string /*endpoint*/>::iterator lPeer = inactive_[id].begin(); lPeer != inactive_[id].end(); lPeer++) {
				IPeerPtr lPeerPtr = peers_[id][*lPeer];
				if (lPeerPtr->status() != IPeer::POSTPONED || (lPeerPtr->status() == IPeer::POSTPONED && lPeerPtr->postponedTick()))
					lPeerPtr->connect();
			}

			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager/touch]: active count = ") + strprintf("%d", active_[id].size()));
			for (std::set<std::string /*endpoint*/>::iterator lPeer = active_[id].begin(); lPeer != active_[id].end(); lPeer++) {
				// TODO?
				peers_[id][*lPeer]->ping();
			}
		} else {
			// log
			gLog().write(Log::ERROR, std::string("[peerManager/touch]: ") + error.message());
		}

		timer->expires_at(timer->expiry() + boost::asio::chrono::seconds(2));
		timer->async_wait(boost::bind(&PeerManager::touch, shared_from_this(), id, timer, boost::asio::placeholders::error));
	}

public:
	void quarantine(IPeerPtr peer) {
		bool lPop = false;
		{
			boost::unique_lock<boost::mutex> lLock(peersIdxMutex_);
			std::map<uint160, std::string>::iterator lPeerIter = peerIdx_.find(peer->addressId());
			if (lPeerIter != peerIdx_.end() && lPeerIter->second == peer->key()) {
				peerIdx_.erase(peer->addressId());
				if (peer->state().client()) {
					clients_.erase(peer->addressId());
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: removing client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));					
				}

				lPop = true;
			}
		}

		{
			if (peer->state().client()) {
				removePeer(peer);
			} else {
				boost::unique_lock<boost::mutex> lLock(contextMutex_[peer->contextId()]);
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

	void ban(IPeerPtr peer) {
		bool lPop = false;
		{
			boost::unique_lock<boost::mutex> lLock(peersIdxMutex_);
			std::map<uint160, std::string>::iterator lPeerIter = peerIdx_.find(peer->addressId());
			if (lPeerIter != peerIdx_.end() && lPeerIter->second == peer->key()) {
				peerIdx_.erase(peer->addressId());
				if (peer->state().client()) {
					clients_.erase(peer->addressId());
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: removing client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));					
				}

				lPop = true;
			}
		}

		{
			if (peer->state().client()) {
				removePeer(peer);
			} else {
				boost::unique_lock<boost::mutex> lLock(contextMutex_[peer->contextId()]);
				peer->toBan();
				quarantine_[peer->contextId()].erase(peer->key());
				active_[peer->contextId()].erase(peer->key());
				banned_[peer->contextId()].insert(peer->key());
				inactive_[peer->contextId()].erase(peer->key());
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

	bool activate(IPeerPtr peer) {
		uint160 lAddress = peer->addressId();
		{
			boost::unique_lock<boost::mutex> lLock(peersIdxMutex_);
			std::map<uint160, std::string>::iterator lPeerIndex = peerIdx_.find(lAddress);
			//
			if (lPeerIndex == peerIdx_.end() || (peer->state().client() && lPeerIndex->second != peer->key())) {
				if (lPeerIndex != peerIdx_.end()) {
					IPeerPtr lPrevPeer = locate(lPeerIndex->second);
					if (lPrevPeer) removePeer(lPrevPeer);
					else {
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: peer is ABSENT ") + 
							strprintf("%s to change on %s", lPeerIndex->second, peer->key()));
					}

					peerIdx_.erase(lPeerIndex);
				}
				//
				peerIdx_.insert(std::map<uint160, std::string>::value_type(peer->addressId(), peer->key()));
				if (peer->state().client()) { 
					clients_.insert(peer->addressId());
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: adding client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));
					//
					if (!peer->extension(peer->state().dApp())) {
						PeerExtensionCreatorPtr lCreator = locateExtensionCreator(peer->state().dApp());
						if (lCreator) {
							peer->setExtension(peer->state().dApp(), lCreator->create(peer, shared_from_this()));
						}
					}
				} else {
					//
					for (std::vector<State::BlockInfo>::iterator lInfo = peer->state().infos().begin(); lInfo != peer->state().infos().end(); lInfo++) {
						//
						if (lInfo->dApp().size() && lInfo->dApp() != "none") {
							if (!peer->extension(lInfo->dApp())) {
								//
								if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: adding peer extension ") + strprintf("%s/%s/%s", lInfo->dApp(), peer->key(), peer->addressId().toHex()));
								PeerExtensionCreatorPtr lCreator = locateExtensionCreator(lInfo->dApp());
								if (lCreator) {
									peer->setExtension(lInfo->dApp(), lCreator->create(peer, shared_from_this()));
								}
							}
						}
					}
				}
			} else {
				IPeerPtr lPeer = locate(peerIdx_[lAddress]);
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: peer ALREADY exists - ") + 
						strprintf("new - %s/%s/%s, old - %s/%s/%s", 
							peer->key(), peer->statusString(), peer->addressId().toHex(), 
							lPeer->key(), lPeer->statusString(), lPeer->addressId().toHex()));

				if (!consensusManager_->peerExists(lAddress)) consensusManager_->pushPeer(lPeer);					
				return false;
			}
		}

		{
			boost::unique_lock<boost::mutex> lLock(contextMutex_[peer->contextId()]);
			peer->toActive();

			if (peers_[peer->contextId()].find(peer->key()) != peers_[peer->contextId()].end()) {
				peers_[peer->contextId()][peer->key()]->release();
				peers_[peer->contextId()].erase(peer->key());
			}

			peers_[peer->contextId()].insert(std::map<std::string /*endpoint*/, IPeerPtr>::value_type(peer->key(), peer));
			peers_[peer->contextId()].insert(std::map<std::string /*endpoint*/, IPeerPtr>::value_type(peer->key(), peer));
			active_[peer->contextId()].insert(peer->key());
			inactive_[peer->contextId()].erase(peer->key());
		}

		// push to consensus
		if (!consensusManager_->pushPeer(peer)) {
			deactivate(peer);
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
			boost::unique_lock<boost::mutex> lLock(contextMutex_[peer->contextId()]);
			if (peers_[peer->contextId()].find(peer->key()) != peers_[peer->contextId()].end()) {
				peers_[peer->contextId()][peer->key()]->release();
				peers_[peer->contextId()].erase(peer->key());
			}
			
			peers_[peer->contextId()].insert(std::map<std::string /*endpoint*/, IPeerPtr>::value_type(peer->key(), peer));
			inactive_[peer->contextId()].insert(peer->key());
		}

		// update peers db
		openPeersContainer(); // if not running
		updatePeer(peer);
	}

	void pending(IPeerPtr peer) {
		bool lPop = false;
		{
			boost::unique_lock<boost::mutex> lLock(peersIdxMutex_);
			std::map<uint160, std::string>::iterator lPeerIter = peerIdx_.find(peer->addressId());
			if (lPeerIter != peerIdx_.end() && lPeerIter->second == peer->key()) {
				peerIdx_.erase(peer->addressId());
				if (peer->state().client()) { 
					clients_.erase(peer->addressId()); 
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: removing client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));
				}

				lPop = true;
			}
		}

		{
			if (peer->state().client()) {
				removePeer(peer);
			} else {
				boost::unique_lock<boost::mutex> lLock(contextMutex_[peer->contextId()]);
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

	bool update(IPeerPtr peer) {
		{
			boost::unique_lock<boost::mutex> lLock(peersIdxMutex_);
			uint160 lAddress = peer->addressId();
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: updating - ") + strprintf("%s/%s", peer->key(), lAddress.toHex()));
			std::map<uint160, std::string>::iterator lPeerIndex = peerIdx_.find(lAddress);
			//
			if (lPeerIndex == peerIdx_.end() || (peer->state().client() && lPeerIndex->second != peer->key())) {
				if (lPeerIndex != peerIdx_.end()) {
					IPeerPtr lPrevPeer = locate(lPeerIndex->second);
					if (lPrevPeer) removePeer(lPrevPeer);
					else {
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: peer is ABSENT ") + 
							strprintf("%s to change on %s", lPeerIndex->second, peer->key()));
					}

					peerIdx_.erase(lPeerIndex);
				}
				//
				peerIdx_.insert(std::map<uint160, std::string>::value_type(peer->addressId(), peer->key()));
				if (peer->state().client()) { 
					clients_.insert(peer->addressId());
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: adding client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));
					//
					if (!peer->extension(peer->state().dApp())) {
						PeerExtensionCreatorPtr lCreator = locateExtensionCreator(peer->state().dApp());
						if (lCreator) {
							peer->setExtension(peer->state().dApp(), lCreator->create(peer, shared_from_this()));
						}
					}
				} else {
					//
					for (std::vector<State::BlockInfo>::iterator lInfo = peer->state().infos().begin(); lInfo != peer->state().infos().end(); lInfo++) {
						//
						if (lInfo->dApp().size() && lInfo->dApp() != "none") {
							if (!peer->extension(lInfo->dApp())) {
								//
								if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: adding peer extension ") + strprintf("%s/%s/%s", lInfo->dApp(), peer->key(), peer->addressId().toHex()));
								PeerExtensionCreatorPtr lCreator = locateExtensionCreator(lInfo->dApp());
								if (lCreator) {
									peer->setExtension(lInfo->dApp(), lCreator->create(peer, shared_from_this()));
								}
							}
						}
					}
				}
			} else {
				IPeerPtr lPeer = locate(peerIdx_[lAddress]);
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: peer ALREADY exists - ") + 
						strprintf("new - %s/%s/%s, old - %s/%s/%s", 
							peer->key(), peer->statusString(), peer->addressId().toHex(), 
							lPeer->key(), lPeer->statusString(), lPeer->addressId().toHex()));

				if (!consensusManager_->peerExists(lAddress)) consensusManager_->pushPeer(lPeer);
				return false;
			}
		}

		{
			boost::unique_lock<boost::mutex> lLock(contextMutex_[peer->contextId()]);
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
			deactivate(peer);
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
			boost::unique_lock<boost::mutex> lLock(peersIdxMutex_);
			std::map<uint160, std::string>::iterator lPeerIter = peerIdx_.find(peer->addressId());
			if (lPeerIter != peerIdx_.end() && lPeerIter->second == peer->key()) {
				peerIdx_.erase(peer->addressId());
				if (peer->state().client()) {
					clients_.erase(peer->addressId());
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peerManager]: removing client ") + strprintf("%s/%s", peer->key(), peer->addressId().toHex()));					
				}

				lPop = true;
			}
		}

		{
			if (peer->state().client()) {
				removePeer(peer);
			} else {
				boost::unique_lock<boost::mutex> lLock(contextMutex_[peer->contextId()]);
				peer->deactivate();

				active_[peer->contextId()].erase(peer->key());
				if (peer->isOutbound()) inactive_[peer->contextId()].insert(peer->key());
			}
		}

		consensusManager_->popPeer(peer);

		// update peers db
		updatePeer(peer);

		//
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::INFO, std::string("[peerManager]: deactivate peer ") + peer->key());
	}

	void updatePeer(IPeerPtr peer) {
		if (peer->isOutbound()) {
			peersContainer_.write(peer->key(), Peer::PersistentState(peer->state(), peer->status()));
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

	boost::mutex nextContextMutex_;
	std::map<int, PeersMap> peers_; // peers by context id's

	boost::ptr_vector<boost::mutex> contextMutex_;
	std::map<int, PeersSet> active_;
	std::map<int, PeersSet> quarantine_;
	std::map<int, PeersSet> banned_;
	std::map<int, PeersSet> inactive_;

	boost::mutex peersIdxMutex_;
	std::map<uint160, std::string> peerIdx_;
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
};

}

#endif