#include "eventsfeed.h"

using namespace qbit;

void EventsfeedItem::merge(const EventsfeedItem& buzz, bool checkSize, bool notify) {
	//
	EventsfeedItemPtr lBuzz = EventsfeedItem::instance(buzz);
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

		// push buzz
		std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lExisting = items_.find(lBuzz->key());
		if (lExisting != items_.end()) {
			removeIndex(lExisting->second);

			lExisting->second->setOrder(lBuzz->timestamp());
			lExisting->second->buzzerInfoMerge(lBuzz->buzzers());
			lBuzz = lExisting->second; // reset 
		} else {
			items_.insert(std::map<Key /*buzz*/, EventsfeedItemPtr>::value_type(lBuzz->key(), lBuzz));
		}

		index_.insert(std::multimap<uint64_t /*order*/, Key /*buzz*/>::value_type(lBuzz->order(), lBuzz->key()));

		for (std::vector<EventInfo>::const_iterator lInfo = lBuzz->buzzers().begin(); lInfo != lBuzz->buzzers().end(); lInfo++) {
			updateTimestamp(lInfo->buzzerId(), lInfo->timestamp());
		}

		if (lBuzz->type() == TX_REBUZZ /*&& !lBuzz->buzzBody().size()*/) {
			// 
			pendings_[lBuzz->buzzId()].insert(lBuzz->key());
		}

		if (checkSize && items_.size() > 300 /*setup*/) {
			//
			std::multimap<uint64_t /*order*/, Key /*buzz*/>::iterator lFirst = index_.begin();
			items_.erase(lFirst->second);
			index_.erase(lFirst);
		}

		if (notify) itemNew(lBuzz);
	} else {
		std::cout << "[ERROR]: " << lBuzz->toString() << "\n";
	}
}	

void EventsfeedItem::merge(const std::vector<BuzzfeedItem>& buzzes) {
	//
	Guard lLock(this);
	std::map<uint256 /*buzz*/, std::set<Key> /*buzz keys*/> lPendings = pendings_;
	//
	for(std::vector<BuzzfeedItem>::const_iterator lBuzz = buzzes.begin(); lBuzz != buzzes.end(); lBuzz++) {
		//
		std::map<uint256 /*buzz*/, std::set<Key> /*buzz keys*/>::iterator lBuzzPtr = lPendings.find(lBuzz->buzzId());
		if (lBuzzPtr != lPendings.end()) {
			//
			for (std::set<Key>::iterator lEventKey = lBuzzPtr->second.begin(); lEventKey != lBuzzPtr->second.end(); lEventKey++) {
				std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lEvent = items_.find(*lEventKey);
				if (lEvent != items_.end()) {
					lEvent->second->setBuzzBody(lBuzz->buzzBody());
					lEvent->second->setBuzzMedia(lBuzz->buzzMedia());
					lEvent->second->setSignature(lBuzz->signature());
				}
			}

			pendings_.erase(lBuzzPtr->first);
		}
	}
}

void EventsfeedItem::mergeUpdate(const std::vector<EventsfeedItem>& chunk) {
	//
	{
		Guard lLock(this);
		for (std::vector<EventsfeedItem>::iterator lItem = const_cast<std::vector<EventsfeedItem>&>(chunk).begin(); 
												lItem != const_cast<std::vector<EventsfeedItem>&>(chunk).end(); lItem++) {
			merge(*lItem, true, true);
		}
	}
}

