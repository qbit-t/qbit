#include "transactionstoreextension.h"
#include "../../log/log.h"
#include "../../mkpath.h"
#include "txbuzzer.h"
#include "txbuzz.h"
#include "txbuzzreply.h"
#include "txbuzzlike.h"
#include "txbuzzersubscribe.h"
#include "txbuzzerunsubscribe.h"

#include <iostream>

using namespace qbit;

bool BuzzerTransactionStoreExtension::open() {
	//
	if (!opened_) { 
		try {
			gLog().write(Log::INFO, std::string("[extension/open]: opening storage extension for ") + strprintf("%s", store_->chain().toHex()));

			if (mkpath(std::string(settings_->dataPath() + "/" + store_->chain().toHex() + "/buzzer/indexes").c_str(), 0777)) return false;

			timeline_.open();
			events_.open();
			subscriptionsIdx_.open();
			likesIdx_.open();
			replies_.open();
			rebuzzes_.open();
			publisherUpdates_.open();
			subscriberUpdates_.open();
			buzzInfo_.open();
			buzzerInfo_.open();

			opened_ = true;
		}
		catch(const std::exception& ex) {
			gLog().write(Log::ERROR, std::string("[extension/open/error]: ") + ex.what());
			return false;
		}
	}

	return opened_;
}

bool BuzzerTransactionStoreExtension::close() {
	//
	gLog().write(Log::INFO, std::string("[extension/close]: closing storage extension for ") + strprintf("%s#...", store_->chain().toHex().substr(0, 10)));

	settings_.reset();
	store_.reset();
	
	timeline_.close();
	events_.close();
	subscriptionsIdx_.close();
	likesIdx_.close();
	replies_.close();
	rebuzzes_.close();
	publisherUpdates_.close();
	subscriberUpdates_.close();
	buzzInfo_.close();
	buzzerInfo_.close();

	opened_ = false;

	return true;
}

