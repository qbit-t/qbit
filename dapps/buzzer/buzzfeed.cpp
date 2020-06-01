#include "buzzfeed.h"

using namespace qbit;

void BuzzfeedItem::merge(const BuzzfeedItem::Update& update) {
	//
	{
		Guard lLock(this);

		// locate buzz
		std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(update.buzzId());
		if (lBuzz != items_.end()) {
			switch(update.field()) {
				case BuzzfeedItem::Update::LIKES: 
					if (lBuzz->second->likes() <= update.count()) 
						lBuzz->second->setLikes(update.count());
					//else
					//	lBuzz->second->setLikes(lBuzz->second->likes()+1);
					break;
				case BuzzfeedItem::Update::REBUZZES: 
					if (lBuzz->second->rebuzzes() <= update.count()) 
						lBuzz->second->setRebuzzes(update.count()); 
					//else
					//	lBuzz->second->setRebuzzes(lBuzz->second->rebuzzes()+1); // default
					break;
				case BuzzfeedItem::Update::REPLIES: 
					if (lBuzz->second->replies() <= update.count()) 
						lBuzz->second->setReplies(update.count()); 
					//else
					//	lBuzz->second->setReplies(lBuzz->second->replies()+1); // default
					break;
			}

			itemUpdated(lBuzz->second);
		}
	}
}

void BuzzfeedItem::merge(const std::vector<BuzzfeedItem::Update>& updates) {
	//
	Guard lLock(this);

	for (std::vector<BuzzfeedItem::Update>::const_iterator lUpdate = updates.begin(); lUpdate != updates.end(); lUpdate++) {
		merge(*lUpdate);
	}

	itemsUpdated(updates);
}

void BuzzfeedItem::merge(const BuzzfeedItem& buzz, bool checkSize, bool notify) {
	//
	BuzzfeedItemPtr lBuzz = BuzzfeedItem::instance(buzz);
	// set resolver
	lBuzz->buzzerInfoResolve_ = buzzerInfoResolve_;
	lBuzz->buzzerInfo_ = buzzerInfo_;
	// verifier
	lBuzz->verifyPublisher_ = verifyPublisher_;
	lBuzz->resolve();

	// verify signature
	Buzzer::VerificationResult lResult = verifyPublisher_(lBuzz);
	if (lResult == Buzzer::VerificationResult::SUCCESS || lResult == Buzzer::VerificationResult::POSTPONED) {
		//
		Guard lLock(this);

		// check result
		lBuzz->setSignatureVerification(lResult);
		// settings
		lBuzz->key_ = key_;
		lBuzz->sortOrder_ = sortOrder_;

		// push buzz
		std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lExisting = items_.find(lBuzz->buzzId());
		if (lExisting != items_.end()) {
			lExisting->second->setReplies(lBuzz->replies());
			lExisting->second->setRebuzzes(lBuzz->rebuzzes());
			lExisting->second->setLikes(lBuzz->likes());

			if (lExisting->second->type() == TX_BUZZ_LIKE && lBuzz->type() == TX_BUZZ_LIKE) {
				//
				lExisting->second->mergeInfos(lBuzz->infos());
			}

			lBuzz = lExisting->second; // reset 
		}

		removeIndex(lBuzz);

		// orphans
		std::map<uint256 /*buzz*/, std::set<uint256>>::iterator lRoot = orphans_.find(lBuzz->buzzId());
		if (lRoot != orphans_.end()) {
			for (std::set<uint256>::iterator lId = lRoot->second.begin(); lId != lRoot->second.end(); lId++) {
				//
				BuzzfeedItemPtr lChild = items_[*lId];
				if (lChild) {
					removeIndex(lChild);
					items_.erase(lChild->buzzId());

					lBuzz->merge(*lChild, notify);
				}
			}

			orphans_.erase(lRoot);
		}

		// pending
		std::map<uint256 /*originalBuzz*/, std::map<uint256, BuzzfeedItemPtr>>::iterator lPenging = pendings_.find(lBuzz->buzzId());
		if (lPenging != pendings_.end()) {
			for (std::map<uint256, BuzzfeedItemPtr>::iterator lItem = lPenging->second.begin(); lItem != lPenging->second.end(); lItem++) {
				lItem->second->merge(*lBuzz, notify);
			}

			pendings_.erase(lPenging);
		}

		// force load...
		bool lAdd = true;
		if (buzz.type() == TX_BUZZ_REPLY) {
			//
			std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lParent = items_.find(buzz.originalBuzzId());
			if (lParent == items_.end()) {
				if (buzzId_ != buzz.originalBuzzId()) {
					// mark orphan
					orphans_[buzz.originalBuzzId()].insert(lBuzz->buzzId());
					// notify
					itemAbsent(buzz.originalBuzzChainId(), buzz.originalBuzzId());
				}
			} else {
				removeIndex(lParent->second);
				lParent->second->merge(*lBuzz, notify);
				insertIndex(lParent->second);
				lAdd = false;
			}
		} else if (buzz.type() == TX_REBUZZ) {
			// move to pending items
			pendings_[buzz.originalBuzzId()][buzz.buzzId()] = lBuzz; // "parent" or aggregator
		}

		//
		if (buzzId_ == buzz.originalBuzzId()) {
			//
			if (order_ < buzz.timestamp()) order_ = buzz.timestamp();
		}

		if (lAdd) {
			items_.insert(std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::value_type(lBuzz->buzzId(), lBuzz));
			index_.insert(std::multimap<OrderKey /*order*/, uint256 /*buzz*/>::value_type(lBuzz->order(), lBuzz->buzzId()));

			updateTimestamp(buzz.buzzerId(), buzz.timestamp());

			if (checkSize && items_.size() > 300 /*setup*/ && lResult == Buzzer::VerificationResult::SUCCESS) {
				//
				std::multimap<OrderKey /*order*/, uint256 /*buzz*/>::iterator lFirst = index_.begin();
				items_.erase(lFirst->second);
				index_.erase(lFirst);
			}
		}

		if (notify) itemNew(lBuzz);
	} else {
		std::cout << "[ERROR]: " << lBuzz->toString() << "\n";
	}
}	

