// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_VALIDATOR_H
#define QBIT_BUZZER_VALIDATOR_H

#include "../../iconsensus.h"
#include "../../isettings.h"
#include "../../imemorypool.h"
#include "../../ivalidator.h"
#include "../../log/log.h"
#include "../../entity.h"
#include "../../block.h"
#include "../../blockcontext.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/chrono.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace qbit {

class BuzzerValidator: public IValidator, public std::enable_shared_from_this<BuzzerValidator> {
public:
	BuzzerValidator(const uint256& chain, IConsensusPtr consensus, IMemoryPoolPtr mempool, ITransactionStorePtr store) : 
		chain_(chain), consensus_(consensus), mempool_(mempool), store_(store) {}

	uint256 chain() { return chain_; }

	void run() {
		if (!controller_) {
			timer_ = TimerPtr(new boost::asio::steady_timer(context_, boost::asio::chrono::seconds(1)));
			timer_->async_wait(boost::bind(&BuzzerValidator::touch, shared_from_this(), boost::asio::placeholders::error));			

			controller_ = boost::shared_ptr<boost::thread>(
						new boost::thread(
							boost::bind(&BuzzerValidator::controller, shared_from_this())));
		}
	}

	void stop() {
		gLog().write(Log::INFO, std::string("[buzzer/stop]: stopping..."));

		context_.stop();
		controller_->join();
		stopMiner();

		consensus_.reset();
		mempool_.reset();
		store_.reset();
	}

	static IValidatorPtr instance(const uint256& chain, IConsensusPtr consensus, IMemoryPoolPtr mempool, ITransactionStorePtr store) {
		return std::make_shared<BuzzerValidator>(chain, consensus, mempool, store);
	}

	IValidator::BlockCheckResult checkBlockHeader(const NetworkBlockHeader& blockHeader) {
		//
		BlockHeader lHeader = store_->currentBlockHeader();
		BlockHeader& lOther = const_cast<NetworkBlockHeader&>(blockHeader).blockHeader();

		// check if work done
		if (!consensus_->checkSequenceConsistency(lOther)) {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[buzzer/checkBlockHeader]: check sequence consistency FAILED ") +
				strprintf("block = %s, prev = %s, chain = %s#", const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().hash().toHex(), 
					const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().prev().toHex(), chain_.toHex().substr(0, 10)));
			return IValidator::INTEGRITY_IS_INVALID;
		}

