// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZ_FEED_H
#define QBIT_BUZZ_FEED_H

#include "../../ipeerextension.h"
#include "../../ipeermanager.h"
#include "../../ipeer.h"
#include "../../irequestprocessor.h"
#include "../../message.h"
#include "../../tinyformat.h"
#include "../../client/handlers.h"

#include "buzzer.h"
#include "txbuzzer.h"
#include "txbuzzerinfo.h"

#include <boost/function.hpp>

namespace qbit {

//
// buzzfeed timeframe for trust score adjustements
#define BUZZFEED_TIMEFRAME (5*60*1000000) // 5 min

//
// number of confirmations from different peers for realtime feed
#define BUZZ_PEERS_CONFIRMATIONS 2 // default

//
// forward
class BuzzfeedItem;
typedef std::shared_ptr<BuzzfeedItem> BuzzfeedItemPtr;

//
// resolver functions
typedef boost::function<void (const uint256& /*buzzerInfoId*/, std::string& /*name*/, std::string& /*alias*/)> buzzerInfoFunction;
typedef boost::function<bool (const uint256& /*buzzerChainId*/, const uint256& /*buzzerId*/, const uint256& /*buzzerInfoId*/, buzzerInfoReadyFunction /*readyFunction*/)> buzzerInfoResolveFunction;

//
// buzzfeed callbacks
typedef boost::function<void (void)> buzzfeedLargeUpdatedFunction;
typedef boost::function<void (BuzzfeedItemPtr)> buzzfeedItemNewFunction;
typedef boost::function<void (BuzzfeedItemPtr)> buzzfeedItemReadyFunction;
typedef boost::function<void (BuzzfeedItemPtr)> buzzfeedItemUpdatedFunction;
typedef boost::function<void (const uint256&, const uint256&)> buzzfeedItemAbsentFunction;
typedef boost::function<Buzzer::VerificationResult (BuzzfeedItemPtr)> buzzfeedItemVerifyFunction;
typedef boost::function<void (BuzzfeedItemPtr)> buzzerInfoLoadedFunction;
typedef boost::function<Buzzer::VerificationResult (const uint256&, uint64_t, const uint512&)> verifyUpdateForMyThreadFunction;
typedef boost::function<bool (const uint256&, PKey&)> buzzfeedItemResolvePKeyFunction;

//
// item info
#define BUZZ_PROPERY_REPLY   0x01
#define BUZZ_PROPERY_LIKE    0x02
#define BUZZ_PROPERY_REBUZZ  0x04
#define BUZZ_PROPERY_REWARD  0x08
#define BUZZ_PROPERY_MEMPOOL 0x10

//
// buzzfeed item update
class BuzzfeedItemUpdate {
public:
	enum Field {
		NONE = 0,
		LIKES = 1,
		REBUZZES = 2,
		REPLIES = 3,
		REWARDS = 4,
		LINKS = 5,
		WRAPPED = 6
	};
public:
	BuzzfeedItemUpdate() { field_ = NONE; count_ = 0; }
	BuzzfeedItemUpdate(const uint256& buzzId, const uint256& eventId, Field field, uint64_t count):
		buzzId_(buzzId), eventId_(eventId), field_(field), count_(count) {}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		//
		READWRITE(buzzId_);
		READWRITE(eventId_);

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
	const uint256& eventId() const { return eventId_; }
	Field field() const { return field_; }
	std::string fieldString() const {
		if (field_ == LIKES) return "LIKES";
		else if (field_ == REBUZZES) return "REBUZZES";
		else if (field_ == REPLIES) return "REPLIES";
		else if (field_ == REWARDS) return "REWARDS";
		else if (field_ == LINKS) return "LINKS";
		else if (field_ == WRAPPED) return "WRAPPED";
		return "NONE";
	}
	uint64_t count() const { return count_; }

private:
	uint256 buzzId_;
	uint256 eventId_;
	Field field_;
	uint64_t count_ = 0;
};

class BuzzfeedPublisherFrom {
public:
	BuzzfeedPublisherFrom() { publisher_.setNull(); from_ = 0; }
	BuzzfeedPublisherFrom(const uint256& publisher, uint64_t from):
		publisher_(publisher), from_(from) {}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		//
		READWRITE(publisher_);
		READWRITE(from_);
	}

	const uint256& publisher() const { return publisher_; }
	uint64_t from() const { return from_; }

private:
	uint256 publisher_;
	uint64_t from_ = 0;
};

//
typedef boost::function<void (const std::vector<BuzzfeedItemUpdate>&)> buzzfeedItemsUpdatedFunction;

//
// buzzfeed item
class BuzzfeedItem:	public std::enable_shared_from_this<BuzzfeedItem> {
public:
	enum Source {
		//
		// BUZZFEED - naturally picked up by the persistent subscriptions and interlinked buzzes
		BUZZFEED,
		//
		// BUZZ_SUBSCRIPTION - picked up by dynamic subscription (only for realtime feeds) 
		BUZZ_SUBSCRIPTION
	};

	enum Merge {
		//
		// UNION - unite feeds, which comes from different nodes
		UNION,
		//
		// INTERSECT - make an intersaction of feeds from different sources
		INTERSECT
	};

	enum Order {
		//
		FORWARD,
		//
		REVERSE
	};

	enum Group {
		//
		TIMESTAMP,
		//
		TIMEFRAME_SCORE
	};

	enum Expand {
		//
		FULL,
		//
		SMART
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
			return strprintf("%s|%s", id_.toHex(), typeToString());
		}

		std::string typeToString() const {
			if (type_ == TX_BUZZ) return "BUZZ";
			else if (type_ == TX_BUZZ_REPLY) return "REPLY";
			else if (type_ == TX_REBUZZ) return "REBUZZ";
			else if (type_ == TX_BUZZ_LIKE) return "LIKE";
			else if (type_ == TX_BUZZ_REWARD) return "REWARD";
			else if (type_ == TX_BUZZER_MISTRUST) return "MISTRUST";
			else if (type_ == TX_BUZZER_ENDORSE) return "ENDORSE";
			else if (type_ == TX_BUZZER_MESSAGE) return "MESSAGE";
			return "NONE";
		}		

	private:
		uint256 id_;
		unsigned short type_;
	};

