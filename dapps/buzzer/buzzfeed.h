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

#include "txbuzzer.h"

#include <boost/function.hpp>

namespace qbit {

// forward
class BuzzfeedItem;
typedef std::shared_ptr<BuzzfeedItem> BuzzfeedItemPtr;

//
// buzzfeed item
class BuzzfeedItem {
public:
	class Alias {
	public:
		Alias() {}
		Alias(const std::vector<unsigned char>& alias): alias_(alias) {}
		Alias(const std::string& alias) { alias_.insert(alias_.end(), alias.begin(), alias.end()); }

		std::string data() {
			std::string lAlias;
			lAlias.insert(lAlias.end(), alias_.begin(), alias_.end());
			return lAlias;
		}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(alias_);
		}

	private:
		std::vector<unsigned char> alias_;		
	};

	class Update {
	public:
		enum Field {
			NONE = 0,
			LIKES = 1,
			REBUZZES = 2,
			REPLIES = 3
		};
	public:
		Update() { field_ = NONE; count_ = 0; }
		Update(const uint256& buzzId, Field field, uint32_t count): buzzId_(buzzId_), field_(field), count_(count) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			//
			READWRITE(buzzId_);

			if (ser_action.ForRead()) {
				short lField;
				s >> lField;
				field_ = (Field)lField;
			} else {
				short lField = (short)field_;
				s << lField;			
			}

			READWRITE(count_);
		}

		const uint256& buzzId() const { return buzzId_; }
		Field field() const { return field_; }
		std::string fieldString() const {
			if (field_ == LIKES) return "LIKES";
			else if (field_ == REBUZZES) return "REBUZZES";
			else if (field_ == REPLIES) return "REPLIES";
			return "NONE";
		}
		uint32_t count() const { return count_; }

	private:
		uint256 buzzId_;
		Field field_;
		uint32_t count_;
	};
public:
	BuzzfeedItem() {
		type_ = Transaction::UNDEFINED;
		buzzId_.setNull();
		buzzChainId_.setNull();
		timestamp_ = 0;
		order_ = 0;
		buzzerId_.setNull();
		buzzerInfoId_.setNull();
		replies_ = 0;
		rebuzzes_ = 0;
		likes_ = 0;
		originalBuzzId_ = 0;
	}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(type_);
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

		if (type_ == TX_BUZZ_REPLY || type_ == TX_REBUZZ)
			READWRITE(originalBuzzId_);

		if (type_ == TX_BUZZ_LIKE)
			READWRITE(aliases_);			

		READWRITE(properties_);

		if (ser_action.ForRead()) {
			order_ = timestamp_;
		}
	}

	uint64_t order() const { return order_; }

	unsigned short type() const { return type_; }
	void setType(unsigned short type) { type_ = type; }

	const uint256& buzzId() const { return buzzId_; }
	void setBuzzId(const uint256& id) { buzzId_ = id; }

	const uint256& originalBuzzId() const { return originalBuzzId_; }
	void setOriginalBuzzId(const uint256& id) { originalBuzzId_ = id; }

	void addAlias(const Alias& alias) {
		aliases_.push_back(alias);
	}

	const std::vector<Alias>& aliases() const { return aliases_; }

	const uint256& buzzChainId() const { return buzzChainId_; }
	void setBuzzChainId(const uint256& id) { buzzChainId_ = id; }

	uint64_t timestamp() const { return timestamp_; }
	void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	const uint256& buzzerId() const { return buzzerId_; } 
	void setBuzzerId(const uint256& id) { buzzerId_ = id; } 

	const uint256& buzzerInfoId() const { return buzzerInfoId_; }
	void setBuzzerInfoId(const uint256& id) { buzzerInfoId_ = id; }

	const std::string& buzzerName() const { return buzzerName_; }
	void setBuzzerName(const std::string& name) { buzzerName_ = name; }

	const std::vector<unsigned char>& buzzerAlias() const { return buzzerAlias_; }
	std::string buzzerAliasString() const { std::string lAlias; lAlias.insert(lAlias.end(), buzzerAlias_.begin(), buzzerAlias_.end()); return lAlias; }
	void setBuzzerAlias(const std::vector<unsigned char>& alias) { buzzerAlias_ = alias; }

	std::vector<unsigned char>& buzzBody() { return buzzBody_; }
	std::string buzzBodyString() const { std::string lBody; lBody.insert(lBody.end(), buzzBody_.begin(), buzzBody_.end()); return lBody; }
	void setBuzzBody(const std::vector<unsigned char>& body) { buzzBody_ = body; }

	uint32_t replies() const { return replies_; }
	void setReplies(uint32_t v) { replies_ = v; }

	uint32_t rebuzzes() const { return rebuzzes_; }
	void setRebuzzes(uint32_t v) { rebuzzes_ = v; }

	uint32_t likes() const { return likes_; }
	void setLikes(uint32_t v) { likes_ = v; }

	static BuzzfeedItemPtr instance(const BuzzfeedItem& item) {
		return std::make_shared<BuzzfeedItem>(item);
	}

	void merge(const BuzzfeedItem::Update&);
	void merge(const std::vector<BuzzfeedItem::Update>&);
	void merge(const BuzzfeedItem&, bool checkSize = true, bool notify = true);
	void merge(const std::vector<BuzzfeedItem>&, bool notify = false);

	void feed(std::list<BuzzfeedItemPtr>&);
	BuzzfeedItemPtr locateBuzz(const uint256&);

	std::string typeString() {
		if (type_ == TX_BUZZ) return "bz";
		else if (type_ == TX_REBUZZ) return "rb";
		else if (type_ == TX_BUZZ_REPLY) return "re";
		return "N";
	}

	std::string toString() {
		return strprintf("%s\n%s | %s | %s (%s)\n%s\n[%d/%d/%d]\n -> %s",
			buzzId_.toHex(),
			buzzerAliasString(),
			buzzerName_,
			formatISO8601DateTime(timestamp_ / 1000000),
			typeString(),
			buzzBodyString(),
			replies_, rebuzzes_, likes_,
			originalBuzzId_.isNull() ? "?" : originalBuzzId_.toHex());
	}

	virtual void lock() {}
	virtual void unlock() {}

