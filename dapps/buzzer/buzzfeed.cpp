#include "buzzfeed.h"
#include "buzzfeedview.h"
#include "../../log/log.h"
#include "txbuzzermessage.h"

using namespace qbit;

// initial values
size_t qbit::G_BUZZ_PEERS_CONFIRMATIONS = BUZZ_PEERS_CONFIRMATIONS;
size_t qbit::G_BUZZFEED_PEERS_CONFIRMATIONS = BUZZFEED_PEERS_CONFIRMATIONS;

void BuzzfeedItem::merge(const BuzzfeedItemUpdate& update) {
	//
	switch(update.field()) {
		case BuzzfeedItemUpdate::LIKES: 
			if (likes() <= update.count())
				setLikes(update.count());
			likesChanged();
			break;
		case BuzzfeedItemUpdate::REBUZZES: 
			if (rebuzzes() <= update.count())
				setRebuzzes(update.count());
			rebuzzesChanged();
			break;
		case BuzzfeedItemUpdate::REPLIES: 
			if (replies() <= update.count())
				setReplies(update.count());
			repliesChanged();
			break;
		case BuzzfeedItemUpdate::REWARDS: 
			if (rewards() <= update.count())
				setRewards(update.count());
			rewardsChanged();
			break;
	}
}

void BuzzfeedItem::merge(const std::vector<BuzzfeedItemUpdate>& updates) {
	//
	Guard lLock(this);

	for (std::vector<BuzzfeedItemUpdate>::const_iterator lUpdate = updates.begin(); lUpdate != updates.end(); lUpdate++) {
		// locate buzz
		BuzzfeedItemPtr lBuzz = locateBuzz(Key(lUpdate->buzzId(), TX_BUZZ));
		if (lBuzz) lBuzz->merge(*lUpdate);

		lBuzz = locateBuzz(Key(lUpdate->buzzId(), TX_REBUZZ));
		if (lBuzz) lBuzz->merge(*lUpdate);

		lBuzz = locateBuzz(Key(lUpdate->buzzId(), TX_BUZZ_REPLY));
		if (lBuzz) lBuzz->merge(*lUpdate);

		lBuzz = locateBuzz(Key(lUpdate->buzzId(), TX_BUZZ_LIKE));
		if (lBuzz) lBuzz->merge(*lUpdate);

		lBuzz = locateBuzz(Key(lUpdate->buzzId(), TX_BUZZ_REWARD));
		if (lBuzz) lBuzz->merge(*lUpdate);
	}

	itemsUpdated(updates);
}

bool BuzzfeedItem::merge(const BuzzfeedItem& buzz, bool checkSize, bool notify) {
	//
	BuzzfeedItemPtr lBuzz = BuzzfeedItem::instance(buzz);
	return mergeInternal(lBuzz, checkSize, notify, false);
}

void BuzzfeedItem::removeIndex(BuzzfeedItemPtr item) {
	// clean-up
	std::pair<std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator,
				std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator> lRange = index_.equal_range(item->order());
	for (std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator lExist = lRange.first; lExist != lRange.second; lExist++) {
		if (lExist->second == item->key()) {
			index_.erase(lExist);
			break;
		}
	}
}

void BuzzfeedItem::updateTimestamp(const uint256& publisher, const uint256& chain, uint64_t timestamp) {
	//
	std::map<uint256 /*publisher*/, std::map<uint256 /*chain*/, uint64_t>>::iterator lTimestamp = lastTimestamps_.find(publisher);
	if (lTimestamp != lastTimestamps_.end()) {
		//
		std::map<uint256 /*publisher*/, uint64_t>::iterator lPubChain = lTimestamp->second.find(chain);
		//
		if (lPubChain != lTimestamp->second.end()) {
			if (lPubChain->second > timestamp) lPubChain->second = timestamp;
		} else {
			lTimestamp->second[chain] = timestamp;
		}
	} else {
		lastTimestamps_[publisher][chain] = timestamp;
	}
}

void BuzzfeedItem::insertIndex(BuzzfeedItemPtr item) {
	index_.insert(std::multimap<OrderKey /*order*/, Key /*buzz*/>::value_type(item->order(), item->key()));
}

void BuzzfeedItem::push(const BuzzfeedItem& buzz, const uint160& peer) {
	// put into unconfirmed
	Guard lLock(this);
	bool lProcess = false;
	std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lItem = unconfirmed_.find(buzz.key());
	if (lItem == unconfirmed_.end()) { // absent
		//
		bool lAdd = true;
		// check for signature if source is !BUZZFEED
		if (buzz.source() == Source::BUZZ_SUBSCRIPTION && (buzz.type() == TX_BUZZ_REPLY || buzz.type() == TX_BUZZ_HIDE)) {
			//
			if (verifyUpdateForMyThread_)
				lAdd = verifyUpdateForMyThread_(rootBuzzId_, nonce_, buzz.subscriptionSignature());
		}

		if (lAdd) {
			BuzzfeedItemPtr lBuzz = BuzzfeedItem::instance(buzz);
			lBuzz->setNonce(nonce_);
			lBuzz->setRootBuzzId(rootBuzzId_);
			lProcess = lBuzz->addConfirmation(peer) >= G_BUZZ_PEERS_CONFIRMATIONS;
			lBuzz->notOnChain(); // only for the dynamic updates
			lBuzz->setDynamic(); // dynamic
			//
			lItem = unconfirmed_.insert(std::map<Key /*buzz*/, BuzzfeedItemPtr>::value_type(buzz.key(), lBuzz)).first;
		} else {
			BuzzfeedItemPtr lBuzz = BuzzfeedItem::instance(buzz);
			std::cout << "[PUSH-ERROR]: signature is wrong, buzz = " << rootBuzzId_.toHex() << ", nonce = " << nonce_ << "\n";
			if (gLog().isEnabled(Log::CLIENT))
				gLog().write(Log::CLIENT, strprintf("[PUSH-ERROR]: signature is wrong, buzz = %s, nonce = %d", rootBuzzId_.toHex(), nonce_));
		}
	} else {
		lProcess = lItem->second->addConfirmation(peer) >= G_BUZZ_PEERS_CONFIRMATIONS;
	}

	if (lProcess) {
		//
		BuzzfeedItemPtr lBuzz = lItem->second;
		// remove from unconfirmed
		unconfirmed_.erase(lItem);
		// merge finally
		mergeInternal(lBuzz, true, true, false /*probably*/);

		// cross-merge in case of pending items
		crossMerge(true);
		// resolve info
		if (buzzer()) buzzer()->resolveBuzzerInfos();
		else {
			std::cout << "[PUSH-ERROR]: Buzzer not found" << "\n";
			if (gLog().isEnabled(Log::CLIENT)) 
				gLog().write(Log::CLIENT, "[PUSH-ERROR]: Buzzer not found");
		}
	}
}

