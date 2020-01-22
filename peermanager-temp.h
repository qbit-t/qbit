// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_PEERMANAGER_H
#define QBIT_PEERMANAGER_H

#include "ipeermanager.h"
#include "log/log.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/chrono.hpp>
#include <boost/algorithm/string.hpp>

namespace qbit {

class PeerManager: public IPeerManager, public std::enable_shared_from_this<PeerManager> {
public:
	PeerManager(ISettingsPtr settings, IConsensusPtr consensus, IMemoryPoolPtr mempool) : 
		settings_(settings), consensus_(consensus), mempool_(mempool) {

		// prepare pool contexts
		for (std::size_t lIdx = 0; lIdx < settings_->threadPoolSize(); ++lIdx) {
			IOContextPtr lContext = std::make_shared<boost::asio::io_context>();
			contexts_.push_back(lContext);
			work_.push_back(asio::make_work_guard(*lContext));
		}		
	}

	inline bool exists(const std::string& endpoint) {
		return peers_.find(endpoint) != peers_.end();
	}

	static IPeerManagerPtr instance(ISettingsPtr settings, IConsensusPtr consensus, IMemoryPoolPtr mempool) { 
		return std::make_shared<PeerManager>(settings, consensus, mempool); 
	}

	bool updatePeerLatency(PeerPtr peer, uint32_t latency) {
		//
		PeerPtr lPeer = peer;

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

	void deactivatePeer(PeerPtr peer) {
		//
		PeerPtr lPeer = peer;
		if (lPeer) {
			deactivate(lPeer);
		}
	}

	void removePeer(PeerPtr peer) {
		// TODO: from peer.cpp while processing server-side 
	}

	bool updatePeerState(SocketPtr socket, State& state) {
		// prepare key - 000.000.000.000:00000
		std::string lEndpoint = Peer::key(socket);
		
		// sanity check
		StatePtr lOwnState = consensus()->currentState();
		if (lOwnState->address().id() == state.address().id()) return false;

		//
		PeerPtr lPeer = locate(lEndpoint);

		// peer exists
		if (lPeer) {
			// check
			if (lPeer->status() == Peer::BANNED) return false;

			// sanity for new key
			if (lPeer->address().id() != state.address().id()) {
				quarantine(lPeer);
				return false;
			}

			// check new state
			if (state.valid()) {
				lPeer->setState(state);

				// check for quarantine
				if (lPeer->onQuarantine(consensus_))
					return false;

				// update state and data
				update(lPeer);
			} else {
				ban(lPeer);
				return false;
			}
		// peer is new
		} else {
			if (state.valid()) {
				PeerPtr lPeer = Peer::instance(socket, state, std::static_pointer_cast<IPeerManager>(shared_from_this()));
				add(lPeer);
			} else {
				return false;
			}
		}

		return true;
	}

	IConsensusPtr consensus() { return consensus_; }
	ISettingsPtr settings() { return settings_; }	

	bool addPeer(const std::string& endpoint) {
		if (locate(endpoint) == nullptr) {
			std::vector<std::string> lParts;
			boost::split(lParts, endpoint, boost::is_any_of(":"), boost::token_compress_on);

			if (lParts.size() == 2) {
				PeerPtr lPeer = Peer::instance(getContextId(), endpoint, std::static_pointer_cast<IPeerManager>(shared_from_this()));
				add(lPeer);

				return true;
			}
		}

		return false;
	}

	PeerPtr locate(SocketPtr socket) {
		return locate(Peer::key(socket));
	}

	PeerPtr locate(const std::string& endpoint) {
		// lock peers
		boost::unique_lock<boost::mutex> lLock(peersMutex_);

		// traverse
		for (std::map<int, PeersMap>::iterator lChannel = peers_.begin(); lChannel != peers_.end(); lChannel++) {
			std::map<std::string /*endpoint*/, PeerPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != peers_.end()) return lPeerIter->second;
		}

		return nullptr;
	}	