public:
	class OrderKey {
	public:
		OrderKey(Group key, uint64_t timestamp):
			key_(key), timestamp_(timestamp) {}

		OrderKey(Group key, uint64_t timeframe, uint64_t score, uint64_t timestamp):
			key_(key), timeframe_(timeframe), score_(score), timestamp_(timestamp) {}

		friend bool operator < (const OrderKey& a, const OrderKey& b) {
			if (a.key() == TIMEFRAME_SCORE || b.key() == TIMEFRAME_SCORE) {
				if (a.timeframe() < b.timeframe()) return true;
				if (a.timeframe() > b.timeframe()) return false;

				if (a.score() < b.score()) return true;
				if (a.score() > b.score()) return false;

				if (a.timestamp() < b.timestamp()) return true;
				if (a.timestamp() > b.timestamp()) return false;
			} else {
				if (a.timestamp() < b.timestamp()) return true;
				if (a.timestamp() > b.timestamp()) return false;
			}

			return false;
		}

		friend bool operator == (const OrderKey& a, const OrderKey& b) {
			return (a.timeframe() == b.timeframe() && a.score() == b.score());
		}

		uint64_t timeframe() const { return timeframe_; }
		uint64_t score() const { return score_; }
		uint64_t timestamp() const { return timestamp_; }
		Group key() const { return key_; }
		std::string keyString() const {
			if (key_ == TIMESTAMP) return "TIMESTAMP";
			return "TIMEFRAME_SCORE";
		}

	private:
		Group key_ = TIMESTAMP;
		uint64_t timeframe_ = 0;
		uint64_t score_ = 0;
		uint64_t timestamp_ = 0;
	};

public:
	class ItemInfo {
	public:
		ItemInfo() {}
		ItemInfo(uint64_t timestamp, uint64_t score, const uint256& buzzerId, const uint256& buzzerInfoChainId, 
			const uint256& buzzerInfoId, const uint256& eventChainId, const uint256& eventId, const uint512& signature): 
			timestamp_(timestamp), score_(score), buzzerId_(buzzerId), buzzerInfoChainId_(buzzerInfoChainId), buzzerInfoId_(buzzerInfoId),
			eventChainId_(eventChainId), eventId_(eventId), signature_(signature) {
		}

		ItemInfo(uint64_t timestamp, uint64_t score, uint64_t amount, const uint256& buzzerId, const uint256& buzzerInfoChainId, 
			const uint256& buzzerInfoId, const uint256& eventChainId, const uint256& eventId, const uint512& signature): 
			timestamp_(timestamp), score_(score), amount_(amount), buzzerId_(buzzerId), buzzerInfoChainId_(buzzerInfoChainId), buzzerInfoId_(buzzerInfoId),
			eventChainId_(eventChainId), eventId_(eventId), signature_(signature) {
		}

		uint64_t timestamp() const { return timestamp_; }
		uint64_t score() const { return score_; }
		uint64_t amount() const { return amount_; }
		const uint256& eventId() const { return eventId_; } 
		const uint256& eventChainId() const { return eventChainId_; } 
		const uint256& buzzerId() const { return buzzerId_; }
		const uint256& buzzerInfoId() const { return buzzerInfoId_; }
		const uint256& buzzerInfoChainId() const { return buzzerInfoChainId_; }
		const uint512& signature() const { return signature_; }

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(timestamp_);
			READWRITE(score_);
			READWRITE(eventId_);
			READWRITE(eventChainId_);
			READWRITE(buzzerId_);
			READWRITE(buzzerInfoId_);
			READWRITE(buzzerInfoChainId_);
			READWRITE(amount_);
			READWRITE(signature_);
		}

	protected:
		uint64_t timestamp_ = 0;
		uint64_t score_ = 0;
		uint256 eventId_;
		uint256 eventChainId_;
		uint256 buzzerId_;
		uint256 buzzerInfoId_;
		uint256 buzzerInfoChainId_;
		uint64_t amount_ = 0;
		uint512 signature_;	
	};

