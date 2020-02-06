// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CONSENSUS_H
#define QBIT_CONSENSUS_H

#include "iconsensus.h"
#include "iwallet.h"
#include "timestamp.h"
#include "ivalidator.h"
#include "ivalidatormanager.h"
#include "itransactionstoremanager.h"
#include "log/log.h"

#include <iterator>

namespace qbit {

// TODO: all checks by active peers: height|block;
class Consensus: public IConsensus, public std::enable_shared_from_this<Consensus> {
public:
	Consensus(const uint256& chain, IConsensusManagerPtr consensusManager, ISettingsPtr settings, IWalletPtr wallet, ITransactionStorePtr store) :
		chain_(chain), consensusManager_(consensusManager), settings_(settings), wallet_(wallet), store_(store) { chainState_ = IConsensus::UNDEFINED; }

	//
	// max block size
	size_t maxBlockSize() { return 1024 * 1024 * 1; }

	//
	// current time (adjusted? averaged?)
	uint64_t currentTime() {
		uint64_t lTime = qbit::getTime();
		uint64_t lMedianTime = consensusManager_->medianTime();

		if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
			strprintf("[currentTime]: local %d, median %d, avg %d", lTime, lMedianTime, (lTime + lMedianTime)/2));

		return (lTime + lMedianTime)/2; 
	}
	
	StatePtr currentState() {
		return consensusManager_->currentState();
	}

	//
	// settings
	ISettingsPtr settings() { return settings_; }

	//
	// block time for main chain, ms
	// TODO: settings
	virtual size_t blockTime() { return 5000; }

	//
	// block count (100 blocks)
	// TODO: settings
	virtual size_t quarantineTime() { return 100; }

	//
	// maturity period (blocks)
	// TODO: settings
	virtual size_t maturity() { return 5; }	

	//
	// coinbase maturity period (blocks)
	// TODO: settings
	virtual size_t coinbaseMaturity() { return 5; }	

	//
	// mining/emission schedule control
	virtual bool checkBalance(amount_t coinbaseAmount, amount_t blockFee, size_t height) {
		// TODO: implement
		// coinbaseAmount - blockFee should/can be in schedule, for example
		// 1		- 1000000 block height, coinbase = 1 qbit
		// 1000001	- 2000000 block height, coinbase = 0.9 qbit
		// 2000001	- 3000000 block height, coinbase = 0.8 qbit
		// ...
		// NOTE: height might be a bit floating, so need flexible borders

		return true;
	}

	//
	// PoW/PoS work sequnce with "block" and "block->prev"
	virtual bool checkSequenceConsistency(const BlockHeader& /*block*/) {
		return true;
	}

	//
	// minimum network size
	// TODO: settings
	virtual size_t simpleNetworkSize() { return 5; }	

	//
	// use peer for network participation
	void pushPeer(IPeerPtr peer) {
		//
		if (peer->state().containsChain(chain_)) {
			//
			peer_t lPeerId = peer->addressId();
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			if (directPeerMap_.find(lPeerId) == directPeerMap_.end()) {
				directPeerMap_[lPeerId] = peer;
				pushState(State::instance(peer->state()));
			}
		}
	}

	//
	// remove peer from consensus participation
	void popPeer(IPeerPtr peer) {
		//
		if (peer->state().containsChain(chain_)) {
			//
			peer_t lPeerId = peer->addressId();
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			directPeerMap_.erase(lPeerId);

			popState(State::instance(peer->state()));
		}
	}

	//
	// push acquired block
	//bool pushBlock(BlockPtr block) {
		// TODO: implement
	//	return false;
	//}