void BuzzfeedItem::merge(const std::vector<BuzzfeedItem>& chunk, bool notify) {
	//
	{
		Guard lLock(this);
		//
		uint256 lChain;
		// control
		bool lChainExists = false; 
		if (chunk.size()) {
			lChainExists = !(chains_.insert((lChain = chunk.begin()->buzzChainId())).second); // one chunk = one chain
		}

		std::map<uint256, std::vector<BuzzfeedItem>::iterator> lFeed;
		for (std::vector<BuzzfeedItem>::iterator lItem = const_cast<std::vector<BuzzfeedItem>&>(chunk).begin(); 
												lItem != const_cast<std::vector<BuzzfeedItem>&>(chunk).end(); lItem++) {
			// preserve for backtrace
			if (merge_ == Merge::INTERSECT) lFeed[lItem->buzzId()] = lItem;

			// process merge
			bool lMerge = true;
			if (lChainExists) {
				//
				std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lExisting = items_.find(lItem->buzzId());
				if (lExisting == items_.end()) {
					if (merge_ == Merge::UNION) {
						lMerge = true;
					} else if (merge_ == Merge::INTERSECT) {
						lMerge = false;
					}
				}
			}

			if (lMerge)
				merge(*lItem, false, false);
		}

		// backtracing
		if (merge_ == Merge::INTERSECT && lFeed.size()) {
			//
			std::map<uint256, BuzzfeedItemPtr> lToRemove;
			for (std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lItem = items_.begin(); lItem != items_.end(); lItem++) {
				if (lItem->second->buzzChainId() == lChain) {
					//
					if (lFeed.find(lItem->second->buzzId()) == lFeed.end()) {
						// missing
						lToRemove[lItem->second->buzzId()] = lItem->second;
					}
				}
			}

			// erase items
			for (std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lRemove = lToRemove.begin(); lRemove != lToRemove.end(); lRemove++) {
				//
				removeIndex(lRemove->second);
				items_.erase(lRemove->second->buzzId());
			}
		}
	}

	if (notify) largeUpdated();
}

