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

	bool exists(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
		return consensuses_.find(chain) != consensuses_.end();
	}
 
	IConsensusPtr locate(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
		std::map<uint256, IConsensusPtr>::iterator lConsensus = consensuses_.find(chain);
		if (lConsensus != consensuses_.end()) return lConsensus->second;
		return nullptr;
	}

	bool add(IConsensusPtr consensus) {
		//
		boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
		if (consensuses_.find(consensus->chain()) == consensuses_.end()) {
			consensuses_[consensus->chain()] = consensus;
			return true;
		}

		return false;
	}

	void pop(const uint256& chain) {
		//
		boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
		std::map<uint256, IConsensusPtr>::iterator lConsensus = consensuses_.find(chain);
		if (lConsensus != consensuses_.end()) {
			lConsensus->second->close();
			consensuses_.erase(chain);
		}
	}

	IConsensusPtr push(const uint256& chain) {
		//
		boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
		if (consensuses_.find(chain) == consensuses_.end()) {
			IConsensusPtr lConsensus = Consensus::instance(chain, shared_from_this(), settings_, wallet_, storeManager_->locate(chain));
			consensuses_[chain] = lConsensus;
			return lConsensus;
		}

		return nullptr;
	}

	std::vector<IConsensusPtr> consensuses() {
		//
		boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
		std::vector<IConsensusPtr> lConsensuses;

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = consensuses_.begin(); lConsensus != consensuses_.end(); lConsensus++) {
			lConsensuses.push_back(lConsensus->second);
		}

		return lConsensuses;
	}

	//
	// collect current state
	StatePtr currentState() {
		StatePtr lState = State::instance(qbit::getTime(), settings_->roles(), wallet_->firstKey().createPKey());

		//
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
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
		if (peer->state().client()) return true;

		//
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
		}

		{
			// push
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			uint160 lAddress = peer->addressId();
			peers_.insert(std::map<uint160 /*peer*/, IPeerPtr>::value_type(lAddress, peer));
			std::vector<State::BlockInfo> lInfos = peer->state().infos();
			for (std::vector<State::BlockInfo>::iterator lInfo = lInfos.begin(); lInfo != lInfos.end(); lInfo++) {
				chainPeers_[(*lInfo).chain()].insert(lAddress);
			}
		}

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			lConsensus->second->pushPeer(peer);
		}

		{
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
		}

		return true;
	}

	//
	// remove peer from consensus participation
	void popPeer(IPeerPtr peer) {
		//
		peer_t lPeerId = peer->addressId();

		//
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
		}

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
		}		

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			lConsensus->second->popPeer(peer);
		}

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
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
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

		return true;
	}

	//
	// pop state
	bool popState(StatePtr state) {
		//
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
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
	SKey mainKey() { return wallet_->firstKey(); }	

	//
	// main key
	PKey mainPKey() {
		return mainKey().createPKey();
	}	

	//
	// broadcast block header
	void broadcastBlockHeader(const NetworkBlockHeader& blockHeader, const uint160& except) {
		IConsensusPtr lConsensus = nullptr;
		{
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
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
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
			std::map<uint256, IConsensusPtr>::iterator lConsensusPtr = consensuses_.find(const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().chain());
			if (lConsensusPtr != consensuses_.end()) lConsensus = lConsensusPtr->second;
		}

		if (lConsensus)
			lConsensus->broadcastBlockHeaderAndState(blockHeader, state, except);
	}

	//
	// broadcast transaction
	void broadcastTransaction(TransactionContextPtr ctx, const uint160& except) {
		IConsensusPtr lConsensus = nullptr;
		{
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
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
		if (!enqueueBlockHeader(blockHeader)) {
			gLog().write(Log::CONSENSUS, "[pushBlockHeader]: block is already PROCESSED " + 
				strprintf("%d/%s/%s#", const_cast<NetworkBlockHeader&>(blockHeader).height(), 
					const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().hash().toHex(), 
					const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().chain().toHex().substr(0, 10)));
			return IValidator::ALREADY_PROCESSED;
		}

		//
		IConsensusPtr lConsensus = nullptr;
		{
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
			std::map<uint256, IConsensusPtr>::iterator lConsensusPtr = consensuses_.find(const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().chain());
			if (lConsensusPtr != consensuses_.end()) lConsensus = lConsensusPtr->second;
		}

		if (lConsensus)
			return lConsensus->pushBlockHeader(blockHeader, validatorManager_->locate(const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().chain()));
		return IValidator::VALIDATOR_ABSENT;
	}

	//
	// broadcast state
	void broadcastState(StatePtr state, const uint160& except) {
		// prepare
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
		}

		std::map<uint160, IPeerPtr> lPeers;
		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			lConsensus->second->collectPeers(lPeers);
		}

		for (std::map<uint160, IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			if (except.isNull() || (except != lPeer->second->addressId())) lPeer->second->broadcastState(state);
		}
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

	IValidatorManagerPtr validatorManager() { return validatorManager_; }	
	ITransactionStoreManagerPtr storeManager() { return storeManager_; }
	IWalletPtr wallet() { return wallet_; }
	IMemoryPoolManagerPtr mempoolManager() { return validatorManager_->mempoolManager(); }

private:
	bool enqueueBlockHeader(const NetworkBlockHeader& blockHeader) {
		boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
		uint256 lHash = const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().hash();
		for(std::list<uint256>::iterator lBlock = incomingBlockQueue_.begin(); lBlock != incomingBlockQueue_.end(); lBlock++) {
			if (lHash == *lBlock) return false;
		}

		incomingBlockQueue_.push_back(lHash);
		if (incomingBlockQueue_.size() > settings_->incomingBlockQueueLength()) incomingBlockQueue_.pop_front();
		return true;
	}

private:
	boost::mutex consensusesMutex_;
	boost::mutex peersMutex_;
	boost::mutex latencyMutex_;
	boost::mutex incomingBlockQueueMutex_;

	std::list<uint256> incomingBlockQueue_;

	std::map<uint256, IConsensusPtr> consensuses_;
	ISettingsPtr settings_;
	IWalletPtr wallet_;
	ITransactionStoreManagerPtr storeManager_;
	IValidatorManagerPtr validatorManager_;

	typedef uint160 peer_t;

	typedef std::multimap<uint32_t /*latency*/, IPeerPtr> LatencyMap;
	LatencyMap latencyMap_;

	std::map<uint160 /*peer*/, IPeerPtr> peers_;
	std::map<uint256 /*chain*/, std::set<uint160>> chainPeers_;
};

} // qbit

#endif
