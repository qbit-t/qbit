// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_EVENTS_FEED_H
#define QBIT_EVENTS_FEED_H

#include "../../ipeerextension.h"
#include "../../ipeermanager.h"
#include "../../ipeer.h"
#include "../../message.h"
#include "../../tinyformat.h"
#include "../../client/handlers.h"

#include "txbuzzer.h"
#include "buzzfeed.h"

#include <boost/function.hpp>

namespace qbit {

// forward
class EventsfeedItem;
typedef std::shared_ptr<EventsfeedItem> EventsfeedItemPtr;

//
typedef boost::function<Buzzer::VerificationResult (EventsfeedItemPtr)> eventsfeedItemVerifyFunction;

//
// eventsfeed item
//  
class EventsfeedItem: public std::enable_shared_from_this<EventsfeedItem> {
public:
	enum Merge {
		//
		// UNION - unite feeds, which comes from different nodes
		UNION,
		//
		// INTERSECT - make an intersaction of feeds from different sources
		INTERSECT
	};

public:
	class Key {
	public:
		Key(const uint256& id, unsigned short type): id_(id), type_(type) {}

		friend bool operator < (const Key& a, const Key& b) {
			if (a.id() < b.id()) return true;
			if (a.id() > b.id()) return false;
			if (a.type() < b.type()) return true;
			if (a.type() > b.type()) return false;

			return false;
		}

		friend bool operator == (const Key& a, const Key& b) {
			return (a.id() == b.id() && a.type() == b.type());
		}

		const uint256& id() const { return id_; }
		unsigned short type() const { return type_; }

		std::string toString() const {
			return strprintf("%s|%d", id_.toHex(), type_);
		}

	private:
		uint256 id_;
		unsigned short type_;
	};

public:
	class EventInfo {
	public:
		EventInfo() {
			timestamp_ = 0;
			eventId_.setNull();
			eventChainId_.setNull();
			buzzerId_.setNull();
			buzzerInfoId_.setNull();
			buzzerInfoChainId_.setNull();
		}
		EventInfo(uint64_t timestamp, const uint256& buzzerId, const uint256& buzzerInfoChainId, const uint256& buzzerInfoId): 
			timestamp_(timestamp), buzzerId_(buzzerId), buzzerInfoChainId_(buzzerInfoChainId), buzzerInfoId_(buzzerInfoId) {
			eventId_.setNull();
			eventChainId_.setNull();
		}

		const uint256& buzzerId() const { return buzzerId_; }
		const uint256& buzzerInfoId() const { return buzzerInfoId_; }
		const uint256& buzzerInfoChainId() const { return buzzerInfoChainId_; }

		void setEvent(const uint256& chain, const uint256& event, const uint512& signature) {
			eventId_ = event;
			eventChainId_ = chain;
			signature_ = signature;
		}

		void setEvent(const uint256& chain, const uint256& event, 
						const std::vector<unsigned char>& body, const std::vector<BuzzerMediaPointer>& media, 
						const uint512& signature) {
			eventId_ = event;
			eventChainId_ = chain;
			signature_ = signature;
			body_ = body;
			media_ = media;
		}

		const uint256& eventId() const { return eventId_; } 
		const uint256& eventChainId() const { return eventChainId_; } 

		uint64_t timestamp() const { return timestamp_; }

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(timestamp_);
			READWRITE(eventId_);
			READWRITE(eventChainId_);
			READWRITE(buzzerId_);
			READWRITE(buzzerInfoId_);
			READWRITE(buzzerInfoChainId_);
			READWRITE(body_);
			READWRITE(media_);
			READWRITE(signature_);
		}

	private:
		uint64_t timestamp_;
		uint256 eventId_;
		uint256 eventChainId_;
		uint256 buzzerId_;
		uint256 buzzerInfoId_;
		uint256 buzzerInfoChainId_;
		std::vector<unsigned char> body_;
		std::vector<BuzzerMediaPointer> media_;
		uint512 signature_;
	};

public:
	EventsfeedItem() {
		type_ = Transaction::UNDEFINED;
		timestamp_ = 0;
		buzzId_.setNull();
		buzzChainId_.setNull();
	}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(type_);
		READWRITE(mention_);
		READWRITE(timestamp_);
		READWRITE(buzzId_);
		READWRITE(buzzChainId_);
		READWRITE(buzzBody_);
		READWRITE(buzzMedia_);
		READWRITE(signature_);
		READWRITE(buzzers_);

		if (ser_action.ForRead()) {
			if (!buzzers_.size()) {
				order_ = timestamp_;	
			} else {
				order_ = buzzers_.begin()->timestamp();
			}
		}

