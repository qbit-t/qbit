// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_SYNCHRONIZATION_JOB_H
#define QBIT_SYNCHRONIZATION_JOB_H

#include "ipeer.h"
#include "timestamp.h"
#include "db/containers.h"
#include "mkpath.h"
#include "isettings.h"

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

	class Chunk {
	public:
		uint256 block_;
		bool frameExists_ = false;
		bool indexed_ = false;

		Chunk() {}
		Chunk(const uint256& block, bool frameExists, bool indexed) : block_(block), frameExists_(frameExists), indexed_(indexed) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			if (ser_action.ForRead()) {
				s >> block_;
				s >> frameExists_;
				s >> indexed_;
			} else {
				s << block_;
				s << frameExists_;
				s << indexed_;
			}
		}
	};

	enum Type {
		UNDEFINED,
		FULL,
		PARTIAL,
		LARGE_PARTIAL
	};

	enum Stage {
		HEADER_FEED,
		BLOCK_FEED
	};

public:
	SynchronizationJob(ISettingsPtr settings, const uint256& chain, const uint256& block, uint64_t delta, Type type) :
		settings_(settings),
		height_(0), delta_(delta), block_(block), nextBlock_(block), type_(type),
		timestamp_(qbit::getMicroseconds()), chain_(chain),
		tempspaceBlocks_(strprintf("%s/%s/b-%d", settings_->dataPath(), chain_.toHex(), timestamp_)),
		tempspaceChunks_(strprintf("%s/%s/c-%d", settings_->dataPath(), chain_.toHex(), timestamp_)),
		spaceBlocks_(std::make_shared<db::DbContainerSpace>(tempspaceBlocks_)),
		spaceChunks_(std::make_shared<db::DbContainerSpace>(tempspaceChunks_)),
		pendingBlocks_("pendingBlocks", spaceBlocks_),
		pendingBlocksIndex_("pendingBlocksIndex", spaceBlocks_),
		chunks_("chunks", spaceChunks_),
		queuedChunks_("queuedChunks", spaceChunks_)		
		{ open(); }
	SynchronizationJob(ISettingsPtr settings, const uint256& chain, const uint256& block, const uint256& lastBlock, uint64_t delta, uint64_t lastHeight, Type type) :
		height_(0), delta_(delta), block_(block),
		nextBlock_(block), lastBlock_(lastBlock), lastHeight_(lastHeight), type_(type),
		timestamp_(qbit::getMicroseconds()), chain_(chain),
		tempspaceBlocks_(strprintf("%s/%s/b-%d", settings_->dataPath(), chain_.toHex(), timestamp_)),
		tempspaceChunks_(strprintf("%s/%s/c-%d", settings_->dataPath(), chain_.toHex(), timestamp_)),
		spaceBlocks_(std::make_shared<db::DbContainerSpace>(tempspaceBlocks_)),
		spaceChunks_(std::make_shared<db::DbContainerSpace>(tempspaceChunks_)),
		pendingBlocks_("pendingBlocks", spaceBlocks_),
		pendingBlocksIndex_("pendingBlocksIndex", spaceBlocks_),
		chunks_("chunks", spaceChunks_),
		queuedChunks_("queuedChunks", spaceChunks_)		
		{ open(); }
	SynchronizationJob(ISettingsPtr settings, const uint256& chain, uint64_t height, const uint256& block, Type type) :
		height_(height), delta_(0), block_(block), type_(),
		timestamp_(qbit::getMicroseconds()), chain_(chain),
		tempspaceBlocks_(strprintf("%s/%s/b-%d", settings_->dataPath(), chain_.toHex(), timestamp_)),
		tempspaceChunks_(strprintf("%s/%s/c-%d", settings_->dataPath(), chain_.toHex(), timestamp_)),
		spaceBlocks_(std::make_shared<db::DbContainerSpace>(tempspaceBlocks_)),
		spaceChunks_(std::make_shared<db::DbContainerSpace>(tempspaceChunks_)),
		pendingBlocks_("pendingBlocks", spaceBlocks_),
		pendingBlocksIndex_("pendingBlocksIndex", spaceBlocks_),
		chunks_("chunks", spaceChunks_),
		queuedChunks_("queuedChunks", spaceChunks_)		
		{ open(); }

	~SynchronizationJob() {
		closeBlocks();
		closeChunks();
	}

	bool open() {
		//
		if (mkpath(tempspaceBlocks_.c_str(), 0777)) return false;

		pendingBlocks_.open();
		pendingBlocks_.attach();
		pendingBlocksIndex_.open();
		pendingBlocksIndex_.attach();
		spaceBlocks_->open();

		openChunks();

		time_ = getTime();

		return true;
	}

	bool openChunks() {
		if (mkpath(tempspaceChunks_.c_str(), 0777)) return false;
		chunks_.open();
		chunks_.attach();
		queuedChunks_.open();
		queuedChunks_.attach();
		spaceChunks_->open();

		return true;
	}

	void closeBlocks() {
		//
		pendingBlocks_.close();
		pendingBlocksIndex_.close();
		spaceBlocks_->close();
		rmpath(tempspaceBlocks_.c_str());
	}

	void closeChunks() {
		chunks_.close();
		queuedChunks_.close();
		spaceChunks_->close();
		rmpath(tempspaceChunks_.c_str());		
	}

	Type type() { return type_; }
	Stage stage() { return stage_; }

	void toBlockFeed() { stage_ = BLOCK_FEED; }
	void toHeaderFeed() { stage_ = HEADER_FEED; }

	uint64_t unique() { return timestamp_; }

	std::string typeString() {
		switch(type_) {
			case FULL: return "FULL";
			case PARTIAL: return "PARTIAL";
			case LARGE_PARTIAL: return "LARGE_PARTIAL";
		}

		return "UNDEFINED";
	}

	void cancel() {
		if (gLog().isEnabled(Log::CONSENSUS))
			gLog().write(Log::CONSENSUS, strprintf("[synchronizationJob]: job cancelled for current %s, next %s.", currentBlock_.toHex(), nextBlock_.toHex()));
		cancelled_ = true;
	}
	void renew() { cancelled_ = false; }
	bool cancelled() { return cancelled_; }

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

	bool setNextBlock(const uint256& block, bool force = false) {
		//
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		if (force || (((type_ == SynchronizationJob::LARGE_PARTIAL || type_ == SynchronizationJob::FULL) &&
					!queuedChunks_.read(block)) || type_ == SynchronizationJob::PARTIAL)) {
			nextBlock_ = block;
			currentBlock_.setNull();
			return true;
		}

		return false;
	}

	void setCurrentBlock(const uint256& block) {
		//
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		currentBlock_ = block;
	}

	void setLastBlock(const uint256& block) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		lastBlock_ = block; 
		nextBlock_.setNull();
		currentBlock_.setNull();
		time_ = getTime(); // timestamp
	}

	void resetNextBlock() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
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
		currentBlock_ = lBlock;
		return lBlock;
	}

	uint256 lastBlock() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return lastBlock_;
	}

	uint64_t lastHeight() {
		return lastHeight_;
	}

	uint256 currentBlock() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return currentBlock_;
	}

	uint256 nextBlockInstant() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return nextBlock_;
	}

	uint256& block() { return block_; }
	uint64_t timestamp() { return time_; }
	uint64_t height() { return height_; }
	uint64_t delta() { return delta_; }

	static SynchronizationJobPtr instance(ISettingsPtr settings, const uint256& chain, uint64_t height, const uint256& block, Type type) {
		return std::make_shared<SynchronizationJob>(settings, chain, height, block, type);
	}
	static SynchronizationJobPtr instance(ISettingsPtr settings, const uint256& chain, const uint256& block, uint64_t delta, Type type) {
		return std::make_shared<SynchronizationJob>(settings, chain, block, delta, type);
	}
	static SynchronizationJobPtr instance(ISettingsPtr settings, const uint256& chain, const uint256& block, const uint256& lastBlock, uint64_t delta, uint64_t lastHeight, Type type) {
		return std::make_shared<SynchronizationJob>(settings, chain, block, lastBlock, delta, lastHeight, type);
	}

	//
	// pending blocks
	void pushPendingBlock(const uint256& block) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		if (!pendingBlocksIndex_.read(block)) {
			pendingBlocks_.write(block); pendingBlocksCount_++;
		}
	}

	void registerPendingBlock(const uint256& block) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		time_ = getTime(); // timestamp
		pendingBlocksIndex_.write(block);
	}

	uint64_t pendingBlocks() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return pendingBlocksCount_;
	}

	uint64_t activeWorkers() {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		return txWorkers_.size();
	}

	uint256 acquireNextPendingBlockJob(IPeerPtr peer) {
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		if (pendingBlocksCount_) {
			db::DbList<uint256>::Iterator lBlock = pendingBlocks_.begin();
			//
			time_ = getTime(); // timestamp
			//
			std::pair<std::map<uint256, Agent>::iterator, bool> lResult = 
				txWorkers_.insert(std::map<uint256, Agent>::value_type(*lBlock, Agent(peer, qbit::getTime())));
			pendingBlocksIndex_.remove(*lBlock);
			pendingBlocks_.remove(lBlock);
			if (pendingBlocksCount_-1 > 0) pendingBlocksCount_--;
			else pendingBlocksCount_ = 0;
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

	uint256 reacquirePendingBlockJob(IPeerPtr peer, uint64_t timeout) {
		//
		boost::unique_lock<boost::mutex> lLock(jobMutex_);

		if (txWorkers_.size()) {
			//
			uint64_t lNow = getTime(); // timestamp
			for (std::map<uint256, Agent>::iterator lWorker = txWorkers_.begin(); lWorker != txWorkers_.end(); lWorker++) {
				//
				if (lNow - lWorker->second.time_ > timeout) {
					//
					time_ = lNow;
					lWorker->second = Agent(peer, lNow);
					return lWorker->first;
				}
			}
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
		return pendingBlocksCount_ || txWorkers_.size();
	}

	void setResync() { resync_ = true; }
	bool resync() { return resync_; }

	void pushChunk(const uint256& block, bool frameExists, bool indexed) {
		//
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		if (!queuedChunks_.read(block)) {
			chunks_.write(Chunk(block, frameExists, indexed));
		}
	}

	uint256 analyzeLinkedChunks() {
		//
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		for (db::DbList<Chunk>::Iterator lChunk = chunks_.begin(); lChunk.valid(); lChunk++) {
			//
			if (!(*lChunk).indexed_) {
				return (*lChunk).block_;
			}
		}

		return uint256();
	}

	void clearChunks() {
		//
		boost::unique_lock<boost::mutex> lLock(jobMutex_);
		closeChunks();
		openChunks();
	}

private:
	boost::mutex jobMutex_;
	uint64_t height_;
	uint64_t delta_;
	uint256 block_;
	uint256 nextBlock_;
	uint256 lastBlock_;
	uint64_t lastHeight_;
	uint256 currentBlock_;
	uint64_t time_;
	std::map<uint64_t, Agent> workers_; // TODO: add timeout & check
	std::map<uint256, Agent> txWorkers_; // TODO: add timeout & check
	//
	Type type_;
	Stage stage_ = HEADER_FEED;
	bool cancelled_ = false;
	bool resync_ = false;
	uint64_t timestamp_;
	uint256 chain_;
	std::string tempspaceBlocks_;
	std::string tempspaceChunks_;
	ISettingsPtr settings_;
	//
	db::DbContainerSpacePtr spaceBlocks_;
	db::DbContainerSpacePtr spaceChunks_;
	db::DbList<uint256> pendingBlocks_;
	db::DbSet<uint256> pendingBlocksIndex_;	
	db::DbList<Chunk> chunks_;
	db::DbSet<uint256> queuedChunks_;
	uint64_t pendingBlocksCount_ = 0;
};

} // qbit

#endif
