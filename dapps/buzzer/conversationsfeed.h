// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CONVERSATIONS_FEED_H
#define QBIT_CONVERSATIONS_FEED_H

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
class ConversationItem;
typedef std::shared_ptr<ConversationItem> ConversationItemPtr;

//
typedef boost::function<Buzzer::VerificationResult (ConversationItemPtr)> conversationItemVerifyFunction;

//
// conversation item
//  
class ConversationItem: public std::enable_shared_from_this<ConversationItem> {
public:
	enum Side {
		CREATOR = 0,
		COUNTERPARTY = 1
	};

public:
	class EventInfo {
	public:
		EventInfo() {
			type_ = 0;
			timestamp_ = 0;
			conversationId_.setNull();
			eventId_.setNull();
			eventChainId_.setNull();
			buzzerId_.setNull();
			buzzerInfoId_.setNull();
			buzzerInfoChainId_.setNull();
		}

		EventInfo(
			unsigned short type,
			uint64_t timestamp, 
			const uint256& conversationId,
			const uint256& eventId, 
			const uint256& eventChainId, 
			const uint256& buzzerId, 
			const uint256& buzzerInfoChainId, 
			const uint256& buzzerInfoId, 
			uint64_t score,
			const uint512& signature): 
			type_(type), timestamp_(timestamp), conversationId_(conversationId), eventId_(eventId), eventChainId_(eventChainId),
			buzzerId_(buzzerId), buzzerInfoChainId_(buzzerInfoChainId), buzzerInfoId_(buzzerInfoId), score_(score),
			signature_(signature) {
		}

		EventInfo(
			unsigned short type,
			uint64_t timestamp, 
			const uint256& conversationId,
			const uint256& eventId, 
			const uint256& eventChainId, 
			const uint256& buzzerId, 
			const uint256& buzzerInfoChainId, 
			const uint256& buzzerInfoId, 
			const std::vector<BuzzerMediaPointer>& media,
			const std::vector<unsigned char>& body,
			uint64_t score,
			const uint512& signature): 
			type_(type), timestamp_(timestamp), conversationId_(conversationId), eventId_(eventId), eventChainId_(eventChainId),
			buzzerId_(buzzerId), buzzerInfoChainId_(buzzerInfoChainId), buzzerInfoId_(buzzerInfoId), score_(score),
			media_(media), body_(body),
			signature_(signature) {
		}

		unsigned short type() const { return type_; }
		uint64_t timestamp() const { return timestamp_; }
		const uint256& conversationId() const { return conversationId_; }
		const uint256& buzzerId() const { return buzzerId_; }
		const uint256& buzzerInfoId() const { return buzzerInfoId_; }
		const uint256& buzzerInfoChainId() const { return buzzerInfoChainId_; }
		const uint512& signature() const { return signature_; }
		uint64_t score() const { return score_; }

		const uint256& eventId() const { return eventId_; } 
		const uint256& eventChainId() const { return eventChainId_; } 

		const std::vector<BuzzerMediaPointer>& media() const { return media_; }
		const std::vector<unsigned char>& body() const { return body_; }
		std::vector<unsigned char>& decryptedBody() { return decryptedBody_; }

		void setMedia(const std::vector<BuzzerMediaPointer>& media) {
			media_ = media;
		}
		void setBody(const std::vector<unsigned char>& body) {
			body_ = body;
		}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(type_);
			READWRITE(timestamp_);
			READWRITE(score_);
			READWRITE(conversationId_);
			READWRITE(eventId_);
			READWRITE(eventChainId_);
			READWRITE(buzzerId_);
			READWRITE(buzzerInfoId_);
			READWRITE(buzzerInfoChainId_);
			if (type_ == TX_BUZZER_MESSAGE || type_ == TX_BUZZER_MESSAGE_REPLY) {
				READWRITE(body_);
				READWRITE(media_);
			}

			READWRITE(signature_);
		}

