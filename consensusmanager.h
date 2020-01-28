// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CONSENSUS_MANAGER_H
#define QBIT_CONSENSUS_MANAGER_H

#include "itransactionstoremanager.h"
#include "iconsensusmanager.h"
#include "consensus.h"

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

	void push(const uint256& chain) {
		//
		boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
		if (consensuses_.find(chain) == consensuses_.end()) {
			IConsensusPtr lConsensus = Consensus::instance(chain, shared_from_this(), settings_, wallet_, storeManager_->locate(chain));
			consensuses_[chain] = lConsensus;
		}
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
				size_t lHeight = lStore->currentHeight(lHeader);
				lState->addHeader(lHeader, lHeight);
			}
		}

		return lState;
	}

	//
	// block count (100 blocks)
	size_t quarantineTime() { return 100; /* settings? */ }	

	//
	// use peer for network participation
	void pushPeer(IPeerPtr peer) {
		//
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
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
				if (lPeerId == lTo->second->addressId())
					latencyMap_.erase(lTo);

			// update latency
			latencyMap_.insert(LatencyMap::value_type(peer->latency(), peer));
		}

		// update state
		//pushState(State::instance(peer->state()));
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

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			lConsensus->second->popPeer(peer);
		}

		{
			boost::unique_lock<boost::mutex> lLock(latencyMutex_);
			// locate latency
			std::pair<LatencyMap::iterator,
						LatencyMap::iterator> lRange = latencyMap_.equal_range(peer->latencyPrev());

			for (LatencyMap::iterator lTo = lRange.first; lTo != lRange.second; lTo++)
				if (lPeerId == lTo->second->addressId())
					latencyMap_.erase(lTo);
		}

		// remove state
		//popState(State::instance(peer->state()));
	}	

	//
	// push state
	bool pushState(StatePtr state) {
		//
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
		}

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			if (!lConsensus->second->pushState(state))
				return false;
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

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			if (!lConsensus->second->popState(state))
				return false;
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
	void broadcastBlockHeader(const NetworkBlockHeader& blockHeader, IPeerPtr except) {
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
	// broadcast transaction
	void broadcastTransaction(TransactionContextPtr ctx, uint160 except) {
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
	bool pushBlockHeader(const NetworkBlockHeader& blockHeader) {
		//
		if (!enqueueBlockHeader(blockHeader)) {
			gLog().write(Log::CONSENSUS, "[pushBlockHeader]: block is already PROCESSED " + 
				strprintf("%s/%d/%s#", const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().hash().toHex(), 
					const_cast<NetworkBlockHeader&>(blockHeader).height(), 
					const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().chain().toHex().substr(0, 10)));
			return false;
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
		return false;
	}

	//
	// broadcast state
	void broadcastState(StatePtr state, IPeerPtr except) {
		// prepare
		std::map<uint256, IConsensusPtr> lConsensuses;
		{
			boost::unique_lock<boost::mutex> lLock(consensusesMutex_);
			lConsensuses = consensuses_; // copy
		}

		for (std::map<uint256, IConsensusPtr>::iterator lConsensus = lConsensuses.begin(); lConsensus != lConsensuses.end(); lConsensus++) {
			lConsensus->second->broadcastState(state, except);
		}
	}

	//
	// make instance
	static IConsensusManagerPtr instance(ISettingsPtr settings) {
		return std::make_shared<ConsensusManager>(settings);
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
};

} // qbit

#endif
