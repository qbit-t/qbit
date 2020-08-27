#include "transactionstoreextension.h"
#include "../../log/log.h"
#include "../../mkpath.h"
#include "txbuzzer.h"
#include "txbuzz.h"
#include "txrebuzz.h"
#include "txrebuzznotify.h"
#include "txbuzzreply.h"
#include "txbuzzlike.h"
#include "txbuzzreward.h"
#include "txbuzzersubscribe.h"
#include "txbuzzerunsubscribe.h"
#include "txbuzzerendorse.h"
#include "txbuzzermistrust.h"

#include <iostream>

using namespace qbit;

bool BuzzerTransactionStoreExtension::open() {
	//
	if (!opened_) { 
		try {
			gLog().write(Log::INFO, std::string("[extension/open]: opening storage extension for ") + strprintf("%s", store_->chain().toHex()));

			if (mkpath(std::string(settings_->dataPath() + "/" + store_->chain().toHex() + "/buzzer/indexes").c_str(), 0777)) return false;

			timeline_.open();
			globalTimeline_.open();
			hashTagTimeline_.open();
			hashTags_.open();
			hashTagUpdates_.open();
			events_.open();
			subscriptionsIdx_.open();
			publishersIdx_.open();
			likesIdx_.open();
			rebuzzesIdx_.open();
			replies_.open();
			//rebuzzes_.open();
			publisherUpdates_.open();
			subscriberUpdates_.open();
			buzzInfo_.open();
			buzzerStat_.open();
			endorsements_.open();
			mistrusts_.open();

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
	globalTimeline_.close();
	hashTagTimeline_.close();
	hashTags_.close();
	hashTagUpdates_.close();
	events_.close();
	subscriptionsIdx_.close();
	publishersIdx_.close();
	likesIdx_.close();
	rebuzzesIdx_.open();
	replies_.close();
	//rebuzzes_.close();
	publisherUpdates_.close();
	subscriberUpdates_.close();
	buzzInfo_.close();
	buzzerStat_.close();
	endorsements_.close();
	mistrusts_.close();

	opened_ = false;

	return true;
}

bool BuzzerTransactionStoreExtension::selectBuzzerEndorseTx(const uint256& actor, const uint256& buzzer, uint256& tx) {
	//
	db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/>::Iterator 
													lEndorsement = endorsements_.find(buzzer, actor);
	if (lEndorsement.valid()) {
		//
		tx = *lEndorsement;
		return true;
	}

	return false;
}

bool BuzzerTransactionStoreExtension::selectBuzzerMistrustTx(const uint256& actor, const uint256& buzzer, uint256& tx) {
	//
	db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/>::Iterator 
													lMitrust = mistrusts_.find(buzzer, actor);
	if (lMitrust.valid()) {
		//
		tx = *lMitrust;
		return true;
	}

	return false;
}

bool BuzzerTransactionStoreExtension::isAllowed(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZ ||
		ctx->tx()->type() == TX_REBUZZ ||
		ctx->tx()->type() == TX_BUZZ_LIKE || 
		ctx->tx()->type() == TX_BUZZ_REPLY ||
		ctx->tx()->type() == TX_BUZZER_ENDORSE ||
		ctx->tx()->type() == TX_BUZZER_MISTRUST ||
		ctx->tx()->type() == TX_BUZZ_REBUZZ_NOTIFY) {

		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());

		// in[0] - publisher/initiator
		uint256 lPublisher = lEvent->in()[0].out().tx();
		// get buzzer
		TransactionPtr lPublisherTx = store_->storeManager()->locate(MainChain::id())->locateTransaction(lPublisher);
		if (!lPublisherTx) return false; // we do not accept txes from unknown publishers

		// get target store
		ITransactionStorePtr lStore = store_->storeManager()->locate(lPublisherTx->in()[TX_BUZZER_SHARD_IN].out().tx());
		if (!lStore) return false; // all buzzer shards need to be present in this node (node that support dapp)

		if (!lStore->extension()) return false; // store should have an extansion
		BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

		// check trust score
		BuzzerInfo lScore;
		lExtension->readBuzzerStat(lPublisher, lScore);
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/is_allowed]: publisher score ") +
			strprintf("%s/%d/%d, trusted = %s", lPublisher.toHex(), lScore.endorsements(), lScore.mistrusts(), (lScore.trusted() ? "true":"false")));

		// TODO: check connected address (pkey) also and if single pkey used to generate another buzzer, but previous 
		// buzzer is untrusted - untrust new buzzer also
		if (!lScore.trusted()) {
			//
			return false; // not allowed to make any actions
		}

		if (lScore.score() < lEvent->score()) {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/is_allowed]: publisher score is INCONSISTENT ") +
				strprintf("%s/%d/%d", lPublisher.toHex(), lScore.score(), lEvent->score()));
			return false; // event score is inconsistent
		}

		if (ctx->tx()->type() == TX_BUZZ || ctx->tx()->type() == TX_REBUZZ) {
			//
			if (store_->chain() != lPublisherTx->in()[TX_BUZZER_SHARD_IN].out().tx()) {
				return false;
			}
		} else if (ctx->tx()->type() == TX_BUZZER_ENDORSE) {
			//
			// in[0] - publisher/initiator
			uint256 lBuzzer = lEvent->in()[TX_BUZZER_ENDORSE_MY_IN].out().tx(); 
			// in[1] - publisher/initiator
			uint256 lEndoser = lEvent->in()[TX_BUZZER_ENDORSE_BUZZER_IN].out().tx(); 
			//
			db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/>::Iterator 
															lEndorsement = endorsements_.find(lBuzzer, lEndoser);
			if (lEndorsement.valid()) {
				//
				return false; // already endorsed
			}
		} else if (ctx->tx()->type() == TX_BUZZER_MISTRUST) {
			//
			// in[0] - publisher/initiator
			uint256 lBuzzer = lEvent->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx(); 
			// in[1] - publisher/initiator
			uint256 lMistruster = lEvent->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 
			//
			db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/>::Iterator 
															lMitrust = mistrusts_.find(lBuzzer, lMistruster);
			if (lMitrust.valid()) {
				//
				return false; // already endorsed
			}
		} else if (ctx->tx()->type() == TX_BUZZ_LIKE) {

			TxBuzzLikePtr lBuzzLike = TransactionHelper::to<TxBuzzLike>(ctx->tx());
			//
			if (lBuzzLike->in().size() >= 1) {
				// extract buzz | buzzer
				uint256 lBuzzChainId = lBuzzLike->in()[TX_BUZZ_LIKE_IN].out().chain(); // buzz chain_id
				uint256 lBuzzId = lBuzzLike->in()[TX_BUZZ_LIKE_IN].out().tx(); // buzz_id
				uint256 lBuzzerId = lBuzzLike->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
				// check index
				if (!checkLike(lBuzzId, lBuzzerId)) {
					//
					TransactionPtr lBuzz = store_->locateTransaction(lBuzzId);
					if (lBuzz) {
						//
						uint256 lOriginalBuzzerId = lBuzz->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
						if (lOriginalBuzzerId == lBuzzerId)
							return false;
					}
				} else {
					return false; // already liked		
				}
			}
		} else if (ctx->tx()->type() == TX_BUZZ_REWARD) {

			TxBuzzRewardPtr lBuzzReward = TransactionHelper::to<TxBuzzReward>(ctx->tx());
			//
			if (lBuzzReward->in().size() >= 1) {
				// extract buzz | buzzer
				uint256 lBuzzChainId = lBuzzReward->in()[TX_BUZZ_REWARD_IN].out().chain(); // buzz chain_id
				uint256 lBuzzId = lBuzzReward->in()[TX_BUZZ_REWARD_IN].out().tx(); // buzz_id
				uint256 lBuzzerId = lBuzzReward->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
				//
				TransactionPtr lBuzz = store_->locateTransaction(lBuzzId);
				if (lBuzz) {
					//
					uint256 lOriginalBuzzerId = lBuzz->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
					if (lOriginalBuzzerId == lBuzzerId)
						return false;
				}
			}
		}
	}

	return true;
}

bool BuzzerTransactionStoreExtension::pushEntity(const uint256& id, TransactionContextPtr ctx) {
	//
	processEvent(id, ctx);
	
	if (ctx->tx()->type() == TX_REBUZZ) {
		processRebuzz(id, ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_SUBSCRIBE) {
		processSubscribe(id, ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_UNSUBSCRIBE) {
		processUnsubscribe(id, ctx);
	} else if (ctx->tx()->type() == TX_BUZZER) {
		// do nothing
	} else if (ctx->tx()->type() == TX_BUZZ_LIKE) {
		processLike(id, ctx);
	} else if (ctx->tx()->type() == TX_BUZZ_REWARD) {
		processReward(id, ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_ENDORSE) {
		processEndorse(id, ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_MISTRUST) {
		processMistrust(id, ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_INFO) {
		//
		updateBuzzerInfo(
			ctx->tx()->in()[TX_BUZZER_INFO_MY_IN].out().tx(),
			ctx->tx()->id()
		);
	}

	return true;
}

void BuzzerTransactionStoreExtension::processEndorse(const uint256& id, TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_ENDORSE) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing endorsement ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());

		// in[0] - publisher/initiator
		uint256 lEndoser = lEvent->in()[TX_BUZZER_ENDORSE_MY_IN].out().tx(); 
		// in[1] - publisher/initiator
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_ENDORSE_BUZZER_IN].out().tx(); 
		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/>::Iterator 
														lEndorsement = endorsements_.find(lBuzzer, lEndoser);
		if (!lEndorsement.valid()) {
			//
			endorsements_.write(lBuzzer, lEndoser, lEvent->id());
			// write timeline
			timeline_.write(lEndoser, lEvent->timestamp(), lEvent->id());
			// write global timeline
			globalTimeline_.write(lEvent->timestamp()/BUZZFEED_TIMEFRAME, lEvent->score(), 
															lEvent->timestamp(), lEndoser, lEvent->id());
			// mark last updates
			publisherUpdates(lEndoser, lEvent->timestamp());
			subscriberUpdates(lBuzzer, lEvent->timestamp());

			// events
			events_.write(lBuzzer, lEvent->timestamp(), lEvent->id(), lEvent->type());

			ITransactionStorePtr lStore = store_->storeManager()->locate(MainChain::id());
			if (lStore && lEvent->in().size() > TX_BUZZER_ENDORSE_FEE_IN) {
				TransactionPtr lFeeTx = lStore->locateTransaction(lEvent->in()[TX_BUZZER_ENDORSE_FEE_IN].out().tx());
				if (lFeeTx) {
					TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

					amount_t lAmount;
					uint64_t lHeight;
					if (TxBuzzerEndorse::extractLockedAmountAndHeight(lFee, lAmount, lHeight)) {
						//
						incrementEndorsements(lBuzzer, lAmount);
					}
				} else {
					//
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/warning]: fee tx for endorsement was NOT FOUND ") +
						strprintf("%s/%s#", lEvent->in()[TX_BUZZER_MISTRUST_FEE_IN].out().tx().toHex(), MainChain::id().toHex().substr(0, 10)));
					//
					TxBuzzerEndorsePtr lEndorse = TransactionHelper::to<TxBuzzerEndorse>(lEvent);
					incrementEndorsements(lBuzzer, lEndorse->amount());
				}
			}
		}
	}
}

void BuzzerTransactionStoreExtension::processMistrust(const uint256& id, TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_MISTRUST) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing mistrustment ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());

		// in[0] - publisher/initiator
		uint256 lMistruster = lEvent->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx(); 
		// in[1] - publisher/initiator
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 
		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/>::Iterator 
														lMistrust = mistrusts_.find(lBuzzer, lMistruster);
		if (!lMistrust.valid()) {
			//
			mistrusts_.write(lBuzzer, lMistruster, lEvent->id());
			// write timeline
			timeline_.write(lMistruster, lEvent->timestamp(), lEvent->id());
			// write global timeline
			globalTimeline_.write(lEvent->timestamp()/BUZZFEED_TIMEFRAME, lEvent->score(), 
												lEvent->timestamp(), lMistruster, lEvent->id());
			// mark last updates
			publisherUpdates(lMistruster, lEvent->timestamp());
			subscriberUpdates(lBuzzer, lEvent->timestamp());

			// events
			events_.write(lBuzzer, lEvent->timestamp(), lEvent->id(), lEvent->type());

			ITransactionStorePtr lStore = store_->storeManager()->locate(MainChain::id());
			if (lStore && lEvent->in().size() > TX_BUZZER_MISTRUST_FEE_IN) {
				TransactionPtr lFeeTx = lStore->locateTransaction(lEvent->in()[TX_BUZZER_MISTRUST_FEE_IN].out().tx());
				if (lFeeTx) {
					TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

					amount_t lAmount;
					uint64_t lHeight;
					if (TxBuzzerMistrust::extractLockedAmountAndHeight(lFee, lAmount, lHeight)) {
						//
						incrementMistrusts(lBuzzer, lAmount);
					}
				} else {
					//
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/warning]: fee tx for mistrustment was NOT FOUND ") +
						strprintf("%s/%s#", lEvent->in()[TX_BUZZER_MISTRUST_FEE_IN].out().tx().toHex(), MainChain::id().toHex().substr(0, 10)));
					//
					TxBuzzerMistrustPtr lMistrust = TransactionHelper::to<TxBuzzerMistrust>(lEvent);
					incrementMistrusts(lBuzzer, lMistrust->amount());
				}
			}
		}
	}
}

void BuzzerTransactionStoreExtension::hashTagUpdate(const uint160& hash, const db::FiveKey<uint160, uint64_t, uint64_t, uint64_t, uint256>& key) {
	//
	db::FiveKey<uint160, uint64_t, uint64_t, uint64_t, uint256> lKey;
	if (hashTagUpdates_.read(hash, lKey)) {
		if (lKey.key4() < key.key4()) {
			hashTagUpdates_.write(hash, key);
		}
	} else {
		hashTagUpdates_.write(hash, key);
	}
}

