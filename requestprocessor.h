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

//
typedef boost::function<void (IPeerPtr /*peer*/, bool /*update*/, int /*count*/)> peerPushedFunction;
typedef boost::function<void (IPeerPtr /*peer*/, int /*count*/)> peerPoppedFunction;
typedef boost::function<void (const uint256& /*chain*/, const uint256 /*block*/, uint64_t /*height*/, uint64_t /*timestamp*/)> chainStateChangedFunction;

// forward
class RequestProcessor;
typedef std::shared_ptr<RequestProcessor> RequestProcessorPtr;

//
class RequestProcessor: public IConsensusManager, public IRequestProcessor, public std::enable_shared_from_this<RequestProcessor> {
public:
	RequestProcessor(ISettingsPtr settings) : settings_(settings) {}
	RequestProcessor(ISettingsPtr settings, peerPushedFunction peerPushed, peerPoppedFunction peerPopped, 
																			chainStateChangedFunction chainStateChanged) : 
		settings_(settings), peerPushed_(peerPushed), peerPopped_(peerPopped), chainStateChanged_(chainStateChanged) {}

	//
	// current state
	StatePtr currentState() {
		//
		StatePtr lState = State::instance(qbit::getTime(), settings_->roles(), wallet_->firstKey()->createPKey());
		for (std::vector<State::DAppInstance>::iterator lInstance = dapps_.begin(); lInstance != dapps_.end(); lInstance++) {
			lState->addDAppInstance(*lInstance);
		}

		// TODO: device_id
		return lState;
	}

	//
	size_t quarantineTime() { return 100; /* settings? */ }		

	//
	// client dapp instance
	void addDAppInstance(const State::DAppInstance& instance, bool notify = true) { 
		//
		for (std::vector<State::DAppInstance>::iterator lInstance = dapps_.begin(); lInstance != dapps_.end(); lInstance++) {
			if (instance.name() == lInstance->name()) return;
		}

		dapps_.push_back(instance);
		if (notify) broadcastState(currentState(), uint160());
	}

	//
	// client dapp instance
	void addDAppInstance(const std::vector<State::DAppInstance>& instance) { 
		//
		for (std::vector<State::DAppInstance>::const_iterator lInstance = instance.begin(); lInstance != instance.end(); lInstance++) {
			addDAppInstance(*lInstance, false);	
		}

		broadcastState(currentState(), uint160());
	}

	//
	// broadcast current state with instances
	void broadcastCurrentState() {
		broadcastState(currentState(), uint160());	
	}


	//
	// client dapp instance
	void clearDApps() {
		dapps_.resize(0);
	}

	//
	// instances supported
	const std::vector<State::DAppInstance>& dApps() const {
		return dapps_;
	}

	//
	// use peer for network participation
	bool pushPeer(IPeerPtr peer) {
		//
		uint160 lAddress = peer->addressId();
		if (gLog().isEnabled(Log::NET))
			gLog().write(Log::NET, std::string("[pushPeer]: try to push peer ") +
				strprintf("%s", lAddress.toHex()));

		bool lAdded = false;
		bool lExists = false;
		int lSize = 0;
		{
			// push
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);

			std::map<uint160 /*peer*/, IPeerPtr>::iterator lExisting = peers_.find(lAddress);
			lExists = lExisting != peers_.end(); // exists before update and merge

			std::vector<State::BlockInfo> lInfos = peer->state()->infos();
			for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
				std::set<uint160> lPeers = chainPeers_[(*lInfo).chain()];
				//
				// if peer collection by chain is less than clientActivePeers() - take this peer
				lExisting = peers_.find(lAddress);
				if (lPeers.size() < settings_->clientActivePeers() /*settings: normal, average, paranoid*/ || lExists) {
					bool lCorrect = false;
					for (std::vector<State::DAppInstance>::iterator lInstance = dapps_.begin(); lInstance != dapps_.end(); lInstance++) {
						if ((*lInfo).dApp() == lInstance->name()) { 
							lCorrect = true;
							break; 
						}
					}

					if (!dapps_.size() || lCorrect || (*lInfo).chain() == MainChain::id())  {
						// if exists - no changes
						chainPeers_[(*lInfo).chain()].insert(lAddress);
						//
						if ((*lInfo).dApp().size()) { 
							dappPeers_[(*lInfo).dApp()].insert(lAddress);
							dappChains_[(*lInfo).dApp()].insert((*lInfo).chain());
						}

						lAdded = true;

						if (lExisting != peers_.end()) peers_.erase(lExisting);
					}
				}
			}

