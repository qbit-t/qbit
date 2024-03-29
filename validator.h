// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_VALIDATOR_H
#define QBIT_VALIDATOR_H

#include "exception.h"
#include "iconsensus.h"
#include "isettings.h"
#include "imemorypool.h"
#include "ivalidator.h"
#include "log/log.h"
#include "entity.h"
#include "block.h"
#include "blockcontext.h"
#include "hash/cuckoo.h"
#include "pow.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/chrono.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace qbit {

class MainValidator: public IValidator, public std::enable_shared_from_this<MainValidator> {
public:
	MainValidator(const uint256& chain, IConsensusPtr consensus, IMemoryPoolPtr mempool, ITransactionStorePtr store) : 
		chain_(chain), consensus_(consensus), mempool_(mempool), store_(store) {}

	uint256 chain() { return chain_; }

	void run() {
		if (!controller_) {
			timer_ = TimerPtr(new boost::asio::steady_timer(context_, boost::asio::chrono::seconds(1)));
			timer_->async_wait(boost::bind(&MainValidator::touch, shared_from_this(), boost::asio::placeholders::error));			

			controller_ = boost::shared_ptr<boost::thread>(
						new boost::thread(
							boost::bind(&MainValidator::controller, shared_from_this())));
		}
	}

	void stop() {
		gLog().write(Log::INFO, std::string("[validator/stop]: stopping..."));

		context_.stop();
		controller_->join();
		stopMiner();

		consensus_.reset();
		mempool_.reset();
		store_.reset();
	}

	static IValidatorPtr instance(const uint256& chain, IConsensusPtr consensus, IMemoryPoolPtr mempool, ITransactionStorePtr store) {
		return std::make_shared<MainValidator>(chain, consensus, mempool, store);
	}

