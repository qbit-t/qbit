// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_PEER_EXTENSION_H
#define QBIT_BUZZER_PEER_EXTENSION_H

#include "../../ipeerextension.h"
#include "../../ipeermanager.h"
#include "../../ipeer.h"
#include "../../message.h"

#include "buzzer.h"
#include "buzzfeed.h"
#include "eventsfeed.h"
#include "conversationsfeed.h"
#include "txbuzzer.h"
#include "txbuzz.h"
#include "handlers.h"

#include "transactionstoreextension.h"

namespace qbit {

// register message type
#define GET_BUZZER_SUBSCRIPTION 			Message::CUSTOM_0000
#define BUZZER_SUBSCRIPTION 				Message::CUSTOM_0001
#define BUZZER_SUBSCRIPTION_IS_ABSENT 		Message::CUSTOM_0002
#define GET_BUZZ_FEED						Message::CUSTOM_0003
#define BUZZ_FEED							Message::CUSTOM_0004
#define NEW_BUZZ_NOTIFY						Message::CUSTOM_0005
#define BUZZ_UPDATE_NOTIFY					Message::CUSTOM_0006
#define GET_BUZZES							Message::CUSTOM_0007
#define GET_EVENTS_FEED						Message::CUSTOM_0008
#define EVENTS_FEED							Message::CUSTOM_0009
#define NEW_EVENT_NOTIFY					Message::CUSTOM_0010
#define EVENT_UPDATE_NOTIFY					Message::CUSTOM_0011

#define GET_BUZZER_TRUST_SCORE				Message::CUSTOM_0012
#define BUZZER_TRUST_SCORE					Message::CUSTOM_0013
#define BUZZER_TRUST_SCORE_UPDATE			Message::CUSTOM_0014

#define GET_BUZZER_MISTRUST_TX				Message::CUSTOM_0015
#define BUZZER_MISTRUST_TX					Message::CUSTOM_0016
#define GET_BUZZER_ENDORSE_TX				Message::CUSTOM_0017
#define BUZZER_ENDORSE_TX					Message::CUSTOM_0018

#define GET_BUZZ_FEED_BY_BUZZ				Message::CUSTOM_0019
#define BUZZ_FEED_BY_BUZZ					Message::CUSTOM_0020
#define GET_BUZZ_FEED_BY_BUZZER				Message::CUSTOM_0021
#define BUZZ_FEED_BY_BUZZER					Message::CUSTOM_0022

/*
#define GET_LIKES_FEED_BY_BUZZ				Message::CUSTOM_0023
#define LIKES_FEED_BY_BUZZ					Message::CUSTOM_0024
#define GET_REBUZZES_FEED_BY_BUZZ			Message::CUSTOM_0025
#define REBUZZES_FEED_BY_BUZZ				Message::CUSTOM_0026
*/

#define GET_MISTRUSTS_FEED_BY_BUZZER		Message::CUSTOM_0027
#define MISTRUSTS_FEED_BY_BUZZER			Message::CUSTOM_0028

#define GET_ENDORSEMENTS_FEED_BY_BUZZER		Message::CUSTOM_0029
#define ENDORSEMENTS_FEED_BY_BUZZER			Message::CUSTOM_0030

#define GET_BUZZER_SUBSCRIPTIONS			Message::CUSTOM_0031
#define BUZZER_SUBSCRIPTIONS				Message::CUSTOM_0032

#define GET_BUZZER_FOLLOWERS				Message::CUSTOM_0033
#define BUZZER_FOLLOWERS					Message::CUSTOM_0034

#define GET_BUZZ_FEED_GLOBAL				Message::CUSTOM_0035
#define BUZZ_FEED_GLOBAL					Message::CUSTOM_0036
#define GET_BUZZ_FEED_BY_TAG				Message::CUSTOM_0037
#define BUZZ_FEED_BY_TAG					Message::CUSTOM_0038

#define GET_HASH_TAGS						Message::CUSTOM_0039
#define HASH_TAGS							Message::CUSTOM_0040

#define BUZZ_SUBSCRIBE						Message::CUSTOM_0041
#define BUZZ_UNSUBSCRIBE					Message::CUSTOM_0042

#define GET_BUZZER_AND_INFO					Message::CUSTOM_0043
#define BUZZER_AND_INFO						Message::CUSTOM_0045
#define BUZZER_AND_INFO_ABSENT				Message::CUSTOM_0046

#define GET_CONVERSATIONS_FEED_BY_BUZZER	Message::CUSTOM_0047
#define CONVERSATIONS_FEED_BY_BUZZER		Message::CUSTOM_0048

#define GET_MESSAGES_FEED_BY_CONVERSATION	Message::CUSTOM_0049
#define MESSAGES_FEED_BY_CONVERSATION		Message::CUSTOM_0050

#define NEW_BUZZER_CONVERSATION_NOTIFY		Message::CUSTOM_0051
#define NEW_BUZZER_MESSAGE_NOTIFY			Message::CUSTOM_0052

#define UPDATE_BUZZER_CONVERSATION_NOTIFY	Message::CUSTOM_0053

//
class BuzzerPeerExtension: public IPeerExtension, public std::enable_shared_from_this<BuzzerPeerExtension> {
public:
	BuzzerPeerExtension(const State::DAppInstance& dapp, IPeerPtr peer, IPeerManagerPtr peerManager, BuzzerPtr buzzer, BuzzfeedPtr buzzfeed, EventsfeedPtr eventsfeed): 
		peer_(peer), peerManager_(peerManager), buzzer_(buzzer), buzzfeed_(buzzfeed), eventsfeed_(eventsfeed) { dapp_ = dapp; }