	protected:
		unsigned short type_;
		uint64_t timestamp_;
		uint64_t score_;
		uint256 conversationId_;
		uint256 eventId_;
		uint256 eventChainId_;
		uint256 buzzerId_;
		uint256 buzzerInfoId_;
		uint256 buzzerInfoChainId_;
		std::vector<unsigned char> body_;
		std::vector<unsigned char> decryptedBody_;
		std::vector<BuzzerMediaPointer> media_;
		uint512 signature_;
	};

public:
	ConversationItem() {
		score_ = 0;
		side_ = 0;
		timestamp_ = 0;
		conversationId_.setNull();
		conversationChainId_.setNull();
		creatorId_.setNull();
		creatorInfoId_.setNull();
		creatorInfoChainId_.setNull();
		counterpartyId_.setNull();
		counterpartyInfoId_.setNull();
		counterpartyInfoChainId_.setNull();
		signature_.setNull();
	}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(timestamp_);
		READWRITE(score_);
		READWRITE(side_); // 0 - creator, 1 - counterparty
		READWRITE(conversationId_);
		READWRITE(conversationChainId_);
		READWRITE(creatorId_);
		READWRITE(creatorInfoId_);
		READWRITE(creatorInfoChainId_);
		READWRITE(counterpartyId_);
		READWRITE(counterpartyInfoId_);
		READWRITE(counterpartyInfoChainId_);
		READWRITE(events_);
		READWRITE(signature_);

		if (ser_action.ForRead()) {
			if (!events_.size()) {
				order_ = timestamp_;	
			} else {
				order_ = events_.rbegin()->timestamp();
			}
		}
	}

	uint64_t score() const { return score_; }
	void setScore(uint64_t score) { score_ = score; }

	const uint256& conversationId() const { return conversationId_; }
	void setConversationId(const uint256& id) { conversationId_ = id; }

	const uint256& conversationChainId() const { return conversationChainId_; }
	void setConversationChainId(const uint256& id) { conversationChainId_ = id; }

	const uint256& key() const { return conversationId_; }

	void addEventInfo(const EventInfo& info) {
		events_.push_back(info);
	}

	void infoMerge(const std::vector<EventInfo>& events) {
		//
		for (std::vector<EventInfo>::const_iterator lEvent = events.begin(); lEvent != events.end(); lEvent++) {
			//
			infoMerge(*lEvent);
		}
	}

	void infoMerge(const EventInfo& event) {
		//
		if (!events_.size()) {
			addEventInfo(event);
			if (event.timestamp() > order_) order_ = event.timestamp();
			return;
		}

		bool lFound = false;
		std::vector<EventInfo>::const_iterator lEvent = events_.begin();
		for (; lEvent != events_.end(); lEvent++) {
			if (event.type() == lEvent->type()) { lFound = true; break; }
		}

		if (lFound) events_.erase(lEvent);

		if (event.type() == TX_BUZZER_ACCEPT_CONVERSATION || event.type() == TX_BUZZER_DECLINE_CONVERSATION) {
			events_.insert(events_.begin(), event);
		} else {
			events_.insert(events_.end(), event);
		}

		if (event.timestamp() > order_) { 
			timestamp_ = event.timestamp();
			order_ = event.timestamp();
		}
	}

	const std::vector<EventInfo>& events() const { return events_; }

	uint64_t order() { return order_; }
	void setOrder(uint64_t order) { order_ = order; }

	const uint256& creatorId() const { return creatorId_; }
	void setCreatorId(const uint256& id) { creatorId_ = id; }

	const uint256& creatorInfoId() const { return creatorInfoId_; }
	void setCreatorInfoId(const uint256& id) { creatorInfoId_ = id; }

	const uint256& creatorInfoChainId() const { return creatorInfoChainId_; }
	void setCreatorInfoChainId(const uint256& id) { creatorInfoChainId_ = id; }

	const uint256& counterpartyId() const { return counterpartyId_; }
	void setCounterpartyId(const uint256& id) { counterpartyId_ = id; }

	const uint256& counterpartyInfoId() const { return counterpartyInfoId_; }
	void setCounterpartyInfoId(const uint256& id) { counterpartyInfoId_ = id; }

	const uint256& counterpartyInfoChainId() const { return counterpartyInfoChainId_; }
	void setCounterpartyInfoChainId(const uint256& id) { counterpartyInfoChainId_ = id; }

	uint64_t timestamp() const { return timestamp_; }
	void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	const uint512& signature() const { return signature_; }
	void setSignature(const uint512& signature) { signature_ = signature; }	

	Side side() { return (Side)side_; }
	void setSide(Side side) { side_ = side; }

	static ConversationItemPtr instance(const ConversationItem& item) {
		return std::make_shared<ConversationItem>(item);
	}

	static ConversationItemPtr instance() {
		return std::make_shared<ConversationItem>();
	}

	void push(const ConversationItem&, const uint160&);
	void merge(const ConversationItem&, bool checkSize = true, bool notify = true);
	void mergeInternal(ConversationItemPtr, bool checkSize = true, bool notify = true);
	void merge(const std::vector<ConversationItem>&, const uint256&, bool notify = false);
	void mergeUpdate(const std::vector<ConversationItem::EventInfo>&, const uint160&);
	void mergeUpdate(const std::vector<ConversationItem>&, const uint160&);
	bool mergeAppend(const std::vector<ConversationItemPtr>&);

	void feed(std::vector<ConversationItemPtr>&);
	int locateIndex(ConversationItemPtr);

	ConversationItemPtr locateItem(const uint256&);

	size_t confirmations() { return confirmed_.size(); }
	size_t addConfirmation(const uint160& peer) {
		confirmed_.insert(peer);
		return confirmed_.size();
	}

	bool decrypt(const PKey&, std::string&);
	bool decrypt(const PKey&, const std::string&, std::string&);

	std::string toString() {
		//
		std::string lBuzzerName = strprintf("<c = %s>", creatorId_.toHex());
		std::string lBuzzerAlias;
		buzzerInfo(lBuzzerName, lBuzzerAlias);

		//
		std::string lState = "?";
		if (events_.size()) {
			EventInfo lInfo = *events_.begin();
			if (lInfo.type() == TX_BUZZER_ACCEPT_CONVERSATION) lState = "A";
			else if (lInfo.type() == TX_BUZZER_ACCEPT_CONVERSATION) lState = "D";
			else if (lInfo.type() == TX_BUZZER_MESSAGE) lState = "*";
		}

		return strprintf("%s\n%s (%s) [%s] | %s",
			conversationId_.toHex(),
			lBuzzerAlias, lBuzzerName, lState,
			formatISO8601DateTime(order_ /*timestamp()*/ / 1000000));
	}

	bool isOnChain() { return onChain_; }
	void notOnChain() { onChain_ = false; }
	void setOnChain() { onChain_ = true; }

	bool isDynamic() { return dynamic_; }
	void setDynamic() { dynamic_ = true; }

	virtual void lock() {}
	virtual void unlock() {}
	virtual ConversationItemPtr parent() { return parent_; }
	virtual BuzzerPtr buzzer() {
		if (parent_) return parent_->buzzer();
		return nullptr;
	}

	virtual void addBuzzer(const std::string& name, const uint256& conversation) {
		buzzers_[name] = conversation;
	}

	virtual bool locateBuzzer(const std::string& name, uint256& conversation) {
		//
		std::map<std::string /*buzzer*/, uint256 /*conversation*/>::iterator lBuzzer = buzzers_.find(name);
		if (lBuzzer != buzzers_.end()) { conversation = lBuzzer->second; return true; }
		return false;
	}

	virtual void locateBuzzerByPart(const std::string& name, std::vector<uint256>& conversation) {
		//
		for (std::map<std::string /*buzzer*/, uint256 /*conversation*/>::iterator lBuzzer = buzzers_.begin();
				lBuzzer != buzzers_.end(); lBuzzer++) {
			//
			if (lBuzzer->first.find(name) != std::string::npos) {
				conversation.push_back(lBuzzer->second);
			}
		}
	}

	virtual uint64_t locateLastTimestamp();
	virtual void collectPendingItems(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>&);

	void setResolver(buzzerInfoResolveFunction buzzerInfoResolve, buzzerInfoFunction buzzerInfo) {
		buzzerInfoResolve_ = buzzerInfoResolve;
		buzzerInfo_ = buzzerInfo;
	}

	bool resolve() {
		//
		if (!buzzerInfoResolve_) return false;

		bool lResultCreator = buzzerInfoResolve_(
				creatorInfoChainId_, 
				creatorId_, 
				creatorInfoId_,
				boost::bind(&ConversationItem::buzzerInfoReady, shared_from_this(), boost::placeholders::_1));

		bool lResultCounterparty = buzzerInfoResolve_(
				counterpartyInfoChainId_, 
				counterpartyId_, 
				counterpartyInfoId_,
				boost::bind(&ConversationItem::buzzerInfoReady, shared_from_this(), boost::placeholders::_1));

		if (lResultCreator || lResultCounterparty) {
			//
			std::string lName;
			std::string lAlias;
			buzzerInfo(lName, lAlias);
		}

		return lResultCreator && lResultCounterparty;
	}

	void buzzerInfoReady(const uint256& /*info*/) {
		//
		std::string lName;
		std::string lAlias;
		buzzerInfo(lName, lAlias);

		//
		if (fed()) itemNew(shared_from_this());		
	}

	void buzzerInfo(std::string& name, std::string& alias) {
		if ((Side)side_ == Side::CREATOR)
			buzzerInfo_(creatorInfoId_, name, alias); 
		else
			buzzerInfo_(counterpartyInfoId_, name, alias); 

		parent_->addBuzzer(name, conversationId_);
	}

	void buzzerInfo(const EventInfo& info, std::string& name, std::string& alias) {
		buzzerInfo_(info.buzzerInfoId(), name, alias);
		parent_->addBuzzer(name, conversationId_);
	}

	virtual void clear() {
		items_.clear();
		index_.clear();
		lastTimestamps_.clear();
		pendings_.clear();
		chains_.clear();
		resetFed();
	}

	virtual ConversationItemPtr last() {
		Guard lLock(this);
		if (index_.size()) {
			std::multimap<uint64_t /*order*/, uint256 /*conversation*/>::iterator lItem = index_.begin();
			std::map<uint256 /*conversation*/, ConversationItemPtr>::iterator lConversation = items_.find(lItem->second);
			if (lConversation != items_.end())
				return lConversation->second;
		}

		return nullptr;
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

	virtual bool fed() { if (parent_) return parent_->fed(); return false; }
	virtual void setFed() {}
	virtual void resetFed() {}

protected:
	virtual void itemUpdated(ConversationItemPtr item) { if (parent_) parent_->itemUpdated(item); }
	virtual void itemNew(ConversationItemPtr item) {
		if (parent_) parent_->itemNew(item);
	}
	virtual void largeUpdated() { if (parent_) parent_->largeUpdated(); }

	void removeIndex(ConversationItemPtr item) {
		// clean-up
		std::pair<std::multimap<uint64_t /*order*/, uint256 /*conversation*/>::iterator,
					std::multimap<uint64_t /*order*/, uint256 /*conversation*/>::iterator> lRange = index_.equal_range(item->order());
		for (std::multimap<uint64_t /*order*/, uint256 /*conversation*/>::iterator lExist = lRange.first; lExist != lRange.second; lExist++) {
			if (lExist->second == item->key()) {
				index_.erase(lExist);
				break;
			}
		}
	}

	void insertIndex(ConversationItemPtr item) {
		index_.insert(std::multimap<uint64_t /*order*/, uint256 /*conversation*/>::value_type(item->order(), item->key()));
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
		Guard(ConversationItem* item): item_(item) {
			item_->lock();
		}

		~Guard() {
			if (item_) item_->unlock();
		}

	private:
		ConversationItem* item_ = 0;
	};

protected:
	uint64_t timestamp_;
	uint64_t score_;
	unsigned char side_;
	uint256 conversationId_;
	uint256 conversationChainId_;
	uint256 creatorId_;
	uint256 creatorInfoId_;
	uint256 creatorInfoChainId_;
	uint256 counterpartyId_;
	uint256 counterpartyInfoId_;
	uint256 counterpartyInfoChainId_;
	std::vector<EventInfo> events_;
	uint512 signature_;

	std::map<uint256 /*conversation*/, ConversationItemPtr> unconfirmed_;
	std::set<uint160> confirmed_;

	uint64_t order_ = 0;

	std::set<uint256> chains_;

	bool onChain_ = true;
	bool dynamic_ = false;

	// check result
	Buzzer::VerificationResult checkResult_ = Buzzer::VerificationResult::INVALID;

	// conversations
	std::map<uint256 /*conversation*/, ConversationItemPtr> items_;
	std::multimap<uint64_t /*order*/, uint256 /*conversation*/> index_;
	std::map<uint256 /*publisher*/, uint64_t> lastTimestamps_;
	std::map<uint256 /*conversation*/, std::set<uint256> /*keys*/> pendings_;
	std::map<std::string /*buzzer*/, uint256 /*conversation*/> buzzers_;

	// resolver
	buzzerInfoFunction buzzerInfo_;
	buzzerInfoResolveFunction buzzerInfoResolve_;
	conversationItemVerifyFunction verifyPublisher_;

	// parent
	ConversationItemPtr parent_;
};