	IValidator::BlockCheckResult checkBlockHeader(const NetworkBlockHeader& blockHeader) {
		//
		BlockHeader lHeader;
		uint64_t lCurrentHeight = store_->currentHeight(lHeader);
		BlockHeader& lOther = const_cast<NetworkBlockHeader&>(blockHeader).blockHeader();

		//
		if (const_cast<NetworkBlockHeader&>(blockHeader).height() < lCurrentHeight) {
			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader/error]: target height is less that current ") + 
				strprintf("current = %d/%d, proposed = %d/%d, new = %s, our = %s, origin = %s/%s#", 
					lHeader.time(), lCurrentHeight, lOther.time(),
					const_cast<NetworkBlockHeader&>(blockHeader).height(),
					lOther.hash().toHex(), lHeader.hash().toHex(), 
						lOther.origin().toHex(), chain_.toHex().substr(0, 10)));
			return IValidator::INTEGRITY_IS_INVALID;
		}

		//
		bool lExtended = true;
		if (!consensus_->checkSequenceConsistency(lOther, lExtended)) {
			//
			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader/error]: check sequence consistency FAILED ") +
				strprintf("block = %s, prev = %s, chain = %s#", lOther.hash().toHex(), 
					lOther.prev().toHex(), chain_.toHex().substr(0, 10)));
			return IValidator::INTEGRITY_IS_INVALID;
		} else if (!lExtended /*&& lOther.height() > lCurrentHeight*/) {
			//
			std::map<uint160, IPeerPtr> lPeers;
			consensus_->collectPeers(lPeers);
			lPeers[consensus_->mainKey()->createPKey().id()] = nullptr; // just stub
			//
			std::vector<uint160> lCycle;
			for (std::map<uint160, IPeerPtr>::iterator lNode = lPeers.begin(); lNode != lPeers.end(); lNode++) {
				//
				lCycle.push_back(lNode->first);
			}

			std::vector<uint160> lResult;
			std::set_intersection(lCycle.begin(), lCycle.end(), lOther.cycle_.begin(), lOther.cycle_.end(), std::back_inserter(lResult));

			if (lResult.size()) {
				//
				if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader/error]: PoC FAILED, broken chain for ") + 
					strprintf("%d/%s/%s#", const_cast<NetworkBlockHeader&>(blockHeader).height(), 
						lOther.hash().toHex(), chain_.toHex().substr(0, 10)));
				consensus_->toNonSynchronized();
				return IValidator::BROKEN_CHAIN;
			}

			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader/error]: peers proof FAILED ") +
				strprintf("block = %s, prev = %s, chain = %s#", lOther.hash().toHex(), 
					lOther.prev().toHex(), chain_.toHex().substr(0, 10)));

			return IValidator::INTEGRITY_IS_INVALID;
		}

		/*
		if (lOther.time() < lHeader.time() && const_cast<NetworkBlockHeader&>(blockHeader).height() > lCurrentHeight) {
			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader]: proposed block time is less than median time ") + 
				strprintf("current = %d, proposed = %d, height = %d, new = %s, our = %s, origin = %s/%s#", 
					lHeader.time(), lOther.time(),
					const_cast<NetworkBlockHeader&>(blockHeader).height(),
					lOther.hash().toHex(), lHeader.hash().toHex(), 
						lOther.origin().toHex(), chain_.toHex().substr(0, 10)));
			return IValidator::INTEGRITY_IS_INVALID;
		}
		*/

		if (lOther.prev() == lHeader.hash()) {
			//
			if (lOther.time() < lHeader.time() && lHeader.time() - lOther.time() > (consensus_->blockTime() / 1000)) {
				if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader/error]: next block time is less than median time ") + 
					strprintf("current = %d, proposed = %d, height = %d, new = %s, our = %s, origin = %s/%s#",
						lHeader.time(), lOther.time(),
						const_cast<NetworkBlockHeader&>(blockHeader).height(), 
						lOther.hash().toHex(), lHeader.hash().toHex(), 
							lOther.origin().toHex(), chain_.toHex().substr(0, 10)));
				return IValidator::INTEGRITY_IS_INVALID;
			} else if (lHeader.time() > lOther.time() && !(lOther.time() - lHeader.time() >= (consensus_->blockTime() / 1000) &&
						lOther.time() - lHeader.time() <= (consensus_->blockTime() / 1000) * 2)) {
				if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader/error]: time passage is wrong ") + 
					strprintf("current = %d, proposed = %d, height = %d, new = %s, our = %s, origin = %s/%s#",
						lHeader.time(), lOther.time(),
						const_cast<NetworkBlockHeader&>(blockHeader).height(), 
						lOther.hash().toHex(), lHeader.hash().toHex(), 
							lOther.origin().toHex(), chain_.toHex().substr(0, 10)));
				return IValidator::INTEGRITY_IS_INVALID;
			}

			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader]: proposed header is correct ") + 
				strprintf("height = %d, new = %s, our = %s, origin = %s/%s#", const_cast<NetworkBlockHeader&>(blockHeader).height(), 
					lOther.hash().toHex(), lHeader.hash().toHex(), lOther.origin().toHex(), chain_.toHex().substr(0, 10)));

			stopMiner();
			return IValidator::SUCCESS;
		}

		if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader/error]: broken chain for ") + 
			strprintf("%d/%s/%s#", const_cast<NetworkBlockHeader&>(blockHeader).height(), 
				lOther.hash().toHex(), chain_.toHex().substr(0, 10)));
		
		consensus_->toNonSynchronized();
		return IValidator::BROKEN_CHAIN;
	}

	bool acceptBlockHeader(const NetworkBlockHeader& blockHeader) {
		//
		IConsensus::ChainState lState = consensus_->chainState();
		if (lState == IConsensus::SYNCHRONIZED) {
			if (!consensus_->acquireBlock(blockHeader)) {
				// 2.1 peer was not found - reset state
				gLog().write(Log::VALIDATOR, std::string("[acceptBlockHeader]: peer for network block ") + 
					strprintf("%s", const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().hash().toHex()) + std::string(" was not found. Begin synchronization..."));
				consensus_->toNonSynchronized();

				return false;
			}
		}

		return true;
	}

	bool reindexed() { return reindexed_; }