bool BuzzfeedItem::publisherResolve() {
	//
	if (buzzerResolve_)
		return buzzerResolve_(
			buzzerId(),
			boost::bind(&BuzzfeedItem::publisherResolved, shared_from_this(), boost::placeholders::_1));
	return false;
}

void BuzzfeedItem::publisherResolved(const uint256& /*publisher*/) {
	//
	if (root_) {
		root_->mergeInternal(shared_from_this(), false, true, false);
	}
}

bool BuzzfeedItem::mergeInternal(BuzzfeedItemPtr buzz, bool checkSize, bool notify, bool suspicious) {
	//
	BuzzfeedItemPtr lBuzz = buzz;

	// callbacks
	lBuzz->setupCallbacks(shared_from_this());
	lBuzz->setRoot(root());
	lBuzz->setHasNextLink(false);
	lBuzz->setHasPrevLink(false);
	lBuzz->resetPulled();
	lBuzz->resetThreaded();

	// merge result
	bool lItemAdded = false;

	// verify signature
	Buzzer::VerificationResult lResult;
	if (suspicious && verifyPublisherLazy_) {
		lResult = verifyPublisherLazy_(lBuzz);
		//if (lResult == Buzzer::VerificationResult::INVALID)
			//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[ERROR-01]: %s", lBuzz->toString()));
	} else {
		lResult = verifyPublisher_(lBuzz);
		//if (lResult == Buzzer::VerificationResult::INVALID)
			//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[ERROR-02]: %s", lBuzz->toString()));
	}

	// supposedly strict
	if (lResult == Buzzer::VerificationResult::INVALID) {
		// try pendings
		std::map<uint256 /*originalBuzz*/, std::map<Key, BuzzfeedItemPtr>>::iterator lPenging = pendings_.find(lBuzz->buzzId());
		if (lPenging != pendings_.end()) { // awaiting for resolve, already pushed to pendings
			// try lazy verification
			lResult = verifyPublisherLazy_(lBuzz);
		}
	}

	if (lResult == Buzzer::VerificationResult::INVALID) {
		if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[ERROR-03]: %s", lBuzz->toString()));
		lBuzz->publisherResolve();
	}

	if (lResult == Buzzer::VerificationResult::SUCCESS || lResult == Buzzer::VerificationResult::POSTPONED) {
		// if buzz was hidden by the owner
		if (buzz->type() == TX_BUZZ_HIDE) {
			//
			std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lExisting = items_.find(Key(buzz->buzzId(), TX_BUZZ));
			if (lExisting == items_.end()) lExisting = items_.find(Key(buzz->buzzId(), TX_BUZZER_MESSAGE));
			//if (lExisting == items_.end()) lExisting = items_.find(Key(buzz->buzzId(), TX_BUZZER_MESSAGE_REPLY));
			if (lExisting == items_.end()) lExisting = items_.find(Key(buzz->buzzId(), TX_BUZZ_REPLY));
			if (lExisting == items_.end()) lExisting = items_.find(Key(buzz->buzzId(), TX_REBUZZ));

			// we finally found referenced item
			if (lExisting != items_.end()) {
				// notify
				itemUpdated(lBuzz); // TODO: implement explicit method to remove
				//
				items_.erase(lExisting);
			}

			return true;
		}

		// check result
		lBuzz->setSignatureVerification(lResult);
		// settings
		lBuzz->key_ = key_;
		lBuzz->sortOrder_ = sortOrder_;
		lBuzz->expand_ = expand_;
		lBuzz->rootBuzzId_ = rootBuzzId_;

		// new item?
		bool lNewItem = true;

		// push buzz
		bool lChanged = false;
		std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lExisting = items_.find(lBuzz->key());
		if (lExisting != items_.end()) {
			//
			if (type_ != TX_BUZZER_MISTRUST && type_ != TX_BUZZER_ENDORSE) {
				lChanged |= lExisting->second->setReplies(lBuzz->replies());
				lChanged |= lExisting->second->setRebuzzes(lBuzz->rebuzzes());
				lChanged |= lExisting->second->setLikes(lBuzz->likes());
				lChanged |= lExisting->second->setRewards(lBuzz->rewards());
			}

			updateTimestamp(buzz->buzzerId(), buzz->buzzChainId(), buzz->timestamp());

			if ((lExisting->second->type() == TX_BUZZ_LIKE && lBuzz->type() == TX_BUZZ_LIKE) ||
				(lExisting->second->type() == TX_BUZZ_REWARD && lBuzz->type() == TX_BUZZ_REWARD) ||
				(lExisting->second->type() == TX_REBUZZ && lBuzz->type() == TX_REBUZZ)) {
				//
				lChanged |= lExisting->second->mergeInfos(lBuzz->infos());
			}

			if (notify && lChanged)
				itemUpdated(lExisting->second);

			return true;
		}

		// orphans
		std::map<uint256 /*buzz*/, std::set<Key>>::iterator lRoot = orphans_.find(lBuzz->buzzId());
		if (lRoot != orphans_.end()) {
			for (std::set<Key>::iterator lId = lRoot->second.begin(); lId != lRoot->second.end(); lId++) {
				//
				std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lChildItem = items_.find(*lId);
				if (lChildItem != items_.end()) {
					BuzzfeedItemPtr lChild = lChildItem->second;
					removeIndex(lChild);
					originalItems_.erase(lChild->buzzId());

					items_.erase(lChildItem);

					lBuzz->setSortOrder(Order::FORWARD);
					/*lItemAdded |=*/ lBuzz->mergeInternal(lChild, checkSize, notify);
				}
			}

			orphans_.erase(lRoot);
		}

		//
		bool lAdd = true;

		// pending
		std::vector<BuzzfeedItemUpdate> lUpdatedPendings;
		std::map<uint256 /*originalBuzz*/, std::map<Key, BuzzfeedItemPtr>>::iterator lPenging = pendings_.find(lBuzz->buzzId());
		if (lPenging != pendings_.end()) {
			for (std::map<Key, BuzzfeedItemPtr>::iterator lItem = lPenging->second.begin(); lItem != lPenging->second.end(); lItem++) {
				if (lItem->second->type() == TX_REBUZZ) {
					//
					lItem->second->wrap(lBuzz);
					lAdd = false;
					//
					lUpdatedPendings.push_back(BuzzfeedItemUpdate(lItem->second->buzzId(),
														  uint256(), BuzzfeedItemUpdate::Field::REBUZZES, 0));
				} else {
					lItem->second->setSortOrder(Order::FORWARD);
					lItemAdded |= lItem->second->mergeInternal(lBuzz, checkSize, notify);
					lAdd = false;
				}
			}

			pendings_.erase(lPenging);
		}

		// 
		if (lUpdatedPendings.size()) itemsUpdated(lUpdatedPendings);

		// force load...
		if (buzz->type() == TX_BUZZ_REPLY) {
			//
			if (buzzId_ != buzz->originalBuzzId()) {
				//
				std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lParent = items_.find(Key(buzz->originalBuzzId(), TX_BUZZ));
				if (lParent == items_.end()) lParent = items_.find(Key(buzz->originalBuzzId(), TX_REBUZZ));
				else if (lParent == items_.end()) lParent = items_.find(Key(buzz->originalBuzzId(), TX_BUZZ_REPLY));

				// merge deep
				if (lParent == items_.end()) {
					//
					BuzzfeedItemPtr lDeepParent = locateBuzzInternal(buzz->originalBuzzId());
					if (lDeepParent) {
						lDeepParent->setSortOrder(Order::FORWARD);
						lItemAdded |= lDeepParent->mergeInternal(lBuzz, checkSize, notify, false /*current*/);
					}
				}

				if (!lItemAdded) {
					if (lParent == items_.end()) {
						//
						lParent = suspicious_.find(Key(buzz->originalBuzzId(), TX_BUZZ));
						if (lParent == suspicious_.end()) lParent = suspicious_.find(Key(buzz->originalBuzzId(), TX_REBUZZ));
						else if (lParent == suspicious_.end()) lParent = suspicious_.find(Key(buzz->originalBuzzId(), TX_BUZZ_REPLY));

						//
						if (lParent == suspicious_.end()) {
							if (isRoot() /*buzzId_ != buzz->originalBuzzId()*/) {
								// mark orphan
								orphans_[buzz->originalBuzzId()].insert(lBuzz->key());
								// notify
								itemAbsent(buzz->originalBuzzChainId(), buzz->originalBuzzId());
								//
								// if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT,
								//	strprintf("[WARNING]: parent is absent for %s", lBuzz->toString()));
							}
							//else {
							//	lAdd = false;
							//}
						} else {
							//
							BuzzfeedItemPtr lParentItem = lParent->second;
							lParentItem->wasSuspicious();

							bool lCandidateAdded = mergeInternal(lParentItem, checkSize, notify, true /*suspicious*/);
							if (lCandidateAdded) {
								//
								lItemAdded |= lCandidateAdded;
								lParentItem->setSortOrder(Order::FORWARD);
								lItemAdded |= lParentItem->mergeInternal(lBuzz, checkSize, notify, false /*current*/);

								suspicious_.erase(lParent); // remove from suspicious

								lAdd = false;
							}
						}
					} else {
						// NOTICE: for static feed it will be annoing behaior - rearrangement of items when you reading
						// removeIndex(lParent->second);
						lParent->second->setSortOrder(Order::FORWARD);
						lItemAdded = lParent->second->mergeInternal(lBuzz, checkSize, notify);

						// NOTICE: for static feed it will be annoing behaior - rearrangement of items when you reading
						// insertIndex(lParent->second);
						lAdd = false;
					}
				}
			}
		} else if (buzz->type() == TX_REBUZZ) {
			// move to pending items
			if (!buzz->originalBuzzId().isNull()) {
				BuzzfeedItemPtr lRef = locateBuzz(buzz->originalBuzzId());
				if (lRef) {
					lBuzz->wrap(lRef);
				} else {
					pendings_[buzz->originalBuzzId()][buzz->key()] = lBuzz; //
				}
			}
		}

		// NOTICE: for static feed it will be annoing behavior - rearrangement of items when you reading
		/*
		if (buzzId_ == buzz->originalBuzzId()) {
			if (order_ < buzz->timestamp()) order_ = buzz->timestamp();
		}
		*/

		if (lAdd && !lItemAdded) {
			if (lNewItem) {
				if (items_.insert(std::map<Key /*buzz*/, BuzzfeedItemPtr>::value_type(lBuzz->key(), lBuzz)).second) {
					originalItems_.insert(std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::value_type(lBuzz->buzzId(), lBuzz));
					index_.insert(std::multimap<OrderKey /*order*/, Key /*buzz*/>::value_type(lBuzz->order(), lBuzz->key()));

					lItemAdded = true;
				}
			}

			updateTimestamp(buzz->buzzerId(), buzz->buzzChainId(), buzz->timestamp());

			if (checkSize && items_.size() > 300 /*setup*/ && lResult == Buzzer::VerificationResult::SUCCESS) {
				// TODO: consider to limit feed, but is that really needed?
				/*
				std::multimap<OrderKey, Key>::iterator lFirst = index_.begin();
				items_[lFirst->second]->clear(); // remove links
				items_.erase(lFirst->second);
				originalItems_.erase(lFirst->second.id());
				index_.erase(lFirst);
				*/
			}

			if (lBuzz->resolve()) {
				if (notify) {
					if (lNewItem) {
						//
						std::vector<BuzzfeedItemUpdate> lUpdated;
						// configuring
						if (lBuzz->type() == TX_BUZZ_REPLY && buzzId_ != rootBuzzId_) {
							// interlink by default
							setHasNextLink(true);
							lBuzz->setHasPrevLink(true);

							// self-update
							lUpdated.push_back(BuzzfeedItemUpdate(buzzId(),
																  uint256(), BuzzfeedItemUpdate::Field::REPLIES, 0));

							// locate neighbours
							std::pair<
									std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator,
									std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator> lRange = index_.equal_range(lBuzz->order());

							for (std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator lSibling = lRange.first; lSibling != lRange.second; lSibling++) {
								if (lSibling->second == lBuzz->key()) {
									//
									// previous sibling
									std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator lPrev = lSibling;
									std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator lNext = lSibling;
									if ((--lPrev) != index_.end()) {
										BuzzfeedItemPtr lSiblingItem = items_[lPrev->second];
										if (lSiblingItem && !(lSiblingItem->key() == lBuzz->key())) {
											lSiblingItem->setHasNextLink(true);
											lUpdated.push_back(BuzzfeedItemUpdate(lSiblingItem->buzzId(),
																				  uint256(), BuzzfeedItemUpdate::Field::REPLIES, 0));
										}
									}

									// next sibling
									if ((++lNext) != index_.end()) {
										BuzzfeedItemPtr lSiblingItem = items_[lNext->second];
										if (lSiblingItem && !(lSiblingItem->key() == lBuzz->key())) {
											lSiblingItem->setHasPrevLink(true);
											lBuzz->setHasNextLink(true);
											lUpdated.push_back(BuzzfeedItemUpdate(lSiblingItem->buzzId(),
																				  uint256(), BuzzfeedItemUpdate::Field::REPLIES, 0));
										}
									}

									break;
								}
							}
						}

						// set pulled
						lBuzz->setPulled();

						// adjust buzzfeed index
						insertNewItem(lBuzz);

						// notify
						itemNew(lBuzz);

						// update neighbors
						if (lUpdated.size()) itemsUpdated(lUpdated);
					}
				}
			}
		}

		return lItemAdded;
	} else {
		//std::cout << "[ERROR]: " << lBuzz->toString() << "\n";
		//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[ERROR]: %s", lBuzz->toString()));

		//
		std::map<uint256 /*buzz*/, std::set<Key>>::iterator lRoot = orphans_.find(lBuzz->buzzId());
		if (lRoot != orphans_.end()) {
			lBuzz->wasSuspicious();
			lItemAdded |= mergeInternal(lBuzz, checkSize, notify, true /*suspicious*/);
		} else {
			suspicious_[lBuzz->key()] = lBuzz;
		}
	}

	return lItemAdded;
}

