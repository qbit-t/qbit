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
// buzzfeed timeframe for trust score madjustements
#define BUZZFEED_TIMEFRAME 5*60*1000000 // 5 min

//
// forward
class BuzzfeedItem;
typedef std::shared_ptr<BuzzfeedItem> BuzzfeedItemPtr;

//
// resolver functions
typedef boost::function<void (const uint256& /*buzzerInfoId*/, std::string& /*name*/, std::string& /*alias*/)> buzzerInfoFunction;
typedef boost::function<void (const uint256& /*buzzerChainId*/, const uint256& /*buzzerId*/, const uint256& /*buzzerInfoId*/)> buzzerInfoResolveFunction;

//
// buzzfeed callbacks
typedef boost::function<void (void)> buzzfeedLargeUpdatedFunction;
typedef boost::function<void (BuzzfeedItemPtr)> buzzfeedItemNewFunction;
typedef boost::function<void (BuzzfeedItemPtr)> buzzfeedItemUpdatedFunction;
typedef boost::function<void (const uint256&, const uint256&)> buzzfeedItemAbsentFunction;
typedef boost::function<Buzzer::VerificationResult (BuzzfeedItemPtr)> buzzfeedItemVerifyFunction;
typedef boost::function<void (BuzzfeedItemPtr)> buzzerInfoLoadedFunction;

//
// buzzfeed item
class BuzzfeedItem: public std::enable_shared_from_this<BuzzfeedItem> {
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
	class OrderKey {
	public:
		OrderKey(uint64_t timeframe, uint64_t score): timeframe_(timeframe), score_(score) {}

		friend bool operator < (const OrderKey& a, const OrderKey& b) {
			if (a.timeframe() < b.timeframe()) return true;
			if (a.timeframe() > b.timeframe()) return false;
			if (a.score() < b.score()) return true;
			if (a.score() > b.score()) return false;

			return false;
		}

		friend bool operator == (const OrderKey& a, const OrderKey& b) {
			return (a.timeframe() == b.timeframe() && a.score() == b.score());
		}

		uint64_t timeframe() const { return timeframe_; }
		uint64_t score() const { return score_; }

	private:
		uint64_t timeframe_;
		uint64_t score_;
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

		uint64_t timestamp() const { return timestamp_; }
		uint64_t score() const { return score_; }
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
			READWRITE(signature_);
		}

	private:
		uint64_t timestamp_;
		uint64_t score_;
		uint256 eventId_;
		uint256 eventChainId_;
		uint256 buzzerId_;
		uint256 buzzerInfoId_;
		uint256 buzzerInfoChainId_;
		uint512 signature_;	
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
		Update(const uint256& buzzId, const uint256& eventId, Field field, uint32_t count): 
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
			return "NONE";
		}
		uint32_t count() const { return count_; }

	private:
		uint256 buzzId_;
		uint256 eventId_;
		Field field_;
		uint32_t count_;
	};