bool BuzzerTransactionStoreExtension::pushEntity(const uint256& id, TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZ ||
		ctx->tx()->type() == TX_REBUZZ ||
		ctx->tx()->type() == TX_BUZZ_LIKE || 
		ctx->tx()->type() == TX_BUZZ_REPLY ||
		ctx->tx()->type() == TX_BUZZER_ENDORSE ||
		ctx->tx()->type() == TX_BUZZER_MISTRUST) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing event ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));

		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		// in[0] - publisher/initiator
		uint256 lPublisher = lEvent->in()[0].out().tx(); 
		// write timeline
		timeline_.write(lPublisher, lEvent->timestamp(), lEvent->id());
		// mark last updates
		publisherUpdates(lPublisher, lEvent->timestamp());

		// extract buzzers
		std::vector<Transaction::In>& lIns = lEvent->in(); 
		std::vector<Transaction::In>::iterator lInPtr = lIns.begin(); lInPtr++; // buzzer pointer
		for(; lInPtr != lIns.end(); lInPtr++) {
			//
			Transaction::In& lIn = (*lInPtr);
			ITransactionStorePtr lLocalStore = store_->storeManager()->locate(lIn.out().chain());
			TransactionPtr lInTx = lLocalStore->locateTransaction(lIn.out().tx());

			if (lInTx != nullptr) {
				if (lInTx->type() == TX_BUZZER) {
					// direct events - mentions
					events_.write(lInTx->id(), lEvent->timestamp(), lEvent->id(), lEvent->type());
					subscriberUpdates(lInTx->id(), lEvent->timestamp());
				} else if (lInTx->type() == TX_BUZZ || lInTx->type() == TX_REBUZZ || lInTx->type() == TX_BUZZ_REPLY) {
					// re-buzz, reply
					// WARNING: reply and rebuzz are the similar by in and outs, that is TX_BUZZ_REPLY_MY_IN applicable for both
					events_.write(lInTx->in()[TX_BUZZ_REPLY_MY_IN].out().tx(), lEvent->timestamp(), lEvent->id(), lEvent->type());
					subscriberUpdates(lInTx->in()[TX_BUZZ_REPLY_MY_IN].out().tx(), lEvent->timestamp());
					// 
					if (ctx->tx()->type() == TX_REBUZZ) { 
						incrementRebuzzes(lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx());
						rebuzzes_.write(lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx(), lEvent->id(), lPublisher);
					} else if (ctx->tx()->type() == TX_BUZZ_REPLY) {
						//
						uint256 lReTxId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
						uint256 lReChainId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
						while (true) {
							//
							lLocalStore = store_->storeManager()->locate(lReChainId);
							TransactionPtr lReTx = store_->locateTransaction(lReTxId);
							if (lReTx->type() == TX_BUZZ_REPLY || lReTx->type() == TX_REBUZZ ||
																		lReTx->type() == TX_BUZZ) {
								// inc replies
								incrementReplies(lReTxId);
								// make tree
								replies_.write(lReTxId, lEvent->id(), lPublisher);

								if (lReTx->type() == TX_BUZZ || lReTx->type() == TX_REBUZZ) {
									break;
								}

								if (lReTx->in().size() > TX_BUZZ_REPLY_BUZZ_IN) { // move up-next
									lReTxId = lReTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
									lReChainId = lReTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
								}
							} else break;
						}
					}
				}
			}
		}
	}
	
	if (ctx->tx()->type() == TX_BUZZER_SUBSCRIBE) {
		//
		TxBuzzerSubscribePtr lBuzzSubscribe = TransactionHelper::to<TxBuzzerSubscribe>(ctx->tx());
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing buzzer subscription ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));

		if (lBuzzSubscribe->in().size() >= 2) {
			//
			uint256 lPublisher = lBuzzSubscribe->in()[0].out().tx(); // in[0] - publisher
			uint256 lSubscriber = lBuzzSubscribe->in()[1].out().tx(); // in[1] - subscriber
			subscriptionsIdx_.write(lSubscriber, lPublisher, lBuzzSubscribe->id());
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: PUSHED buzzer subscription ") +
				strprintf("p = %s, s = %s, %s/%s#", lPublisher.toHex(), lSubscriber.toHex(), ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
		} else {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/error]: buzzer subscription IS NOT consistent"));
		}
	} else if (ctx->tx()->type() == TX_BUZZER_UNSUBSCRIBE) {
		//
		TxBuzzerUnsubscribePtr lBuzzUnsubscribe = TransactionHelper::to<TxBuzzerUnsubscribe>(ctx->tx());
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removing buzzer subscription ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));

		if (lBuzzUnsubscribe->in().size()) {
			// locate subscription
			TransactionPtr lBuzzSubscribe = store_->locateTransaction(lBuzzUnsubscribe->in()[0].out().tx());
			if (lBuzzSubscribe) {
				//
				if (lBuzzSubscribe->in().size() >= 2) {
					//
					uint256 lPublisher = lBuzzSubscribe->in()[0].out().tx(); // in[0] - publisher
					uint256 lSubscriber = lBuzzSubscribe->in()[1].out().tx(); // in[1] - subscriber
					subscriptionsIdx_.remove(lSubscriber, lPublisher);
					//
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzzer subscription REMOVED ") +
						strprintf("p = %s, s = %s, %s/%s#", lPublisher.toHex(), lSubscriber.toHex(), ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));

				} else {
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/error]: removing buzzer subscription - subscription IS NOT consistent"));
				}
			} else {
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/error]: removing buzzer subscription - buzzer subscription IS NOT found"));
			}
		} else {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/error]: buzzer unsubscription IS NOT consistent"));
		}
	} else if (ctx->tx()->type() == TX_BUZZER) {
		// do nothing
	} else if (ctx->tx()->type() == TX_BUZZ_LIKE) {
		// extract buzz_id
		TxBuzzLikePtr lBuzzLike = TransactionHelper::to<TxBuzzLike>(ctx->tx());
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing buzz like ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		if (lBuzzLike->in().size() >= 1) {
			// extract buzz | buzzer
			uint256 lBuzzId = lBuzzLike->in()[TX_BUZZ_LIKE_IN].out().tx(); // buzz_id
			uint256 lBuzzerId = lBuzzLike->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
			// check index
			if (!checkLike(lBuzzId, lBuzzerId)) {
				// write index
				likesIdx_.write(lBuzzId, lBuzzerId, lBuzzLike->id());

				// update like count
				incrementLikes(lBuzzId);

				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzz like PUSHED: ") +
					strprintf("tx = %s, buzz = %s, liker = %s", ctx->tx()->id().toHex(), lBuzzId.toHex(), lBuzzerId.toHex()));
			} else {
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/error]: buzz is ALREADY liked: ") +
					strprintf("tx = %s, buzz = %s, liker = %s", ctx->tx()->id().toHex(), lBuzzId.toHex(), lBuzzerId.toHex()));
			}
		}
	}

	return true;
}