public:
	BuzzfeedItem() {
		type_ = Transaction::UNDEFINED;
		source_ = Source::BUZZFEED;
		buzzId_.setNull();
		buzzChainId_.setNull();
		buzzerInfoId_.setNull();
		buzzerInfoChainId_.setNull();
		timestamp_ = 0;
		order_ = 0;
		buzzerId_.setNull();
		replies_ = 0;
		rebuzzes_ = 0;
		likes_ = 0;
		rewards_ = 0;
		value_ = 0;
		score_ = 0;
		originalBuzzChainId_.setNull();
		originalBuzzId_.setNull();
		signature_.setNull();
		subscriptionSignature_.setNull();
		rootBuzzId_.setNull();
	}
	~BuzzfeedItem() {
		wrapped_.reset();
	}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(type_);
		READWRITE(source_);
		READWRITE(buzzId_);
		READWRITE(buzzChainId_);
		READWRITE(timestamp_);
		READWRITE(score_);
		READWRITE(buzzerId_);
		READWRITE(buzzerInfoId_);
		READWRITE(buzzerInfoChainId_);
		READWRITE(buzzBody_);
		READWRITE(buzzMedia_);
		READWRITE(signature_);

		if (type_ != TX_BUZZER_MISTRUST && type_ != TX_BUZZER_ENDORSE && type_ != TX_BUZZER_SUBSCRIBE) {
			READWRITE(replies_);
			READWRITE(rebuzzes_);
			READWRITE(likes_);
			READWRITE(rewards_);
		}

		READWRITE(order_);

		if (type_ == TX_BUZZ_REPLY || type_ == TX_REBUZZ) {
			READWRITE(originalBuzzChainId_);
			READWRITE(originalBuzzId_);
		}

		if (type_ == TX_REBUZZ || type_ == TX_BUZZ_LIKE || type_ == TX_BUZZ_REWARD || 
			type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE)
			READWRITE(infos_);

		if (type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			READWRITE(value_);
		}

		if (type_ == TX_BUZZER_MESSAGE || type_ == TX_BUZZER_MESSAGE_REPLY) {
			READWRITE(rootBuzzId_);
		}

		if (source_ != Source::BUZZFEED)
			READWRITE(subscriptionSignature_);

		READWRITE(properties_);

		loaded();
	}

	virtual void loaded() {}

	Key key() const { return Key(buzzId_, type_); }

	OrderKey order() const {
		//
		uint64_t lOrder;
		if (order_) lOrder = order_; 
		else lOrder = timestamp_; 

		if (type_ == TX_BUZZ_REPLY) return OrderKey(TIMESTAMP, lOrder);
		return OrderKey(key_, (key_ == TIMEFRAME_SCORE ? lOrder/BUZZFEED_TIMEFRAME : lOrder), score_, lOrder);
	}

	uint64_t actualOrder() const {
		if (order_) return order_;
		return timestamp_;
	}

	void setOrder(uint64_t order) { order_ = order; }

	unsigned short type() const { return type_; }
	void setType(unsigned short type) { type_ = type; }

	unsigned char source() const { return source_; }
	void setSource(unsigned char source) { source_ = source; }

	uint64_t value() const { return value_; }
	void setValue(uint64_t value) { value_ = value; }

	const uint256& buzzId() const { return buzzId_; }
	void setBuzzId(const uint256& id) { buzzId_ = id; }

	bool threaded() { return threaded_; }
	void resetThreaded() { threaded_ = false; }

	const uint256& originalBuzzChainId() const { return originalBuzzChainId_; }
	void setOriginalBuzzChainId(const uint256& id) { originalBuzzChainId_ = id; }

	const uint256& originalBuzzId() const { return originalBuzzId_; }
	void setOriginalBuzzId(const uint256& id) { originalBuzzId_ = id; }

	bool addItemInfo(const ItemInfo& info) {
		if (info_.insert(info.buzzerId()).second) {
			infos_.push_back(info);

			addExtraInfo(info);
			return true;
		}

		return false;
	}

	virtual void addExtraInfo(const ItemInfo&) {}

	const std::vector<ItemInfo>& infos() const { return infos_; }

	bool mergeInfos(const std::vector<ItemInfo>& infos) {
		//
		if (infos_.size() && !info_.size()) {
			for (std::vector<ItemInfo>::iterator lInfo = infos_.begin(); lInfo != infos_.end(); lInfo++) {
				info_.insert(lInfo->buzzerId());
				// updateTimestamp(lInfo->buzzerId(), lInfo->timestamp());
			}

			infosUpdated();
			return true;
		}

		//
		if (infos_.size() > 50) {
			infosCount_++;
			infosUpdated();
			return true;
		}

		bool lResult = false;
		for (std::vector<ItemInfo>::const_iterator lBuzzer = infos.begin(); lBuzzer != infos.end(); lBuzzer++) {
			//
			if (addItemInfo(*lBuzzer)) lResult = true;
			if (lBuzzer->timestamp() > order_) order_ = lBuzzer->timestamp();

			// updateTimestamp(lBuzzer->buzzerId(), lBuzzer->timestamp());
		}

		infosUpdated();

		return lResult;
	}

	virtual void infosUpdated() {}

	const uint256& buzzChainId() const { return buzzChainId_; }
	void setBuzzChainId(const uint256& id) { buzzChainId_ = id; }

	uint64_t timestamp() const { return timestamp_; }
	void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	uint64_t score() const { return score_; }
	void setScore(uint64_t score) { score_ = score; }

	const uint256& buzzerId() const { return buzzerId_; } 
	void setBuzzerId(const uint256& id) { buzzerId_ = id; } 

	const uint256& buzzerInfoId() const { return buzzerInfoId_; }
	void setBuzzerInfoId(const uint256& id) { buzzerInfoId_ = id; }

	const uint256& buzzerInfoChainId() const { return buzzerInfoChainId_; }
	void setBuzzerInfoChainId(const uint256& id) { buzzerInfoChainId_ = id; }

	const std::vector<unsigned char>& buzzBody() const;
	std::string buzzBodyString();
	std::string decryptedBuzzBodyString();
	void setBuzzBody(const std::vector<unsigned char>& body) { buzzBody_ = body; }
	bool decrypt(const PKey&);

	const std::vector<BuzzerMediaPointer>& buzzMedia() const { return buzzMedia_; } 

	void setBuzzMedia(const std::vector<BuzzerMediaPointer>& media) {
		buzzMedia_.insert(buzzMedia_.end(), media.begin(), media.end());
	}

	const uint512& signature() const { return signature_; }
	void setSignature(const uint512& signature) { signature_ = signature; }

	const uint512& subscriptionSignature() const { return subscriptionSignature_; }
	void setSubscriptionSignature(const uint512& subscriptionSignature) { subscriptionSignature_ = subscriptionSignature; }

	uint32_t replies() const { return replies_; }
	bool setReplies(uint32_t v) { if (replies_ != v) { replies_ = v; return true; } return false; }

	uint32_t rebuzzes() const { return rebuzzes_; }
	bool setRebuzzes(uint32_t v) { if (rebuzzes_ != v) { rebuzzes_ = v; return true; } return false; }

	uint32_t likes() const { return likes_; }
	bool setLikes(uint32_t v) { if (likes_ != v) { likes_ = v; return true; } return false; }

	uint64_t rewards() const { return rewards_; }
	bool setRewards(uint64_t v) { if (rewards_ != v) { rewards_ = v; return true; } return false; }

	size_t confirmations() { return confirmed_.size(); }
	size_t addConfirmation(const uint160& peer) {
		confirmed_.insert(peer);
		return confirmed_.size();
	}

	void setReply() {
		checkProperties();
		properties_[0] |= BUZZ_PROPERY_REPLY;
	}
	bool hasReply() { checkProperties(); return (properties_[0] & BUZZ_PROPERY_REPLY) != 0; }

	void setRebuzz() {
		checkProperties();
		properties_[0] |= BUZZ_PROPERY_REBUZZ;
	}
	bool hasRebuzz() { checkProperties(); return (properties_[0] & BUZZ_PROPERY_REBUZZ) != 0; }

	void setLike() {
		checkProperties();
		properties_[0] |= BUZZ_PROPERY_LIKE;
	}
	bool hasLike() { checkProperties(); return (properties_[0] & BUZZ_PROPERY_LIKE) != 0; }

	void setReward() {
		checkProperties();
		properties_[0] |= BUZZ_PROPERY_REWARD;
	}
	bool hasReward() { checkProperties(); return (properties_[0] & BUZZ_PROPERY_REWARD) != 0; }

	void setMempool() {
		checkProperties();
		properties_[0] |= BUZZ_PROPERY_MEMPOOL;
	}
	bool hasMempool() { checkProperties(); return (properties_[0] & BUZZ_PROPERY_MEMPOOL) != 0; }

	void setNonce(uint64_t nonce) { nonce_ = nonce; }
	void setRootBuzzId(const uint256& root) { rootBuzzId_ = root; }
	const uint256& rootBuzzId() const { return rootBuzzId_; }

	virtual void likesChanged() {}
	virtual void rebuzzesChanged() {}
	virtual void repliesChanged() {}
	virtual void rewardsChanged() {}

	void setParent(BuzzfeedItemPtr parent) { parent_ = parent; }
	void setRoot(BuzzfeedItemPtr root) { root_ = root; }

	void buzzerInfo(std::string& name, std::string& alias) {
		buzzerInfo_(buzzerInfoId_, name, alias); 
	}

	void buzzerInfo(const ItemInfo& info, std::string& name, std::string& alias) {
		buzzerInfo_(info.buzzerInfoId(), name, alias); 
	}

	static BuzzfeedItemPtr instance(const BuzzfeedItem& item) {
		return std::make_shared<BuzzfeedItem>(item);
	}

	static BuzzfeedItemPtr instance() {
		return std::make_shared<BuzzfeedItem>();
	}

	BuzzfeedItem& self() {
		return *this;
	}

	void merge(const BuzzfeedItemUpdate&);
	void merge(const std::vector<BuzzfeedItemUpdate>&);
	bool merge(const BuzzfeedItem&, bool checkSize = true, bool notify = true);
	void merge(const std::vector<BuzzfeedItem>&, bool notify = false);
	bool mergeAppend(const std::vector<BuzzfeedItemPtr>&);
	void push(const BuzzfeedItem&, const uint160&);

	virtual void feed(std::vector<BuzzfeedItemPtr>&, bool expanded = false);
	virtual int locateIndex(BuzzfeedItemPtr);
	virtual BuzzfeedItemPtr locateBuzz(const Key& key) {
		if (root_) root_->locateBuzz(key);
	}
	virtual BuzzfeedItemPtr locateBuzz(const uint256&);

	void setSortOrder(Order sortOrder) { sortOrder_  = sortOrder; }
	void setGroupKey(Group key) { key_ = key; }

	void setResolver(buzzerInfoResolveFunction buzzerInfoResolve, buzzerInfoFunction buzzerInfo) {
		buzzerInfoResolve_ = buzzerInfoResolve;
		buzzerInfo_ = buzzerInfo;
	}

	std::string& buzzerName() {
		//
		if (!buzzerName_.length()) resolveBuzzerName();
		return buzzerName_;
	}

	std::string& buzzerAlias() {
		//
		if (!buzzerAlias_.length()) resolveBuzzerName();
		return buzzerAlias_;
	}

	void resolveBuzzerName() {
		buzzerName_ = strprintf("<%s>", buzzerId_.toHex());
		buzzerInfo(buzzerName_, buzzerAlias_);

		if (!buzzerAlias_.size()) buzzerAlias_ = "?";
	}

	bool resolve() {
		bool lResolved = true;
		if (!buzzerInfoResolve_(
				buzzerInfoChainId_, 
				buzzerId_, 
				buzzerInfoId_,
				boost::bind(&BuzzfeedItem::buzzerInfoReady, shared_from_this(), boost::placeholders::_1))) {
			//
			lResolved = false;
		}

		if (type_ == TX_REBUZZ || type_ == TX_BUZZ_LIKE || type_ == TX_BUZZ_REWARD || type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			for (std::vector<ItemInfo>::const_iterator lItem = infos_.begin(); lItem != infos_.end(); lItem++) {
				if (!buzzerInfoResolve_(
						lItem->buzzerInfoChainId(), 
						lItem->buzzerId(), 
						lItem->buzzerInfoId(),
						boost::bind(&BuzzfeedItem::buzzerInfoReady, shared_from_this(), boost::placeholders::_1))) {
					//
					lResolved = false;
				}
			}
		}

		return lResolved;
	}

	void buzzerInfoReady(const uint256& /*info*/) {
		//
		if (fed()) {
			insertNewItem(shared_from_this());
			//
			itemNew(shared_from_this());
		}
	}

	std::string infosToString() {
		//
		if (!infos_.size()) return "?";
		
		ItemInfo& lInfo = *infos_.begin();

		std::string lBuzzerName = strprintf("<%s>", lInfo.buzzerId().toHex());
		std::string lBuzzerAlias;
		buzzerInfo(lInfo, lBuzzerName, lBuzzerAlias);

		if (!lBuzzerAlias.size()) lBuzzerAlias = "?";

		if (type_ == TX_BUZZER_MISTRUST) {
			return strprintf("%s (%s) mistrusted on %s", lBuzzerAlias, lBuzzerName, 
				strprintf(TxAssetType::scaleFormat(QBIT), (double)value_ / (double)QBIT));
		} else if (type_ == TX_BUZZER_ENDORSE) {
			return strprintf("%s (%s) endorsed on %s", lBuzzerAlias, lBuzzerName, 
				strprintf(TxAssetType::scaleFormat(QBIT), (double)value_ / (double)QBIT));
		} else {
			if (infos_.size() > 1) {
				return strprintf("%s (%s) and %d others", 
					lBuzzerAlias, lBuzzerName, infos_.size());
			}

			return strprintf("%s (%s)", lBuzzerAlias, lBuzzerName);
		}
	}

	std::string typeString() {
		if (type_ == TX_BUZZ) return "bz";
		else if (type_ == TX_REBUZZ) return "rb";
		else if (type_ == TX_BUZZ_REPLY) return "re";
		else if (type_ == TX_BUZZ_LIKE) return "lk";
		else if (type_ == TX_BUZZ_REWARD) return "dn";
		else if (type_ == TX_BUZZER_MISTRUST) return "mt";
		else if (type_ == TX_BUZZER_ENDORSE) return "es";
		else if (type_ == TX_BUZZER_MESSAGE) return "msg";
		return "N";
	}

	std::string scoreString() {
		return strprintf("%d/%d", order().timeframe(), score_);
	}

	std::string toString() {
		//
		std::string lBuzzerName = strprintf("<%s>", buzzerId_.toHex());
		std::string lBuzzerAlias;
		buzzerInfo(lBuzzerName, lBuzzerAlias);

		if (!lBuzzerAlias.size()) lBuzzerAlias = "?";

		if (type_ == TX_REBUZZ) {
			if (infos_.size())
				return strprintf("%s rebuzzed\n%s\n%s | %s | %s (%s) - (%s)\n%s\n[%d/%d/%d/%d]\n -> %s",
					infosToString(),
					buzzId_.toHex(),
					lBuzzerAlias,
					lBuzzerName,
					formatISO8601DateTime(timestamp_ / 1000000),
					typeString(),
					scoreString(),
					buzzBodyString(),
					replies_, rebuzzes_, likes_, rewards_,
					originalBuzzId_.isNull() ? "?" : originalBuzzId_.toHex());
			return strprintf("%s\n%s | %s | %s (%s) - (%s)\n%s\n%s\n[%d/%d/%d/%d]\n -> %s",
				buzzId_.toHex(),
				lBuzzerAlias,
				lBuzzerName,
				formatISO8601DateTime(timestamp_ / 1000000),
				typeString(),
				scoreString(),
				buzzBodyString().size() ? buzzBodyString() : "[...]", wrapped_ ? wrapped_->toRebuzzString() : "\t[---]",
				replies_, rebuzzes_, likes_, rewards_,
				originalBuzzId_.isNull() ? "?" : originalBuzzId_.toHex());
		} else if (type_ == TX_BUZZ_LIKE) {
			return strprintf("%s liked\n%s\n%s | %s | %s (%s) - (%s)\n%s\n[%d/%d/%d/%d]\n -> %s",
				infosToString(),
				buzzId_.toHex(),
				lBuzzerAlias,
				lBuzzerName,
				formatISO8601DateTime(timestamp_ / 1000000),
				typeString(),
				scoreString(),
				buzzBodyString(),
				replies_, rebuzzes_, likes_, rewards_,
				originalBuzzId_.isNull() ? "?" : originalBuzzId_.toHex());
		} else if (type_ == TX_BUZZ_REWARD) {
			return strprintf("%s donated\n%s\n%s | %s | %s (%s) - (%s)\n%s\n[%d/%d/%d/%d]\n -> %s",
				infosToString(),
				buzzId_.toHex(),
				lBuzzerAlias,
				lBuzzerName,
				formatISO8601DateTime(timestamp_ / 1000000),
				typeString(),
				scoreString(),
				buzzBodyString(),
				replies_, rebuzzes_, likes_, rewards_,
				originalBuzzId_.isNull() ? "?" : originalBuzzId_.toHex());
		} else if (type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			//		
			return strprintf("%s\n%s | %s | %s (%s) - (%s)\n%s",
				buzzId_.toHex(),
				lBuzzerAlias,
				lBuzzerName,
				formatISO8601DateTime(timestamp_ / 1000000),
				typeString(),
				scoreString(),
				infosToString());
		}

		return strprintf("%s\n%s | %s | %s (%s) - (%s)\n%s\n[%d/%d/%d/%d]\n -> %s",
			buzzId_.toHex(),
			lBuzzerAlias,
			lBuzzerName,
			formatISO8601DateTime(timestamp_ / 1000000),
			typeString(),
			scoreString(),
			buzzBodyString(),
			replies_, rebuzzes_, likes_, rewards_,
			originalBuzzId_.isNull() ? "?" : originalBuzzId_.toHex());
	}

	std::string toRebuzzString() {
		//
		std::string lBuzzerName = strprintf("<%s>", buzzerId_.toHex());
		std::string lBuzzerAlias;
		buzzerInfo(lBuzzerName, lBuzzerAlias);

		if (!lBuzzerAlias.size()) lBuzzerAlias = "?";

		return strprintf("\t%s\n\t%s | %s | %s (%s) - (%s)\n\t%s\n",
			buzzId_.toHex(),
			lBuzzerAlias,
			lBuzzerName,
			formatISO8601DateTime(timestamp_ / 1000000),
			typeString(),
			scoreString(),
			buzzBodyString());
	}

	virtual void lock() {
		if (root_) { root_->lock(); }
	}

	virtual void unlock() {
		if (root_) { root_->unlock(); }
	}

	virtual BuzzerPtr buzzer() const {
		if (root_) return root_->buzzer();
		return nullptr;
	}

	virtual void insertNewItem(BuzzfeedItemPtr item) {
		if (root_) root_->insertNewItem(item);
	}

	virtual bool fed() {
		if (root_) return root_->fed();

		return pulled_;
	}

	virtual uint64_t locateLastTimestamp();
	virtual void locateLastTimestamp(std::set<uint64_t>&);
	virtual void locateLastTimestamp(std::map<uint256 /*chain*/, std::vector<BuzzfeedPublisherFrom>>&);
	virtual void collectPendingItems(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>&);
	virtual void crossMerge(bool notify = false);
	virtual void wrap(BuzzfeedItemPtr item) {
		wrapped_ = item;
	}
	virtual BuzzfeedItemPtr wrapped() { return wrapped_; }
	virtual BuzzfeedItemPtr root() { return root_; }
	void wasSuspicious() { wasSuspicious_ = true; }
	virtual bool isRoot() { return false; }

	bool hasPrevLink() { return hasPrevLink_; }
	void setHasPrevLink(bool value) { hasPrevLink_ = value; }

	bool hasNextLink() { return hasNextLink_; }
	void setHasNextLink(bool value) { hasNextLink_ = value; }

	bool pulled() { return pulled_; }
	void setPulled() { pulled_ = true; }
	void resetPulled() { pulled_ = false; }

	bool isOnChain() { return onChain_; }
	void notOnChain() { onChain_ = false; }
	void setOnChain() { onChain_ = true; }

	bool isDynamic() {
		if (dynamic_) return true;
		return hasMempool();
	}
	void setDynamic() { dynamic_ = true; }

	bool resolvePKey(PKey&);

	virtual void clear();

	size_t count() const { return items_.size(); }	
	BuzzfeedItemPtr firstChild();

	Buzzer::VerificationResult signatureVerification() { return checkResult_; }

	virtual bool valid() {
		if (checkResult_ == Buzzer::VerificationResult::SUCCESS) return true;
		else if (checkResult_ == Buzzer::VerificationResult::INVALID) return false;

		Buzzer::VerificationResult lResult;
		if (wasSuspicious_ && verifyPublisherLazy_) lResult = verifyPublisherLazy_(shared_from_this());
		else lResult = verifyPublisher_(shared_from_this());
		return (lResult == Buzzer::VerificationResult::SUCCESS);
	}


	virtual void setVerifyPublisher(buzzfeedItemVerifyFunction verify) {
		verifyPublisher_ = verify;
	}

	virtual void setSignatureVerification(Buzzer::VerificationResult result) {
		//
		checkResult_ = result;
	}

	virtual BuzzfeedItemPtr last() {
		Guard lLock(this);
		if (index_.size()) {
			std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator lItem = index_.begin();
			std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
			if (lBuzz != items_.end())
				return lBuzz->second;
		}

		return nullptr;
	}

	uint64_t actualScore() {
		//
		if ((type_ == TX_REBUZZ && infos_.size()) || type_ == TX_BUZZ_LIKE || type_ == TX_BUZZ_REWARD || type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			std::vector<ItemInfo>::const_iterator lItem = infos_.begin();
			return lItem->score();
		}

		return score_;
	}

	uint64_t actualTimestamp() {
		//
		if ((type_ == TX_REBUZZ && infos_.size()) || type_ == TX_BUZZ_LIKE || type_ == TX_BUZZ_REWARD || type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			std::vector<ItemInfo>::const_iterator lItem = infos_.begin();
			return lItem->timestamp();
		}

		return timestamp_;
	}

	uint256 actualPublisher() {
		//
		if ((type_ == TX_REBUZZ && infos_.size()) || type_ == TX_BUZZ_LIKE || type_ == TX_BUZZ_REWARD || type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			std::vector<ItemInfo>::const_iterator lItem = infos_.begin();
			return lItem->buzzerId();
		}

		return buzzerId_;
	}

	uint64_t actualTimeframe() {
		//
		uint64_t lOrder;
		if (order_) lOrder = order_; 
		else lOrder = timestamp_; 
		return lOrder/BUZZFEED_TIMEFRAME;
	}

	void setupCallbacks(BuzzfeedItemPtr item) {
		//		
		buzzerInfo_ = item->buzzerInfo_;
		buzzerInfoResolve_ = item->buzzerInfoResolve_;
		verifyPublisher_ = item->verifyPublisher_;
		verifyPublisherLazy_ = item->verifyPublisherLazy_;
		verifyUpdateForMyThread_ = item->verifyUpdateForMyThread_;
		pkeyResolve_ = item->pkeyResolve_;

		//
		largeUpdated_ = item->largeUpdated_;
		itemNew_ = item->itemNew_;
		itemUpdated_ = item->itemUpdated_;
		itemsUpdated_ = item->itemsUpdated_;
		itemAbsent_ = item->itemAbsent_;
	}

