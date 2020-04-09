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
#include "txbuzzer.h"
#include "txbuzz.h"

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

	class BuzzerInfo {
	public:
		BuzzerInfo() {}

		uint64_t endorsements_ = 0;
		uint64_t mistrusts_ = 0;

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(endorsements_);
			READWRITE(mistrusts_);
		}
	};

public:
	BuzzerTransactionStoreExtension(ISettingsPtr settings, ITransactionStorePtr store) : 
		settings_(settings),
		store_(store),
		timeline_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/timeline"), 
		events_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/events"), 
		subscriptionsIdx_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/indexes/subscriptions"),
		likesIdx_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/indexes/likes"),
		publisherUpdates_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/indexes/publisher_updates"),		
		subscriberUpdates_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/indexes/subscriber_updates"),		
		replies_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/replies"),		
		rebuzzes_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/rebuzzes"),		
		buzzInfo_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/buzz_info"),
		buzzerInfo_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/buzzer_info")
		{}

	bool open();
	bool close();

	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) {}
	bool popUnlinkedOut(const uint256&, TransactionContextPtr) {}
	bool pushEntity(const uint256&, TransactionContextPtr);

	TransactionPtr locateSubscription(const uint256& /*subscriber*/, const uint256& /*publisher*/);
	void selectBuzzfeed(uint64_t /*from*/, const uint256& /*subscriber*/, std::vector<BuzzfeedItem>& /*feed*/);
	bool checkSubscription(const uint256& /*subscriber*/, const uint256& /*publisher*/);
	bool checkLike(const uint256& /*buzz*/, const uint256& /*liker*/);
	bool readBuzzInfo(const uint256& /*buzz*/, BuzzInfo& /*info*/);

	static ITransactionStoreExtensionPtr instance(ISettingsPtr settings, ITransactionStorePtr store) {
		return std::make_shared<BuzzerTransactionStoreExtension>(settings, store);
	}

	void removeTransaction(TransactionPtr);

private:
	void incrementLikes(const uint256&);
	void decrementLikes(const uint256&);

	void incrementReplies(const uint256&);
	void decrementReplies(const uint256&);

	void incrementRebuzzes(const uint256&);
	void decrementRebuzzes(const uint256&);

	void prepareBuzzfeedItem(BuzzfeedItem&, TxBuzzPtr, TxBuzzerPtr);
	bool makeBuzzfeedItem(int&, TxBuzzerPtr, TransactionPtr, ITransactionStorePtr, std::multimap<uint64_t, BuzzfeedItem>&, std::set<uint256>&);

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

private:
	ISettingsPtr settings_;
	ITransactionStorePtr store_;
	bool opened_ = false;

	// timeline: time | publisher -> tx
	db::DbTwoKeyContainer<uint256 /*publisher*/, uint64_t /*timestamp*/, uint256 /*buzz/reply/rebuzz/like/...*/> timeline_;
	// events: time | subscriber | tx -> type
	db::DbThreeKeyContainer<uint256 /*subscriber*/, uint64_t /*timestamp*/, uint256 /*tx*/, unsigned short /*type - buzz/reply/rebuzz/like/...*/> events_;
	// subscriber|publisher -> tx
	db::DbTwoKeyContainer<uint256 /*subscriber*/, uint256 /*publisher*/, uint256 /*tx*/> subscriptionsIdx_;
	// buzz | liker -> like_tx
	db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*liker*/, uint256 /*like_tx*/> likesIdx_;
	// buzz | reply
	db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*rebuzz|reply*/, uint256 /*publisher*/> replies_;
	// buzz | re-buzz
	db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*rebuzz|reply*/, uint256 /*publisher*/> rebuzzes_;
	// buzz | info
	db::DbContainer<uint256 /*buzz*/, BuzzInfo /*buzz info*/> buzzInfo_;
	// buzzer | info
	db::DbContainer<uint256 /*buzzer*/, BuzzerInfo /*buzzer info*/> buzzerInfo_;
	// publisher | timestamp
	db::DbContainer<uint256 /*publisher*/, uint64_t /*timestamp*/> publisherUpdates_;
	// subscriber | timestamp
	db::DbContainer<uint256 /*subscriber*/, uint64_t /*timestamp*/> subscriberUpdates_;
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