void BuzzfeedItem::merge(const std::vector<BuzzfeedItem>& chunk, int requests, bool notify) {
	//
	{
		Guard lLock(this);
		//
		for (std::vector<BuzzfeedItem>::iterator lItem = const_cast<std::vector<BuzzfeedItem>&>(chunk).begin(); 
												lItem != const_cast<std::vector<BuzzfeedItem>&>(chunk).end(); lItem++) {
			//
			std::map<Key /*id*/, _commit>::iterator lExisting = commit_.find(lItem->key());
			if (lExisting == commit_.end()) {
				commit_.insert(std::map<Key /*id*/, _commit>::value_type(lItem->key(), _commit(BuzzfeedItem::instance(*lItem))));
			} else {
				lExisting->second.commit();
			}
		}

		if (notify /*done*/) {
			//
			for (std::map<Key /*id*/, _commit>::iterator lCandidate = commit_.begin(); lCandidate != commit_.end(); lCandidate++) {
				if (merge_ == Merge::UNION ||
						lCandidate->second.count_ == requests) {
					mergeInternal(lCandidate->second.candidate_, false, false, false);
				}
			}

			commit_.clear();
		}
	}

	if (notify) largeUpdated();
}

bool BuzzfeedItem::mergeAppend(const std::vector<BuzzfeedItemPtr>& items) {
	//
	Guard lLock(this);
	//
	bool lItemsAdded = false;
	for (std::vector<BuzzfeedItemPtr>::const_iterator lItem = items.begin(); lItem != items.end(); lItem++) {
		//
		lItemsAdded |= merge(*(*lItem), true, false);
	}

	return lItemsAdded;
}

