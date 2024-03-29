#include "buzzfeed.h"
#include "../../client/dapps/buzzer/buzzerrequestprocessor.h"

using namespace qbit;

void Buzzer::buzzerInfoLoaded(TransactionPtr tx) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	if (tx) {
		TxBuzzerInfoPtr lInfo = TransactionHelper::to<TxBuzzerInfo>(tx);
		std::map<uint256 /*info tx*/, Buzzer::Info>::iterator lItem = pendingInfos_.find(tx->id());
		if (lItem != pendingInfos_.end() && lInfo->verifySignature() && // signature and ...
				lInfo->in()[TX_BUZZER_INFO_MY_IN].out().tx() == lItem->second.id()) { // ... expected buzzer is match
			pushBuzzerInfo(lInfo);
		}
	} else {
		loadingPendingInfos_.clear();
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
			//
			//if (gLog().isEnabled(Log::CLIENT))
			//	gLog().write(Log::CLIENT, strprintf("[resolveBuzzerInfos]: requesting info = %s, chain = %d", 
			//		lItem->first.toHex(), lItem->second.chain().toHex()));
			//
			if (requestProcessor_->loadTransaction(lItem->second.chain(), lItem->first,
				LoadTransaction::instance(
					boost::bind(&Buzzer::buzzerInfoLoaded, shared_from_this(), boost::placeholders::_1),
					boost::bind(&Buzzer::timeout, shared_from_this()))
			)) {
				loadingPendingInfos_.insert(lItem->first);
			} else {
				//if (gLog().isEnabled(Log::CLIENT))
				//	gLog().write(Log::CLIENT, strprintf("[resolveBuzzerInfos]: NOT RESOLVED for info = %s, chain = %d", 
				//		lItem->first.toHex(), lItem->second.chain().toHex()));
			}
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

namespace qbit {
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

bool Buzzer::processConversations(const BuzzfeedItem& item, const uint160& peer) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	std::map<uint256 /*conversation*/, BuzzfeedItemPtr>::iterator lConversation = activeConversations_.find(item.rootBuzzId());
	//
	if (lConversation != activeConversations_.end()) {
		lConversation->second->push(item, peer);
		return true;
	}

	return false;
}

void Buzzer::resolvePendingItems() {
	//
	// if we have postponed items, request missing
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> lPending;
	//
	buzzfeed()->collectPendingItems(lPending);

	if (gLog().isEnabled(Log::CLIENT))
		gLog().write(Log::CLIENT, strprintf("[resolvePendingItems]: pending count = %d", lPending.size()));

	if (lPending.size()) {
		//
		for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = lPending.begin(); lChain != lPending.end(); lChain++) {
			//
			std::vector<uint256> lBuzzes(lChain->second.begin(), lChain->second.end());
			buzzerRequestProcessor_->selectBuzzes(lChain->first, lBuzzes, 
				SelectBuzzFeed::instance(
					boost::bind(&Buzzer::pendingItemsLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&Buzzer::timeout, shared_from_this()))
			);
		}
	}	
}

void Buzzer::resolvePendingEventsItems() {
	//
	// if we have postponed items, request missing
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> lPending;
	//
	eventsfeed()->collectPendingItems(lPending);

	if (lPending.size()) {
		//
		for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = lPending.begin(); lChain != lPending.end(); lChain++) {
			//
			std::vector<uint256> lBuzzes(lChain->second.begin(), lChain->second.end());
			buzzerRequestProcessor_->selectBuzzes(lChain->first, lBuzzes, 
				SelectBuzzFeed::instance(
					boost::bind(&Buzzer::pendingEventItemsLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&Buzzer::timeout, shared_from_this()))
			);
		}
	}	
}

void Buzzer::pendingItemsLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	// merge and notify
	buzzfeed()->merge(feed, 1 /*exact ONE*/, true);

	// force
	resolveBuzzerInfos();
}

void Buzzer::pendingEventItemsLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	// merge and notify
	eventsfeed()->merge(feed);

	// force
	resolveBuzzerInfos();
}

BuzzerPtr Buzzer::instance(IRequestProcessorPtr requestProcessor, BuzzerRequestProcessorPtr buzzerRequestProcessor, IWalletPtr wallet, buzzerTrustScoreUpdatedFunction trustScoreUpdated) {
	return std::make_shared<Buzzer>(requestProcessor, buzzerRequestProcessor, wallet, trustScoreUpdated);
}

void Buzzer::pushPendingMessage(const uint160& peer, const uint256& chain, const uint256& conversation, BuzzfeedItemPtr item) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	item->notOnChain(); // only for the dynamic updates
	item->setDynamic(); // dynamic
	pendingMessages_[peer][chain][conversation].push_back(item);
	if (pendingMessages_[peer][chain][conversation].size() > 5) pendingMessages_[peer][chain][conversation].pop_front();
}