void BuzzerTransactionStoreExtension::removeTransaction(TransactionPtr tx) {
	//
	if (tx->type() == TX_BUZZ ||
		tx->type() == TX_REBUZZ ||
		tx->type() == TX_BUZZ_LIKE || 
		tx->type() == TX_BUZZ_REPLY ||
		tx->type() == TX_BUZZER_ENDORSE ||
		tx->type() == TX_BUZZER_MISTRUST) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/removeTransaction]: removing event from timeline ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(tx);
		// in[0] - publisher/initiator
		uint256 lPublisher = lEvent->in()[0].out().tx();

		// remove rebuzz/reply count
		if (tx->type() == TX_REBUZZ ||
			tx->type() == TX_BUZZ_REPLY) {
			//
			std::vector<Transaction::In>& lIns = lEvent->in(); 
			std::vector<Transaction::In>::iterator lInPtr = lIns.begin(); lInPtr++; // buzzer pointer
			for(; lInPtr != lIns.end(); lInPtr++) {
				//
				Transaction::In& lIn = (*lInPtr);
				TransactionPtr lInTx = store_->locateTransaction(lIn.out().tx());

				if (lInTx != nullptr) {
					if (lInTx->type() == TX_BUZZ || lInTx->type() == TX_REBUZZ || lInTx->type() == TX_BUZZ_REPLY) {
						// dec replies
						if (tx->type() == TX_REBUZZ) { 
							decrementRebuzzes(lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx());
							rebuzzes_.remove(lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx(), lEvent->id());
						} else if (lInTx->type() == TX_BUZZ_REPLY) {
							uint256 lReTxId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
							while (true) {
								//
								TransactionPtr lReTx = store_->locateTransaction(lReTxId);
								// dec replies
								decrementReplies(lReTxId);
								// make tree
								replies_.remove(lReTxId, lEvent->id());

								if (lReTx->type() == TX_BUZZ) { 
									break;
								}

								if (lReTx->in().size() > TX_BUZZ_REPLY_BUZZ_IN) // move up-next
									lReTxId = lReTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
							}
						}
					}
				}
			}
		}

		db::DbTwoKeyContainer<uint256, uint64_t /*timestamp*/, uint256>::Transaction lTimelineTransaction = timeline_.transaction();
		db::DbTwoKeyContainer<uint256, uint64_t /*timestamp*/, uint256>::Iterator lPoint = timeline_.find(lPublisher, lEvent->timestamp());

		for (; lPoint.valid(); lPoint++) {
			lTimelineTransaction.remove(lPoint);
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/removeTransaction]: removing event entry from timeline ") +
				strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		}

		lTimelineTransaction.commit();

		// WARNING: events clean-up will be done at select...

		//
		if (tx->type() == TX_BUZZ ||
			tx->type() == TX_REBUZZ ||
			tx->type() == TX_BUZZ_REPLY) {
			//
			db::DbTwoKeyContainer<uint256 /*buzz*/, uint256 /*liker*/, uint256 /*like_tx*/>::Iterator lBuzzPos = likesIdx_.find(tx->id());
			db::DbTwoKeyContainer<uint256 /*buzz*/, uint256 /*liker*/, uint256 /*like_tx*/>::Transaction lBuzzTransaction = likesIdx_.transaction(); 

			for (; lBuzzPos.valid(); ++lBuzzPos) {
				lBuzzTransaction.remove(lBuzzPos);
			}

			lBuzzTransaction.commit();
		}
	}

	//
	if (tx->type() == TX_BUZZER_SUBSCRIBE) {
		//
		TxBuzzerSubscribePtr lBuzzSubscribe = TransactionHelper::to<TxBuzzerSubscribe>(tx);
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removing buzzer subscription ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));

		if (lBuzzSubscribe->in().size() >= 2) {
			//
			uint256 lPublisher = lBuzzSubscribe->in()[0].out().tx(); // in[0] - publisher
			uint256 lSubscriber = lBuzzSubscribe->in()[1].out().tx(); // in[1] - subscriber
			subscriptionsIdx_.remove(lSubscriber, lPublisher);
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzzer subscription removed ") +
				strprintf("p = %s, s = %s, %s/%s#", lPublisher.toHex(), lSubscriber.toHex(), tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		}
	} else if (tx->type() == TX_BUZZER_UNSUBSCRIBE) {
		// unsubscribe - just stub
	} else if (tx->type() == TX_BUZZ_LIKE) {
		// extract buzz_id
		TxBuzzLikePtr lBuzzLike = TransactionHelper::to<TxBuzzLike>(tx);
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removing buzz like ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		if (lBuzzLike->in().size() >= 1) {
			// extract buzz | buzzer
			uint256 lBuzzId = lBuzzLike->in()[1].out().tx(); // in[0] - buzz_id
			uint256 lBuzzerId = lBuzzLike->in()[0].out().tx(); // in[1] - initiator
			// remove index
			likesIdx_.remove(lBuzzId, lBuzzerId);

			// update like count
			decrementLikes(lBuzzId);

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzz like removed ") +
				strprintf("buzz = %s, liker = %s", lBuzzId.toHex(), lBuzzerId.toHex()));
		}
	}
}

