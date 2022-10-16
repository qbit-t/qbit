// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_H
#define QBIT_BUZZER_H

#include "../../ipeerextension.h"
#include "../../ipeermanager.h"
#include "../../ipeer.h"
#include "../../irequestprocessor.h"
#include "../../message.h"
#include "../../tinyformat.h"
#include "../../client/handlers.h"

#include "txbuzzer.h"
#include "txbuzzerinfo.h"

#include <boost/function.hpp>

namespace qbit {

// forward
class Buzzer;
typedef std::shared_ptr<Buzzer> BuzzerPtr;

class BuzzfeedItem;
typedef std::shared_ptr<BuzzfeedItem> BuzzfeedItemPtr;

class EventsfeedItem;
typedef std::shared_ptr<EventsfeedItem> EventsfeedItemPtr;

class ConversationItem;
typedef std::shared_ptr<ConversationItem> ConversationItemPtr;

class BuzzerRequestProcessor;
typedef std::shared_ptr<BuzzerRequestProcessor> BuzzerRequestProcessorPtr;

//
typedef boost::function<void (const uint256&)> buzzerInfoReadyFunction;
typedef boost::function<void (const uint256&)> buzzerReadyFunction;

//
// buzzer callbacks
typedef boost::function<void (amount_t, amount_t)> buzzerTrustScoreUpdatedFunction;

//
// buzzfeed
class Buzzer: public std::enable_shared_from_this<Buzzer> {
public:
	class Info {
	public:
		Info() {}
		Info(const uint256& chain, const uint256& id) : chain_(chain), id_(id) {}

		const uint256& chain() const { return chain_; }
		const uint256& id() const { return id_; }

	private:
		uint256 chain_;
		uint256 id_;
	};

	enum VerificationResult {
		INVALID = 0,
		SUCCESS = 1,
		POSTPONED = 2
	};

	class HashTag {
	public:
		HashTag() {}
		HashTag(const uint160& hash, const std::string& tag) : hash_(hash), tag_(tag) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(hash_);
			READWRITE(tag_);
		}

		const uint160& hash() const { return hash_; }
		const std::string& tag() const { return tag_; }

	private:
		uint160 hash_;
		std::string tag_;
	};

public:
	Buzzer(IRequestProcessorPtr requestProcessor, BuzzerRequestProcessorPtr buzzerRequestProcessor, IWalletPtr wallet, buzzerTrustScoreUpdatedFunction trustScoreUpdated) : 
		requestProcessor_(requestProcessor), buzzerRequestProcessor_(buzzerRequestProcessor), wallet_(wallet), trustScoreUpdated_(trustScoreUpdated) {}

	void updateTrustScore(amount_t endorsements, amount_t mistrusts, uint32_t subscriptions, uint32_t followers, const ProcessingError& err) {
		if (err.success()) {
			following_ = subscriptions;
			followers_ = followers;

			updateTrustScore(endorsements, mistrusts);
		}
	}

	void updateTrustScoreFull(amount_t endorsements, amount_t mistrusts, uint32_t subscriptions, uint32_t followers, const ProcessingError& err) {
		if (err.success()) {
			following_ = subscriptions;
			followers_ = followers;
			endorsements_ = endorsements;
			mistrusts_ = mistrusts;

			updateTrustScore(endorsements, mistrusts);
		}
	}

	void updateTrustScore(amount_t endorsements, amount_t mistrusts) {
		// NOTICE: raw ts - is not correct, in terms of blockchain
		// endorsements_ = endorsements;
		// mistrusts_ = mistrusts;

		trustScoreUpdated_(endorsements, mistrusts);
	}

	void setTrustScore(amount_t endorsements, amount_t mistrusts) {
		//
		endorsements_ = endorsements;
		mistrusts_ = mistrusts;
	}

	void setBuzzfeed(BuzzfeedItemPtr buzzfeed) {
		buzzfeed_ = buzzfeed;
	}

	void setEventsfeed(EventsfeedItemPtr eventsfeed) {
		eventsfeed_ = eventsfeed;
	}

