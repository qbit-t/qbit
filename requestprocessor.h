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

//
class RequestProcessor: public IConsensusManager, public IRequestProcessor, public std::enable_shared_from_this<RequestProcessor> {
public:
	RequestProcessor(ISettingsPtr settings) : settings_(settings) {}
	RequestProcessor(ISettingsPtr settings, peerPushedFunction peerPushed, peerPoppedFunction peerPopped) : 
		settings_(settings), peerPushed_(peerPushed), peerPopped_(peerPopped) {}

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
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[pushPeer]: try to push peer ") + 
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
				// if peer collection by chain is less than 3 - take this peer
				lExisting = peers_.find(lAddress);
				if (lPeers.size() < 3 /*settings: normal, average, paranoid*/ || lExists) {
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
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[popPeer]: popping peer ") + 
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
	static IRequestProcessorPtr instance(ISettingsPtr settings, peerPushedFunction peerPushed, peerPoppedFunction peerPopped) {
		return std::make_shared<RequestProcessor>(settings, peerPushed, peerPopped);
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
			lOrder.begin()->second->loadTransaction(chain, tx, tryMempool, handler);
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
			lOrder.begin()->second->loadTransactions(chain, txs, handler);
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
			lOrder.begin()->second->loadEntity(entityName, handler);
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
			lOrder.begin()->second->selectUtxoByAddress(source, chain, handler);
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
			lOrder.begin()->second->selectUtxoByAddressAndAsset(source, chain, asset, handler);
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
			lOrder.begin()->second->selectUtxoByTransaction(chain, tx, handler);
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
			lOrder.begin()->second->selectUtxoByEntity(entityName, handler);
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
			lOrder.begin()->second->selectUtxoByEntityNames(entityNames, handler);
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
			lOrder.begin()->second->selectEntityCountByShards(dapp, handler);
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
			lOrder.begin()->second->selectEntityNames(pattern, handler);
			return true;
		}

		return false;
	}

	bool askForQbits() {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(MainChain::id(), lOrder);

		for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
			lPeer->second->tryAskForQbits();
		}

		if (lOrder.size()) return true;
		return false;
	}

	bool selectEntityCountByDApp(const std::string& dapp, ISelectEntityCountByDAppHandlerPtr handler, int& destinations) {
		//
		std::map<uint256, std::map<uint32_t, IPeerPtr>> lOrder;
		collectPeersByDApp(dapp, lOrder);

		destinations = 0;
		//
		if (lOrder.size()) {
			// use nearest
			std::list<IPeerPtr> lDests;
			for (std::map<uint256, std::map<uint32_t, IPeerPtr>>::iterator lItem = lOrder.begin(); lItem != lOrder.end(); lItem++) {
				//
				if (!lDests.size() || (lDests.size() && (*lDests.rbegin())->addressId() != lItem->second.begin()->second->addressId())) {
					lDests.push_back(lItem->second.begin()->second);
				}
			}

			destinations = lDests.size();
			for (std::list<IPeerPtr>::iterator lPeer = lDests.begin(); lPeer != lDests.end(); lPeer++)
				(*lPeer)->selectEntityCountByDApp(dapp, handler);
			return true;
		}

		return false;
	}

	IPeerPtr sendTransaction(TransactionContextPtr ctx, ISentTransactionHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(ctx->tx()->chain(), lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.begin()->second->sendTransaction(ctx, handler);
			return lOrder.begin()->second;
		}

		return nullptr;
	}	

	IPeerPtr sendTransaction(const uint256& destination, TransactionContextPtr ctx, ISentTransactionHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		collectPeersByChain(destination, lOrder);

		if (lOrder.size()) {
			// use nearest
			lOrder.begin()->second->sendTransaction(ctx, handler);
			return lOrder.begin()->second;
		}

		return nullptr;
	}

	void sendTransaction(IPeerPtr peer, TransactionContextPtr ctx, ISentTransactionHandlerPtr handler) {
		//
		if (peer) {
			peer->sendTransaction(ctx, handler);
		}
	}

	bool broadcastTransaction(TransactionContextPtr ctx) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
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
	// TODO: maybe we need to take in account chain height for the given peer?
	// probably nearest node has to be fully synchronized
	void collectPeersByChain(const uint256& chain, std::map<IRequestProcessor::KeyOrder, IPeerPtr>& order) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		std::map<uint256 /*chain*/, std::set<uint160>>::iterator lPeers = chainPeers_.find(chain);
		if (lPeers != chainPeers_.end()) {
			for (std::set<uint160>::iterator lAddress = lPeers->second.begin(); lAddress != lPeers->second.end(); lAddress++) {
				std::map<uint160 /*peer*/, IPeerPtr>::iterator lPeer = peers_.find(*lAddress);
				LatencyMap::iterator lLatency = latencyMap_.find(*lAddress);
				if (lLatency != latencyMap_.end() && lPeer != peers_.end()) {
					//
					State::BlockInfo lInfo;
					if (lPeer->second->state()->locateChain(chain, lInfo)) {
						order.insert(std::map<IRequestProcessor::KeyOrder, IPeerPtr>::value_type(
							IRequestProcessor::KeyOrder(lInfo.height(), lLatency->second), lPeer->second));
					}
				}
			}
		}
	}

	void collectPeersByDApp(const std::string& dapp, std::map<uint256, std::map<uint32_t, IPeerPtr>>& order) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
		std::map<std::string /*dapp*/, std::set<uint160>>::iterator lDApp = dappPeers_.find(dapp);

		if (lDApp != dappPeers_.end()) {
			for (std::set<uint160>::iterator lPeerAddress = lDApp->second.begin(); lPeerAddress != lDApp->second.end(); lPeerAddress++) {
				//
				std::map<uint160 /*peer*/, IPeerPtr>::iterator lPeer = peers_.find(*lPeerAddress);
				if (lPeer != peers_.end()) {
					for (std::vector<State::BlockInfo>::iterator lInfo = lPeer->second->state()->infos().begin(); lInfo != lPeer->second->state()->infos().end(); lInfo++) {
						if (lInfo->dApp() == dapp) { 
							order[lInfo->chain()].insert(std::map<uint32_t, IPeerPtr>::value_type(lPeer->second->latency(), lPeer->second));
						}
					}
				}
			}
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

private:
	std::vector<State::DAppInstance> dapps_;

	boost::recursive_mutex peersMutex_;

	ISettingsPtr settings_;
	IWalletPtr wallet_;
	peerPushedFunction peerPushed_;
	peerPoppedFunction peerPopped_;

	typedef uint160 peer_t;

	typedef std::map<uint160 /*peer*/, uint32_t> LatencyMap;
	LatencyMap latencyMap_;

	std::map<uint160 /*peer*/, IPeerPtr> peers_;
	std::map<uint160 /*peer*/, StatePtr> states_;
	std::map<uint256 /*chain*/, std::map<uint64_t, std::set<uint160>>> chainHeights_;
	std::map<uint256 /*chain*/, std::set<uint160>> chainPeers_;
	std::map<std::string /*dapp*/, std::set<uint160>> dappPeers_;
	std::map<std::string /*dapp*/, std::set<uint256>> dappChains_;
};

} // qbit

#endif