protected:
	virtual void itemUpdated(BuzzfeedItemPtr item) { itemUpdated_(item); }
	virtual void itemNew(BuzzfeedItemPtr item) { itemNew_(item); }
	virtual void itemsUpdated(const std::vector<BuzzfeedItemUpdate>& items) { itemsUpdated_(items); }
	virtual void largeUpdated() { largeUpdated_(); }
	virtual void itemAbsent(const uint256& chain, const uint256& id) { itemAbsent_(chain, id); }

	bool mergeInternal(BuzzfeedItemPtr, bool checkSize = true, bool notify = true, bool suspicious = false);
	void removeIndex(BuzzfeedItemPtr item);
	void insertIndex(BuzzfeedItemPtr item);
	void updateTimestamp(const uint256& publisher, const uint256& chain, uint64_t timestamp);
	bool locateIndex(BuzzfeedItemPtr, int&, uint256&, bool expanded = false);
	BuzzfeedItemPtr locateBuzzInternal(const uint256&);

	void checkProperties() {
		//
		if (!properties_.size()) properties_.resize(1, 0);
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
	unsigned char source_;
	uint256 buzzId_;
	uint256 buzzChainId_;
	uint64_t timestamp_;
	uint64_t score_;
	uint256 buzzerId_;
	uint256 buzzerInfoId_;
	uint256 buzzerInfoChainId_;
	std::vector<unsigned char> buzzBody_;
	mutable std::vector<unsigned char> decryptedBody_;
	std::vector<BuzzerMediaPointer> buzzMedia_;
	uint512 signature_;
	uint32_t replies_;
	uint32_t rebuzzes_;
	uint32_t likes_;
	uint64_t rewards_;
	uint512 subscriptionSignature_;
	std::vector<unsigned char> properties_; // extra properties for the future use

	// ordering
	uint64_t order_; // timestamp initially

	// reply/rebuzz - extra data
	uint256 originalBuzzChainId_;
	uint256 originalBuzzId_;

	// likes
	std::vector<ItemInfo> infos_;
	uint32_t infosCount_ = 0;
	std::set<uint256> info_;

	// endorse/mistrust
	uint64_t value_ = 0;

	// buzzer name/alias resolve
	std::string buzzerName_;
	std::string buzzerAlias_;

	// merge strategy
	Merge merge_ = Merge::UNION;
	std::set<uint256> chains_;

	// sort order
	Order sortOrder_ = Order::REVERSE;

	// key structure
	Group key_ = Group::TIMEFRAME_SCORE;

	// expand threads to feed
	Expand expand_ = Expand::SMART;

	//
	bool threaded_ = false;

	// cache for aggregates
	std::set<uint256> repliesSet_;
	std::set<uint256> rebuzzesSet_;
	std::set<uint256> likesSet_;

	// check result
	Buzzer::VerificationResult checkResult_ = Buzzer::VerificationResult::POSTPONED;

	// threads
	std::map<Key /*buzz*/, BuzzfeedItemPtr> items_;
	std::map<uint256 /*buzz*/, BuzzfeedItemPtr> originalItems_;
	std::multimap<OrderKey /*order*/, Key /*buzz*/> index_;
	std::map<uint256 /*buzz*/, std::set<Key>> orphans_;
	std::map<uint256 /*publisher*/, std::map<uint256 /*chain*/, uint64_t>> lastTimestamps_;
	std::map<uint256 /*originalBuzz*/, std::map<Key, BuzzfeedItemPtr>> pendings_;
	std::map<Key /*buzz*/, BuzzfeedItemPtr> suspicious_;
	std::map<Key /*buzz*/, BuzzfeedItemPtr> unconfirmed_;
	std::set<uint160> confirmed_; // current buzz
	bool wasSuspicious_ = false;
	bool onChain_ = true;
	bool dynamic_ = false;

	// resolver
	buzzerInfoFunction buzzerInfo_;
	buzzerInfoResolveFunction buzzerInfoResolve_;
	buzzfeedItemVerifyFunction verifyPublisher_;
	buzzfeedItemVerifyFunction verifyPublisherLazy_;
	verifyUpdateForMyThreadFunction verifyUpdateForMyThread_;
	buzzfeedItemResolvePKeyFunction pkeyResolve_;

	// notifications
	buzzfeedLargeUpdatedFunction largeUpdated_;
	buzzfeedItemNewFunction itemNew_;
	buzzfeedItemUpdatedFunction itemUpdated_;
	buzzfeedItemsUpdatedFunction itemsUpdated_;
	buzzfeedItemAbsentFunction itemAbsent_;

	// parent
	BuzzfeedItemPtr parent_ = nullptr;
	mutable BuzzfeedItemPtr root_ = nullptr;

	// wrapped item
	BuzzfeedItemPtr wrapped_ = nullptr; // for rebuzz-comment

	// nonce & buzzid for buzz threads realtime updates
	uint64_t nonce_ = 0;
	uint256 rootBuzzId_;

	// linkage info
	bool hasPrevLink_ = false;
	bool hasNextLink_ = false;
	bool pulled_ = false;
};