void BuzzerTransactionStoreExtension::processEvent(const uint256& id, TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZ ||
		ctx->tx()->type() == TX_REBUZZ ||
		ctx->tx()->type() == TX_BUZZ_LIKE || 
		ctx->tx()->type() == TX_BUZZ_REWARD || 
		ctx->tx()->type() == TX_BUZZ_REPLY ||
		ctx->tx()->type() == TX_BUZZ_REBUZZ_NOTIFY) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing event ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));

		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		// in[0] - publisher/initiator
		uint256 lPublisher = lEvent->in()[0].out().tx(); 
		// write timeline
		timeline_.write(lPublisher, lEvent->timestamp(), lEvent->id());
		// write global timeline
		globalTimeline_.write(lEvent->timestamp()/BUZZFEED_TIMEFRAME, lEvent->score(), 
										lEvent->timestamp(), lPublisher, lEvent->id());		
		// mark last updates
		publisherUpdates(lPublisher, lEvent->timestamp());

		// hash tags
		if (ctx->tx()->type() == TX_BUZZ ||
			ctx->tx()->type() == TX_REBUZZ ||
			ctx->tx()->type() == TX_BUZZ_REPLY) {
			//
			std::map<uint160, std::string> lTags;
			TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(ctx->tx());
			lBuzz->extractTags(lTags);

			// NOTICE: clean-up in selects
			for (std::map<uint160, std::string>::iterator lTag = lTags.begin(); lTag != lTags.end(); lTag++) {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: write hash tag ") +
					strprintf("%s/%s/%s", lTag->first.toHex(), boost::algorithm::to_lower_copy(lTag->second), lTag->second));

				hashTagTimeline_.write(
					lTag->first,
					lEvent->timestamp()/BUZZFEED_TIMEFRAME,
					lEvent->score(),
					lEvent->timestamp(), 
					lPublisher, 
					lEvent->id()
				);

				hashTagUpdate(
					lTag->first,
					db::FiveKey<
						uint160 /*hash*/, 
						uint64_t /*timeframe*/, 
						uint64_t /*score*/, 
						uint64_t /*timestamp*/,
						uint256 /*publisher*/> (
						lTag->first,
						lEvent->timestamp()/BUZZFEED_TIMEFRAME,
						lEvent->score(),
						lEvent->timestamp(), 
						lPublisher
					)
				);

				// #lower | #Original
				hashTags_.write(boost::algorithm::to_lower_copy(lTag->second), lTag->second);
			}
		}

		// check, update
		if (ctx->tx()->type() == TX_BUZZ_REBUZZ_NOTIFY) {
			//
			TxReBuzzNotifyPtr lRebuzzNotify = TransactionHelper::to<TxReBuzzNotify>(ctx->tx());
			// inc re-buzzes
			incrementRebuzzes(lRebuzzNotify->buzzId());
			// re-buzz index
			db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*rebuzzer*/, uint256 /*rebuzz_tx*/>::Iterator 
				lRebuzzIdx = rebuzzesIdx_.find(lRebuzzNotify->buzzId(), lPublisher);

			// WARNING: breaks current logic
			/*
			if (!lRebuzzIdx.valid() && lPublisher != lRebuzzNotify->buzzerId()) 
				incrementEndorsements(lRebuzzNotify->buzzerId(), BUZZER_ENDORSE_REBUZZ);
			*/
			rebuzzesIdx_.write(lRebuzzNotify->buzzId(), lPublisher, lEvent->id());
			// re-buzz extended info
			// rebuzzes_.write(lRebuzzNotify->buzzChainId(), lRebuzzNotify->buzzId(), lRebuzzNotify->rebuzzId(), lPublisher); // increment buzzes
			// re-buzz as event
			events_.write(lRebuzzNotify->buzzerId(), lRebuzzNotify->timestamp(),  lRebuzzNotify->rebuzzId(), TX_REBUZZ); // put event for buzzer
			
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: incrementing rebuzzes for ") +
				strprintf("%s/%s#", lRebuzzNotify->buzzId().toHex(), store_->chain().toHex().substr(0, 10)));

			return;
		}

		// direct rebuzz
		if (ctx->tx()->type() == TX_REBUZZ) { 
			//
			TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(ctx->tx());
			// inc re-buzzes
			incrementRebuzzes(lRebuzz->buzzId());
			// re-buzz index
			db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*rebuzzer*/, uint256 /*rebuzz_tx*/>::Iterator 
				lRebuzzIdx = rebuzzesIdx_.find(lRebuzz->buzzId(), lPublisher);

			// WARNING: breaks current logic
			/*
			if (!lRebuzzIdx.valid() && lPublisher != lRebuzz->buzzerId()) 
				incrementEndorsements(lRebuzz->buzzerId(), BUZZER_ENDORSE_REBUZZ);
			*/
			rebuzzesIdx_.write(lRebuzz->buzzId(), lPublisher, lEvent->id());
			// re-buzz extended info
			// rebuzzes_.write(lRebuzz->buzzChainId(), lRebuzz->buzzId(), lEvent->id(), lPublisher); // increment buzzes
			// event
			events_.write(lRebuzz->buzzerId(), lEvent->timestamp(), lEvent->id(), lEvent->type()); // put event for buzzer
		}

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
				} else if ((lInTx->type() == TX_BUZZ || lInTx->type() == TX_REBUZZ || lInTx->type() == TX_BUZZ_REPLY) &&
																				lInTx->in().size() > TX_BUZZ_REPLY_MY_IN) {
					// re-buzz, reply
					// WARNING: reply and rebuzz are the similar by in and outs, that is TX_BUZZ_REPLY_MY_IN applicable for both
					events_.write(lInTx->in()[TX_BUZZ_REPLY_MY_IN].out().tx(), lEvent->timestamp(), lEvent->id(), lEvent->type());
					subscriberUpdates(lInTx->in()[TX_BUZZ_REPLY_MY_IN].out().tx(), lEvent->timestamp());
					// 
					if (ctx->tx()->type() == TX_BUZZ_REPLY) {
						// inc replies
						incrementReplies(lInTx->id());
						// make tree
						replies_.write(lInTx->id(), lEvent->timestamp(), lEvent->id(), lPublisher);
						//  process sub-tree
						if (lInTx->type() != TX_REBUZZ && lInTx->in().size() > TX_BUZZ_REPLY_BUZZ_IN) {
							uint256 lReTxId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
							uint256 lReChainId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
							while (true) {
								//
								lLocalStore = store_->storeManager()->locate(lReChainId);
								TransactionPtr lReTx = store_->locateTransaction(lReTxId);
								if (lReTx && (lReTx->type() == TX_BUZZ_REPLY || lReTx->type() == TX_REBUZZ ||
																					lReTx->type() == TX_BUZZ)) {
									// inc replies
									incrementReplies(lReTxId);
									// make tree
									replies_.write(lReTxId, lEvent->timestamp(), lEvent->id(), lPublisher);

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
	}
}

void BuzzerTransactionStoreExtension::processRebuzz(const uint256& id, TransactionContextPtr ctx) {
	//
	TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(ctx->tx());
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/processRebuzz]: pushing rebuzz ") +
		strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));

	// WARNING: this buzz can not be under current node
	uint256 lBuzzId = lRebuzz->buzzId();
	uint256 lBuzzChainId = lRebuzz->buzzChainId();

	ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzChainId);

	if (lStore) {
		//
		TransactionPtr lTx = lStore->locateTransaction(lBuzzId);
		if (lTx && lTx->in().size() > TX_REBUZZ_MY_IN) {
			events_.write(lTx->in()[TX_REBUZZ_MY_IN].out().tx(), lRebuzz->timestamp(), lRebuzz->id(), lRebuzz->type());
			subscriberUpdates(lTx->in()[TX_REBUZZ_MY_IN].out().tx(), lRebuzz->timestamp());
		}
	} else {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/processRebuzz/error]: source buzz storage not found."));
	}
}

void BuzzerTransactionStoreExtension::processSubscribe(const uint256& id, TransactionContextPtr ctx) {
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
		publishersIdx_.write(lPublisher, lSubscriber, lBuzzSubscribe->id());
		events_.write(lPublisher, lBuzzSubscribe->timestamp(), lBuzzSubscribe->id(), lBuzzSubscribe->type());
		publisherUpdates(lSubscriber, lBuzzSubscribe->timestamp());
		subscriberUpdates(lPublisher, lBuzzSubscribe->timestamp());
		incrementSubscriptions(lSubscriber);
		incrementFollowers(lPublisher);

		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: PUSHED buzzer subscription ") +
			strprintf("p = %s, s = %s, %s/%s#", lPublisher.toHex(), lSubscriber.toHex(), ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
	} else {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/error]: buzzer subscription IS NOT consistent"));
	}
}

void BuzzerTransactionStoreExtension::processUnsubscribe(const uint256& id, TransactionContextPtr ctx) {
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
				publishersIdx_.remove(lPublisher, lSubscriber);
				decrementSubscriptions(lSubscriber);
				decrementFollowers(lPublisher);

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
}

void BuzzerTransactionStoreExtension::processLike(const uint256& id, TransactionContextPtr ctx) {
	// extract buzz_id
	TxBuzzLikePtr lBuzzLike = TransactionHelper::to<TxBuzzLike>(ctx->tx());
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing buzz like ") +
		strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
	//
	if (lBuzzLike->in().size() >= 1) {
		// extract buzz | buzzer
		uint256 lBuzzChainId = lBuzzLike->in()[TX_BUZZ_LIKE_IN].out().chain(); // buzz chain_id
		uint256 lBuzzId = lBuzzLike->in()[TX_BUZZ_LIKE_IN].out().tx(); // buzz_id
		uint256 lBuzzerId = lBuzzLike->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
		// check index
		if (!checkLike(lBuzzId, lBuzzerId)) {
			// write index
			likesIdx_.write(lBuzzId, lBuzzerId, lBuzzLike->id());

			// update like count
			incrementLikes(lBuzzId);

			TransactionPtr lBuzz = store_->locateTransaction(lBuzzId);
			if (lBuzz) {
				//
				// WARNING: breaks current logic
				/*
				uint256 lPublisherBuzzerId = lBuzz->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
				// update endorsements
				incrementEndorsements(lPublisherBuzzerId, BUZZER_ENDORSE_LIKE);
				*/
			}

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzz like PUSHED: ") +
				strprintf("tx = %s, buzz = %s, liker = %s", ctx->tx()->id().toHex(), lBuzzId.toHex(), lBuzzerId.toHex()));
		} else {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/error]: buzz is ALREADY liked: ") +
				strprintf("tx = %s, buzz = %s, liker = %s", ctx->tx()->id().toHex(), lBuzzId.toHex(), lBuzzerId.toHex()));
		}
	}
}

void BuzzerTransactionStoreExtension::processReward(const uint256& id, TransactionContextPtr ctx) {
	// extract buzz_id
	TxBuzzRewardPtr lBuzzReward = TransactionHelper::to<TxBuzzReward>(ctx->tx());
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing buzz reward ") +
		strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
	//
	if (lBuzzReward->in().size() >= 1) {
		// extract buzz | buzzer
		uint256 lBuzzChainId = lBuzzReward->in()[TX_BUZZ_REWARD_IN].out().chain(); // buzz chain_id
		uint256 lBuzzId = lBuzzReward->in()[TX_BUZZ_REWARD_IN].out().tx(); // buzz_id
		uint256 lBuzzerId = lBuzzReward->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator

		ITransactionStorePtr lStore = store_->storeManager()->locate(MainChain::id());
		if (lStore) {
			TransactionPtr lRewardTx = lStore->locateTransaction(lBuzzReward->rewardTx());
			if (lRewardTx) {
				amount_t lAmount;
				if (lBuzzReward->extractAmount(lRewardTx, lAmount)) {
					incrementRewards(lBuzzId, lAmount);
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzz reward PUSHED: ") +
						strprintf("amount = %d, tx = %s, buzz = %s, rewarder = %s", lAmount, ctx->tx()->id().toHex(), lBuzzId.toHex(), lBuzzerId.toHex()));
				}
			} else {
				incrementRewards(lBuzzId, lBuzzReward->amount());
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzz reward PUSHED: ") +
					strprintf("amount = %d, tx = %s, buzz = %s, rewarder = %s", lBuzzReward->amount(), ctx->tx()->id().toHex(), lBuzzId.toHex(), lBuzzerId.toHex()));
			}
		}
	}
}