TransactionPtr BuzzerTransactionStoreExtension::locateSubscription(const uint256& subscriber, const uint256& publisher) {
	//
	db::DbTwoKeyContainer<uint256 /*subscriber*/, uint256 /*publisher*/, uint256 /*tx*/>::Iterator lItem = subscriptionsIdx_.find(subscriber, publisher);
	if (lItem.valid()) {
		//
		TransactionPtr lTx = store_->locateTransaction(*lItem);
		return lTx;
	}

	return nullptr;
}

void BuzzerTransactionStoreExtension::incrementLikes(const uint256& buzz) {
	//
	BuzzInfo lInfo;
	buzzInfo_.read(buzz, lInfo);
	lInfo.likes_++;
	buzzInfo_.write(buzz, lInfo);
}

void BuzzerTransactionStoreExtension::decrementLikes(const uint256& buzz) {
	//
	BuzzInfo lInfo;
	if (buzzInfo_.read(buzz, lInfo)) {
		if ((int)lInfo.likes_ - 1 < 0) lInfo.likes_ = 0;
		else lInfo.likes_--;

		buzzInfo_.write(buzz, lInfo);
	}
}

void BuzzerTransactionStoreExtension::incrementReplies(const uint256& buzz) {
	//
	BuzzInfo lInfo;
	buzzInfo_.read(buzz, lInfo);
	lInfo.replies_++;
	buzzInfo_.write(buzz, lInfo);
}

void BuzzerTransactionStoreExtension::decrementReplies(const uint256& buzz) {
	//
	BuzzInfo lInfo;
	if (buzzInfo_.read(buzz, lInfo)) {
		if ((int)lInfo.replies_ - 1 < 0) lInfo.replies_ = 0;
		else lInfo.replies_--;

		buzzInfo_.write(buzz, lInfo);
	}
}

void BuzzerTransactionStoreExtension::incrementRebuzzes(const uint256& buzz) {
	//
	BuzzInfo lInfo;
	buzzInfo_.read(buzz, lInfo);
	lInfo.rebuzzes_++;
	buzzInfo_.write(buzz, lInfo);
}

void BuzzerTransactionStoreExtension::decrementRebuzzes(const uint256& buzz) {
	//
	BuzzInfo lInfo;
	if (buzzInfo_.read(buzz, lInfo)) {
		if ((int)lInfo.rebuzzes_ - 1 < 0) lInfo.rebuzzes_ = 0;
		else lInfo.rebuzzes_--;

		buzzInfo_.write(buzz, lInfo);
	}
}

bool BuzzerTransactionStoreExtension::checkSubscription(const uint256& subscriber, const uint256& publisher) {
	//
	db::DbTwoKeyContainer<uint256 /*subscriber*/, uint256 /*publisher*/, uint256 /*tx*/>::Iterator lItem = subscriptionsIdx_.find(subscriber, publisher);
	return lItem.valid();
}

bool BuzzerTransactionStoreExtension::checkLike(const uint256& buzz, const uint256& liker) {
	//
	db::DbTwoKeyContainer<uint256 /*nuzz*/, uint256 /*liker*/, uint256 /*like_tx*/>::Iterator lItem = likesIdx_.find(buzz, liker);
	return lItem.valid();
}

bool BuzzerTransactionStoreExtension::readBuzzInfo(const uint256& buzz, BuzzInfo& info) {
	//
	return buzzInfo_.read(buzz, info);
}