		if (type_ == TX_BUZZER_ENDORSE || type_ == TX_BUZZER_MISTRUST) {
			READWRITE(value_);				
		}
	}

	unsigned short type() const { return type_; }
	void setType(unsigned short type) { type_ = type; }

	const uint256& buzzId() const { return buzzId_; }
	void setBuzzId(const uint256& id) { buzzId_ = id; }

	void setMention() { mention_ = 1; }
	bool isMentioned() { return mention_ == 1; }

	Key key() { return Key(buzzId_, type_); }

	void addEventInfo(const EventInfo& info) {
		if (buzzer_.insert(info.buzzerId()).second)
			buzzers_.push_back(info);
	}

	void buzzerInfoMerge(const std::vector<EventInfo>& buzzers) {
		//
		if (buzzers_.size() && !buzzer_.size()) {
			for (std::vector<EventInfo>::iterator lInfo = buzzers_.begin(); lInfo != buzzers_.end(); lInfo++) {
				buzzer_.insert(lInfo->buzzerId());
			}
		}

		//
		if (buzzer_.size() > 50) { buzzerCount_++; return; }
		for (std::vector<EventInfo>::const_iterator lBuzzer = buzzers.begin(); lBuzzer != buzzers.end(); lBuzzer++) {
			//
			addEventInfo(*lBuzzer);

			if (lBuzzer->timestamp() > order_) order_ = lBuzzer->timestamp();
		}
	}

	const std::vector<EventInfo>& buzzers() const { return buzzers_; }

	uint64_t order() { return order_; }
	void setOrder(uint64_t order) { order_ = order; }

	uint64_t value() { return value_; }
	void setValue(uint64_t value) { value_ = value; }

	const uint256& buzzChainId() const { return buzzChainId_; }
	void setBuzzChainId(const uint256& id) { buzzChainId_ = id; }

	uint64_t timestamp() const { return timestamp_; }
	void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	const std::vector<unsigned char>& buzzBody() const { return buzzBody_; }
	std::string buzzBodyString() const { std::string lBody; lBody.insert(lBody.end(), buzzBody_.begin(), buzzBody_.end()); return lBody; }
	void setBuzzBody(const std::vector<unsigned char>& body) { buzzBody_ = body; }

	const uint512& signature() const { return signature_; }
	void setSignature(const uint512& signature) { signature_ = signature; }	

	const std::vector<BuzzerMediaPointer>& buzzMedia() const { return buzzMedia_; } 
	void setBuzzMedia(const std::vector<BuzzerMediaPointer>& media) {
		buzzMedia_.insert(buzzMedia_.end(), media.begin(), media.end());
	}

	static EventsfeedItemPtr instance(const EventsfeedItem& item) {
		return std::make_shared<EventsfeedItem>(item);
	}

	static EventsfeedItemPtr instance() {
		return std::make_shared<EventsfeedItem>();
	}

	void merge(const std::vector<BuzzfeedItem>&);
	void merge(const EventsfeedItem&, bool checkSize = true, bool notify = true);
	void merge(const std::vector<EventsfeedItem>&, bool notify = false);
	void mergeUpdate(const std::vector<EventsfeedItem>&);
	void mergeAppend(const std::list<EventsfeedItemPtr>&);	

	void feed(std::list<EventsfeedItemPtr>&);

	std::string buzzersToString() {
		//
		if (!buzzers_.size()) return "?";

		EventInfo lInfo = *buzzers_.begin();

		std::string lBuzzerName = strprintf("<%s>", lInfo.buzzerId().toHex());
		std::string lBuzzerAlias;
		buzzerInfo(lInfo, lBuzzerName, lBuzzerAlias);

		if (!lBuzzerAlias.size()) lBuzzerAlias = "?";

		if (buzzers_.size() > 1) {
			return strprintf("%s (%s) and %d others are %s", 
				lBuzzerAlias, lBuzzerName, (buzzers_.size()-1) + buzzerCount_, typeString());
		}

		if (type_ == TX_BUZZER_ENDORSE || type_ == TX_BUZZER_MISTRUST) {
			return strprintf("%s (%s) %s on %s", lBuzzerAlias, lBuzzerName, typeString(), 
				strprintf(TxAssetType::scaleFormat(QBIT), (double)value_ / (double)QBIT));
		}

		return strprintf("%s (%s) %s", lBuzzerAlias, lBuzzerName, typeString());
	}

	std::string buzzerFeedInfoString() {
		//
		if (!buzzers_.size()) return "?";

		EventInfo lInfo = *buzzers_.begin();

		std::string lBuzzerName = strprintf("<%s>", lInfo.buzzerId().toHex());
		std::string lBuzzerAlias;
		buzzerInfo(lInfo, lBuzzerName, lBuzzerAlias);

		if (!lBuzzerAlias.size()) lBuzzerAlias = "?";

		if (type_ == TX_BUZZER_ENDORSE || type_ == TX_BUZZER_MISTRUST) {
			return strprintf("%s (%s) %s on %s", lBuzzerAlias, lBuzzerName, typeString(), 
				strprintf(TxAssetType::scaleFormat(QBIT), (double)value_ / (double)QBIT));
		}

		return strprintf("%s (%s)", lBuzzerAlias, lBuzzerName);	
	}

	std::string typeString() {
		if (type_ == TX_BUZZ) return "mentioned you";
		else if (type_ == TX_REBUZZ) { if (mention_) return "mentioned you"; return "rebuzzed your buzz"; }
		else if (type_ == TX_BUZZ_REPLY) return "replied to your buzz";
		else if (type_ == TX_BUZZ_LIKE) return "liked your buzz";
		else if (type_ == TX_BUZZER_SUBSCRIBE) return "started reading you";
		else if (type_ == TX_BUZZER_ENDORSE) return "endorsed you";
		else if (type_ == TX_BUZZER_MISTRUST) return "mistrusted you";
		return "N";
	}

	std::string toString() {
		return strprintf("%s\n%s | %s\n%s",
			buzzId_.toHex(),
			buzzersToString(),
			formatISO8601DateTime(timestamp_ / 1000000),
			buzzBodyString());
	}

	std::string toFeedingString() {
		//
		return strprintf("%s\n%s | %s",
			buzzId_.toHex(),
			buzzerFeedInfoString(),
			formatISO8601DateTime(timestamp_ / 1000000));
	}

	virtual void lock() {}
	virtual void unlock() {}

	virtual uint64_t locateLastTimestamp();
	virtual void collectPendingItems(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>&);

	void setResolver(buzzerInfoResolveFunction buzzerInfoResolve, buzzerInfoFunction buzzerInfo) {
		buzzerInfoResolve_ = buzzerInfoResolve;
		buzzerInfo_ = buzzerInfo;
	}

	void resolve() {
		//
		for (std::vector<EventInfo>::const_iterator lBuzzer = buzzers_.begin(); lBuzzer != buzzers_.end(); lBuzzer++) {
			// just first one for now
			buzzerInfoResolve_(lBuzzer->buzzerInfoChainId(), lBuzzer->buzzerId(), lBuzzer->buzzerInfoId());
			break;
		}
	}

	void buzzerInfo(const EventInfo& info, std::string& name, std::string& alias) {
		buzzerInfo_(info.buzzerInfoId(), name, alias); 
	}

	virtual void clear() {
		items_.clear();
		index_.clear();
		lastTimestamps_.clear();
		pendings_.clear();
		chains_.clear();	
	}

	virtual EventsfeedItemPtr last() {
		Guard lLock(this);
		if (index_.size()) {
			std::multimap<uint64_t /*order*/, Key /*buzz*/>::iterator lItem = index_.begin();
			std::map<Key /*buzz*/, EventsfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
			if (lBuzz != items_.end())
				return lBuzz->second;
		}

		return nullptr;
	}

	uint256 buzzer() const {
		//
		if (buzzers_.size()) {
			std::vector<EventInfo>::const_iterator lItem = buzzers_.begin();
			return lItem->buzzerId();
		}

		return uint256();
	}	

	Buzzer::VerificationResult signatureVerification() { return checkResult_; }

	virtual bool valid() {
		if (checkResult_ == Buzzer::VerificationResult::SUCCESS) return true;
		else if (checkResult_ == Buzzer::VerificationResult::INVALID) return false;

		Buzzer::VerificationResult lResult = verifyPublisher_(shared_from_this());
		return (lResult == Buzzer::VerificationResult::SUCCESS);
	}

	virtual void setSignatureVerification(Buzzer::VerificationResult result) {
		//
		checkResult_ = result;
	}