void BuzzerTransactionStoreExtension::removeTransaction(TransactionPtr tx) {
	//
	if (tx->type() == TX_BUZZ ||
		tx->type() == TX_REBUZZ ||
		tx->type() == TX_BUZZ_LIKE || 
		tx->type() == TX_BUZZ_REWARD || 
		tx->type() == TX_BUZZ_REPLY ||
		tx->type() == TX_BUZZ_REBUZZ_NOTIFY) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/removeTransaction]: removing event from timeline ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(tx);
		// in[0] - publisher/initiator
		uint256 lPublisher = lEvent->in()[0].out().tx();

		// special case
		if (tx->type() == TX_BUZZ_REBUZZ_NOTIFY) {
			//
			TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(tx);
			decrementRebuzzes(lRebuzz->buzzId());
			// rebuzzes_.remove(lRebuzz->buzzChainId(), lRebuzz->buzzId(), lEvent->id());
			decrementEndorsements(lRebuzz->buzzerId(), BUZZER_ENDORSE_REBUZZ);			
		}

		// remove rebuzz/reply count
		if (tx->type() == TX_REBUZZ ||
			tx->type() == TX_BUZZ_REPLY) {
			// direct rebuzz
			if (tx->type() == TX_REBUZZ) { 
				//
				TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(tx);
				decrementRebuzzes(lRebuzz->buzzId());
				// rebuzzes_.remove(lRebuzz->buzzChainId(), lRebuzz->buzzId(), lEvent->id());
				decrementEndorsements(lRebuzz->buzzerId(), BUZZER_ENDORSE_REBUZZ);
			}
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
						decrementReplies(lInTx->id());
						// make tree
						replies_.remove(lInTx->id(), lEvent->timestamp(), lEvent->id());
						//  process sub-tree
						if (lInTx->type() != TX_REBUZZ && lInTx->in().size() > TX_BUZZ_REPLY_BUZZ_IN) {
							if (lInTx->type() == TX_BUZZ_REPLY) {
								uint256 lReTxId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
								while (true) {
									//
									TransactionPtr lReTx = store_->locateTransaction(lReTxId);
									if (lReTx) {
										// dec replies
										decrementReplies(lReTxId);
										// make tree
										replies_.remove(lReTxId, lEvent->timestamp(), lEvent->id());

										if (lReTx->type() == TX_BUZZ || lReTx->type() == TX_REBUZZ) { 
											break;
										}

										if (lReTx->in().size() > TX_BUZZ_REPLY_BUZZ_IN) // move up-next
											lReTxId = lReTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
									} else break;
								}
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
			publishersIdx_.remove(lPublisher, lSubscriber);
			decrementSubscriptions(lSubscriber);
			decrementFollowers(lPublisher);

			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzzer subscription removed ") +
				strprintf("p = %s, s = %s, %s/%s#", lPublisher.toHex(), lSubscriber.toHex(), tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		}
	} else if (tx->type() == TX_BUZZER_UNSUBSCRIBE) {
		// revert to subscription state
		uint256 lSubscribeTx = tx->in()[0].out().tx(); // in[0] - subscribe
		TransactionPtr lSubscribe = store_->locateTransaction(lSubscribeTx);

		if (lSubscribe) {
			TxBuzzerSubscribePtr lSubscribeTx = TransactionHelper::to<TxBuzzerSubscribe>(lSubscribe);

			uint256 lPublisher = lSubscribe->in()[0].out().tx(); // in[0] - publisher
			uint256 lSubscriber = lSubscribe->in()[1].out().tx(); // in[1] - subscriber
			subscriptionsIdx_.write(lSubscriber, lPublisher, lSubscribe->id());
			publishersIdx_.write(lPublisher, lSubscriber, lSubscribe->id());
			events_.write(lPublisher, lSubscribeTx->timestamp(), lSubscribe->id(), lSubscribe->type());
			incrementSubscriptions(lSubscriber);
			incrementFollowers(lPublisher);
		}

	} else if (tx->type() == TX_BUZZ_LIKE) {
		// extract buzz_id
		TxBuzzLikePtr lBuzzLike = TransactionHelper::to<TxBuzzLike>(tx);
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removing buzz like ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		if (lBuzzLike->in().size() >= 1) {
			// extract buzz | buzzer
			uint256 lBuzzId = lBuzzLike->in()[1].out().tx(); //
			uint256 lBuzzerId = lBuzzLike->in()[0].out().tx(); //
			// remove index
			db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*liker*/, uint256 /*like_tx*/>::Iterator lLikeIdx = 
				likesIdx_.find(lBuzzId, lBuzzerId);
			//
			if (lLikeIdx.valid()) {
				//
				likesIdx_.remove(lBuzzId, lBuzzerId);
				// update like count
				decrementLikes(lBuzzId);
				// remove index
				TransactionPtr lBuzz = store_->locateTransaction(lBuzzId);
				if (lBuzz) {
					uint256 lPublisherBuzzerId = lBuzz->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
					decrementEndorsements(lPublisherBuzzerId, BUZZER_ENDORSE_LIKE);
				}
			}

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzz like removed ") +
				strprintf("buzz = %s, liker = %s", lBuzzId.toHex(), lBuzzerId.toHex()));
		}
	} else if (tx->type() == TX_BUZZ_REWARD) {
		// extract buzz_id
		TxBuzzRewardPtr lBuzzReward = TransactionHelper::to<TxBuzzReward>(tx);
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removing buzz reward ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		if (lBuzzReward->in().size() >= 1) {
			// extract buzz | buzzer
			uint256 lBuzzChainId = lBuzzReward->in()[TX_BUZZ_REWARD_IN].out().chain(); // buzz chain_id
			uint256 lBuzzId = lBuzzReward->in()[TX_BUZZ_REWARD_IN].out().tx(); // buzz_id
			uint256 lBuzzerId = lBuzzReward->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator

			ITransactionStorePtr lStore = store_->storeManager()->locate(MainChain::id());
			if (lStore) {
				TransactionPtr lRewardTx = lStore->locateTransaction(lBuzzReward->rewardTx());
				if (lRewardTx) {
					amount_t lAmount;
					if (lBuzzReward->extractAmount(lRewardTx, lAmount)) {
						decrementRewards(lBuzzId, lAmount);
						if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzz reward removed ") +
							strprintf("amount = %d, tx = %s, buzz = %s, rewarder = %s", lAmount, tx->id().toHex(), lBuzzId.toHex(), lBuzzerId.toHex()));
					}
				}
			}
		}
	} else if (tx->type() == TX_BUZZER_ENDORSE) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removing endorsement ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(tx);

		// in[0] - publisher/initiator
		uint256 lEndoser = lEvent->in()[TX_BUZZER_ENDORSE_MY_IN].out().tx(); 
		// in[1] - publisher/initiator
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_ENDORSE_BUZZER_IN].out().tx(); 
		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/>::Iterator 
														lEndorse = endorsements_.find(lBuzzer, lEndoser);
		//
		if (lEndorse.valid()) {
			//
			endorsements_.remove(lBuzzer, lEndoser);
			//
			ITransactionStorePtr lStore = store_->storeManager()->locate(MainChain::id());
			if (lStore && lEvent->in().size() > TX_BUZZER_ENDORSE_FEE_IN) {
				TransactionPtr lFeeTx = lStore->locateTransaction(lEvent->in()[TX_BUZZER_ENDORSE_FEE_IN].out().tx());
				if (lFeeTx) {
					TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

					amount_t lAmount;
					uint64_t lHeight;
					if (TxBuzzerEndorse::extractLockedAmountAndHeight(lFee, lAmount, lHeight)) {
						//
						decrementEndorsements(lBuzzer, lAmount);
						if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removed endorsement ") +
							strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
					}
				}
			}
		}
	} else if (tx->type() == TX_BUZZER_MISTRUST) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removing mistrust ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(tx);

		// in[0] - publisher/initiator
		uint256 lMistruster = lEvent->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx(); 
		// in[1] - publisher/initiator
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 
		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/>::Iterator 
														lMistrust = mistrusts_.find(lBuzzer, lMistruster);

		if (lMistrust.valid()) {
			//
			mistrusts_.remove(lBuzzer, lMistruster);
			//
			ITransactionStorePtr lStore = store_->storeManager()->locate(MainChain::id());
			if (lStore && lEvent->in().size() > TX_BUZZER_MISTRUST_FEE_IN) {
				TransactionPtr lFeeTx = lStore->locateTransaction(lEvent->in()[TX_BUZZER_MISTRUST_FEE_IN].out().tx());
				if (lFeeTx) {
					TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

					amount_t lAmount;
					uint64_t lHeight;
					if (TxBuzzerMistrust::extractLockedAmountAndHeight(lFee, lAmount, lHeight)) {
						//
						decrementMistrusts(lBuzzer, lAmount);
						if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removed mistrust ") +
							strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
					}
				}
			}
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

void BuzzerTransactionStoreExtension::incrementRewards(const uint256& buzz, amount_t reward) {
	//
	BuzzInfo lInfo;
	buzzInfo_.read(buzz, lInfo);
	lInfo.rewards_ += reward;
	buzzInfo_.write(buzz, lInfo);
}

void BuzzerTransactionStoreExtension::decrementRewards(const uint256& buzz, amount_t reward) {
	//
	BuzzInfo lInfo;
	buzzInfo_.read(buzz, lInfo);
	lInfo.rewards_ -= reward;
	buzzInfo_.write(buzz, lInfo);
}

void BuzzerTransactionStoreExtension::incrementEndorsements(const uint256& buzzer, amount_t delta) {
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.incrementEndorsements(delta);
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::decrementEndorsements(const uint256& buzzer, amount_t delta) {
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.decrementEndorsements(delta);
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::incrementMistrusts(const uint256& buzzer, amount_t delta) {
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.incrementMistrusts(delta);
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::decrementMistrusts(const uint256& buzzer, amount_t delta) {
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.decrementMistrusts(delta);
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::updateBuzzerInfo(const uint256& buzzer, const uint256& info) {
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.setInfo(info);
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::incrementSubscriptions(const uint256& buzzer) {
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.incrementSubscriptions();
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::decrementSubscriptions(const uint256& buzzer) {
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.decrementSubscriptions();
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::incrementFollowers(const uint256& buzzer) {
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.incrementFollowers();
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::decrementFollowers(const uint256& buzzer) {
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.decrementFollowers();
	buzzerStat_.write(buzzer, lInfo);
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

bool BuzzerTransactionStoreExtension::readBuzzerStat(const uint256& buzzer, BuzzerInfo& info) {
	//
	return buzzerStat_.read(buzzer, info);
}

TxBuzzerInfoPtr BuzzerTransactionStoreExtension::readBuzzerInfo(const uint256& buzzer) {
	//
	BuzzerInfo lInfo;
	if (buzzerStat_.read(buzzer, lInfo)) {
		//
		TransactionPtr lTx = store_->locateTransaction(lInfo.info());
		if (lTx) {
			return TransactionHelper::to<TxBuzzerInfo>(lTx);
		}
	}

	return nullptr;
}

bool BuzzerTransactionStoreExtension::selectBuzzItem(const uint256& buzzId, BuzzfeedItem& item) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzItem]: selecting buzz item ") +
		strprintf("%s/%s#", buzzId.toHex(), store_->chain().toHex().substr(0, 10)));

	TxBuzzerPtr lBuzzer = nullptr;
	TransactionPtr lTx = store_->locateTransaction(buzzId);
	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());

	//
	if (lTx && (lTx->type() == TX_BUZZ || lTx->type() == TX_REBUZZ || lTx->type() == TX_BUZZ_REPLY)) {
		//
		TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lTx);

		uint256 lTxPublisher = lTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer allways is the first in
		TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lTxPublisher);
		if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);

		prepareBuzzfeedItem(item, lBuzz, lBuzzer);

		if (lTx->type() == TX_BUZZ_REPLY) {
			// NOTE: should be in the same chain
			uint256 lOriginalId = lBuzz->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
			uint256 lOriginalChainId = lBuzz->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
			item.setOriginalBuzzId(lOriginalId);
			item.setOriginalBuzzChainId(lOriginalChainId);
		} else 	if (lTx->type() == TX_REBUZZ) {
			// NOTE: source buzz can be in different chain, so just add info for postponed loading
			TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lBuzz);
			item.setOriginalBuzzId(lRebuzz->buzzId());
			item.setOriginalBuzzChainId(lRebuzz->buzzChainId());
		}

		return true;
	}

	return false;
}

void BuzzerTransactionStoreExtension::selectMistrusts(const uint256& from, const uint256& buzzer, std::vector<EventsfeedItem>& feed) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectMistrusts]: selecting mistrusts for ") +
		strprintf("buzzer = %s, from = %s, chain = %s#", buzzer.toHex(), buzzer.toHex(), store_->chain().toHex().substr(0, 10)));

	//
	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());

	std::multimap<uint64_t, EventsfeedItem::Key> lRawBuzzfeed;
	std::map<EventsfeedItem::Key, EventsfeedItemPtr> lBuzzItems;

	db::DbTwoKeyContainer<
		uint256 /*buzzer*/, 
		uint256 /*mistruster*/, 
		uint256 /*tx*/>::Transaction lTransaction = mistrusts_.transaction();

	db::DbTwoKeyContainer<
		uint256 /*buzzer*/, 
		uint256 /*mistruster*/, 
		uint256 /*tx*/>::Iterator lFrom = mistrusts_.find(buzzer, from);

	lFrom.setKey2Empty();
	//
	for (int lCount = 0; lCount < 50 /*TODO: settings?*/ && lFrom.valid(); ++lFrom, ++lCount) {
		//
		uint256 lBuzzer;
		uint256 lMistruster;
		uint256 lEvent; 

		if (lFrom.first(lBuzzer, lMistruster) && lFrom.second(lEvent)) {
			//
			TransactionPtr lTx = store_->locateTransaction(lEvent);
			if (!lTx) {
				lTransaction.remove(lFrom);
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectMistrusts]: event NOT FOUND ") +
					strprintf("%s/%s#", lEvent.toHex(), store_->chain().toHex().substr(0, 10)));
				continue;
			}

			TxBuzzerMistrustPtr lMistrust = TransactionHelper::to<TxBuzzerMistrust>(lTx);

			EventsfeedItemPtr lItem = EventsfeedItem::instance();
			lItem->setType(TX_BUZZER_MISTRUST);
			lItem->setTimestamp(lMistrust->timestamp());
			lItem->setBuzzId(lMistrust->id());
			lItem->setBuzzChainId(lMistrust->chain());
			lItem->setValue(lMistrust->amount());
			lItem->setSignature(lMistrust->signature());
			lItem->setPublisher(lBuzzer);

			EventsfeedItem::EventInfo lInfo(lMistrust->timestamp(), lMistruster, lMistrust->buzzerInfoChain(), lMistrust->buzzerInfo(), lMistrust->score());
			lInfo.setEvent(lMistrust->chain(), lMistrust->id(), lMistrust->signature());
			lItem->addEventInfo(lInfo);

			lRawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
			lBuzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));
		}
	}

	lTransaction.commit(); // clean-up

	for (std::multimap<uint64_t, EventsfeedItem::Key>::reverse_iterator lItem = lRawBuzzfeed.rbegin(); lItem != lRawBuzzfeed.rend(); lItem++) {
		//
		if (feed.size() < 100 /*max last events*/) {
			std::map<EventsfeedItem::Key, EventsfeedItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) feed.push_back(*(lBuzzItem->second));
		} else {
			break;
		}
	}	
}

