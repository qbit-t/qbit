// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_REQUEST_PROCESSOR_H
#define QBIT_REQUEST_PROCESSOR_H

#include "iconsensusmanager.h"
#include "irequestprocessor.h"
#include "state.h"
#include "iwallet.h"

#include <boost/atomic.hpp>

namespace qbit {

class RequestProcessor: public IConsensusManager, public IRequestProcessor, public std::enable_shared_from_this<RequestProcessor> {
public:
	RequestProcessor(ISettingsPtr settings) : settings_(settings) { dApp_ = "none"; }
	RequestProcessor(ISettingsPtr settings, const std::string& dApp) : settings_(settings), dApp_(dApp) {}

	//
	// current state
	StatePtr currentState() {
		StatePtr lState = State::instance(qbit::getTime(), settings_->roles(), wallet_->firstKey().createPKey(), dApp_, dAppInstance_);
		// TODO: device_id
		return lState;
	}

	//
	// client dapp instance
	void setDAppInstance(const uint256& dAppInstance) { 
		dAppInstance_ = dAppInstance; 
		broadcastState(currentState(), uint160());
	}

	//
	// use peer for network participation
	bool pushPeer(IPeerPtr peer) {
		//
		uint160 lAddress = peer->addressId();
		{
			// push
			boost::unique_lock<boost::mutex> lLock(peersMutex_);

			bool lAdded = false;
			std::vector<State::BlockInfo> lInfos = peer->state().infos();
			for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
				std::set<uint160> lPeers = chainPeers_[(*lInfo).chain()];
				if (!lPeers.size())
					if (!dApp_.size() || (*lInfo).dApp() == dApp_ || (*lInfo).chain() == MainChain::id())  {
						chainPeers_[(*lInfo).chain()].insert(lAddress);
						lAdded = true;
					}
			}			

			if (lAdded) peers_.insert(std::map<uint160 /*peer*/, IPeerPtr>::value_type(lAddress, peer));
			else return false;

			// update latency
			latencyMap_.insert(LatencyMap::value_type(lAddress, peer->latency()));
		}

		return true;
	}

