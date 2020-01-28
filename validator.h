// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_VALIDATOR_H
#define QBIT_VALIDATOR_H

#include "iconsensus.h"
#include "isettings.h"
#include "imemorypool.h"
#include "ivalidator.h"
#include "log/log.h"
#include "entity.h"
#include "block.h"
#include "blockcontext.h"

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

	bool checkBlockHeader(const NetworkBlockHeader& blockHeader) {
		// TODO: quick check block
		// if workDone()
		//
		BlockHeader lHeader = store_->currentBlockHeader();
		BlockHeader& lOther = const_cast<NetworkBlockHeader&>(blockHeader).blockHeader();
		if (lOther.prev() == lHeader.hash())
			return true;
		else {
			gLog().write(Log::VALIDATOR, std::string("[checkBlockHeader]: broken chain for ") + 
				strprintf("%s/%s#", lOther.hash().toHex(), chain_.toHex().substr(0, 10)));
			consensus_->toNonSynchronized();
			return false;
		}

		return true;
	}

	bool acceptBlockHeader(const NetworkBlockHeader& blockHeader) {
		// TODO: deep check and process
		// ?? check all transactions for existense and merkleroot

		IConsensus::ChainState lState = consensus_->chainState();
		if (lState == IConsensus::SYNCHRONIZED) {
			// 1. stop mining
			stopMiner();
			// 2. request found block
			if (!consensus_->acquireBlock(blockHeader)) {
				// 2.1 peer was not found - reset state
				gLog().write(Log::VALIDATOR, std::string("[acceptBlockHeader]: peer for network block ") + 
					strprintf("%s", const_cast<NetworkBlockHeader&>(blockHeader).blockHeader().hash().toHex()) + std::string(" was not found. Begin synchronization..."));
				consensus_->toNonSynchronized();
			}

		} else return false;

		return true;
	}

