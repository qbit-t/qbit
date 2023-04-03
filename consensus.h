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
#include "hash/cuckoo.h"

#include <iterator>

namespace qbit {

// TODO: all checks by active peers: height|block;
class Consensus: public IConsensus, public std::enable_shared_from_this<Consensus> {
public:
	Consensus(const uint256& chain, IConsensusManagerPtr consensusManager, ISettingsPtr settings, IWalletPtr wallet, ITransactionStorePtr store) :
		chain_(chain), consensusManager_(consensusManager), settings_(settings), wallet_(wallet), store_(store) { chainState_ = IConsensus::UNDEFINED; }

	//
	//
	void setDApp(const std::string& name) { dApp_ = name; }
	std::string dApp() { return	dApp_; }

	//
	// max block size
	virtual uint32_t maxBlockSize() { return 1024 * 1024 * 1; }

	//
	// current time (adjusted? averaged?)
	uint64_t currentTime() {
		uint64_t lTime = qbit::getTime();
		uint64_t lMedianTime = qbit::getMedianTime();

		return (lTime + lMedianTime)/2;
	}
	
	StatePtr currentState() {
		return consensusManager_->currentState();
	}

	bool close() {
		settings_.reset();
		wallet_.reset();
		store_.reset();
		consensusManager_.reset();

		return true;
	}

	IConsensusManagerPtr consensusManager() {
		return consensusManager_;
	}

	//
	// settings
	ISettingsPtr settings() { return settings_; }

	//
	// block time for main chain, ms
	// TODO: settings
	virtual uint32_t blockTime() { return 2000; }

	//
	// block count (100 blocks)
	// TODO: settings
	virtual uint32_t quarantineTime() { return 100; }

	//
	// maturity period (blocks)
	virtual uint32_t maturity() {
		// TODO: select from settings by dApp_ if dapp != "" || dapp_ != "none"
		return settings_->mainChainMaturity(); 
	}

	//
	// coinbase maturity period (blocks)
	virtual uint32_t coinbaseMaturity() {
		// TODO: select from settings by dApp_ if dapp != "" || dapp_ != "none"
		return settings_->mainChainCoinbaseMaturity(); 
	}

	//
	// mining/emission schedule control
	virtual bool checkBalance(amount_t coinbaseAmount, amount_t blockFee, uint64_t height) {
		//
		// coinbaseAmount = reward + blockFee
		if (coinbaseAmount <= blockFee) return false;

		//
		uint64_t lTargetReward = blockReward(height);
		if (lTargetReward >= (coinbaseAmount - blockFee)) return true;

		return false;
	}

	//
	// calc current reward
	amount_t blockReward(uint64_t height) {
		//
		uint64_t lBlocksPerYear = 31536000/(blockTime()/1000); // 365*24*60*(60/2)
		uint64_t lYear = height / lBlocksPerYear;

		if (height == 1 /*first block*/) {
			return ((uint64_t)100000000) * ((uint64_t)7000000); // three million qbits as premine for user unboarding
		}

		switch(lYear) {
			case   0: return 100000000; // first year
			case   1: return  90000000; // second year
			case   2: return  81000000; // third year
			case   3: return  72900000; // ...
			case   4: return  65610000;
			case   5: return  59049000;
			case   6: return  53144100;
			case   7: return  47829690;
			case   8: return  43046721;
			case   9: return  38742049;

			case  10: return  34867844;
			case  11: return  31381060;
			case  12: return  28242954;
			case  13: return  25418658;
			case  14: return  22876792;
			case  15: return  20589113;
			case  16: return  18530202;
			case  17: return  16677182;
			case  18: return  15009464;
			case  19: return  13508517;

			case  20: return  12157665;
			case  21: return  10941899;
			case  22: return   9847709;
			case  23: return   8862938;
			case  24: return   7976644;
			case  25: return   7178980;
			case  26: return   6461082;
			case  27: return   5814974;
			case  28: return   5233476;
			case  29: return   4710129;

			case  30: return   4239116;
			case  31: return   3815204;
			case  32: return   3433684;
			case  33: return   3090315;
			case  34: return   2781284;
			case  35: return   2503156;
			case  36: return   2252840;
			case  37: return   2027556;
			case  38: return   1824800;
			case  39: return   1642320;

			case  40: return   1478088;
			case  41: return   1330279;
			case  42: return   1197252;
			case  43: return   1077526;
			case  44: return    969774;
			case  45: return    872796;
			case  46: return    785517;
			case  47: return    706965;
			case  48: return    636269;
			case  49: return    572642;

			case  50: return    515378;
			case  51: return    463840;
			case  52: return    417456;
			case  53: return    375710;
			case  54: return    338139;
			case  55: return    304325;
			case  56: return    273893;
			case  57: return    246503;
			case  58: return    221853;
			case  59: return    199668;

			case  60: return    179701;
			case  61: return    161731;
			case  62: return    145558;
			case  63: return    131002;
			case  64: return    117902;
			case  65: return    106112;
			case  66: return     95500;
			case  67: return     85950;
			case  68: return     77355;
			case  69: return     69620;

			case  70: return     62658;
			case  71: return     56392;
			case  72: return     50753;
			case  73: return     45678;
			case  74: return     41110;
			case  75: return     36999;
			case  76: return     33299;
			case  77: return     29969;
			case  78: return     26972;
			case  79: return     24275;

			case  80: return     21847;
			case  81: return     19663;
			case  82: return     17696;
			case  83: return     15927;
			case  84: return     14334;
			case  85: return     12901;
			case  86: return     11611;
			case  87: return     10450;
			case  88: return      9405;
			case  89: return      8464;

			case  90: return      7618;
			case  91: return      6856;
			case  92: return      6170;
			case  93: return      5553;
			case  94: return      4998;
			case  95: return      4498;
			case  96: return      4048;
			case  97: return      3644;
			case  98: return      3279;
			case  99: return      2951;

			case 100: return      2656;
			case 101: return      2391;
			case 102: return      2151;
			case 103: return      1936;
			case 104: return      1743;
			case 105: return      1568;
			case 106: return      1412;
			case 107: return      1270;
			case 108: return      1143;
			case 109: return      1029;

			default: return       1000; // infinite
		}
	}

