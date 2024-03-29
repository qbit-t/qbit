// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CONSENSUS_MANAGER_H
#define QBIT_CONSENSUS_MANAGER_H

#include "itransactionstoremanager.h"
#include "iconsensusmanager.h"
#include "consensus.h"

#include <boost/atomic.hpp>

namespace qbit {

class ConsensusManager: public IConsensusManager, public std::enable_shared_from_this<ConsensusManager> {
public:
	ConsensusManager(ISettingsPtr settings) : settings_(settings) {}

	static void registerConsensus(const std::string& name, ConsensusCreatorPtr creator) {
		gConsensuses[name] = creator;
	}

	bool exists(const uint256& chain) {
		boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
		return consensuses_.find(chain) != consensuses_.end();
	}
 
	IConsensusPtr locate(const uint256& chain) {
		boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
		std::map<uint256, IConsensusPtr>::iterator lConsensus = consensuses_.find(chain);
		if (lConsensus != consensuses_.end()) return lConsensus->second;
		return nullptr;
	}

	bool add(IConsensusPtr consensus) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
		if (consensuses_.find(consensus->chain()) == consensuses_.end()) {
			consensuses_[consensus->chain()] = consensus;
			return true;
		}

		return false;
	}

	void pop(const uint256& chain) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
		std::map<uint256, IConsensusPtr>::iterator lConsensus = consensuses_.find(chain);
		if (lConsensus != consensuses_.end()) {
			lConsensus->second->close();
			consensuses_.erase(chain);
		}
	}

	IConsensusPtr push(const uint256& chain, EntityPtr dapp) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
		if (consensuses_.find(chain) == consensuses_.end()) {

			if (dapp) {
				Consensuses::iterator lCreator = gConsensuses.find(dapp->entityName());
				if (lCreator != gConsensuses.end()) {
					ITransactionStorePtr lStore = storeManager_->locate(chain);
					if (lStore) {
						IConsensusPtr lConsensus = lCreator->second->create(chain, shared_from_this(), settings_, wallet_, lStore);
						consensuses_[chain] = lConsensus;
						return lConsensus;
					}
				}
			} else {
				ITransactionStorePtr lStore = storeManager_->locate(chain);
				if (lStore) {
					IConsensusPtr lConsensus = Consensus::instance(chain, shared_from_this(), settings_, wallet_, lStore);
					consensuses_[chain] = lConsensus;
					return lConsensus;
				}
			}
		}

		return nullptr;
	}

	std::vector<IConsensusPtr> consensuses() {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
		std::vector<IConsensusPtr> lConsensuses;

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = consensuses_.begin(); lConsensus != consensuses_.end(); lConsensus++) {
			lConsensuses.push_back(lConsensus->second);
		}

		return lConsensuses;
	}

	//
	// collect current state
	StatePtr currentState() {
		StatePtr lState = State::instance(qbit::getTime(), settings_->roles(), wallet_->firstKey()->createPKey());

		//
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
		}

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			ITransactionStorePtr lStore = storeManager_->locate(lConsensus->first);
			if (lStore) {
				BlockHeader lHeader;
				uint64_t lHeight = lStore->currentHeight(lHeader);
				if (!lHeight) lHeader.setChain(lStore->chain());
				lState->addHeader(lHeader, lHeight, lConsensus->second->dApp());
			}
		}

		/*
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			for (std::map<uint256, IConsensusPtr>::iterator lConsensus = consensuses_.begin(); lConsensus != consensuses_.end(); lConsensus++) {
				ITransactionStorePtr lStore = storeManager_->locate(lConsensus->first);
				if (lStore) {
					BlockHeader lHeader;
					uint64_t lHeight = lStore->currentHeight(lHeader);
					if (!lHeight) lHeader.setChain(lStore->chain());
					lState->addHeader(lHeader, lHeight, lConsensus->second->dApp());
				}
			}
		}
		*/

		//
		lState->prepare();

		return lState;
	}

	//
	// block count (100 blocks)
	size_t quarantineTime() { return 100; /* settings? */ }	

	//
	// use peer for network participation
	bool pushPeer(IPeerPtr peer) {
		//
		// skip peer with client protocol
		if (peer == nullptr || peer->state()->client()) return true;

		//
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
		}

		{
			// push
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			uint160 lAddress = peer->addressId();
			peers_[lAddress] = peer;
			pushedPeers_.insert(peer->key()); // push control
			std::vector<State::BlockInfo> lInfos = peer->state()->infos();
			for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
				chainPeers_[(*lInfo).chain()].insert(lAddress);
			}
		}

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			lConsensus->second->pushPeer(peer);
		}

		pushPeerLatency(peer);
		
		return true;
	}

	//
	// update latency
	void pushPeerLatency(IPeerPtr peer) {
		// NOTICE: latency management is not used in node-to-node communications
		/*
		peer_t lPeerId = peer->addressId();
		boost::unique_lock<boost::mutex> lLock(latencyMutex_);
		// locate latency
		std::pair<LatencyMap::iterator,
					LatencyMap::iterator> lRange = latencyMap_.equal_range(peer->latencyPrev());

		for (LatencyMap::iterator lTo = lRange.first; lTo != lRange.second; lTo++)
			if (lPeerId == lTo->second->addressId()) {
				latencyMap_.erase(lTo);
				break;
			}

		// update latency
		latencyMap_.insert(LatencyMap::value_type(peer->latency(), peer));

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[pushPeer]: peer ") + 
				strprintf("%s, latency = %d", 
					lPeerId.toHex(), peer->latency()));
		*/
	}


	//
	// remove peer from consensus participation
	void popPeer(IPeerPtr peer) {
		//
		if (peer == nullptr) return;

		//
		peer_t lPeerId = peer->addressId();
		std::string lPeerKey = peer->key();

		//
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
		}

		{
			// pop
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			if (pushedPeers_.find(lPeerKey) == pushedPeers_.end()) {
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS,
					strprintf("[popPeer]: peer %s was not pushed, skipping...", lPeerKey));
				return;
			}

			uint160 lAddress = peer->addressId();
			std::vector<State::BlockInfo> lInfos = peer->state()->infos();
			for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
				chainPeers_[(*lInfo).chain()].erase(lAddress);
				if (!chainPeers_[(*lInfo).chain()].size()) chainPeers_.erase((*lInfo).chain()); 
			}

			peers_.erase(lAddress);
			pushedPeers_.erase(lPeerKey); // pop control
		}		

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			lConsensus->second->popPeer(peer);
		}

		// NOTICE: latency management is not used in node-to-node communications
		/*
		{
			boost::unique_lock<boost::mutex> lLock(latencyMutex_);
			// locate latency
			std::pair<LatencyMap::iterator,
						LatencyMap::iterator> lRange = latencyMap_.equal_range(peer->latencyPrev());

			for (LatencyMap::iterator lTo = lRange.first; lTo != lRange.second; lTo++)
				if (lPeerId == lTo->second->addressId()) {
					latencyMap_.erase(lTo);
					break;
				}
		}
		*/
	}	

	bool peerExists(const uint160& peer) {
		boost::unique_lock<boost::mutex> lLock(peersMutex_);
		return peers_.find(peer) != peers_.end();
	}

	//
	// push state
	bool pushState(StatePtr state) {
		//
		// skip peer with client protocol
		if (state->client()) return true;

		//
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
		}

		{
			// push
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			uint160 lAddress = state->addressId();
			std::vector<State::BlockInfo> lInfos = state->infos();
			for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
				chainPeers_[(*lInfo).chain()].insert(lAddress);
			}
		}

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			lConsensus->second->pushState(state);
		}

		/*
		{
			// push
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			uint160 lAddress = state->addressId();
			std::vector<State::BlockInfo> lInfos = state->infos();
			for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
				chainPeers_[(*lInfo).chain()].insert(lAddress);
			}
		}

		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			for (std::map<uint256, IConsensusPtr>::iterator lConsensus = consensuses_.begin(); lConsensus != consensuses_.end(); lConsensus++) {
				lConsensus->second->pushState(state);
			}
		}
		*/

		return true;
	}

	//
	// pop state
	bool popState(StatePtr state) {
		//
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
		}

		// pop
		{
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			uint160 lAddress = state->addressId();
			std::vector<State::BlockInfo> lInfos = state->infos();
			for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
				chainPeers_[(*lInfo).chain()].erase(lAddress);
				if (!chainPeers_[(*lInfo).chain()].size()) chainPeers_.erase((*lInfo).chain()); 
			}
		}

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			lConsensus->second->popState(state);
		}

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
	// broadcast block header
	void broadcastBlockHeader(const NetworkBlockHeader& blockHeader, const uint160& except) {
		IConsensusPtr lConsensus = nullptr;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			std::map<uint256, IConsensusPtr>::iterator lConsensusPtr = consensuses_.find(const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().chain());
			if (lConsensusPtr != consensuses_.end()) lConsensus = lConsensusPtr->second;
		}

		if (lConsensus)
			lConsensus->broadcastBlockHeader(blockHeader, except);
	}

	//
	// broadcast block header
	void broadcastBlockHeaderAndState(const NetworkBlockHeader& blockHeader, StatePtr state, const uint160& except) {
		IConsensusPtr lConsensus = nullptr;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			std::map<uint256, IConsensusPtr>::iterator lConsensusPtr = consensuses_.find(const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().chain());
			if (lConsensusPtr != consensuses_.end()) lConsensus = lConsensusPtr->second;
		}

		if (lConsensus)
			lConsensus->broadcastBlockHeaderAndState(blockHeader, state, except);
	}

	//
	// broadcast airdrop request
	void broadcastAirdropRequest(const PKey& key, const uint160& except) {
		IConsensusPtr lConsensus = nullptr;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			std::map<uint256, IConsensusPtr>::iterator lConsensusPtr = consensuses_.find(MainChain::id());
			if (lConsensusPtr != consensuses_.end()) lConsensus = lConsensusPtr->second;
		}

		if (lConsensus)
			lConsensus->broadcastAirdropRequest(key, except);
	}

	//
	// b-cast state to clients only
	void broadcastStateToClients(StatePtr state) {
		//
		peerManager_->broadcastState(state);
	}

	//
	// broadcast transaction
	void broadcastTransaction(TransactionContextPtr ctx, const uint160& except) {
		IConsensusPtr lConsensus = nullptr;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			std::map<uint256, IConsensusPtr>::iterator lConsensusPtr = consensuses_.find(ctx->tx()->chain());
			if (lConsensusPtr != consensuses_.end()) lConsensus = lConsensusPtr->second;
		}

		if (lConsensus)
			lConsensus->broadcastTransaction(ctx, except);
	}

	//
	// check block
	IValidator::BlockCheckResult pushBlockHeader(const NetworkBlockHeader& blockHeader) {
		//
		IConsensusPtr lConsensus = nullptr;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			std::map<uint256, IConsensusPtr>::iterator lConsensusPtr = consensuses_.find(const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().chain());
			if (lConsensusPtr != consensuses_.end()) lConsensus = lConsensusPtr->second;
		}

		if (lConsensus) {
			IValidatorPtr lValidator = validatorManager_->locate(const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().chain());
			if (lValidator) {
				//
				IValidator::BlockCheckResult lResult = lConsensus->pushBlockHeader(blockHeader, lValidator);
				//
				if (lResult == IValidator::SUCCESS && !enqueueBlockHeader(blockHeader)) {
					gLog().write(Log::CONSENSUS, "[pushBlockHeader]: block is already PROCESSED " + 
						strprintf("%d/%s/%s#", const_cast<NetworkBlockHeader&>(blockHeader).height(), 
							const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().hash().toHex(), 
							const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().chain().toHex().substr(0, 10)));
					return IValidator::ALREADY_PROCESSED;
				}

				return lResult;
			}
		}

		return IValidator::VALIDATOR_ABSENT;
	}

	//
	// broadcast state
	void broadcastState(StatePtr state, const uint160& except) {
		// prepare
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
		}

		std::map<uint160, IPeerPtr> lPeers;
		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			lConsensus->second->collectPeers(lPeers);
		}

		for (std::map<uint160, IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			if (except.isNull() || (except != lPeer->second->addressId())) lPeer->second->broadcastState(state);
		}

		// re-broadcast to clients
		peerManager_->broadcastState(state);
	}

	//
	// make instance
	static IConsensusManagerPtr instance(ISettingsPtr settings) {
		return std::make_shared<ConsensusManager>(settings);
	}

	//
	// nodes supported given chain
	size_t chainSupportPeersCount(const uint256& chain) {
		//
		boost::unique_lock<boost::mutex> lLock(peersMutex_);
		std::map<uint256 /*chain*/, std::set<uint160>>::iterator lPeers = chainPeers_.find(chain);
		if (lPeers != chainPeers_.end()) return lPeers->second.size();

		return 0;
	}

	//
	// get block header
	void acquireBlockHeaderWithCoinbase(const uint256& block, const uint256& chain, INetworkBlockHandlerWithCoinBasePtr handler) {
		//
		boost::unique_lock<boost::mutex> lLock(peersMutex_);
		size_t lCount = 0;
		std::map<uint256 /*chain*/, std::set<uint160>>::iterator lPeers = chainPeers_.find(chain);
		if (lPeers == chainPeers_.end()) return;
		for (std::set<uint160>::iterator lPeer = lPeers->second.begin(); lPeer != lPeers->second.end() && lCount < settings_->minDAppNodes(); lPeer++) {
			std::map<uint160 /*peer*/, IPeerPtr>::iterator lPeerPtr = peers_.find(*lPeer);
			if (lPeerPtr != peers_.end()) {
				lPeerPtr->second->acquireBlockHeaderWithCoinbase(block, chain, handler);
				lCount++;
			}
		}
	}

	void setWallet(IWalletPtr wallet) { wallet_ = wallet; }
	void setStoreManager(ITransactionStoreManagerPtr storeManager) { storeManager_ = storeManager; }
	void setValidatorManager(IValidatorManagerPtr validatorManager) { validatorManager_ = validatorManager; }
	void setPeerManager(IPeerManagerPtr peerManager) { peerManager_ = peerManager; }

	IValidatorManagerPtr validatorManager() { return validatorManager_; }	
	ITransactionStoreManagerPtr storeManager() { return storeManager_; }
	IWalletPtr wallet() { return wallet_; }
	IMemoryPoolManagerPtr mempoolManager() { return validatorManager_->mempoolManager(); }
	IPeerManagerPtr peerManager() { return peerManager_; }