			lSize = peers_.size();

			// if exist - no changes
			if (lAdded) { 
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[pushPeer]: peer pushed ") + 
						strprintf("%s", lAddress.toHex()));
				peers_.insert(std::map<uint160 /*peer*/, IPeerPtr>::value_type(lAddress, peer));
				lSize = peers_.size();
			} else {
				return false;
			}
		}

		//
		if (peerPushed_) peerPushed_(peer, lExists, lSize);

		pushPeerLatency(peer);
		return true;
	}

	//
	// update latency
	void pushPeerLatency(IPeerPtr peer) {
		// push
		uint160 lAddress = peer->addressId();
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		// update latency
		latencyMap_.erase(lAddress);
		latencyMap_.insert(LatencyMap::value_type(lAddress, peer->latency()));

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[pushPeer]: peer ") + 
				strprintf("%s, latency = %d", 
					lAddress.toHex(), peer->latency()));
	}	

	//
	// remove peer
	void popPeer(IPeerPtr peer) {
		//
		peer_t lPeerId = peer->addressId();
		//if (gLog().isEnabled(Log::NET))
		    gLog().write(Log::NET, std::string("[popPeer]: popping peer ") +
				strprintf("%s", lPeerId.toHex()));

		int lSize = 0;
		{
			// pop
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
			uint160 lAddress = peer->addressId();
			std::vector<State::BlockInfo> lInfos = peer->state()->infos();
			for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
				chainPeers_[(*lInfo).chain()].erase(lAddress);
				if ((*lInfo).dApp().size()) { 
					dappPeers_[(*lInfo).dApp()].erase(lPeerId);
					if (!dappPeers_[(*lInfo).dApp()].size()) dappPeers_.erase((*lInfo).dApp());
				}

				if (!chainPeers_[(*lInfo).chain()].size()) chainPeers_.erase((*lInfo).chain()); 
			}

			peers_.erase(lAddress);
			latencyMap_.erase(lPeerId);
			lSize = peers_.size();
		}

		if (peerPopped_) peerPopped_(peer, lSize);
	}

	bool peerExists(const uint160& peer) {
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		return peers_.find(peer) != peers_.end();
	}	

	//
	// push state
	bool pushState(StatePtr state) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		states_.erase(state->addressId());
		states_[state->addressId()] = state;

		//
		std::map<uint256 /*chain*/, std::map<uint64_t, std::set<uint160>>> lChainHeights = chainHeights_;

		//
		std::vector<State::BlockInfo>& lInfos = state->infos();
		for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
			//
			std::map<uint64_t, std::set<uint160>>& lChainMap = chainHeights_[lInfo->chain()];
			lChainMap[lInfo->height()].insert(state->addressId());

			if (lChainMap.size() > 10) {
				lChainMap.erase(lChainMap.begin());
			}
		}

		//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[pushState]: ") +
		//            strprintf("%d/%d", chainHeights_.size(), lChainHeights.size()));

		// check if new block confirmed (at least 2 peers) - notification
		if (chainStateChanged_) {
			for (std::map<uint256 /*chain*/, std::map<uint64_t, std::set<uint160>>>::iterator 
					lChain = chainHeights_.begin(); 
			            lChain != chainHeights_.end() /*&& lChainHeights.size()*/; lChain++) {
				//
				//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[pushState]: ") +
				//            strprintf("%s, %d, %d", lChain->first.toHex(), lChain->second.rbegin()->first, lChain->second.rbegin()->second.size()));
				//
				std::map<uint64_t, std::set<uint160>>& lChainMap = lChainHeights[lChain->first];
				if (lChainMap.rbegin() != lChainMap.rend() &&
				        lChain->second.rbegin()->first >= lChainMap.rbegin()->first &&
				            lChain->second.rbegin()->second.size() >= (qbit::gTestNet ? 1 : 2)) {
					//
					//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[pushState]: ") +
					//            strprintf(" -> %d", lChainMap.rbegin()->first));
					//
					uint160 lAddressId = *lChain->second.rbegin()->second.begin();
					StatePtr lState = states_[lAddressId];
					if (lState) {
						State::BlockInfo lInfo;
						if (lState->locateChain(lChain->first, lInfo)) {
							//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[pushState]: ") +
							//            strprintf("%s, %d, %d", lChain->first.toHex(), lInfo.height(), lInfo.timestamp()));
							chainStateChanged_(lChain->first, lInfo.hash(), lInfo.height(), lInfo.timestamp());
						}
					}
				}
			}
		}

		return true;
	}

	//
	// pop state
	bool popState(StatePtr state) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		states_.erase(state->addressId());
		return true;
	}

	//
	// main key
	SKeyPtr mainKey() { return wallet_->firstKey(); }	

	//
	// main key
	PKey mainPKey() {
		return mainKey()->createPKey();
	}	

	//
	// make instance
	static IRequestProcessorPtr instance(ISettingsPtr settings) {
		return std::make_shared<RequestProcessor>(settings);
	}
	static IRequestProcessorPtr instance(ISettingsPtr settings, peerPushedFunction peerPushed, peerPoppedFunction peerPopped, chainStateChangedFunction chainStateChanged) {
		return std::make_shared<RequestProcessor>(settings, peerPushed, peerPopped, chainStateChanged);
	}

	void requestState() {
		//
		std::map<uint160, IPeerPtr> lPeers;
		collectPeers(lPeers);

		for (std::map<uint160, IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			//
			lPeer->second->requestState();
		}
	}	

	bool loadTransaction(const uint256& chain, const uint256& tx, ILoadTransactionHandlerPtr handler) {
		//
		return loadTransaction(chain, tx, false, handler);
	}

	bool loadTransaction(const uint256& chain, const uint256& tx, bool tryMempool, ILoadTransactionHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.rbegin()->second->loadTransaction(chain, tx, tryMempool, handler);
			return true;
		}

		return false;
	}

	bool loadTransactions(const uint256& chain, const std::vector<uint256>& txs, ILoadTransactionsHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.rbegin()->second->loadTransactions(chain, txs, handler);
			return true;
		}

		return false;
	}

	bool loadEntity(const std::string& entityName, ILoadEntityHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(MainChain::id(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.rbegin()->second->loadEntity(entityName, handler);
			return true;
		}

		return false;
	}

	bool selectUtxoByAddress(const PKey& source, const uint256& chain, ISelectUtxoByAddressHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.rbegin()->second->selectUtxoByAddress(source, chain, handler);
			return true;
		}

		return false;
	}

	bool selectUtxoByAddressAndAsset(const PKey& source, const uint256& chain, const uint256& asset, ISelectUtxoByAddressAndAssetHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.rbegin()->second->selectUtxoByAddressAndAsset(source, chain, asset, handler);
			return true;
		}

		return false;
	}

	bool selectUtxoByTransaction(const uint256& chain, const uint256& tx, ISelectUtxoByTransactionHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.rbegin()->second->selectUtxoByTransaction(chain, tx, handler);
			return true;
		}

		return false;
	}

	bool selectUtxoByEntity(const std::string& entityName, ISelectUtxoByEntityNameHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(MainChain::id(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.rbegin()->second->selectUtxoByEntity(entityName, handler);
			return true;
		}

		return false;
	}

	bool selectUtxoByEntityNames(const std::vector<std::string>& entityNames, ISelectUtxoByEntityNamesHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(MainChain::id(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.rbegin()->second->selectUtxoByEntityNames(entityNames, handler);
			return true;
		}

		return false;
	}

	bool selectEntityCountByShards(const std::string& dapp, ISelectEntityCountByShardsHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(MainChain::id(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.rbegin()->second->selectEntityCountByShards(dapp, handler);
			return true;
		}

		return false;
	}

	bool selectEntityNames(const std::string& pattern, ISelectEntityNamesHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(MainChain::id(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.rbegin()->second->selectEntityNames(pattern, handler);
			return true;
		}

		return false;
	}

	bool askForQbits() {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(MainChain::id(), lOrder);

		int lCount = 0;
		for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::reverse_iterator lPeer = lOrder.rbegin(); lPeer != lOrder.rend() && lCount < 3 /* it's enough */; lPeer++, lCount++) {
			lPeer->second->tryAskForQbits();
		}

		if (lOrder.size()) return true;
		return false;
	}

	bool selectEntityCountByDApp(const std::string& dapp, ISelectEntityCountByDAppHandlerPtr handler, int& destinations) {
		//
		std::map<uint256, std::multimap<uint32_t, IPeerPtr>> lOrder;
		collectPeersByDApp(dapp, lOrder);

		destinations = 0;
		//
		if (lOrder.size()) {
			//
			std::set<uint160> lControl;
			std::list<IPeerPtr> lDests;
			for (std::map<uint256, std::multimap<uint32_t, IPeerPtr>>::iterator lItem = lOrder.begin(); lItem != lOrder.end(); lItem++) {
				//
				std::map<IRequestProcessor::KeyOrder, IPeerPtr> lPeers;
				collectPeersByChain(lItem->first, lPeers);
				//
				for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::reverse_iterator lPeer = lPeers.rbegin(); lPeer != lPeers.rend(); lPeer++) {
					if (lControl.insert(lPeer->second->addressId()).second) {
						lDests.push_back(lPeer->second);
					}
				}
			}

			destinations = 0;
			for (std::list<IPeerPtr>::iterator lPeer = lDests.begin(); lPeer != lDests.end() && destinations < 3; lPeer++, destinations++) {
				(*lPeer)->selectEntityCountByDApp(dapp, handler);
			}

			return true;
		}

		return false;
	}

	IPeerPtr sendTransaction(TransactionContextPtr ctx, ISentTransactionHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(ctx->tx()->chain(), lOrder);

		// try locate last used peer
		IPeerPtr lLastPeer = nullptr;
		for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::reverse_iterator lPeer = lOrder.rbegin(); lPeer != lOrder.rend(); lPeer++) {
			//
			// if (!lPeer->second->state()->minerOrValidator()) continue; // ONLY validators can take a part
			if (lPeer->second->addressId() == lastPeer_) {
				lLastPeer = lPeer->second;
				break;
			}
		}

		if (!lLastPeer && lOrder.size()) {
			// use nearest
			lastPeer_ = lOrder.rbegin()->second->addressId();
			if (!lOrder.rbegin()->second->sendTransaction(ctx, handler)) return nullptr;
			return lOrder.rbegin()->second;
		}

		if (!lLastPeer->sendTransaction(ctx, handler)) return nullptr;
		return lLastPeer;
	}	

	IPeerPtr sendTransaction(const uint256& destination, TransactionContextPtr ctx, ISentTransactionHandlerPtr handler) {
		//
		return sendTransaction(nullptr, destination, ctx, handler);
	}

	IPeerPtr sendTransaction(IPeerPtr peer, const uint256& destination, TransactionContextPtr ctx, ISentTransactionHandlerPtr handler) {
		//
		if (peer) {
			//
			if (!peer->sendTransaction(ctx, handler)) return nullptr;
			return peer;
		}

		// doing hard
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(destination, lOrder);

		// try locate last used peer
		IPeerPtr lLastPeer = nullptr;
		for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::reverse_iterator lPeer = lOrder.rbegin(); lPeer != lOrder.rend(); lPeer++) {
			//
			// if (!lPeer->second->state()->minerOrValidator()) continue; // ONLY validators can take a part
			if (lPeer->second->addressId() == lastPeer_) {
				lLastPeer = lPeer->second;
				break;
			}
		}

		if (!lLastPeer && lOrder.size()) {
			// use nearest
			lastPeer_ = lOrder.rbegin()->second->addressId();
			if (!lOrder.rbegin()->second->sendTransaction(ctx, handler)) {
				//
				return nullptr;
			}

			return lOrder.rbegin()->second;
		}

		if (!lLastPeer->sendTransaction(ctx, handler)) return nullptr;
		return lLastPeer;
	}

	bool sendTransaction(IPeerPtr peer, TransactionContextPtr ctx, ISentTransactionHandlerPtr handler) {
		//
		if (peer) {
			return peer->sendTransaction(ctx, handler);
		}

		return false;
	}

	bool broadcastTransaction(TransactionContextPtr ctx) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(ctx->tx()->chain(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.rbegin()->second->broadcastTransaction(ctx);
			return true;
		}

		return false;
	}

	void broadcastState(StatePtr state, const uint160& /*except*/) {
		// prepare
		std::map<uint160 /*peer*/, IPeerPtr> lPeers;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
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

	void collectPeers(std::map<uint160, IPeerPtr>& peers) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		for (std::map<uint256 /*chain*/, std::set<uint160>>::iterator lPeers = chainPeers_.begin(); lPeers != chainPeers_.end(); lPeers++) {
			for (std::set<uint160>::iterator lAddress = lPeers->second.begin(); lAddress != lPeers->second.end(); lAddress++) {
				std::map<uint160 /*peer*/, IPeerPtr>::iterator lPeer = peers_.find(*lAddress);
				if (lPeer != peers_.end()) peers[lPeer->first] = lPeer->second;
			}
		}
	}

	//
	void collectPeersByChain(const uint256& chain, std::map<IRequestProcessor::KeyOrder, IPeerPtr>& order) {
		collectPeersByChain(chain, order, true);
	}

	void collectPeersByChain(const uint256& chain, std::map<IRequestProcessor::KeyOrder, IPeerPtr>& order, bool mostSuitable) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		std::map<uint256 /*chain*/, std::set<uint160>>::iterator lPeers = chainPeers_.find(chain);
		if (lPeers != chainPeers_.end()) {
			for (std::set<uint160>::iterator lAddress = lPeers->second.begin(); lAddress != lPeers->second.end(); lAddress++) {
				std::map<uint160 /*peer*/, IPeerPtr>::iterator lPeer = peers_.find(*lAddress);
				std::map<uint160 /*peer*/, StatePtr>::iterator lState = states_.find(*lAddress);
				//
				LatencyMap::iterator lLatency = latencyMap_.find(*lAddress);
				if (lLatency != latencyMap_.end() && lPeer != peers_.end()) {
					//
					StatePtr lStateData = lState != states_.end() ? lState->second : lPeer->second->state();
					State::BlockInfo lInfo;
					if (((mostSuitable && lStateData->infos().size() > 1) || !mostSuitable) && // NOTICE: only nodes with multiple shards support
					        lStateData->locateChain(chain, lInfo)) {
						//
						// aggregate ALL chain heights
						uint64_t lHeights = 0;
						std::vector<State::BlockInfo> lInfos = lStateData->infos();
						for (std::vector<State::BlockInfo>::iterator lHeightInfo = lInfos.begin(); lHeightInfo != lInfos.end(); lHeightInfo++) {
							//
							lHeights += (*lHeightInfo).height();
						}

						//
						// ordering
						order.insert(std::map<IRequestProcessor::KeyOrder, IPeerPtr>::value_type(
							IRequestProcessor::KeyOrder(lHeights, lLatency->second), lPeer->second));
					}
				}
			}
		}

		if (chain == MainChain::id() && !order.size() && mostSuitable) {
			collectPeersByChain(chain, order, false);
		}

		// make distance map
		std::map<int64_t, std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator> lDistances;
		if (order.size() >= 3 /*at least 3 nodes shoukd be available*/) {
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lItem = order.begin(); lItem != order.end(); lItem++) {
				//
				std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lNext = std::next(lItem, 1);
				if (lNext != order.end()) {
					lDistances.insert(std::map<int64_t, std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator>::value_type(
						abs((int64_t)lItem->first.height() - (int64_t)lNext->first.height()),
						lItem
					));
				}
			}
		}

		// cleanup
		for (std::map<int64_t, std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator>::iterator lDist = lDistances.begin(); lDist != lDistances.end(); lDist++) {
			//
			if (lDist->first > 20) order.erase(lDist->second);
		}

		/*
		if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[collectPeersByChain]: ") +
					strprintf("chain - %s#", chain.toHex().substr(0, 10)));
		for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::reverse_iterator lItem = order.rbegin(); lItem != order.rend(); lItem++) {
			//
			if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[collectPeersByChain]: ") +
						strprintf("%s, %d, %d", lItem->second->key(), lItem->first.height(), lItem->first.latency()));
		}
		*/
	}

	void collectPeersByDApp(const std::string& dapp, std::map<uint256, std::multimap<uint32_t, IPeerPtr>>& order) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		std::map<std::string /*dapp*/, std::set<uint160>>::iterator lDApp = dappPeers_.find(dapp);

		if (lDApp != dappPeers_.end()) {
			for (std::set<uint160>::iterator lPeerAddress = lDApp->second.begin(); lPeerAddress != lDApp->second.end(); lPeerAddress++) {
				//
				std::map<uint160 /*peer*/, IPeerPtr>::iterator lPeer = peers_.find(*lPeerAddress);
				std::map<uint160 /*peer*/, StatePtr>::iterator lState = states_.find(*lPeerAddress);
				//
				if (lPeer != peers_.end()) {
					std::vector<State::BlockInfo> lInfos(lState != states_.end() ? lState->second->infos() :
					                                                               lPeer->second->state()->infos());
					for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
						if (lInfo->dApp() == dapp) { 
							order[lInfo->chain()].insert(std::map<uint32_t, IPeerPtr>::value_type(lPeer->second->latency(), lPeer->second));
						}
					}
				}
			}
		} else {
			if (gLog().isEnabled(Log::CLIENT))
				gLog().write(Log::CLIENT, std::string("[collectPeersByDApp]: ") + strprintf("dApp not found - %s", dapp));
		}
	}

	void collectChains(const std::string& dApp, std::vector<uint256>& chains) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		std::map<std::string /*dapp*/, std::set<uint256>>::iterator lDApp = dappChains_.find(dApp);
		if (lDApp != dappChains_.end()) {
			chains.insert(chains.end(), lDApp->second.begin(), lDApp->second.end());
		}
	}

	uint64_t locateHeight(const uint256& chain) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		std::set<uint64_t /*height*/> lOrder;
		std::map<uint256 /*chain*/, std::set<uint160>>::iterator lPeers = chainPeers_.find(chain);
		if (lPeers != chainPeers_.end()) {
			for (std::set<uint160>::iterator lAddress = lPeers->second.begin(); lAddress != lPeers->second.end(); lAddress++) {
				std::map<uint160 /*peer*/, IPeerPtr>::iterator lPeer = peers_.find(*lAddress);
				//
				if (lPeer != peers_.end()) {
					State::BlockInfo lInfo;
					if (lPeer->second->state()->locateChain(chain, lInfo)) {
						lOrder.insert(lInfo.height());
					}
				}
			}
		}

		if (lOrder.size()) return *(lOrder.rbegin());

		return 0;
	}

	IPeerPtr locatePeer(const uint160& peer) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		std::map<uint160 /*peer*/, IPeerPtr>::iterator lPeer = peers_.find(peer);
		if (lPeer != peers_.end()) return lPeer->second;
		return nullptr;
	}

	ITransactionStoreManagerPtr storeManager() { return nullptr; }

private:
	std::vector<State::DAppInstance> dapps_;

	boost::recursive_mutex peersMutex_;

	ISettingsPtr settings_;
	IWalletPtr wallet_;
	peerPushedFunction peerPushed_;
	peerPoppedFunction peerPopped_;
	chainStateChangedFunction chainStateChanged_;

	typedef uint160 peer_t;

	typedef std::map<uint160 /*peer*/, uint32_t> LatencyMap;
	LatencyMap latencyMap_;

	std::map<uint160 /*peer*/, IPeerPtr> peers_;
	std::map<uint160 /*peer*/, StatePtr> states_;
	std::map<uint256 /*chain*/, std::map<uint64_t, std::set<uint160>>> chainHeights_;
	std::map<uint256 /*chain*/, std::set<uint160>> chainPeers_;
	std::map<std::string /*dapp*/, std::set<uint160>> dappPeers_;
	std::map<std::string /*dapp*/, std::set<uint256>> dappChains_;

	peer_t lastPeer_;
	uint64_t lastPeerUsageTimestamp_ = 0; // not used for now
};

} // qbit

#endif
