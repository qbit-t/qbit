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
#include "conversationsfeed.h"
#include "txbuzzer.h"
#include "txbuzz.h"
#include "txbuzzerendorse.h"

#include <memory>

namespace qbit {

class BuzzerTransactionStoreExtension;
typedef std::shared_ptr<BuzzerTransactionStoreExtension> BuzzerTransactionStoreExtensionPtr;

class BuzzerTransactionStoreExtension: public ITransactionStoreExtension, public std::enable_shared_from_this<BuzzerTransactionStoreExtension> {
public:
	class BuzzInfo {
	public:
		BuzzInfo() {}

		uint32_t replies_ = 0;
		uint32_t rebuzzes_ = 0;
		uint32_t likes_ = 0;
		uint64_t rewards_ = 0;

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(replies_);
			READWRITE(rebuzzes_);
			READWRITE(likes_);
			READWRITE(rewards_);
		}
	};

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

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(endorsements_);
			READWRITE(mistrusts_);
			READWRITE(subscriptions_);
			READWRITE(followers_);
		}

	private:
		uint64_t endorsements_ = BUZZER_TRUST_SCORE_BASE + BUZZER_TRUST_SCORE_BASE / 2; // initial
		uint64_t mistrusts_ = BUZZER_TRUST_SCORE_BASE; // initial
		uint32_t subscriptions_ = 0;
		uint32_t followers_ = 0;
	};

	class ConversationInfo {
	public:
		enum State {
			PENDING = 0,
			ACCEPTED = 1,
			DECLINED = 2
		};

	public:
		ConversationInfo() {}
		ConversationInfo(const uint256& id, const uint256& chain) : 
			id_(id), chain_(chain) {}
		ConversationInfo(const uint256& id, const uint256& chain, ConversationInfo::State state) : 
			id_(id), chain_(chain), state_(state) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(id_);
			READWRITE(chain_);
			READWRITE(stateId_);
			READWRITE(stateChain_);
			READWRITE(messageId_);
			READWRITE(messageChain_);

			if (ser_action.ForRead()) {
				unsigned char lState;
				s >> lState;
				state_ = (ConversationInfo::State)lState;
			} else {
				s << (unsigned char)state_;
			}
		}

		const uint256& id() const { return id_; }
		const uint256& chain() const { return chain_; }
		
		const uint256& stateId() const { return stateId_; } // TX_ACCEPT | TX_DECLINE
		const uint256& stateChain() const { return stateChain_; }
		void setState(const uint256& id, const uint256& chain, ConversationInfo::State state) {
			stateId_ = id;
			stateChain_ = chain;
			state_ = state;
		}

		const uint256& messageId() const { return messageId_; }
		const uint256& messageChain() const { return messageChain_; }
		void setMessage(const uint256& id, const uint256& chain) {
			messageId_ = id;
			messageChain_ = chain;
		}

		ConversationInfo::State state() const { return state_; }
		void setState(ConversationInfo::State state) { state_ = state; }

	private:
		uint256 id_;
		uint256 chain_;
		uint256 stateId_;
		uint256 stateChain_;
		uint256 messageId_;
		uint256 messageChain_;
		State state_ = ConversationInfo::PENDING;
	};

	class GroupInfo {
	public:
		enum Type {
			PUBLIC = 0,
			PRIVATE = 1
		};

	public:
		GroupInfo() {}
		GroupInfo(const uint256& id, const uint256& chain) : 
			id_(id), chain_(chain) {}
		GroupInfo(const uint256& id, const uint256& chain, GroupInfo::Type type) : 
			id_(id), chain_(chain), type_(type) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(id_);
			READWRITE(chain_);
			READWRITE(messageId_);
			READWRITE(messageChain_);
			READWRITE(membersCount_);

			if (ser_action.ForRead()) {
				unsigned char lType;
				s >> lType;
				type_ = (GroupInfo::Type)lType;
			} else {
				s << (unsigned char)type_;
			}
		}

		const uint256& id() const { return id_; }
		const uint256& chain() const { return chain_; }

		const uint256& messageId() const { return messageId_; }
		const uint256& messageChain() const { return messageChain_; }
		void setMessage(const uint256& id, const uint256& chain) {
			messageId_ = id;
			messageChain_ = chain;
		}

		GroupInfo::Type type() const { return type_; }
		void setType(GroupInfo::Type type) { type_ = type; }

		uint32_t membersCount() const { return membersCount_; }
		void setMembersCount(uint32_t membersCount) { membersCount_ = membersCount; }

	private:
		uint256 id_;
		uint256 chain_;
		uint256 messageId_;
		uint256 messageChain_;
		uint32_t membersCount_ = 0;
		Type type_ = GroupInfo::PRIVATE;
	};