void BuzzerTransactionStoreExtension::selectBuzzfeed(uint64_t from, const uint256& subscriber, std::vector<BuzzfeedItem>& feed) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: selecting buzzfeed for ") +
		strprintf("subscriber = %s, from = %d, chain = %s#", subscriber.toHex(), from, store_->chain().toHex().substr(0, 10)));

	// 0. get/set current time
	uint64_t lTimeFrom = from;
	if (lTimeFrom == 0) lTimeFrom = qbit::getMicroseconds();

	// 1.collect publishers
	uint64_t lPublisherTime;
	std::multimap<uint64_t, uint256> lPublishers;
	if (publisherUpdates_.read(subscriber, lPublisherTime)) {
		// insert own
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: subscriber time is ") +
			strprintf("%s/%d", subscriber.toHex(), lPublisherTime));		
		lPublishers.insert(std::multimap<uint64_t, uint256>::value_type(lPublisherTime, subscriber));
	}

	// 1.1. get publishers by subscriber
	db::DbTwoKeyContainer<uint256 /*subscriber*/, uint256 /*publisher*/, uint256 /*tx*/>::Iterator lSubscription = subscriptionsIdx_.find(subscriber);
	if (!lSubscription.valid()) {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: publishers NOT FOUND for ") +
			strprintf("subscriber = %s, chain = %s#", subscriber.toHex(), store_->chain().toHex().substr(0, 10)));		
	}

	// 1.2. iterate
	for (; lSubscription.valid(); ++lSubscription) {
		uint256 lSubscriber;
		uint256 lPublisher;
		if (lSubscription.first(lSubscriber, lPublisher)) {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: found publisher for ") +
				strprintf("subscriber = %s, p = %s, chain = %s#", subscriber.toHex(), lPublisher.toHex(), store_->chain().toHex().substr(0, 10)));
			
			// 1.2.1. get last publisher updates
			if (publisherUpdates_.read(lPublisher, lPublisherTime)) {
				// 1.2.2. inserting most recent action made by publisher
				lPublishers.insert(std::multimap<uint64_t, uint256>::value_type(lPublisherTime, lPublisher));
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: publisher time is ") +
					strprintf("%s/%d", lPublisher.toHex(), lPublisherTime));

				// up to 300 most recent
				if (lPublishers.size() > 300 /*TODO: settings?*/) {
					lPublishers.erase(lPublishers.begin());
				}
			} else {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: publisher feed NOT FOUND for ") +
					strprintf("subscriber = %s, p = %s, chain = %s#", subscriber.toHex(), lPublisher.toHex(), store_->chain().toHex().substr(0, 10)));				
			}
		}
	}

	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: publishers count for ") +
		strprintf("subscriber = %s, count = %d, chain = %s#", subscriber.toHex(), lPublishers.size(), store_->chain().toHex().substr(0, 10)));

	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());
	// 2. select timeline items by prepared publishers
	std::multimap<uint64_t, BuzzfeedItem> lRawBuzzfeed;
	std::set<uint256> lBuzzItems;
	for (std::multimap<uint64_t, uint256>::reverse_iterator lPublisher = lPublishers.rbegin(); lPublisher != lPublishers.rend(); lPublisher++) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: processing publisher ") +
			strprintf("%s/%d", lPublisher->second.toHex(), lPublisher->first));
		// 2.1. locating most recent action made by publisher
		db::DbTwoKeyContainer<uint256 /*publisher*/, uint64_t /*timestamp*/, uint256 /*tx*/>::Iterator lFrom = 
				timeline_.find(lPublisher->second, lTimeFrom < lPublisher->first ? lTimeFrom : lPublisher->first);
		// 2.1. resetting timestamp to ensure appropriate backtracing
		lFrom.setKey2Empty();
		// 2.3. stepping down up to max 10 events 
		for (int lCount = 0; lCount < 10 /*TODO: settings?*/ && lFrom.valid(); --lFrom, ++lCount) {
			//
			int lContext = 0;
			TxBuzzerPtr lBuzzer = nullptr;
			TransactionPtr lTx = store_->locateTransaction(*lFrom);
			//
			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lPublisher->second);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			//
			makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems);
		}
	}

	// 3.collect events by subscriber
	uint64_t lSubscriberTime = lTimeFrom;
	subscriberUpdates_.read(subscriber, lSubscriberTime);

	// 3.0. prepare clean-up transaction
	db::DbThreeKeyContainer<uint256, uint64_t, uint256, unsigned short>::Transaction lEventsTransaction = events_.transaction();

	// 3.1. locating most recent event made by publisher
	db::DbThreeKeyContainer<uint256 /*subscriber*/, uint64_t /*timestamp*/, uint256 /*tx*/, unsigned short /*type*/>::Iterator lFromEvent = 
			events_.find(subscriber, lTimeFrom < lSubscriberTime ? lTimeFrom : lSubscriberTime);
	// 3.2. resetting timestamp to ensure appropriate backtracing
	lFromEvent.setKey2Empty();
	// 3.3. stepping down up to max 10 events 
	for (int lCount = 0; lCount < 10 /*TODO: settings?*/ && lFromEvent.valid(); --lFromEvent, ++lCount) {
		//
		int lContext = 0;
		TxBuzzerPtr lBuzzer = nullptr;

		if (*lFromEvent == TX_BUZZ_REPLY || *lFromEvent == TX_REBUZZ) {
			//
			uint256 lSubscriber;
			uint64_t lTimestamp;
			uint256 lEvent; 

			if (lFromEvent.first(lSubscriber, lTimestamp, lEvent)) {
				//
				TransactionPtr lTx = store_->locateTransaction(lEvent);
				if (!lTx) {
					lEventsTransaction.remove(lFromEvent);
					//
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: event NOT FOUND ") +
						strprintf("%s/%s#", lEvent.toHex(), store_->chain().toHex().substr(0, 10)));
					continue;
				}

				uint256 lPublisher = lTx->in()[TX_BUZZ_REPLY_MY_IN].out().tx();
				//
				TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lPublisher);
				if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
				//
				makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems);
			}
		}
	}

	lEventsTransaction.commit(); // clean-up

	// 4. fill reply
	for (std::multimap<uint64_t, BuzzfeedItem>::reverse_iterator lItem = lRawBuzzfeed.rbegin(); lItem != lRawBuzzfeed.rend(); lItem++) {
		//
		if (feed.size() < 30 /*max last events*/) {
			feed.push_back(lItem->second);
		} else {
			break;
		}
	}
}