	//
	virtual bool checkSequenceConsistency(const BlockHeader& block, bool& extended, bool lazy = false) {
		// prepare generator
		boost::random::mt19937 lGen(block.time_);
		// prepare distribution
		boost::random::uniform_int_distribution<> lDistribute(0, block.cycle_.size()-1);
		// get index
		int lNodeIndex = lDistribute(lGen);
		// locate node
		uint160 lId = *std::next(block.cycle_.begin(), lNodeIndex);
		
		// check
		uint160 lOriginId = block.origin_.id();
		bool lTargetCheck = (lId == lOriginId);
		//
		extended = true; // default
		
		// check signature
		uint256 lHash = const_cast<BlockHeader&>(block).hash();
		bool lSignatureCheck = const_cast<BlockHeader&>(block).origin().verify(lHash, block.signature_);

		// check available asset balance
		bool lProofAssetCheck = lazy || settings_->reindex() || settings_->resync();
		uint256 lProofAsset = settings_->proofAsset();
		if (!lazy && !lProofAsset.isNull()) {
			if (block.time_ >= settings_->proofFrom()) {
				if (!block.proofTx_.isNull()) {
					//
					std::vector<Transaction::NetworkUnlinkedOut> lUtxo;
					ITransactionStorePtr lStore = store_->storeManager()->locate(MainChain::id());
					lStore->selectUtxoByRawTransaction(block.proofTx_, lUtxo);

					if (lUtxo.size()) {
						//
						for (std::vector<Transaction::NetworkUnlinkedOut>::iterator lOut = lUtxo.begin(); lOut != lUtxo.end(); lOut++) {
							if (lOut->utxo().amount() >= settings_->proofAmount() && lOut->utxo().address().id() == lOriginId) {
								lProofAssetCheck = true;
								break;
							}
						}
					}
				}
			} else lProofAssetCheck = true;
		} else lProofAssetCheck = true;

		// check challenge
		bool lChecked = false;
		int lLevel = -1;
		uint256 lHashChallenge;
		uint256 lBottom = BlockHeader().hash();
		//
		if (!block.prev_.isNull() && block.prev_ != lBottom) {
			//
			BlockPtr lChallengeBlock = store_->block(block.prev_);
			if (lChallengeBlock != nullptr) {
				//
				if (/*lChallengeBlock->prev_ != lBottom &&*/ !lChallengeBlock->nextBlockChallenge().isNull()) {
					//
					BlockPtr lTargetBlock = store_->block(lChallengeBlock->nextBlockChallenge());
					if (lTargetBlock != nullptr /*&& !block.prevChallenge_.isNull()*/) {
						//
						if (lTargetBlock->transactions().size() > lChallengeBlock->nextTxChallenge() &&
							lChallengeBlock->nextTxChallenge() >= 0) {
							//
							TransactionPtr lTransaction = lTargetBlock->transactions()[lChallengeBlock->nextTxChallenge()];
							uint256 lTxId = lTransaction->hash();

							// make hash
							DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
							lSource << lChallengeBlock->nextBlockChallenge();
							lSource << lTxId;
							lSource << block.time_;
							
							lHashChallenge = Hash(lSource.begin(), lSource.end());
							extended = lHashChallenge == block.prevChallenge_; // proof-of-content
							lChecked = true;
							lLevel = 3;
						} else {
							extended = false; lLevel = 2;
						}
					} else {
						extended = false; lLevel = 1;
					}
				}
			} else {
				extended = false; lLevel = 0;
			}
		}

		if (gLog().isEnabled(Log::CONSENSUS) && (!(lTargetCheck && lSignatureCheck && lProofAssetCheck) || (!extended && lLevel > 0))) 
			gLog().write(Log::CONSENSUS, std::string("[checkSequenceConsistency/error]: sequence consistency result FAILED: ") +
				strprintf("target = %d, signature = %d, proof = %d, ext = %d / %s / %s - (c = %d, l = %d), block = %s, prevBlock = %s",
						lTargetCheck, lSignatureCheck, lProofAssetCheck,
							extended, lHashChallenge.toHex(), block.prevChallenge_.toHex(), lChecked, lLevel,
								lHash.toHex(), block.prev_.toHex()));

		return lTargetCheck && lSignatureCheck && lProofAssetCheck;
	}