public:
	BuzzerTransactionStoreExtension(ISettingsPtr settings, ITransactionStorePtr store) : 
		settings_(settings),
		store_(store),
		space_(std::make_shared<db::DbContainerSpace>(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/common")),
		timeline_("buzzer_timeline", space_),
		conversations_("buzzer_conversations", space_),
		conversationsIndex_("buzzer_indexes_conversations", space_),
		conversationsOrder_("buzzer_indexes_conversations_order", space_),
		conversationsActivity_("buzzer_indexes_conversations_activity", space_),
		groups_("buzzer_groups", space_),
		groupInfo_("buzzer_group_info", space_),
		membership_("buzzer_group_membership", space_),
		groupInvitations_("buzzer_group_invitations", space_),
		globalTimeline_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/global_timeline"),
		hashTagTimeline_("buzzer_hashed_timeline", space_),
		hashTagUpdates_("buzzerhash_tag_updates", space_),
		hashTags_("buzzer_hash_tags", space_),
		events_("buzzer_events", space_), 
		subscriptionsIdx_("buzzer_indexes_subscriptions", space_),
		publishersIdx_("buzzer_indexes_publishers", space_),
		likesIdx_("buzzer_indexes_likes", space_),
		rebuzzesIdx_("buzzer_indexes_rebuzzes", space_),
		publisherUpdates_("buzzer_indexes_publisher_updates", space_),
		subscriberUpdates_("buzzer_indexes_subscriber_updates", space_),
		replies_("buzzer_replies", space_),
		//rebuzzes_("buzzer_rebuzzes", space_),
		endorsements_("buzzer_endorsements", space_),
		mistrusts_("buzzer_mistrusts", space_),
		blocks_("buzzer_blocks", space_),
		buzzInfo_("buzzer_buzz_info", space_),
		buzzerStat_("buzzer_buzzer_stat", space_),
		buzzerInfo_("buzzer_buzzer_info", space_),
		hiddenIdx_("buzzer_indexes_hidden", space_)
		{}

	bool open();
	bool close();

	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) { return false; }
	bool popUnlinkedOut(const uint256&, TransactionContextPtr) { return false; }
	bool pushEntity(const uint256&, TransactionContextPtr);
	bool isAllowed(TransactionContextPtr);
	bool locateParents(TransactionContextPtr /*root*/, std::list<uint256>& /*parents*/);
	void processBuzzerStat(const uint256&);

	TransactionPtr locateSubscription(const uint256& /*subscriber*/, const uint256& /*publisher*/);
	void selectBuzzfeed(const std::vector<BuzzfeedPublisherFrom>& /*from*/, const uint256& /*subscriber*/, std::vector<BuzzfeedItem>& /*feed*/);
	void selectBuzzfeedGlobal(uint64_t /*timeframeFrom*/, uint64_t /*scoreFrom*/, uint64_t /*timestampFrom*/, const uint256& /*publisher*/, const uint256& /*subscriber*/, std::vector<BuzzfeedItem>& /*feed*/);
	void selectBuzzfeedByTag(const std::string& /*tag*/, uint64_t /*timeframeFrom*/, uint64_t /*scoreFrom*/, uint64_t /*timestampFrom*/, const uint256& /*publisher*/, const uint256& /*subscriber*/, std::vector<BuzzfeedItem>& /*feed*/);
	void selectBuzzfeedByBuzz(uint64_t /*from*/, const uint256& /*buzz*/, const uint256& /*subscriber*/, std::vector<BuzzfeedItem>& /*feed*/);
	void selectBuzzfeedByBuzzer(uint64_t /*from*/, const uint256& /*buzzer*/, const uint256& /*subscriber*/, std::vector<BuzzfeedItem>& /*feed*/);
	void selectEventsfeed(uint64_t /*from*/, const uint256& /*buzzer*/, std::vector<EventsfeedItem>& /*feed*/);
	void selectMistrusts(const uint256& /*from*/, const uint256& /*buzzer*/, std::vector<EventsfeedItem>& /*feed*/);
	void selectEndorsements(const uint256& /*from*/, const uint256& /*buzzer*/, std::vector<EventsfeedItem>& /*feed*/);
	void selectSubscriptions(const uint256& /*from*/, const uint256& /*buzzer*/, std::vector<EventsfeedItem>& /*feed*/);
	void selectFollowers(const uint256& /*from*/, const uint256& /*buzzer*/, std::vector<EventsfeedItem>& /*feed*/);
	void selectConversations(uint64_t /*from*/, const uint256& /*buzzer*/, std::vector<ConversationItem>& /*feed*/);
	void selectMessages(uint64_t /*from*/, const uint256& /*conversation*/, std::vector<BuzzfeedItem>& /*feed*/);
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
	void processReward(const uint256&, TransactionContextPtr);
	void processEndorse(const uint256&, TransactionContextPtr);
	void processMistrust(const uint256&, TransactionContextPtr);
	void processConversation(const uint256&, TransactionContextPtr);
	void processAcceptConversation(const uint256&, TransactionContextPtr);
	void processDeclineConversation(const uint256&, TransactionContextPtr);
	void processGroupInvite(const uint256&, TransactionContextPtr);
	void processGroupAcceptInvitation(const uint256&, TransactionContextPtr);
	void processGroupDeclineInvitation(const uint256&, TransactionContextPtr);
	void processMessage(const uint256&, TransactionContextPtr);
	void processHide(const uint256&, TransactionContextPtr);
	void processBuzzerHide(const uint256&, TransactionContextPtr);
	void processBuzzerBlock(const uint256&, TransactionContextPtr);
	void processBuzzerUnBlock(const uint256&, TransactionContextPtr);

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

	void incrementRewards(const uint256&, amount_t);
	void decrementRewards(const uint256&, amount_t);

	void incrementSubscriptions(const uint256&);
	void decrementSubscriptions(const uint256&);

	void incrementFollowers(const uint256&);
	void decrementFollowers(const uint256&);

	void updateBuzzerInfo(const uint256&, const uint256&);

	void prepareBuzzfeedItem(BuzzfeedItem&, TxBuzzPtr, TxBuzzerPtr);
	BuzzfeedItemPtr makeBuzzfeedItem(int&, TxBuzzerPtr, TransactionPtr, ITransactionStorePtr, std::multimap<uint64_t, BuzzfeedItem::Key>&, std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>&, const uint256&, bool expand = true);
	void makeBuzzfeedLikeItem(TransactionPtr, ITransactionStorePtr, std::multimap<uint64_t, BuzzfeedItem::Key>&, std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>&, const uint256&);
	void makeBuzzfeedRewardItem(TransactionPtr, ITransactionStorePtr, std::multimap<uint64_t, BuzzfeedItem::Key>&, std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>&, const uint256&);
	void makeBuzzfeedRebuzzItem(TransactionPtr, ITransactionStorePtr, std::multimap<uint64_t, BuzzfeedItem::Key>&, std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>&, const uint256&);

	void prepareEventsfeedItem(EventsfeedItem&, TxBuzzPtr, TxBuzzerPtr, const uint256&, const uint256&, uint64_t, const uint256&, const uint256&, uint64_t, const uint512&);
	EventsfeedItemPtr makeEventsfeedItem(TxBuzzerPtr, TransactionPtr, ITransactionStorePtr, std::multimap<uint64_t, EventsfeedItem::Key>&, std::map<EventsfeedItem::Key, EventsfeedItemPtr>&);
	void makeEventsfeedLikeItem(TransactionPtr, ITransactionStorePtr, std::multimap<uint64_t, EventsfeedItem::Key>&, std::map<EventsfeedItem::Key, EventsfeedItemPtr>&);
	void makeEventsfeedRewardItem(EventsfeedItemPtr item, TransactionPtr tx, const uint256& buzz, const uint256& buzzChain, const uint256& buzzer);

	void fillEventsfeed(unsigned short /*type*/, TransactionPtr /*tx*/, const uint256& /*subscriber*/, std::multimap<uint64_t, EventsfeedItem::Key>&, std::map<EventsfeedItem::Key, EventsfeedItemPtr>&);

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

	void hashTagUpdate(const uint160&, const db::FiveKey<uint160, uint64_t, uint64_t, uint64_t, uint256>&);

	BuzzerTransactionStoreExtensionPtr locateBuzzerExtension(const uint256& /*buzzer*/);

private:
	ISettingsPtr settings_;
	ITransactionStorePtr store_;
	bool opened_ = false;

	//
	// container space
	//

	db::DbContainerSpacePtr space_;

	// timeline: publisher | timestamp -> tx
	db::DbTwoKeyContainerShared<
		uint256 /*publisher | conversation*/, 
		uint64_t /*timestamp*/, 
		uint256 /*buzz/reply/rebuzz/like/message...*/> timeline_;

	//
	// conversations & groups
	//

	// buzzer | conversation || group
	db::DbTwoKeyContainerShared<uint256 /*buzzer*/, uint256 /*conversation*/, ConversationInfo /*state*/> conversations_;

	// buzzer conversations timestamp
	db::DbTwoKeyContainerShared<
		uint256 /*buzzer*/, 
		uint256 /*conversation || group*/,
		uint64_t /*timestamp*/> conversationsIndex_;

	// conversations ordering by timestamp
	db::DbTwoKeyContainerShared<
		uint256 /*buzzer*/, 
		uint64_t /*timestamp*/,
		uint256 /*conversation || group*/> conversationsOrder_;
	
	// buzzer last activity
	db::DbContainerShared<uint256 /*buzzer*/, uint64_t /*timestamp*/> conversationsActivity_;

	// groups
	db::DbTwoKeyContainerShared<
		uint256 /*group*/, 
		uint256 /*member*/, 
		uint256 /*confirmation || 0x00*/> groups_;

	// group info
	db::DbContainerShared<uint256 /*group*/, GroupInfo /*group info*/> groupInfo_;

	// groups membership idx
	db::DbTwoKeyContainerShared<
		uint256 /*buzzer*/,
		uint256 /*group*/,
		uint256 /*confirmation*/> membership_;

	// group invitations
	db::DbThreeKeyContainerShared<
		uint256 /*inviter*/,
		uint256 /*group*/,
		uint256 /*invitee*/,
		uint256 /*invitation*/> groupInvitations_;

	//

	// global timeline: timeframe | score | publisher -> tx
	// NOTICE(1): this index is not absolute unique, but very close
	// NOTICE(2): that is separate db container
	db::DbFourKeyContainer<
		uint64_t /*timeframe*/,
		uint64_t /*score*/,
		uint64_t /*timestamp*/,
		uint256 /*publisher*/,
		uint256 /*buzz/reply/rebuzz/like/...*/> globalTimeline_;

	// hash-tag indexed timeline: hash | timeframe | score | publisher -> tx
	// NOTICE: this index is not absolute unique, but very close
	db::DbFiveKeyContainerShared<
		uint160 /*hash*/, 
		uint64_t /*timeframe*/, 
		uint64_t /*score*/, 
		uint64_t /*timestamp*/,
		uint256 /*publisher*/, 
		uint256 /*buzz/reply/rebuzz/like/...*/> hashTagTimeline_;

	// last tag-key
	db::DbContainerShared<uint160 /*hash*/, db::FiveKey<
		uint160 /*hash*/, 
		uint64_t /*timeframe*/, 
		uint64_t /*score*/, 
		uint64_t /*timestamp*/,
		uint256 /*publisher*/>> hashTagUpdates_;

	// tags map
	db::DbContainerShared<std::string, std::string> hashTags_;

	// events: time | subscriber | tx -> type
	db::DbThreeKeyContainerShared<
		uint256 /*subscriber*/, 
		uint64_t /*timestamp*/, 
		uint256 /*tx*/, 
		unsigned short /*type - buzz/reply/rebuzz/like/...*/> events_;

	// subscriber|publisher -> tx
	db::DbTwoKeyContainerShared<uint256 /*subscriber*/, uint256 /*publisher*/, uint256 /*tx*/> subscriptionsIdx_;
	// subscriber|publisher -> tx
	db::DbTwoKeyContainerShared<uint256 /*publisher*/, uint256 /*subscriber*/, uint256 /*tx*/> publishersIdx_;
	// buzz | liker -> like_tx
	db::DbTwoKeyContainerShared<uint256 /*buzz|rebuzz|reply*/, uint256 /*liker*/, uint256 /*like_tx*/> likesIdx_;
	// buzz | rebuzzer -> rebuzz_tx
	db::DbTwoKeyContainerShared<uint256 /*buzz|rebuzz|reply*/, uint256 /*rebuzzer*/, uint256 /*rebuzz_tx*/> rebuzzesIdx_;
	// buzz | owner
	db::DbContainerShared<uint256 /*buzz|rebuzz|reply*/, uint256 /*buzzer*/> hiddenIdx_;

	// buzz | reply
	db::DbThreeKeyContainerShared<
		uint256 /*buzz|rebuzz|reply*/, 
		uint64_t /*timestamp*/, 
		uint256 /*rebuzz|reply*/, 
		uint256 /*publisher*/> replies_;

	// buzz | re-buzz
	// db::DbThreeKeyContainer<uint256 /*buzz chain*/, uint256 /*buzz*/, uint256 /*rebuzz*/, uint256 /*publisher*/> rebuzzes_;

	// buzz | info
	db::DbContainerShared<uint256 /*buzz*/, BuzzInfo /*buzz info*/> buzzInfo_;
	// buzzer | info
	db::DbContainerShared<uint256 /*buzz*/, uint256 /*buzz info*/> buzzerInfo_;
	// buzzer | stat
	db::DbContainerShared<uint256 /*buzzer*/, BuzzerInfo /*buzzer info*/> buzzerStat_;
	// publisher | timestamp
	db::DbContainerShared<uint256 /*publisher*/, uint64_t /*timestamp*/> publisherUpdates_;
	// subscriber | timestamp
	db::DbContainerShared<uint256 /*subscriber*/, uint64_t /*timestamp*/> subscriberUpdates_;
	// buzzer | endorser -> tx
	db::DbTwoKeyContainerShared<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/> endorsements_;
	// buzzer | mistruster -> tx
	db::DbTwoKeyContainerShared<uint256 /*buzzer*/, uint256 /*mistruster*/, uint256 /*tx*/> mistrusts_;
	// buzzer | blocker -> tx
	db::DbTwoKeyContainerShared<uint256 /*buzzer*/, uint256 /*blocker*/, uint256 /*tx*/> blocks_;
};

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