	//
	//
	bool acquireBlock(const NetworkBlockHeader& block) {
		// 1. try to look at direct peers
		IPeerPtr lPeer = nullptr;
		{
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			PeersMap::iterator lPeerPtr = directPeerMap_.find(const_cast<NetworkBlockHeader&>(block).blockHeader().origin());
			if (lPeerPtr != directPeerMap_.end()) {
				lPeer = lPeerPtr->second;
			}
		}

		// 2. if !lPeer - search for connected peer with such block
		if (!lPeer) {
			//
			peer_t lPeerId;
			{
				boost::unique_lock<boost::mutex> lLock(stateMutex_);

				HeightMap::iterator lStateMap = heightMap_.find(const_cast<NetworkBlockHeader&>(block).height());
				if (lStateMap != heightMap_.end()) {
					//
					StateMap::iterator lPeerSet = lStateMap->second.find(const_cast<NetworkBlockHeader&>(block).blockHeader().hash());
					if(lPeerSet != lStateMap->second.end() && lPeerSet->second.size()) {
						//
						lPeerId = *lPeerSet->second.begin(); // TODO: take first of? or near by latency
					}
				}
			}

			// select peer
			if (!lPeerId.isEmpty()) {
				boost::unique_lock<boost::mutex> lLock(peersMutex_);
				PeersMap::iterator lPeerPtr = directPeerMap_.find(lPeerId);
				if (lPeerPtr != directPeerMap_.end()) {
					lPeer = lPeerPtr->second;
				}
			}
		}

		if (lPeer) {
			if (store_->enqueueBlock(block)) {
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
					strprintf("[acquireBlock]: acquiring BLOCK %s/%s#", 
						const_cast<NetworkBlockHeader&>(block).blockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));
				lPeer->acquireBlock(block);
			} else {
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
					strprintf("[acquireBlock]: block is already ENQUEUED %s/%s#, skipping.", 
						const_cast<NetworkBlockHeader&>(block).blockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));
			}
		} else {
			return false;
		}

		return true;
	}

	//
	// check block
	IValidator::BlockCheckResult pushBlockHeader(const NetworkBlockHeader& blockHeader, IValidatorPtr validator) {
		// check work
		IValidator::BlockCheckResult lResult = validator->checkBlockHeader(blockHeader);

		//
		if (lResult == IValidator::SUCCESS) {
			//
			size_t lPeersCount = 0;
			{
				boost::unique_lock<boost::mutex> lLock(peersMutex_);
				lPeersCount = directPeerMap_.size();
			}

			// process
			if (!validator->acceptBlockHeader(blockHeader)) {
				// peers not found
				return IValidator::PEERS_IS_ABSENT;
			}
		}

		// reject
		return lResult;
	}

	//
	// broadcast block header
	void broadcastBlockHeader(const NetworkBlockHeader& blockHeader, const uint160& except) {
		// prepare
		std::list<IPeerPtr> lPeers;
		{
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			for (PeersMap::iterator lItem = directPeerMap_.begin(); lItem != directPeerMap_.end(); lItem++) {
				lPeers.push_back(lItem->second);
			}
		}

		// broadcast
		for (std::list<IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			if (except.isNull() || (except != (*lPeer)->addressId())) (*lPeer)->broadcastBlockHeader(blockHeader);
		}
	}

	//
	// broadcast block header and state
	void broadcastBlockHeaderAndState(const NetworkBlockHeader& block, StatePtr state, const uint160& except) {
		//
		if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
			strprintf("[broadcastTransaction]: broadcasting block header and state %s/%s#", 
				const_cast<NetworkBlockHeader&>(block).blockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));

		// prepare
		std::list<IPeerPtr> lPeers;
		{
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			for (PeersMap::iterator lItem = directPeerMap_.begin(); lItem != directPeerMap_.end(); lItem++) {
				lPeers.push_back(lItem->second);
			}
		}

		// broadcast
		bool lResult = false;
		for (std::list<IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			if (except.isNull() || (except != (*lPeer)->addressId())) {
				(*lPeer)->broadcastBlockHeaderAndState(block, state);
				lResult = true; // at least one
			}
		}
	}

	//
	// broadcast transaction
	bool broadcastTransaction(TransactionContextPtr ctx, const uint160& except) {
		//
		if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
			strprintf("[broadcastTransaction]: broadcasting transaction %s/%s#", ctx->tx()->id().toHex(), chain_.toHex().substr(0, 10)));

		// prepare
		std::list<IPeerPtr> lPeers;
		{
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			for (PeersMap::iterator lItem = directPeerMap_.begin(); lItem != directPeerMap_.end(); lItem++) {
				lPeers.push_back(lItem->second);
			}
		}

		// broadcast
		bool lResult = false;
		for (std::list<IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			if (except.isNull() || (except != (*lPeer)->addressId())) {
				(*lPeer)->broadcastTransaction(ctx);
				lResult = true; // at least one
			}
		}

		if (lResult) {
			if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
				strprintf("[broadcastTransaction]: removing pending transaction %s/%s#", ctx->tx()->id().toHex(), chain_.toHex().substr(0, 10)));

			wallet_->removePendingTransaction(ctx->tx()->id());
		} else {
			if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
				strprintf("[broadcastTransaction]: transaction is in pending state %s/%s#", ctx->tx()->id().toHex(), chain_.toHex().substr(0, 10)));
		}

		return lResult; 
	}

	//
	// broadcast state
	void broadcastState(StatePtr state, const uint160& except) {
		//
		if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
			strprintf("[broadcastState]: broadcasting state - %s", state->toStringShort()));

		// prepare
		std::list<IPeerPtr> lPeers;
		{
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			for (PeersMap::iterator lItem = directPeerMap_.begin(); lItem != directPeerMap_.end(); lItem++) {
				lPeers.push_back(lItem->second);
			}
		}

		// broadcast
		for (std::list<IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			if (except.isNull() || (except != (*lPeer)->addressId())) (*lPeer)->broadcastState(state);
		}
	}

	//
	// push state
	bool pushState(StatePtr state) {
		// prepare
		State::BlockInfo lInfo;
		if (state->locateChain(chain_, lInfo)) {
			//		
			boost::unique_lock<boost::mutex> lLock(stateMutex_);

			// peer id
			peer_t lPeerId = state->addressId();

			// previous/exists
			PeersStateMap::iterator lState = peerStateMap_.find(lPeerId);
			if (lState == peerStateMap_.end()) {
				peerStateMap_[lPeerId] = state;
			} else {
				// check for updates
				if (state->equals(lState->second)) return false; // already upated

				// clean-up
				peerSet_.erase(lPeerId);

				HeightMap::iterator lStateMap = heightMap_.find(lInfo.height());
				if (lStateMap != heightMap_.end()) {
					//
					StateMap::iterator lPeerSet = lStateMap->second.find(lInfo.hash());
					if(lPeerSet != lStateMap->second.end()) {
						lPeerSet->second.erase(lPeerId);

						// remove set
						if (!lPeerSet->second.size()) {
							lStateMap->second.erase(lPeerSet);
						}
					}

					// remove map
					if (!lStateMap->second.size()) {
						heightMap_.erase(lStateMap);
					}						
				}
			}

			// height -> block = peer
			heightMap_[lInfo.height()][lInfo.hash()].insert(lPeerId);

			gLog().write(Log::CONSENSUS, 
				strprintf("[pushState]: %d/%s/%s/%s#", lInfo.height(), lInfo.hash().toHex(), lPeerId.toHex(), chain_.toHex().substr(0, 10)));

			// all peers
			peerSet_.insert(lPeerId);

			// update state
			peerStateMap_[lPeerId] = state;
		}

		return true;
	}

	//
	// pop state
	bool popState(StatePtr state) {
		// prepare
		State::BlockInfo lInfo;
		if (state->locateChain(chain_, lInfo)) {
			//		
			boost::unique_lock<boost::mutex> lLock(stateMutex_);

			// peer id
			peer_t lPeerId = state->addressId();

			// previous/exists
			PeersStateMap::iterator lState = peerStateMap_.find(lPeerId);
			if (lState != peerStateMap_.end()) {
				// clean-up
				peerSet_.erase(lPeerId);

				HeightMap::iterator lStateMap = heightMap_.find(lInfo.height());
				if (lStateMap != heightMap_.end()) {
					//
					StateMap::iterator lPeerSet = lStateMap->second.find(lInfo.hash());
					if(lPeerSet != lStateMap->second.end()) {
						lPeerSet->second.erase(lPeerId);

						// remove set
						if (!lPeerSet->second.size()) {
							lStateMap->second.erase(lPeerSet);
						}
					}

					// remove map
					if (!lStateMap->second.size()) {
						heightMap_.erase(lStateMap);
					}
				}
			}
		}

		return true;
	}

	//
	// network is simple?  
	bool isSimpleNetwork() {
		boost::unique_lock<boost::mutex> lLock(stateMutex_);
		return peerSet_.size() < simpleNetworkSize();
	}

	//
	// check if chain synchronized
	bool isChainSynchronized() {
		//
		BlockHeader lHeader;
		size_t lHeight = store_->currentHeight(lHeader);

		bool lResult = false;
		{
			boost::unique_lock<boost::mutex> lLock(stateMutex_);
			//
			if (heightMap_.size()) {
				HeightMap::reverse_iterator lStateMap = heightMap_.rbegin();
				if (lStateMap->first > lHeight) {
					lResult = false;
				} else lResult = true;
			} else lResult = false;
		}

		gLog().write(Log::CONSENSUS, 
			strprintf("[isChainSynchronized]: %s/%d/%s/%s#", 
				(lResult ? "true" : "FALSE"), lHeight, lHeader.hash().toHex(), chain_.toHex().substr(0, 10)));

		return lResult;
	}

	//
	// find fully synced root
	size_t locateSynchronizedRoot(std::list<IPeerPtr>& peers, uint256& block) {
		//
		size_t lResultHeight = 0;
		//
		PeersSet lPeers;
		BlockHeader lHeader;
		size_t lHeight = store_->currentHeight(lHeader);		
		{
			boost::unique_lock<boost::mutex> lLock(stateMutex_);
			// reverse traversal of heights of the chain
			bool lFound = false;
			for (HeightMap::reverse_iterator lHeightPos = heightMap_.rbegin(); !lFound && lHeightPos != heightMap_.rend(); lHeightPos++) {
				for (StateMap::iterator lState = lHeightPos->second.begin(); lState != lHeightPos->second.end(); lState++) {
					if (lHeightPos->first > lHeight) {
						// collect known peers for the synchronized root
						lPeers.insert(lState->second.begin(), lState->second.end());
						// height
						lResultHeight = lHeightPos->first;
						// block
						block = lState->first;

						gLog().write(Log::CONSENSUS, 
							strprintf("[locateSynchronizedRoot]: ROOT [%d]/%s/%s#", 
								lResultHeight, block.toHex(), chain_.toHex().substr(0, 10)));

						lFound = true;

						break;
					} else if (lHeightPos->first == lHeight) {
						gLog().write(Log::CONSENSUS, 
							strprintf("[locateSynchronizedRoot]: already SYNCHRONIZED %d/%s/%s#", 
								lHeightPos->first, lState->first.toHex(), chain_.toHex().substr(0, 10)));
						break;
					}
				}
			}
		}

		{
			boost::unique_lock<boost::mutex> lLock(peersMutex_);
			//
			for (PeersSet::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
				PeersMap::iterator lPeerPtr = directPeerMap_.find(*lPeer);
				if (lPeerPtr != directPeerMap_.end()) {
					peers.push_back(lPeerPtr->second);
				}
			}
		}

		return lResultHeight;
	}

	//
	// current chain state
	ChainState chainState() {
		//
		if (chainState_ == IConsensus::UNDEFINED) {
			//
			bool lIsSynchronized = isChainSynchronized();
			{
				boost::unique_lock<boost::mutex> lLock(transitionMutex_);
				if (lIsSynchronized) chainState_ = IConsensus::SYNCHRONIZED;
				else chainState_ = IConsensus::NOT_SYNCRONIZED;
			}
		}

		return chainState_;
	}

	//
	// last sync job
	SynchronizationJobPtr lastJob() {
		return job_;
	}

	//
	// begin synchronization
	bool doSynchronize() {
		//
		bool lProcess = false; 
		{
			boost::unique_lock<boost::mutex> lLock(transitionMutex_);
			if (chainState_ == IConsensus::UNDEFINED || chainState_ == IConsensus::NOT_SYNCRONIZED) {
				chainState_ = IConsensus::SYNCHRONIZING;
				lProcess = true;
			}
		}

		if (lProcess) {
			std::list<IPeerPtr> lPeers;
			uint256 lBlock;
			size_t lHeight = locateSynchronizedRoot(lPeers, lBlock); // get peers, height and block
			if (lHeight && lPeers.size()) {
				if (settings_->isFullNode()) {
					// store is empty - full sync
					BlockHeader lHeader;
					if (!store_->currentHeight(lHeader)) {
						job_ = SynchronizationJob::instance(lHeight, lBlock); // height from
						for(std::list<IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
							(*lPeer)->synchronizeFullChain(shared_from_this(), job_);
						}
					} else {
						std::list<IPeerPtr>::iterator lPeer = lPeers.begin(); // just first?
						job_ = SynchronizationJob::instance(lBlock); // block from
						(*lPeer)->synchronizeFullChainHead(shared_from_this(), job_);
					}
				} else if (settings_->isNode()) {

				} else {
					gLog().write(Log::CONSENSUS, "[doSynchronize]: synchronization is allowed for NODE or FULLNODE only.");
					//
					boost::unique_lock<boost::mutex> lLock(transitionMutex_);
					chainState_ = IConsensus::SYNCHRONIZED;

					return true;
				}			
			} else {
				// there is no peers
				boost::unique_lock<boost::mutex> lLock(transitionMutex_);
				chainState_ = IConsensus::SYNCHRONIZED;

				return true;
			}
		}

		return false;
	}

	//
	// to indexing
	void doIndex(const uint256& block) {
		//
		bool lProcess = false; 
		{
			boost::unique_lock<boost::mutex> lLock(transitionMutex_);
			if (chainState_ == IConsensus::SYNCHRONIZING) {
				chainState_ = IConsensus::INDEXING;
				lProcess = true;
			}
		}

		if (lProcess) {
			// full re-index
			store_->reindexFull(block, consensusManager_->mempoolManager()->locate(chain_));

			// check
			bool lIsSynchronized = isChainSynchronized();
			{
				boost::unique_lock<boost::mutex> lLock(transitionMutex_);
				if (lIsSynchronized) chainState_ = IConsensus::SYNCHRONIZED;
				else chainState_ = IConsensus::NOT_SYNCRONIZED;
			}
		}
	}

	//
	// to indexing
	bool doIndex(const uint256& block, const uint256& lastBlock) {
		//
		bool lProcess = false; 
		{
			boost::unique_lock<boost::mutex> lLock(transitionMutex_);
			if (chainState_ == IConsensus::SYNCHRONIZING) {
				chainState_ = IConsensus::INDEXING;
				lProcess = true;
			}
		}

		if (lProcess) {
			// partial re-index
			if (!store_->reindex(block, lastBlock, consensusManager_->mempoolManager()->locate(chain_))) {
				// partial reindex for given blocks intervale is not possible
				return false;
			}

			// check
			bool lIsSynchronized = isChainSynchronized();
			{
				boost::unique_lock<boost::mutex> lLock(transitionMutex_);
				if (lIsSynchronized) chainState_ = IConsensus::SYNCHRONIZED;
				else chainState_ = IConsensus::NOT_SYNCRONIZED;
			}
		}

		return true;
	}

	//
	// to non-sync
	void toNonSynchronized() {
		{
			boost::unique_lock<boost::mutex> lLock(transitionMutex_);
			if (chainState_ == IConsensus::SYNCHRONIZING || chainState_ == IConsensus::SYNCHRONIZED || chainState_ == IConsensus::INDEXING) {
				chainState_ = IConsensus::NOT_SYNCRONIZED;
			}
		}
	}

	//
	// to sync'ing
	void toSynchronizing() {
		{
			boost::unique_lock<boost::mutex> lLock(transitionMutex_);
			if (chainState_ == IConsensus::NOT_SYNCRONIZED || chainState_ == IConsensus::SYNCHRONIZED) {
				chainState_ = IConsensus::SYNCHRONIZING;
			}
		}
	}

	//
	// main key
	SKey mainKey() {
		return wallet_->firstKey();
	}

	ITransactionStorePtr store() { return store_; }

	uint256 chain() { return chain_; }

	static IConsensusPtr instance(const uint256& chain, IConsensusManagerPtr consensusManager, ISettingsPtr settings, IWalletPtr wallet, ITransactionStorePtr store) { 
		return std::make_shared<Consensus>(chain, consensusManager, settings, wallet, store);
	}

	std::string chainStateString() {
		switch(chainState_) {
			case IConsensus::UNDEFINED: return "UNDEFINED";
			case IConsensus::NOT_SYNCRONIZED: return "NOT_SYNCRONIZED";
			case IConsensus::SYNCHRONIZING: return "SYNCHRONIZING";
			case IConsensus::INDEXING: return "INDEXING";
			case IConsensus::SYNCHRONIZED: return "SYNCHRONIZED";
		}

		return "ESTATE";
	}	