void BuzzerTransactionStoreExtension::selectEndorsements(const uint256& from, const uint256& buzzer, std::vector<EventsfeedItem>& feed) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEndorsements]: selecting endorsements for ") +
		strprintf("buzzer = %s, from = %s, chain = %s#", buzzer.toHex(), buzzer.toHex(), store_->chain().toHex().substr(0, 10)));

	//
	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());

	std::multimap<uint64_t, EventsfeedItem::Key> lRawBuzzfeed;
	std::map<EventsfeedItem::Key, EventsfeedItemPtr> lBuzzItems;

	db::DbTwoKeyContainer<
		uint256 /*buzzer*/, 
		uint256 /*endorser*/, 
		uint256 /*tx*/>::Transaction lTransaction = endorsements_.transaction();

	db::DbTwoKeyContainer<
		uint256 /*buzzer*/, 
		uint256 /*endorser*/, 
		uint256 /*tx*/>::Iterator lFrom = endorsements_.find(buzzer, from);

	lFrom.setKey2Empty();
	//
	for (int lCount = 0; lCount < 50 /*TODO: settings?*/ && lFrom.valid(); ++lFrom, ++lCount) {
		//
		uint256 lBuzzer;
		uint256 lEndorser;
		uint256 lEvent; 

		if (lFrom.first(lBuzzer, lEndorser) && lFrom.second(lEvent)) {
			//
			TransactionPtr lTx = store_->locateTransaction(lEvent);
			if (!lTx) {
				lTransaction.remove(lFrom);
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEndorsements]: event NOT FOUND ") +
					strprintf("%s/%s#", lEvent.toHex(), store_->chain().toHex().substr(0, 10)));
				continue;
			}

			TxBuzzerEndorsePtr lEndorse = TransactionHelper::to<TxBuzzerEndorse>(lTx);

			EventsfeedItemPtr lItem = EventsfeedItem::instance();
			lItem->setType(TX_BUZZER_ENDORSE);
			lItem->setTimestamp(lEndorse->timestamp());
			lItem->setBuzzId(lEndorse->id());
			lItem->setBuzzChainId(lEndorse->chain());
			lItem->setValue(lEndorse->amount());
			lItem->setSignature(lEndorse->signature());
			lItem->setPublisher(lBuzzer);

			EventsfeedItem::EventInfo lInfo(lEndorse->timestamp(), lEndorser, lEndorse->buzzerInfoChain(), lEndorse->buzzerInfo(), lEndorse->score());
			lInfo.setEvent(lEndorse->chain(), lEndorse->id(), lEndorse->signature());
			lItem->addEventInfo(lInfo);

			lRawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
			lBuzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));
		}
	}

	lTransaction.commit(); // clean-up

	for (std::multimap<uint64_t, EventsfeedItem::Key>::reverse_iterator lItem = lRawBuzzfeed.rbegin(); lItem != lRawBuzzfeed.rend(); lItem++) {
		//
		if (feed.size() < 100 /*max last events*/) {
			std::map<EventsfeedItem::Key, EventsfeedItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) feed.push_back(*(lBuzzItem->second));
		} else {
			break;
		}
	}	
}

void BuzzerTransactionStoreExtension::selectSubscriptions(const uint256& from, const uint256& buzzer, std::vector<EventsfeedItem>& feed) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectSubscriptions]: selecting subscriptions for ") +
		strprintf("buzzer = %s, from = %s, chain = %s#", buzzer.toHex(), from.toHex(), store_->chain().toHex().substr(0, 10)));

	//
	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());

	std::multimap<uint64_t, EventsfeedItem::Key> lRawBuzzfeed;
	std::map<EventsfeedItem::Key, EventsfeedItemPtr> lBuzzItems;

	db::DbTwoKeyContainer<
		uint256 /*subscriber*/, 
		uint256 /*publisher*/, 
		uint256 /*tx*/>::Transaction lTransaction = subscriptionsIdx_.transaction();

	db::DbTwoKeyContainer<
		uint256 /*subscriber*/, 
		uint256 /*publisher*/, 
		uint256 /*tx*/>::Iterator lFrom;

	if (from.isNull()) lFrom = subscriptionsIdx_.find(buzzer);
	else lFrom = subscriptionsIdx_.find(buzzer, from);

	lFrom.setKey2Empty();
	//
	for (int lCount = 0; lCount < 50 /*TODO: settings?*/ && lFrom.valid(); ++lFrom, ++lCount) {
		//
		uint256 lSubscriber;
		uint256 lPublisher;
		uint256 lEvent; 

		if (lFrom.first(lSubscriber, lPublisher) && lFrom.second(lEvent) /*&& buzzer != lPublisher*/) {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectSubscriptions]: adding publisher ") +
				strprintf("%s for %s", lPublisher.toHex(), lSubscriber.toHex()));
			//
			TransactionPtr lTx = store_->locateTransaction(lEvent);
			if (!lTx) {
				lTransaction.remove(lFrom);
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectSubscriptions]: event NOT FOUND ") +
					strprintf("%s/%s#", lEvent.toHex(), store_->chain().toHex().substr(0, 10)));
				continue;
			}

			TxBuzzerSubscribePtr lSubscribe = TransactionHelper::to<TxBuzzerSubscribe>(lTx);
			lPublisher = lSubscribe->in()[0].out().tx(); // in[0] - publisher
			lSubscriber = lSubscribe->in()[1].out().tx(); // in[1] - subscriber

			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lSubscriber);
			TransactionPtr lPublisherTx = lMainStore->locateTransaction(lPublisher);

			ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
			if (!lStore) continue;

			ITransactionStorePtr lPublisherStore = store_->storeManager()->locate(lPublisherTx->in()[TX_BUZZER_SHARD_IN].out().tx());
			if (!lPublisherStore) continue;

			if (!lStore->extension()) continue;
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			if (!lPublisherStore->extension()) continue;
			BuzzerTransactionStoreExtensionPtr lPublisherExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lPublisherStore->extension());

			TxBuzzerInfoPtr lBuzzerInfo = lExtension->readBuzzerInfo(lSubscriber);
			TxBuzzerInfoPtr lPublisherInfo = lPublisherExtension->readBuzzerInfo(lPublisher);
			if (lBuzzerInfo && lPublisherInfo) {
				//
				EventsfeedItemPtr lItem = EventsfeedItem::instance();
				lItem->setType(TX_BUZZER_SUBSCRIBE);
				lItem->setTimestamp(lSubscribe->timestamp());
				lItem->setBuzzId(lSubscribe->id());
				lItem->setBuzzChainId(lSubscribe->chain());
				lItem->setSignature(lSubscribe->signature());
				lItem->setPublisher(lPublisher);
				lItem->setPublisherInfo(lPublisherInfo->id());
				lItem->setPublisherInfoChain(lPublisherInfo->chain());

				EventsfeedItem::EventInfo lInfo(lSubscribe->timestamp(), lSubscriber, lBuzzerInfo->chain(), lBuzzerInfo->id(), lSubscribe->score());
				lInfo.setEvent(lSubscribe->chain(), lSubscribe->id(), lSubscribe->signature());
				lItem->addEventInfo(lInfo);

				lRawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
				lBuzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));
			}
		}
	}

	lTransaction.commit(); // clean-up

	for (std::multimap<uint64_t, EventsfeedItem::Key>::reverse_iterator lItem = lRawBuzzfeed.rbegin(); lItem != lRawBuzzfeed.rend(); lItem++) {
		//
		if (feed.size() < 100 /*max last events*/) {
			std::map<EventsfeedItem::Key, EventsfeedItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) feed.push_back(*(lBuzzItem->second));
		} else {
			break;
		}
	}	
}

void BuzzerTransactionStoreExtension::selectFollowers(const uint256& from, const uint256& buzzer, std::vector<EventsfeedItem>& feed) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectFollowers]: selecting followers for ") +
		strprintf("buzzer = %s, from = %s, chain = %s#", buzzer.toHex(), buzzer.toHex(), store_->chain().toHex().substr(0, 10)));

	//
	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());

	std::multimap<uint64_t, EventsfeedItem::Key> lRawBuzzfeed;
	std::map<EventsfeedItem::Key, EventsfeedItemPtr> lBuzzItems;

	db::DbTwoKeyContainer<
		uint256 /*publisher*/, 
		uint256 /*subscriber*/, 
		uint256 /*tx*/>::Transaction lTransaction = publishersIdx_.transaction();

	db::DbTwoKeyContainer<
		uint256 /*publisher*/, 
		uint256 /*subscriber*/, 
		uint256 /*tx*/>::Iterator lFrom = publishersIdx_.find(buzzer, from);

	lFrom.setKey2Empty();
	//
	for (int lCount = 0; lCount < 50 /*TODO: settings?*/ && lFrom.valid(); ++lFrom, ++lCount) {
		//
		uint256 lSubscriber;
		uint256 lPublisher;
		uint256 lEvent; 

		if (lFrom.first(lPublisher, lSubscriber) && lFrom.second(lEvent)) {
			//
			TransactionPtr lTx = store_->locateTransaction(lEvent);
			if (!lTx) {
				lTransaction.remove(lFrom);
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectFollowers]: event NOT FOUND ") +
					strprintf("%s/%s#", lEvent.toHex(), store_->chain().toHex().substr(0, 10)));
				continue;
			}

			TxBuzzerSubscribePtr lSubscribe = TransactionHelper::to<TxBuzzerSubscribe>(lTx);
			lPublisher = lSubscribe->in()[0].out().tx(); // in[0] - publisher
			lSubscriber = lSubscribe->in()[1].out().tx(); // in[1] - subscriber

			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lSubscriber);
			TransactionPtr lPublisherTx = lMainStore->locateTransaction(lPublisher);

			ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
			if (!lStore) continue;

			if (!lStore->extension()) continue;
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			ITransactionStorePtr lPublisherStore = store_->storeManager()->locate(lPublisherTx->in()[TX_BUZZER_SHARD_IN].out().tx());
			if (!lPublisherStore) continue;

			if (!lPublisherStore->extension()) continue;
			BuzzerTransactionStoreExtensionPtr lPublisherExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lPublisherStore->extension());

			TxBuzzerInfoPtr lBuzzerInfo = lExtension->readBuzzerInfo(lSubscriber);
			TxBuzzerInfoPtr lPublisherInfo = lPublisherExtension->readBuzzerInfo(lPublisher);
			if (lBuzzerInfo && lPublisherInfo) {
				//
				EventsfeedItemPtr lItem = EventsfeedItem::instance();
				lItem->setType(TX_BUZZER_SUBSCRIBE);
				lItem->setTimestamp(lSubscribe->timestamp());
				lItem->setBuzzId(lSubscribe->id());
				lItem->setBuzzChainId(lSubscribe->chain());
				lItem->setSignature(lSubscribe->signature());
				lItem->setPublisher(lPublisher);
				lItem->setPublisherInfo(lPublisherInfo->id());
				lItem->setPublisherInfoChain(lPublisherInfo->chain());

				EventsfeedItem::EventInfo lInfo(lSubscribe->timestamp(), lSubscriber, lBuzzerInfo->chain(), lBuzzerInfo->id(), lSubscribe->score());
				lInfo.setEvent(lSubscribe->chain(), lSubscribe->id(), lSubscribe->signature());
				lItem->addEventInfo(lInfo);

				lRawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
				lBuzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));
			}
		}
	}

	lTransaction.commit(); // clean-up

	for (std::multimap<uint64_t, EventsfeedItem::Key>::reverse_iterator lItem = lRawBuzzfeed.rbegin(); lItem != lRawBuzzfeed.rend(); lItem++) {
		//
		if (feed.size() < 100 /*max last events*/) {
			std::map<EventsfeedItem::Key, EventsfeedItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) feed.push_back(*(lBuzzItem->second));
		} else {
			break;
		}
	}	
}

void BuzzerTransactionStoreExtension::selectEventsfeed(uint64_t from, const uint256& subscriber, std::vector<EventsfeedItem>& feed) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEventsfeed]: selecting eventsfeed for ") +
		strprintf("subscriber = %s, from = %d, chain = %s#", subscriber.toHex(), from, store_->chain().toHex().substr(0, 10)));

	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());

	// 0. get/set current time
	uint64_t lTimeFrom = from;
	if (lTimeFrom == 0) lTimeFrom = qbit::getMicroseconds();
	
	// 1.collect events by subscriber
	uint64_t lSubscriberTime = lTimeFrom;
	subscriberUpdates_.read(subscriber, lSubscriberTime);

	std::multimap<uint64_t, EventsfeedItem::Key> lRawBuzzfeed;
	std::map<EventsfeedItem::Key, EventsfeedItemPtr> lBuzzItems;

	// 1.0. prepare clean-up transaction
	db::DbThreeKeyContainer<uint256, uint64_t, uint256, unsigned short>::Transaction lEventsTransaction = events_.transaction();

	// 1.1. locating most recent event made by publisher
	db::DbThreeKeyContainer<uint256 /*subscriber*/, uint64_t /*timestamp*/, uint256 /*tx*/, unsigned short /*type*/>::Iterator lFromEvent = 
			events_.find(subscriber, lTimeFrom < lSubscriberTime ? lTimeFrom : lSubscriberTime);
	// 1.2. resetting timestamp to ensure appropriate backtracing
	lFromEvent.setKey2Empty();
	// 1.3. stepping down up to max 50 events 
	for (int lCount = 0; lCount < 50 /*TODO: settings?*/ && lFromEvent.valid(); --lFromEvent, ++lCount) {
		//
		TxBuzzerPtr lBuzzer = nullptr;

		if (*lFromEvent == TX_BUZZ || *lFromEvent == TX_BUZZ_REPLY || 
			*lFromEvent == TX_REBUZZ || *lFromEvent == TX_BUZZ_LIKE || *lFromEvent == TX_BUZZ_REWARD ||
			*lFromEvent == TX_BUZZER_MISTRUST || *lFromEvent == TX_BUZZER_ENDORSE ||
			*lFromEvent == TX_BUZZER_SUBSCRIBE) {
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
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEventsfeed]: event NOT FOUND ") +
						strprintf("%s/%s#", lEvent.toHex(), store_->chain().toHex().substr(0, 10)));
					continue;
				}

				if (*lFromEvent == TX_BUZZ || *lFromEvent == TX_BUZZ_REPLY || *lFromEvent == TX_REBUZZ) {
					//
					uint256 lPublisher = lTx->in()[TX_BUZZ_REPLY_MY_IN].out().tx();
					//
					TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lPublisher);
					if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
					//
					makeEventsfeedItem(lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems);
				} else if (*lFromEvent == TX_BUZZ_LIKE) {
					//
					makeEventsfeedLikeItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems);
				} else if (*lFromEvent == TX_BUZZ_REWARD) {
					//
					uint256 lPublisher = lTx->in()[TX_BUZZ_MY_BUZZER_IN].out().tx();
					uint256 lBuzzId = lTx->in()[TX_BUZZ_REWARD_IN].out().tx(); // buzz_id
					uint256 lBuzzChainId = lTx->in()[TX_BUZZ_REWARD_IN].out().chain(); // buzz_id

					EventsfeedItemPtr lItem = EventsfeedItem::instance();
					makeEventsfeedRewardItem(lItem, lTx, lBuzzId, lBuzzChainId, lPublisher);
					//
					lRawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
					lBuzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));

				} else if (*lFromEvent == TX_BUZZER_MISTRUST) {
					//
					uint256 lMistruster = lTx->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx();
					TxBuzzerMistrustPtr lMistrust = TransactionHelper::to<TxBuzzerMistrust>(lTx);
					//
					EventsfeedItemPtr lItem = EventsfeedItem::instance();
					makeEventsfeedTrustScoreItem<TxBuzzerMistrust>(lItem, lTx, *lFromEvent, lMistruster, lMistrust->score());
					//
					lRawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
					lBuzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));

				} else if (*lFromEvent == TX_BUZZER_ENDORSE) {
					//
					uint256 lEndorser = lTx->in()[TX_BUZZER_ENDORSE_MY_IN].out().tx(); 
					TxBuzzerEndorsePtr lEndorse = TransactionHelper::to<TxBuzzerEndorse>(lTx);
					//
					EventsfeedItemPtr lItem = EventsfeedItem::instance();
					makeEventsfeedTrustScoreItem<TxBuzzerEndorse>(lItem, lTx, *lFromEvent, lEndorser, lEndorse->score());
					//
					lRawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
					lBuzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));
				} else if (*lFromEvent == TX_BUZZER_SUBSCRIBE) {
					//
					TxBuzzerSubscribePtr lSubscribeTx = TransactionHelper::to<TxBuzzerSubscribe>(lTx);
					uint256 lPublisher = lSubscribeTx->in()[0].out().tx(); // in[0] - publisher
					uint256 lSubscriber = lSubscribeTx->in()[1].out().tx(); // in[1] - subscriber

					EventsfeedItemPtr lItem = EventsfeedItem::instance();
					TxBuzzerPtr lBuzzer = nullptr;
					TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lSubscriber);
					if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
					//
					lItem->setType(TX_BUZZER_SUBSCRIBE);
					lItem->setTimestamp(lSubscribeTx->timestamp());
					lItem->setBuzzId(lSubscribeTx->id());
					lItem->setBuzzChainId(lSubscribeTx->chain());
					lItem->setSignature(lSubscribeTx->signature());
					lItem->setPublisher(lPublisher);

					EventsfeedItem::EventInfo lInfo(lSubscribeTx->timestamp(), lBuzzer->id(), lSubscribeTx->buzzerInfoChain(), lSubscribeTx->buzzerInfo(), lSubscribeTx->score());
					lInfo.setEvent(lSubscribeTx->chain(), lSubscribeTx->id(), lSubscribeTx->signature());
					lItem->addEventInfo(lInfo);					
					//
					lRawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
					lBuzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));
					//
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEventsfeed]: ") +
						strprintf("add subscribe = %s, %s, %s#", lPublisher.toHex(), lSubscriber.toHex(), store_->chain().toHex().substr(0, 10)));		
				}
			}
		} 
	}

	lEventsTransaction.commit(); // clean-up

	// 4. fill reply
	for (std::multimap<uint64_t, EventsfeedItem::Key>::reverse_iterator lItem = lRawBuzzfeed.rbegin(); lItem != lRawBuzzfeed.rend(); lItem++) {
		//
		if (feed.size() < 100 /*max last events*/) {
			std::map<EventsfeedItem::Key, EventsfeedItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) feed.push_back(*(lBuzzItem->second));
		} else {
			break;
		}
	}

	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEventsfeed]: ") +
		strprintf("items count = %d, %s#", feed.size(), store_->chain().toHex().substr(0, 10)));		
}