	void run() {
		// create peers maps/sets
		int lId = 0;
		for (std::vector<IOContextPtr>::iterator lCtx = contexts_.begin(); lCtx != contexts_.end(); lCtx++)	{
			peers_[lId] = PeersMap();
			active_[lId] = PeersSet();
			quarantine_[lId] = PeersSet();
			banned_[lId] = PeersSet();
		}

		// create a pool of threads to run all of the io_contexts
		std::vector<boost::shared_ptr<asio::thread> > lThreads;
		for (std::vector<IOContextPtr>::iterator lCtx = contexts_.begin(); lCtx != contexts_.end(); lCtx++)	{
			//boost::shared_ptr<asio::thread> lThread(
			//	new boost::asio::thread(
			//		boost::bind(&boost::asio::io_context::run, *lCtx))); // *(*lCtx))
			//lThreads.push_back(lThread);
		}

		// create timers
		lId = 0;
		for (std::vector<IOContextPtr>::iterator lCtx = contexts_.begin(); lCtx != contexts_.end(); lCtx++, lId++) {
			//boost::asio::steady_timer lTimer(*(*lCtx), boost::asio::chrono::seconds(1));
			//lTimer.async_wait(boost::bind(&PeerManager::touch, this, lId));
		}

		// wait for all threads in the pool to exit
		for (std::vector<boost::shared_ptr<asio::thread> >::iterator lThread = lThreads.begin(); lThread != lThreads.end(); lThread++)
			lThread->join();
	}

	void stop() {
		// stop all contexts
		for (std::vector<IOContextPtr>::iterator lCtx = contexts_.begin(); lCtx != contexts_.end(); lCtx++)
			lCtx->stop();
	}

	IOContextPtr getContext(int id) {
		return contexts_[id];
	}

	int getContextId() {
		int lId = nextContext_++;
		
		if (nextContext_ == contexts_.size()) nextContext_ = 0;
		return lId;
	}

private:
	void touch(int id, const boost::system::error_code& error) {
		// touch active peers
		if(!error) {
			for (std::set<std::string /*endpoint*/>::iterator lPeer = active_[id].begin(); lPeer != active_[id].end(); lPeer++) {
				peers_[id][*lPeer]->ping();
			}
		}
	}

private:
	void quarantine(PeerPtr peer) {
		peer->toQuarantine(consensus_->currentState()->height() + consensus_->quarantineTime());
		active_[id].erase(peer->key());
		quarantine_[id].insert(peer->key());

		// pop from consensus
		consensus_->popPeer(peer->key(), peer);
	}

	void ban(PeerPtr peer) {
		peer->toBan();
		quarantine_[peer->contextId()].erase(peer->key());
		active_[peer->contextId()].erase(peer->key());
		banned_[peer->contextId()].insert(peer->key());

		// pop from consensus
		consensus_->popPeer(peer->key(), peer);
	}

	void add(PeerPtr peer) {
		peer->toActive();

		peers_[peer->contextId()].insert(std::map<std::string /*endpoint*/, PeerPtr>::value_type(peer->key(), peer));
		active_[peer->contextId()].insert(peer->key());

		// push to consensus
		consensus_->pushPeer(peer->key(), peer);
	}

	void update(PeerPtr peer) {
		peer->toActive();
		active_[peer->contextId()].insert(peer->key());
		quarantine_[peer->contextId()].erase(peer->key());
		banned_[peer->contextId()].erase(peer->key());

		// push to consensus
		consensus_->pushPeer(peer->key(), peer);
	}

	void deactivate(PeerPtr peer) {
		peer->deactivate();
		active_[peer->contextId()].erase(peer->key());

		// push to consensus
		consensus_->pushPeer(peer->key(), peer);
	}

private:
	typedef std::map<std::string /*endpoint*/, PeerPtr> PeersMap;
	typedef std::set<std::string /*endpoint*/> PeersSet;

	std::map<int, PeersMap> peers_; // peers by context id's
	std::map<int, PeersSet> active_;
	std::map<int, PeersSet> quarantine_;
	std::map<int, PeersSet> banned_;
	boost::mutex peersMutex_;

	std::vector<IOContextPtr> contexts_;
	std::list<IOContextWork> work_;
	int nextContext_ = 0;	

	ISettingsPtr settings_;
	IConsensusPtr consensus_;
	IMemoryPoolPtr mempool_;
};

}

#endif