void BuzzfeedItem::mergeAppend(const std::list<BuzzfeedItemPtr>& items) {
	//
	for (std::list<BuzzfeedItemPtr>::const_iterator lItem = items.begin(); lItem != items.end(); lItem++) {
		//
		std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lExisting = items_.find((*lItem)->buzzId());
		if (lExisting == items_.end()) {
			items_.insert(std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::value_type((*lItem)->buzzId(), *lItem));
			index_.insert(std::multimap<OrderKey /*order*/, uint256 /*buzz*/>::value_type((*lItem)->order(), (*lItem)->buzzId()));

			updateTimestamp((*lItem)->buzzerId(), (*lItem)->timestamp());

			if (items_.size() > 300 /*setup*/) {
				//
				std::multimap<OrderKey /*order*/, uint256 /*buzz*/>::iterator lFirst = index_.begin();
				items_.erase(lFirst->second);
				index_.erase(lFirst);
			}
		}
	}
}

void BuzzfeedItem::feed(std::list<BuzzfeedItemPtr>& feed) {
	//
	if (type_ == TX_REBUZZ) return;

	//
	Guard lLock(this);
	if (sortOrder_ == Order::REVERSE) {
		for (std::multimap<OrderKey /*order*/, uint256 /*buzz*/>::reverse_iterator lItem = index_.rbegin(); 
												lItem != index_.rend(); lItem++) {
			//
			std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
			// if exists and VALID (signature chacked)
			if (lBuzz != items_.end())
				if (lBuzz->second->valid()) 
					feed.push_back(lBuzz->second);
				else
					std::cout << "[FEED-ERROR]: " << lBuzz->second->toString() << "\n";

		}
	} else {
		for (std::multimap<OrderKey /*order*/, uint256 /*buzz*/>::iterator lItem = index_.begin(); 
												lItem != index_.end(); lItem++) {
			//
			std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
			// if exists and VALID (signature chacked)
			if (lBuzz != items_.end())
				if (lBuzz->second->valid()) 
					feed.push_back(lBuzz->second);
				else
					std::cout << "[FEED-ERROR]: " << lBuzz->second->toString() << "\n";

		}		
	}
}

BuzzfeedItemPtr BuzzfeedItem::locateBuzz(const uint256& buzz) {
	//
	Guard lLock(this);
	std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(buzz);
	if (lBuzz != items_.end()) return lBuzz->second;
	return nullptr;
}

uint64_t BuzzfeedItem::locateLastTimestamp() {
	//
	Guard lLock(this);
	std::set<uint64_t> lTimes;
	for (std::map<uint256 /*publisher*/, uint64_t>::iterator lTimestamp = lastTimestamps_.begin(); lTimestamp != lastTimestamps_.end(); lTimestamp++) {
		lTimes.insert(lTimestamp->second);
	}

	return lTimes.size() ? *lTimes.rbegin() : 0;
}

void BuzzfeedItem::collectPendingItems(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>& items) {
	//
	for (std::map<uint256 /*originalBuzz*/, std::map<uint256, BuzzfeedItemPtr>>::iterator lPending = pendings_.begin(); lPending != pendings_.end(); lPending++) {
		for (std::map<uint256, BuzzfeedItemPtr>::iterator lItem = lPending->second.begin(); lItem != lPending->second.end(); lItem++)
			items[lItem->second->originalBuzzChainId()].insert(lItem->second->originalBuzzId());
	}
}

void BuzzfeedItem::crossMerge() {
	//
	std::map<uint256 /*originalBuzz*/, std::map<uint256, BuzzfeedItemPtr>> lPendings = pendings_;
	for (std::map<uint256 /*originalBuzz*/, std::map<uint256, BuzzfeedItemPtr>>::iterator lPending = lPendings.begin(); lPending != lPendings.end(); lPending++) {
		//
		std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lParent = items_.find(lPending->first);
		if (lParent != items_.end()) {
			for (std::map<uint256, BuzzfeedItemPtr>::iterator lItem = lPending->second.begin(); lItem != lPending->second.end(); lItem++) {
				lItem->second->merge(*(lParent->second), true/*?*/);
			}

			pendings_.erase(lPending->first);
		}
	}
}

