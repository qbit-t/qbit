// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZ_FEED_H
#define QBIT_BUZZ_FEED_H

#include "../../ipeerextension.h"
#include "../../ipeermanager.h"
#include "../../ipeer.h"
#include "../../message.h"
#include "../../tinyformat.h"
#include "../../client/handlers.h"

#include <boost/function.hpp>

namespace qbit {

// forward
class BuzzfeedItem;
typedef std::shared_ptr<BuzzfeedItem> BuzzfeedItemPtr;

//
// buzzfeed item
class BuzzfeedItem {
public:
	BuzzfeedItem() {
		buzzId_.setNull();
		buzzChainId_.setNull();
		timestamp_ = 0;
		buzzerId_.setNull();
		buzzerInfoId_.setNull();
		replies_ = 0;
		rebuzzes_ = 0;
		likes_ = 0;
	}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(buzzId_);
		READWRITE(buzzChainId_);
		READWRITE(timestamp_);
		READWRITE(buzzerId_);
		READWRITE(buzzerInfoId_);
		READWRITE(buzzerName_);
		READWRITE(buzzerAlias_);
		READWRITE(buzzBody_);
		READWRITE(replies_);
		READWRITE(rebuzzes_);
		READWRITE(likes_);
		READWRITE(properties_);
	}

	uint256& buzzId() { return buzzId_; }
	void setBuzzId(const uint256& id) { buzzId_ = id; }

	uint256& buzzChainId() { return buzzChainId_; }
	void setBuzzChainId(const uint256& id) { buzzChainId_ = id; }

	uint64_t timestamp() { return timestamp_; }
	void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	uint256& buzzerId() { return buzzerId_; } 
	void setBuzzerId(const uint256& id) { buzzerId_ = id; } 

	uint256& buzzerInfoId() { return buzzerInfoId_; }
	void setBuzzerInfoId(const uint256& id) { buzzerInfoId_ = id; }

	std::string& buzzerName() { return buzzerName_; }
	void setBuzzerName(const std::string& name) { buzzerName_ = name; }

	std::vector<unsigned char>& buzzerAlias() { return buzzerAlias_; }
	std::string buzzerAliasString() { std::string lAlias; lAlias.insert(lAlias.end(), buzzerAlias_.begin(), buzzerAlias_.end()); return lAlias; }
	void setBuzzerAlias(const std::vector<unsigned char>& alias) { buzzerAlias_ = alias; }

	std::vector<unsigned char>& buzzBody() { return buzzBody_; }
	std::string buzzBodyString() { std::string lBody; lBody.insert(lBody.end(), buzzBody_.begin(), buzzBody_.end()); return lBody; }
	void setBuzzBody(const std::vector<unsigned char>& body) { buzzBody_ = body; }

	uint32_t replies() { return replies_; }
	void setReplies(uint32_t v) { replies_ = v; }

	uint32_t rebuzzes() { return rebuzzes_; }
	void setRebuzzes(uint32_t v) { rebuzzes_ = v; }

	uint32_t likes() { return likes_; }
	void setLikes(uint32_t v) { likes_ = v; }

	static BuzzfeedItemPtr instance(const BuzzfeedItem& item) {
		return std::make_shared<BuzzfeedItem>(item);
	}

	std::string toString() {
		return strprintf("%s\n%s | %s | %s\n%s\n[%d/%d/%d]",
			buzzId_.toHex(),
			buzzerAliasString(),
			buzzerName_,
			formatISO8601DateTime(timestamp_ / 1000000),
			buzzBodyString(),
			replies_, rebuzzes_, likes_);
	}

private:
	uint256 buzzId_;
	uint256 buzzChainId_;
	uint64_t timestamp_;
	uint256 buzzerId_;
	uint256 buzzerInfoId_;
	std::string buzzerName_;
	std::vector<unsigned char> buzzerAlias_;
	std::vector<unsigned char> buzzBody_;
	uint32_t replies_;
	uint32_t rebuzzes_;
	uint32_t likes_;
	std::vector<unsigned char> properties_; // extra properties for the future use
};

//
// buzzfeed callbacks
typedef boost::function<void (void)> buzzfeedLargeUpdatedFunction;
typedef boost::function<void (BuzzfeedItemPtr)> buzzfeedItemUpdatedFunction;

class Buzzfeed;
typedef std::shared_ptr<Buzzfeed> BuzzfeedPtr;

//
// buzzfeed
class Buzzfeed {
public:
	Buzzfeed(buzzfeedLargeUpdatedFunction largeUpdated, buzzfeedItemUpdatedFunction itemUpdated) : largeUpdated_(largeUpdated), itemUpdated_(itemUpdated) {}