void BuzzfeedItem::clear() {
	//
	{
		Guard lLock(this);
		//
		for (std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lItem = items_.begin(); lItem != items_.end(); lItem++) {
			//
			lItem->second->clear();
		}

		for (std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lItem = originalItems_.begin(); lItem != originalItems_.end(); lItem++) {
			//
			lItem->second->clear();
		}

		items_.clear();
		originalItems_.clear();
		index_.clear();
		orphans_.clear();
		lastTimestamps_.clear();
		pendings_.clear();
		suspicious_.clear();
		unconfirmed_.clear();
		commit_.clear();

		wrapped_.reset();
		parent_.reset();
	}

	root_.reset();
}

BuzzfeedItemPtr BuzzfeedItem::firstChild() {
	//
	Guard lLock(this);
	if (sortOrder_ == Order::REVERSE) {
		//
		std::multimap<OrderKey /*order*/, Key /*buzz*/>::reverse_iterator lItem = index_.rbegin();
		if (lItem != index_.rend()) {
			//
			std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
			if (lBuzz != items_.end()) {
				if (lBuzz->second->valid()) {
					return lBuzz->second;
				}
			}
		}
	} else {
		//
		std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator lItem = index_.begin();
		if (lItem != index_.end()) {
			//
			std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
			if (lBuzz != items_.end()) {
				if (lBuzz->second->valid()) {
					return lBuzz->second;
				}
			}
		}
	}

	return nullptr;
}