protected:
	virtual void itemUpdated(EventsfeedItemPtr) {}
	virtual void itemNew(EventsfeedItemPtr) {}
	virtual void itemsUpdated(const std::vector<EventsfeedItem>&) {}
	virtual void largeUpdated() {}

	void removeIndex(EventsfeedItemPtr item) {
		// clean-up
		std::pair<std::multimap<uint64_t /*order*/, Key /*buzz*/>::iterator,
					std::multimap<uint64_t /*order*/, Key /*buzz*/>::iterator> lRange = index_.equal_range(item->order());
		for (std::multimap<uint64_t /*order*/, Key /*buzz*/>::iterator lExist = lRange.first; lExist != lRange.second; lExist++) {
			if (lExist->second == item->key()) {
				index_.erase(lExist);
				break;
			}
		}
	}

	void insertIndex(EventsfeedItemPtr item) {
		index_.insert(std::multimap<uint64_t /*order*/, Key /*buzz*/>::value_type(item->timestamp(), item->key()));
	}

	void updateTimestamp(const uint256& publisher, uint64_t timestamp) {
		//
		std::map<uint256 /*publisher*/, uint64_t>::iterator lTimestamp = lastTimestamps_.find(publisher);
		if (lTimestamp != lastTimestamps_.end()) {
			if (lTimestamp->second > timestamp) lTimestamp->second = timestamp;
		} else {
			lastTimestamps_[publisher] = timestamp;
		}
	}

