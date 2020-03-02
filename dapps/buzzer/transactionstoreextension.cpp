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