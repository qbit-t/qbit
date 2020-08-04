#include "buzzfeed.h"
#include "../../client/dapps/buzzer/buzzerrequestprocessor.h"

using namespace qbit;

void Buzzer::buzzerInfoLoaded(TransactionPtr tx) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	if (tx) {
		TxBuzzerInfoPtr lInfo = TransactionHelper::to<TxBuzzerInfo>(tx);
		//
		std::map<uint256 /*info tx*/, Buzzer::Info>::iterator lItem = pendingInfos_.find(tx->id());
		if (lItem != pendingInfos_.end() && lInfo->verifySignature() && // signature and ...
				lInfo->in()[TX_BUZZER_INFO_MY_IN].out().tx() == lItem->second.id()) // ... expected buzzer is match
			pushBuzzerInfo(lInfo);
	}
}

void Buzzer::resolveBuzzerInfos() {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	for (std::map<uint256 /*info tx*/, Buzzer::Info>::iterator lItem = pendingInfos_.begin(); 
																			lItem != pendingInfos_.end(); lItem++) {
		//
		if (loadingPendingInfos_.find(lItem->first) == loadingPendingInfos_.end()) {
			requestProcessor_->loadTransaction(lItem->second.chain(), lItem->first, 
				LoadTransaction::instance(
					boost::bind(&Buzzer::buzzerInfoLoaded, shared_from_this(), _1),
					boost::bind(&Buzzer::timeout, shared_from_this()))
			);

			//
			loadingPendingInfos_.insert(lItem->first);
		}
	}
}

void Buzzer::collectPengingInfos(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>& items) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	for (std::map<uint256 /*info tx*/, Buzzer::Info>::iterator lItem = pendingInfos_.begin(); 
																			lItem != pendingInfos_.end(); lItem++) {
		//
		items[lItem->second.chain()].insert(lItem->first);
	}
}

template<>
void Buzzer::broadcastUpdate<std::vector<BuzzfeedItemUpdate>>(const std::vector<BuzzfeedItemUpdate>& updates) {
	std::set<BuzzfeedItemPtr> lUpdates;		
	{
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		lUpdates.insert(updates_.begin(), updates_.end());
	}

	for (std::set<BuzzfeedItemPtr>::iterator lItem = lUpdates.begin(); lItem != lUpdates.end(); lItem++) {
		//
		(*lItem)->merge(updates);
	}
}

bool Buzzer::processSubscriptions(const BuzzfeedItem& item, const uint160& peer) {
	//
	if (item.source() == BuzzfeedItem::Source::BUZZ_SUBSCRIPTION) {
		boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
		std::map<uint512 /*signature-key*/, BuzzfeedItemPtr>::iterator lSubscription = subscriptions_.find(item.subscriptionSignature());
		//
		if (lSubscription != subscriptions_.end()) {
			lSubscription->second->push(item, peer);
			return true;
		}
	}

	return false;
}

void Buzzer::resolvePendingItems() {
	//
	// if we have postponed items, request missing
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> lPending;
	//
	buzzfeed()->collectPendingItems(lPending);

	if (lPending.size()) {
		//
		for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = lPending.begin(); lChain != lPending.end(); lChain++) {
			//
			std::vector<uint256> lBuzzes(lChain->second.begin(), lChain->second.end());
			buzzerRequestProcessor_->selectBuzzes(lChain->first, lBuzzes, 
				SelectBuzzFeed::instance(
					boost::bind(&Buzzer::pendingItemsLoaded, shared_from_this(), _1, _2),
					boost::bind(&Buzzer::timeout, shared_from_this()))
			);
		}
	}	
}

void Buzzer::pendingItemsLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	// merge and notify
	buzzfeed()->merge(feed, true);

	// force
	buzzfeed()->buzzer()->resolveBuzzerInfos();
}

BuzzerPtr Buzzer::instance(IRequestProcessorPtr requestProcessor, BuzzerRequestProcessorPtr buzzerRequestProcessor, buzzerTrustScoreUpdatedFunction trustScoreUpdated) {
	return std::make_shared<Buzzer>(requestProcessor, buzzerRequestProcessor, trustScoreUpdated);
}