protected:
	class Guard {
	public:
		Guard(EventsfeedItem* item): item_(item) {
			item_->lock();
		}

		~Guard() {
			if (item_) item_->unlock();
		}

	private:
		EventsfeedItem* item_ = 0;
	};

protected:
	unsigned short type_;
	uint64_t timestamp_;
	uint256 buzzId_;
	uint256 buzzChainId_;
	std::vector<unsigned char> buzzBody_;
	std::vector<BuzzerMediaPointer> buzzMedia_;
	uint512 signature_;
	std::vector<EventInfo> buzzers_;

	uint64_t order_ = 0;
	std::set<uint256> buzzer_;
	uint32_t buzzerCount_ = 0;
	char mention_ = 0;
	amount_t value_ = 0;

	// merge strategy
	Merge merge_ = Merge::UNION;
	std::set<uint256> chains_;

	// check result
	Buzzer::VerificationResult checkResult_ = Buzzer::VerificationResult::INVALID;

	// threads
	std::map<Key /*buzz*/, EventsfeedItemPtr> items_;
	std::multimap<uint64_t /*order*/, Key /*buzz*/> index_;
	std::map<uint256 /*publisher*/, uint64_t> lastTimestamps_;
	std::map<uint256 /*buzz*/, std::set<Key> /*buzz keys*/> pendings_;

	// resolver
	buzzerInfoFunction buzzerInfo_;
	buzzerInfoResolveFunction buzzerInfoResolve_;
	eventsfeedItemVerifyFunction verifyPublisher_;
};

//
// buzzfeed callbacks
typedef boost::function<void (void)> eventsfeedLargeUpdatedFunction;
typedef boost::function<void (EventsfeedItemPtr)> eventsfeedItemNewFunction;
typedef boost::function<void (EventsfeedItemPtr)> eventsfeedItemUpdatedFunction;
typedef boost::function<void (const std::vector<EventsfeedItem>&)> eventsfeedItemsUpdatedFunction;

class Eventsfeed;
typedef std::shared_ptr<Eventsfeed> EventsfeedPtr;

//
// eventsfeed
class Eventsfeed: public EventsfeedItem {
public:
	Eventsfeed(BuzzerPtr buzzer, eventsfeedItemVerifyFunction verifyPublisher, 
								eventsfeedLargeUpdatedFunction largeUpdated, 
								eventsfeedItemNewFunction itemNew, 
								eventsfeedItemUpdatedFunction itemUpdated,
								eventsfeedItemsUpdatedFunction itemsUpdated, Merge merge) : 
		buzzer_(buzzer),
		largeUpdated_(largeUpdated), 
		itemNew_(itemNew), 
		itemUpdated_(itemUpdated),
		itemsUpdated_(itemsUpdated) {
		verifyPublisher_ = verifyPublisher;
		merge_ = merge;
	}

	Eventsfeed(EventsfeedPtr eventsfeed) {
		buzzer_ = eventsfeed->buzzer_;
		largeUpdated_ = eventsfeed->largeUpdated_; 
		itemNew_ = eventsfeed->itemNew_; 
		itemUpdated_ = eventsfeed->itemUpdated_;
		itemsUpdated_ = eventsfeed->itemsUpdated_;
		verifyPublisher_ = eventsfeed->verifyPublisher_;
		merge_ = eventsfeed->merge_;
	}

	EventsfeedItemPtr toItem() {
		return std::static_pointer_cast<EventsfeedItem>(shared_from_this());
	}