bool BuzzerTransactionStoreExtension::makeBuzzfeedItem(int& context, TxBuzzerPtr buzzer, TransactionPtr tx, ITransactionStorePtr mainStore, std::multimap<uint64_t, BuzzfeedItem>& rawBuzzfeed, std::set<uint256>& buzzes) {
	//
	if (tx->type() == TX_BUZZ ||
		tx->type() == TX_REBUZZ ||
		tx->type() == TX_BUZZ_REPLY) {
		//
		TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(tx);
		if (buzzes.find(lBuzz->id()) != buzzes.end()) return false;
		if (context > 100) return false;

		//
		BuzzfeedItem lItem;
		prepareBuzzfeedItem(lItem, lBuzz, buzzer);

		if (tx->type() == TX_BUZZ_REPLY) {
			// load original tx
			// NOTE: should be in the same chain
			uint256 lOriginalId = lBuzz->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
			TransactionPtr lOriginalTx = store_->locateTransaction(lOriginalId);
			lItem.setOriginalBuzzId(lOriginalId);

			TxBuzzerPtr lBuzzer = nullptr;
			uint256 lBuzzerId = lBuzz->in()[TX_BUZZ_REPLY_MY_IN].out().tx();
			TransactionPtr lBuzzerTx = mainStore->locateTransaction(lBuzzerId);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);

			// add buzz
			makeBuzzfeedItem(++context, lBuzzer, lOriginalTx, mainStore, rawBuzzfeed, buzzes);
		}

		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: item added ") +
			strprintf("%d/%s/%s#", lBuzz->timestamp(), lBuzz->id().toHex(), store_->chain().toHex().substr(0, 10)));

		rawBuzzfeed.insert(std::multimap<uint64_t, BuzzfeedItem>::value_type(lItem.timestamp(), lItem));
		buzzes.insert(lBuzz->id());
		return true;
	}

	return false;
}

void BuzzerTransactionStoreExtension::prepareBuzzfeedItem(BuzzfeedItem& item, TxBuzzPtr buzz, TxBuzzerPtr buzzer) {
	//
	item.setType(buzz->type());
	item.setBuzzId(buzz->id());
	item.setBuzzChainId(buzz->chain());
	item.setTimestamp(buzz->timestamp());
	item.setBuzzBody(buzz->body());

	if (buzzer) {
		item.setBuzzerId(buzzer->id());
		item.setBuzzerName(buzzer->myName());
		item.setBuzzerAlias(buzzer->alias());
	}

	// TODO:
	// TxBuzzerInfo loaded by buzzer_id or using "in"?
	// and fill setBuzzerInfoId()

	// read info
	BuzzInfo lInfo;
	if (buzzInfo_.read(buzz->id(), lInfo)) {
		item.setReplies(lInfo.replies_);
		item.setRebuzzes(lInfo.rebuzzes_);
		item.setLikes(lInfo.likes_);
	}
}