void BuzzfeedItem::feed(std::vector<BuzzfeedItemPtr>& feed, bool expanded) {
	//
	Guard lLock(this);
	if (sortOrder_ == Order::REVERSE) {
		//
		if (expand_ == Expand::FULL || (!expanded || (expanded && index_.size() < 5 /**/))) {
			for (std::multimap<OrderKey /*order*/, Key /*buzz*/>::reverse_iterator lItem = index_.rbegin(); 
													lItem != index_.rend(); lItem++) {
				//
				std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
				// if exists and VALID (signature checked)
				if (lBuzz != items_.end()) {
					if (lBuzz->second->valid() && !lBuzz->second->blocked()) {
						//
						if (!buzzId_.isEmpty() && lBuzz->second->originalBuzzId() == buzzId_ && buzzId_ != rootBuzzId_) {
							setHasNextLink(true);
							lBuzz->second->setHasPrevLink(true);
						}
						//
						std::vector<BuzzfeedItemPtr>::reverse_iterator lLastItem = feed.rbegin();
						if (lLastItem != feed.rend()) {
							if (!buzzId_.isEmpty() && (((*lLastItem)->originalBuzzId() == buzzId_ && buzzId_ != rootBuzzId_) ||
									(lBuzz->second->hasPrevLink() && (*lLastItem)->hasPrevLink()))) {
								(*lLastItem)->setHasNextLink(true);
							}
						}

						// push
						feed.push_back(lBuzz->second);
						lBuzz->second->setPulled();

						// expand
						lBuzz->second->feed(feed, true);
					} else {
						std::cout << "[FEED-ERROR]: " << lBuzz->second->toString() << "\n";
						if (gLog().isEnabled(Log::CLIENT))
							gLog().write(Log::CLIENT, strprintf("[FEED-ERROR]: %s", lBuzz->second->toString()));
					}
				} else {
					if (gLog().isEnabled(Log::CLIENT))
						gLog().write(Log::CLIENT, strprintf("[FEED-ERROR-NOT-FOUND]: %s", lBuzz->second->toString()));
				}
			}
		} else {
			//
			for (std::multimap<OrderKey /*order*/, Key /*buzz*/>::reverse_iterator lItem = index_.rbegin(); 
													lItem != index_.rend(); lItem++) {
				//
				std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
				// if exists and VALID (signature checked)
				if (lBuzz != items_.end()) {
					if (lBuzz->second->valid() && !lBuzz->second->blocked()) {
						// push
						bool lAdd = true;
						std::vector<BuzzfeedItemPtr>::reverse_iterator lLastItem = feed.rbegin();
						if (lLastItem != feed.rend()) {
							if ((*lLastItem)->buzzerId() == lBuzz->second->buzzerId()) {
								lAdd = false;
							}
						}

						if (lAdd) {
							//
							if (!buzzId_.isEmpty() && lBuzz->second->originalBuzzId() == buzzId_ && buzzId_ != rootBuzzId_) {
								setHasNextLink(true);
								lBuzz->second->setHasPrevLink(true);
							}
							//
							if (lLastItem != feed.rend()) {
								if (!buzzId_.isEmpty() && (((*lLastItem)->originalBuzzId() == buzzId_ && buzzId_ != rootBuzzId_) ||
										(lBuzz->second->hasPrevLink() && (*lLastItem)->hasPrevLink()))) {
									(*lLastItem)->setHasNextLink(true);
								}
							}

							// add
							feed.push_back(lBuzz->second);
							lBuzz->second->setPulled();
							// ... and expand
							lBuzz->second->feed(feed, true);
						} else {
							threaded_ = true;
						}
					} else {
						std::cout << "[FEED-ERROR]: " << lBuzz->second->toString() << "\n";
						if (gLog().isEnabled(Log::CLIENT))
							gLog().write(Log::CLIENT, strprintf("[FEED-ERROR]: %s", lBuzz->second->toString()));
					}
				} else {
					if (gLog().isEnabled(Log::CLIENT))
						gLog().write(Log::CLIENT, strprintf("[FEED-ERROR-NOT-FOUND]: %s", lBuzz->second->toString()));
				}
			}
		}
	} else {
		// FORWARD
		if (expand_ == Expand::FULL || (!expanded || (expanded && index_.size() < 5 /**/))) {
			for (std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator lItem = index_.begin(); 
													lItem != index_.end(); lItem++) {
				//
				std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
				// if exists and VALID (signature checked)
				if (lBuzz != items_.end()) {
					if (lBuzz->second->valid() && !lBuzz->second->blocked()) {
						//
						if (!buzzId_.isEmpty() && lBuzz->second->originalBuzzId() == buzzId_ && buzzId_ != rootBuzzId_) {
							setHasNextLink(true);
							lBuzz->second->setHasPrevLink(true);
						}
						//
						std::vector<BuzzfeedItemPtr>::reverse_iterator lLastItem = feed.rbegin();
						if (lLastItem != feed.rend()) {
							if (!buzzId_.isEmpty() && (((*lLastItem)->originalBuzzId() == buzzId_ && buzzId_ != rootBuzzId_) ||
									(lBuzz->second->hasPrevLink() && (*lLastItem)->hasPrevLink()))) {
								(*lLastItem)->setHasNextLink(true);
							}
						}

						// push
						feed.push_back(lBuzz->second);
						lBuzz->second->setPulled();
						// expand
						lBuzz->second->feed(feed, true);
					} else {
						std::cout << "[FEED-ERROR]: " << lBuzz->second->toString() << "\n";
						if (gLog().isEnabled(Log::CLIENT))
							gLog().write(Log::CLIENT, strprintf("[FEED-ERROR]: %s", lBuzz->second->toString()));
					}
				} else {
					if (gLog().isEnabled(Log::CLIENT))
						gLog().write(Log::CLIENT, strprintf("[FEED-ERROR-NOT-FOUND]: %s", lBuzz->second->toString()));
				}
			}
		} else {
			for (std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator lItem = index_.begin(); 
													lItem != index_.end(); lItem++) {
				//
				std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
				// if exists and VALID (signature checked)
				if (lBuzz != items_.end()) {
					if (lBuzz->second->valid() && !lBuzz->second->blocked()) {
						// push
						bool lReplace = false;
						std::vector<BuzzfeedItemPtr>::reverse_iterator lLastItem = feed.rbegin();
						if (lLastItem != feed.rend()) {
							if ((*lLastItem)->buzzerId() == lBuzz->second->buzzerId()) {
								lReplace = true;
							}
						}

						if (lReplace) {
							// replace
							feed[feed.size()-1]->resetPulled();
							feed[feed.size()-1] = lBuzz->second;

							//
							if (!buzzId_.isEmpty() && lBuzz->second->originalBuzzId() == buzzId_ && buzzId_ != rootBuzzId_) {
								setHasNextLink(true);
								lBuzz->second->setHasPrevLink(true);
							}
							//
							if (lLastItem != feed.rend() && ++lLastItem != feed.rend()) {
								if (!buzzId_.isEmpty() && (((*lLastItem)->originalBuzzId() == buzzId_ && buzzId_ != rootBuzzId_) ||
										(lBuzz->second->hasPrevLink() && (*lLastItem)->hasPrevLink()))) {
									(*lLastItem)->setHasNextLink(true);
								}
							}

							// ... and expand
							lBuzz->second->feed(feed, true);
							//
							threaded_ = true;
						} else {
							//
							if (lBuzz->second->originalBuzzId() == buzzId_ && buzzId_ != rootBuzzId_) {
								setHasNextLink(true);
								lBuzz->second->setHasPrevLink(true);
							}
							//
							if (lLastItem != feed.rend()) {
								if (!buzzId_.isEmpty() && (((*lLastItem)->originalBuzzId() == buzzId_ && buzzId_ != rootBuzzId_) ||
										(lBuzz->second->hasPrevLink() && (*lLastItem)->hasPrevLink()))) {
									(*lLastItem)->setHasNextLink(true);
								}
							}

							// add
							feed.push_back(lBuzz->second);
							lBuzz->second->setPulled();

							// ... and expand
							lBuzz->second->feed(feed, true);
						}
					} else {
						std::cout << "[FEED-ERROR]: " << lBuzz->second->toString() << "\n";
						if (gLog().isEnabled(Log::CLIENT))
							gLog().write(Log::CLIENT, strprintf("[FEED-ERROR]: %s", lBuzz->second->toString()));
					}
				} else {
					if (gLog().isEnabled(Log::CLIENT))
						gLog().write(Log::CLIENT, strprintf("[FEED-ERROR-NOT-FOUND]: %s", lBuzz->second->toString()));
				}
			}
		}
	}
}

