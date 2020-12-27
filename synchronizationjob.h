// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_SYNCHRONIZATION_JOB_H
#define QBIT_SYNCHRONIZATION_JOB_H

#include "ipeer.h"
#include "timestamp.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/chrono.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <memory>

namespace qbit {

class SynchronizationJob;
typedef std::shared_ptr<SynchronizationJob> SynchronizationJobPtr;

class IPeer;
typedef std::shared_ptr<IPeer> IPeerPtr;

class SynchronizationJob {
public:
	struct Agent {
		IPeerPtr peer_ = nullptr;
		uint64_t time_ = 0;
		Agent() {}
		Agent(IPeerPtr peer, uint64_t time) : peer_(peer), time_(time) {}
	};

	enum Type {
		UNDEFINED,
		FULL,
		PARTIAL,
		LARGE_PARTIAL
	};

public:
	SynchronizationJob(const uint256& block, Type type) : height_(0), block_(block), nextBlock_(block), type_(type) {}
	SynchronizationJob(const uint256& block, const uint256& lastBlock, Type type) : height_(0), block_(block), nextBlock_(block), lastBlock_(lastBlock), type_(type) {}
	SynchronizationJob(uint64_t height, const uint256& block, Type type) : height_(height), block_(block), type_() {}

	Type type() { return type_; }

	std::string typeString() {
		switch(type_) {
			case FULL: return "FULL";
			case PARTIAL: return "PARTIAL";
			case LARGE_PARTIAL: return "LARGE_PARTIAL";
		}

		return "UNDEFINED";
	}

	uint64_t acquireNextJob(IPeerPtr peer) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		if (height_) {
			std::pair<std::map<uint64_t, Agent>::iterator,bool> lPtr = 
				workers_.insert(std::map<uint64_t, Agent>::value_type(height_--, Agent(peer, qbit::getTime())));
			return lPtr.first->first;
		}

		return 0;
	}

	uint64_t reacquireJob(uint64_t height, IPeerPtr peer) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		workers_.erase(height);
		workers_[height] = Agent(peer, qbit::getTime());
		return height;
	}

	bool releaseJob(uint64_t height) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		return workers_.erase(height) == 1;
	}

	std::map<uint64_t, Agent> pendingJobs() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return workers_;
	}

	void setNextBlock(const uint256& block) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		nextBlock_ = block; 
	}

	void setLastBlock(const uint256& block) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		lastBlock_ = block; 
		nextBlock_.setNull();
		time_ = getTime(); // timestamp
	}

	void setLastBlockInstant(const uint256& block) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		lastBlock_ = block;
		// time_ = getTime(); // timestamp
	}

	uint256 nextBlock() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		// time_ = getTime(); // timestamp
		uint256 lBlock = nextBlock_;
		nextBlock_.setNull();
		return lBlock;
	}

	uint256 lastBlock() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return lastBlock_;
	}

	uint256 nextBlockInstant() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return nextBlock_;
	}

	uint256& block() { return block_; }
	uint64_t timestamp() { return time_; }
	uint64_t height() { return height_; }

	static SynchronizationJobPtr instance(uint64_t height, const uint256& block, Type type) { return std::make_shared<SynchronizationJob>(height, block, type); }
	static SynchronizationJobPtr instance(const uint256& block, Type type) { return std::make_shared<SynchronizationJob>(block, type); }
	static SynchronizationJobPtr instance(const uint256& block, const uint256& lastBlock, Type type) 
	{ return std::make_shared<SynchronizationJob>(block, lastBlock, type); }

	//
	// pending blocks
	void pushPendingBlock(const uint256& block) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		std::pair<std::set<uint256>::iterator, bool> lExists = pendingBlocksIndex_.insert(block);
		if (lExists.second) {
			pendingBlocks_.push_back(block);
		}
	}

	uint256 lastPendingBlock() {
		//
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		if (pendingBlocks_.size()) {
			return *pendingBlocks_.rbegin();
		}

		return uint256();
	}

	uint64_t pendingBlocks() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return pendingBlocks_.size();
	}

	uint64_t activeWorkers() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return txWorkers_.size();
	}

	uint256 acquireNextPendingBlockJob(IPeerPtr peer) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		if (pendingBlocks_.size()) {
			std::list<uint256>::iterator lBlock = pendingBlocks_.begin();
			//
			time_ = getTime(); // timestamp
			//
			std::pair<std::map<uint256, Agent>::iterator, bool> lResult = 
				txWorkers_.insert(std::map<uint256, Agent>::value_type(*lBlock, Agent(peer, qbit::getTime())));
			pendingBlocksIndex_.erase(*lBlock);
			pendingBlocks_.pop_front();
			return lResult.first->first;
		}

		return uint256();
	}

	bool releasePendingBlockJob(const uint256& block) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		return txWorkers_.erase(block) == 1;
	}

	uint256 reacquirePendingBlockJob(const uint256& block, IPeerPtr peer) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		txWorkers_.erase(block);
		txWorkers_[block] = Agent(peer, qbit::getTime());
		return block;
	}

	uint256 reacquirePendingBlockJob(IPeerPtr peer) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);

		if (txWorkers_.size()) {
			//
			time_ = getTime(); // timestamp
			//
			std::map<uint256, Agent>::iterator lWorker = txWorkers_.begin();
			lWorker->second = Agent(peer, qbit::getTime());
			return lWorker->first;
		}

		return uint256();
	}

	std::map<uint256, Agent> pendingBlockJobs() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return txWorkers_;
	}

	bool hasPendingBlocks() {
		//
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return pendingBlocks_.size() || txWorkers_.size();
	}

private:
	boost::mutex jobMutex_;
	uint64_t height_;
	uint256 block_;
	uint256 nextBlock_;
	uint256 lastBlock_;
	uint64_t time_;
	std::map<uint64_t, Agent> workers_; // TODO: add timeout & check
	std::map<uint256, Agent> txWorkers_; // TODO: add timeout & check
	std::list<uint256> pendingBlocks_;
	std::set<uint256> pendingBlocksIndex_;
	Type type_;
};

} // qbit

#endif