private:
	bool enqueueBlockHeader(const NetworkBlockHeader& blockHeader) {
		boost::unique_lock<boost::recursive_mutex> lLock(consensusesMutex_);
		uint256 lHash = const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().hash();
		for(std::list<uint256>::iterator lBlock = incomingBlockQueue_.begin(); lBlock != incomingBlockQueue_.end(); lBlock++) {
			if (lHash == *lBlock) return false;
		}

		incomingBlockQueue_.push_back(lHash);
		if (incomingBlockQueue_.size() > settings_->incomingBlockQueueLength()) incomingBlockQueue_.pop_front();
		return true;
	}

private:
	boost::recursive_mutex consensusesMutex_;
	boost::mutex peersMutex_;
	boost::mutex latencyMutex_;
	boost::mutex incomingBlockQueueMutex_;

	std::list<uint256> incomingBlockQueue_;

	std::map<uint256, IConsensusPtr> consensuses_;
	ISettingsPtr settings_;
	IWalletPtr wallet_;
	ITransactionStoreManagerPtr storeManager_;
	IValidatorManagerPtr validatorManager_;
	IPeerManagerPtr peerManager_;

	typedef uint160 peer_t;

	typedef std::multimap<uint32_t /*latency*/, IPeerPtr> LatencyMap;
	LatencyMap latencyMap_;

	std::set<std::string> pushedPeers_;
	std::map<uint160 /*peer*/, IPeerPtr> peers_;
	std::map<uint256 /*chain*/, std::set<uint160>> chainPeers_;
};

} // qbit

#endif