void BuzzerTransactionStoreExtension::makeEventsfeedItem(TxBuzzerPtr buzzer, TransactionPtr tx, ITransactionStorePtr mainStore, std::multimap<uint64_t, EventsfeedItem::Key>& rawBuzzfeed, std::map<EventsfeedItem::Key, EventsfeedItemPtr>& buzzes) {
	//
	if (tx->type() == TX_BUZZ ||
		tx->type() == TX_REBUZZ ||
		tx->type() == TX_BUZZ_REPLY) {
		//
		bool lAdd = true;
		TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(tx);
		//
		EventsfeedItemPtr lItem = EventsfeedItem::instance();
		//
		if (tx->type() == TX_BUZZ || tx->type() == TX_BUZZ_REPLY) {
			//
			prepareEventsfeedItem(*lItem, lBuzz, buzzer, lBuzz->chain(), lBuzz->id(), 
				lBuzz->timestamp(), lBuzz->buzzerInfoChain(), lBuzz->buzzerInfo(), lBuzz->score(), lBuzz->signature());
		} else if (tx->type() == TX_REBUZZ) {
			//
			TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lBuzz);
			uint256 lBuzzId = lRebuzz->buzzId();
			uint256 lBuzzChainId = lRebuzz->buzzChainId();

			// simple rebuzz
			if (lRebuzz->simpleRebuzz()) {
				EventsfeedItem::Key lKey(lBuzzId /*original*/, TX_REBUZZ /*this action*/);

				std::map<EventsfeedItem::Key, EventsfeedItemPtr>::iterator lExistingItem = buzzes.find(lKey);
				if (lExistingItem != buzzes.end()) {
					std::pair<std::multimap<uint64_t, EventsfeedItem::Key>::iterator,
								std::multimap<uint64_t, EventsfeedItem::Key>::iterator> lRange = rawBuzzfeed.equal_range(lExistingItem->second->timestamp());
					for (std::multimap<uint64_t, EventsfeedItem::Key>::iterator lTimestamp = lRange.first; lTimestamp != lRange.second; lTimestamp++) {
						if (lTimestamp->second == lKey) {
							rawBuzzfeed.erase(lTimestamp);
							break;
						}
					}

					lAdd = false;
					lItem = lExistingItem->second;
					//if (lItem->timestamp() < lBuzz->timestamp()) lItem->setTimestamp(lBuzz->timestamp());
					EventsfeedItem::EventInfo lInfo = EventsfeedItem::EventInfo(lRebuzz->timestamp(), buzzer->id(), lRebuzz->buzzerInfoChain(), lRebuzz->buzzerInfo(), lRebuzz->score());
					lInfo.setEvent(lBuzz->chain(), lBuzz->id(), lBuzz->body(), lBuzz->mediaPointers(), lBuzz->signature());
					lItem->addEventInfo(lInfo);
				} else {
					prepareEventsfeedItem(*lItem, lBuzz, buzzer, lBuzz->chain(), lBuzz->id(), 
						lBuzz->timestamp(), lBuzz->buzzerInfoChain(), lBuzz->buzzerInfo(), lBuzz->score(), lBuzz->signature());
					lItem->setBuzzId(lBuzzId);
					lItem->setBuzzChainId(lBuzzChainId);
				}
			} else {
				// rebuzz with comments
				prepareEventsfeedItem(*lItem, lBuzz, buzzer, lBuzz->chain(), lBuzz->id(), 
					lBuzz->timestamp(), lBuzz->buzzerInfoChain(), lBuzz->buzzerInfo(), lBuzz->score(), lBuzz->signature());

				lItem->setType(TX_REBUZZ_REPLY); // special case
				lItem->setBuzzId(lBuzzId);
				lItem->setBuzzChainId(lBuzzChainId);
			}
		}

		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEventsfeed]: item added ") +
			strprintf("buzzer = %s, %d/%s/%s#", buzzer->id().toHex(), lBuzz->timestamp(), lBuzz->id().toHex(), store_->chain().toHex().substr(0, 10)));

		rawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
		if (lAdd) buzzes.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));
	}
}

void BuzzerTransactionStoreExtension::makeEventsfeedLikeItem(TransactionPtr tx, ITransactionStorePtr mainStore, std::multimap<uint64_t, EventsfeedItem::Key>& rawBuzzfeed, std::map<EventsfeedItem::Key, EventsfeedItemPtr>& buzzes) {
	//
	TxBuzzLikePtr lLike = TransactionHelper::to<TxBuzzLike>(tx);

	uint256 lPublisherId = lLike->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
	uint256 lBuzzId = lLike->in()[TX_BUZZ_LIKE_IN].out().tx(); // buzz_id
	uint256 lBuzzChainId = lLike->in()[TX_BUZZ_LIKE_IN].out().chain(); // buzz_id

	bool lAdd = true;
	ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzChainId);
	TransactionPtr lBuzzTx = lStore->locateTransaction(lBuzzId);
	if (lBuzzTx) {
		//
		TxBuzzerPtr lPublisher;
		TransactionPtr lPublisherTx = mainStore->locateTransaction(lPublisherId);
		if (lPublisherTx) lPublisher = TransactionHelper::to<TxBuzzer>(lPublisherTx);
		else return;

		EventsfeedItemPtr lItem = EventsfeedItem::instance();

		EventsfeedItem::Key lKey(lBuzzId, TX_BUZZ_LIKE);
		std::map<EventsfeedItem::Key, EventsfeedItemPtr>::iterator lExistingItem = buzzes.find(lKey);
		if (lExistingItem != buzzes.end()) {
			std::pair<std::multimap<uint64_t, EventsfeedItem::Key>::iterator,
						std::multimap<uint64_t, EventsfeedItem::Key>::iterator> lRange = rawBuzzfeed.equal_range(lExistingItem->second->timestamp());
			for (std::multimap<uint64_t, EventsfeedItem::Key>::iterator lTimestamp = lRange.first; lTimestamp != lRange.second; lTimestamp++) {
				if (lTimestamp->second == lKey) {
					rawBuzzfeed.erase(lTimestamp);
					break;
				}
			}

			lAdd = false;
			lItem = lExistingItem->second;
			//if (lItem->timestamp() < lLike->timestamp()) lItem->setTimestamp(lLike->timestamp());
			EventsfeedItem::EventInfo lInfo = EventsfeedItem::EventInfo(lLike->timestamp(), lPublisher->id(), lLike->buzzerInfoChain(), lLike->buzzerInfo(), lLike->score());
			lInfo.setEvent(lLike->chain(), lLike->id(), lLike->signature());
			lItem->addEventInfo(lInfo);
		} else {
			TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lBuzzTx);
			prepareEventsfeedItem(*lItem, lBuzz, lPublisher, lLike->chain(), lLike->id(), 
				lLike->timestamp(), lLike->buzzerInfoChain(), lLike->buzzerInfo(), lLike->score(), lLike->signature());
			lItem->setType(TX_BUZZ_LIKE);
		}

		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEventsfeed]: item added ") +
			strprintf("buzzer = %s, %d/%s/%s#", lPublisher->id().toHex(), lLike->timestamp(), lLike->id().toHex(), store_->chain().toHex().substr(0, 10)));

		rawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
		if (lAdd) buzzes.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));		
	}
}

void BuzzerTransactionStoreExtension::makeEventsfeedRewardItem(EventsfeedItemPtr item, TransactionPtr tx, const uint256& buzz, const uint256& buzzChain, const uint256& buzzer) {
	//
	TxBuzzRewardPtr lEvent = TransactionHelper::to<TxBuzzReward>(tx);

	item->setType(TX_BUZZ_REWARD);
	item->setTimestamp(lEvent->timestamp());
	item->setBuzzId(buzz);
	item->setBuzzChainId(buzzChain);
	item->setValue(lEvent->amount());
	item->setSignature(lEvent->signature());

	EventsfeedItem::EventInfo lInfo(lEvent->timestamp(), buzzer, lEvent->buzzerInfoChain(), lEvent->buzzerInfo(), lEvent->score());
	lInfo.setEvent(lEvent->chain(), lEvent->id(), lEvent->signature());
	item->addEventInfo(lInfo);
}

void BuzzerTransactionStoreExtension::prepareEventsfeedItem(EventsfeedItem& item, TxBuzzPtr buzz, TxBuzzerPtr buzzer, 
		const uint256& eventChainId, const uint256& eventId, uint64_t eventTimestamp, 
		const uint256& buzzerInfoChain, const uint256& buzzerInfo, uint64_t score, const uint512& signature) {
	//
	item.setType(buzz->type());
	item.setTimestamp(buzz->timestamp());
	item.setBuzzId(buzz->id());
	item.setBuzzChainId(buzz->chain());
	item.setScore(buzz->score());

	if (buzz->type() != TX_REBUZZ) {
		item.setBuzzBody(buzz->body());
		item.setBuzzMedia(buzz->mediaPointers());
		item.setSignature(buzz->signature());
	}

	if (buzzer) {
		EventsfeedItem::EventInfo lInfo(eventTimestamp, buzzer->id(), buzzerInfoChain, buzzerInfo, score);

		if (buzz->type() == TX_BUZZ || buzz->type() == TX_BUZZ_REPLY || buzz->type() == TX_REBUZZ) {
			lInfo.setEvent(eventChainId, eventId, buzz->body(), buzz->mediaPointers(), signature);
		} else {
			lInfo.setEvent(eventChainId, eventId, signature);			
		}

		item.addEventInfo(lInfo);
	}	
}