	void processTransaction(TransactionContextPtr /*tx*/);
	bool processMessage(Message& /*message*/, std::list<DataStream>::iterator /*data*/, const boost::system::error_code& /*error*/);

	static IPeerExtensionPtr instance(const State::DAppInstance& dapp, IPeerPtr peer, IPeerManagerPtr peerManager, BuzzerPtr buzzer, BuzzfeedPtr buzzfeed, EventsfeedPtr eventsfeed) {
		return std::make_shared<BuzzerPeerExtension>(dapp, peer, peerManager, buzzer, buzzfeed, eventsfeed);
	}

	void release() {
		peer_.reset();
		peerManager_.reset();
		buzzer_.reset();
		buzzfeed_.reset();
		eventsfeed_.reset();
	}

	//
	// client-side facade methods
	bool loadSubscription(const uint256& /*chain*/, const uint256& /*subscriber*/, const uint256& /*publisher*/, ILoadTransactionHandlerPtr /*handler*/);
	bool selectBuzzfeed(const uint256& /*chain*/, const std::vector<BuzzfeedPublisherFrom>& /*from*/, const uint256& /*subscriber*/, ISelectBuzzFeedHandlerPtr /*handler*/);
	bool selectBuzzfeedGlobal(const uint256& /*chain*/, uint64_t /*timeframeFrom*/, uint64_t /*scoreFrom*/, uint64_t /*timestampFrom*/, const uint256& /*publisherTs*/, const uint256& /*subscriberTs*/, ISelectBuzzFeedHandlerPtr /*handler*/);
	bool selectBuzzfeedByTag(const uint256& /*chain*/, const std::string& /*tag*/, uint64_t /*timeframeFrom*/, uint64_t /*scoreFrom*/, uint64_t /*timestampFrom*/, const uint256& /*publisher*/, const uint256& /*subscriber*/, ISelectBuzzFeedHandlerPtr /*handler*/);
	bool selectBuzzfeedByBuzz(const uint256& /*chain*/, uint64_t /*from*/, const uint256& /*buzz*/, const uint256& /*subscriber*/, ISelectBuzzFeedByEntityHandlerPtr /*handler*/);
	bool selectBuzzfeedByBuzzer(const uint256& /*chain*/, uint64_t /*from*/, const uint256& /*buzzer*/, const uint256& /*subscriber*/, ISelectBuzzFeedByEntityHandlerPtr /*handler*/);
	bool selectMistrustsByBuzzer(const uint256& /*chain*/, const uint256& /*from*/, const uint256& /*buzzer*/, ISelectEventsFeedByEntityHandlerPtr /*handler*/);
	bool selectEndorsementsByBuzzer(const uint256& /*chain*/, const uint256& /*from*/, const uint256& /*buzzer*/, ISelectEventsFeedByEntityHandlerPtr /*handler*/);
	bool selectSubscriptionsByBuzzer(const uint256& /*chain*/, const uint256& /*from*/, const uint256& /*buzzer*/, ISelectEventsFeedByEntityHandlerPtr /*handler*/);
	bool selectFollowersByBuzzer(const uint256& /*chain*/, const uint256& /*from*/, const uint256& /*buzzer*/, ISelectEventsFeedByEntityHandlerPtr /*handler*/);
	bool selectBuzzes(const uint256& /*chain*/, const std::vector<uint256>& /*buzzes*/, ISelectBuzzFeedHandlerPtr /*handler*/);
	bool selectEventsfeed(const uint256& /*chain*/, uint64_t /*from*/, const uint256& /*subscriber*/, ISelectEventsFeedHandlerPtr /*handler*/);
	bool notifyRebuzzed(const uint256& /*chain*/, uint64_t /*from*/, const uint256& /*subscriber*/, ISelectEventsFeedHandlerPtr /*handler*/);
	bool loadTrustScore(const uint256& /*chain*/, const uint256& /*buzzer*/, ILoadTrustScoreHandlerPtr /*handler*/);
	bool selectBuzzerEndorse(const uint256& /*chain*/, const uint256& /*actor*/, const uint256& /*buzzer*/, ILoadEndorseMistrustHandlerPtr /*handler*/);
	bool selectBuzzerMistrust(const uint256& /*chain*/, const uint256& /*actor*/, const uint256& /*buzzer*/, ILoadEndorseMistrustHandlerPtr /*handler*/);
	bool selectHashTags(const uint256& /*chain*/, const std::string& /*tag*/, ISelectHashTagsHandlerPtr /*handler*/);
	bool subscribeBuzzThread(const uint256& /*chain*/, const uint256& /*buzzId*/, const uint512& /*signature*/);
	bool unsubscribeBuzzThread(const uint256& /*chain*/, const uint256& /*buzzId*/);
	bool loadBuzzerAndInfo(const std::string& /*buzzer*/, ILoadBuzzerAndInfoHandlerPtr /*handler*/);
	bool selectConversations(const uint256& /*chain*/, uint64_t /*from*/, const uint256& /*buzzer*/, ISelectConversationsFeedByEntityHandlerPtr /*handler*/);
	bool selectMessages(const uint256& /*chain*/, uint64_t /*from*/, const uint256& /*conversation*/, ISelectBuzzFeedByEntityHandlerPtr /*handler*/);

