#include "conversationsfeed.h"
#include "txbuzzermessage.h"

using namespace qbit;

void ConversationItem::push(const ConversationItem& buzz, const uint160& peer) {
	// put into unconfirmed
	Guard lLock(this);
	std::map<uint256 /*conversation*/, ConversationItemPtr>::iterator lItem = unconfirmed_.find(buzz.key());
	if (lItem == unconfirmed_.end()) { // absent
		//
		ConversationItemPtr lBuzz = ConversationItem::instance(buzz);
		lBuzz->addConfirmation(peer);
		lBuzz->notOnChain(); // only for the dynamic updates
		lBuzz->setDynamic(); // dynamic
		
		unconfirmed_[buzz.key()] = lBuzz;
		if (gLog().isEnabled(Log::CLIENT))
			gLog().write(Log::CLIENT, "[PUSH-0]");
	} else {
		if (lItem->second->addConfirmation(peer) >= BUZZ_PEERS_CONFIRMATIONS) {
			//
			ConversationItemPtr lBuzz = lItem->second;
			// TODO: remove from unconfirmed?
			// unconfirmed_.erase(lItem);
			
			// merge finally
			mergeInternal(lBuzz, true, true);

			if (gLog().isEnabled(Log::CLIENT))
				gLog().write(Log::CLIENT, "[PUSH-1]");

			// resolve info
			if (buzzer()) { 
				buzzer()->resolvePendingEventsItems();
				buzzer()->resolveBuzzerInfos();
			} else {
				std::cout << "[PUSH-ERROR]: Buzzer not found" << "\n";
				if (gLog().isEnabled(Log::CLIENT))
					gLog().write(Log::CLIENT, "[PUSH-ERROR]: Buzzer not found");
			}
		}
	}
}

void ConversationItem::merge(const ConversationItem& buzz, bool checkSize, bool notify) {
	//
	ConversationItemPtr lBuzz = ConversationItem::instance(buzz);
	mergeInternal(lBuzz, checkSize, notify);
}

void ConversationItem::mergeInternal(ConversationItemPtr buzz, bool checkSize, bool notify) {
	//
	ConversationItemPtr lBuzz = buzz;
	// set resolver
	lBuzz->buzzerInfoResolve_ = buzzerInfoResolve_;
	lBuzz->buzzerInfo_ = buzzerInfo_;
	// verifier
	lBuzz->verifyPublisher_ = verifyPublisher_;
	// parent
	lBuzz->parent_ = parent();

	// verify signature
	Buzzer::VerificationResult lResult = verifyPublisher_(lBuzz);
	if (lResult == Buzzer::VerificationResult::SUCCESS || lResult == Buzzer::VerificationResult::POSTPONED) {
		//
		Guard lLock(this);

		// check result
		lBuzz->setSignatureVerification(lResult);

		// push buzz
		std::map<uint256 /*conversation*/, ConversationItemPtr>::iterator lExisting = items_.find(lBuzz->key());
		if (lExisting != items_.end()) {
			removeIndex(lExisting->second);

			lExisting->second->setOrder(lBuzz->order());
			lExisting->second->infoMerge(lBuzz->events());

			index_.insert(std::multimap<uint64_t /*order*/, uint256 /*buzz*/>::value_type(lBuzz->order(), lBuzz->key()));
			for (std::vector<EventInfo>::const_iterator lInfo = lBuzz->events().begin(); lInfo != lBuzz->events().end(); lInfo++) {
				updateTimestamp(lInfo->buzzerId(), lInfo->timestamp());
			}

			// TODO: updates notification
			if (lExisting->second->resolve()) {
				// if all buzz infos are already resolved - just notify
				if (notify)
					itemNew(lExisting->second);
			}

			return;
		} else {
			items_.insert(std::map<uint256 /*buzz*/, ConversationItemPtr>::value_type(lBuzz->key(), lBuzz));
		}

		index_.insert(std::multimap<uint64_t /*order*/, uint256 /*buzz*/>::value_type(lBuzz->order(), lBuzz->key()));
		for (std::vector<EventInfo>::const_iterator lInfo = lBuzz->events().begin(); lInfo != lBuzz->events().end(); lInfo++) {
			updateTimestamp(lInfo->buzzerId(), lInfo->timestamp());
		}

		if (checkSize && items_.size() > 300 /*setup*/) {
			// TODO: do we need to remove?
		}

		if (lBuzz->resolve()) {
			// if all buzz infos are already resolved - just notify
			if (notify)
				itemNew(lBuzz);
		}
	} else {
		std::cout << "[CONVERSATIONS-ERROR]: " << lBuzz->toString() << "\n";
		if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[CONVERSATIONS-ERROR]: %s", lBuzz->toString()));
	}
}	