	void merge(const BuzzfeedItem& buzz) {
		//
		BuzzfeedItemPtr lBuzz = BuzzfeedItem::instance(buzz);
		{
			boost::unique_lock<boost::recursive_mutex> lLock(mutex_);

			// push buzz
			items_.insert(std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::value_type(lBuzz->buzzId(), lBuzz));

			// clean-up
			std::pair<std::multimap<uint64_t /*timestamp*/, uint256 /*buzz*/>::iterator,
						std::multimap<uint64_t /*timestamp*/, uint256 /*buzz*/>::iterator> lRange = index_.equal_range(lBuzz->timestamp());
			for (std::multimap<uint64_t /*timestamp*/, uint256 /*buzz*/>::iterator lExist = lRange.first; lExist != lRange.second; lExist++) {
				if (lExist->second == lBuzz->buzzId()) {
					index_.erase(lExist);
					break;
				}
			}

			index_.insert(std::multimap<uint64_t /*timestamp*/, uint256 /*buzz*/>::value_type(lBuzz->timestamp(), lBuzz->buzzId()));
		}

		itemUpdated_(lBuzz);
	}	

	void merge(const std::vector<BuzzfeedItem>& chunk, bool notify = false) {
		//
		{
			boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
			for (std::vector<BuzzfeedItem>::iterator lItem = const_cast<std::vector<BuzzfeedItem>&>(chunk).begin(); 
													lItem != const_cast<std::vector<BuzzfeedItem>&>(chunk).end(); lItem++) {

				items_.insert(std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::value_type(lItem->buzzId(), BuzzfeedItem::instance(*lItem)));

				// clean-up
				std::pair<std::multimap<uint64_t /*timestamp*/, uint256 /*buzz*/>::iterator,
							std::multimap<uint64_t /*timestamp*/, uint256 /*buzz*/>::iterator> lRange = index_.equal_range(lItem->timestamp());
				for (std::multimap<uint64_t /*timestamp*/, uint256 /*buzz*/>::iterator lExist = lRange.first; lExist != lRange.second; lExist++) {
					if (lExist->second == lItem->buzzId()) {
						index_.erase(lExist);
						break;
					}
				}

				index_.insert(std::multimap<uint64_t /*timestamp*/, uint256 /*buzz*/>::value_type(lItem->timestamp(), lItem->buzzId()));
			}
		}

		if (notify) largeUpdated_();
	}

	void feed(std::list<BuzzfeedItemPtr>& feed) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		for (std::multimap<uint64_t /*timestamp*/, uint256 /*buzz*/>::reverse_iterator lItem = index_.rbegin(); 
												lItem != index_.rend(); lItem++) {
			//
			std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
			if (lBuzz != items_.end())
				feed.push_back(lBuzz->second);
		}
	}

	static BuzzfeedPtr instance(buzzfeedLargeUpdatedFunction largeUpdated, buzzfeedItemUpdatedFunction itemUpdated) {
		return std::make_shared<Buzzfeed>(largeUpdated, itemUpdated);
	}

private:
	buzzfeedLargeUpdatedFunction largeUpdated_;
	buzzfeedItemUpdatedFunction itemUpdated_;
	std::map<uint256 /*buzz*/, BuzzfeedItemPtr> items_;
	std::multimap<uint64_t /*timestamp*/, uint256 /*buzz*/> index_;
	boost::recursive_mutex mutex_;
};

//
// select buzzfeed handler
class ISelectBuzzFeedHandler: public IReplyHandler {
public:
	ISelectBuzzFeedHandler() {}
	virtual void handleReply(const std::vector<BuzzfeedItem>&, const uint256&) = 0;
};
typedef std::shared_ptr<ISelectBuzzFeedHandler> ISelectBuzzFeedHandlerPtr;

// special callbacks
typedef boost::function<void (const std::vector<BuzzfeedItem>&, const uint256&)> buzzfeedLoadedFunction;

// load transaction
class SelectBuzzFeed: public ISelectBuzzFeedHandler, public std::enable_shared_from_this<SelectBuzzFeed> {
public:
	SelectBuzzFeed(buzzfeedLoadedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ISelectBuzzFeedHandler
	void handleReply(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
		 function_(feed, chain);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ISelectBuzzFeedHandlerPtr instance(buzzfeedLoadedFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectBuzzFeed>(function, timeout); 
	}

private:
	buzzfeedLoadedFunction function_;
	timeoutFunction timeout_;
};

} // qbit

#endif