//
class Buzzfeed;
typedef std::shared_ptr<Buzzfeed> BuzzfeedPtr;

//
// buzzfeed
class Buzzfeed: public BuzzfeedItem {
public:
	Buzzfeed(BuzzerPtr buzzer,	buzzfeedItemVerifyFunction verifyPublisher,
								buzzfeedLargeUpdatedFunction largeUpdated, 
								buzzfeedItemNewFunction itemNew, 
								buzzfeedItemUpdatedFunction itemUpdated,
								buzzfeedItemsUpdatedFunction itemsUpdated,
								buzzfeedItemAbsentFunction itemAbsent, 
								Merge merge, Order order, Group key) : 
		buzzer_(buzzer) {
		largeUpdated_ = largeUpdated;
		itemNew_ = itemNew;
		itemUpdated_ = itemUpdated;
		itemsUpdated_ = itemsUpdated;
		itemAbsent_ = itemAbsent;
		verifyPublisher_ = verifyPublisher;
		merge_ = merge;
		sortOrder_ = order;
		key_ = key;
	}

	Buzzfeed(BuzzerPtr buzzer,	buzzfeedItemResolvePKeyFunction pkeyResolve,
								buzzfeedItemVerifyFunction verifyPublisher,
								buzzfeedLargeUpdatedFunction largeUpdated, 
								buzzfeedItemNewFunction itemNew, 
								buzzfeedItemUpdatedFunction itemUpdated,
								buzzfeedItemsUpdatedFunction itemsUpdated,
								buzzfeedItemAbsentFunction itemAbsent, 
								Merge merge, Order order, Group key) : 
		buzzer_(buzzer) {
		largeUpdated_ = largeUpdated;
		itemNew_ = itemNew;
		itemUpdated_ = itemUpdated;
		itemsUpdated_ = itemsUpdated;
		itemAbsent_ = itemAbsent;
		verifyPublisher_ = verifyPublisher;
		pkeyResolve_ = pkeyResolve;
		merge_ = merge;
		sortOrder_ = order;
		key_ = key;
	}