	//
	// remove peer
	void popPeer(IPeerPtr peer) {
		//
		peer_t lPeerId = peer->addressId();
		{
			// pop
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			uint160 lAddress = peer->addressId();
			std::vector<State::BlockInfo> lInfos = peer->state().infos();
			for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
				chainPeers_[(*lInfo).chain()].erase(lAddress);
				if (!chainPeers_[(*lInfo).chain()].size()) chainPeers_.erase((*lInfo).chain()); 
			}

			peers_.erase(lAddress);

			latencyMap_.erase(lPeerId);
		}
	}

	bool peerExists(const uint160& peer) {
		boost::unique_lock<boost::mutex> lLock(peersMutex_);
		return peers_.find(peer) != peers_.end();
	}	

	//
	// push state
	bool pushState(StatePtr state) {
		return true;
	}

	//
	// pop state
	bool popState(StatePtr state) {
		return true;
	}

	//
	// main key
	SKey mainKey() { return wallet_->firstKey(); }	

	//
	// main key
	PKey mainPKey() {
		return mainKey().createPKey();
	}	

	//
	// make instance
	static IRequestProcessorPtr instance(ISettingsPtr settings) {
		return std::make_shared<RequestProcessor>(settings);
	}

	static IRequestProcessorPtr instance(ISettingsPtr settings, const std::string& dApp) {
		return std::make_shared<RequestProcessor>(settings, dApp);
	}

	bool loadTransaction(const uint256& chain, const uint256& tx, ILoadTransactionHandlerPtr handler) {
		//
		std::map<uint32_t, IPeerPtr> lOrder;
		collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.begin()->second->loadTransaction(chain, tx, handler);
			return true;
		}

		return false;
	}

	bool loadEntity(const std::string& entityName, ILoadEntityHandlerPtr handler) {
		//
		std::map<uint32_t, IPeerPtr> lOrder;
		collectPeersByChain(MainChain::id(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.begin()->second->loadEntity(entityName, handler);
			return true;
		}

		return false;
	}

	bool selectUtxoByAddress(const PKey& source, const uint256& chain, ISelectUtxoByAddressHandlerPtr handler) {
		//
		std::map<uint32_t, IPeerPtr> lOrder;
		collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.begin()->second->selectUtxoByAddress(source, chain, handler);
			return true;
		}

		return false;
	}

	bool selectUtxoByAddressAndAsset(const PKey& source, const uint256& chain, const uint256& asset, ISelectUtxoByAddressAndAssetHandlerPtr handler) {
		//
		std::map<uint32_t, IPeerPtr> lOrder;
		collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.begin()->second->selectUtxoByAddressAndAsset(source, chain, asset, handler);
			return true;
		}

		return false;
	}

	bool selectUtxoByTransaction(const uint256& chain, const uint256& tx, ISelectUtxoByTransactionHandlerPtr handler) {
		//
		std::map<uint32_t, IPeerPtr> lOrder;
		collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.begin()->second->selectUtxoByTransaction(chain, tx, handler);
			return true;
		}

		return false;
	}

	bool selectUtxoByEntity(const std::string& entityName, ISelectUtxoByEntityNameHandlerPtr handler) {
		//
		std::map<uint32_t, IPeerPtr> lOrder;
		collectPeersByChain(MainChain::id(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.begin()->second->selectUtxoByEntity(entityName, handler);
			return true;
		}

		return false;
	}

	bool selectEntityCountByShards(const std::string& dapp, ISelectEntityCountByShardsHandlerPtr handler) {
		//
		std::map<uint32_t, IPeerPtr> lOrder;
		collectPeersByChain(MainChain::id(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.begin()->second->selectEntityCountByShards(dapp, handler);
			return true;
		}

		return false;
	}

	bool sendTransaction(TransactionContextPtr ctx, ISentTransactionHandlerPtr handler) {
		//
		std::map<uint32_t, IPeerPtr> lOrder;
		collectPeersByChain(ctx->tx()->chain(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.begin()->second->sendTransaction(ctx, handler);
			return true;
		}

		return false;
	}	

	bool broadcastTransaction(TransactionContextPtr ctx) {
		//
		std::map<uint32_t, IPeerPtr> lOrder;
		collectPeersByChain(ctx->tx()->chain(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.begin()->second->broadcastTransaction(ctx);
			return true;
		}

		return false;
	}

	void broadcastState(StatePtr state, const uint160& /*except*/) {
		// prepare
		std::map<uint160 /*peer*/, IPeerPtr> lPeers;
		{
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			lPeers = peers_;
		}

		for (std::map<uint160, IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			lPeer->second->broadcastState(state);
		}
	}

	void setWallet(IWalletPtr wallet) { wallet_ = wallet; }
	IWalletPtr wallet() { return wallet_; }

	IConsensusManagerPtr consensusManager() {
		return std::static_pointer_cast<IConsensusManager>(shared_from_this());
	}

	TransactionContextPtr processTransaction(TransactionPtr tx) {
		return wallet_->processTransaction(tx);
	}

	void collectPeersByChain(const uint256& chain, std::map<uint32_t, IPeerPtr>& order) {
		boost::unique_lock<boost::mutex> lLock(peersMutex_);
		std::map<uint256 /*chain*/, std::set<uint160>>::iterator lPeers = chainPeers_.find(chain);
		if (lPeers != chainPeers_.end()) {
			for (std::set<uint160>::iterator lAddress = lPeers->second.begin(); lAddress != lPeers->second.end(); lAddress++) {
				std::map<uint160 /*peer*/, IPeerPtr>::iterator lPeer = peers_.find(*lAddress);
				LatencyMap::iterator lLatency = latencyMap_.find(*lAddress);
				if (lLatency != latencyMap_.end() && lPeer != peers_.end())
					order.insert(std::map<uint32_t, IPeerPtr>::value_type(lLatency->second, lPeer->second));
			}
		}
	}

	void collectChains(std::vector<uint256>& chains) {
		//
		boost::unique_lock<boost::mutex> lLock(peersMutex_);
		for (std::map<uint256 /*chain*/, std::set<uint160>>::iterator lChain = chainPeers_.begin(); lChain != chainPeers_.end(); lChain++) {
			if (lChain->first != MainChain::id()) chains.push_back(lChain->first);
		}
	}

private:
	std::string dApp_;
	uint256 dAppInstance_;

	boost::mutex peersMutex_;

	ISettingsPtr settings_;
	IWalletPtr wallet_;

	typedef uint160 peer_t;

	typedef std::map<uint160 /*peer*/, uint32_t> LatencyMap;
	LatencyMap latencyMap_;

	std::map<uint160 /*peer*/, IPeerPtr> peers_;
	std::map<uint256 /*chain*/, std::set<uint160>> chainPeers_;
};

} // qbit

#endif
