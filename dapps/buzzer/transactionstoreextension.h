// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_TRANSACTION_STORE_EXTENSION_H
#define QBIT_BUZZER_TRANSACTION_STORE_EXTENSION_H

#include "../../transaction.h"
#include "../../block.h"
#include "../../blockcontext.h"
#include "../../transactioncontext.h"
#include "../../itransactionstoreextension.h"
#include "../../db/containers.h"

#include "buzzfeed.h"
#include "eventsfeed.h"
#include "txbuzzer.h"
#include "txbuzz.h"
#include "txbuzzerendorse.h"

#include <memory>

namespace qbit {

class BuzzerTransactionStoreExtension: public ITransactionStoreExtension, public std::enable_shared_from_this<BuzzerTransactionStoreExtension> {
public:
	class BuzzInfo {
	public:
		BuzzInfo() {}

		uint32_t replies_ = 0;
		uint32_t rebuzzes_ = 0;
		uint32_t likes_ = 0;

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(replies_);
			READWRITE(rebuzzes_);
			READWRITE(likes_);
		}
	};

	// TODO: rename to BuzzerStats
	// add my_subscribers, my_subscriptions aggregates
	class BuzzerInfo {
	public:
		BuzzerInfo() {}

		void incrementEndorsements(uint64_t endorsements) {
			endorsements_ += endorsements;
		}
		void decrementEndorsements(uint64_t endorsements) {
			//
			if ((int64_t)endorsements_ - endorsements > 0) endorsements_ -= endorsements;
			else endorsements_ = 0;
		}

		void incrementMistrusts(uint64_t mistrusts) {
			mistrusts_ += mistrusts;
		}
		void decrementMistrusts(uint64_t mistrusts) {
			//
			if ((int64_t)mistrusts_ - mistrusts > 0) mistrusts_ -= mistrusts;
			else mistrusts_ = 0;
		}

		bool trusted() {
			return endorsements_ >= mistrusts_;
		}

		uint64_t score() {
			if (endorsements_ >= mistrusts_) return endorsements_ - mistrusts_;
			return 0;
		}

		uint64_t endorsements() const { return endorsements_; }
		void setEndorsements(uint64_t v) { endorsements_ = v; }

		uint64_t mistrusts() const { return mistrusts_; }
		void setMistrusts(uint64_t v) { mistrusts_ = v; }

		uint32_t subscriptions() { return subscriptions_; }
		void setSubscriptions(uint32_t v) { subscriptions_ = v; }
		void incrementSubscriptions() { subscriptions_++; }
		void decrementSubscriptions() {
			//
			if ((int32_t)subscriptions_-1 > 0) subscriptions_--;
			else subscriptions_ = 0;
		}

		uint32_t followers() { return followers_; }
		void setFollowers(uint32_t v) { followers_ = v; }
		void incrementFollowers() { followers_++; }
		void decrementFollowers() {
			//
			if ((int32_t)followers_-1 > 0) followers_--;
			else followers_ = 0;
		}

		const uint256& info() const { return info_; }
		void setInfo(const uint256& info) { info_ = info; }

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(endorsements_);
			READWRITE(mistrusts_);
			READWRITE(subscriptions_);
			READWRITE(followers_);
			READWRITE(info_);
		}

	private:
		uint64_t endorsements_ = BUZZER_TRUST_SCORE_BASE; // initial
		uint64_t mistrusts_ = BUZZER_TRUST_SCORE_BASE; // initial
		uint32_t subscriptions_ = 0;
		uint32_t followers_ = 0;
		uint256 info_;
	};