	Buzzfeed(BuzzerPtr buzzer,	buzzfeedItemVerifyFunction verifyPublisher, 
								buzzfeedItemVerifyFunction verifyPublisherLazy,
								buzzfeedLargeUpdatedFunction largeUpdated, 
								buzzfeedItemNewFunction itemNew, 
								buzzfeedItemUpdatedFunction itemUpdated,
								buzzfeedItemsUpdatedFunction itemsUpdated,
								buzzfeedItemAbsentFunction itemAbsent, 
								Merge merge, Order order, Group key) : 
		Buzzfeed (buzzer, verifyPublisher, largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge, order, key) {
		verifyPublisherLazy_ = verifyPublisherLazy;
	}

	Buzzfeed(BuzzerPtr buzzer,	buzzfeedItemVerifyFunction verifyPublisher,
								buzzfeedItemVerifyFunction verifyPublisherLazy, 
								verifyUpdateForMyThreadFunction verifyUpdateForMyThread,
								buzzfeedLargeUpdatedFunction largeUpdated, 
								buzzfeedItemNewFunction itemNew, 
								buzzfeedItemUpdatedFunction itemUpdated,
								buzzfeedItemsUpdatedFunction itemsUpdated,
								buzzfeedItemAbsentFunction itemAbsent, 
								Merge merge, Order order, Group key, Expand expand) :
		Buzzfeed (buzzer, verifyPublisher, verifyPublisherLazy, largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge, order, key) {
		verifyUpdateForMyThread_ = verifyUpdateForMyThread;
		expand_ = expand;
	}