void ConversationItem::mergeUpdate(const std::vector<ConversationItem::EventInfo>& chunk, const uint160& peer) {
	//
	{
		Guard lLock(this);
		for (std::vector<ConversationItem::EventInfo>::iterator lItem = const_cast<std::vector<ConversationItem::EventInfo>&>(chunk).begin(); 
												lItem != const_cast<std::vector<ConversationItem::EventInfo>&>(chunk).end(); lItem++) {
			// push
			std::map<uint256 /*conversation*/, ConversationItemPtr>::iterator lExisting = items_.find(lItem->conversationId());
			if (lExisting != items_.end()) {
				// clean-up
				removeIndex(lExisting->second);

				// merge & update
				lExisting->second->infoMerge(*lItem);

				// new index
				insertIndex(lExisting->second);

				// notify
				itemUpdated(lExisting->second);
			}
		}
	}
}

void ConversationItem::mergeUpdate(const std::vector<ConversationItem>& chunk, const uint160& peer) {
	//
	{
		Guard lLock(this);
		for (std::vector<ConversationItem>::iterator lItem = const_cast<std::vector<ConversationItem>&>(chunk).begin(); 
												lItem != const_cast<std::vector<ConversationItem>&>(chunk).end(); lItem++) {
			push(*lItem, peer);
		}
	}
}

void ConversationItem::merge(const std::vector<ConversationItem>& chunk, const uint256& chain, bool notify) {
	//
	{
		Guard lLock(this);
		//
		for (std::vector<ConversationItem>::iterator lItem = const_cast<std::vector<ConversationItem>&>(chunk).begin(); 
												lItem != const_cast<std::vector<ConversationItem>&>(chunk).end(); lItem++) {
			//
			std::map<uint256 /*id*/, _commit>::iterator lExisting = commit_.find(lItem->key());
			if (lExisting == commit_.end()) {
				commit_.insert(std::map<uint256 /*id*/, _commit>::value_type(lItem->key(), _commit(ConversationItem::instance(*lItem))));
			} else {
				lExisting->second.commit();
			}
		}

		if (notify /*done*/) {
			//
			for (std::map<uint256 /*id*/, _commit>::iterator lCandidate = commit_.begin(); lCandidate != commit_.end(); lCandidate++) {
				//
				if (lCandidate->second.count_ == CONVERSATIONSFEED_CONFIRMATIONS) {
					mergeInternal(lCandidate->second.candidate_, true, false);
				}
			}

			commit_.clear();
		}
	}

	if (notify) largeUpdated();	
}

bool ConversationItem::mergeAppend(const std::vector<ConversationItemPtr>& items) {
	//
	bool lAdded = false;
	for (std::vector<ConversationItemPtr>::const_iterator lItem = items.begin(); lItem != items.end(); lItem++) {
		//
		std::map<uint256 /*conversation*/, ConversationItemPtr>::iterator lExisting = items_.find((*lItem)->key());
		if (lExisting == items_.end()) {
			//
			lAdded |= items_.insert(std::map<uint256 /*conversation*/, ConversationItemPtr>::value_type((*lItem)->key(), (*lItem))).second;
			index_.insert(std::multimap<uint64_t /*order*/, uint256 /*conversation*/>::value_type((*lItem)->order(), (*lItem)->key()));

			for (std::vector<EventInfo>::const_iterator lInfo = (*lItem)->events().begin(); lInfo != (*lItem)->events().end(); lInfo++) {
				updateTimestamp(lInfo->buzzerId(), lInfo->timestamp());
			}

			if (items_.size() > 300 /*setup*/) {
				// TODO: erase top?
			}
		}
	}

	return lAdded;
}

void ConversationItem::feed(std::vector<ConversationItemPtr>& feed) {
	//
	Guard lLock(this);
	for (std::multimap<uint64_t /*order*/, uint256 /*conversation*/>::reverse_iterator lItem = index_.rbegin(); 
											lItem != index_.rend(); lItem++) {
		//
		std::map<uint256 /*conversation*/, ConversationItemPtr>::iterator lBuzz = items_.find(lItem->second);
		if (lBuzz != items_.end()) {
			if (lBuzz->second->valid()) {
				feed.push_back(lBuzz->second);
			} else {
				std::cout << "[FEED-ERROR]: " << lBuzz->second->toString() << "\n";
				if (gLog().isEnabled(Log::CLIENT))
					gLog().write(Log::CLIENT, strprintf("[FEED-ERROR]: %s", lBuzz->second->toString()));
			}
		}
	}

	setFed();
}