private:
	void stopMiner() {
		// stop miner
		if (consensus_ && consensus_->settings()->isMiner() && minerRunning_) {
			gLog().write(Log::VALIDATOR, std::string("[miner]: stopping for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
			minerRunning_ = false;
			minerActive_.notify_one();
		}
	}

	void startMiner() {
		// start miner
		if (!minerRunning_ && !store_->synchronizing() && mempool_->wallet()->isOpened()) {
			BlockHeader lHeader = store_->currentBlockHeader();
			if (!miner_) miner_ = boost::shared_ptr<boost::thread>(
						new boost::thread(
							boost::bind(&MainValidator::miner, shared_from_this())));
			minerRunning_ = true;
			minerActive_.notify_one();
		}
	}

	void controller() {
		// log
		gLog().write(Log::INFO, std::string("[validator]: starting for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));

		try {
			context_.run();
		} 
		catch(qbit::exception& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[validator]: qbit error -> ") + ex.what());
		}
		catch(boost::system::system_error& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[validator]: context error -> ") + ex.what());
		}
		catch(std::runtime_error& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[validator]: runtime error -> ") + ex.what());
		}

		// log
		gLog().write(Log::INFO, std::string("[validator]: stopping for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
	}

	//
	// miner thread
	void miner() {
		//
		bool lTimeout = false;
		uint64_t lWaitTo = 0;
		//
		while(true) {
			boost::mutex::scoped_lock lLock(minerMutex_);
			while(!minerRunning_) minerActive_.wait(lLock);

			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[miner]: starting for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));

			// check and run
			while(minerRunning_) {
				// get last header
				BlockHeader lLastHeader = store_->currentBlockHeader();
				// get seed
				uint64_t lCurrentTime = consensus_->currentTime();

				// !miner -> control and track clean-ups
				if (!consensus_->settings()->isMiner()) {
					//
					if (lCurrentTime > lWaitTo) {
						if (lWaitTo) {
							//
							if (gLog().isEnabled(Log::VALIDATOR))
								gLog().write(Log::VALIDATOR, std::string("[miner]: cleaning-up ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
							// get block template
							BlockPtr lCurrentBlock = Block::instance();
							// make pseudo-block
							mempool_->beginBlock(lCurrentBlock);
						}

						// calc next run
						lWaitTo = lCurrentTime + ((consensus_->blockTime())/1000) * 10; // next +10 blocks
					}

					// switch-out
					boost::this_thread::sleep_for(boost::chrono::milliseconds(100)); // TODO: not good, but it's ok for this case
					//
					continue;
				}

				// check if time passage was take a place
				if ((lTimeout && lCurrentTime >= lWaitTo) || 
						(!lTimeout && lCurrentTime > lLastHeader.time() &&
							lCurrentTime - lLastHeader.time() >= (consensus_->blockTime())/1000)) {
					//
					try {
						//
						if (lTimeout && gLog().isEnabled(Log::VALIDATOR))
							gLog().write(Log::VALIDATOR, std::string("[miner]: timedout for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));

						lTimeout = false;

						// select next leader
						std::map<uint160, IPeerPtr> lPeers;
						consensus_->collectValidators(lPeers);

						// add self
						uint160 lSelfId = consensus_->mainKey()->createPKey().id();
						lPeers[lSelfId] = nullptr; // just stub

						// prepare generator
						boost::random::mt19937 lGen(lCurrentTime);
						// prepare distribution
						boost::random::uniform_int_distribution<> lDistribute(0, lPeers.size()-1);
						// get NEXT index
						int lNodeIndex = lDistribute(lGen);

						//
						uint160 lNextId = std::next(lPeers.begin(), lNodeIndex)->first;

						// check
						if (lNextId == lSelfId) {
							// get block template
							BlockPtr lCurrentBlock = Block::instance();

							// prepare block
							BlockContextPtr lCurrentBlockContext = mempool_->beginBlock(lCurrentBlock);

							// make coinbase tx
							BlockHeader lHeader;
							uint64_t lCurrentHeight = store_->currentHeight(lHeader);
							TransactionContextPtr lCoinbase = mempool_->wallet()->createTxCoinBase(
								consensus_->blockReward(lCurrentHeight + 1 /*next height*/) + 
									lCurrentBlockContext->fee());
							lCurrentBlock->prepend(lCoinbase->tx());

							// set effective time (need for integrity check)
							lCurrentBlock->setTime(lCurrentTime);

							// reset current block linkage
							lCurrentBlock->setPrev(lLastHeader.hash());

							// fill-up nodes with order (need for integrity check)
							for (std::map<uint160, IPeerPtr>::iterator lNode = lPeers.begin(); lNode != lPeers.end(); lNode++) {
								//
								lCurrentBlock->cycle_.push_back(lNode->first);
							}

							// prepare next challenge
							BlockHeader lCurrentBlockHeader;
							int32_t lChallengeBlockTx = -1;
							uint256 lNextBlockChallenge;
							uint64_t lChallengeBlock = 0, lCurrentBlockChallenge = 0;
							bool lChallengeSaved = false;
							// ONLY approx value to measure height
							uint64_t lCurrentBlockHeight = store_->currentHeight(lCurrentBlockHeader);
							if (lCurrentBlockHeight > 100) {
								// define dig deep
								boost::random::uniform_int_distribution<uint64_t> lBlockDistribution(1, 100);
								lChallengeBlock = lBlockDistribution(lGen);

								BlockPtr lBlock;
								uint256 lPrevBlock = lLastHeader.prev();
								while ((lBlock = store_->block(lPrevBlock)) != nullptr &&
														lCurrentBlockChallenge < lChallengeBlock) {
									lPrevBlock = lBlock->prev();
									lCurrentBlockChallenge++;
								}

								if (lBlock && lCurrentBlockChallenge == lChallengeBlock) {
									lNextBlockChallenge = lBlock->hash();
									if (lBlock->transactions().size()) {
										//
										boost::random::uniform_int_distribution<> lTxDistribution(0, lBlock->transactions().size()-1);
										lChallengeBlockTx = lTxDistribution(lGen);

										// finally - set
										lCurrentBlock->setChallenge(lNextBlockChallenge, lChallengeBlockTx);
										lChallengeSaved = true;
									}
								}
							}

							// resolve previous challenge
							bool lPrevChallengeResolved = false;
							if (!lLastHeader.nextBlockChallenge().isNull()) {
								//
								BlockPtr lTargetBlock = store_->block(lLastHeader.nextBlockChallenge());
								if (lTargetBlock != nullptr) {
									//
									if (lTargetBlock->transactions().size() > lLastHeader.nextTxChallenge() &&
										lLastHeader.nextTxChallenge() >= 0) {
										//
										TransactionPtr lTransaction = lTargetBlock->transactions()[lLastHeader.nextTxChallenge()];
										uint256 lTxId = lTransaction->hash();

										// make hash
										DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
										lSource << lLastHeader.nextBlockChallenge();
										lSource << lTxId;
										lSource << lCurrentTime;
										
										uint256 lHashChallenge = Hash(lSource.begin(), lSource.end());
										lCurrentBlock->setPrevChallenge(lHashChallenge);
										lPrevChallengeResolved = true;
									}
								}
							} else lPrevChallengeResolved = true;

							if (!lPrevChallengeResolved) {
								// timeout
								if (!lTimeout) {
									lTimeout = true;
									lWaitTo = lCurrentTime + (consensus_->blockTime())/1000;
									if (gLog().isEnabled(Log::VALIDATOR))
										gLog().write(Log::VALIDATOR, std::string("[miner]: going for timeout ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
								}

								continue;
							}

							// prepare proof
							uint256 lProofTx;
							uint256 lProofAsset = consensus_->settings()->proofAsset();
							std::vector<Transaction::NetworkUnlinkedOut> lFreeOuts;
							if (!lProofAsset.isNull()) {
								ITransactionStorePtr lStore = store_->storeManager()->locate(MainChain::id());
								lStore->selectUtxoByAddressAndAsset(consensus_->mainKey()->createPKey(), lProofAsset, lFreeOuts);

								if (lFreeOuts.size()) {
									//
									for (std::vector<Transaction::NetworkUnlinkedOut>::iterator lOut = lFreeOuts.begin(); lOut != lFreeOuts.end(); lOut++) {
										if (lOut->utxo().amount() >= consensus_->settings()->proofAmount()) {
											lProofTx = lOut->utxo().out().tx();
											break;
										}
									}

									if (!lProofTx.isNull()) lCurrentBlock->setProofTx(lProofTx);
								}
							}


							if (gLog().isEnabled(Log::VALIDATOR))
								gLog().write(Log::VALIDATOR, std::string("[validator/miner]: challenge and proof ") +
									strprintf("b = %s/%d/%d/%d, h = %d, proof = %d/%s, saved = %d, %s#",
										lNextBlockChallenge.toHex(),
										lChallengeBlock, 
										lCurrentBlockChallenge,
										lChallengeBlockTx,
										lCurrentBlockHeight, lFreeOuts.size(), lProofTx.toHex(), lChallengeSaved, chain_.toHex().substr(0, 10)));

							// calc merkle root
							lCurrentBlock->setRoot(lCurrentBlockContext->calculateMerkleRoot());

							// make signature
							consensus_->mainKey()->sign(lCurrentBlock->hash(), lCurrentBlock->signature_);

							// get current header
							currentBlockHeader_ = lCurrentBlock->blockHeader();

							//
							if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[validator/miner]: new block found ") + strprintf("%s/%s#", lCurrentBlock->hash().toHex(), chain_.toHex().substr(0, 10)));
							
							IConsensus::ChainState lState = consensus_->chainState();
							if (minerRunning_ && lState == IConsensus::SYNCHRONIZED) {
								// commit block
								uint64_t lHeight = 0;
								mempool_->commit(lCurrentBlockContext);
								if (store_->commitBlock(lCurrentBlockContext, lHeight)) {
									// clean-up mempool (sanity)
									mempool_->removeTransactions(lCurrentBlockContext->block());
									// broadcast
									if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[validator/miner]: broadcasting found block ") + strprintf("%s/%s#", lCurrentBlock->hash().toHex(), chain_.toHex().substr(0, 10)));
									NetworkBlockHeader lHeader(lCurrentBlock->blockHeader(), lHeight);
									StatePtr lState = consensus_->currentState();
									consensus_->broadcastBlockHeaderAndState(lHeader, lState, lState->addressId()); // broadcast new header and state
									minerRunning_ = false; // stop until next block or timeout
								} else {
									if (lCurrentBlockContext->errors().size()) {
										for (std::map<uint256, std::list<std::string>>::iterator lErrors = lCurrentBlockContext->errors().begin(); lErrors != lCurrentBlockContext->errors().end(); lErrors++) {
											for (std::list<std::string>::iterator lError = lErrors->second.begin(); lError != lErrors->second.end(); lError++) {
												gLog().write(Log::GENERAL_ERROR, std::string("[miner/error]: ") + (*lError));
											}

											// drop from mempool
											mempool_->removeTransaction(lErrors->first);
											if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::GENERAL_ERROR, std::string("[miner/error]: DROP transaction from mempool ") + lErrors->first.toHex());
										}
									}

									minerRunning_ = false; // stop until next block or timeout
								}
							} else {
								if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[validator/miner]: skip found block ") + strprintf("%s/%s#", lCurrentBlock->hash().toHex(), chain_.toHex().substr(0, 10)));
							}
						} else {
							// timeout
							if (!lTimeout) {
								lTimeout = true;
								lWaitTo = lCurrentTime + (consensus_->blockTime())/1000;
								if (gLog().isEnabled(Log::VALIDATOR))
									gLog().write(Log::VALIDATOR, std::string("[miner]: timeout for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
							}
						}
					}
					catch(boost::thread_interrupted&) {
						minerRunning_ = false;
					}
				}

				// switch-out
				boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
			}

			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[miner]: stopped for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
		}
	}	

	void touch(const boost::system::error_code& error) {
		// life control
		if(!error) {
			//
			if ((consensus_->settings()->reindex() || consensus_->settings()->reindexShard() == chain_) && !reindexed_) {
				// stop miner
				stopMiner();
				//
				if (gLog().isEnabled(Log::VALIDATOR))
					gLog().write(Log::VALIDATOR, std::string("[touch]: reindexing ") + strprintf("%s#...", chain_.toHex().substr(0, 10)));
				//
				consensus_->doReindex();
				reindexed_ = true;
			}

			if (consensus_->settings()->resync() && !reindexed_) {
				// stop miner
				stopMiner();
				//
				if (gLog().isEnabled(Log::VALIDATOR))
					gLog().write(Log::VALIDATOR, std::string("[touch]: resyncing ") + strprintf("%s#...", chain_.toHex().substr(0, 10)));
				//
				consensus_->doSynchronize(true);
				reindexed_ = true;
			}

			//
			IConsensus::ChainState lState = consensus_->chainState();
			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[touch]: chain state = ") + strprintf("%s/%s#", consensus_->chainStateString(), chain_.toHex().substr(0, 10)));
			if (lState != IConsensus::SYNCHRONIZED || store_->synchronizing()) {
				// stop miner
				stopMiner();
			}

			if (lState != IConsensus::SYNCHRONIZED && lState != IConsensus::SYNCHRONIZING && lState != IConsensus::INDEXING) {
				//
				SynchronizationJobPtr lJob = consensus_->lastJob();
				if (lJob && getTime() - lJob->timestamp() > ((lJob->type() == SynchronizationJob::FULL ? 5 : 1) * consensus_->settings()->consensusSynchronizationLatency())) {
					consensus_->finishJob(nullptr); // force and restart job
					if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::GENERAL_ERROR, std::string("[validator/touch/error]: synchronization is seems to be stalled."));
				} else {
					//
					if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[touch]: chain ") + strprintf("%s#", chain_.toHex().substr(0, 10)) + std::string(" NOT synchronized, starting synchronization..."));
					if (consensus_->doSynchronize()) {
						// already synchronized
						if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[touch]: chain ") + strprintf("%s#", chain_.toHex().substr(0, 10)) + std::string(" IS synchronized."));
						startMiner();
					}
				}
			} else if (lState == IConsensus::SYNCHRONIZING) {
				//
				SynchronizationJobPtr lJob = consensus_->lastJob();
				if (lJob && getTime() - lJob->timestamp() > ((lJob->type() == SynchronizationJob::FULL ? 5 : 1) * consensus_->settings()->consensusSynchronizationLatency())) {
					consensus_->finishJob(nullptr); // force and restart job
					if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::GENERAL_ERROR, std::string("[validator/touch/error]: synchronization was stalled."));
				} else {
					// in case of acquiring block is in progress
					NetworkBlockHeader lEnqueuedBlock;
					if (store_->firstEnqueuedBlock(lEnqueuedBlock)) {
						// remove block
						store_->dequeueBlock(lEnqueuedBlock.blockHeader().hash());
						// re-acquire block
						if (!consensus_->acquireBlock(lEnqueuedBlock)) {
							// 2.1 peer was not found - reset state
							if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[touch]: peer for network block ") + 
								strprintf("%s", const_cast<NetworkBlockHeader&>(lEnqueuedBlock).blockHeader().hash().toHex()) + std::string(" was not found. Begin synchronization..."));
							consensus_->toNonSynchronized();
						}
					} else {
						// 2.2 nothing to do?
						if (!lJob) {
							if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::GENERAL_ERROR, std::string("[validator/touch/error]: synchronization probably was stalled."));
							consensus_->toNonSynchronized();
						}
					}
				}
			} else if (lState == IConsensus::SYNCHRONIZED) {
				// mining
				startMiner();

				// try to start synchronization
				if (!consensus_->settings()->isMiner()) {
					//
					BlockHeader lHeader;
					if (!store_->currentHeight(lHeader)) {
						if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[touch]: state seems NOT synchronized for ") + 
							strprintf("%s#...", chain_.toHex().substr(0, 10)));
						consensus_->toNonSynchronized();
					}
				}
			}

			// check pending transactions
			std::list<TransactionPtr> lPendingTxs;
			mempool_->wallet()->collectPendingTransactions(lPendingTxs);
			for (std::list<TransactionPtr>::iterator lTx = lPendingTxs.begin(); lTx != lPendingTxs.end(); lTx++) {
				if (*lTx == nullptr) continue;
				if (mempool_->isTransactionExists((*lTx)->id())) {
					TransactionContextPtr lCtx = TransactionContext::instance(*lTx);
					consensus_->broadcastTransaction(lCtx, mempool_->wallet()->firstKey()->createPKey().id());
				} else {
					//
					if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, 
						strprintf("[touch]: removing pending transaction %s/%s#", (*lTx)->id().toHex(), chain_.toHex().substr(0, 10)));					
					mempool_->wallet()->removePendingTransaction((*lTx)->id());
				}
			}

			// try to reprocess tx candidates in pool
			mempool_->processCandidates();
		}

		timer_->expires_at(timer_->expiry() + boost::asio::chrono::milliseconds(500));
		timer_->async_wait(boost::bind(&MainValidator::touch, shared_from_this(), boost::asio::placeholders::error));
	}

	IMemoryPoolPtr mempool() { return mempool_; }

private:
	uint256 chain_;
	IConsensusPtr consensus_;
	IMemoryPoolPtr mempool_;
	ITransactionStorePtr store_;

	// last maybe found block
	BlockHeader currentBlockHeader_;

	// controller
	boost::shared_ptr<boost::thread> controller_;

	// miner
	boost::shared_ptr<boost::thread> miner_;
	boost::mutex minerMutex_;
	boost::condition_variable minerActive_;

	// context
	boost::asio::io_context context_;

	typedef std::shared_ptr<boost::asio::steady_timer> TimerPtr;
	TimerPtr timer_;

	bool minerRunning_ = false;
	bool reindexed_ = false;
};

class ValidatorFactory {
public:
	// shard - entity based transaction
	static IValidatorPtr create(const uint256& chain, IConsensusPtr consensus, IMemoryPoolPtr mempool, ITransactionStorePtr store, EntityPtr dapp) {
		if (MainChain::id() == chain) {
			return MainValidator::instance(chain, consensus, mempool, store);
		} else if (dapp) {
			Validators::iterator lCreator = gValidators.find(dapp->entityName());
			if (lCreator != gValidators.end()) {
				return lCreator->second->create(chain, consensus, mempool, store);
			}			
		}

		return nullptr;
	}
};

} // qbit

#endif