protected:
	virtual void itemUpdated(BuzzfeedItemPtr) {}
	virtual void itemNew(BuzzfeedItemPtr) {}
	virtual void itemsUpdated(const std::vector<BuzzfeedItem::Update>&) {}
	virtual void largeUpdated() {}
	virtual void itemAbsent(const uint256&, const uint256&) {}

	void removeIndex(BuzzfeedItemPtr item) {
		// clean-up
		std::pair<std::multimap<uint64_t /*order*/, uint256 /*buzz*/>::iterator,
					std::multimap<uint64_t /*order*/, uint256 /*buzz*/>::iterator> lRange = index_.equal_range(item->order());
		for (std::multimap<uint64_t /*order*/, uint256 /*buzz*/>::iterator lExist = lRange.first; lExist != lRange.second; lExist++) {
			if (lExist->second == item->buzzId()) {
				index_.erase(lExist);
				break;
			}
		}
	}

	void insertIndex(BuzzfeedItemPtr item) {
		index_.insert(std::multimap<uint64_t /*order*/, uint256 /*buzz*/>::value_type(item->order(), item->buzzId()));
	}

protected:
	class Guard {
	public:
		Guard(BuzzfeedItem* item): item_(item) {
			item_->lock();
		}

		~Guard() {
			if (item_) item_->unlock();
		}

	private:
		BuzzfeedItem* item_ = 0;
	};

protected:
	unsigned short type_;
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

	// ordering
	uint64_t order_; // timestamp initially

	// reply/rebuzz - extra data
	uint256 originalBuzzId_;

	// likes
	std::vector<Alias> aliases_;

	// threads
	std::map<uint256 /*buzz*/, BuzzfeedItemPtr> items_;
	std::multimap<uint64_t /*order*/, uint256 /*buzz*/> index_;
	std::map<uint256 /*buzz*/, std::set<uint256>> orphans_;
};

//
// buzzfeed callbacks
typedef boost::function<void (void)> buzzfeedLargeUpdatedFunction;
typedef boost::function<void (BuzzfeedItemPtr)> buzzfeedItemNewFunction;
typedef boost::function<void (BuzzfeedItemPtr)> buzzfeedItemUpdatedFunction;
typedef boost::function<void (const std::vector<BuzzfeedItem::Update>&)> buzzfeedItemsUpdatedFunction;
typedef boost::function<void (const uint256&, const uint256&)> buzzfeedItemAbsentFunction;

class Buzzfeed;
typedef std::shared_ptr<Buzzfeed> BuzzfeedPtr;

//
// buzzfeed
class Buzzfeed: public BuzzfeedItem, public std::enable_shared_from_this<Buzzfeed> {
public:
	Buzzfeed(buzzfeedLargeUpdatedFunction largeUpdated, 
								buzzfeedItemNewFunction itemNew, 
								buzzfeedItemUpdatedFunction itemUpdated,
								buzzfeedItemsUpdatedFunction itemsUpdated,
								buzzfeedItemAbsentFunction itemAbsent) : 
		largeUpdated_(largeUpdated), 
		itemNew_(itemNew), 
		itemUpdated_(itemUpdated),
		itemsUpdated_(itemsUpdated),
		itemAbsent_(itemAbsent) {}

	BuzzfeedItemPtr toItem() {
		return std::static_pointer_cast<BuzzfeedItem>(shared_from_this());
	}

	void lock() {
		mutex_.lock();
	}

	void unlock() {
		mutex_.unlock();
	}

	static BuzzfeedPtr instance(buzzfeedLargeUpdatedFunction largeUpdated, 
			buzzfeedItemNewFunction itemNew, 
			buzzfeedItemUpdatedFunction itemUpdated,
			buzzfeedItemsUpdatedFunction itemsUpdated,
			buzzfeedItemAbsentFunction itemAbsent) {
		return std::make_shared<Buzzfeed>(largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent);
	}

protected:
	virtual void itemUpdated(BuzzfeedItemPtr item) { itemUpdated_(item); }
	virtual void itemNew(BuzzfeedItemPtr item) { itemNew_(item); }
	virtual void itemsUpdated(const std::vector<BuzzfeedItem::Update>& items) { itemsUpdated_(items); }
	virtual void largeUpdated() { largeUpdated_(); }
	virtual void itemAbsent(const uint256& chain, const uint256& id) { itemAbsent_(chain, id); }	

private:
	buzzfeedLargeUpdatedFunction largeUpdated_;
	buzzfeedItemNewFunction itemNew_;
	buzzfeedItemUpdatedFunction itemUpdated_;
	buzzfeedItemsUpdatedFunction itemsUpdated_;
	buzzfeedItemAbsentFunction itemAbsent_;

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