	//
	// minimum network size
	// TODO: settings
	virtual uint32_t simpleNetworkSize() { return 4; }

	//
	// mini-tree for sync
	// TODO: settings
	virtual uint32_t partialTreeThreshold() { return 200; }

	//
	// use peer for network participation
	void pushPeer(IPeerPtr peer) {
		//
		if (peer->state()->containsChain(chain_)) {
			//
			peer_t lPeerId = peer->addressId();
			{
				boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
				directPeerMap_[lPeerId] = peer;
			}

			// push new state
			pushState(peer->state());
			// reset previous stats
			peer->resetStat();

			if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
				strprintf("[pushPeer]: peer pushed %s/%s/%s#", 
					lPeerId.toHex(), peer->key(), chain_.toHex().substr(0, 10)));
		}
	}

	//
	// remove peer from consensus participation
	void popPeer(IPeerPtr peer) {
		//
		if (peer->state()->containsChain(chain_)) {
			//
			peer_t lPeerId = peer->addressId();
			{
				boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
				directPeerMap_.erase(lPeerId);
			}

			popState(peer->state());

			if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
				strprintf("[popPeer]: peer popped %s/%s/%s#", 
					lPeerId.toHex(), peer->key(), chain_.toHex().substr(0, 10)));
		}
	}

	//
	//
	bool acquireBlock(const NetworkBlockHeader& block) {
		//
		IPeerPtr lPeer = nullptr;
		PeersMap lDirectPeerMap;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
			lDirectPeerMap = directPeerMap_;
		}

		// 1. search for connected peer with such block
		if (!lPeer) {
			//
			peer_t lPeerId;
			{
				boost::unique_lock<boost::recursive_mutex> lLock(stateMutex_);

				HeightMap::iterator lStateMap = heightMap_.find(const_cast<NetworkBlockHeader&>(block).height());
				if (lStateMap != heightMap_.end()) {
					//
					StateMap::iterator lPeerSet = lStateMap->second.find(const_cast<NetworkBlockHeader&>(block).blockHeader().hash());
					if(lPeerSet != lStateMap->second.end() && lPeerSet->second.size()) {
						//
						for (PeersSet::iterator lPeerIdx = lPeerSet->second.begin(); lPeerIdx != lPeerSet->second.end(); lPeerIdx++) {
 							lPeerId = *lPeerIdx;
							// select peer
							if (!lPeerId.isEmpty()) {
								PeersMap::iterator lPeerPtr = lDirectPeerMap.find(lPeerId);
								if (lPeerPtr != lDirectPeerMap.end() && lPeerPtr->second->state()->minerOrValidator() &&
									lPeerPtr->second->syncRequestsHeaders() < 5 &&
									lPeerPtr->second->syncRequestsBlocks() < 5) {
									lPeer = lPeerPtr->second;
									break;
								}
							}
 						}
					}
				}
			}
		}

		// 2. if absent - try to look at direct peers
		if (!lPeer) {
			PeersMap::iterator lPeerPtr = lDirectPeerMap.find(const_cast<NetworkBlockHeader&>(block).blockHeader().origin().id());
			if (lPeerPtr != lDirectPeerMap.end() && lPeerPtr->second->state()->minerOrValidator() &&
				lPeerPtr->second->syncRequestsHeaders() < 5 &&
				lPeerPtr->second->syncRequestsBlocks() < 5) {
				lPeer = lPeerPtr->second;
			}
		}

		if (lPeer) {
			if (store_->enqueueBlock(block)) {
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
					strprintf("[acquireBlock]: acquiring BLOCK %s/%s# from %s", 
						const_cast<NetworkBlockHeader&>(block).blockHeader().hash().toHex(), chain_.toHex().substr(0, 10), lPeer->key()));
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
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
			for (PeersMap::iterator lItem = directPeerMap_.begin(); lItem != directPeerMap_.end(); lItem++) {
				lPeers.push_back(lItem->second);
			}
		}

		// broadcast
		for (std::list<IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			if (except.isNull() || (except != (*lPeer)->addressId())) {
				if (!(*lPeer)->state()->daemon()) {
					(*lPeer)->broadcastBlockHeader(blockHeader);

					if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
						strprintf("[broadcastBlockHeader]: block header broadcasted to %s/%s/%s/%s#", 
							(*lPeer)->key(), (*lPeer)->statusString(), const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));
				}
			}
		}
	}

	//
	// broadcast block header and state
	void broadcastBlockHeaderAndState(const NetworkBlockHeader& block, StatePtr state, const uint160& except) {
		// prepare
		std::list<IPeerPtr> lPeers;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
			for (PeersMap::iterator lItem = directPeerMap_.begin(); lItem != directPeerMap_.end(); lItem++) {
				lPeers.push_back(lItem->second);
			}
		}

		//
		if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
			strprintf("[broadcastBlockHeaderAndState]: try broadcasting block header and state %d/%s/%s#", 
				lPeers.size(), const_cast<NetworkBlockHeader&>(block).blockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));

		// broadcast
		for (std::list<IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			if (except.isNull() || (except != (*lPeer)->addressId())) {
				if (!(*lPeer)->state()->daemon()) {
					//
					(*lPeer)->broadcastBlockHeaderAndState(block, state);
					//
					if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
						strprintf("[broadcastBlockHeaderAndState]: block header and state broadcasted to %s/%s/%s/%s#", 
							(*lPeer)->key(), (*lPeer)->statusString(), const_cast<NetworkBlockHeader&>(block).blockHeader().hash().toHex(), chain_.toHex().substr(0, 10)));
				}
			}
		}

		//
		// NOTICE: current state collected and sent to the clients only in case if main chain has changes - lower traffic without losing the information
		if (const_cast<NetworkBlockHeader&>(block).blockHeader().chain() == MainChain::id()) {
			consensusManager_->broadcastStateToClients(state);
		}
	}

	//
	// broadcast airdrop request
	void broadcastAirdropRequest(const PKey& key, const uint160& except) {
		// prepare
		std::list<IPeerPtr> lPeers;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
			for (PeersMap::iterator lItem = directPeerMap_.begin(); lItem != directPeerMap_.end(); lItem++) {
				lPeers.push_back(lItem->second);
			}
		}

		//
		if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
			strprintf("[broadcastAirdropRequest]: try broadcasting airdrop request %d/%s/%s#", 
				lPeers.size(), key.toString(), chain_.toHex().substr(0, 10)));

		// broadcast
		for (std::list<IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			if (except.isNull() || (except != (*lPeer)->addressId())) {
				(*lPeer)->tryAskForQbits(key);

			if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
				strprintf("[broadcastAirdropRequest]: airdrop request broadcasted to %s for %s/%s#", 
					(*lPeer)->addressId().toHex(), key.toString(), chain_.toHex().substr(0, 10)));
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
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
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
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
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
	// collect peers
	void collectPeers(std::map<uint160, IPeerPtr>& peers) {
		//
		// prepare
		std::list<IPeerPtr> lPeers;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
			for (PeersMap::iterator lItem = directPeerMap_.begin(); lItem != directPeerMap_.end(); lItem++) {
				//if (lItem->second->status() == IPeer::Status::ACTIVE)
				lPeers.push_back(lItem->second);
			}
		}

		for (std::list<IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			peers[(*lPeer)->addressId()] = (*lPeer);
		}
	}

	//
	// collect validators
	void collectValidators(std::map<uint160, IPeerPtr>& peers) {
		//
		std::list<IPeerPtr> lPeers;
		PeersMap lDirectPeerMap;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
			lDirectPeerMap = directPeerMap_;
		}

		{
			for (PeersMap::iterator lItem = lDirectPeerMap.begin(); lItem != lDirectPeerMap.end(); lItem++) {
				if (lItem->second->state()->minerOrValidator() /*&& lItem->second->status() == IPeer::Status::ACTIVE*/) {
					//
					bool lAdd = false;
					// check collateral if configured
					uint256 lProofAsset = settings_->proofAsset();
					if (!lProofAsset.isNull()) {
						//
						if (currentTime() >= settings_->proofFrom()) {
							//
							std::vector<Transaction::NetworkUnlinkedOut> lUtxo;
							ITransactionStorePtr lStore = store_->storeManager()->locate(MainChain::id());
							lStore->selectUtxoByRawAddressAndAsset(lItem->second->address(), lProofAsset, lUtxo, 5 /*top max*/);

							if (lUtxo.size()) {
								//
								for (std::vector<Transaction::NetworkUnlinkedOut>::iterator lOut = lUtxo.begin(); lOut != lUtxo.end(); lOut++) {
									if (lOut->utxo().amount() >= settings_->proofAmount()) {
										lAdd = true;
										break;
									}
								}
							}
						} else lAdd = true;
					} else lAdd = true;

					if (lAdd) {
						State::BlockInfo lInfo;
						if (lItem->second->state()->locateChain(chain_, lInfo)) {
							//
							BlockHeader lHeader;
							ITransactionStorePtr lStore = store_->storeManager()->locate(chain_);
							if (lStore) {
								uint64_t lHeight = lStore->currentHeight(lHeader);
								// +\- 100 from our height 
								if ((lHeight > lInfo.height() && lHeight - lInfo.height() < 100) ||
										(lHeight < lInfo.height() && lInfo.height() - lHeight < 100) || 
											lHeight == lInfo.height())
									lPeers.push_back(lItem->second);
							}
						}
					}
				}
			}
		}

		// broadcast
		for (std::list<IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			peers[(*lPeer)->addressId()] = (*lPeer);
		}		
	}

	//
	// push state
	bool pushState(StatePtr state) {
		// prepare
		State::BlockInfo lInfo;
		if (state->locateChain(chain_, lInfo)) {
			//		
			boost::unique_lock<boost::recursive_mutex> lLock(stateMutex_);

			// peer id
			peer_t lPeerId = state->addressId();
			std::map<peer_t, uint512>::iterator lPeerState = peerSet_.find(lPeerId);
			if (lPeerState != peerSet_.end() && lPeerState->second == state->signature()) // already updated
				return false;

			// clean-up
			if (heightMap_.size() > 50) {
				HeightMap::iterator lFirst = heightMap_.begin();
				for (StateMap::iterator lState = lFirst->second.begin(); lState != lFirst->second.end(); lState++)
					lState->second.clear();
				lFirst->second.clear();
				//
				heightMap_.erase(lFirst);
			}

			// height -> block = peer
			heightMap_[lInfo.height()][lInfo.hash()].insert(lPeerId);

			// all peers
			peerSet_[lPeerId] = state->signature();

			gLog().write(Log::CONSENSUS, 
				strprintf("[pushState]: state pushed %d/%s/%s/%s#", lInfo.height(), lInfo.hash().toHex(), lPeerId.toHex(), chain_.toHex().substr(0, 10)));
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
			boost::unique_lock<boost::recursive_mutex> lLock(stateMutex_);

			// peer id
			peer_t lPeerId = state->addressId();

			// clean-up
			peerSet_.erase(lPeerId);
		}

		return true;
	}

	//
	// network is simple?  
	bool isSimpleNetwork() {
		boost::unique_lock<boost::recursive_mutex> lLock(stateMutex_);
		return peerSet_.size() < simpleNetworkSize();
	}

	//
	// check if chain synchronized
	bool isChainSynchronized() {
		//
		BlockHeader lHeader;
		uint64_t lHeight = store_->currentHeight(lHeader);

		bool lResult = false;
		if (lHeight) {
			//
			boost::unique_lock<boost::recursive_mutex> lLock(stateMutex_);
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
	//
	uint64_t locateSynchronizedRoot(std::multimap<uint32_t, IPeerPtr>& peers, uint256& block, uint256& last) {
		//
		uint64_t lResultHeight = 0;
		//
		PeersSet lPeers;
		BlockHeader lHeader;
		uint64_t lHeight = store_->currentHeight(lHeader);
		{
			boost::unique_lock<boost::recursive_mutex> lLock(stateMutex_);
			//
			last = lHeader.hash();
			// reverse traversal of heights of the chain
			bool lFound = false;
			for (HeightMap::reverse_iterator lHeightPos = heightMap_.rbegin(); !lFound && lHeightPos != heightMap_.rend(); lHeightPos++) {
				// check 
				if (!lHeightPos->second.size()) { lFound = true; continue; }
				// try
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
					strprintf("[locateSynchronizedRoot]: try [%d:%d]/%d/%s#", 
						lHeightPos->first, lHeight, lHeightPos->second.size(), chain_.toHex().substr(0, 10)));
				for (StateMap::iterator lState = lHeightPos->second.begin(); lState != lHeightPos->second.end(); lState++) {
					if (lHeightPos->first > lHeight) {
						// collect known peers for the synchronized root
						lPeers.insert(lState->second.begin(), lState->second.end());
						// height
						lResultHeight = lHeightPos->first;
						// block
						block = lState->first;

						if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
							strprintf("[locateSynchronizedRoot]: ROOT candidate [%d:%d:%d]/%s/%s#", 
								lResultHeight, lHeight, lPeers.size(), block.toHex(), chain_.toHex().substr(0, 10)));
						break;
					} else if (lHeightPos->first == lHeight) {
						if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, 
							strprintf("[locateSynchronizedRoot]: already SYNCHRONIZED %d/%s/%s#", 
								lHeightPos->first, lState->first.toHex(), chain_.toHex().substr(0, 10)));

						lFound = true;
						break;
					}
				}

				// check
				for (PeersSet::iterator lPeer = lPeers.begin(); !lFound && lPeer != lPeers.end(); lPeer++) {
					PeersMap::iterator lPeerPtr = directPeerMap_.find(*lPeer);
					if (lPeerPtr != directPeerMap_.end()) {
						if (lPeerPtr->second->state()->minerOrValidator() &&
							lPeerPtr->second->syncRequestsHeaders() < 5 &&
							lPeerPtr->second->syncRequestsBlocks() < 5) {
							lFound = true;
							break;
						}
					}
				}

				//
				if (!lFound) lPeers.clear();
			}
		}

		{
			boost::unique_lock<boost::recursive_mutex> lLock(peersMutex_);
			//
			for (PeersSet::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
				PeersMap::iterator lPeerPtr = directPeerMap_.find(*lPeer);
				if (lPeerPtr != directPeerMap_.end()) {
					if (!lPeerPtr->second->syncRequestsHeaders() && !lPeerPtr->second->syncRequestsBlocks()) {
						if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS,
							strprintf("[locateSynchronizedRoot]: try to add peer %s/%s/%s#", 
								lPeerPtr->second->key(), lPeerPtr->second->statusString(),
								chain_.toHex().substr(0, 10)));
					} else {
						if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS,
							strprintf("[locateSynchronizedRoot]: try to add peer %s/%s/%s#, h = %d, b = %d", 
								lPeerPtr->second->key(), lPeerPtr->second->statusString(),
								chain_.toHex().substr(0, 10),
								lPeerPtr->second->syncRequestsHeaders(), lPeerPtr->second->syncRequestsBlocks()));
					}

					if (lPeerPtr->second->state()->minerOrValidator() &&
						lPeerPtr->second->syncRequestsHeaders() < 5 &&
						lPeerPtr->second->syncRequestsBlocks() < 5) {
						if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS,
							strprintf("[locateSynchronizedRoot]: peer added %s/%s/%s#", 
								lPeerPtr->second->key(), lPeerPtr->second->statusString(),
								chain_.toHex().substr(0, 10)));
						peers.insert(std::multimap<uint32_t, IPeerPtr>::value_type(lPeerPtr->second->latency(), lPeerPtr->second));
					}
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
				boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
				if (lIsSynchronized) chainState_ = IConsensus::SYNCHRONIZED;
				else chainState_ = IConsensus::NOT_SYNCHRONIZED;
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
	// expand sync job
	void expandJob(SynchronizationJobPtr job) {
		//
		if (chainState_ == IConsensus::SYNCHRONIZING) {
			//
			std::multimap<uint32_t, IPeerPtr> lPeers;
			uint256 lBlock, lLast;
			locateSynchronizedRoot(lPeers, lBlock, lLast); // get peers, height and block

			// add more peers to enforce sync job
			if (lPeers.size()) {
				if (settings_->isFullNode() || settings_->isNode()) {
					for(std::multimap<uint32_t, IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
						if (!lPeer->second->jobExists(chain_) && 
								(job->type() == SynchronizationJob::FULL || 
									job->type() == SynchronizationJob::LARGE_PARTIAL)) {
							gLog().write(Log::CONSENSUS, strprintf("[expandJob]: starting block feed for %s# from %s/%d", chain_.toHex().substr(0, 10), lPeer->second->key(), lPeers.size()));
							lPeer->second->synchronizePendingBlocks(shared_from_this(), job); // last job
						}
					}
				}
			}
		}
	}

	//
	// finish sync job
	void finishJob(SynchronizationJobPtr job) {
		//
		std::multimap<uint32_t, IPeerPtr> lPeers;
		uint256 lBlock, lLast;
		uint64_t lHeight = locateSynchronizedRoot(lPeers, lBlock, lLast); // get peers, height and block

		// force
		if (job == nullptr && job_) {
			// TODO: check if we on GET_BLOCK_HEADER stage
			if (job_->currentBlock() != uint256()) {
				// job stalled, try to restart
				if (lPeers.size()) {
					// 1. clean-up
					if (settings_->isFullNode() || settings_->isNode()) {
						for(std::multimap<uint32_t, IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
								gLog().write(Log::CONSENSUS, strprintf("[finishJob]: cleaning up for %s# from %s/%d", chain_.toHex().substr(0, 10), lPeer->second->key(), lPeers.size()));
								lPeer->second->removeJob(chain_); // last job
						}
					}

					// 2. push
					std::multimap<uint32_t, IPeerPtr>::iterator lPeer = lPeers.begin();
					if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[finishJob]: continue LARGE PARTIAL tree synchronization ") + 
						strprintf("%d/%s-%s/%s# for %s", lHeight, lBlock.toHex(), lLast.toHex(), chain_.toHex().substr(0, 10), lPeer->second->key()));

					if (job_->cancelled()) {
						job_->renew(); // in case if job was cancelled: current block is absent
						job_->setNextBlock(lBlock); // set latest new
					} else {
						job_->setNextBlock(job_->currentBlock()); // reset to last one
					}

					lPeer->second->synchronizeLargePartialTree(shared_from_this(), job_);

					// 3. jump out
					return;
				}
			} else {
				job_->cancel();
			}
		}

		// add more peers to enforce sync job
		if (lPeers.size()) {
			if (settings_->isFullNode() || settings_->isNode()) {
				for(std::multimap<uint32_t, IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
						gLog().write(Log::CONSENSUS, strprintf("[finishJob]: cleaning up for %s# from %s/%d", chain_.toHex().substr(0, 10), lPeer->second->key(), lPeers.size()));
						lPeer->second->removeJob(chain_); // last job
				}
			}

			{
				gLog().write(Log::CONSENSUS, strprintf("[finishJob]: reset synchronization job for %s#", chain_.toHex().substr(0, 10)));
				job_ = nullptr;
			}
		}

		// force
		if (job == nullptr) {
			job_ = nullptr; // reset
			gLog().write(Log::CONSENSUS, strprintf("[finishJob]: forcing synchronization for %s#", chain_.toHex().substr(0, 10)));
			{
				boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
				chainState_ = IConsensus::NOT_SYNCHRONIZED;
			}
		}
	}

	//
	// begin synchronization
	bool doSynchronize(bool resync = false) {
		//
		bool lProcess = false; 
		{
			boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
			if (((chainState_ == IConsensus::UNDEFINED || chainState_ == IConsensus::NOT_SYNCHRONIZED) && 
					!store_->synchronizing()) || 
						resync) {
				chainState_ = IConsensus::SYNCHRONIZING;
				lProcess = true;
				if (resync) resync_ = true;
			}
		}

		if (lProcess) {
			//
			std::multimap<uint32_t, IPeerPtr> lPeers;
			uint256 lBlock, lLast;
			uint64_t lHeight = locateSynchronizedRoot(lPeers, lBlock, lLast); // get peers, height and block
			if (lHeight && lPeers.size()) {
				if (settings_->isFullNode() || settings_->isNode()) {
					// sanity
					/*
					for (std::list<IPeerPtr>::iterator lCandidate = lPeers.begin(); lCandidate != lPeers.end(); lCandidate++) {
						if ((*lCandidate)->jobExists(chain_)) {
							SynchronizationJobPtr lJob = (*lCandidate)->locateJob(chain_);
							// clean-up
							if (!lJob->pendingBlocks() && !lJob->activeWorkers() && ) {
								if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[doSynchronize]: synchronization job is EMPTY ") + 
									strprintf("%d/%s/%s# - (blocks = %d/peers = %d)", lHeight, lBlock.toHex(), 
										chain_.toHex().substr(0, 10), lJob->pendingBlocks(), lJob->activeWorkers()));
								(*lCandidate)->removeJob(chain_);
								continue;
							}

							if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[doSynchronize]: synchronization is already RUNNING ") + 
								strprintf("%d/%s/%s# - (blocks = %d/peers = %d)", lHeight, lBlock.toHex(), 
									chain_.toHex().substr(0, 10), lJob->pendingBlocks(), lJob->activeWorkers()));
							return false;
						}
					}
					*/

					// store is empty - full sync
					BlockHeader lHeader;
					uint64_t lOurHeight = store_->currentHeight(lHeader);

					if (resync_) {
						//
						std::multimap<uint32_t, IPeerPtr>::iterator lPeer = lPeers.begin(); // just first?
						//
						if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[doSynchronize]: starting FULL RESYNC for ") + 
							strprintf("%d/%s-%s/%s#", lHeight, lBlock.toHex(), lLast.toHex(), chain_.toHex().substr(0, 10)));

						job_ = SynchronizationJob::instance(lBlock, BlockHeader().hash(), 1000000000000, 0, SynchronizationJob::LARGE_PARTIAL);
						job_->setResync(); // resync!
						lPeer->second->synchronizeLargePartialTree(shared_from_this(), job_);
						resync_ = false;
					} else {
						if (!lOurHeight) {
							//
							std::multimap<uint32_t, IPeerPtr>::iterator lPeer = lPeers.begin(); // this node -> get current block thread
							if (!job_ || job_->nextBlockInstant().isNull()) { 
								//
								if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[doSynchronize]: starting FULL synchronization ") + 
									strprintf("%d/%s/%s#", lHeight, lBlock.toHex(), chain_.toHex().substr(0, 10)));

								job_ = SynchronizationJob::instance(lBlock, BlockHeader().hash(), lHeight, 0, SynchronizationJob::FULL); // block from
								lPeer->second->synchronizeLargePartialTree(shared_from_this(), job_);
							}
						} else if (lHeight > lOurHeight && lHeight - lOurHeight < partialTreeThreshold()) {
							//
							/*
							BlockHeader lConfirmed;
							if (store_->blockHeader(lOurHeight - coinbaseMaturity(), lConfirmed)) {
								lLast = lConfirmed.hash();
								lOurHeight -= coinbaseMaturity();
							}
							*/
							//
							std::multimap<uint32_t, IPeerPtr>::iterator lPeer = lPeers.begin(); // just first?
							if (!job_ || job_->nextBlockInstant().isNull()) { 
								if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[doSynchronize]: starting PARTIAL tree synchronization ") + 
									strprintf("%d/%s-%s/%s#", lHeight, lBlock.toHex(), lLast.toHex(), chain_.toHex().substr(0, 10)));
								job_ = SynchronizationJob::instance(lBlock, lLast, lHeight - lOurHeight, lOurHeight, SynchronizationJob::PARTIAL); // block from
								lPeer->second->synchronizePartialTree(shared_from_this(), job_);
							}
						} else {
							std::multimap<uint32_t, IPeerPtr>::iterator lPeer = lPeers.begin(); // just first?
							if (!job_ || job_->nextBlockInstant().isNull()) { 
								//
								bool lVeryLast = (lLast == BlockHeader().hash());
								BlockHeader lConfirmed;
								if (store_->blockHeader(lOurHeight - coinbaseMaturity(), lConfirmed)) {
									lLast = lConfirmed.hash();
									lOurHeight -= coinbaseMaturity();
									lVeryLast = (lLast == BlockHeader().hash());
								}
								//
								if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[doSynchronize]: starting LARGE PARTIAL tree synchronization ") + 
									strprintf("%d:%d/%s-%s/%s#", lHeight, lOurHeight, lBlock.toHex(), 
										(lVeryLast ? "LAST" : lLast.toHex()), chain_.toHex().substr(0, 10)));

								job_ = SynchronizationJob::instance(
									lBlock,
									lLast,
									lHeight - lOurHeight,
									lOurHeight,
									SynchronizationJob::LARGE_PARTIAL); // block from
								lPeer->second->synchronizeLargePartialTree(shared_from_this(), job_);
							}
						}
					}	
				} else {
					if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, "[doSynchronize]: synchronization is allowed for NODE or FULLNODE only.");
					//
					boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
					chainState_ = IConsensus::SYNCHRONIZED;

					return true;
				}			
			} else {
				// there is no peers
				boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
				chainState_ = IConsensus::SYNCHRONIZED;

				return true;
			}
		}

		return false;
	}

	//
	// process downloaded partial tree blocks
	bool processPartialTreeHeaders(SynchronizationJobPtr job) {
		//
		bool lProcess = false; 
		{
			boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
			if (chainState_ == IConsensus::SYNCHRONIZING && !job->cancelled() /*|| chainState_ == IConsensus::NOT_SYNCHRONIZED*/) {
				lProcess = true;
			}
		}

		if (lProcess) {
			std::multimap<uint32_t, IPeerPtr> lPeers;
			uint256 lBlock, lLast;
			locateSynchronizedRoot(lPeers, lBlock, lLast); // get peers, height and block
			if (lPeers.size()) {
				// adjust job parameters
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, strprintf("[processPartialTreeHeaders]: adjusting last block for %s#", chain_.toHex().substr(0, 10)));
				uint256 lActualLastBlock = job->analyzeLinkedChunks();
				if (lActualLastBlock != uint256()) {
					//
					if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, strprintf("[processPartialTreeHeaders]: last block adjusted to %s/%s#", lActualLastBlock.toHex(), chain_.toHex().substr(0, 10)));
					job->setLastBlock(lActualLastBlock);
				}

				if (settings_->isFullNode() || settings_->isNode()) {
					for(std::multimap<uint32_t, IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
						if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, strprintf("[processPartialTreeHeaders]: starting block feed for %s# from %s/%d", chain_.toHex().substr(0, 10), lPeer->second->key(), lPeers.size()));
						lPeer->second->synchronizePendingBlocks(shared_from_this(), job);
					}

					return true;
				} else {
					if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, "[processPartialTreeHeaders]: synchronization is allowed for NODE or FULLNODE only.");
					//
					boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
					chainState_ = IConsensus::SYNCHRONIZED;

					return true;
				}			
			} else {
				// there is no peers
				boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
				chainState_ = IConsensus::NOT_SYNCHRONIZED;

				return false;
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
			boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
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
				boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
				if (lIsSynchronized) chainState_ = IConsensus::SYNCHRONIZED;
				else chainState_ = IConsensus::NOT_SYNCHRONIZED;
			}
		}
	}

	//
	// do full reindex
	void doReindex() {
		//
		bool lProcess = false; 
		{
			boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
			chainState_ = IConsensus::INDEXING;
		}

		//
		BlockHeader lHeader;
		if (store_->currentHeight(lHeader)) {
			// full re-index
			store_->reindexFull(lHeader.hash(), consensusManager_->mempoolManager()->locate(chain_));
		}

		{
			boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
			chainState_ = IConsensus::NOT_SYNCHRONIZED;
		}
	}

	//
	// to indexing
	bool doIndex(const uint256& block, const uint256& lastBlock) {
		//
		bool lProcess = false; 
		{
			boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
			if (chainState_ == IConsensus::SYNCHRONIZING) {
				chainState_ = IConsensus::INDEXING;
				lProcess = true;
			}
		}

		if (lProcess) {
			// partial re-index
			if (!store_->reindex(block, lastBlock, consensusManager_->mempoolManager()->locate(chain_))) {
				// partial reindex for given blocks intervale is not possible
				{
					boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
					chainState_ = IConsensus::NOT_SYNCHRONIZED;
				}

				return false;
			}

			// check
			bool lIsSynchronized = isChainSynchronized();
			{
				boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
				if (lIsSynchronized) chainState_ = IConsensus::SYNCHRONIZED;
				else chainState_ = IConsensus::NOT_SYNCHRONIZED;
			}
		}

		return true;
	}

	//
	// to non-sync
	void toNonSynchronized(bool force = false) {
		//
		{
			boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
			// NOTICE: if we already synchronizing - do not fallback to non_synchronized
			if (/*chainState_ == IConsensus::SYNCHRONIZING ||*/ chainState_ == IConsensus::SYNCHRONIZED || chainState_ == IConsensus::UNDEFINED/*|| chainState_ == IConsensus::INDEXING*/ || force) {
				chainState_ = IConsensus::NOT_SYNCHRONIZED;
			}
		}
	}

	//
	// to sync'ing
	void toSynchronizing() {
		{
			boost::unique_lock<boost::recursive_mutex> lLock(transitionMutex_);
			if (chainState_ == IConsensus::NOT_SYNCHRONIZED || chainState_ == IConsensus::SYNCHRONIZED) {
				chainState_ = IConsensus::SYNCHRONIZING;
			}
		}
	}

	//
	// main key
	SKeyPtr mainKey() {
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
			case IConsensus::NOT_SYNCHRONIZED: return "NOT_SYNCHRONIZED";
			case IConsensus::SYNCHRONIZING: return "SYNCHRONIZING";
			case IConsensus::INDEXING: return "INDEXING";
			case IConsensus::SYNCHRONIZED: return "SYNCHRONIZED";
		}

		return "ESTATE";
	}	

protected:
	std::string dApp_;
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
	typedef uint64_t height_t;

	//
	typedef std::map<peer_t, IPeerPtr> PeersMap;

	//
	// chain -> height -> block_id -> peer
	typedef std::set<peer_t> PeersSet;
	typedef std::map<block_t, PeersSet> StateMap;
	typedef std::map<height_t, StateMap> HeightMap;

	//
	// chain -> time -> peer
	typedef std::map<time_t, peer_t> BlockTimePeerMap;

	boost::recursive_mutex peersMutex_;
	boost::recursive_mutex stateMutex_;
	boost::recursive_mutex queueMutex_;

	boost::recursive_mutex transitionMutex_;

	PeersMap directPeerMap_; // peer_id -> IPeer
	std::map<peer_t, uint512> peerSet_;

	BlockTimePeerMap blockTimePeer_;

	HeightMap heightMap_;
	bool resync_ = false;
};

} // qbit

#endif