public:
	BuzzerTransactionStoreExtension(ISettingsPtr settings, ITransactionStorePtr store) : 
		settings_(settings),
		store_(store),
		timeline_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/timeline"), 
		globalTimeline_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/global_timeline"), 
		hashTagTimeline_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/hashed_timeline"), 
		hashTags_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/hash_tags"), 
		events_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/events"), 
		subscriptionsIdx_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/indexes/subscriptions"),
		publishersIdx_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/indexes/publishers"),
		likesIdx_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/indexes/likes"),
		rebuzzesIdx_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/indexes/rebuzzes"),
		publisherUpdates_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/indexes/publisher_updates"),		
		subscriberUpdates_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/indexes/subscriber_updates"),		
		replies_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/replies"),		
		//rebuzzes_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/rebuzzes"),
		endorsements_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/endorsements"),
		mistrusts_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/mistrusts"),
		buzzInfo_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/buzz_info"),
		buzzerStat_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/buzzer_stat")
		{}

	bool open();
	bool close();

	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) {}
	bool popUnlinkedOut(const uint256&, TransactionContextPtr) {}
	bool pushEntity(const uint256&, TransactionContextPtr);
	bool isAllowed(TransactionContextPtr);

	TransactionPtr locateSubscription(const uint256& /*subscriber*/, const uint256& /*publisher*/);
	void selectBuzzfeed(uint64_t /*from*/, const uint256& /*subscriber*/, std::vector<BuzzfeedItem>& /*feed*/);
	void selectBuzzfeedGlobal(uint64_t /*timeframeFrom*/, uint64_t /*scoreFrom*/, const uint160& /*publisherTs*/, std::vector<BuzzfeedItem>& /*feed*/);
	void selectBuzzfeedByTag(const std::string& /*tag*/, uint64_t /*timeframeFrom*/, uint64_t /*scoreFrom*/, const uint160& /*publisherTs*/, std::vector<BuzzfeedItem>& /*feed*/);
	void selectBuzzfeedByBuzz(uint64_t /*from*/, const uint256& /*buzz*/, std::vector<BuzzfeedItem>& /*feed*/);
	void selectBuzzfeedByBuzzer(uint64_t /*from*/, const uint256& /*buzzer*/, std::vector<BuzzfeedItem>& /*feed*/);
	void selectEventsfeed(uint64_t /*from*/, const uint256& /*buzzer*/, std::vector<EventsfeedItem>& /*feed*/);
	void selectMistrusts(const uint256& /*from*/, const uint256& /*buzzer*/, std::vector<EventsfeedItem>& /*feed*/);
	void selectEndorsements(const uint256& /*from*/, const uint256& /*buzzer*/, std::vector<EventsfeedItem>& /*feed*/);
	void selectSubscriptions(const uint256& /*from*/, const uint256& /*buzzer*/, std::vector<EventsfeedItem>& /*feed*/);
	void selectFollowers(const uint256& /*from*/, const uint256& /*buzzer*/, std::vector<EventsfeedItem>& /*feed*/);
	bool checkSubscription(const uint256& /*subscriber*/, const uint256& /*publisher*/);
	bool checkLike(const uint256& /*buzz*/, const uint256& /*liker*/);
	bool readBuzzInfo(const uint256& /*buzz*/, BuzzInfo& /*info*/);
	bool readBuzzerStat(const uint256& /*buzzer*/, BuzzerInfo& /*stat*/);
	TxBuzzerInfoPtr readBuzzerInfo(const uint256& /*buzzer*/);
	bool selectBuzzerEndorseTx(const uint256& /*actor*/, const uint256& /*buzzer*/, uint256& /*tx*/);
	bool selectBuzzerMistrustTx(const uint256& /*actor*/, const uint256& /*buzzer*/, uint256& /*tx*/);
	void selectHashTags(const std::string& /*tag*/, std::vector<Buzzer::HashTag>&);

	bool selectBuzzItem(const uint256& /*buzzId*/, BuzzfeedItem& /*item*/);

	static ITransactionStoreExtensionPtr instance(ISettingsPtr settings, ITransactionStorePtr store) {
		return std::make_shared<BuzzerTransactionStoreExtension>(settings, store);
	}

	void removeTransaction(TransactionPtr);