private:
	void stopMiner() {
		// stop miner
		if (consensus_->settings()->isMiner() && miner_) {
			gLog().write(Log::VALIDATOR, std::string("[miner]: stopping for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
			miner_->interrupt();
			miner_->join();
			miner_ = nullptr; // reset
		}
	}

	void startMiner() {
		// start miner
		if (consensus_->settings()->isMiner() && !miner_)
			miner_ = boost::shared_ptr<boost::thread>(
						new boost::thread(
							boost::bind(&MainValidator::miner, shared_from_this())));
	}

	void controller() {
		// log
		gLog().write(Log::INFO, std::string("[validator]: starting for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));

		try {
			context_.run();
		} 
		catch(boost::system::system_error& ex) {
			gLog().write(Log::ERROR, std::string("[validator]: context error -> ") + ex.what());
		}

		// log
		gLog().write(Log::INFO, std::string("[validator]: stopping for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
	}

	void miner() {
		gLog().write(Log::VALIDATOR, std::string("[miner]: starting for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));

		// mark
		minerRunning_ = true;

		// check and run
		while(minerRunning_) {
			try {
				// get block template
				currentBlock_ = Block::instance();

				// make coinbase tx
				TransactionContextPtr lCoinbase = mempool_->wallet()->createTxCoinBase(QBIT);
				currentBlock_->append(lCoinbase->tx());

				// prepare block
				currentBlockContext_ = mempool_->beginBlock(currentBlock_); // LOCK!

				// TODO: mine
				gLog().write(Log::VALIDATOR, std::string("[validator/miner]: looking for a block for ") + strprintf("%s#", chain_.toHex().substr(0, 10)) + "...");
				
				boost::random::mt19937 lGen;
				boost::random::uniform_int_distribution<> lDist(1000, consensus_->blockTime());
				int lMSeconds = lDist(lGen);
				boost::this_thread::sleep_for(boost::chrono::milliseconds(lMSeconds));
				
				gLog().write(Log::VALIDATOR, std::string("[validator/miner]: new block found ") + strprintf("%s/%s#", currentBlock_->hash().toHex(), chain_.toHex().substr(0, 10)));

				// commit block
				size_t lHeight = 0;
				mempool_->commit(currentBlockContext_);
				if (store_->commitBlock(currentBlockContext_, lHeight)) {
					// broadcast
					gLog().write(Log::VALIDATOR, std::string("[validator/miner]: broadcasting found block ") + strprintf("%s/%s#", currentBlock_->hash().toHex(), chain_.toHex().substr(0, 10)));
					NetworkBlockHeader lHeader(currentBlock_->blockHeader(), lHeight, lCoinbase->tx(), mempool_->wallet()->firstKey().createPKey().id());
					consensus_->broadcastState(consensus_->currentState(), nullptr); // broadcast changed state
					consensus_->broadcastBlockHeader(lHeader, nullptr); // broadcast new header
				} else {
					for (std::list<std::string>::iterator lError = currentBlockContext_->errors().begin(); lError != currentBlockContext_->errors().end(); lError++) {
						gLog().write(Log::ERROR, std::string("[miner/error]: ") + (*lError));
					}
				}
			}
			catch(boost::thread_interrupted&) {
				minerRunning_ = false;
			}
		}

		gLog().write(Log::VALIDATOR, std::string("[miner]: stopped for ") + strprintf("%s#", chain_.toHex().substr(0, 10)));
	}	

	void touch(const boost::system::error_code& error) {
		// TODO: touch process
		if(!error) {
			//
			IConsensus::ChainState lState = consensus_->chainState();
			gLog().write(Log::VALIDATOR, std::string("[touch]: chain state = ") + strprintf("%s/%s#", consensus_->chainStateString(), chain_.toHex().substr(0, 10)));

			if (lState != IConsensus::SYNCHRONIZED && lState != IConsensus::SYNCHRONIZING) {
				// stop miner
				stopMiner();

				gLog().write(Log::VALIDATOR, std::string("[touch]: chain ") + strprintf("%s#", chain_.toHex().substr(0, 10)) + std::string(" NOT synchronized, starting synchronization..."));
				if (consensus_->doSynchronize()) {
					// already synchronized
					gLog().write(Log::VALIDATOR, std::string("[touch]: chain ") + strprintf("%s#", chain_.toHex().substr(0, 10)) + std::string(" IS synchronized."));
					startMiner();
				}
			} else if (lState == IConsensus::SYNCHRONIZING) {
				SynchronizationJobPtr lJob = consensus_->lastJob();
				if (lJob && getTime() - lJob->timestamp() > consensus_->settings()->consensusSynchronizationLatency()) {
					consensus_->toNonSynchronized(); // restart sync process
					gLog().write(Log::ERROR, std::string("[validator/touch/error]: synchronization was stalled."));
				}
			} else if (lState == IConsensus::SYNCHRONIZED) {
				// mining
				startMiner();
			}
		} else {
			// log
			// gLog().write(Log::ERROR, std::string("[validator/touch]: ") + error.message());
		}

		timer_->expires_at(timer_->expiry() + boost::asio::chrono::seconds(1));
		timer_->async_wait(boost::bind(&MainValidator::touch, shared_from_this(), boost::asio::placeholders::error));
	}

	IMemoryPoolPtr mempool() { return mempool_; }

private:
	uint256 chain_;
	IConsensusPtr consensus_;
	IMemoryPoolPtr mempool_;
	ITransactionStorePtr store_;

	// current block template
	BlockPtr currentBlock_;
	BlockContextPtr currentBlockContext_;

	// controller
	boost::shared_ptr<boost::thread> controller_;

	// miner
	boost::shared_ptr<boost::thread> miner_;

	// context
	boost::asio::io_context context_;

	typedef std::shared_ptr<boost::asio::steady_timer> TimerPtr;
	TimerPtr timer_;

	bool minerRunning_ = false;
};

class ValidatorFactory {
public:
	// shard - entity based transaction
	static IValidatorPtr create(const uint256& chain, IConsensusPtr consensus, IMemoryPoolPtr mempool, ITransactionStorePtr store, EntityPtr /*entity*/) {
		if (MainChain::id() == chain) {
			return MainValidator::instance(chain, consensus, mempool, store);
		}

		return nullptr;
	}
};

} // qbit

#endif