	Buzzfeed(BuzzerPtr buzzer,	buzzfeedItemVerifyFunction verifyPublisher,
								buzzfeedLargeUpdatedFunction largeUpdated, 
								buzzfeedItemNewFunction itemNew, 
								buzzfeedItemUpdatedFunction itemUpdated,
								buzzfeedItemsUpdatedFunction itemsUpdated,
								buzzfeedItemAbsentFunction itemAbsent, 
								Merge merge) : 
		buzzer_(buzzer) {
		largeUpdated_ = largeUpdated; 
		itemNew_ = itemNew; 
		itemUpdated_ = itemUpdated;
		itemsUpdated_ = itemsUpdated;
		itemAbsent_ = itemAbsent;
		verifyPublisher_ = verifyPublisher;
		merge_ = merge;
	}

	Buzzfeed(BuzzerPtr buzzer,	buzzfeedItemVerifyFunction verifyPublisher, 
								buzzfeedItemVerifyFunction verifyPublisherLazy,
								buzzfeedLargeUpdatedFunction largeUpdated, 
								buzzfeedItemNewFunction itemNew, 
								buzzfeedItemUpdatedFunction itemUpdated,
								buzzfeedItemsUpdatedFunction itemsUpdated,
								buzzfeedItemAbsentFunction itemAbsent, 
								Merge merge, Expand expand) :
		Buzzfeed (buzzer, verifyPublisher, largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge) {
		verifyPublisherLazy_ = verifyPublisherLazy;
		expand_ = expand;
	}

	Buzzfeed(BuzzerPtr buzzer,	buzzfeedItemVerifyFunction verifyPublisher,
								buzzfeedItemVerifyFunction verifyPublisherLazy,
								verifyUpdateForMyThreadFunction verifyUpdateForMyThread,
								buzzfeedLargeUpdatedFunction largeUpdated, 
								buzzfeedItemNewFunction itemNew, 
								buzzfeedItemUpdatedFunction itemUpdated,
								buzzfeedItemsUpdatedFunction itemsUpdated,
								buzzfeedItemAbsentFunction itemAbsent, 
								Merge merge) :
		Buzzfeed (buzzer, verifyPublisher, verifyPublisherLazy, largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge, Expand::SMART) {
		verifyUpdateForMyThread_ = verifyUpdateForMyThread;
	}

	Buzzfeed(BuzzfeedPtr buzzfeed) {
		buzzer_ = buzzfeed->buzzer_;
		largeUpdated_ = buzzfeed->largeUpdated_;
		itemNew_ = buzzfeed->itemNew_;
		itemUpdated_ = buzzfeed->itemUpdated_;
		itemsUpdated_ = buzzfeed->itemsUpdated_;
		itemAbsent_ = buzzfeed->itemAbsent_;
		verifyPublisher_ = buzzfeed->verifyPublisher_;
		verifyPublisherLazy_ = buzzfeed->verifyPublisherLazy_;
		verifyUpdateForMyThread_ = buzzfeed->verifyUpdateForMyThread_;
		pkeyResolve_ = buzzfeed->pkeyResolve_;
		buzzerInfo_ = buzzfeed->buzzerInfo_;
		buzzerInfoResolve_ = buzzfeed->buzzerInfoResolve_;
		merge_ = buzzfeed->merge_;
		sortOrder_ = buzzfeed->sortOrder_;
		key_ = buzzfeed->key_;
		expand_ = buzzfeed->expand_;
	}

	BuzzfeedItemPtr toItem() {
		return std::static_pointer_cast<BuzzfeedItem>(shared_from_this());
	}

	void prepare() {
		buzzerInfo_ = boost::bind(&Buzzer::locateBuzzerInfo, buzzer_, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3);
		buzzerInfoResolve_ = boost::bind(&Buzzer::enqueueBuzzerInfoResolve, buzzer_, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3, boost::placeholders::_4);
	}

	BuzzerPtr buzzer() const { return buzzer_; }
	bool isRoot() { return true; }

	void lock() {
		mutex_.lock();
	}

	void unlock() {
		mutex_.unlock();
	}

	bool fed() { return fed_; }

	BuzzfeedItemPtr root() { return shared_from_this(); }

	void collectLastItemsByChains(std::map<uint256, BuzzfeedItemPtr>&);

	void feed(std::vector<BuzzfeedItemPtr>&, bool expanded = false);
	int locateIndex(BuzzfeedItemPtr);
	void insertNewItem(BuzzfeedItemPtr);
	BuzzfeedItemPtr locateBuzz(const Key&);
	BuzzfeedItemPtr locateBuzz(const uint256& id) { return BuzzfeedItem::locateBuzz(id); }
	void clear();

	static BuzzfeedPtr instance(BuzzerPtr buzzer, buzzfeedItemVerifyFunction verifyPublisher,
			buzzfeedLargeUpdatedFunction largeUpdated, 
			buzzfeedItemNewFunction itemNew, 
			buzzfeedItemUpdatedFunction itemUpdated,
			buzzfeedItemsUpdatedFunction itemsUpdated,
			buzzfeedItemAbsentFunction itemAbsent, Merge merge, Order order, Group key) {
		return std::make_shared<Buzzfeed>(buzzer, verifyPublisher,
					largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge, order, key);
	}

	static BuzzfeedPtr instance(BuzzerPtr buzzer, buzzfeedItemResolvePKeyFunction pkeyResolve, 
			buzzfeedItemVerifyFunction verifyPublisher,
			buzzfeedLargeUpdatedFunction largeUpdated, 
			buzzfeedItemNewFunction itemNew, 
			buzzfeedItemUpdatedFunction itemUpdated,
			buzzfeedItemsUpdatedFunction itemsUpdated,
			buzzfeedItemAbsentFunction itemAbsent, Merge merge, Order order, Group key) {
		return std::make_shared<Buzzfeed>(buzzer, pkeyResolve, verifyPublisher,
					largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge, order, key);
	}

