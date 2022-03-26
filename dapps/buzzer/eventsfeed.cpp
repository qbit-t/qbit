#include "eventsfeed.h"

using namespace qbit;

void EventsfeedItem::push(const EventsfeedItem& buzz, const uint160& peer) {
	// put into unconfirmed
	Guard lLock(this);
	std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lItem = unconfirmed_.find(buzz.key());
	if (lItem == unconfirmed_.end()) { // absent
		//
		EventsfeedItemPtr lBuzz = EventsfeedItem::instance(buzz);
		lBuzz->addConfirmation(peer);
		unconfirmed_[buzz.key()] = lBuzz;
	} else {
		if (lItem->second->addConfirmation(peer) >= BUZZ_PEERS_CONFIRMATIONS) {
			//
			EventsfeedItemPtr lBuzz = lItem->second;
			// TODO: remove from unconfirmed?
			//unconfirmed_.erase(lItem);

			// merge finally
			mergeInternal(lBuzz, true, true);

			// resolve info
			if (buzzer()) { 
				buzzer()->resolvePendingEventsItems();
				buzzer()->resolveBuzzerInfos();
			} else {
				std::cout << "[PUSH-ERROR]: Buzzer not found" << "\n";
				if (gLog().isEnabled(Log::CLIENT))
					gLog().write(Log::CLIENT, "[PUSH-ERROR]: Buzzer not found");
			}
		}
	}
}

void EventsfeedItem::merge(const EventsfeedItem& buzz, bool checkSize, bool notify) {
	//
	EventsfeedItemPtr lBuzz = EventsfeedItem::instance(buzz);
	mergeInternal(lBuzz, checkSize, notify);
}

void EventsfeedItem::mergeInternal(EventsfeedItemPtr buzz, bool checkSize, bool notify) {
	//
	EventsfeedItemPtr lBuzz = buzz;
	// set resolver
	lBuzz->buzzerInfoResolve_ = buzzerInfoResolve_;
	lBuzz->buzzerInfo_ = buzzerInfo_;
	// verifier
	lBuzz->verifyPublisher_ = verifyPublisher_;
	lBuzz->verifySource_ = verifySource_;
	// parent
	lBuzz->parent_ = parent();

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

			index_.insert(std::multimap<uint64_t /*order*/, Key /*buzz*/>::value_type(lBuzz->order(), lBuzz->key()));
			for (std::vector<EventInfo>::const_iterator lInfo = lBuzz->buzzers().begin(); lInfo != lBuzz->buzzers().end(); lInfo++) {
				updateTimestamp(lInfo->buzzerId(), lInfo->timestamp());
			}

			// TODO: updates notification
			if (lExisting->second->resolve()) {
				// if all buzz infos are already resolved - just notify
				if (notify)
					itemNew(lExisting->second);
			}

			return;
		} else {
			items_.insert(std::map<Key /*buzz*/, EventsfeedItemPtr>::value_type(lBuzz->key(), lBuzz));
		}

		index_.insert(std::multimap<uint64_t /*order*/, Key /*buzz*/>::value_type(lBuzz->order(), lBuzz->key()));
		for (std::vector<EventInfo>::const_iterator lInfo = lBuzz->buzzers().begin(); lInfo != lBuzz->buzzers().end(); lInfo++) {
			updateTimestamp(lInfo->buzzerId(), lInfo->timestamp());
		}

		if (lBuzz->type() == TX_REBUZZ || lBuzz->type() == TX_REBUZZ_REPLY || 
								lBuzz->type() == TX_BUZZ_LIKE || lBuzz->type() == TX_BUZZ_REWARD) {
			// 
			pendings_[lBuzz->buzzId()].insert(lBuzz->key());
		}

		if (checkSize && items_.size() > 300 /*setup*/) {
			// TODO: do we need to remove?
			//std::multimap<uint64_t /*order*/, Key /*buzz*/>::iterator lFirst = index_.begin();
			//items_.erase(lFirst->second);
			//index_.erase(lFirst);
		}

		if (lBuzz->resolve()) {
			// if all buzz infos are already resolved - just notify
			if (notify)
				itemNew(lBuzz);
		}
	} else {
		std::cout << "[EVENTS-ERROR]: " << lBuzz->toString() << "\n";
		if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[EVENTS-ERROR]: %s", lBuzz->toString()));
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
					BuzzfeedItemPtr lItem = BuzzfeedItem::instance(*lBuzz);
					lItem->setVerifyPublisher(verifySource_);
					if (lItem->valid()) {
						lEvent->second->setTimestamp(lBuzz->timestamp());
						lEvent->second->setScore(lBuzz->score());
						lEvent->second->setPublisher(lBuzz->buzzerId());
						lEvent->second->setPublisherInfo(lBuzz->buzzerInfoId());
						lEvent->second->setPublisherInfoChain(lBuzz->buzzerInfoChainId());
						lEvent->second->setBuzzBody(lBuzz->buzzBody());
						lEvent->second->setBuzzMedia(lBuzz->buzzMedia());
						lEvent->second->setSignature(lBuzz->signature());

						std::string lBuzzerName;
						std::string lBuzzerAlias;
						buzzerInfo_(lBuzz->buzzerInfoId(), lBuzzerName, lBuzzerAlias);

						if (lEvent->second->resolve()) {
							// if all buzz infos are already resolved - just notify
							itemNew(lEvent->second);
						}
					} else {
						std::cout << "[MERGE-ERROR]: " << lItem->toString() << "\n";
						if (gLog().isEnabled(Log::CLIENT))
							gLog().write(Log::CLIENT, strprintf("[MERGE-ERROR]: %s", lItem->toString()));
					}
				}
			}

			pendings_.erase(lBuzzPtr->first);
		}
	}
}

