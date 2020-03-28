#include "transactionstoreextension.h"
#include "../../log/log.h"
#include "../../mkpath.h"
#include "txbuzzer.h"
#include "txbuzz.h"
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
			buzzInfo_.open();
			subscriptionsIdx_.open();

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
	subscriptionsIdx_.close();

	opened_ = false;

	return true;
}

bool BuzzerTransactionStoreExtension::pushEntity(const uint256& id, TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZ) {
		TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(ctx->tx());
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing buzz ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		timeline_.write(lBuzz->timestamp(), lBuzz->id());
	} else if (ctx->tx()->type() == TX_BUZZER_SUBSCRIBE) {
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
	}

	return true;
}

void BuzzerTransactionStoreExtension::removeTransaction(TransactionPtr tx) {
	//
	if (tx->type() == TX_BUZZ) {
		//
		TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(tx);
		for (db::DbMultiContainer<uint64_t /*timestamp*/, uint256 /*buzz*/>::Iterator 
									lEntry = timeline_.find(lBuzz->timestamp()); lEntry.valid(); ++lEntry) {
			if (*lEntry == tx->id()) {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/removeTransaction]: removing buzz from timeline ") +
					strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
				timeline_.remove(lEntry);
				break;
			}
		}
	} else if (tx->type() == TX_BUZZER_SUBSCRIBE) {
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

bool BuzzerTransactionStoreExtension::checkSubscription(const uint256& subscriber, const uint256& publisher) {
	//
	db::DbTwoKeyContainer<uint256 /*subscriber*/, uint256 /*publisher*/, uint256 /*tx*/>::Iterator lItem = subscriptionsIdx_.find(subscriber, publisher);
	return lItem.valid();
}

bool BuzzerTransactionStoreExtension::readBuzzInfo(const uint256& buzz, BuzzInfo& info) {
	//
	return buzzInfo_.read(buzz, info);
}

void BuzzerTransactionStoreExtension::selectBuzzfeed(uint64_t from, const uint256& subscriber, std::vector<BuzzfeedItem>& feed) {
	//
	// positioning
	db::DbMultiContainer<uint64_t /*timestamp*/, uint256 /*buzz*/>::Iterator lFrom;
	if (from) lFrom = timeline_.find(from);
	else lFrom = timeline_.last();

	// main store
	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());

	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: selecting buzzfeed for ") +
		strprintf("subscriber = %s, from = %d, chain = %s#", subscriber.toHex(), from, store_->chain().toHex().substr(0, 10)));

	// traverse
	for (int lCount = 0; lCount < 20 /*settings_->property("buzzer", "buzzFeedLength")*/ && lFrom.valid(); --lFrom, lCount++) {
		//
		TransactionPtr lTx = store_->locateTransaction(*lFrom);
		if (lTx) {
			TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lTx);
			//
			if (lBuzz->in().size()) {
				bool lAdd = false;
				uint256 lRealPublisher = (*lBuzz->in().begin()).out().tx();
				db::DbTwoKeyContainer<uint256 /*subscriber*/, uint256 /*publisher*/, uint256 /*tx*/>::Iterator lSubscription = subscriptionsIdx_.find(subscriber, lRealPublisher);
				if (lSubscription.valid()) {

					uint256 lSubscriber;
					uint256 lPublisher;
					if (lSubscription.first(lSubscriber, lPublisher) && lSubscriber == subscriber && lPublisher == lRealPublisher) {
						lAdd = true;
					} else {
						if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: subscription NOT VALID for ") +
							strprintf("s = %s, p = %s, c = %s#", lSubscriber.toHex(), lPublisher.toHex(), store_->chain().toHex().substr(0, 10)));					
					}
				} else {
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: subscription NOT found for ") +
						strprintf("s = %s, p = %s, c = %s#", subscriber.toHex(), lRealPublisher.toHex(), store_->chain().toHex().substr(0, 10)));					
				}

				if (subscriber == lRealPublisher) lAdd = true;

				if (lAdd) {
					//
					BuzzfeedItem lItem;
					lItem.setBuzzId(*lFrom);
					lItem.setBuzzChainId(lBuzz->chain());
					lItem.setTimestamp(lBuzz->timestamp());
					lItem.setBuzzBody(lBuzz->body());

					TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lRealPublisher);
					if (lBuzzerTx) {
						TxBuzzerPtr lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
						lItem.setBuzzerId(lRealPublisher);
						lItem.setBuzzerName(lBuzzer->myName());
						lItem.setBuzzerAlias(lBuzzer->alias());
					}


					// TODO:
					// TxBuzzerInfo loaded by buzzer_id or using "in"?
					// and fill setBuzzerInfoId()

					// read info
					BuzzInfo lInfo;
					if (buzzInfo_.read(*lFrom, lInfo)) {
						lItem.setReplies(lInfo.replies_);
						lItem.setRebuzzes(lInfo.rebuzzes_);
						lItem.setLikes(lInfo.likes_);
					}

					feed.push_back(lItem);
				}
			}
		}
	}	
}