void BuzzerTransactionStoreExtension::selectBuzzfeedByBuzz(uint64_t from, const uint256& buzz, std::vector<BuzzfeedItem>& feed) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzz]: selecting buzzfeed for ") +
		strprintf("buzz = %s, from = %d, chain = %s#", buzz.toHex(), from, store_->chain().toHex().substr(0, 10)));
	//
	db::DbThreeKeyContainer<
		uint256 /*buzz|rebuzz|reply*/, 
		uint64_t /*timestamp*/, 
		uint256 /*rebuzz|reply*/, 
		uint256 /*publisher*/>::Iterator lFrom = replies_.find(buzz, from);
	db::DbThreeKeyContainer<
		uint256 /*buzz|rebuzz|reply*/, 
		uint64_t /*timestamp*/, 
		uint256 /*rebuzz|reply*/, 
		uint256 /*publisher*/>::Transaction lTransaction = replies_.transaction();

	//
	lFrom.setKey2Empty();

	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());
	std::multimap<uint64_t, BuzzfeedItem::Key> lRawBuzzfeed;
	std::map<BuzzfeedItem::Key, BuzzfeedItemPtr> lBuzzItems;
	
	// add root item
	if (!from) {
		//
		int lContext = 0;
		TxBuzzerPtr lBuzzer = nullptr;
		TransactionPtr lTx = store_->locateTransaction(buzz);
		if (lTx && (lTx->type() == TX_BUZZ || lTx->type() == TX_REBUZZ || lTx->type() == TX_BUZZ_REPLY)) {
			//
			uint256 lTxPublisher = lTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer allways is the first in
			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lTxPublisher);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			//
			bool lProcessed = false;
			if (lTx->type() == TX_REBUZZ) {
				TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lTx);
				if (lRebuzz->simpleRebuzz()) {
					//
					makeBuzzfeedRebuzzItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, uint256());
					lProcessed = true;
				}
			}

			if (!lProcessed) {
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzz]: try to add item ") +
					strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

				makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems);
			}
		}
	}

	// expand thread
	for (;lBuzzItems.size() < 30 /*TODO: settings?*/ && lFrom.valid(); ++lFrom) {
		//
		uint256 lId; 
		uint64_t lTimestamp; 
		uint256 lEventId;

		//
		if (!lFrom.first(lId, lTimestamp, lEventId)) {
			lTransaction.remove(lFrom);
			continue;
		}

		//
		int lContext = 0;
		TxBuzzerPtr lBuzzer = nullptr;
		TransactionPtr lTx = store_->locateTransaction(lEventId);
		if (!lTx) {
			lTransaction.remove(lFrom);
			continue;
		}
		//
		if (lTx->type() == TX_BUZZ_REPLY) {
			//
			uint256 lTxPublisher = lTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer allways is the first in
			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lTxPublisher);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzz]: try to added item ") +
				strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			//
			// TODO: consider to limit uplinkage for parents (at least when from != 0)
			makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems, !from);
		} 
	}

	lTransaction.commit();

	/*
	for (std::multimap<uint64_t, BuzzfeedItem::Key>::iterator lItem = lRawBuzzfeed.begin(); lItem != lRawBuzzfeed.end(); lItem++) {
		//
		if (feed.size() < 30) {
			std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) feed.push_back(*(lBuzzItem->second));
		} else {
			break;
		}
	}
	*/
	bool lContinue = false;
	std::set<BuzzfeedItem::Key> lAdded;
	for (std::multimap<uint64_t, BuzzfeedItem::Key>::iterator lItem = lRawBuzzfeed.begin(); lItem != lRawBuzzfeed.end(); lItem++) {
		//
		if (!lContinue && feed.size() < 30 /*max last mixed events*/ || lContinue && feed.size() < 100) {
			std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) {
				if (lAdded.insert(lItem->second).second) {
					feed.push_back(*(lBuzzItem->second));
					//
					if (lBuzzItem->second->type() == TX_BUZZ_REPLY) {
						lContinue = true;
					} else lContinue = false;
				}
			} else {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzz]: item is MISSING ") +
					strprintf("%s/%s, %s#", lItem->second.id().toHex(), lItem->second.typeToString(), store_->chain().toHex().substr(0, 10)));
			}
		} else {
			break;
		}
	}

	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzz]: ") +
		strprintf("items count = %d, %s#", feed.size(), store_->chain().toHex().substr(0, 10)));	
}

void BuzzerTransactionStoreExtension::selectBuzzfeedByBuzzer(uint64_t from, const uint256& buzzer, std::vector<BuzzfeedItem>& feed) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzzer]: selecting buzzfeed for ") +
		strprintf("buzzer = %s, from = %d, chain = %s#", buzzer.toHex(), from, store_->chain().toHex().substr(0, 10)));

	uint64_t lTimeFrom = from;
	if (lTimeFrom == 0) lTimeFrom = qbit::getMicroseconds();

	uint64_t lPublisherTime = 0;
	if (publisherUpdates_.read(buzzer, lPublisherTime)) {
		// insert own
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzzer]: publisher time is ") +
			strprintf("%s/%d", buzzer.toHex(), lPublisherTime));
	}

	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());
	std::multimap<uint64_t, BuzzfeedItem::Key> lRawBuzzfeed;
	std::map<BuzzfeedItem::Key, BuzzfeedItemPtr> lBuzzItems;

	db::DbTwoKeyContainer<uint256 /*publisher*/, uint64_t /*timestamp*/, uint256 /*tx*/>::Iterator lFrom = 
			timeline_.find(buzzer, lTimeFrom < lPublisherTime ? lTimeFrom : lPublisherTime);
	db::DbTwoKeyContainer<uint256 /*publisher*/, uint64_t /*timestamp*/, uint256 /*tx*/>::Transaction lTransaction = 
			timeline_.transaction();
	lFrom.setKey2Empty();

	for (;lBuzzItems.size() < 30 /*TODO: settings?*/ && lFrom.valid(); --lFrom) {
		//
		int lContext = 0;
		TxBuzzerPtr lBuzzer = nullptr;
		TransactionPtr lTx = store_->locateTransaction(*lFrom);
		if (!lTx) {
			lTransaction.remove(lFrom);
			continue;
		}
		//
		if (lTx->type() == TX_BUZZ || lTx->type() == TX_REBUZZ || lTx->type() == TX_BUZZ_REPLY) {
			//
			uint256 lTxPublisher = lTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer allways is the first in
			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lTxPublisher);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			//
			bool lProcessed = false;
			if (lTx->type() == TX_REBUZZ) {
				TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lTx);
				if (lRebuzz->simpleRebuzz()) {
					//
					makeBuzzfeedRebuzzItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, uint256());
					lProcessed = true;
				}
			}

			if (!lProcessed) {
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzzer]: try to add item ") +
					strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

				makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems);
			}
		} else if (lTx->type() == TX_BUZZ_LIKE) {
			//
			makeBuzzfeedLikeItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, uint256());
		} else if (lTx->type() == TX_BUZZ_REWARD) {
			//
			makeBuzzfeedRewardItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, uint256());
		} else if (lTx->type() == TX_BUZZER_MISTRUST) {
			//
			TxBuzzerMistrustPtr lEvent = TransactionHelper::to<TxBuzzerMistrust>(lTx);
			uint256 lMistruster = lEvent->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx(); 
			uint256 lBuzzerId = lTx->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 

			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);

			ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
			if (!lStore) continue;

			if (!lStore->extension()) continue;
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			TxBuzzerInfoPtr lInfo = lExtension->readBuzzerInfo(lBuzzerId);
			if (lInfo) {
				//
				BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
				makeBuzzfeedTrustScoreItem<TxBuzzerMistrust>(lItem, lEvent, lMistruster, lBuzzerId, lInfo);
				lRawBuzzfeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
				lBuzzItems.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));
			}
		} else if (lTx->type() == TX_BUZZER_ENDORSE) {
			//
			TxBuzzerEndorsePtr lEvent = TransactionHelper::to<TxBuzzerEndorse>(lTx);
			uint256 lEndorser = lEvent->in()[TX_BUZZER_ENDORSE_MY_IN].out().tx(); 
			uint256 lBuzzerId = lTx->in()[TX_BUZZER_ENDORSE_BUZZER_IN].out().tx(); 

			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);

			ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
			if (!lStore) continue;

			if (!lStore->extension()) continue;
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			TxBuzzerInfoPtr lInfo = lExtension->readBuzzerInfo(lBuzzerId);
			if (lInfo) {
				//
				BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
				makeBuzzfeedTrustScoreItem<TxBuzzerEndorse>(lItem, lEvent, lEndorser, lBuzzerId, lInfo);
				lRawBuzzfeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
				lBuzzItems.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));
			}
		}
	}

	lTransaction.commit();

	bool lContinue = false;
	std::set<BuzzfeedItem::Key> lAdded;
	for (std::multimap<uint64_t, BuzzfeedItem::Key>::iterator lItem = lRawBuzzfeed.begin(); lItem != lRawBuzzfeed.end(); lItem++) {
		//
		if (!lContinue && feed.size() < 30 /*max last mixed events*/ || lContinue && feed.size() < 100) {
			std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) {
				if (lAdded.insert(lItem->second).second) {
					feed.push_back(*(lBuzzItem->second));
					//
					if (lBuzzItem->second->type() == TX_BUZZ_REPLY) {
						lContinue = true;
					} else lContinue = false;
				}
			} else {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzzer]: item is MISSING ") +
					strprintf("%s/%s, %s#", lItem->second.id().toHex(), lItem->second.typeToString(), store_->chain().toHex().substr(0, 10)));
			}
		} else {
			break;
		}
	}

	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzzer]: ") +
		strprintf("items count = %d, %s#", feed.size(), store_->chain().toHex().substr(0, 10)));	
}

void BuzzerTransactionStoreExtension::selectBuzzfeedGlobal(uint64_t timeframeFrom, uint64_t scoreFrom, uint64_t timestampFrom, 
															const uint256& publisher, std::vector<BuzzfeedItem>& feed) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedGlobal]: selecting global buzzfeed for ") +
		strprintf("timeframe = %d, score = %d, timestamp = %d, publisher = %s, chain = %s#", 
			timeframeFrom, scoreFrom, timestampFrom, publisher.toHex(), store_->chain().toHex().substr(0, 10)));

	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());
	std::multimap<uint64_t, BuzzfeedItem::Key> lRawBuzzfeed;
	std::map<BuzzfeedItem::Key, BuzzfeedItemPtr> lBuzzItems;

	db::DbFourKeyContainer<
		uint64_t /*timeframe*/, 
		uint64_t /*score*/, 
		uint64_t /*timestamp*/, 
		uint256 /*publisher*/,
		uint256 /*buzz/reply/rebuzz/like/...*/>::Iterator lFrom;

	db::DbFourKeyContainer<
		uint64_t /*timeframe*/, 
		uint64_t /*score*/,
		uint64_t /*timestamp*/, 
		uint256 /*publisher*/, 
		uint256 /*buzz/reply/rebuzz/like/...*/>::Transaction lTransaction = globalTimeline_.transaction();

	if (!timeframeFrom){
		//
		lFrom = globalTimeline_.last();
	} else {
		//
		lFrom = globalTimeline_.find(timeframeFrom, scoreFrom, timestampFrom, publisher);
	}

	lFrom.setKey1Empty(); // reset key to iterate without limits

	for (;lBuzzItems.size() < 30 /*TODO: settings?*/ && lFrom.valid(); --lFrom) {
		//
		int lContext = 0;
		TxBuzzerPtr lBuzzer = nullptr;
		TransactionPtr lTx = store_->locateTransaction(*lFrom);
		if (!lTx) {
			lTransaction.remove(lFrom);
			continue;
		}

		//
		if (lTx->type() == TX_BUZZ) {
			//
			uint256 lTxPublisher = lTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer allways is the first in
			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lTxPublisher);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedGlobal]: try to added item ") +
				strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems);
		} else if (lTx->type() == TX_BUZZER_MISTRUST) {
			//
			TxBuzzerMistrustPtr lEvent = TransactionHelper::to<TxBuzzerMistrust>(lTx);
			uint256 lMistruster = lEvent->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx(); 
			uint256 lBuzzerId = lTx->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 

			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);

			ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
			if (!lStore) continue;

			if (!lStore->extension()) continue;
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			TxBuzzerInfoPtr lInfo = lExtension->readBuzzerInfo(lBuzzerId);
			if (lInfo) {
				//
				BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
				makeBuzzfeedTrustScoreItem<TxBuzzerMistrust>(lItem, lEvent, lMistruster, lBuzzerId, lInfo);
				lRawBuzzfeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
				lBuzzItems.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));
			}
		} else if (lTx->type() == TX_BUZZER_ENDORSE) {
			//
			TxBuzzerEndorsePtr lEvent = TransactionHelper::to<TxBuzzerEndorse>(lTx);
			uint256 lEndorser = lEvent->in()[TX_BUZZER_ENDORSE_MY_IN].out().tx(); 
			uint256 lBuzzerId = lTx->in()[TX_BUZZER_ENDORSE_BUZZER_IN].out().tx(); 

			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);

			ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
			if (!lStore) continue;

			if (!lStore->extension()) continue;
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			TxBuzzerInfoPtr lInfo = lExtension->readBuzzerInfo(lBuzzerId);
			if (lInfo) {
				//
				BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
				makeBuzzfeedTrustScoreItem<TxBuzzerEndorse>(lItem, lEvent, lEndorser, lBuzzerId, lInfo);
				lRawBuzzfeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
				lBuzzItems.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));
			}
		}
	}

	lTransaction.commit();

	for (std::multimap<uint64_t, BuzzfeedItem::Key>::reverse_iterator lItem = lRawBuzzfeed.rbegin(); lItem != lRawBuzzfeed.rend(); lItem++) {
		//
		if (feed.size() < 30 /*max last events*/) {
			std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) feed.push_back(*(lBuzzItem->second));
			else {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedGlobal]: item is MISSING ") +
					strprintf("%s/%s, %s#", lItem->second.id().toHex(), lItem->second.typeToString(), store_->chain().toHex().substr(0, 10)));
			}
		} else {
			break;
		}
	}

	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedGlobal]: ") +
		strprintf("items count = %d, %s#", feed.size(), store_->chain().toHex().substr(0, 10)));	
}

void BuzzerTransactionStoreExtension::selectBuzzfeedByTag(const std::string& tag, uint64_t timeframeFrom, 
									uint64_t scoreFrom, uint64_t timestampFrom, const uint256& publisher, std::vector<BuzzfeedItem>& feed) {
	//
	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());
	std::multimap<uint64_t, BuzzfeedItem::Key> lRawBuzzfeed;
	std::map<BuzzfeedItem::Key, BuzzfeedItemPtr> lBuzzItems;

	std::string lTag = boost::algorithm::to_lower_copy(tag);
	uint160 lHash = Hash160(lTag.begin(), lTag.end());

	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByTag]: selecting global buzzfeed for ") +
		strprintf("tag = %s, hash = %s, timeframe = %d, score = %d, timestamp = %d, publisher = %s, chain = %s#", 
			tag, lHash.toHex(), timeframeFrom, scoreFrom, timestampFrom, publisher.toHex(), store_->chain().toHex().substr(0, 10)));

	db::DbFiveKeyContainer<
		uint160 /*hash*/,
		uint64_t /*timeframe*/, 
		uint64_t /*score*/, 
		uint64_t /*timestamp*/,
		uint256 /*publisher*/, 
		uint256 /*buzz/reply/rebuzz/like/...*/>::Iterator lFrom;

	db::DbFiveKeyContainer<
		uint160 /*hash*/,
		uint64_t /*timeframe*/, 
		uint64_t /*score*/, 
		uint64_t /*timestamp*/,
		uint256 /*publisher*/, 
		uint256 /*buzz/reply/rebuzz/like/...*/>::Transaction lTransaction = 
												hashTagTimeline_.transaction();

	if (!timeframeFrom){
		//
		db::FiveKey<
			uint160 /*hash*/, 
			uint64_t /*timeframe*/, 
			uint64_t /*score*/, 
			uint64_t /*timestamp*/,
			uint256 /*publisher*/> lKey;

		if (hashTagUpdates_.read(lHash, lKey)) {
			lFrom = hashTagTimeline_.find(lHash, lKey.key2(), lKey.key3(), lKey.key4(), lKey.key5());
		} else {
			lFrom = hashTagTimeline_.find(lHash);
		}
	} else {
		//
		lFrom = hashTagTimeline_.find(lHash, timeframeFrom, scoreFrom, timestampFrom, publisher);
	}

	lFrom.setKey2Empty(); // reset key to iterate without limits

	for (;lBuzzItems.size() < 30 /*TODO: settings?*/ && lFrom.valid(); --lFrom) {
		//
		int lContext = 0;
		TxBuzzerPtr lBuzzer = nullptr;
		TransactionPtr lTx = store_->locateTransaction(*lFrom);
		if (!lTx) {
			lTransaction.remove(lFrom);
			continue;
		}

		//
		if (lTx->type() == TX_BUZZ) {
			//
			uint256 lTxPublisher = lTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer allways is the first in
			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lTxPublisher);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByTag]: try to added item ") +
				strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems);
		} 
	}

	lTransaction.commit();

	for (std::multimap<uint64_t, BuzzfeedItem::Key>::reverse_iterator lItem = lRawBuzzfeed.rbegin(); lItem != lRawBuzzfeed.rend(); lItem++) {
		//
		if (feed.size() < 30 /*max last events*/) {
			std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) feed.push_back(*(lBuzzItem->second));
		} else {
			break;
		}
	}

	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByTag]: ") +
		strprintf("items count = %d, %s#", feed.size(), store_->chain().toHex().substr(0, 10)));	
}

