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

		// check if work done
		if (!consensus_->checkSequenceConsistency(lOther)) {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[checkBlockHeader]: check sequence consistency FAILED ") +
				strprintf("block = %s, prev = %s, chain = %s#", const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().hash().toHex(), 
					const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().prev().toHex(), chain_.toHex().substr(0, 10)));
			return IValidator::INTEGRITY_IS_INVALID;
		}

		if (lOther.time() < lHeader.time() && const_cast<NetworkBlockHeader&>(blockHeader).height() > lCurrentHeight) {
			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader]: proposed block time is less than median time ") + 
				strprintf("current = %d, proposed = %d, height = %d, new = %s, our = %s, origin = %s/%s#", 
					lHeader.time(), lOther.time(),
					const_cast<NetworkBlockHeader&>(blockHeader).height(),
					lOther.hash().toHex(), lHeader.hash().toHex(), 
						lOther.origin().toHex(), chain_.toHex().substr(0, 10)));
			return IValidator::INTEGRITY_IS_INVALID;
		}

		if (lOther.prev() == lHeader.hash()) {
			if (lOther.origin() == lHeader.origin() && !consensus_->isSimpleNetwork()) {
				if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader]: sequential blocks is not allowed ") + 
					strprintf("height = %d, new = %s, our = %s, origin = %s/%s#", const_cast<NetworkBlockHeader&>(blockHeader).height(), 
						lOther.hash().toHex(), lHeader.hash().toHex(), 
							lOther.origin().toHex(), chain_.toHex().substr(0, 10)));
				return IValidator::ORIGIN_NOT_ALLOWED;
			} else if (lOther.time() < lHeader.time()) {
				if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader]: next block time is less than median time ") + 
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

		if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader]: broken chain for ") + 
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