public:
	BuzzfeedItem() {
		type_ = Transaction::UNDEFINED;
		buzzId_.setNull();
		buzzChainId_.setNull();
		buzzerInfoId_.setNull();
		buzzerInfoChainId_.setNull();
		timestamp_ = 0;
		order_ = 0;
		buzzerId_.setNull();
		buzzerInfoId_.setNull();
		replies_ = 0;
		rebuzzes_ = 0;
		likes_ = 0;
		originalBuzzChainId_ = 0;
		originalBuzzId_ = 0;
		signature_.setNull();
	}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(type_);
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
		}

		READWRITE(order_);

		if (type_ == TX_BUZZ_REPLY || type_ == TX_REBUZZ) {
			READWRITE(originalBuzzChainId_);
			READWRITE(originalBuzzId_);
		}

		if (type_ == TX_BUZZ_LIKE || type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE)
			READWRITE(infos_);

		if (type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			READWRITE(value_);
		}

		READWRITE(properties_);
	}

	OrderKey order() const {
		//
		uint64_t lOrder;
		if (order_) lOrder = order_; 
		else lOrder = timestamp_; 
		return OrderKey(lOrder/BUZZFEED_TIMEFRAME, score_);
	}

	uint64_t actualOrder() {
		if (order_) return order_;
		return timestamp_;
	}

	void setOrder(uint64_t order) { order_ = order; }

	unsigned short type() const { return type_; }
	void setType(unsigned short type) { type_ = type; }

	uint64_t value() { return value_; }
	void setValue(uint64_t value) { value_ = value; }

	const uint256& buzzId() const { return buzzId_; }
	void setBuzzId(const uint256& id) { buzzId_ = id; }

	const uint256& originalBuzzChainId() const { return originalBuzzChainId_; }
	void setOriginalBuzzChainId(const uint256& id) { originalBuzzChainId_ = id; }

	const uint256& originalBuzzId() const { return originalBuzzId_; }
	void setOriginalBuzzId(const uint256& id) { originalBuzzId_ = id; }

	void addItemInfo(const ItemInfo& info) {
		if (info_.insert(info.buzzerId()).second)
			infos_.push_back(info);
	}

	const std::vector<ItemInfo>& infos() const { return infos_; }

	void mergeInfos(const std::vector<ItemInfo>& infos) {
		//
		if (infos_.size() && !info_.size()) {
			for (std::vector<ItemInfo>::iterator lInfo = infos_.begin(); lInfo != infos_.end(); lInfo++) {
				info_.insert(lInfo->buzzerId());
			}
		}

		//
		if (infos_.size() > 50) { infosCount_++; return; }
		for (std::vector<ItemInfo>::const_iterator lBuzzer = infos.begin(); lBuzzer != infos.end(); lBuzzer++) {
			//
			addItemInfo(*lBuzzer);

			if (lBuzzer->timestamp() > order_) order_ = lBuzzer->timestamp();
		}
	}

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

	const std::vector<unsigned char>& buzzBody() const { return buzzBody_; }
	std::string buzzBodyString() const { std::string lBody; lBody.insert(lBody.end(), buzzBody_.begin(), buzzBody_.end()); return lBody; }
	void setBuzzBody(const std::vector<unsigned char>& body) { buzzBody_ = body; }

	const std::vector<BuzzerMediaPointer>& buzzMedia() const { return buzzMedia_; } 
	void setBuzzMedia(const std::vector<BuzzerMediaPointer>& media) {
		buzzMedia_.insert(buzzMedia_.end(), media.begin(), media.end());
	}

	const uint512& signature() const { return signature_; }
	void setSignature(const uint512& signature) { signature_ = signature; }

	uint32_t replies() const { return replies_; }
	void setReplies(uint32_t v) { replies_ = v; }

	uint32_t rebuzzes() const { return rebuzzes_; }
	void setRebuzzes(uint32_t v) { rebuzzes_ = v; }

	uint32_t likes() const { return likes_; }
	void setLikes(uint32_t v) { likes_ = v; }

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

	void merge(const BuzzfeedItem::Update&);
	void merge(const std::vector<BuzzfeedItem::Update>&);
	void merge(const BuzzfeedItem&, bool checkSize = true, bool notify = true);
	void merge(const std::vector<BuzzfeedItem>&, bool notify = false);
	void mergeAppend(const std::list<BuzzfeedItemPtr>&);

	void feed(std::list<BuzzfeedItemPtr>&);
	BuzzfeedItemPtr locateBuzz(const uint256&);

	void setResolver(buzzerInfoResolveFunction buzzerInfoResolve, buzzerInfoFunction buzzerInfo) {
		buzzerInfoResolve_ = buzzerInfoResolve;
		buzzerInfo_ = buzzerInfo;
	}

	void resolve() {
		buzzerInfoResolve_(buzzerInfoChainId_, buzzerId_, buzzerInfoId_);

		if (type_ == TX_BUZZ_LIKE || type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			for (std::vector<ItemInfo>::const_iterator lItem = infos_.begin(); lItem != infos_.end(); lItem++) {
				// just first one for now
				buzzerInfoResolve_(lItem->buzzerInfoChainId(), lItem->buzzerId(), lItem->buzzerInfoId());
				break;
			}			
		}
	}

	std::string infosToString() {
		//
		if (!infos_.size()) return "?";
		
		ItemInfo lInfo = *infos_.begin();

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
		else if (type_ == TX_BUZZER_MISTRUST) return "mt";
		else if (type_ == TX_BUZZER_ENDORSE) return "es";
		return "N";
	}

	std::string toString() {
		//
		std::string lBuzzerName = strprintf("<%s>", buzzerId_.toHex());
		std::string lBuzzerAlias;
		buzzerInfo(lBuzzerName, lBuzzerAlias);

		if (!lBuzzerAlias.size()) lBuzzerAlias = "?";

		if (type_ == TX_REBUZZ) {
			return strprintf("%s\n%s | %s | %s (%s)\n%s\n%s\n[%d/%d/%d]\n -> %s",
				buzzId_.toHex(),
				lBuzzerAlias,
				lBuzzerName,
				formatISO8601DateTime(timestamp_ / 1000000),
				typeString(),
				buzzBodyString().size() ? buzzBodyString() : "[...]", items_.size() ? items_.begin()->second->toRebuzzString() : "[---]",
				replies_, rebuzzes_, likes_,
				originalBuzzId_.isNull() ? "?" : originalBuzzId_.toHex());
		} else if (type_ == TX_BUZZ_LIKE) {
			return strprintf("%s liked\n%s\n%s | %s | %s (%s)\n%s\n[%d/%d/%d]\n -> %s",
				infosToString(),
				buzzId_.toHex(),
				lBuzzerAlias,
				lBuzzerName,
				formatISO8601DateTime(timestamp_ / 1000000),
				typeString(),
				buzzBodyString(),
				replies_, rebuzzes_, likes_,
				originalBuzzId_.isNull() ? "?" : originalBuzzId_.toHex());
		} else if (type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			//		
			return strprintf("%s\n%s | %s | %s (%s)\n%s",
				buzzId_.toHex(),
				lBuzzerAlias,
				lBuzzerName,
				formatISO8601DateTime(timestamp_ / 1000000),
				typeString(),
				infosToString());
		}

		return strprintf("%s\n%s | %s | %s (%s)\n%s\n[%d/%d/%d]\n -> %s",
			buzzId_.toHex(),
			lBuzzerAlias,
			lBuzzerName,
			formatISO8601DateTime(timestamp_ / 1000000),
			typeString(),
			buzzBodyString(),
			replies_, rebuzzes_, likes_,
			originalBuzzId_.isNull() ? "?" : originalBuzzId_.toHex());
	}

	std::string toRebuzzString() {
		//
		std::string lBuzzerName = strprintf("<%s>", buzzerId_.toHex());
		std::string lBuzzerAlias;
		buzzerInfo(lBuzzerName, lBuzzerAlias);

		if (!lBuzzerAlias.size()) lBuzzerAlias = "?";

		return strprintf("\t%s\n\t%s | %s | %s (%s)\n\t%s\n",
			buzzId_.toHex(),
			lBuzzerAlias,
			lBuzzerName,
			formatISO8601DateTime(timestamp_ / 1000000),
			typeString(),
			buzzBodyString());
	}

	virtual void lock() {}
	virtual void unlock() {}

	virtual uint64_t locateLastTimestamp();
	virtual void collectPendingItems(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>&);
	virtual void crossMerge();

	virtual void clear() {
		chains_.clear();
		items_.clear();
		index_.clear();
		orphans_.clear();
		lastTimestamps_.clear();
		pendings_.clear();		
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

	virtual BuzzfeedItemPtr last() {
		Guard lLock(this);
		if (index_.size()) {
			std::multimap<OrderKey /*order*/, uint256 /*buzz*/>::iterator lItem = index_.begin();
			std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
			if (lBuzz != items_.end())
				return lBuzz->second;
		}

		return nullptr;
	}

	uint64_t actualScore() {
		//
		if (type_ == TX_BUZZ_LIKE || type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			std::vector<ItemInfo>::const_iterator lItem = infos_.begin();
			return lItem->score();
		}

		return score_;
	}

	uint64_t actualTimestamp() {
		//
		if (type_ == TX_BUZZ_LIKE || type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			std::vector<ItemInfo>::const_iterator lItem = infos_.begin();
			return lItem->timestamp();
		}

		return timestamp_;
	}

	uint160 createPublisherTimestamp() {
		DataStream lPublisherTsStream(SER_NETWORK, PROTOCOL_VERSION); 

		if (type_ == TX_BUZZ_LIKE || type_ == TX_BUZZER_MISTRUST || type_ == TX_BUZZER_ENDORSE) {
			std::vector<ItemInfo>::const_iterator lItem = infos_.begin();
			lPublisherTsStream << lItem->timestamp();
			lPublisherTsStream << lItem->buzzerId();
		} else {
			lPublisherTsStream << timestamp_;
			lPublisherTsStream << buzzerId_;
		}

		return Hash160(lPublisherTsStream.begin(), lPublisherTsStream.end());
	}

protected:
	//virtual bool verifyPublisher(BuzzfeedItemPtr) { return true; }
	virtual void itemUpdated(BuzzfeedItemPtr) {}
	virtual void itemNew(BuzzfeedItemPtr) {}
	virtual void itemsUpdated(const std::vector<BuzzfeedItem::Update>&) {}
	virtual void largeUpdated() {}
	virtual void itemAbsent(const uint256&, const uint256&) {}

	void removeIndex(BuzzfeedItemPtr item) {
		// clean-up
		std::pair<std::multimap<OrderKey /*order*/, uint256 /*buzz*/>::iterator,
					std::multimap<OrderKey /*order*/, uint256 /*buzz*/>::iterator> lRange = index_.equal_range(item->order());
		for (std::multimap<OrderKey /*order*/, uint256 /*buzz*/>::iterator lExist = lRange.first; lExist != lRange.second; lExist++) {
			if (lExist->second == item->buzzId()) {
				index_.erase(lExist);
				break;
			}
		}
	}

	void insertIndex(BuzzfeedItemPtr item) {
		index_.insert(std::multimap<OrderKey /*order*/, uint256 /*buzz*/>::value_type(item->order(), item->buzzId()));
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
	uint64_t score_;
	uint256 buzzerId_;
	uint256 buzzerInfoId_;
	uint256 buzzerInfoChainId_;
	std::vector<unsigned char> buzzBody_;
	std::vector<BuzzerMediaPointer> buzzMedia_;
	uint512 signature_;
	uint32_t replies_;
	uint32_t rebuzzes_;
	uint32_t likes_;
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

	// merge strategy
	Merge merge_ = Merge::UNION;
	std::set<uint256> chains_;

	// cache for aggregates
	std::set<uint256> repliesSet_;
	std::set<uint256> rebuzzesSet_;
	std::set<uint256> likesSet_;

	// check result
	Buzzer::VerificationResult checkResult_ = Buzzer::VerificationResult::INVALID;

	// threads
	std::map<uint256 /*buzz*/, BuzzfeedItemPtr> items_;
	std::multimap<OrderKey /*order*/, uint256 /*buzz*/> index_;
	std::map<uint256 /*buzz*/, std::set<uint256>> orphans_;
	std::map<uint256 /*publisher*/, uint64_t> lastTimestamps_;
	std::map<uint256 /*originalBuzz*/, std::map<uint256, BuzzfeedItemPtr>> pendings_;

	// resolver
	buzzerInfoFunction buzzerInfo_;
	buzzerInfoResolveFunction buzzerInfoResolve_;
	buzzfeedItemVerifyFunction verifyPublisher_;
};

//
typedef boost::function<void (const std::vector<BuzzfeedItem::Update>&)> buzzfeedItemsUpdatedFunction;

class Buzzfeed;
typedef std::shared_ptr<Buzzfeed> BuzzfeedPtr;

//
// buzzfeed
class Buzzfeed: public BuzzfeedItem {
public:
	Buzzfeed(BuzzerPtr buzzer, buzzfeedItemVerifyFunction verifyPublisher,
								buzzfeedLargeUpdatedFunction largeUpdated, 
								buzzfeedItemNewFunction itemNew, 
								buzzfeedItemUpdatedFunction itemUpdated,
								buzzfeedItemsUpdatedFunction itemsUpdated,
								buzzfeedItemAbsentFunction itemAbsent, Merge merge) : 
		buzzer_(buzzer),
		largeUpdated_(largeUpdated), 
		itemNew_(itemNew), 
		itemUpdated_(itemUpdated),
		itemsUpdated_(itemsUpdated),
		itemAbsent_(itemAbsent) {
		verifyPublisher_ = verifyPublisher;
		merge_ = merge;
	}

	Buzzfeed(BuzzfeedPtr buzzfeed) {
		buzzer_ = buzzfeed->buzzer_;
		largeUpdated_ = buzzfeed->largeUpdated_;
		itemNew_ = buzzfeed->itemNew_;
		itemUpdated_ = buzzfeed->itemUpdated_;
		itemsUpdated_ = buzzfeed->itemsUpdated_;
		itemAbsent_ = buzzfeed->itemAbsent_;
		verifyPublisher_ = buzzfeed->verifyPublisher_;
		buzzerInfo_ = buzzfeed->buzzerInfo_;
		buzzerInfoResolve_ = buzzfeed->buzzerInfoResolve_;
		merge_ = buzzfeed->merge_;
	}

	BuzzfeedItemPtr toItem() {
		return std::static_pointer_cast<BuzzfeedItem>(shared_from_this());
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

	static BuzzfeedPtr instance(BuzzerPtr buzzer, buzzfeedItemVerifyFunction verifyPublisher,
			buzzfeedLargeUpdatedFunction largeUpdated, 
			buzzfeedItemNewFunction itemNew, 
			buzzfeedItemUpdatedFunction itemUpdated,
			buzzfeedItemsUpdatedFunction itemsUpdated,
			buzzfeedItemAbsentFunction itemAbsent, Merge merge) {
		return std::make_shared<Buzzfeed>(buzzer, verifyPublisher,
					largeUpdated, itemNew, itemUpdated, itemsUpdated, itemAbsent, merge);
	}

	static BuzzfeedPtr instance(BuzzfeedPtr buzzFeed) {
		return std::make_shared<Buzzfeed>(buzzFeed);
	}

protected:
	virtual void itemUpdated(BuzzfeedItemPtr item) { itemUpdated_(item); }
	virtual void itemNew(BuzzfeedItemPtr item) { itemNew_(item); }
	virtual void itemsUpdated(const std::vector<BuzzfeedItem::Update>& items) { itemsUpdated_(items); }
	virtual void largeUpdated() { largeUpdated_(); }
	virtual void itemAbsent(const uint256& chain, const uint256& id) { itemAbsent_(chain, id); }

protected:
	BuzzerPtr buzzer_;
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