void EventsfeedItem::mergeUpdate(const std::vector<EventsfeedItem>& chunk, const uint160& peer) {
	//
	{
		Guard lLock(this);
		for (std::vector<EventsfeedItem>::iterator lItem = const_cast<std::vector<EventsfeedItem>&>(chunk).begin(); 
												lItem != const_cast<std::vector<EventsfeedItem>&>(chunk).end(); lItem++) {
			push(*lItem, peer);
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

bool EventsfeedItem::mergeAppend(const std::vector<EventsfeedItemPtr>& items) {
	//
	bool lAdded = false;
	for (std::vector<EventsfeedItemPtr>::const_iterator lItem = items.begin(); lItem != items.end(); lItem++) {
		//
		std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lExisting = items_.find((*lItem)->key());
		if (lExisting == items_.end()) {
			//
			lAdded |= items_.insert(std::map<Key /*buzz*/, EventsfeedItemPtr>::value_type((*lItem)->key(), (*lItem))).second;
			index_.insert(std::multimap<uint64_t /*order*/, Key /*buzz*/>::value_type((*lItem)->order(), (*lItem)->key()));

			for (std::vector<EventInfo>::const_iterator lInfo = (*lItem)->buzzers().begin(); lInfo != (*lItem)->buzzers().end(); lInfo++) {
				updateTimestamp(lInfo->buzzerId(), lInfo->timestamp());
			}

			if (items_.size() > 300 /*setup*/) {
				// TODO: do we need to remove?
				// std::multimap<uint64_t /*order*/, Key /*buzz*/>::iterator lFirst = index_.begin();
				// items_.erase(lFirst->second);
				// index_.erase(lFirst);
			}
		}
	}

	return lAdded;
}

void EventsfeedItem::feed(std::vector<EventsfeedItemPtr>& feed) {
	//
	Guard lLock(this);
	for (std::multimap<uint64_t /*order*/, Key /*buzz*/>::reverse_iterator lItem = index_.rbegin(); 
											lItem != index_.rend(); lItem++) {
		//
		std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
		if (lBuzz != items_.end()) {
			if (lBuzz->second->valid()) {
				feed.push_back(lBuzz->second);
			} else {
				std::cout << "[FEED-ERROR]: " << lBuzz->second->toString() << "\n";
				if (gLog().isEnabled(Log::CLIENT))
					gLog().write(Log::CLIENT, strprintf("[FEED-ERROR]: %s", lBuzz->second->toString()));
			}
		}
	}

	setFed();
}

int EventsfeedItem::locateIndex(EventsfeedItemPtr item) {
	//
	Guard lLock(this);
	//
	int lIndex = 0;
	bool lFound = false;
	for (std::multimap<uint64_t /*order*/, Key /*buzz*/>::reverse_iterator lItem = index_.rbegin();
											lItem != index_.rend(); lItem++, lIndex++) {
		//
		std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
		if (lBuzz != items_.end() && lBuzz->second->key() == item->key()) {
			lFound = true;
			break;
		}
	}

	return lFound ? lIndex : -1;
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

EventsfeedItemPtr EventsfeedItem::locateItem(const Key& key) {
	//
	Guard lLock(this);
	std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lItem = items_.find(key);
	if (lItem != items_.end()) {
		return lItem->second;
	}

	return nullptr;
}
