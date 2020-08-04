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
	SynchronizationJob(const uint256& block) : height_(0), block_(block), nextBlock_(block) {}
	SynchronizationJob(uint64_t height, const uint256& block) : height_(height), block_(block) {}

	uint64_t acquireNextJob(IPeerPtr peer) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		if (height_) {
			std::pair<std::map<uint64_t, IPeerPtr>::iterator,bool> lPtr = 
				workers_.insert(std::map<uint64_t, IPeerPtr>::value_type(height_--, peer));
			return lPtr.first->first;
		}

		return 0;
	}

	uint64_t reacquireJob(uint64_t height, IPeerPtr peer) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		workers_[height] = peer;
		return height;
	}

	bool releaseJob(uint64_t height) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		return workers_.erase(height) == 1;
	}

	std::map<uint64_t, IPeerPtr> pendingJobs() {
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
	}

	uint256 nextBlock() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		uint256 lBlock = nextBlock_;
		nextBlock_.setNull();
		return lBlock;
	}

	uint256& lastBlock() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return lastBlock_;
	}

	uint256& block() { return block_; }
	uint64_t timestamp() { return time_; }

	static SynchronizationJobPtr instance(uint64_t height, const uint256& block) { return std::make_shared<SynchronizationJob>(height, block); }
	static SynchronizationJobPtr instance(const uint256& block) { return std::make_shared<SynchronizationJob>(block); }

	//
	// pending blocks
	void pushPendingBlock(const uint256& block) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		pendingBlocks_.push_back(block);
	}

	uint256 acquireNextPendingBlockJob(IPeerPtr peer) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		std::list<uint256>::iterator lBlock = pendingBlocks_.begin();
		if (lBlock != pendingBlocks_.end()) {
			std::pair<std::map<uint256, IPeerPtr>::iterator, bool> lResult = txWorkers_.insert(std::map<uint256, IPeerPtr>::value_type(*lBlock, peer));
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
		txWorkers_[block] = peer;
		return block;
	}

	std::map<uint256, IPeerPtr> pendingBlockJobs() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return txWorkers_;
	}	

private:
	boost::mutex jobMutex_;
	uint64_t height_;
	uint256 block_;
	uint256 nextBlock_;
	uint256 lastBlock_;
	uint64_t time_;
	std::map<uint64_t, IPeerPtr> workers_;
	std::map<uint256, IPeerPtr> txWorkers_;
	std::list<uint256> pendingBlocks_;
};

} // qbit

#endif