private:
	uint256 chain_;
	ISettingsPtr settings_;
	IWalletPtr wallet_;
	ITransactionStorePtr store_;
	IConsensus::ChainState chainState_;
	SynchronizationJobPtr job_;
	IConsensusManagerPtr consensusManager_;

	//
	typedef uint160 peer_t;
	typedef uint256 chain_t;
	typedef uint256 block_t;
	typedef uint64_t time_t;
	typedef uint32_t height_t;

	//
	typedef std::map<peer_t, IPeerPtr> PeersMap;
	typedef std::map<peer_t, StatePtr> PeersStateMap;

	//
	// chain -> height -> block_id -> peer
	typedef std::set<peer_t> PeersSet;
	typedef std::map<block_t, PeersSet> StateMap;
	typedef std::map<height_t, StateMap> HeightMap;

	//
	// chain -> time -> peer
	typedef std::map<time_t, peer_t> BlockTimePeerMap;

	//
	// chain -> active peers
	typedef std::map<chain_t, std::set<peer_t>> ChainPeersMap;

	boost::mutex peersMutex_;
	boost::mutex stateMutex_;
	boost::mutex queueMutex_;

	boost::mutex transitionMutex_;

	PeersStateMap peerStateMap_; // peer_id -> State
	PeersMap directPeerMap_; // peer_id -> IPeer
	PeersSet peerSet_;

	BlockTimePeerMap blockTimePeer_;

	HeightMap heightMap_;
};

} // qbit

#endif