void EventsfeedItem::merge(const std::vector<EventsfeedItem>& chunk, bool notify) {
	//
	{
		Guard lLock(this);
		uint256 lChain;
		bool lChainExists = false;
		if (chunk.size()) {
			// control
			lChainExists = !(chains_.insert((lChain = chunk.begin()->buzzChainId())).second); // one chunk = one chain
		}

		std::map<Key, std::vector<EventsfeedItem>::iterator> lFeed;
		for (std::vector<EventsfeedItem>::iterator lItem = const_cast<std::vector<EventsfeedItem>&>(chunk).begin(); 
												lItem != const_cast<std::vector<EventsfeedItem>&>(chunk).end(); lItem++) {
			// preserve for backtrace
			if (merge_ == Merge::INTERSECT) lFeed[lItem->key()] = lItem;

			// process merge
			bool lMerge = true;
			if (lChainExists) {
				//
				std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lExisting = items_.find(lItem->key());
				if (lExisting == items_.end()) {
					if (merge_ == Merge::UNION) {
						lMerge = true;
					} else if (merge_ == Merge::INTERSECT) {
						lMerge = false;
					}
				}
			}

			if (lMerge)
				merge(*lItem, true, false);
		}

		// backtracing
		if (merge_ == Merge::INTERSECT && lFeed.size()) {
			//
			std::map<Key, EventsfeedItemPtr> lToRemove;
			for (std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lItem = items_.begin(); lItem != items_.end(); lItem++) {
				if (lItem->second->buzzChainId() == lChain) {
					//
					if (lFeed.find(lItem->second->key()) == lFeed.end()) {
						// missing
						lToRemove[lItem->second->key()] = lItem->second;
					}
				}
			}

			// erase items
			for (std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lRemove = lToRemove.begin(); lRemove != lToRemove.end(); lRemove++) {
				//
				removeIndex(lRemove->second);
				items_.erase(lRemove->second->key());
			}
		}
	}

	if (notify) largeUpdated();
}

void EventsfeedItem::mergeAppend(const std::list<EventsfeedItemPtr>& items) {
	//
	for (std::list<EventsfeedItemPtr>::const_iterator lItem = items.begin(); lItem != items.end(); lItem++) {
		//
		std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lExisting = items_.find((*lItem)->key());
		if (lExisting == items_.end()) {
			//
			items_.insert(std::map<Key /*buzz*/, EventsfeedItemPtr>::value_type((*lItem)->key(), (*lItem)));
			index_.insert(std::multimap<uint64_t /*order*/, Key /*buzz*/>::value_type((*lItem)->order(), (*lItem)->key()));

			for (std::vector<EventInfo>::const_iterator lInfo = (*lItem)->buzzers().begin(); lInfo != (*lItem)->buzzers().end(); lInfo++) {
				updateTimestamp(lInfo->buzzerId(), lInfo->timestamp());
			}

			if (items_.size() > 300 /*setup*/) {
				//
				std::multimap<uint64_t /*order*/, Key /*buzz*/>::iterator lFirst = index_.begin();
				items_.erase(lFirst->second);
				index_.erase(lFirst);
			}
		}
	}
}

void EventsfeedItem::feed(std::list<EventsfeedItemPtr>& feed) {
	//
	if (type_ == TX_REBUZZ) return;

	//
	Guard lLock(this);
	for (std::multimap<uint64_t /*order*/, Key /*buzz*/>::reverse_iterator lItem = index_.rbegin(); 
											lItem != index_.rend(); lItem++) {
		//
		std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
		if (lBuzz != items_.end())
			if (lBuzz->second->valid()) 
				feed.push_back(lBuzz->second);
			else
				std::cout << "[FEED-ERROR]: " << lBuzz->second->toString() << "\n";
	}
}

uint64_t EventsfeedItem::locateLastTimestamp() {
	//
	Guard lLock(this);
	std::set<uint64_t> lTimes;
	for (std::map<uint256 /*publisher*/, uint64_t>::iterator lTimestamp = lastTimestamps_.begin(); lTimestamp != lastTimestamps_.end(); lTimestamp++) {
		lTimes.insert(lTimestamp->second);
	}

	return lTimes.size() ? *lTimes.rbegin() : 0;
}

void EventsfeedItem::collectPendingItems(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>& items) {
	//
	Guard lLock(this);
	for (std::map<uint256 /*originalBuzz*/, std::set<Key>>::iterator lPending = pendings_.begin(); lPending != pendings_.end(); lPending++) {
		for (std::set<Key>::iterator lItem = lPending->second.begin(); lItem != lPending->second.end(); lItem++) {
			EventsfeedItemPtr lEvent = items_[*lItem];
			if (lEvent)
				items[lEvent->buzzChainId()].insert(lEvent->buzzId());
		}
	}
}