bool BuzzfeedItem::locateIndex(BuzzfeedItemPtr item, int& index, uint256& lastBuzerId, bool expanded) {
	//
	if (sortOrder_ == Order::REVERSE) {
		//
		if (expand_ == Expand::FULL || (!expanded || (expanded && index_.size() < 5 /**/))) {
			for (std::multimap<OrderKey /*order*/, Key /*buzz*/>::reverse_iterator lItem = index_.rbegin();
													lItem != index_.rend(); lItem++, index++) {
				//
				std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
				if (lBuzz != items_.end() && item->key() == lBuzz->second->key()) {
					//
					//index++;
					return true;
				}

				// expand
				if (lBuzz->second->locateIndex(item, index, lastBuzerId, true)) {
					index++; // +level
					return true;
				}
			}
		} else {
			//
			for (std::multimap<OrderKey /*order*/, Key /*buzz*/>::reverse_iterator lItem = index_.rbegin();
													lItem != index_.rend(); lItem++) {
				//
				std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
				if (lBuzz != items_.end() && item->key() == lBuzz->second->key()) {
					//
					//index++;
					return true;
				}

				// push
				bool lAdd = true;
				if (lastBuzerId == lBuzz->second->buzzerId()) {
					lAdd = false;
				} else {
					lastBuzerId = lBuzz->second->buzzerId();
				}

				if (lAdd || lBuzz->second->pulled()) {
					//
					index++;
					// ... and expand
					if (lBuzz->second->locateIndex(item, index, lastBuzerId, true)) {
						index++; // +level
						return true;
					}
				}
			}
		}
	} else {
		// FORWARD
		if (expand_ == Expand::FULL || (!expanded || (expanded && index_.size() < 5 /**/))) {
			for (std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator lItem = index_.begin();
													lItem != index_.end(); lItem++, index++) {
				//
				std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
				if (lBuzz != items_.end() && item->key() == lBuzz->second->key()) {
					//
					//index++;
					return true;
				}

				// expand
				if (lBuzz->second->locateIndex(item, index, lastBuzerId, true)) {
					index++; // +level
					return true;
				}
			}
		} else {
			for (std::multimap<OrderKey /*order*/, Key /*buzz*/>::iterator lItem = index_.begin();
													lItem != index_.end(); lItem++) {
				//
				std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = items_.find(lItem->second);
				if (lBuzz != items_.end() && item->key() == lBuzz->second->key()) {
					//
					//index++;
					return true;
				}

				if (lastBuzerId != lBuzz->second->buzzerId() || lBuzz->second->pulled()) {
					index++;
				}

				//
				lastBuzerId = lBuzz->second->buzzerId();
				// ... and expand
				if (lBuzz->second->locateIndex(item, index, lastBuzerId, true)) {
					index++; // +level
					return true;
				}
			}
		}
	}

	return false;
}