	void prepare() {
		buzzerInfo_ = boost::bind(&Buzzer::locateBuzzerInfo, buzzer_, _1, _2, _3);
		buzzerInfoResolve_ = boost::bind(&Buzzer::enqueueBuzzerInfoResolve, buzzer_, _1, _2, _3);		
	}

	BuzzerPtr buzzer() { return buzzer_; }

	void lock() {
		mutex_.lock();
	}

	void unlock() {
		mutex_.unlock();
	}

	static EventsfeedPtr instance(EventsfeedPtr eventsfeed) {
		return std::make_shared<Eventsfeed>(eventsfeed);
	}

	static EventsfeedPtr instance(BuzzerPtr buzzer, eventsfeedItemVerifyFunction verifyPublisher,
			eventsfeedLargeUpdatedFunction largeUpdated, 
			eventsfeedItemNewFunction itemNew, 
			eventsfeedItemUpdatedFunction itemUpdated,
			eventsfeedItemsUpdatedFunction itemsUpdated, Merge merge) {
		return std::make_shared<Eventsfeed>(buzzer, verifyPublisher, largeUpdated, itemNew, itemUpdated, itemsUpdated, merge);
	}

protected:
	virtual void itemUpdated(EventsfeedItemPtr item) { itemUpdated_(item); }
	virtual void itemNew(EventsfeedItemPtr item) { itemNew_(item); }
	virtual void itemsUpdated(const std::vector<EventsfeedItem>& items) { itemsUpdated_(items); }
	virtual void largeUpdated() { largeUpdated_(); }

private:
	BuzzerPtr buzzer_;
	eventsfeedLargeUpdatedFunction largeUpdated_;
	eventsfeedItemNewFunction itemNew_;
	eventsfeedItemUpdatedFunction itemUpdated_;
	eventsfeedItemsUpdatedFunction itemsUpdated_;

	boost::recursive_mutex mutex_;
};

//
// select eventsfeed handler
class ISelectEventsFeedHandler: public IReplyHandler {
public:
	ISelectEventsFeedHandler() {}
	virtual void handleReply(const std::vector<EventsfeedItem>&, const uint256&) = 0;
};
typedef std::shared_ptr<ISelectEventsFeedHandler> ISelectEventsFeedHandlerPtr;

// special callbacks
typedef boost::function<void (const std::vector<EventsfeedItem>&, const uint256&)> eventsfeedLoadedFunction;
typedef boost::function<void (const std::vector<EventsfeedItem>&, const uint256&, int)> eventsfeedLoadedRequestFunction;

// load transaction
class SelectEventsFeed: public ISelectEventsFeedHandler, public std::enable_shared_from_this<SelectEventsFeed> {
public:
	SelectEventsFeed(eventsfeedLoadedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ISelectEventsFeedHandler
	void handleReply(const std::vector<EventsfeedItem>& feed, const uint256& chain) {
		 function_(feed, chain);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ISelectEventsFeedHandlerPtr instance(eventsfeedLoadedFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectEventsFeed>(function, timeout); 
	}

private:
	eventsfeedLoadedFunction function_;
	timeoutFunction timeout_;
};

//
// select eventsfeed by entity handler
class ISelectEventsFeedByEntityHandler: public IReplyHandler {
public:
	ISelectEventsFeedByEntityHandler() {}
	virtual void handleReply(const std::vector<EventsfeedItem>&, const uint256&, const uint256&) = 0;
};
typedef std::shared_ptr<ISelectEventsFeedByEntityHandler> ISelectEventsFeedByEntityHandlerPtr;

// special callbacks
typedef boost::function<void (const std::vector<EventsfeedItem>&, const uint256&, const uint256&)> eventsfeedLoadedByEntityFunction;
typedef boost::function<void (const std::vector<EventsfeedItem>&, const uint256&, const uint256&, int)> eventsfeedLoadedByEntityRequestFunction;

// load transaction
class SelectEventsFeedByEntity: public ISelectEventsFeedByEntityHandler, public std::enable_shared_from_this<SelectEventsFeedByEntity> {
public:
	SelectEventsFeedByEntity(eventsfeedLoadedByEntityFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ISelectEventsFeedHandler
	void handleReply(const std::vector<EventsfeedItem>& feed, const uint256& chain, const uint256& entity) {
		 function_(feed, chain, entity);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ISelectEventsFeedByEntityHandlerPtr instance(eventsfeedLoadedByEntityFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectEventsFeedByEntity>(function, timeout); 
	}

private:
	eventsfeedLoadedByEntityFunction function_;
	timeoutFunction timeout_;
};

} // qbit

#endif