private:
	void processEvent(const uint256&, TransactionContextPtr);
	void processRebuzz(const uint256&, TransactionContextPtr);
	void processSubscribe(const uint256&, TransactionContextPtr);
	void processUnsubscribe(const uint256&, TransactionContextPtr);
	void processLike(const uint256&, TransactionContextPtr);
	void processEndorse(const uint256&, TransactionContextPtr);
	void processMistrust(const uint256&, TransactionContextPtr);

	void incrementLikes(const uint256&);
	void decrementLikes(const uint256&);

	void incrementReplies(const uint256&);
	void decrementReplies(const uint256&);

	void incrementRebuzzes(const uint256&);
	void decrementRebuzzes(const uint256&);

	void incrementMistrusts(const uint256&, amount_t);
	void decrementMistrusts(const uint256&, amount_t);
	
	void incrementEndorsements(const uint256&, amount_t);
	void decrementEndorsements(const uint256&, amount_t);

	void incrementSubscriptions(const uint256&);
	void decrementSubscriptions(const uint256&);

	void incrementFollowers(const uint256&);
	void decrementFollowers(const uint256&);

	void updateBuzzerInfo(const uint256&, const uint256&);

	void prepareBuzzfeedItem(BuzzfeedItem&, TxBuzzPtr, TxBuzzerPtr);
	bool makeBuzzfeedItem(int&, TxBuzzerPtr, TransactionPtr, ITransactionStorePtr, std::multimap<uint64_t, uint256>&, std::map<uint256, BuzzfeedItemPtr>&);
	void makeBuzzfeedLikeItem(TransactionPtr, ITransactionStorePtr, std::multimap<uint64_t, uint256>&, std::map<uint256, BuzzfeedItemPtr>&);

	void prepareEventsfeedItem(EventsfeedItem&, TxBuzzPtr, TxBuzzerPtr, const uint256&, const uint256&, uint64_t, const uint256&, const uint256&, uint64_t, const uint512&);
	void makeEventsfeedItem(TxBuzzerPtr, TransactionPtr, ITransactionStorePtr, std::multimap<uint64_t, EventsfeedItem::Key>&, std::map<EventsfeedItem::Key, EventsfeedItemPtr>&);
	void makeEventsfeedLikeItem(TransactionPtr, ITransactionStorePtr, std::multimap<uint64_t, EventsfeedItem::Key>&, std::map<EventsfeedItem::Key, EventsfeedItemPtr>&);

	template<typename _event> void makeEventsfeedTrustScoreItem(EventsfeedItemPtr item, TransactionPtr tx, unsigned short type, const uint256& buzzer, uint64_t score) {
		//
		std::shared_ptr<_event> lEvent = TransactionHelper::to<_event>(tx);

		item->setType(type);
		item->setTimestamp(lEvent->timestamp());
		item->setBuzzId(lEvent->id());
		item->setBuzzChainId(lEvent->chain());
		item->setValue(lEvent->amount());
		item->setSignature(lEvent->signature());
		item->setPublisher(tx->in()[TX_BUZZER_ENDORSE_BUZZER_IN].out().tx());

		EventsfeedItem::EventInfo lInfo(lEvent->timestamp(), buzzer, lEvent->buzzerInfoChain(), lEvent->buzzerInfo(), score);
		lInfo.setEvent(lEvent->chain(), lEvent->id(), lEvent->signature());
		item->addEventInfo(lInfo);
	}

	template<typename _event> void makeBuzzfeedTrustScoreItem(BuzzfeedItemPtr item, TransactionPtr tx, const uint256& publisher, const uint256& buzzer, TxBuzzerInfoPtr info) {
		//
		std::shared_ptr<_event> lEvent = TransactionHelper::to<_event>(tx);

		item->setType(lEvent->type());
		item->setBuzzerId(publisher);
		item->setBuzzId(lEvent->id());
		item->setBuzzerInfoId(lEvent->buzzerInfo());
		item->setBuzzerInfoChainId(lEvent->buzzerInfoChain());
		item->setBuzzChainId(lEvent->chain());
		item->setValue(lEvent->amount());
		item->setTimestamp(lEvent->timestamp());
		item->setSignature(lEvent->signature());
		item->setScore(lEvent->score());

		item->addItemInfo(BuzzfeedItem::ItemInfo(
			lEvent->timestamp(), lEvent->score(),
			buzzer, info->chain(), info->id(), 
			lEvent->chain(), lEvent->id(), lEvent->signature()
		));
	}

	void publisherUpdates(const uint256& publisher, uint64_t timestamp) {
		//
		uint64_t lTimestamp;
		if (publisherUpdates_.read(publisher, lTimestamp)) {
			//
			if (lTimestamp < timestamp)
				publisherUpdates_.write(publisher, timestamp);
		} else {
			publisherUpdates_.write(publisher, timestamp);
		}
	}

	void subscriberUpdates(const uint256& subscriber, uint64_t timestamp) {
		//
		uint64_t lTimestamp;
		if (subscriberUpdates_.read(subscriber, lTimestamp)) {
			//
			if (lTimestamp < timestamp)
				subscriberUpdates_.write(subscriber, timestamp);
		} else {
			subscriberUpdates_.write(subscriber, timestamp);
		}
	}

	uint160 createPublisherTs(uint64_t timestamp, const uint256& publisher) {
		DataStream lPublisherTsStream(SER_NETWORK, PROTOCOL_VERSION); 
		lPublisherTsStream << timestamp;
		lPublisherTsStream << publisher;
		return Hash160(lPublisherTsStream.begin(), lPublisherTsStream.end());
	}