	void setConversations(ConversationItemPtr conversations) {
		conversations_ = conversations;
	}

	void setConversation(BuzzfeedItemPtr conversation) {
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		conversation_ = conversation;
	}

	void registerUpdate(BuzzfeedItemPtr buzzfeed) {
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		updates_.insert(buzzfeed);
	}

	void removeUpdate(BuzzfeedItemPtr buzzfeed) {
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		updates_.erase(buzzfeed);
	}

	void registerActiveConversation(const uint256& id, BuzzfeedItemPtr buzzfeed) {
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		activeConversations_[id] = buzzfeed;
	}

	void removeActiveConversation(const uint256& id) {
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		activeConversations_.erase(id);
	}

	void registerSubscription(const uint512& subscription, BuzzfeedItemPtr buzzfeed) {
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		subscriptions_[subscription] = buzzfeed;
	}

	void removeSubscription(const uint512& subscription) {
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		subscriptions_.erase(subscription);
	}

	bool processSubscriptions(const BuzzfeedItem& item, const uint160& peer);
	bool processConversations(const BuzzfeedItem& item, const uint160& peer);

	BuzzfeedItemPtr buzzfeed() { return buzzfeed_; }
	EventsfeedItemPtr eventsfeed() { return eventsfeed_; }
	ConversationItemPtr conversations() { return conversations_; }
	BuzzfeedItemPtr conversation() { return conversation_; }
	IWalletPtr wallet() { return wallet_; }

	template<typename _update>
	void broadcastUpdate(const _update& updates);

	amount_t endorsements() { return endorsements_; }
	amount_t mistrusts() { return mistrusts_; }

	uint32_t following() { return following_; }
	uint32_t followers() { return followers_; }

	amount_t score() {
		if (endorsements_ >= mistrusts_) return endorsements_ - mistrusts_;
		return 0;
	}

	static BuzzerPtr instance(IRequestProcessorPtr requestProcessor, BuzzerRequestProcessorPtr buzzerRequestProcessor, IWalletPtr wallet, buzzerTrustScoreUpdatedFunction trustScoreUpdated);

	std::map<uint256 /*info tx*/, Buzzer::Info> pendingInfos() {
		// copy
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		return pendingInfos_;
	}
	void collectPengingInfos(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>& items);

	bool enqueueBuzzerInfoResolve(const uint256& buzzerChainId, const uint256& buzzerId, const uint256& buzzerInfoId, buzzerInfoReadyFunction readyFunction) {
		//
		if (buzzerChainId.isNull()) return true;
		
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		if (buzzerInfos_.find(buzzerInfoId) == buzzerInfos_.end()) {
			//
			pendingInfos_[buzzerInfoId] = Buzzer::Info(buzzerChainId, buzzerId);
			pendingNotifications_[buzzerInfoId].push_back(readyFunction);
			return false;
		}

		return true;
	}

	TxBuzzerInfoPtr locateBuzzerInfo(const uint256& buzzerInfoId) {
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		std::map<uint256 /*info tx*/, TxBuzzerInfoPtr>::iterator lInfo = buzzerInfos_.find(buzzerInfoId);
		if (lInfo != buzzerInfos_.end()) {
			return	lInfo->second;
		}

		return nullptr;
	}