private:
	void stopMiner() {
		// stop miner
		if (consensus_->settings()->isMiner() && minerRunning_) {
			gLog().write(Log::VALIDATOR, std::string("[miner]: stopping for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
			minerRunning_ = false;
			minerActive_.notify_one();
		}
	}

	void startMiner() {
		// start miner
		if (consensus_->settings()->isMiner() && !minerRunning_ && !store_->synchronizing()) {
			BlockHeader lHeader = store_->currentBlockHeader();
			if (currentBlockHeader_.origin() != lHeader.origin() && !consensus_->isSimpleNetwork() || 
					consensus_->isSimpleNetwork()
				/*lHeader.hash() != currentBlockHeader_.hash() || 
						consensus_->currentTime() - currentBlockHeader_.time() >= (consensus_->blockTime() / 1000)*/) {
				if (!miner_) miner_ = boost::shared_ptr<boost::thread>(
							new boost::thread(
								boost::bind(&MainValidator::miner, shared_from_this())));
				minerRunning_ = true;
				minerActive_.notify_one();
			}
		}
	}

	void controller() {
		// log
		gLog().write(Log::INFO, std::string("[validator]: starting for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));

		try {
			context_.run();
		} 
		catch(qbit::exception& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[peerManager]: qbit error -> ") + ex.what());
		}
		catch(boost::system::system_error& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[validator]: context error -> ") + ex.what());
		}
		catch(std::runtime_error& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[peerManager]: runtime error -> ") + ex.what());
		}

		// log
		gLog().write(Log::INFO, std::string("[validator]: stopping for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
	}

	//
	// TODO: remove
	boost::random::mt19937 lGen;

	//
	// miner thread
	void miner() {
		//
		while(true) {
			boost::mutex::scoped_lock lLock(minerMutex_);
			while(!minerRunning_) minerActive_.wait(lLock);

			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[miner]: starting for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));

			// check and run
			while(minerRunning_) {
				//
				uint8_t EDGEBITS_ = 20;
				uint8_t PROOFSIZE_ = 40;

				//
				const uint64_t CHANGE_ALG_TIME_0 = 1614422850; // in seconds 1614163650 + 3 days
				if (consensus_->currentTime() > CHANGE_ALG_TIME_0) {
					EDGEBITS_ = 20;
					PROOFSIZE_ = 42;
				}
				
				try {
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

					// calc merkle root
					lCurrentBlock->setRoot(lCurrentBlockContext->calculateMerkleRoot());

					// get current header
					currentBlockHeader_ = lCurrentBlock->blockHeader();

					// TODO: mining -----------------------------------------
					if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[validator/miner]: looking for a block for ") + strprintf("%s#", chain_.toHex().substr(0, 10)) + "...");

					//
					// begin:cuckoo -----------------------------------------------
					//

					//calculate work required
					BlockHeader lPrev, lPPrev;
					bool lNegative = false;
    				bool lOverflow = false;
					arith_uint256 lTarget;
					lCurrentBlock->bits_ = qbit::getNextWorkRequired(store_, lCurrentBlock, consensus_->blockTime()/1000);
					lTarget.SetCompact(lCurrentBlock->bits_, &lNegative, &lOverflow);

					uint64_t lStartTime = consensus_->currentTime();

					int lNonce = 0;
					bool lTimeout = false;
					bool lResult = false;
					while(minerRunning_) {
						//
						if (qbit::gSparingMode) {
							boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
						}

						//
						std::set<uint32_t> lCycle;
						lCurrentBlock->nonce_ = lNonce;
						lResult = FindCycle(lCurrentBlock->hash(), EDGEBITS_, PROOFSIZE_, lCycle);
						lNonce++;

						// calculation timed out
						if (consensus_->currentTime() - lStartTime >= (consensus_->blockTime())/1000) { lTimeout = true; break; }
						if (!lCycle.size()) { /*cycle not found*/  continue; }

						HashWriter lStream(SER_GETHASH, PROTOCOL_VERSION);
						std::vector<uint32_t> lVector(lCycle.begin(), lCycle.end());
						lCurrentBlock->cycle_ = lVector; // save vector
						lStream << lVector;
						uint256 lCycleHash = lStream.GetHash();

						arith_uint256 lCycleHashArith = UintToArith256(lCycleHash);
						if (lNegative || lTarget == 0 || lOverflow || lTarget < lCycleHashArith) { continue; }
						if (lResult) { 
							break;
						}
					}

					// timeout
					if (lTimeout) { 
						if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[validator/miner]: timeout exiped during calculation for ") + strprintf("%s/%s#", lCurrentBlock->hash().toHex(), chain_.toHex().substr(0, 10)));
						continue;
					}

					if (!lResult) {
						if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[validator/miner]: cycle not found for ") + strprintf("%s/%s#", lCurrentBlock->hash().toHex(), chain_.toHex().substr(0, 10)));
						continue;
					}

					// check block pow
					int lVerifyResult = VerifyCycle(lCurrentBlock->hash(), EDGEBITS_, PROOFSIZE_, lCurrentBlock->cycle_);
					if (lVerifyResult != verify_code::POW_OK) {
						if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[validator/miner/error]: cycle verification FAILED for ") + strprintf("%s/%s#", lCurrentBlock->hash().toHex(), chain_.toHex().substr(0, 10)));
						continue;
					}

					//
					// end:cuckoo -----------------------------------------------
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
						}
					} else {
						if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[validator/miner]: skip found block ") + strprintf("%s/%s#", lCurrentBlock->hash().toHex(), chain_.toHex().substr(0, 10)));
					}
				}
				catch(boost::thread_interrupted&) {
					minerRunning_ = false;
				}
			}

			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[miner]: stopped for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
		}
	}	

	void touch(const boost::system::error_code& error) {
		// life control
		if(!error) {
			//
			if (consensus_->settings()->reindex() && !reindexed_) {
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
				if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[touch]: chain ") + strprintf("%s#", chain_.toHex().substr(0, 10)) + std::string(" NOT synchronized, starting synchronization..."));
				if (consensus_->doSynchronize()) {
					// already synchronized
					if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[touch]: chain ") + strprintf("%s#", chain_.toHex().substr(0, 10)) + std::string(" IS synchronized."));
					startMiner();
				}
			} else if (lState == IConsensus::SYNCHRONIZING) {
				//
				SynchronizationJobPtr lJob = consensus_->lastJob();
				if (lJob && getTime() - lJob->timestamp() > consensus_->settings()->consensusSynchronizationLatency()) {
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