//
// buzzfeed callbacks
typedef boost::function<void (void)> conversationLargeUpdatedFunction;
typedef boost::function<void (ConversationItemPtr)> conversationItemNewFunction;
typedef boost::function<void (ConversationItemPtr)> conversationItemUpdatedFunction;
typedef boost::function<void (const std::vector<ConversationItem>&)> conversationItemsUpdatedFunction;

class Conversationsfeed;
typedef std::shared_ptr<Conversationsfeed> ConversationsfeedPtr;

//
// eventsfeed
class Conversationsfeed: public ConversationItem {
public:
	Conversationsfeed(BuzzerPtr buzzer, conversationItemVerifyFunction verifyPublisher,
								conversationLargeUpdatedFunction largeUpdated, 
								conversationItemNewFunction itemNew, 
								conversationItemUpdatedFunction itemUpdated,
								conversationItemsUpdatedFunction itemsUpdated) : 
		buzzer_(buzzer),
		largeUpdated_(largeUpdated), 
		itemNew_(itemNew), 
		itemUpdated_(itemUpdated),
		itemsUpdated_(itemsUpdated) {
		verifyPublisher_ = verifyPublisher;
	}

	Conversationsfeed(ConversationsfeedPtr feed) {
		buzzer_ = feed->buzzer_;
		largeUpdated_ = feed->largeUpdated_;
		itemNew_ = feed->itemNew_;
		itemUpdated_ = feed->itemUpdated_;
		itemsUpdated_ = feed->itemsUpdated_;
		verifyPublisher_ = feed->verifyPublisher_;
		buzzerInfo_ = feed->buzzerInfo_;
		buzzerInfoResolve_ = feed->buzzerInfoResolve_;
	}