	/*
	static BuzzfeedPtr instance(BuzzerPtr buzzer, 
			buzzfeedItemVerifyFunction verifyPublisher, 
			buzzfeedItemVerifyFunction verifyPublisherLazy,
			buzzfeedLargeUpdatedFunction largeUpdated, 
			buzzfeedItemNewFunction itemNew, 
			buzzfeedItemUpdatedFunction itemUpdated,
			buzzfeedItemsUpdatedFunction itemsUpdated,
			buzzfeedItemAbsentFunction itemAbsent, Merge merge, Order order, Group key) {
		return std::make_shared<Buzzfeed>(buzzer, verifyPublisher, verifyPublisherLazy,
					largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge, order, key);
	}*/

	static BuzzfeedPtr instance(BuzzerPtr buzzer, 
			buzzfeedItemVerifyFunction verifyPublisher,
			buzzfeedItemVerifyFunction verifyPublisherLazy, 
			verifyUpdateForMyThreadFunction verifyUpdateForMyThread,
			buzzfeedLargeUpdatedFunction largeUpdated, 
			buzzfeedItemNewFunction itemNew, 
			buzzfeedItemUpdatedFunction itemUpdated,
			buzzfeedItemsUpdatedFunction itemsUpdated,
			buzzfeedItemAbsentFunction itemAbsent, Merge merge, Order order, Group key, Expand expand) {
		return std::make_shared<Buzzfeed>(buzzer, verifyPublisher, verifyPublisherLazy, verifyUpdateForMyThread,
					largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge, order, key, expand);
	}

	static BuzzfeedPtr instance(BuzzerPtr buzzer, buzzfeedItemVerifyFunction verifyPublisher,
			buzzfeedLargeUpdatedFunction largeUpdated, 
			buzzfeedItemNewFunction itemNew, 
			buzzfeedItemUpdatedFunction itemUpdated,
			buzzfeedItemsUpdatedFunction itemsUpdated,
			buzzfeedItemAbsentFunction itemAbsent, Merge merge) {
		return std::make_shared<Buzzfeed>(buzzer, verifyPublisher,
					largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge);
	}

	static BuzzfeedPtr instance(BuzzerPtr buzzer, 
			buzzfeedItemVerifyFunction verifyPublisher,
			buzzfeedItemVerifyFunction verifyPublisherLazy,
			buzzfeedLargeUpdatedFunction largeUpdated, 
			buzzfeedItemNewFunction itemNew, 
			buzzfeedItemUpdatedFunction itemUpdated,
			buzzfeedItemsUpdatedFunction itemsUpdated,
			buzzfeedItemAbsentFunction itemAbsent, Merge merge, Expand expand) {
		return std::make_shared<Buzzfeed>(buzzer, verifyPublisher, verifyPublisherLazy,
					largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge, expand);
	}

	static BuzzfeedPtr instance(BuzzerPtr buzzer, 
			buzzfeedItemVerifyFunction verifyPublisher,
			buzzfeedItemVerifyFunction verifyPublisherLazy,
			verifyUpdateForMyThreadFunction verifyUpdateForMyThread,
			buzzfeedLargeUpdatedFunction largeUpdated, 
			buzzfeedItemNewFunction itemNew, 
			buzzfeedItemUpdatedFunction itemUpdated,
			buzzfeedItemsUpdatedFunction itemsUpdated,
			buzzfeedItemAbsentFunction itemAbsent, Merge merge) {
		return std::make_shared<Buzzfeed>(buzzer, verifyPublisher, verifyPublisherLazy, verifyUpdateForMyThread,
					largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge);
	}

	static BuzzfeedPtr instance(BuzzfeedPtr buzzFeed) {
		return std::make_shared<Buzzfeed>(buzzFeed);
	}

protected:
	BuzzerPtr buzzer_;
	boost::recursive_mutex mutex_;

	std::vector<BuzzfeedItemPtr> list_;
	std::map<BuzzfeedItem::Key, int> index_;
	bool fed_ = false;
};

//
// buzzfeed ready
typedef boost::function<void (BuzzfeedPtr /*base feed*/, BuzzfeedPtr /*part feed*/)> buzzfeedReadyFunction;

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
typedef boost::function<void (const std::vector<BuzzfeedItem>&, const uint256&, int)> buzzfeedLoadedRequestFunction;

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

//
// select buzzfeed by entity handler
class ISelectBuzzFeedByEntityHandler: public IReplyHandler {
public:
	ISelectBuzzFeedByEntityHandler() {}
	virtual void handleReply(const std::vector<BuzzfeedItem>&, const uint256&, const uint256&) = 0;
};
typedef std::shared_ptr<ISelectBuzzFeedByEntityHandler> ISelectBuzzFeedByEntityHandlerPtr;

// special callbacks
typedef boost::function<void (const std::vector<BuzzfeedItem>&, const uint256&, const uint256&)> buzzfeedLoadedByEntityFunction;
typedef boost::function<void (const std::vector<BuzzfeedItem>&, const uint256&, const uint256&, int)> buzzfeedLoadedByEntityRequestFunction;

// load transaction
class SelectBuzzFeedByEntity: public ISelectBuzzFeedByEntityHandler, public std::enable_shared_from_this<SelectBuzzFeedByEntity> {
public:
	SelectBuzzFeedByEntity(buzzfeedLoadedByEntityFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ISelectBuzzFeedHandler
	void handleReply(const std::vector<BuzzfeedItem>& feed, const uint256& chain, const uint256& entity) {
		 function_(feed, chain, entity);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ISelectBuzzFeedByEntityHandlerPtr instance(buzzfeedLoadedByEntityFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectBuzzFeedByEntity>(function, timeout); 
	}

private:
	buzzfeedLoadedByEntityFunction function_;
	timeoutFunction timeout_;
};

//
// select hash tags handler
class ISelectHashTagsHandler: public IReplyHandler {
public:
	ISelectHashTagsHandler() {}
	virtual void handleReply(const std::vector<Buzzer::HashTag>&, const uint256&) = 0;
};
typedef std::shared_ptr<ISelectHashTagsHandler> ISelectHashTagsHandlerPtr;

// special callbacks
typedef boost::function<void (const std::vector<Buzzer::HashTag>&, const uint256&)> hashTagsLoadedFunction;
typedef boost::function<void (const std::vector<Buzzer::HashTag>&, const uint256&, int)> hashTagsLoadedRequestFunction;
typedef boost::function<void (const std::string&, const std::vector<Buzzer::HashTag>&, const ProcessingError&)> hashTagsLoadedWithErrorFunction;

// load transaction
class SelectHashTags: public ISelectHashTagsHandler, public std::enable_shared_from_this<SelectHashTags> {
public:
	SelectHashTags(hashTagsLoadedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ISelectHashTagsHandler
	void handleReply(const std::vector<Buzzer::HashTag>& tags, const uint256& chain) {
		 function_(tags, chain);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ISelectHashTagsHandlerPtr instance(hashTagsLoadedFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectHashTags>(function, timeout); 
	}

private:
	hashTagsLoadedFunction function_;
	timeoutFunction timeout_;
};

} // qbit

#endif