void BuzzerTransactionStoreExtension::selectHashTags(const std::string& tag, std::vector<Buzzer::HashTag>& feed) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectHashTags]: selecting hash tags for ") +
		strprintf("tag = %s, chain = %s#", tag, store_->chain().toHex().substr(0, 10)));
	//
	db::DbContainer<std::string, std::string>::Iterator lFrom = hashTags_.find(tag);
	for (int lCount = 0; lFrom.valid() && feed.size() < 6 && lCount < 100; ++lFrom, ++lCount) {
		std::string lTag;
		if (lFrom.first(lTag) && lTag.find(tag) != std::string::npos) {
			feed.push_back(Buzzer::HashTag(Hash160(lTag.begin(), lTag.end()), *lFrom));

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectHashTags]: add hash tag ") +
				strprintf("tag = %s, chain = %s#", *lFrom, store_->chain().toHex().substr(0, 10)));
		}
	}
}

void BuzzerTransactionStoreExtension::selectBuzzfeed(const std::vector<BuzzfeedPublisherFrom>& from, const uint256& subscriber, std::vector<BuzzfeedItem>& feed) {
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: selecting buzzfeed for ") +
		strprintf("subscriber = %s, from = %d, chain = %s#", subscriber.toHex(), from.size(), store_->chain().toHex().substr(0, 10)));

	// 0. get/set current time
	uint64_t lGlobalTimeFrom = qbit::getMicroseconds();

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

	// 1.2 expand "from"
	std::set <uint64_t> lLastFrom;
	std::map<uint256, uint64_t> lFromMap;
	for (std::vector<BuzzfeedPublisherFrom>::const_iterator lItemFrom = from.begin(); lItemFrom != from.end(); lItemFrom++) {
		//
		lFromMap[lItemFrom->publisher()] = lItemFrom->from();
		lLastFrom.insert(lItemFrom->from());
	}

	lGlobalTimeFrom = lLastFrom.size() ? *lLastFrom.rbegin() : lGlobalTimeFrom;

	// 1.3. iterate
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
	std::multimap<uint64_t, BuzzfeedItem::Key> lRawBuzzfeed;
	std::map<BuzzfeedItem::Key, BuzzfeedItemPtr> lBuzzItems;
	size_t lCurrentPublisher = 0;
	for (std::multimap<uint64_t, uint256>::reverse_iterator lPublisher = lPublishers.rbegin(); lPublisher != lPublishers.rend(); lPublisher++) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: processing publisher ") +
			strprintf("%s/%d", lPublisher->second.toHex(), lPublisher->first));

		// 2.1. detramine last timestamp
		std::map<uint256, uint64_t>::iterator lPublisherFrom = lFromMap.find(lPublisher->second);
		uint64_t lTimeFrom = (lPublisherFrom != lFromMap.end() ? lPublisherFrom->second : lGlobalTimeFrom);

		if (from.size() && lPublisherFrom == lFromMap.end())
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: publisher timestamp from NOT found for ") +
				strprintf("p = %s, chain = %s#", lPublisher->second.toHex(), store_->chain().toHex().substr(0, 10)));

		if (lTimeFrom < lPublisher->first) {	
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: publisher timestamp selecting from ") +
				strprintf("t = %d, pt = %d, pf = %d, gt = %d, p = %s, chain = %s#", lTimeFrom, lPublisherFrom->second, 
					lPublisher->first, lGlobalTimeFrom, lPublisher->second.toHex(), store_->chain().toHex().substr(0, 10)));
		}

		lTimeFrom = lTimeFrom < lPublisher->first ? lTimeFrom : lPublisher->first;

		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: publisher final timestamp selecting from ") +
			strprintf("p = %s, t = %d, chain = %s#", lPublisher->second.toHex(), lTimeFrom, store_->chain().toHex().substr(0, 10)));

		// 2.2 positioning
		db::DbTwoKeyContainer<uint256 /*publisher*/, uint64_t /*timestamp*/, uint256 /*tx*/>::Iterator lFrom = 
				timeline_.find(lPublisher->second, lTimeFrom);

		db::DbTwoKeyContainer<uint256 /*publisher*/, uint64_t /*timestamp*/, uint256 /*tx*/>::Transaction lTransaction = 
				timeline_.transaction();

		// 2.3. resetting timestamp to ensure appropriate backtracing
		lFrom.setKey2Empty();

		// 2.4. stepping down up to max 100 events for each publisher
		for (; (lBuzzItems.size() - lCurrentPublisher) < 100 /*TODO: settings?*/ && lFrom.valid(); --lFrom) {
			//
			int lContext = 0;
			TxBuzzerPtr lBuzzer = nullptr;
			TransactionPtr lTx = store_->locateTransaction(*lFrom);
			if (!lTx) { 
				lTransaction.remove(lFrom);
				continue;
			}

			//
			if (lTx->type() == TX_BUZZ || lTx->type() == TX_REBUZZ || lTx->type() == TX_BUZZ_REPLY) {
				//
				uint256 lTxPublisher = lTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer allways is the first in
				TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lTxPublisher);
				if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
				//
				bool lProcessed = false;
				if (lTx->type() == TX_REBUZZ) {
					TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lTx);
					if (lRebuzz->simpleRebuzz()) {
						//
						if (subscriber != lTxPublisher) 
							makeBuzzfeedRebuzzItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
						lProcessed = true;
					}
				}

				if (!lProcessed) {
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: try to add item ") +
						strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

					makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems);
				}
			} else if (lTx->type() == TX_BUZZ_LIKE) {
				//
				uint256 lTxPublisher = lTx->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
				if (subscriber != lTxPublisher) 
					makeBuzzfeedLikeItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
				//
				// TODO: if lTx->liked buzz is absent -> add to the output
			} else if (lTx->type() == TX_BUZZ_REWARD) {
				//
				uint256 lTxPublisher = lTx->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
				if (subscriber != lTxPublisher) 
					makeBuzzfeedRewardItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
				//
				// TODO: if lTx->liked buzz is absent -> add to the output
			} else if (lTx->type() == TX_BUZZER_MISTRUST) {
				//
				TxBuzzerMistrustPtr lEvent = TransactionHelper::to<TxBuzzerMistrust>(lTx);
				uint256 lMistruster = lEvent->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx(); 
				uint256 lBuzzerId = lTx->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 

				TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);

				ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
				if (!lStore) continue;

				if (!lStore->extension()) continue;
				BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

				TxBuzzerInfoPtr lInfo = lExtension->readBuzzerInfo(lBuzzerId);
				if (lInfo) {
					//
					BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
					makeBuzzfeedTrustScoreItem<TxBuzzerMistrust>(lItem, lEvent, lMistruster, lBuzzerId, lInfo);
					lRawBuzzfeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
					lBuzzItems.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));
				}
			} else if (lTx->type() == TX_BUZZER_ENDORSE) {
				//
				TxBuzzerEndorsePtr lEvent = TransactionHelper::to<TxBuzzerEndorse>(lTx);
				uint256 lEndorser = lEvent->in()[TX_BUZZER_ENDORSE_MY_IN].out().tx(); 
				uint256 lBuzzerId = lTx->in()[TX_BUZZER_ENDORSE_BUZZER_IN].out().tx(); 

				TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);

				ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
				if (!lStore) continue;

				if (!lStore->extension()) continue;
				BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

				TxBuzzerInfoPtr lInfo = lExtension->readBuzzerInfo(lBuzzerId);
				if (lInfo) {
					//
					BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
					makeBuzzfeedTrustScoreItem<TxBuzzerEndorse>(lItem, lEvent, lEndorser, lBuzzerId, lInfo);
					lRawBuzzfeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
					lBuzzItems.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));
				}
			}
		}

		// 2.4
		lTransaction.commit();

		// 2.5
		lCurrentPublisher = lBuzzItems.size();

	}

	// 3. fill reply
	bool lContinue = false;
	std::set<BuzzfeedItem::Key> lAdded;
	for (std::multimap<uint64_t, BuzzfeedItem::Key>::reverse_iterator lItem = lRawBuzzfeed.rbegin(); lItem != lRawBuzzfeed.rend(); lItem++) {
		//
		if (!lContinue && feed.size() < 100 /*max last mixed events*/ || lContinue && feed.size() < 500) {
			std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) {
				if (lAdded.insert(lItem->second).second) {
					feed.push_back(*(lBuzzItem->second));
					//
					if (lBuzzItem->second->type() == TX_BUZZ_REPLY) {
						lContinue = true;
					} else { 
						lContinue = false;
					}
				}
			} else {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: item is MISSING ") +
					strprintf("%s/%s, %s#", lItem->second.id().toHex(), lItem->second.typeToString(), store_->chain().toHex().substr(0, 10)));
			}
		} else {
			break;
		}
	}

	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: ") +
		strprintf("items count = %d, %s#", feed.size(), store_->chain().toHex().substr(0, 10)));	
}

void BuzzerTransactionStoreExtension::makeBuzzfeedLikeItem(TransactionPtr tx, ITransactionStorePtr mainStore, std::multimap<uint64_t, BuzzfeedItem::Key>& buzzFeed, std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>& buzzes, const uint256& subscriber) {
	//
	TxBuzzerPtr lBuzzer = nullptr;
	TransactionPtr lTx = tx;
	TxBuzzLikePtr lLike = TransactionHelper::to<TxBuzzLike>(lTx);
	//
	uint256 lPublisherId = lTx->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
	uint256 lBuzzId = lTx->in()[TX_BUZZ_LIKE_IN].out().tx(); // buzz_id
	//
	TxBuzzerPtr lPublisher;
	TransactionPtr lPublisherTx = mainStore->locateTransaction(lPublisherId);
	if (lPublisherTx) lPublisher = TransactionHelper::to<TxBuzzer>(lPublisherTx);

	// if buzz is already in the feed
	std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::iterator lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_BUZZ));
	if (lExisting == buzzes.end()) lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_REBUZZ));
	else if (lExisting == buzzes.end()) lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_BUZZ_REPLY));

	if (lExisting != buzzes.end()) return;

	// if we already have like
	lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_BUZZ_LIKE));	
	//
	TransactionPtr lBuzzTx = store_->locateTransaction(lBuzzId);

	// check direct subscription
	if (!subscriber.isNull()) {
		uint256 lBuzzerId = lBuzzTx->in()[TX_BUZZ_REPLY_MY_IN].out().tx();

		db::DbTwoKeyContainer<
			uint256 /*subscriber*/, 
			uint256 /*publisher*/, 
			uint256 /*tx*/>::Iterator lSubscription = subscriptionsIdx_.find(subscriber, lBuzzerId);
		if (lSubscription.valid()) {
			//
			uint256 lTxPublisher = lBuzzTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer allways is the first in
			TransactionPtr lBuzzerTx = mainStore->locateTransaction(lTxPublisher);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/makeBuzzfeedLikeItem]: try to add item ") +
				strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lBuzzTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			int lContext = 0;
			makeBuzzfeedItem(lContext, lBuzzer, lBuzzTx, mainStore, buzzFeed, buzzes);

			return;
		}
	}

	if (lBuzzTx && lPublisher) {

		if (lExisting != buzzes.end()) {
			//
			lExisting->second->addItemInfo(BuzzfeedItem::ItemInfo(
				lLike->timestamp(), lLike->score(), lPublisherId, lLike->buzzerInfoChain(), lLike->buzzerInfo(), lLike->chain(), lLike->id(),
				lLike->signature()
			));
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: liked item added ") +
				strprintf("by publisher = %s, %s/%s#", lPublisherId.toHex(), lLike->id().toHex(), store_->chain().toHex().substr(0, 10)));
		} else {
			TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lBuzzTx);
			uint256 lBuzzerId = lBuzz->in()[TX_BUZZ_REPLY_MY_IN].out().tx();
			//
			TransactionPtr lBuzzerTx = mainStore->locateTransaction(lBuzzerId);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);

			//
			BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
			prepareBuzzfeedItem(*lItem, lBuzz, lBuzzer);
			lItem->setType(TX_BUZZ_LIKE);
			lItem->setOrder(lLike->timestamp());
			lItem->addItemInfo(BuzzfeedItem::ItemInfo(
				lLike->timestamp(), lLike->score(), lPublisherId, lLike->buzzerInfoChain(), lLike->buzzerInfo(), lLike->chain(), lLike->id(),
				lLike->signature()
			));
			//
			buzzFeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
			buzzes.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: liked item added ") +
				strprintf("(%d/%d), buzzer = %s, %s/%s#", buzzes.size(), buzzFeed.size(), lBuzzer->id().toHex(), lBuzz->id().toHex(), store_->chain().toHex().substr(0, 10)));			
		}
	} else {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: buzz tx was not found ") +
			strprintf("%s/%s#", lBuzzId.toHex(), store_->chain().toHex().substr(0, 10)));
	}
}