	ConversationItemPtr toItem() {
		return std::static_pointer_cast<ConversationItem>(shared_from_this());
	}

	ConversationItemPtr parent() { return toItem(); }

	void prepare() {
		buzzerInfo_ = boost::bind(&Buzzer::locateBuzzerInfo, buzzer_, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3);
		buzzerInfoResolve_ = boost::bind(&Buzzer::enqueueBuzzerInfoResolve, buzzer_, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3, boost::placeholders::_4);
	}

	BuzzerPtr buzzer() { return buzzer_; }

	bool fed() { return fed_; }
	void setFed() { fed_ = true; }
	void resetFed() { fed_ = false; }

	void lock() {
		mutex_.lock();
	}

	void unlock() {
		mutex_.unlock();
	}

	static ConversationsfeedPtr instance(ConversationsfeedPtr feed) {
		return std::make_shared<Conversationsfeed>(feed);
	}

	static ConversationsfeedPtr instance(BuzzerPtr buzzer, conversationItemVerifyFunction verifyPublisher, 
			conversationLargeUpdatedFunction largeUpdated, 
			conversationItemNewFunction itemNew, 
			conversationItemUpdatedFunction itemUpdated,
			conversationItemsUpdatedFunction itemsUpdated) {
		return std::make_shared<Conversationsfeed>(buzzer, verifyPublisher, largeUpdated, 
																itemNew, itemUpdated, itemsUpdated);
	}

protected:
	virtual void itemUpdated(ConversationItemPtr item) { itemUpdated_(item); }
	virtual void itemNew(ConversationItemPtr item) { itemNew_(item); }
	virtual void largeUpdated() { largeUpdated_(); }

private:
	BuzzerPtr buzzer_;
	conversationLargeUpdatedFunction largeUpdated_;
	conversationItemNewFunction itemNew_;
	conversationItemUpdatedFunction itemUpdated_;
	conversationItemsUpdatedFunction itemsUpdated_;
	bool fed_ = false;

