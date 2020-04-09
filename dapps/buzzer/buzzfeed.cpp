#include "buzzfeed.h"

using namespace qbit;

void BuzzfeedItem::merge(const BuzzfeedItem::Update& update) {
	//
	{
		Guard lLock(this);

		// locate buzz
		std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(const_cast<BuzzfeedItem::Update&>(update).buzzId());
		if (lBuzz != items_.end()) {
			switch(const_cast<BuzzfeedItem::Update&>(update).field()) {
				case BuzzfeedItem::Update::LIKES: 
					if (lBuzz->second->likes() < const_cast<BuzzfeedItem::Update&>(update).count()) 
						lBuzz->second->setLikes(const_cast<BuzzfeedItem::Update&>(update).count()); 
					break;
				case BuzzfeedItem::Update::REBUZZES: 
					if (lBuzz->second->rebuzzes() < const_cast<BuzzfeedItem::Update&>(update).count()) 
						lBuzz->second->setRebuzzes(const_cast<BuzzfeedItem::Update&>(update).count()); 
					break;
				case BuzzfeedItem::Update::REPLIES: 
					if (lBuzz->second->replies() < const_cast<BuzzfeedItem::Update&>(update).count()) 
						lBuzz->second->setReplies(const_cast<BuzzfeedItem::Update&>(update).count()); 
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
	{
		Guard lLock(this);

		// push buzz
		std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lExisting = items_.find(lBuzz->buzzId());
		if (lExisting != items_.end()) {
			lExisting->second->setReplies(lBuzz->replies());
			lExisting->second->setRebuzzes(lBuzz->rebuzzes());
			lExisting->second->setLikes(lBuzz->likes());

			lBuzz = lExisting->second; // reset 
		}

		removeIndex(lBuzz);

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

		// force load...
		bool lAdd = true;
		if ((buzz.type() == TX_BUZZ_REPLY || buzz.type() == TX_REBUZZ)) {
			//
			std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lParent = items_.find(buzz.originalBuzzId());
			if (lParent == items_.end()) {
				if (buzzId_ != buzz.originalBuzzId()) {
					// mark orphan
					orphans_[buzz.originalBuzzId()].insert(lBuzz->buzzId());
					// notify
					itemAbsent(buzz.buzzChainId(), buzz.originalBuzzId());
				}
			} else {
				removeIndex(lParent->second);
				lParent->second->merge(*lBuzz, notify);
				insertIndex(lParent->second);
				lAdd = false;
			}
		}

		//
		if (buzzId_ == buzz.originalBuzzId()) {
			//
			if (order_ < buzz.timestamp()) order_ = buzz.timestamp();
		}

		if (lAdd) {
			items_.insert(std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::value_type(lBuzz->buzzId(), lBuzz));
			index_.insert(std::multimap<uint64_t /*order*/, uint256 /*buzz*/>::value_type(lBuzz->order(), lBuzz->buzzId()));

			if (checkSize && items_.size() > 300 /*setup*/) {
				//
				std::multimap<uint64_t /*order*/, uint256 /*buzz*/>::iterator lFirst = index_.begin();
				items_.erase(lFirst->second);
				index_.erase(lFirst);
			}
		}
	}

	if (notify) itemNew(lBuzz);
}	

void BuzzfeedItem::merge(const std::vector<BuzzfeedItem>& chunk, bool notify) {
	//
	{
		Guard lLock(this);
		for (std::vector<BuzzfeedItem>::iterator lItem = const_cast<std::vector<BuzzfeedItem>&>(chunk).begin(); 
												lItem != const_cast<std::vector<BuzzfeedItem>&>(chunk).end(); lItem++) {
			merge(*lItem, false, false);
		}
	}

	if (notify) largeUpdated();
}

void BuzzfeedItem::feed(std::list<BuzzfeedItemPtr>& feed) {
	//
	Guard lLock(this);
	for (std::multimap<uint64_t /*order*/, uint256 /*buzz*/>::reverse_iterator lItem = index_.rbegin(); 
											lItem != index_.rend(); lItem++) {
		//
		std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
		if (lBuzz != items_.end())
			feed.push_back(lBuzz->second);
	}
}

BuzzfeedItemPtr BuzzfeedItem::locateBuzz(const uint256& buzz) {
	//
	Guard lLock(this);
	std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(buzz);
	if (lBuzz != items_.end()) return lBuzz->second;
	return nullptr;
}