		if (lOther.prev() == lHeader.hash()) {
			if (lOther.origin() == lHeader.origin() && !consensus_->isSimpleNetwork()) {
				if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/checkBlockHeader]: sequential blocks is not allowed ") + 
					strprintf("height = %d, new = %s, our = %s, origin = %s/%s#", const_cast<NetworkBlockHeader&>(blockHeader).height(), 
						lOther.hash().toHex(), lHeader.hash().toHex(), 
							lOther.origin().toHex(), chain_.toHex().substr(0, 10)));
				return IValidator::ORIGIN_NOT_ALLOWED;
			}

			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/checkBlockHeader]: proposed header is correct ") + 
				strprintf("height = %d, new = %s, our = %s, origin = %s/%s#", const_cast<NetworkBlockHeader&>(blockHeader).height(), 
					lOther.hash().toHex(), lHeader.hash().toHex(), lOther.origin().toHex(), chain_.toHex().substr(0, 10)));

			stopMiner();
			return IValidator::SUCCESS;
		}

		if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/checkBlockHeader]: broken chain for ") + 
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
				gLog().write(Log::VALIDATOR, std::string("[buzzer/acceptBlockHeader]: peer for network block ") + 
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
		if (consensus_->settings()->isMiner()) {
			gLog().write(Log::VALIDATOR, std::string("[buzzer/miner]: stopping for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
			minerRunning_ = false;
			minerActive_.notify_one();
		}
	}

	void startMiner() {
		// start miner
		if (consensus_->settings()->isMiner() && !minerRunning_) {
			BlockHeader lHeader = store_->currentBlockHeader();
			if (lHeader.hash() != currentBlockHeader_.hash() || 
						consensus_->currentTime() - currentBlockHeader_.time() >= (consensus_->blockTime() / 1000)) {
				if (!miner_) miner_ = boost::shared_ptr<boost::thread>(
							new boost::thread(
								boost::bind(&BuzzerValidator::miner, shared_from_this())));
				minerRunning_ = true;
				minerActive_.notify_one();
			}
		}
	}

	void controller() {
		// log
		gLog().write(Log::INFO, std::string("[buzzer/validator]: starting for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));

		try {
			context_.run();
		} 
		catch(boost::system::system_error& ex) {
			gLog().write(Log::ERROR, std::string("[buzzer/validator]: context error -> ") + ex.what());
		}

		// log
		gLog().write(Log::INFO, std::string("[buzzer/validator]: stopping for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
	}

	//
	// TODO: remove
	boost::random::mt19937 lGen;

	//
	// TODO: rewrite
	void miner() {
		//
		while(true) {
			boost::mutex::scoped_lock lLock(minerMutex_);
			while(!minerRunning_) minerActive_.wait(lLock);

			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/miner]: starting for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));

			// check and run
			while(minerRunning_) {
				try {
					// get block template
					BlockPtr lCurrentBlock = Block::instance();

					// prepare block
					BlockContextPtr lCurrentBlockContext = mempool_->beginBlock(lCurrentBlock); // LOCK!

					// make coinbase tx
					BlockHeader lHeader;
					uint64_t lCurrentHeight = store_->currentHeight(lHeader);
					// just fee collecting
					// TODO: we need to push this fees to the main chain
					TransactionContextPtr lBase = mempool_->wallet()->createTxBase(chain_, lCurrentBlockContext->fee());
					lCurrentBlock->prepend(lBase->tx());

					// calc merkle root
					lCurrentBlock->setRoot(lCurrentBlockContext->calculateMerkleRoot());

					// get current header
					currentBlockHeader_ = lCurrentBlock->blockHeader();

					// TODO: mining -----------------------------------------
					if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/validator/miner]: looking for a block for ") + strprintf("%s#", chain_.toHex().substr(0, 10)) + "...");
					
					boost::random::uniform_int_distribution<> lDist(3, (consensus_->blockTime())/1000);
					int lMSeconds = lDist(lGen);
					uint64_t lStartTime = getTime();
					while(minerRunning_) {
						if (getTime() - lStartTime >= lMSeconds) break;
						boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
					}
					// TODO: mining -----------------------------------------
					
					if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/validator/miner]: new block found ") + strprintf("%s/%s#", lCurrentBlock->hash().toHex(), chain_.toHex().substr(0, 10)));

					IConsensus::ChainState lState = consensus_->chainState();
					if (minerRunning_ && lState == IConsensus::SYNCHRONIZED) {
						// commit block
						uint64_t lHeight = 0;
						mempool_->commit(lCurrentBlockContext);
						if (store_->commitBlock(lCurrentBlockContext, lHeight)) {
							// clean-up mempool (sanity)
							mempool_->removeTransactions(lCurrentBlockContext->block());
							
							// broadcast
							if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/validator/miner]: broadcasting found block ") + strprintf("%s/%s#", lCurrentBlock->hash().toHex(), chain_.toHex().substr(0, 10)));
							NetworkBlockHeader lHeader(lCurrentBlock->blockHeader(), lHeight);
							StatePtr lState = consensus_->currentState();
							// broadcast new header and state
							consensus_->broadcastBlockHeaderAndState(lHeader, lState, lState->addressId()); 

							// make blockbase transaction to claim mining-fee reward
							try {
								TransactionContextPtr lRewardTx = mempool_->wallet()->createTxBlockBase(lCurrentBlock->blockHeader(), lCurrentBlockContext->fee());
								if (lRewardTx) {
									//
									if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/miner]: ") + strprintf("BLOCKBASE transaction created %d/%s/%s#", 
										lCurrentBlockContext->fee(), lRewardTx->tx()->id().toHex(), chain_.toHex().substr(0, 10)));

									IMemoryPoolPtr lRewardPool = mempool_->wallet()->mempoolManager()->locate(lRewardTx->tx()->chain());
									if (lRewardPool) {
										//
										if (lRewardPool->pushTransaction(lRewardTx)) {
											// check for errors
											if (lRewardTx->errors().size()) {
												gLog().write(Log::ERROR, std::string("[buzzer/miner/error]: ") + strprintf("BLOCKBASE transaction push failed %s/%s#: %s", 
													lRewardTx->tx()->id().toHex(), chain_.toHex().substr(0, 10), *lRewardTx->errors().begin()));
											}

											if (!lRewardPool->consensus()->broadcastTransaction(lRewardTx, mempool_->wallet()->firstKey().createPKey().id())) {
												gLog().write(Log::WARNING, std::string("[buzzer/miner/warning]: ") + strprintf("BLOCKBASE transaction broadcast failed %s/%s#", 
													lRewardTx->tx()->id().toHex(), chain_.toHex().substr(0, 10)));
											}
										}
									} else {
										gLog().write(Log::ERROR, std::string("[buzzer/miner/error]: ") + strprintf("memory pool was not found for BLOCKBASE transaction %s/%s#", 
											lRewardTx->tx()->id().toHex(), chain_.toHex().substr(0, 10)));
									}									
								}
							} catch(qbit::exception& ex) {
								gLog().write(Log::ERROR, std::string("[buzzer/miner/error]: ") + strprintf("BLOCKBASE transaction creation failed - %s: %s", 
									ex.code(), ex.what()));
							}

							minerRunning_ = false; // stop until next block or timeout
						} else {
							if (lCurrentBlockContext->errors().size()) {
								for (std::list<std::string>::iterator lError = lCurrentBlockContext->errors().begin(); lError != lCurrentBlockContext->errors().end(); lError++) {
									if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::ERROR, std::string("[buzzer/miner/error]: ") + (*lError));
								}
							}
						}
					} else {
						if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/validator/miner]: skip found block ") + strprintf("%s/%s#", lCurrentBlock->hash().toHex(), chain_.toHex().substr(0, 10)));
					}
				}
				catch(boost::thread_interrupted&) {
					minerRunning_ = false;
				}
			}

			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/miner]: stopped for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
		}
	}	

	void touch(const boost::system::error_code& error) {
		// life control
		if(!error) {
			//
			IConsensus::ChainState lState = consensus_->chainState();
			if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/touch]: chain state = ") + strprintf("%s/%s#", consensus_->chainStateString(), chain_.toHex().substr(0, 10)));

			if (lState != IConsensus::SYNCHRONIZED && lState != IConsensus::SYNCHRONIZING) {
				// stop miner
				stopMiner();

				if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/touch]: chain ") + strprintf("%s#", chain_.toHex().substr(0, 10)) + std::string(" NOT synchronized, starting synchronization..."));
				if (consensus_->doSynchronize()) {
					// already synchronized
					if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/touch]: chain ") + strprintf("%s#", chain_.toHex().substr(0, 10)) + std::string(" IS synchronized."));
					startMiner();
				}
			} else if (lState == IConsensus::SYNCHRONIZING) {
				SynchronizationJobPtr lJob = consensus_->lastJob();
				if (lJob && getTime() - lJob->timestamp() > consensus_->settings()->consensusSynchronizationLatency()) {
					consensus_->toNonSynchronized(); // restart sync process
					if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::ERROR, std::string("[buzzer/validator/touch/error]: synchronization was stalled."));
				} else {
					// in case of acquiring block is in progress
					NetworkBlockHeader lEnqueuedBlock;
					if (store_->firstEnqueuedBlock(lEnqueuedBlock)) {
						// remove block
						store_->dequeueBlock(lEnqueuedBlock.blockHeader().hash());
						// re-acquire block
						if (!consensus_->acquireBlock(lEnqueuedBlock)) {
							// 2.1 peer was not found - reset state
							if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, std::string("[buzzer/touch]: peer for network block ") + 
								strprintf("%s", const_cast<NetworkBlockHeader&>(lEnqueuedBlock).blockHeader().hash().toHex()) + std::string(" was not found. Begin synchronization..."));
							consensus_->toNonSynchronized();
						}
					}
				}
			} else if (lState == IConsensus::SYNCHRONIZED) {
				// mining
				startMiner();
			}

			// check pending transactions
			std::list<TransactionPtr> lPendingTxs;
			mempool_->wallet()->collectPendingTransactions(lPendingTxs);
			for (std::list<TransactionPtr>::iterator lTx = lPendingTxs.begin(); lTx != lPendingTxs.end(); lTx++) {
				if (mempool_->isTransactionExists((*lTx)->id())) {
					TransactionContextPtr lCtx = TransactionContext::instance(*lTx);
					consensus_->broadcastTransaction(lCtx, mempool_->wallet()->firstKey().createPKey().id());
				} else {
					//
					if (gLog().isEnabled(Log::VALIDATOR)) gLog().write(Log::VALIDATOR, 
						strprintf("[buzzer/touch]: removing pending transaction %s/%s#", (*lTx)->id().toHex(), chain_.toHex().substr(0, 10)));					
					mempool_->wallet()->removePendingTransaction((*lTx)->id());
				}
			}

		}

		timer_->expires_at(timer_->expiry() + boost::asio::chrono::milliseconds(500));
		timer_->async_wait(boost::bind(&BuzzerValidator::touch, shared_from_this(), boost::asio::placeholders::error));
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
};

class BuzzerValidatorCreator: public ValidatorCreator {
public:
	BuzzerValidatorCreator() {}
	IValidatorPtr create(const uint256& chain, IConsensusPtr consensus, IMemoryPoolPtr mempool, ITransactionStorePtr store) { 
		return std::make_shared<BuzzerValidator>(chain, consensus, mempool, store); 
	}

	static ValidatorCreatorPtr instance() { return std::make_shared<BuzzerValidatorCreator>(); }
};

} // qbit

#endif