int BuzzfeedItem::locateIndex(BuzzfeedItemPtr item) {
	//
	Guard lLock(this);

	uint256 lLastBuzerId;
	int lIndex = 0;

	if (locateIndex(item, lIndex, lLastBuzerId, false)) return lIndex;
	return -1;
}

BuzzfeedItemPtr BuzzfeedItem::locateBuzz(const uint256& buzz) {
	//
	Guard lLock(this);
	return locateBuzzInternal(buzz);
}

BuzzfeedItemPtr BuzzfeedItem::locateBuzzInternal(const uint256& buzz) {
	//
	std::map<uint256 /*buzz*/, BuzzfeedItemPtr>::iterator lBuzz = originalItems_.find(buzz);
	if (lBuzz != originalItems_.end()) return lBuzz->second;

	//
	for (std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lItem = items_.begin(); 
														lItem != items_.end(); lItem++) {
		if (lItem->second) {
			BuzzfeedItemPtr lBuzzItem = lItem->second->locateBuzzInternal(buzz);
			if (lBuzzItem) return lBuzzItem;
		}
	}

	return nullptr;
}

void BuzzfeedItem::locateLastTimestamp(std::map<uint256 /*chain*/, std::vector<BuzzfeedPublisherFrom>>& timestamps) {
	//
	Guard lLock(this);
	for (std::map<uint256 /*publisher*/, std::map<uint256 /*chain*/, uint64_t>>::iterator lTimestamp = lastTimestamps_.begin(); lTimestamp != lastTimestamps_.end(); lTimestamp++) {
		//
		for (std::map<uint256 /*chain*/, uint64_t>::iterator lPubChain = lTimestamp->second.begin(); lPubChain != lTimestamp->second.end(); lPubChain++) {
			timestamps[lPubChain->first].push_back(BuzzfeedPublisherFrom(lTimestamp->first, lPubChain->second));

			//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT,
			//	strprintf("[locateLastTimestamp]: %s/%s#, ts = %d", lTimestamp->first.toHex(), lPubChain->first.toHex().substr(0, 10), lPubChain->second));
		}
	}

	for (std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lItem = items_.begin();
														lItem != items_.end(); lItem++) {
		lItem->second->locateLastTimestamp(timestamps);
	}
}

uint64_t BuzzfeedItem::locateLastTimestamp() {
	//
	Guard lLock(this);
	std::set<uint64_t> lOrder;
	for (std::map<uint256 /*publisher*/, std::map<uint256 /*chain*/, uint64_t>>::iterator lTimestamp = lastTimestamps_.begin(); lTimestamp != lastTimestamps_.end(); lTimestamp++) {
		for (std::map<uint256 /*chain*/, uint64_t>::iterator lPubChain = lTimestamp->second.begin(); lPubChain != lTimestamp->second.end(); lPubChain++) {
			lOrder.insert(lPubChain->second);
		}
	}

	for (std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lItem = items_.begin();
														lItem != items_.end(); lItem++) {
		lItem->second->locateLastTimestamp(lOrder);
	}

	for (auto &lId: lOrder) {
		std::cout << lId << "\n";
	}

	return lOrder.size() ? (sortOrder_ == Order::REVERSE ? *lOrder.begin() : *lOrder.rbegin()) : 0;
}

void BuzzfeedItem::locateLastTimestamp(std::set<uint64_t>& set) {
	//
	Guard lLock(this);
	for (std::map<uint256 /*publisher*/, std::map<uint256 /*chain*/, uint64_t>>::iterator lTimestamp = lastTimestamps_.begin(); lTimestamp != lastTimestamps_.end(); lTimestamp++) {
		for (std::map<uint256 /*chain*/, uint64_t>::iterator lPubChain = lTimestamp->second.begin(); lPubChain != lTimestamp->second.end(); lPubChain++)
			set.insert(lPubChain->second);
	}

	for (std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lItem = items_.begin();
														lItem != items_.end(); lItem++) {
		lItem->second->locateLastTimestamp(set);
	}
}

void BuzzfeedItem::collectPendingItems(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>& items) {
	//
	Guard lLock(this);
	for (std::map<uint256 /*originalBuzz*/, std::map<Key, BuzzfeedItemPtr>>::iterator lPending = pendings_.begin(); lPending != pendings_.end(); lPending++) {
		for (std::map<Key, BuzzfeedItemPtr>::iterator lItem = lPending->second.begin(); lItem != lPending->second.end(); lItem++)
			if (!lItem->second->originalBuzzId().isNull()) items[lItem->second->originalBuzzChainId()].insert(lItem->second->originalBuzzId());
	}
}

void BuzzfeedItem::crossMerge(bool notify) {
	//
	Guard lLock(this);
	std::vector<BuzzfeedItemUpdate> lUpdatedPendings;
	std::map<uint256 /*originalBuzz*/, std::map<Key, BuzzfeedItemPtr>> lPendings = pendings_;
	for (std::map<uint256 /*originalBuzz*/, std::map<Key, BuzzfeedItemPtr>>::iterator lPending = lPendings.begin(); lPending != lPendings.end(); lPending++) {
		//
		std::map<Key /*buzz*/, BuzzfeedItemPtr>::iterator lParent = items_.find(Key(lPending->first, TX_REBUZZ));
		if (lParent != items_.end()) {
			for (std::map<Key, BuzzfeedItemPtr>::iterator lItem = lPending->second.begin(); lItem != lPending->second.end(); lItem++) {
				if (lItem->second->type() == TX_REBUZZ) {
					lItem->second->wrap(lParent->second);
					lUpdatedPendings.push_back(BuzzfeedItemUpdate(lItem->second->buzzId(),
														  uint256(), BuzzfeedItemUpdate::Field::REBUZZES, 0));

				} else {
					lItem->second->merge(*(lParent->second), true, notify);
				}
			}

			pendings_.erase(lPending->first);
		}
	}
	// 
	if (lUpdatedPendings.size()) itemsUpdated(lUpdatedPendings);
}