int ConversationItem::locateIndex(ConversationItemPtr item) {
	//
	Guard lLock(this);
	//
	int lIndex = 0;
	bool lFound = false;
	for (std::multimap<uint64_t /*order*/, uint256 /*conversation*/>::reverse_iterator lItem = index_.rbegin();
											lItem != index_.rend(); lItem++, lIndex++) {
		//
		std::map<uint256 /*buzz*/, ConversationItemPtr>::iterator lBuzz = items_.find(lItem->second);
		if (lBuzz != items_.end() && lBuzz->second->key() == item->key()) {
			lFound = true;
			break;
		}
	}

	return lFound ? lIndex : -1;
}

uint64_t ConversationItem::locateLastTimestamp() {
	//
	Guard lLock(this);
	std::set<uint64_t> lTimes;
	for (std::map<uint256 /*publisher*/, uint64_t>::iterator lTimestamp = lastTimestamps_.begin(); lTimestamp != lastTimestamps_.end(); lTimestamp++) {
		lTimes.insert(lTimestamp->second);
	}

	return lTimes.size() ? *lTimes.rbegin() : 0;
}

void ConversationItem::collectPendingItems(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>& items) {
	//
	Guard lLock(this);
	for (std::map<uint256 /*originalBuzz*/, std::set<uint256>>::iterator lPending = pendings_.begin(); lPending != pendings_.end(); lPending++) {
		for (std::set<uint256>::iterator lItem = lPending->second.begin(); lItem != lPending->second.end(); lItem++) {
			ConversationItemPtr lEvent = items_[*lItem];
			if (lEvent)
				items[lEvent->conversationChainId()].insert(lEvent->conversationId());
		}
	}
}

ConversationItemPtr ConversationItem::locateItem(const uint256& key) {
	//
	Guard lLock(this);
	std::map<uint256 /*conversation*/, ConversationItemPtr>::iterator lItem = items_.find(key);
	if (lItem != items_.end()) {
		return lItem->second;
	}

	return nullptr;
}

bool ConversationItem::decrypt(const PKey& pkey, std::string& body) {
	//
	if (!events_.size()) return false;

	//
	bool lProcess = false;
	ConversationItem::EventInfo lInfo = *events_.begin();
	if (lInfo.type() == TX_BUZZER_MESSAGE || lInfo.type() == TX_BUZZER_MESSAGE_REPLY) lProcess = true;
	else {
		if ((++events_.begin()) != events_.end()) {
			lInfo = *(++events_.begin());
			if (lInfo.type() == TX_BUZZER_MESSAGE || lInfo.type() == TX_BUZZER_MESSAGE_REPLY) lProcess = true;
		}
	}

	if (lProcess) {
		//
		if (!lInfo.decryptedBody().size() && buzzer()) {
			SKeyPtr lSKey = buzzer()->wallet()->firstKey();
			//
			uint256 lNonce = lSKey->shared(pkey);
			bool lResult = TxBuzzerMessage::decrypt(lNonce, lInfo.body(), lInfo.decryptedBody());
			if (lResult) {
				body.insert(body.end(), lInfo.decryptedBody().begin(), lInfo.decryptedBody().end());
			}

			return lResult;
		}
	}

	return false;
}

bool ConversationItem::decrypt(const PKey& pkey, const std::string& hex, std::string& body) {
	//
	if (!events_.size()) return false;

	//
	bool lProcess = false;
	ConversationItem::EventInfo lInfo = *events_.begin();
	if (lInfo.type() == TX_BUZZER_ACCEPT_CONVERSATION) lProcess = true;
	//
	if (lProcess && buzzer()) {
		//
		std::vector<unsigned char> lBody = ParseHex(hex);
		std::vector<unsigned char> lDecryptedBody;

		SKeyPtr lSKey = buzzer()->wallet()->firstKey();
		//
		uint256 lNonce = lSKey->shared(pkey);
		bool lResult = TxBuzzerMessage::decrypt(lNonce, lBody, lDecryptedBody);
		if (lResult) {
			body.insert(body.end(), lDecryptedBody.begin(), lDecryptedBody.end());
		}

		return lResult;
	}

	return false;
}