void BuzzerTransactionStoreExtension::makeBuzzfeedRewardItem(TransactionPtr tx, ITransactionStorePtr mainStore, std::multimap<uint64_t, BuzzfeedItem::Key>& buzzFeed, std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>& buzzes, const uint256& subscriber) {
	//
	TxBuzzerPtr lBuzzer = nullptr;
	TransactionPtr lTx = tx;
	TxBuzzRewardPtr lReward = TransactionHelper::to<TxBuzzReward>(lTx);
	//
	uint256 lPublisherId = lTx->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
	uint256 lBuzzId = lTx->in()[TX_BUZZ_REWARD_IN].out().tx(); // buzz_id
	//
	TxBuzzerPtr lPublisher;
	TransactionPtr lPublisherTx = mainStore->locateTransaction(lPublisherId);
	if (lPublisherTx) lPublisher = TransactionHelper::to<TxBuzzer>(lPublisherTx);

	// if buzz is already in the feed
	std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::iterator lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_BUZZ));
	if (lExisting == buzzes.end()) lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_REBUZZ));
	else if (lExisting == buzzes.end()) lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_BUZZ_REPLY));

	if (lExisting != buzzes.end()) return;
	
	// if we already have reward
	lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_BUZZ_REWARD));
	//
	
	TransactionPtr lBuzzTx = store_->locateTransaction(lBuzzId);
	// check direct subscription
	if (!subscriber.isNull()) {
		uint256 lBuzzerId = lBuzzTx->in()[TX_BUZZ_REPLY_MY_IN].out().tx();

		db::DbTwoKeyContainer<
			uint256 /*subscriber*/, 
			uint256 /*publisher*/, 
			uint256 /*tx*/>::Iterator lSubscription = subscriptionsIdx_.find(subscriber, lBuzzerId);
		if (lSubscription.valid()) {
			//
			uint256 lTxPublisher = lBuzzTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer allways is the first in
			TransactionPtr lBuzzerTx = mainStore->locateTransaction(lTxPublisher);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/makeBuzzfeedRewardItem]: try to add item ") +
				strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lBuzzTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			int lContext = 0;
			makeBuzzfeedItem(lContext, lBuzzer, lBuzzTx, mainStore, buzzFeed, buzzes);

			return;
		}
	}

	if (lBuzzTx && lPublisher) {

		if (lExisting != buzzes.end()) {
			//
			lExisting->second->addItemInfo(BuzzfeedItem::ItemInfo(
				lReward->timestamp(), lReward->score(), lReward->amount(), 
				lPublisherId, lReward->buzzerInfoChain(), lReward->buzzerInfo(), lReward->chain(), lReward->id(),
				lReward->signature()
			));
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: rewarded item added ") +
				strprintf("by publisher = %s, %s/%s#", lPublisherId.toHex(), lReward->id().toHex(), store_->chain().toHex().substr(0, 10)));
		} else {
			TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lBuzzTx);
			uint256 lBuzzerId = lBuzz->in()[TX_BUZZ_REPLY_MY_IN].out().tx();
			//
			TransactionPtr lBuzzerTx = mainStore->locateTransaction(lBuzzerId);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);

			//
			BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
			prepareBuzzfeedItem(*lItem, lBuzz, lBuzzer);
			lItem->setType(TX_BUZZ_REWARD);
			lItem->setOrder(lReward->timestamp());
			lItem->addItemInfo(BuzzfeedItem::ItemInfo(
				lReward->timestamp(), lReward->score(), lReward->amount(), 
				lPublisherId, lReward->buzzerInfoChain(), lReward->buzzerInfo(), lReward->chain(), lReward->id(),
				lReward->signature()
			));
			//
			buzzFeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
			buzzes.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));
			
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: rewarded item added ") +
				strprintf("(%d/%d), buzzer = %s, %s/%s#", buzzes.size(), buzzFeed.size(), lBuzzer->id().toHex(), lBuzz->id().toHex(), store_->chain().toHex().substr(0, 10)));

		}
	} else {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: buzz tx was not found ") +
			strprintf("%s/%s#", lBuzzId.toHex(), store_->chain().toHex().substr(0, 10)));
	}
}

void BuzzerTransactionStoreExtension::makeBuzzfeedRebuzzItem(TransactionPtr tx, ITransactionStorePtr mainStore, std::multimap<uint64_t, BuzzfeedItem::Key>& buzzFeed, std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>& buzzes, const uint256& subscriber) {
	//
	TxBuzzerPtr lBuzzer = nullptr;
	TransactionPtr lTx = tx;
	//
	TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lTx);
	uint256 lBuzzId = lRebuzz->buzzId();
	uint256 lBuzzChainId = lRebuzz->buzzChainId();
	//
	uint256 lPublisherId = lTx->in()[TX_REBUZZ_MY_IN].out().tx(); // initiator
	//
	TxBuzzerPtr lPublisher;
	TransactionPtr lPublisherTx = mainStore->locateTransaction(lPublisherId);
	if (lPublisherTx) lPublisher = TransactionHelper::to<TxBuzzer>(lPublisherTx);

	// if buzz is already in the feed
	std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::iterator lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_BUZZ));
	if (lExisting == buzzes.end()) lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_REBUZZ));
	else if (lExisting == buzzes.end()) lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_BUZZ_REPLY));

	if (lExisting != buzzes.end()) return;
	
	// if we already have rebuzz
	lExisting = buzzes.find(BuzzfeedItem::Key(lBuzzId, TX_REBUZZ));
	//
	ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzChainId);
	if (!lStore) return;

	TransactionPtr lBuzzTx = lStore->locateTransaction(lBuzzId);
	// check direct subscription
	if (!subscriber.isNull()) {
		uint256 lBuzzerId = lBuzzTx->in()[TX_BUZZ_REPLY_MY_IN].out().tx();

		db::DbTwoKeyContainer<
			uint256 /*subscriber*/, 
			uint256 /*publisher*/, 
			uint256 /*tx*/>::Iterator lSubscription = subscriptionsIdx_.find(subscriber, lBuzzerId);
		if (lSubscription.valid()) {
			//
			uint256 lTxPublisher = lBuzzTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer allways is the first in
			TransactionPtr lBuzzerTx = mainStore->locateTransaction(lTxPublisher);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/makeBuzzfeedRebuzzItem]: try to add item ") +
				strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lBuzzTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			int lContext = 0;
			makeBuzzfeedItem(lContext, lBuzzer, lBuzzTx, mainStore, buzzFeed, buzzes);

			return;
		}
	}

	if (lBuzzTx && lPublisher) {

		if (lExisting != buzzes.end()) {
			//
			lExisting->second->addItemInfo(BuzzfeedItem::ItemInfo(
				lRebuzz->timestamp(), lRebuzz->score(), 
				lPublisherId, lRebuzz->buzzerInfoChain(), lRebuzz->buzzerInfo(), lRebuzz->chain(), lRebuzz->id(),
				lRebuzz->signature()
			));
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: rebuzzed item added ") +
				strprintf("by publisher = %s, %s/%s#", lPublisherId.toHex(), lRebuzz->id().toHex(), store_->chain().toHex().substr(0, 10)));
		} else {
			TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lBuzzTx);
			uint256 lBuzzerId = lBuzz->in()[TX_BUZZ_REPLY_MY_IN].out().tx();
			//
			TransactionPtr lBuzzerTx = mainStore->locateTransaction(lBuzzerId);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);

			//
			BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
			prepareBuzzfeedItem(*lItem, lBuzz, lBuzzer);
			lItem->setType(TX_REBUZZ);
			lItem->setOrder(lRebuzz->timestamp());
			lItem->addItemInfo(BuzzfeedItem::ItemInfo(
				lRebuzz->timestamp(), lRebuzz->score(),
				lPublisherId, lRebuzz->buzzerInfoChain(), lRebuzz->buzzerInfo(), lRebuzz->chain(), lRebuzz->id(),
				lRebuzz->signature()
			));
			//
			buzzFeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
			buzzes.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));
			
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: rebuzzed item added ") +
				strprintf("(%d/%d), buzzer = %s, %s/%s#", buzzes.size(), buzzFeed.size(), lBuzzer->id().toHex(), lBuzz->id().toHex(), store_->chain().toHex().substr(0, 10)));

		}
	} else {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: buzz tx was not found ") +
			strprintf("%s/%s#", lBuzzId.toHex(), store_->chain().toHex().substr(0, 10)));
	}
}

bool BuzzerTransactionStoreExtension::makeBuzzfeedItem(int& context, TxBuzzerPtr buzzer, TransactionPtr tx, ITransactionStorePtr mainStore, std::multimap<uint64_t, BuzzfeedItem::Key>& rawBuzzfeed, std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>& buzzes, bool expand) {
	//
	if (tx->type() == TX_BUZZ ||
		tx->type() == TX_REBUZZ ||
		tx->type() == TX_BUZZ_REPLY) {
		//
		TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(tx);

		// check likes
		std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::iterator lExistingItem = buzzes.find(BuzzfeedItem::Key(lBuzz->id(), TX_BUZZ_LIKE));
		if (lExistingItem != buzzes.end()) {
			//
			std::pair<std::multimap<uint64_t, BuzzfeedItem::Key>::iterator,
						std::multimap<uint64_t, BuzzfeedItem::Key>::iterator> lRange = rawBuzzfeed.equal_range(lExistingItem->second->actualOrder());
			for (std::multimap<uint64_t, BuzzfeedItem::Key>::iterator lTimestamp = lRange.first; lTimestamp != lRange.second; lTimestamp++) {
				if (lTimestamp->second == BuzzfeedItem::Key(lBuzz->id(), TX_BUZZ_LIKE)) {
					rawBuzzfeed.erase(lTimestamp);
					break;
				}
			}

			buzzes.erase(lExistingItem);
		}

		// check rewards
		lExistingItem = buzzes.find(BuzzfeedItem::Key(lBuzz->id(), TX_BUZZ_REWARD));
		if (lExistingItem != buzzes.end()) {
			std::pair<std::multimap<uint64_t, BuzzfeedItem::Key>::iterator,
						std::multimap<uint64_t, BuzzfeedItem::Key>::iterator> lRange = rawBuzzfeed.equal_range(lExistingItem->second->actualOrder());
			for (std::multimap<uint64_t, BuzzfeedItem::Key>::iterator lTimestamp = lRange.first; lTimestamp != lRange.second; lTimestamp++) {
				if (lTimestamp->second == BuzzfeedItem::Key(lBuzz->id(), TX_BUZZ_REWARD)) {
					rawBuzzfeed.erase(lTimestamp);
					break;
				}
			}

			buzzes.erase(lExistingItem);
		}

		// check rebuzzes
		lExistingItem = buzzes.find(BuzzfeedItem::Key(lBuzz->id(), TX_REBUZZ));
		if (lExistingItem != buzzes.end()) {
			std::pair<std::multimap<uint64_t, BuzzfeedItem::Key>::iterator,
						std::multimap<uint64_t, BuzzfeedItem::Key>::iterator> lRange = rawBuzzfeed.equal_range(lExistingItem->second->actualOrder());
			for (std::multimap<uint64_t, BuzzfeedItem::Key>::iterator lTimestamp = lRange.first; lTimestamp != lRange.second; lTimestamp++) {
				if (lTimestamp->second == BuzzfeedItem::Key(lBuzz->id(), TX_REBUZZ)) {
					rawBuzzfeed.erase(lTimestamp);
					break;
				}
			}

			buzzes.erase(lExistingItem);
		}

		// sanity
		if (context > 100) return false;

		//
		BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
		prepareBuzzfeedItem(*lItem, lBuzz, buzzer);

		if (tx->type() == TX_BUZZ_REPLY) {
			//
			uint256 lOriginalId = lBuzz->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
			uint256 lOriginalChainId = lBuzz->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
			lItem->setOriginalBuzzId(lOriginalId);
			lItem->setOriginalBuzzChainId(lOriginalChainId);
			//
			if (expand) {
				// load original tx
				// NOTE: should be in the same chain
				TransactionPtr lOriginalTx = store_->locateTransaction(lOriginalId);

				TxBuzzerPtr lBuzzer = nullptr;
				uint256 lBuzzerId = lOriginalTx->in()[TX_BUZZ_REPLY_MY_IN].out().tx();
				TransactionPtr lBuzzerTx = mainStore->locateTransaction(lBuzzerId);
				if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);

				// add buzz
				makeBuzzfeedItem(++context, lBuzzer, lOriginalTx, mainStore, rawBuzzfeed, buzzes);
			}
		} else 	if (tx->type() == TX_REBUZZ) {
			// NOTE: source buzz can be in different chain, so just add info for postponed loading
			TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lBuzz);
			lItem->setOriginalBuzzId(lRebuzz->buzzId());
			lItem->setOriginalBuzzChainId(lRebuzz->buzzChainId());
		}

		rawBuzzfeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
		buzzes.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));

		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: item added ") +
			strprintf("(%d/%d), buzzer = %s, %d/%s/%s#", buzzes.size(), rawBuzzfeed.size(), buzzer->id().toHex(), lBuzz->timestamp(), lBuzz->id().toHex(), store_->chain().toHex().substr(0, 10)));

		return true;
	}

	return false;
}

void BuzzerTransactionStoreExtension::prepareBuzzfeedItem(BuzzfeedItem& item, TxBuzzPtr buzz, TxBuzzerPtr buzzer) {
	//
	item.setType(buzz->type());
	item.setBuzzId(buzz->id());
	item.setBuzzerInfoId(buzz->buzzerInfo());
	item.setBuzzerInfoChainId(buzz->buzzerInfoChain());
	item.setBuzzChainId(buzz->chain());
	item.setTimestamp(buzz->timestamp());
	item.setScore(buzz->score());
	item.setBuzzBody(buzz->body());
	item.setBuzzMedia(buzz->mediaPointers());
	item.setSignature(buzz->signature());

	if (buzzer) {
		item.setBuzzerId(buzzer->id());
	}

	// TODO:
	// TxEventInfo loaded by buzzer_id or using "in"?
	// and fill setEventInfoId()

	// read info
	BuzzInfo lInfo;
	if (buzzInfo_.read(buzz->id(), lInfo)) {
		item.setReplies(lInfo.replies_);
		item.setRebuzzes(lInfo.rebuzzes_);
		item.setLikes(lInfo.likes_);
		item.setRewards(lInfo.rewards_);
	}
}