	boost::recursive_mutex mutex_;
};

//
// conversations ready
typedef boost::function<void (ConversationsfeedPtr /*base feed*/, ConversationsfeedPtr /*part feed*/)> conversationsReadyFunction;

//
// select conversations handler
class ISelectConversationsFeedHandler: public IReplyHandler {
public:
	ISelectConversationsFeedHandler() {}
	virtual void handleReply(const std::vector<ConversationItem>&, const uint256&) = 0;
};
typedef std::shared_ptr<ISelectConversationsFeedHandler> ISelectConversationsFeedHandlerPtr;

// special callbacks
typedef boost::function<void (const std::vector<ConversationItem>&, const uint256&)> conversationsLoadedFunction;
typedef boost::function<void (const std::vector<ConversationItem>&, const uint256&, int)> conversationsLoadedRequestFunction;

// load transaction
class SelectConversationsFeed: public ISelectConversationsFeedHandler, public std::enable_shared_from_this<SelectConversationsFeed> {
public:
	SelectConversationsFeed(conversationsLoadedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// IConversationsFeedHandler
	void handleReply(const std::vector<ConversationItem>& feed, const uint256& chain) {
		 function_(feed, chain);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ISelectConversationsFeedHandlerPtr instance(conversationsLoadedFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectConversationsFeed>(function, timeout); 
	}

private:
	conversationsLoadedFunction function_;
	timeoutFunction timeout_;
};

//
// select eventsfeed by entity handler
class ISelectConversationsFeedByEntityHandler: public IReplyHandler {
public:
	ISelectConversationsFeedByEntityHandler() {}
	virtual void handleReply(const std::vector<ConversationItem>&, const uint256&, const uint256&) = 0;
};
typedef std::shared_ptr<ISelectConversationsFeedByEntityHandler> ISelectConversationsFeedByEntityHandlerPtr;

// special callbacks
typedef boost::function<void (const std::vector<ConversationItem>&, const uint256&, const uint256&)> conversationsLoadedByEntityFunction;
typedef boost::function<void (const std::vector<ConversationItem>&, const uint256&, const uint256&, int)> conversationsLoadedByEntityRequestFunction;

//
class SelectConversationsFeedByEntity: public ISelectConversationsFeedByEntityHandler, public std::enable_shared_from_this<SelectConversationsFeedByEntity> {
public:
	SelectConversationsFeedByEntity(conversationsLoadedByEntityFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ISelectConversationsFeedByEntityHandler
	void handleReply(const std::vector<ConversationItem>& feed, const uint256& chain, const uint256& entity) {
		 function_(feed, chain, entity);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ISelectConversationsFeedByEntityHandlerPtr instance(conversationsLoadedByEntityFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectConversationsFeedByEntity>(function, timeout); 
	}

private:
	conversationsLoadedByEntityFunction function_;
	timeoutFunction timeout_;
};

} // qbit

#endif