	void locateBuzzerInfo(const uint256& buzzerInfoId, std::string& name, std::string& alias) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		std::map<uint256 /*info tx*/, TxBuzzerInfoPtr>::iterator lInfo = buzzerInfos_.find(buzzerInfoId);
		if (lInfo != buzzerInfos_.end()) {
			name = lInfo->second->myName();
			alias.insert(alias.end(), lInfo->second->alias().begin(), lInfo->second->alias().end());
		}
	}

	void resolveBuzzerInfos();
	void resolvePendingItems();
	void resolvePendingEventsItems();

	void pushBuzzerInfo(TxBuzzerInfoPtr info) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		buzzerInfos_[info->id()] = info;
		pendingInfos_.erase(info->id());
		loadingPendingInfos_.erase(info->id());

		std::map<
			uint256 /*info tx*/, 
			std::list<buzzerInfoReadyFunction>>::iterator lItem = pendingNotifications_.find(info->id());
		if (lItem != pendingNotifications_.end()) {
			//
			for (std::list<buzzerInfoReadyFunction>::iterator lFunction = lItem->second.begin(); lFunction != lItem->second.end(); lFunction++) {
				//
				(*lFunction)(info->id());
			}
		}
	}

	bool locateAvatarMedia(const uint256& info, BuzzerMediaPointer& pointer) {
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		std::map<uint256 /*info tx*/, TxBuzzerInfoPtr>::iterator lInfo = buzzerInfos_.find(info);
		if (lInfo != buzzerInfos_.end()) {
			//
			pointer.set(lInfo->second->avatar());
			return true;
		}

		return false;
	}

	bool locateHeaderMedia(const uint256& info, BuzzerMediaPointer& pointer) {
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		std::map<uint256 /*info tx*/, TxBuzzerInfoPtr>::iterator lInfo = buzzerInfos_.find(info);
		if (lInfo != buzzerInfos_.end()) {
			//
			pointer.set(lInfo->second->header());
			return true;
		}

		return false;
	}

	void pushPendingMessage(const uint160&, const uint256&, const uint256&, BuzzfeedItemPtr);
	void pendingMessages(const uint160& peer, const uint256& chain, const uint256& conversation, std::list<BuzzfeedItemPtr>& msgs) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		std::map<uint160, std::map<uint256, std::map<uint256, std::list<BuzzfeedItemPtr>>>>::iterator lPeer = pendingMessages_.find(peer);
		if (lPeer != pendingMessages_.end()) {
			//
			std::map<uint256, std::map<uint256, std::list<BuzzfeedItemPtr>>>::iterator lChain = lPeer->second.find(chain);
			if (lChain != lPeer->second.end()) {
				std::map<uint256, std::list<BuzzfeedItemPtr>>::iterator lMsgs = lChain->second.find(conversation);

				if (lMsgs != lChain->second.end()) {
					msgs.insert(msgs.end(), lMsgs->second.begin(), lMsgs->second.end());
					lChain->second.erase(lMsgs);
				}
			}
		}
	}

private:
	void buzzerInfoLoaded(TransactionPtr);
	void pendingItemsLoaded(const std::vector<BuzzfeedItem>&, const uint256&);
	void pendingEventItemsLoaded(const std::vector<BuzzfeedItem>&, const uint256&);
	void timeout() {}

private:
	IRequestProcessorPtr requestProcessor_;
	BuzzerRequestProcessorPtr buzzerRequestProcessor_;
	IWalletPtr wallet_;
	buzzerTrustScoreUpdatedFunction trustScoreUpdated_;

	amount_t endorsements_ = 0;
	amount_t mistrusts_ = 0;
	uint32_t following_ = 0;
	uint32_t followers_ = 0;

	BuzzfeedItemPtr buzzfeed_;
	EventsfeedItemPtr eventsfeed_;
	ConversationItemPtr conversations_;
	BuzzfeedItemPtr conversation_;
	std::set<BuzzfeedItemPtr> updates_;

	// shared buzzer infos
	std::map<uint256 /*info tx*/, TxBuzzerInfoPtr> buzzerInfos_;
	std::map<uint256 /*info tx*/, Buzzer::Info> pendingInfos_;
	std::map<uint256 /*info tx*/, std::list<buzzerInfoReadyFunction>> pendingNotifications_;
	std::set<uint256> loadingPendingInfos_;
	std::map<uint512 /*signature-key*/, BuzzfeedItemPtr> subscriptions_;
	std::map<uint256 /*conversation*/, BuzzfeedItemPtr> activeConversations_;
	std::map<
		uint160 /*peer*/, 
		std::map<
			uint256 /*chain*/, 
			std::map<
				uint256 /*conversation*/, 
				std::list<BuzzfeedItemPtr>>> /*up to 5*/> pendingMessages_;

	boost::recursive_mutex mutex_;
};

} // qbit

#endif