private:
	ISettingsPtr settings_;
	ITransactionStorePtr store_;
	bool opened_ = false;

	// timeline: publisher | timestamp -> tx
	db::DbTwoKeyContainer<
		uint256 /*publisher*/, 
		uint64_t /*timestamp*/, 
		uint256 /*buzz/reply/rebuzz/like/...*/> timeline_;

	// global timeline: timeframe | score | publisher -> tx
	// NOTICE: this index is not absolute unique, but very close
	db::DbThreeKeyContainer<
		uint64_t /*timeframe*/, 
		uint64_t /*score*/, 
		uint160 /*timestamp+publisher*/, 
		uint256 /*buzz/reply/rebuzz/like/...*/> globalTimeline_;

	// hash-tag indexed timeline: hash | timeframe | score | publisher -> tx
	// NOTICE: this index is not absolute unique, but very close
	db::DbFourKeyContainer<
		uint160 /*hash*/, 
		uint64_t /*timeframe*/, 
		uint64_t /*score*/, 
		uint160 /*timestamp+publisher*/, 
		uint256 /*buzz/reply/rebuzz/like/...*/> hashTagTimeline_;

	// tags map
	db::DbContainer<std::string, std::string> hashTags_;

	// events: time | subscriber | tx -> type
	db::DbThreeKeyContainer<
		uint256 /*subscriber*/, 
		uint64_t /*timestamp*/, 
		uint256 /*tx*/, 
		unsigned short /*type - buzz/reply/rebuzz/like/...*/> events_;

	// subscriber|publisher -> tx
	db::DbTwoKeyContainer<uint256 /*subscriber*/, uint256 /*publisher*/, uint256 /*tx*/> subscriptionsIdx_;
	// subscriber|publisher -> tx
	db::DbTwoKeyContainer<uint256 /*publisher*/, uint256 /*subscriber*/, uint256 /*tx*/> publishersIdx_;
	// buzz | liker -> like_tx
	db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*liker*/, uint256 /*like_tx*/> likesIdx_;
	// buzz | rebuzzer -> rebuzz_tx
	db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*rebuzzer*/, uint256 /*rebuzz_tx*/> rebuzzesIdx_;

	// buzz | reply
	db::DbThreeKeyContainer<
		uint256 /*buzz|rebuzz|reply*/, 
		uint64_t /*timestamp*/, 
		uint256 /*rebuzz|reply*/, 
		uint256 /*publisher*/> replies_;

	// buzz | re-buzz
	// db::DbThreeKeyContainer<uint256 /*buzz chain*/, uint256 /*buzz*/, uint256 /*rebuzz*/, uint256 /*publisher*/> rebuzzes_;

	// buzz | info
	db::DbContainer<uint256 /*buzz*/, BuzzInfo /*buzz info*/> buzzInfo_;
	// buzzer | info
	db::DbContainer<uint256 /*buzzer*/, BuzzerInfo /*buzzer info*/> buzzerStat_;
	// publisher | timestamp
	db::DbContainer<uint256 /*publisher*/, uint64_t /*timestamp*/> publisherUpdates_;
	// subscriber | timestamp
	db::DbContainer<uint256 /*subscriber*/, uint64_t /*timestamp*/> subscriberUpdates_;
	// buzzer | endorser -> tx
	db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/> endorsements_;
	// buzzer | mistruster -> tx
	db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*mistruster*/, uint256 /*tx*/> mistrusts_;
};

typedef std::shared_ptr<BuzzerTransactionStoreExtension> BuzzerTransactionStoreExtensionPtr;

//
class BuzzerTransactionStoreExtensionCreator: public TransactionStoreExtensionCreator {
public:
	BuzzerTransactionStoreExtensionCreator() {}
	ITransactionStoreExtensionPtr create(ISettingsPtr settings, ITransactionStorePtr store) {
		return BuzzerTransactionStoreExtension::instance(settings, store);
	}

	static TransactionStoreExtensionCreatorPtr instance() {
		return std::make_shared<BuzzerTransactionStoreExtensionCreator>();
	}
};

} // qbit

#endif