	//
	// node-to-peer notifications  
	bool notifyNewBuzz(const BuzzfeedItem& /*buzz*/);
	bool notifyUpdateBuzz(const std::vector<BuzzfeedItemUpdate>& /*update*/);
	bool notifyNewEvent(const EventsfeedItem& /*buzz*/, bool tryForward = false);
	bool notifyUpdateTrustScore(const BuzzerTransactionStoreExtension::BuzzerInfo& /*score*/);
	bool notifyNewConversation(const ConversationItem& /*conversation*/);
	bool notifyUpdateConversation(const ConversationItem::EventInfo& /*info*/);
	bool notifyNewMessage(const BuzzfeedItem& /*message*/);

private:
	// server-side processing
	void processGetSubscription(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBuzzfeed(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBuzzfeedGlobal(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBuzzfeedByTag(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBuzzfeedByBuzz(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBuzzfeedByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetMistrustsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetEndorsementsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetSubscriptionsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetFollowersByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBuzzes(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetEventsfeed(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBuzzerTrustScore(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBuzzerEndorseTx(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBuzzerMistrustTx(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetHashTags(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processSubscribeBuzzThread(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processUnsubscribeBuzzThread(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBuzzerAndInfo(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetConversationsFeedByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetMessagesFeedByConversation(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	// client-side answer processing
	void processSubscription(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzfeed(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzfeedGlobal(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzfeedByTag(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzfeedByBuzz(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzfeedByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processMistrustsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processEndorsementsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processSubscriptionsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processFollowersByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processNewBuzzNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzUpdateNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processEventsfeed(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processNewEventNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processEventUpdateNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzerTrustScore(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzerTrustScoreUpdateNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzerEndorseTx(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzerMistrustTx(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processHashTags(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzerAndInfo(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzerAndInfoAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processConversationsFeedByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processMessagesFeedByConversation(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processNewConversationNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processUpdateConversationNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processNewMessageNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	//
	void processSubscriptionAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

private:
	void prepareBuzzfeedItem(BuzzfeedItem&, TxBuzzPtr, TxBuzzerPtr, BuzzerTransactionStoreExtensionPtr);
	void prepareEventsfeedItem(EventsfeedItem&, TxBuzzPtr, TxBuzzerPtr, const uint256&, const uint256&, uint64_t, const uint256&, const uint256&, uint64_t, const uint512&);

	bool processBuzzCommon(TransactionContextPtr ctx, bool processHide = false, uint64_t timestamp = 0, const uint512& signature = uint512());
	bool processBuzzLike(TransactionContextPtr ctx);
	bool processBuzzHide(TransactionContextPtr ctx);
	bool processBuzzReward(TransactionContextPtr ctx);
	bool processRebuzz(TransactionContextPtr ctx);

	bool processEventCommon(TransactionContextPtr ctx);
	bool processEventLike(TransactionContextPtr ctx);
	bool processEventReward(TransactionContextPtr ctx);
	bool processEventSubscribe(TransactionContextPtr ctx);

	bool processEndorse(TransactionContextPtr ctx);
	bool processMistrust(TransactionContextPtr ctx);

	bool processConversation(TransactionContextPtr ctx);
	bool processAcceptConversation(TransactionContextPtr ctx);
	bool processDeclineConversation(TransactionContextPtr ctx);
	bool processConversationMessage(TransactionContextPtr ctx, bool processHide = false, uint64_t timestamp = 0, const uint512& signature = uint512());
	bool processConversationMessageReply(TransactionContextPtr ctx);

	bool haveBuzzSubscription(TransactionPtr ctx, uint512& signature);

	BuzzerTransactionStoreExtensionPtr locateBuzzerExtension(const uint256& /*buzzer*/);

public:
	IPeerPtr peer_;
	IPeerManagerPtr peerManager_;
	BuzzerPtr buzzer_;
	BuzzfeedPtr buzzfeed_;
	EventsfeedPtr eventsfeed_;

	// NOTICE: do we need new_buzz notification also bundled?

	// buzzfeed items pending updates
	uint64_t lastEventsfeedItemTimestamp_ = 0;
	std::vector<EventsfeedItem> lastEventsfeedItems_;
	bool pendingEventsfeedItems_ = false;

	// buzzfeed items pending updates
	uint64_t lastBuzzfeedItemTimestamp_ = 0;
	std::vector<BuzzfeedItemUpdate> lastBuzzfeedItems_;
	bool pendingBuzzfeedItems_ = false;

	// trust score pending updates
	uint64_t lastScoreTimestamp_ = 0;
	BuzzerTransactionStoreExtension::BuzzerInfo lastScore_;
	bool pendingScore_ = false;

	// buzz threads subscriptions
	std::map<uint256 /*buzz_id*/, uint512 /*signature*/> buzzSubscriptions_; 

	boost::recursive_mutex notificationMutex_;
};

//
class BuzzerPeerExtensionCreator: public PeerExtensionCreator {
public:
	BuzzerPeerExtensionCreator() {}
	BuzzerPeerExtensionCreator(BuzzerPtr buzzer, BuzzfeedPtr buzzfeed, EventsfeedPtr eventsfeed): buzzer_(buzzer), buzzfeed_(buzzfeed), eventsfeed_(eventsfeed) {}
	IPeerExtensionPtr create(const State::DAppInstance& dapp, IPeerPtr peer, IPeerManagerPtr peerManager) {
		return BuzzerPeerExtension::instance(dapp, peer, peerManager, buzzer_, buzzfeed_, eventsfeed_);
	}

	static PeerExtensionCreatorPtr instance(BuzzerPtr buzzer, BuzzfeedPtr buzzfeed, EventsfeedPtr eventsfeed) {
		return std::make_shared<BuzzerPeerExtensionCreator>(buzzer, buzzfeed, eventsfeed);
	}

	static PeerExtensionCreatorPtr instance() {
		return std::make_shared<BuzzerPeerExtensionCreator>();
	}

private:
	BuzzerPtr buzzer_ = nullptr;
	BuzzfeedPtr buzzfeed_ = nullptr;
	EventsfeedPtr eventsfeed_ = nullptr;
};

} // qbit

#endif