const std::vector<unsigned char>& BuzzfeedItem::buzzBody() const {
	return buzzBody_; 
}

bool BuzzfeedItem::resolvePKey(PKey& pkey) {
	//
	if (pkeyResolve_ && pkeyResolve_(rootBuzzId_, pkey)) {
		return true;
	}

	return false;
}

bool BuzzfeedItem::decrypt(const PKey& pkey) {
	//
	if (type_ == TX_BUZZER_MESSAGE || type_ == TX_BUZZER_MESSAGE_REPLY) {
		//
		if (!decryptedBody_.size() && buzzBody_.size() && buzzer()) {
			SKeyPtr lSKey = buzzer()->wallet()->firstKey();
			//
			uint256 lNonce = lSKey->shared(pkey);
			return TxBuzzerMessage::decrypt(lNonce, buzzBody_, decryptedBody_);
		}

		if (decryptedBody_.size()) return true;
	}

	return false;
}

const std::string& BuzzfeedItem::buzzBodyString() const {
	//
	if (!preparedBody_.size()) {
		if (type_ == TX_BUZZER_MESSAGE || type_ == TX_BUZZER_MESSAGE_REPLY) {
			//
			if (!decryptedBody_.size() && buzzBody_.size() && buzzer()) {
				SKeyPtr lSKey = buzzer()->wallet()->firstKey();
				//
				PKey lPKey;
				if (pkeyResolve_ && pkeyResolve_(rootBuzzId_, lPKey)) {
					//
					uint256 lNonce = lSKey->shared(lPKey);
					TxBuzzerMessage::decrypt(lNonce, buzzBody_, decryptedBody_);
				}
			}

			preparedBody_.insert(preparedBody_.end(), decryptedBody_.begin(), decryptedBody_.end());
			return preparedBody_;
		}

		preparedBody_.insert(preparedBody_.end(), buzzBody_.begin(), buzzBody_.end());
	}

	return preparedBody_;
}

const std::string& BuzzfeedItem::decryptedBuzzBodyString() const {
	//
	if (!preparedBody_.size()) {
		preparedBody_.insert(preparedBody_.end(), decryptedBody_.begin(), decryptedBody_.end());
	}

	return preparedBody_;
}

//
// Buzzfeed
//

void Buzzfeed::feed(std::vector<BuzzfeedItemPtr>& list, bool /*expanded*/) {
	//
	Guard lLock(this);
	list_.clear();
	index_.clear();
	BuzzfeedItem::feed(list_);

	// build index
	for (size_t lItem = 0; lItem < list_.size(); lItem++) {
		index_[list_[lItem]->key()] = lItem;
	}

	//
	list.insert(list.end(), list_.begin(), list_.end());

	//
	fed_ = true;
}

int Buzzfeed::locateIndex(BuzzfeedItemPtr item) {
	//
	Guard lLock(this);
	std::map<BuzzfeedItem::Key, int>::iterator lItem = index_.find(item->key());
	if (lItem != index_.end()) return lItem->second;
	return -1;
}

void Buzzfeed::insertNewItem(BuzzfeedItemPtr item) {
	//
	// NOTICE: bug with unlocked merge from two threads
	//
	Guard lLock(this);
	//
	int lIndex = BuzzfeedItem::locateIndex(item);
	if (lIndex != -1 && list_.begin() != list_.end()) {
		//
		if (index_.find(item->key()) == index_.end()) {
			list_.insert(list_.begin() + lIndex, item);
			index_[item->key()] = lIndex;

			//
			if (lIndex-1 >= 0 && list_[lIndex-1]->hasNextLink()) {
				item->setHasPrevLink(true);
			}

			if (lIndex+1 < (int)list_.size() && list_[lIndex+1]->hasPrevLink()) {
				item->setHasNextLink(true);
			}

			if (lIndex-1 >= 0 && /*list_[lIndex-1]->originalBuzzId() == item->originalBuzzId() &&*/
					item->hasPrevLink() &&
					list_[lIndex-1]->hasPrevLink()) {
				list_[lIndex-1]->setHasNextLink(true);
			}
		}
	} else if (!list_.size() && fed_) {
		// push first, BUG?
		list_.push_back(item);
		index_[item->key()] = 0;
	}
}

BuzzfeedItemPtr Buzzfeed::locateBuzz(const Key& key) {
	//
	Guard lLock(this);
	//
	std::map<qbit::BuzzfeedItem::Key, int>::iterator lEntry = index_.find(key);
	if (lEntry != index_.end() && (size_t)lEntry->second < list_.size()) {
		return list_[lEntry->second];
	}

	return nullptr;
}

void Buzzfeed::clear() {
	//
	Guard lLock(this);
	list_.clear();
	index_.clear();

	BuzzfeedItem::clear();

	fed_ = false;
}

void Buzzfeed::collectLastItemsByChains(std::map<uint256, BuzzfeedItemPtr>& items) {
	//
	for (std::vector<BuzzfeedItemPtr>::reverse_iterator lItem = list_.rbegin(); lItem != list_.rend(); lItem++) {
		//
		items.insert(std::map<uint256, BuzzfeedItemPtr>::value_type((*lItem)->buzzChainId(), *lItem));
	}
}
