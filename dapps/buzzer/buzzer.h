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
		HashTag(const uint160& hash, const std::string& tag) {}

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
	Buzzer(IRequestProcessorPtr requestProcessor, buzzerTrustScoreUpdatedFunction trustScoreUpdated) : 
		requestProcessor_(requestProcessor), trustScoreUpdated_(trustScoreUpdated) {}

	void updateTrustScore(amount_t endorsements, amount_t mistrusts) {
		//
		endorsements_ = endorsements;
		mistrusts_ = mistrusts;

		trustScoreUpdated_(endorsements_, mistrusts_);
	}

	void setBuzzfeed(BuzzfeedItemPtr buzzfeed) {
		buzzfeed_ = buzzfeed;
	}

	BuzzfeedItemPtr buzzfeed() { return buzzfeed_; }

	amount_t endorsements() { return endorsements_; }
	amount_t mistrusts() { return mistrusts_; }

	amount_t score() {
		if (endorsements_ >= mistrusts_) return endorsements_ - mistrusts_;
		return 0;
	}

	static BuzzerPtr instance(IRequestProcessorPtr requestProcessor, buzzerTrustScoreUpdatedFunction trustScoreUpdated) {
		return std::make_shared<Buzzer>(requestProcessor, trustScoreUpdated);
	}

	std::map<uint256 /*info tx*/, Buzzer::Info>& pendingInfos() { return pendingInfos_; }
	void collectPengingInfos(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>& items);

	void enqueueBuzzerInfoResolve(const uint256& buzzerChainId, const uint256& buzzerId, const uint256& buzzerInfoId) {
		//
		if (buzzerChainId.isNull()) return;
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		if (buzzerInfos_.find(buzzerInfoId) == buzzerInfos_.end()) {
			pendingInfos_[buzzerInfoId] = Buzzer::Info(buzzerChainId, buzzerId);
		}
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

	void pushBuzzerInfo(TxBuzzerInfoPtr info) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		buzzerInfos_[info->id()] = info;
		pendingInfos_.erase(info->id());
		loadingPendingInfos_.erase(info->id());
	}

private:
	void buzzerInfoLoaded(TransactionPtr);
	void timeout() {}

private:
	IRequestProcessorPtr requestProcessor_;
	buzzerTrustScoreUpdatedFunction trustScoreUpdated_;

	amount_t endorsements_ = 0;
	amount_t mistrusts_ = 0;

	BuzzfeedItemPtr buzzfeed_;

	// shared buzzer infos
	std::map<uint256 /*info tx*/, TxBuzzerInfoPtr> buzzerInfos_;
	std::map<uint256 /*info tx*/, Buzzer::Info> pendingInfos_;
	std::set<uint256> loadingPendingInfos_;	
	boost::recursive_mutex mutex_;
};

} // qbit

#endif
