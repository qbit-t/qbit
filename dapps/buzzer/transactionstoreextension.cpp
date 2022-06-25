#include "transactionstoreextension.h"
#include "../../log/log.h"
#include "../../mkpath.h"
#include "txbuzzer.h"
#include "txbuzz.h"
#include "txrebuzz.h"
#include "txrebuzznotify.h"
#include "txbuzzreply.h"
#include "txbuzzlike.h"
#include "txbuzzhide.h"
#include "txbuzzreward.h"
#include "txbuzzersubscribe.h"
#include "txbuzzerunsubscribe.h"
#include "txbuzzerendorse.h"
#include "txbuzzermistrust.h"
#include "txbuzzerconversation.h"
#include "txbuzzeracceptconversation.h"
#include "txbuzzerdeclineconversation.h"
#include "txbuzzermessage.h"
#include "txbuzzermessagereply.h"

#include <iostream>

using namespace qbit;

bool BuzzerTransactionStoreExtension::open() {
	//
	if (!opened_) { 
		try {
			gLog().write(Log::INFO, std::string("[extension/open]: opening storage extension for ") + strprintf("%s", store_->chain().toHex()));

			if (mkpath(std::string(settings_->dataPath() + "/" + store_->chain().toHex() + "/buzzer/indexes").c_str(), 0777)) return false;

			timeline_.open();
			conversations_.open();
			conversationsOrder_.open();
			conversationsIndex_.open();
			conversationsActivity_.open();
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
			buzzerInfo_.open();
			endorsements_.open();
			mistrusts_.open();
			hiddenIdx_.open();

			opened_ = true;
		}
		catch(const std::exception& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[extension/open/error]: ") + ex.what());
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
	conversations_.close();
	conversationsOrder_.close();
	conversationsIndex_.close();
	conversationsActivity_.close();
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
	buzzerInfo_.close();
	buzzerStat_.close();
	endorsements_.close();
	mistrusts_.close();
	hiddenIdx_.close();

	opened_ = false;

	return true;
}

bool BuzzerTransactionStoreExtension::selectBuzzerEndorseTx(const uint256& actor, const uint256& buzzer, uint256& tx) {
	//
	if (!opened_) return false;
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
	if (!opened_) return false;
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
	if (!opened_) return false;
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

		// TODO: consider to remove this check
		// for now: if event ts and current ts differs more than 2 steps
		if (lScore.score() < lEvent->score() && (lEvent->score() - lScore.score()) > 20000000) {
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

			//
			if (ctx->tx()->type() == TX_REBUZZ) {
				//
				TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(ctx->tx());
				// re-buzz index
				db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*rebuzzer*/, uint256 /*rebuzz_tx*/>::Iterator 
					lRebuzzIdx = rebuzzesIdx_.find(lRebuzz->buzzId(), lPublisher);

				if (lRebuzzIdx.valid())
					return false;
			}

		} else if (ctx->tx()->type() == TX_BUZZ_REPLY) {
			// get publisher
			uint256 lReplyPublisher = lEvent->in()[TX_BUZZ_REPLY_MY_IN].out().tx();
			//  process sub-tree
			uint256 lReTxId = lEvent->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
			uint256 lReChainId = lEvent->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
			// NOTICE: depth is only for performance reasons
			int lCount = 0;
			while (lCount++ < 100) {
				//
				ITransactionStorePtr lLocalStore = store_->storeManager()->locate(lReChainId);
				if (!lLocalStore) break;

				TransactionPtr lReTx = lLocalStore->locateTransaction(lReTxId);
				if (lReTx && (lReTx->type() == TX_BUZZ_REPLY || lReTx->type() == TX_REBUZZ ||
																	lReTx->type() == TX_BUZZ)) {
					// check mistrusts
					uint256 lOriginalPublisher = lReTx->in()[TX_BUZZ_REPLY_MY_IN /*for the ALL types above*/].out().tx();
					// check for personal top buzz owner trust
					uint256 lTrustTx;
					if ((lReTx->type() == TX_BUZZ || lReTx->type() == TX_REBUZZ) &&
							mistrusts_.read(qbit::db::TwoKey<uint256, uint256>(lReplyPublisher, lOriginalPublisher), lTrustTx)) {
						// re-ceck if we have compensation
						if (!endorsements_.read(qbit::db::TwoKey<uint256, uint256>(lReplyPublisher, lOriginalPublisher), lTrustTx))
							return false; // mistrusted and not able to make replies
					}

					//
					if (lReTx->type() == TX_BUZZ || lReTx->type() == TX_REBUZZ) {
						// we reach top
						break;
					}

					if (lReTx->in().size() > TX_BUZZ_REPLY_BUZZ_IN) { // move up-next
						lReTxId = lReTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
						lReChainId = lReTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
					}
				} else break;
			}

		} else if (ctx->tx()->type() == TX_BUZZ_REBUZZ_NOTIFY) {
			//
			TxReBuzzNotifyPtr lRebuzzNotify = TransactionHelper::to<TxReBuzzNotify>(ctx->tx());
			// re-buzz index
			db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*rebuzzer*/, uint256 /*rebuzz_tx*/>::Iterator 
				lRebuzzIdx = rebuzzesIdx_.find(lRebuzzNotify->buzzId(), lPublisher);

			if (lRebuzzIdx.valid())
				return false;
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

			ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());
			if (lMainStore && lEvent->in().size() > TX_BUZZER_ENDORSE_FEE_IN) {
				// try commited
				TransactionPtr lFeeTx = lMainStore->locateTransaction(lEvent->in()[TX_BUZZER_ENDORSE_FEE_IN].out().tx());
				// try mempool
				if (lFeeTx == nullptr) lFeeTx = lMainStore->locateMempoolTransaction(lEvent->in()[TX_BUZZER_ENDORSE_FEE_IN].out().tx());
				// main and primary case
				if (lFeeTx) {
					TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

					amount_t lAmount;
					uint64_t lHeight;
					uint256 lAsset;
					if (TxBuzzerEndorse::extractLockedAmountHeightAndAsset(lFee, lAmount, lHeight, lAsset) &&
							!(lAmount == settings_->oneVoteProofAmount() && lAsset == settings_->proofAsset())) {
						// if amount is not fixed - for NOW
						// maybe later we'll implement schedule
						return false;
					}
				} else { // anyway we SHOULD try to check
					//
					TxBuzzerEndorsePtr lEndorse = TransactionHelper::to<TxBuzzerEndorse>(lEvent);
					if (!(lEndorse->amount() == BUZZER_MIN_EM_STEP && 
							settings_->proofAsset() == lEvent->in()[TX_BUZZER_ENDORSE_FEE_IN].out().asset())) {
						// if amount is not fixed - for NOW
						// maybe later we'll implement schedule
						return false;
					}
				}
			} else return false;

		} else if (ctx->tx()->type() == TX_BUZZER_MISTRUST) {
			//
			// in[0] - publisher/initiator
			uint256 lBuzzer = lEvent->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx(); 
			// in[1] - publisher/initiator
			uint256 lMistruster = lEvent->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 
			//
			db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/>::Iterator 
															lMistrust = mistrusts_.find(lBuzzer, lMistruster);
			if (lMistrust.valid()) {
				//
				return false; // already mistrusted
			}

			ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());
			if (lMainStore && lEvent->in().size() > TX_BUZZER_MISTRUST_FEE_IN) {
				// try commited
				TransactionPtr lFeeTx = lMainStore->locateTransaction(lEvent->in()[TX_BUZZER_MISTRUST_FEE_IN].out().tx());
				// try mempool
				if (lFeeTx == nullptr) lFeeTx = lMainStore->locateMempoolTransaction(lEvent->in()[TX_BUZZER_MISTRUST_FEE_IN].out().tx());
				// main and primary case
				if (lFeeTx) {
					TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

					amount_t lAmount;
					uint64_t lHeight;
					uint256 lAsset;
					if (TxBuzzerMistrust::extractLockedAmountHeightAndAsset(lFee, lAmount, lHeight, lAsset) &&
							!(lAmount == settings_->oneVoteProofAmount() && lAsset == settings_->proofAsset())) {
						// if amount is not fixed - for NOW
						// maybe later we'll implement schedule
						return false;
					}
				} else {
					//
					TxBuzzerMistrustPtr lMistrust = TransactionHelper::to<TxBuzzerMistrust>(lEvent);
					if (!(lMistrust->amount() == BUZZER_MIN_EM_STEP && 
							settings_->proofAsset() == lMistrust->in()[TX_BUZZER_MISTRUST_FEE_IN].out().asset())) {
						// if amount is not fixed - for NOW
						// maybe later we'll implement schedule
						return false;
					}
				}
			} else return false;

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
	} else if (ctx->tx()->type() == TX_BUZZER_MESSAGE) {
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		//
		// in[1] - conversation
		uint256 lConversation = lEvent->in()[TX_BUZZER_CONVERSATION_IN].out().tx();
		uint256 lChain = lEvent->in()[TX_BUZZER_CONVERSATION_IN].out().chain();

		//
		ITransactionStorePtr lStore = store_->storeManager()->locate(lChain);
		if (lStore) {
			TransactionPtr lTx = lStore->locateTransaction(lConversation);
			if (lTx) {
				//
				TxBuzzerConversationPtr lConversationTx = TransactionHelper::to<TxBuzzerConversation>(lTx);
				uint256 lInitiator = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx(); 
				uint256 lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx(); 

				db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*conversation*/, ConversationInfo /*state*/>::Iterator 
													lConversationItem = conversations_.find(lCounterparty, lConversation);

				if (lConversationItem.valid()) {
					ConversationInfo lInfo = *lConversationItem;
					if (lInfo.state() == ConversationInfo::DECLINED)
						return false;
				}
			}
		}
	} else if (ctx->tx()->type() == TX_BUZZER_CONVERSATION) {
		//
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		uint256 lInitiator = lEvent->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx(); 
		uint256 lCounterparty = lEvent->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx(); 

		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*conversation*/, ConversationInfo /*state*/>::Iterator 
											lConversationItem = conversations_.find(lInitiator, lCounterparty);
		if (lConversationItem.valid()) {
			return false;
		}

		lConversationItem = conversations_.find(lCounterparty, lInitiator);
		if (lConversationItem.valid()) {
			return false;
		}
	} else if (ctx->tx()->type() == TX_BUZZER_ACCEPT_CONVERSATION || ctx->tx()->type() == TX_BUZZER_DECLINE_CONVERSATION) {
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		// in[0]
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_MY_IN].out().tx();
		// in[1] - conversation
		uint256 lConversation = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().tx();
		uint256 lChain = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().chain();

		//
		ITransactionStorePtr lStore = store_->storeManager()->locate(lChain);
		if (lStore) {
			TransactionPtr lTx = lStore->locateTransaction(lConversation);
			if (lTx) {
				//
				uint256 lCounterparty = lTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx(); 
				if (lBuzzer != lCounterparty) return false;
			}
		}
	} else if (ctx->tx()->type() == TX_BUZZER) {
		//
#ifdef PRODUCTION_MOD
		TxBuzzerPtr lBuzzer = TransactionHelper::to<TxBuzzer>(ctx->tx());
		std::string lName = lBuzzer->myName();

		// tolower
		std::transform(lName.begin(), lName.end(), lName.begin(), ::tolower);

		//
		if (lName.find("@buz" ) != std::string::npos ||
			lName.find("buzz" ) != std::string::npos ||
			lName.find("buzer") != std::string::npos ||
			lName.find("bazer") != std::string::npos ||
			lName.find("bazze") != std::string::npos || 
			// lName.find("baze" ) != std::string::npos ||

			lName.find("admin"  ) != std::string::npos ||
			lName.find("founder") != std::string::npos ||
			lName.find("owner"  ) != std::string::npos ||
			lName.find("member" ) != std::string::npos)
			return false;
#endif
	} else if (ctx->tx()->type() == TX_BUZZ_HIDE) {
		//
		TxBuzzHidePtr lBuzzHide = TransactionHelper::to<TxBuzzHide>(ctx->tx());
		//
		if (lBuzzHide->in().size() > TX_BUZZ_HIDE_IN) {
			// extract buzz | buzzer
			uint256 lBuzzId = lBuzzHide->in()[TX_BUZZ_HIDE_IN].out().tx(); // buzz_id
			uint256 lBuzzerId = lBuzzHide->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // owner
			
			//
			uint256 lOwner;
			if (hiddenIdx_.read(lBuzzId, lOwner)) return false;
		}
	} 

	return true;
}

bool BuzzerTransactionStoreExtension::locateParents(TransactionContextPtr ctx, std::list<uint256>& parents) {
	//
	if (!opened_) return false;
	//
	if (ctx->tx()->type() == TX_BUZZ_REPLY) {
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		//
		uint256 lBuzzerId = lEvent->in()[TX_BUZZ_REPLY_MY_IN].out().tx();
		uint256 lTxId = lEvent->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
		uint256 lChainId = lEvent->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
		//
		size_t lDeep = 0;
		while (lDeep++ < 100) {
			//
			ITransactionStorePtr lLocalStore = store_->storeManager()->locate(lChainId);
			if (!lLocalStore) break;
			//
			TransactionPtr lTx = lLocalStore->locateTransaction(lTxId);
			if (lTx && (lTx->type() == TX_BUZZ_REPLY || lTx->type() == TX_REBUZZ ||
																lTx->type() == TX_BUZZ)) {

				//
				parents.push_back(lTxId);
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/locateParents]: adding parent ") +
					strprintf("%s for %s/%s#", lTxId.toHex(), lEvent->id().toHex(), lChainId.toHex().substr(0, 10)));

				if (lTx->type() == TX_BUZZ || lTx->type() == TX_REBUZZ) {
					break;
				}

				if (lTx->in().size() > TX_BUZZ_REPLY_BUZZ_IN) { // move up-next
					lTxId = lTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
					lChainId = lTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
				}
			} else break;
		}
		// for events
		parents.push_back(lBuzzerId);
	} else if (ctx->tx()->type() == TX_BUZZER_MESSAGE) {
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		//
		uint256 lConversationId = lEvent->in()[TX_BUZZER_CONVERSATION_IN].out().tx();
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_MY_IN].out().tx(); 
		uint256 lChain = lEvent->in()[TX_BUZZER_CONVERSATION_IN].out().chain();

		//
		ITransactionStorePtr lStore = store_->storeManager()->locate(lChain);
		if (lStore) {
			TransactionPtr lTx = lStore->locateTransaction(lConversationId);
			if (lTx) {
				//
				TxBuzzerConversationPtr lConversationTx = TransactionHelper::to<TxBuzzerConversation>(lTx);
				uint256 lInitiator = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx(); 
				uint256 lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx();

				if (lBuzzer == lInitiator) {
					parents.push_back(lCounterparty); // events
				} else {
					parents.push_back(lInitiator); // events
				}				
			}
		}

		// conversation
		parents.push_back(lConversationId);
	} else if (ctx->tx()->type() == TX_REBUZZ) {
		//
		TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(ctx->tx());
		parents.push_back(lRebuzz->buzzerId());
	} else if (ctx->tx()->type() == TX_BUZZ_REBUZZ_NOTIFY) {
		//
		TxReBuzzNotifyPtr lRebuzzNotify = TransactionHelper::to<TxReBuzzNotify>(ctx->tx());
		parents.push_back(lRebuzzNotify->buzzerId());
	} else if (ctx->tx()->type() == TX_BUZZER_CONVERSATION) {
		//
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		uint256 lOriginator = lEvent->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx();
		parents.push_back(lBuzzer);
	} else if (ctx->tx()->type() == TX_BUZZER_ACCEPT_CONVERSATION) {
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());

		//
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_MY_IN].out().tx(); 
		uint256 lConversationId = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().tx();
		uint256 lConversationChain = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().chain();

		// counterparty
		uint256 lCounterparty;

		// make event
		ITransactionStorePtr lStore = store_->storeManager()->locate(lConversationChain);
		if (lStore) {
			TransactionPtr lConversationTx = lStore->locateTransaction(lConversationId);
			if (lConversationTx) {
				//
				lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
				parents.push_back(lCounterparty);
			}
		}
	} else if (ctx->tx()->type() == TX_BUZZER_DECLINE_CONVERSATION) {
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		//
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_CONVERSATION_DECLINE_MY_IN].out().tx(); 
		uint256 lConversationId = lEvent->in()[TX_BUZZER_CONVERSATION_DECLINE_IN].out().tx();
		uint256 lConversationChain = lEvent->in()[TX_BUZZER_CONVERSATION_DECLINE_IN].out().chain();

		// counterparty
		uint256 lCounterparty;

		// make event
		ITransactionStorePtr lStore = store_->storeManager()->locate(lConversationChain);
		if (lStore) {
			TransactionPtr lConversationTx = lStore->locateTransaction(lConversationId);
			if (lConversationTx) {
				//
				lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
				parents.push_back(lCounterparty);
			}
		}
	} else if (ctx->tx()->type() == TX_BUZZER_ENDORSE) {
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		//
		uint256 lEndoser = lEvent->in()[TX_BUZZER_ENDORSE_MY_IN].out().tx(); 
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_ENDORSE_BUZZER_IN].out().tx(); 
		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/>::Iterator 
														lEndorsement = endorsements_.find(lBuzzer, lEndoser);
		if (!lEndorsement.valid()) {
			parents.push_back(lBuzzer);
		}
	} else if (ctx->tx()->type() == TX_BUZZER_MISTRUST) {
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		//
		uint256 lMistruster = lEvent->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx(); 
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 
		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*endoser*/, uint256 /*tx*/>::Iterator 
														lMistrust = mistrusts_.find(lBuzzer, lMistruster);
		if (!lMistrust.valid()) {
			parents.push_back(lBuzzer);
		}
	} else if (ctx->tx()->type() == TX_BUZZER_SUBSCRIBE) {
		//
		TxBuzzerSubscribePtr lBuzzSubscribe = TransactionHelper::to<TxBuzzerSubscribe>(ctx->tx());
		if (lBuzzSubscribe->in().size() >= 2) {
			//
			uint256 lPublisher = lBuzzSubscribe->in()[0].out().tx(); // in[0] - publisher
			parents.push_back(lPublisher);
		}
	}

	// that should be tx_event object
	TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
	// extract buzzers
	std::vector<Transaction::In>& lIns = lEvent->in(); 
	std::vector<Transaction::In>::iterator lInPtr = lIns.begin(); lInPtr++; // buzzer pointer
	for(; lInPtr != lIns.end(); lInPtr++) {
		//
		Transaction::In& lIn = (*lInPtr);
		ITransactionStorePtr lLocalStore = store_->storeManager()->locate(lIn.out().chain());
		if (!lLocalStore) continue;

		TransactionPtr lInTx = lLocalStore->locateTransaction(lIn.out().tx());
		if (lInTx != nullptr) {
			if (lInTx->type() == TX_BUZZER) {
				// direct events - mentions
				parents.push_back(lInTx->id());
			} else if ((lInTx->type() == TX_BUZZ || lInTx->type() == TX_REBUZZ || lInTx->type() == TX_BUZZ_REPLY) &&
																			lInTx->in().size() > TX_BUZZ_REPLY_MY_IN) {
				// re-buzz, reply
				parents.push_back(lInTx->in()[TX_BUZZ_REPLY_MY_IN].out().tx());
			}
		}
	}

	if (parents.size() > 0) return true;
	return false;
}

bool BuzzerTransactionStoreExtension::pushEntity(const uint256& id, TransactionContextPtr ctx) {
	//
	if (!opened_) return false;
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
	} else if (ctx->tx()->type() == TX_BUZZER_CONVERSATION) {
		processConversation(id, ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_ACCEPT_CONVERSATION) {
		processAcceptConversation(id, ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_DECLINE_CONVERSATION) {
		processDeclineConversation(id, ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_MESSAGE) {
		processMessage(id, ctx);
	} else if (ctx->tx()->type() == TX_BUZZ_HIDE) {
		processHide(id, ctx);
	}

	//
	// TODO: qbit value transactions - make events (receive transactions)
	//

	return true;
}

void BuzzerTransactionStoreExtension::processConversation(const uint256& id, TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_CONVERSATION) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing conversation ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());

		// in[0] - originator
		uint256 lOriginator = lEvent->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx(); 
		// in[1] - counterparty
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx();

		//
		bool lAdded = false;
		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*conversation*/, ConversationInfo /*state*/>::Iterator 
											lOriginatorConversation = conversations_.find(lOriginator, lEvent->id());
		if (!lOriginatorConversation.valid()) {
			conversations_.write(lOriginator, lEvent->id(), ConversationInfo(lEvent->id(), lEvent->chain(), ConversationInfo::PENDING));
			conversationsIndex_.write(lOriginator, lEvent->id(), lEvent->timestamp());
			conversationsOrder_.write(lOriginator, lEvent->timestamp(), lEvent->id());
			conversationsActivity_.write(lOriginator, lEvent->timestamp());

			lAdded = true;

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: PUSHED conversation for originator ") +
				strprintf("%s/%s/%s#", lOriginator.toHex(), ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
		}

		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*conversation*/, ConversationInfo /*state*/>::Iterator 
											lBuzzerConversation = conversations_.find(lBuzzer, lEvent->id());
		if (!lBuzzerConversation.valid()) {
			conversations_.write(lBuzzer, lEvent->id(), ConversationInfo(lEvent->id(), lEvent->chain(), ConversationInfo::PENDING));
			conversationsIndex_.write(lBuzzer, lEvent->id(), lEvent->timestamp());
			conversationsOrder_.write(lBuzzer, lEvent->timestamp(), lEvent->id());
			conversationsActivity_.write(lBuzzer, lEvent->timestamp());

			lAdded = true;
			events_.write(lBuzzer, lEvent->timestamp(), lEvent->id(), lEvent->type());
			subscriberUpdates(lBuzzer, lEvent->timestamp());

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: PUSHED conversation for buzzer ") +
				strprintf("%s/%s/%s#", lBuzzer.toHex(), ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
		}

		if (lAdded) conversations_.write(lOriginator, lBuzzer, ConversationInfo(lEvent->id(), lEvent->chain(), ConversationInfo::PENDING));
	}
}

void BuzzerTransactionStoreExtension::processAcceptConversation(const uint256& id, TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_ACCEPT_CONVERSATION) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: accepting conversation ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());

		// in[0] - buzzer
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_MY_IN].out().tx(); 
		// in[1] - conversation
		uint256 lConversationId = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().tx();
		uint256 lConversationChain = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().chain();

		// counterparty
		uint256 lCounterparty;

		// make event
		ITransactionStorePtr lStore = store_->storeManager()->locate(lConversationChain);
		if (lStore) {
			TransactionPtr lConversationTx = lStore->locateTransaction(lConversationId);
			if (lConversationTx) {
				//
				lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
				events_.write(lCounterparty, lEvent->timestamp(), lEvent->id(), lEvent->type());
				subscriberUpdates(lCounterparty, lEvent->timestamp());
			}
		}

		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*conversation*/, ConversationInfo /*state*/>::Iterator 
											lConversation = conversations_.find(lBuzzer, lConversationId);
		if (lConversation.valid()) {
			ConversationInfo lInfo = *lConversation;
			lInfo.setState(lEvent->id(), lEvent->chain(), ConversationInfo::ACCEPTED);
			conversations_.write(lBuzzer, lConversationId, lInfo);
			conversationsIndex_.write(lBuzzer, lEvent->id(), lEvent->timestamp());
			conversationsOrder_.write(lBuzzer, lEvent->timestamp(), lEvent->id());
			conversationsActivity_.write(lBuzzer, lEvent->timestamp());
		}

		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*conversation*/, ConversationInfo /*state*/>::Iterator 
											lCounterpartyConversation = conversations_.find(lCounterparty, lConversationId);
		if (lCounterpartyConversation.valid()) {
			ConversationInfo lInfo = *lCounterpartyConversation;
			lInfo.setState(lEvent->id(), lEvent->chain(), ConversationInfo::ACCEPTED);
			conversations_.write(lCounterparty, lConversationId, lInfo);
			conversationsIndex_.write(lCounterparty, lEvent->id(), lEvent->timestamp());
			conversationsOrder_.write(lCounterparty, lEvent->timestamp(), lEvent->id());
			conversationsActivity_.write(lCounterparty, lEvent->timestamp());
		}

		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: ACCEPTED conversation for buzzer ") +
			strprintf("%s/%s/%s#", lBuzzer.toHex(), ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
	}
}

void BuzzerTransactionStoreExtension::processDeclineConversation(const uint256& id, TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_DECLINE_CONVERSATION) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: declining conversation ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());

		// in[0] - buzzer
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_CONVERSATION_DECLINE_MY_IN].out().tx(); 
		// in[1] - conversation
		uint256 lConversationId = lEvent->in()[TX_BUZZER_CONVERSATION_DECLINE_IN].out().tx();
		uint256 lConversationChain = lEvent->in()[TX_BUZZER_CONVERSATION_DECLINE_IN].out().chain();

		// counterparty
		uint256 lCounterparty;

		// make event
		ITransactionStorePtr lStore = store_->storeManager()->locate(lConversationChain);
		if (lStore) {
			TransactionPtr lConversationTx = lStore->locateTransaction(lConversationId);
			if (lConversationTx) {
				//
				lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
				events_.write(lCounterparty, lEvent->timestamp(), lEvent->id(), lEvent->type());
				subscriberUpdates(lCounterparty, lEvent->timestamp());
			}
		}

		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*conversation*/, ConversationInfo /*state*/>::Iterator 
											lConversation = conversations_.find(lBuzzer, lConversationId);
		if (lConversation.valid()) {
			ConversationInfo lInfo = *lConversation;
			lInfo.setState(lEvent->id(), lEvent->chain(), ConversationInfo::DECLINED);
			conversations_.write(lBuzzer, lConversationId, lInfo);
			conversationsIndex_.write(lBuzzer, lEvent->id(), lEvent->timestamp());
			conversationsOrder_.write(lBuzzer, lEvent->timestamp(), lEvent->id());
			conversationsActivity_.write(lBuzzer, lEvent->timestamp());
		}

		//
		db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*conversation*/, ConversationInfo /*state*/>::Iterator 
											lCounterpartyConversation = conversations_.find(lCounterparty, lConversationId);
		if (lCounterpartyConversation.valid()) {
			ConversationInfo lInfo = *lCounterpartyConversation;
			lInfo.setState(lEvent->id(), lEvent->chain(), ConversationInfo::DECLINED);
			conversations_.write(lCounterparty, lConversationId, lInfo);
			conversationsIndex_.write(lCounterparty, lEvent->id(), lEvent->timestamp());
			conversationsOrder_.write(lCounterparty, lEvent->timestamp(), lEvent->id());
			conversationsActivity_.write(lCounterparty, lEvent->timestamp());
		}

		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: DECLINED conversation for buzzer ") +
			strprintf("%s/%s/%s#", lBuzzer.toHex(), ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
	}
}

void BuzzerTransactionStoreExtension::processMessage(const uint256& id, TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_MESSAGE) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing message ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(ctx->tx());
		// in[0] - buzzer
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_MY_IN].out().tx(); 
		// in[1] - conversation
		uint256 lConversation = lEvent->in()[TX_BUZZER_CONVERSATION_IN].out().tx();
		uint256 lChain = lEvent->in()[TX_BUZZER_CONVERSATION_IN].out().chain();

		//
		ITransactionStorePtr lStore = store_->storeManager()->locate(lChain);
		if (lStore) {
			TransactionPtr lTx = lStore->locateTransaction(lConversation);
			if (lTx) {
				//
				TxBuzzerConversationPtr lConversationTx = TransactionHelper::to<TxBuzzerConversation>(lTx);
				uint256 lInitiator = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx(); 
				uint256 lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx(); 

				// initiator
				db::DbTwoKeyContainer<
					uint256 /*buzzer*/, 
					uint256 /*conversation*/,
					uint64_t /*timestamp*/>::Iterator lIndex = conversationsIndex_.find(lInitiator, lConversation);
				if (lIndex.valid()) {
					conversationsOrder_.remove(lInitiator, *lIndex);
				}

				conversationsIndex_.write(lInitiator, lConversation, lEvent->timestamp());
				conversationsOrder_.write(lInitiator, lEvent->timestamp(), lConversation);
				conversationsActivity_.write(lInitiator, lEvent->timestamp());

				// counterparty
				lIndex = conversationsIndex_.find(lCounterparty, lConversation);
				if (lIndex.valid()) {
					conversationsOrder_.remove(lCounterparty, *lIndex);
				}

				conversationsIndex_.write(lCounterparty, lConversation, lEvent->timestamp());
				conversationsOrder_.write(lCounterparty, lEvent->timestamp(), lConversation);
				conversationsActivity_.write(lCounterparty, lEvent->timestamp());

				//
				db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*conversation*/, ConversationInfo /*state*/>::Iterator 
													lConversationIter = conversations_.find(lInitiator, lConversation);
				if (lConversationIter.valid()) {
					ConversationInfo lInfo = *lConversationIter;
					lInfo.setMessage(lEvent->id(), lEvent->chain());
					conversations_.write(lInitiator, lConversation, lInfo);
				}

				//
				db::DbTwoKeyContainer<uint256 /*buzzer*/, uint256 /*conversation*/, ConversationInfo /*state*/>::Iterator 
													lCounterpartyConversationIter = conversations_.find(lCounterparty, lConversation);
				if (lCounterpartyConversationIter.valid()) {
					ConversationInfo lInfo = *lCounterpartyConversationIter;
					lInfo.setMessage(lEvent->id(), lEvent->chain());
					conversations_.write(lCounterparty, lConversation, lInfo);
				}

				//
				if (lBuzzer == lInitiator) {
					events_.write(lCounterparty, lEvent->timestamp(), lEvent->id(), lEvent->type());
					subscriberUpdates(lCounterparty, lEvent->timestamp());
				} else {
					events_.write(lInitiator, lEvent->timestamp(), lEvent->id(), lEvent->type());
					subscriberUpdates(lInitiator, lEvent->timestamp());
				}
			}
		}

		// conversation last timestamp
		publisherUpdates(lConversation, lEvent->timestamp());
		// conversation -> message
		timeline_.write(lConversation, lEvent->timestamp(), lEvent->id());

		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: PUSHED message ") +
			strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
	}
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
				// try commited
				TransactionPtr lFeeTx = lStore->locateTransaction(lEvent->in()[TX_BUZZER_ENDORSE_FEE_IN].out().tx());
				// try mempool
				if (lFeeTx == nullptr) lFeeTx = lStore->locateMempoolTransaction(lEvent->in()[TX_BUZZER_ENDORSE_FEE_IN].out().tx());
				// main and primary case
				if (lFeeTx) {
					TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

					amount_t lAmount;
					uint64_t lHeight;
					uint256 lAsset;
					if (TxBuzzerEndorse::extractLockedAmountHeightAndAsset(lFee, lAmount, lHeight, lAsset) &&
							(lAmount == settings_->oneVoteProofAmount() && lAsset == settings_->proofAsset())) {
						//
						incrementEndorsements(lBuzzer, BUZZER_MIN_EM_STEP); // increment ONLY on specified step
					}
				} else {
					//
					// NOTICE: it can be pitfall on the road and maybe we do not allow to pass it in the following way
					//
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/warning]: fee tx for endorsement was NOT FOUND ") +
						strprintf("%s/%s#", lEvent->in()[TX_BUZZER_ENDORSE_FEE_IN].out().tx().toHex(), MainChain::id().toHex().substr(0, 10)));
					//
					TxBuzzerEndorsePtr lEndorse = TransactionHelper::to<TxBuzzerEndorse>(lEvent);
					// WARNING: commenting out for now
					// incrementEndorsements(lBuzzer, BUZZER_MIN_EM_STEP); // increment ONLY on specified step
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
				// try commited
				TransactionPtr lFeeTx = lStore->locateTransaction(lEvent->in()[TX_BUZZER_MISTRUST_FEE_IN].out().tx());
				// try mempool
				if (lFeeTx == nullptr) lFeeTx = lStore->locateMempoolTransaction(lEvent->in()[TX_BUZZER_MISTRUST_FEE_IN].out().tx());
				// main and primary case
				if (lFeeTx) {
					TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

					amount_t lAmount;
					uint64_t lHeight;
					uint256 lAsset;
					if (TxBuzzerMistrust::extractLockedAmountHeightAndAsset(lFee, lAmount, lHeight, lAsset) &&
							(lAmount == settings_->oneVoteProofAmount() && lAsset == settings_->proofAsset())) {
						//
						incrementMistrusts(lBuzzer, BUZZER_MIN_EM_STEP);
					}
				} else {
					//
					// NOTICE: it can be pitfall on the road and maybe we do not allow to pass it in the following way
					//
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/warning]: fee tx for mistrustment was NOT FOUND ") +
						strprintf("%s/%s#", lEvent->in()[TX_BUZZER_MISTRUST_FEE_IN].out().tx().toHex(), MainChain::id().toHex().substr(0, 10)));
					//
					TxBuzzerMistrustPtr lMistrust = TransactionHelper::to<TxBuzzerMistrust>(lEvent);
					// WARNING: commenting out for now
					// incrementMistrusts(lBuzzer, BUZZER_MIN_EM_STEP); // increment ONLY on specified step
				}
			}
		}
	}
}

void BuzzerTransactionStoreExtension::hashTagUpdate(const uint160& hash, const db::FiveKey<uint160, uint64_t, uint64_t, uint64_t, uint256>& key) {
	//
	if (!opened_) return;
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
	if (!opened_) return;
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
				std::string lLower(lTag->second);
				UTFStringToLowerCase((unsigned char*)lLower.c_str());
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: write hash tag ") +
					strprintf("%s/%s/%s", lTag->first.toHex(), lLower, lTag->second));

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
				hashTags_.write(lLower, lTag->second);
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
			if (!lLocalStore) continue;

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
								if (!lLocalStore) break;

								TransactionPtr lReTx = lLocalStore->locateTransaction(lReTxId);
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

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzz like PUSHED: ") +
				strprintf("tx = %s, buzz = %s, liker = %s", ctx->tx()->id().toHex(), lBuzzId.toHex(), lBuzzerId.toHex()));
		} else {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/error]: buzz is ALREADY liked: ") +
				strprintf("tx = %s, buzz = %s, liker = %s", ctx->tx()->id().toHex(), lBuzzId.toHex(), lBuzzerId.toHex()));
		}
	}
}

void BuzzerTransactionStoreExtension::processHide(const uint256& id, TransactionContextPtr ctx) {
	// extract buzz_id
	TxBuzzHidePtr lBuzzHide = TransactionHelper::to<TxBuzzHide>(ctx->tx());
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: pushing buzz hide ") +
		strprintf("%s/%s#", ctx->tx()->id().toHex(), store_->chain().toHex().substr(0, 10)));
	//
	if (lBuzzHide->in().size() > TX_BUZZ_HIDE_IN) {
		// extract buzz | buzzer
		uint256 lBuzzId = lBuzzHide->in()[TX_BUZZ_HIDE_IN].out().tx(); // buzz_id
		uint256 lBuzzerId = lBuzzHide->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // owner
		
		// check index
		uint256 lOwner;
		if (!hiddenIdx_.read(lBuzzId, lOwner)) {
			// write index
			hiddenIdx_.write(lBuzzId, lBuzzerId);

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzz hide PUSHED: ") +
				strprintf("tx = %s, buzz = %s, owner = %s", ctx->tx()->id().toHex(), lBuzzId.toHex(), lBuzzerId.toHex()));
		} else {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity/error]: buzz is ALREADY hidden: ") +
				strprintf("tx = %s, buzz = %s, owner = %s", ctx->tx()->id().toHex(), lBuzzId.toHex(), lBuzzerId.toHex()));
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
	if (!opened_) return;
	//
	if (tx->type() == TX_BUZZ ||
		tx->type() == TX_REBUZZ ||
		tx->type() == TX_BUZZ_LIKE || 
		tx->type() == TX_BUZZ_REWARD || 
		tx->type() == TX_BUZZ_REPLY ||
		tx->type() == TX_BUZZ_REBUZZ_NOTIFY || 
		tx->type() == TX_BUZZER_MESSAGE ||
		tx->type() == TX_BUZZER_MESSAGE_REPLY) {
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
		}

		// remove rebuzz/reply count
		if (tx->type() == TX_REBUZZ ||
			tx->type() == TX_BUZZ_REPLY) {
			// direct rebuzz
			if (tx->type() == TX_REBUZZ) {
				//
				TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(tx);
				decrementRebuzzes(lRebuzz->buzzId());
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
			}

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzz like removed ") +
				strprintf("buzz = %s, liker = %s", lBuzzId.toHex(), lBuzzerId.toHex()));
		}
	} else if (tx->type() == TX_BUZZ_HIDE) {
		// extract buzz_id
		TxBuzzHidePtr lBuzzHide = TransactionHelper::to<TxBuzzHide>(tx);
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removing buzz hide ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		if (lBuzzHide->in().size() > TX_BUZZ_HIDE_IN) {
			// extract buzz | buzzer
			uint256 lBuzzId = lBuzzHide->in()[TX_BUZZ_HIDE_IN].out().tx(); //
			uint256 lBuzzerId = lBuzzHide->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); //
			// remove index
			uint256 lOwner;
			if (hiddenIdx_.read(lBuzzId, lOwner)) {
				//
				hiddenIdx_.remove(lBuzzId);
			}

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: buzz hide removed ") +
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
				if (lFeeTx == nullptr) lFeeTx = lStore->locateMempoolTransaction(lEvent->in()[TX_BUZZER_ENDORSE_FEE_IN].out().tx());
				if (lFeeTx) {
					TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

					amount_t lAmount;
					uint64_t lHeight;
					uint256 lAsset;
					if (TxBuzzerEndorse::extractLockedAmountHeightAndAsset(lFee, lAmount, lHeight, lAsset) &&
							(lAmount == settings_->oneVoteProofAmount() && lAsset == settings_->proofAsset())) {
						//
						decrementEndorsements(lBuzzer, BUZZER_MIN_EM_STEP);
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
				if (lFeeTx == nullptr) lFeeTx = lStore->locateMempoolTransaction(lEvent->in()[TX_BUZZER_MISTRUST_FEE_IN].out().tx());
				if (lFeeTx) {
					TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

					amount_t lAmount;
					uint64_t lHeight;
					uint256 lAsset;
					if (TxBuzzerMistrust::extractLockedAmountHeightAndAsset(lFee, lAmount, lHeight, lAsset) &&
							(lAmount == settings_->oneVoteProofAmount() && lAsset == settings_->proofAsset())) {
						//
						decrementMistrusts(lBuzzer, BUZZER_MIN_EM_STEP);
						if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removed mistrust ") +
							strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
					}
				}
			}
		}
	} else if (tx->type() == TX_BUZZER_CONVERSATION) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removing conversation ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(tx);

		// in[0] - originator
		uint256 lOriginator = lEvent->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx(); 
		// in[1] - counterparty
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx();

		//
		conversations_.remove(lOriginator, lEvent->id());
		conversationsIndex_.remove(lOriginator, lEvent->id());
		conversationsOrder_.remove(lOriginator, lEvent->timestamp());

		//
		conversations_.remove(lBuzzer, lEvent->id());
		conversationsIndex_.remove(lBuzzer, lEvent->id());
		conversationsOrder_.remove(lBuzzer, lEvent->timestamp());

		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removed conversation ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
	} else if (tx->type() == TX_BUZZER_ACCEPT_CONVERSATION) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removing accept conversation ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));

		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(tx);

		// in[0] - buzzer
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_MY_IN].out().tx(); 
		// in[1] - conversation
		uint256 lConversationId = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().tx();
		uint256 lConversationChain = lEvent->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().chain();

		// counterparty
		uint256 lCounterparty;

		// make event
		ITransactionStorePtr lStore = store_->storeManager()->locate(lConversationChain);
		if (lStore) {
			TransactionPtr lConversationTx = lStore->locateTransaction(lConversationId);
			if (lConversationTx) {
				lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
			}
		}

		//
		conversations_.remove(lBuzzer, lConversationId);
		if (!lCounterparty.isNull()) conversations_.remove(lCounterparty, lConversationId);
		//	
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removed accept conversation ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
	} else if (tx->type() == TX_BUZZER_DECLINE_CONVERSATION) {
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removing decline conversation ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));

		// that should be tx_event object
		TxEventPtr lEvent = TransactionHelper::to<TxEvent>(tx);

		// in[0] - buzzer
		uint256 lBuzzer = lEvent->in()[TX_BUZZER_CONVERSATION_DECLINE_MY_IN].out().tx(); 
		// in[1] - conversation
		uint256 lConversationId = lEvent->in()[TX_BUZZER_CONVERSATION_DECLINE_IN].out().tx();
		uint256 lConversationChain = lEvent->in()[TX_BUZZER_CONVERSATION_DECLINE_IN].out().chain();

		// counterparty
		uint256 lCounterparty;

		// make event
		ITransactionStorePtr lStore = store_->storeManager()->locate(lConversationChain);
		if (lStore) {
			TransactionPtr lConversationTx = lStore->locateTransaction(lConversationId);
			if (lConversationTx) {
				lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
			}
		}

		//
		conversations_.remove(lBuzzer, lConversationId);
		if (!lCounterparty.isNull()) conversations_.remove(lCounterparty, lConversationId);
		//	
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/pushEntity]: removed decline conversation ") +
			strprintf("%s/%s#", tx->id().toHex(), store_->chain().toHex().substr(0, 10)));
		//
	}
}

void BuzzerTransactionStoreExtension::processBuzzerStat(const uint256& buzzer) {
	//
	if (!opened_) return;
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/processBuzzerStat]: reprocessing stat for ") +
		strprintf("buzzer %s/%s#", buzzer.toHex(), store_->chain().toHex().substr(0, 10)));

	{
		db::DbTwoKeyContainer<
			uint256 /*buzzer*/, 
			uint256 /*endorser*/, 
			uint256 /*tx*/>::Iterator lFrom = endorsements_.find(buzzer, uint256());

		lFrom.setKey2Empty();
		//
		for (; lFrom.valid(); ++lFrom) {
			//
			uint256 lBuzzer;
			uint256 lEndorser;
			uint256 lEvent; 

			if (lFrom.first(lBuzzer, lEndorser) && lFrom.second(lEvent)) {
				//
				TransactionPtr lTx = store_->locateTransaction(lEvent);
				if (lTx) {
					TxBuzzerEndorsePtr lEndorse = TransactionHelper::to<TxBuzzerEndorse>(lTx);
					incrementEndorsements(lBuzzer, lEndorse->amount());
				}
			}
		}
	}

	{			
		db::DbTwoKeyContainer<
			uint256 /*buzzer*/, 
			uint256 /*endorser*/, 
			uint256 /*tx*/>::Iterator lFrom = mistrusts_.find(buzzer, uint256());

		lFrom.setKey2Empty();
		//
		for (; lFrom.valid(); ++lFrom) {
			//
			uint256 lBuzzer;
			uint256 lMistruster;
			uint256 lEvent; 

			if (lFrom.first(lBuzzer, lMistruster) && lFrom.second(lEvent)) {
				//
				TransactionPtr lTx = store_->locateTransaction(lEvent);
				if (lTx) {
					TxBuzzerMistrustPtr lMistrust = TransactionHelper::to<TxBuzzerMistrust>(lTx);
					incrementMistrusts(lBuzzer, lMistrust->amount());
				}
			}
		}
	}

	{
		db::DbTwoKeyContainer<
			uint256 /*publisher*/, 
			uint256 /*subscriber*/, 
			uint256 /*tx*/>::Iterator lFrom = publishersIdx_.find(buzzer, uint256());

		lFrom.setKey2Empty();
		//
		for (; lFrom.valid(); ++lFrom) {
			//
			uint256 lSubscriber;
			uint256 lPublisher;
			uint256 lEvent; 

			if (lFrom.first(lPublisher, lSubscriber) && lFrom.second(lEvent)) {
				//
				TransactionPtr lTx = store_->locateTransaction(lEvent);
				if (lTx) {
					TxBuzzerSubscribePtr lSubscribe = TransactionHelper::to<TxBuzzerSubscribe>(lTx);
					lPublisher = lSubscribe->in()[0].out().tx(); // in[0] - publisher
					lSubscriber = lSubscribe->in()[1].out().tx(); // in[1] - subscriber					
					incrementSubscriptions(lSubscriber);
					incrementFollowers(lPublisher);
				}
			}
		}
	}
}

TransactionPtr BuzzerTransactionStoreExtension::locateSubscription(const uint256& subscriber, const uint256& publisher) {
	//
	if (!opened_) return nullptr;
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
	if (!opened_) return;
	//
	BuzzInfo lInfo;
	buzzInfo_.read(buzz, lInfo);
	lInfo.likes_++;
	buzzInfo_.write(buzz, lInfo);
}

void BuzzerTransactionStoreExtension::decrementLikes(const uint256& buzz) {
	//
	if (!opened_) return;
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
	if (!opened_) return;
	//
	BuzzInfo lInfo;
	buzzInfo_.read(buzz, lInfo);
	lInfo.replies_++;
	buzzInfo_.write(buzz, lInfo);
}

void BuzzerTransactionStoreExtension::decrementReplies(const uint256& buzz) {
	//
	if (!opened_) return;
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
	if (!opened_) return;
	//
	BuzzInfo lInfo;
	buzzInfo_.read(buzz, lInfo);
	lInfo.rebuzzes_++;
	buzzInfo_.write(buzz, lInfo);
}

void BuzzerTransactionStoreExtension::decrementRebuzzes(const uint256& buzz) {
	//
	if (!opened_) return;
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
	if (!opened_) return;
	//
	BuzzInfo lInfo;
	buzzInfo_.read(buzz, lInfo);
	lInfo.rewards_ += reward;
	buzzInfo_.write(buzz, lInfo);
}

void BuzzerTransactionStoreExtension::decrementRewards(const uint256& buzz, amount_t reward) {
	//
	if (!opened_) return;
	//
	BuzzInfo lInfo;
	buzzInfo_.read(buzz, lInfo);
	lInfo.rewards_ -= reward;
	buzzInfo_.write(buzz, lInfo);
}

void BuzzerTransactionStoreExtension::incrementEndorsements(const uint256& buzzer, amount_t delta) {
	//
	if (!opened_) return;
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.incrementEndorsements(delta);
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::decrementEndorsements(const uint256& buzzer, amount_t delta) {
	//
	if (!opened_) return;
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.decrementEndorsements(delta);
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::incrementMistrusts(const uint256& buzzer, amount_t delta) {
	//
	if (!opened_) return;
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.incrementMistrusts(delta);
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::decrementMistrusts(const uint256& buzzer, amount_t delta) {
	//
	if (!opened_) return;
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.decrementMistrusts(delta);
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::updateBuzzerInfo(const uint256& buzzer, const uint256& info) {
	//
	if (!opened_) return;
	//
	buzzerInfo_.write(buzzer, info);
}

void BuzzerTransactionStoreExtension::incrementSubscriptions(const uint256& buzzer) {
	//
	if (!opened_) return;
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.incrementSubscriptions();
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::decrementSubscriptions(const uint256& buzzer) {
	//
	if (!opened_) return;
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.decrementSubscriptions();
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::incrementFollowers(const uint256& buzzer) {
	//
	if (!opened_) return;
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.incrementFollowers();
	buzzerStat_.write(buzzer, lInfo);
}

void BuzzerTransactionStoreExtension::decrementFollowers(const uint256& buzzer) {
	//
	if (!opened_) return;
	//
	BuzzerInfo lInfo;
	buzzerStat_.read(buzzer, lInfo);
	lInfo.decrementFollowers();
	buzzerStat_.write(buzzer, lInfo);
}

bool BuzzerTransactionStoreExtension::checkSubscription(const uint256& subscriber, const uint256& publisher) {
	//
	if (!opened_) return false;
	//
	db::DbTwoKeyContainer<uint256 /*subscriber*/, uint256 /*publisher*/, uint256 /*tx*/>::Iterator lItem = subscriptionsIdx_.find(subscriber, publisher);
	return lItem.valid();
}

bool BuzzerTransactionStoreExtension::checkLike(const uint256& buzz, const uint256& liker) {
	//
	if (!opened_) return false;
	//
	db::DbTwoKeyContainer<uint256 /*nuzz*/, uint256 /*liker*/, uint256 /*like_tx*/>::Iterator lItem = likesIdx_.find(buzz, liker);
	return lItem.valid();
}

bool BuzzerTransactionStoreExtension::readBuzzInfo(const uint256& buzz, BuzzInfo& info) {
	//
	if (!opened_) return false;
	//
	return buzzInfo_.read(buzz, info);
}

bool BuzzerTransactionStoreExtension::readBuzzerStat(const uint256& buzzer, BuzzerInfo& info) {
	//
	if (!opened_) return false;
	//
	return buzzerStat_.read(buzzer, info);
}

TxBuzzerInfoPtr BuzzerTransactionStoreExtension::readBuzzerInfo(const uint256& buzzer) {
	//
	if (!opened_) return nullptr;
	//
	uint256 lInfo;
	if (buzzerInfo_.read(buzzer, lInfo)) {
		//
		TransactionPtr lTx = store_->locateTransaction(lInfo);
		if (lTx) {
			return TransactionHelper::to<TxBuzzerInfo>(lTx);
		}
	}

	return nullptr;
}

bool BuzzerTransactionStoreExtension::selectBuzzItem(const uint256& buzzId, BuzzfeedItem& item) {
	//
	if (!opened_) return false;
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
		//else return false;

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
	if (!opened_) return;
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
	if (!opened_) return;
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
	if (!opened_) return;
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
			if (!lBuzzerTx) continue;
			TransactionPtr lPublisherTx = lMainStore->locateTransaction(lPublisher);
			if (!lPublisherTx) continue;

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
	if (!opened_) return;
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
			if (!lBuzzerTx) continue;
			TransactionPtr lPublisherTx = lMainStore->locateTransaction(lPublisher);
			if (!lPublisherTx) continue;

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

BuzzerTransactionStoreExtensionPtr BuzzerTransactionStoreExtension::locateBuzzerExtension(const uint256& buzzer) {
	//
	if (!opened_) return nullptr;
	//
	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());
	TransactionPtr lBuzzerTx = lMainStore->locateTransaction(buzzer);
	if (!lBuzzerTx) return nullptr;

	ITransactionStorePtr lStore = lMainStore->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
	if (!lStore) return nullptr;

	if (!lStore->extension()) return nullptr;
	BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());
	return lExtension;
}

void BuzzerTransactionStoreExtension::selectConversations(uint64_t from, const uint256& buzzer, std::vector<ConversationItem>& feed) {
	//
	if (!opened_) return;
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectConversations]: selecting conversations for ") +
		strprintf("buzzer = %s, from = %d, chain = %s#", buzzer.toHex(), from, store_->chain().toHex().substr(0, 10)));

	std::multimap<uint64_t, uint256> lRawBuzzfeed;
	std::map<uint256, ConversationItemPtr> lBuzzItems;

	// 0. get/set current time
	uint64_t lTimeFrom = from;
	if (lTimeFrom == 0) {
		//
		if (conversationsActivity_.read(buzzer, lTimeFrom)) {
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectConversations]: select conversations - corrected from for ") +
				strprintf("buzzer = %s, from = %d, chain = %s#", buzzer.toHex(), from, store_->chain().toHex().substr(0, 10)));
		}
	}

	db::DbTwoKeyContainer<
		uint256 /*buzzer*/, 
		uint64_t /*timestamp*/,
		uint256 /*conversation*/>::Iterator lFrom;

	if (lTimeFrom == 0) lFrom = conversationsOrder_.last(); // just in case
	else lFrom = conversationsOrder_.find(buzzer, lTimeFrom);

	// 1.0 resetting timestamp to ensure appropriate backtracing
	lFrom.setKey2Empty();

	// 1.1. stepping down up to max 100 conversations 
	for (int lCount = 0; lCount < 100 /*TODO: settings?*/ && lFrom.valid(); --lFrom, ++lCount) {
		//
		uint256 lConversation = *lFrom;
		uint256 lBuzzer;
		uint64_t lTimestamp;
		lFrom.first(lBuzzer, lTimestamp);

		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectConversations]: order... "));

		//
		db::DbTwoKeyContainer<
			uint256 /*buzzer*/,
			uint256 /*conversation*/,
			ConversationInfo /*state*/>::Iterator lConversationInfo = conversations_.find(buzzer, lConversation);

		if (lConversationInfo.valid()) {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectConversations]: conversations... "));
			//
			ConversationInfo lInfo = *lConversationInfo;
			if ((lInfo.state() == ConversationInfo::PENDING || lInfo.state() == ConversationInfo::ACCEPTED) && 
					lBuzzItems.find(lInfo.id()) == lBuzzItems.end()) {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectConversations]: try to select ") +
					strprintf("conversation = %s, chain = %s#", lConversation.toHex(), store_->chain().toHex().substr(0, 10)));
				//
				ITransactionStorePtr lConversationStore = store_->storeManager()->locate(lInfo.chain()); // conversation store
				if (!lConversationStore) continue;
				TransactionPtr lTx = lConversationStore->locateTransaction(lInfo.id());
				if (!lTx) continue;
				TxBuzzerConversationPtr lConversationTx = TransactionHelper::to<TxBuzzerConversation>(lTx);
				//
				uint256 lCreator = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
				uint256 lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx();

				//
				BuzzerTransactionStoreExtensionPtr lCreatorExtension = locateBuzzerExtension(lCreator);
				if (!lCreatorExtension) continue;
				BuzzerTransactionStoreExtensionPtr lCounterpartyExtension = locateBuzzerExtension(lCounterparty);
				if (!lCounterpartyExtension) continue;

				ConversationItemPtr lItem = ConversationItem::instance();

				lItem->setTimestamp(lTimestamp); // last update
				lItem->setConversationId(lInfo.id());
				lItem->setConversationChainId(lInfo.chain());

				TxBuzzerInfoPtr lCreatorInfo = lCreatorExtension->readBuzzerInfo(lCreator);
				if (!lCreatorInfo) continue;
				lItem->setCreatorId(lCreator);
				lItem->setCreatorInfoId(lCreatorInfo->id());
				lItem->setCreatorInfoChainId(lCreatorInfo->chain());

				TxBuzzerInfoPtr lCounterpartyInfo = lCounterpartyExtension->readBuzzerInfo(lCounterparty);
				if (!lCounterpartyInfo) continue;
				lItem->setCounterpartyId(lCounterparty);
				lItem->setCounterpartyInfoId(lCounterpartyInfo->id());
				lItem->setCounterpartyInfoChainId(lCounterpartyInfo->chain());

				lItem->setSignature(lConversationTx->signature());

				BuzzerInfo lCreatorScore;
				BuzzerInfo lCounterpartyScore;
				//
				if (buzzer == lCreator) {
					// get score
					lCounterpartyExtension->readBuzzerStat(lCounterparty, lCounterpartyScore);
					//
					lItem->setSide(ConversationItem::COUNTERPARTY);
					lItem->setScore(lCounterpartyScore.score());
				} else {
					// get score
					lCreatorExtension->readBuzzerStat(lCreator, lCreatorScore);
					//
					lItem->setSide(ConversationItem::CREATOR);
					lItem->setScore(lCreatorScore.score());
				}

				// add ACCEPT for verification
				if (!lInfo.stateChain().isNull() && !lInfo.stateId().isNull()) {
					//
					ITransactionStorePtr lAcceptStore = store_->storeManager()->locate(lInfo.stateChain());
					if (!lAcceptStore) continue;

					TransactionPtr lStateTx = lAcceptStore->locateTransaction(lInfo.stateId());
					if (!lStateTx) continue;

					if (lStateTx->type() == TX_BUZZER_ACCEPT_CONVERSATION) {
						//
						TxBuzzerAcceptConversationPtr lAccept = TransactionHelper::to<TxBuzzerAcceptConversation>(lStateTx);

						ConversationItem::EventInfo lAcceptInfo(
							lAccept->type(),
							lAccept->timestamp(), 
							lConversation,
							lAccept->id(), 
							lAccept->chain(), 
							lCounterparty,
							lCounterpartyInfo->chain(),
							lCounterpartyInfo->id(),
							lCounterpartyScore.score(),
							lAccept->signature());
						
						lItem->addEventInfo(lAcceptInfo);
					}
				}

				if (!lInfo.messageChain().isNull() && !lInfo.messageId().isNull()) {
					//
					ITransactionStorePtr lMessageStore = store_->storeManager()->locate(lInfo.messageChain());
					if (!lMessageStore) continue;

					TransactionPtr lMessageTx = lMessageStore->locateTransaction(lInfo.messageId());
					if (!lMessageTx) continue;
					//
					TxBuzzerMessagePtr lMessage = TransactionHelper::to<TxBuzzerMessage>(lMessageTx);

					ConversationItem::EventInfo lMessageInfo(
						lMessage->type(),
						lMessage->timestamp(), 
						lConversation,
						lMessage->id(), 
						lMessage->chain(), 
						lMessage->in()[TX_BUZZER_MY_IN].out().tx(),
						lMessage->buzzerInfoChain(),
						lMessage->buzzerInfo(),
						lMessage->mediaPointers(),
						lMessage->body(),
						lMessage->in()[TX_BUZZER_MY_IN].out().tx() == lCounterparty ? 
																lCounterpartyScore.score() : lCreatorScore.score(),
						lMessage->signature());
					
					lItem->addEventInfo(lMessageInfo);
				}

				lRawBuzzfeed.insert(std::multimap<uint64_t, uint256>::value_type(lItem->timestamp(), lItem->key()));
				lBuzzItems.insert(std::map<uint256, ConversationItemPtr>::value_type(lItem->key(), lItem));
			}
		}
	}

	for (std::multimap<uint64_t, uint256>::reverse_iterator lItem = lRawBuzzfeed.rbegin(); lItem != lRawBuzzfeed.rend(); lItem++) {
		//
		if (feed.size() < 100 /*max last events*/) {
			std::map<uint256, ConversationItemPtr>::iterator lBuzzItem = lBuzzItems.find(lItem->second);
			if (lBuzzItem != lBuzzItems.end()) feed.push_back(*(lBuzzItem->second));
		} else {
			break;
		}
	}

	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectConversations]: ") +
		strprintf("items count = %d, %s#", feed.size(), store_->chain().toHex().substr(0, 10)));			
}

void BuzzerTransactionStoreExtension::selectMessages(uint64_t from, const uint256& conversation, std::vector<BuzzfeedItem>& feed) {
	//
	if (!opened_) return;
	//	
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectMessages]: selecting messages for ") +
		strprintf("conversation = %s, from = %d, chain = %s#", conversation.toHex(), from, store_->chain().toHex().substr(0, 10)));

	// 0. get/set current time
	uint64_t lPublisherTime = from;
	if (!lPublisherTime && publisherUpdates_.read(conversation, lPublisherTime)) {
		// insert own
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectMessages]: conversation time is ") +
			strprintf("%s/%d", conversation.toHex(), lPublisherTime));
	}

	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());
	std::multimap<uint64_t, BuzzfeedItem::Key> lRawBuzzfeed;
	std::map<BuzzfeedItem::Key, BuzzfeedItemPtr> lBuzzItems;

	db::DbTwoKeyContainer<uint256 /*publisher*/, uint64_t /*timestamp*/, uint256 /*tx*/>::Iterator lFrom = 
			timeline_.find(conversation, lPublisherTime);
	db::DbTwoKeyContainer<uint256 /*publisher*/, uint64_t /*timestamp*/, uint256 /*tx*/>::Transaction lTransaction = 
			timeline_.transaction();
	lFrom.setKey2Empty();

	// 0. try mempool
	IMemoryPoolPtr lMempool = store_->storeManager()->locateMempool(store_->chain());
	if (lMempool) {
		std::list<TransactionContextPtr> lPending;
		lMempool->selectTransactions(conversation, lPending);
		//
		if (lPending.size()) {
			for (std::list<TransactionContextPtr>::iterator lCtx = lPending.begin(); lCtx != lPending.end(); lCtx++) {
				//
				TxEventPtr lTx = TransactionHelper::to<TxEvent>((*lCtx)->tx());
				if ((lTx->type() == TX_BUZZER_MESSAGE || lTx->type() == TX_BUZZER_MESSAGE_REPLY) && 
					lTx->timestamp() < lPublisherTime) {
					//
					TxBuzzerPtr lBuzzer;
					uint256 lTxPublisher = lTx->in()[TX_BUZZER_MY_IN].out().tx(); // buzzer allways is the first in
					TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lTxPublisher);
					if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
					else continue;
					//
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectMessages]: try to added item ") +
						strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

					//
					int lContext = 0;
					BuzzfeedItemPtr lItem = makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems, uint256());
					if (lItem) {
						lItem->setMempool();
					}					
				}
			}
		}
	}

	// 1. select
	for (;lBuzzItems.size() < 50 /*TODO: settings?*/ && lFrom.valid(); --lFrom) {
		//
		uint256 lId; 
		uint64_t lTimestamp; 

		//
		if (!lFrom.first(lId, lTimestamp)) {
			lTransaction.remove(lFrom);
			continue;
		}

		//
		int lContext = 0;
		TxBuzzerPtr lBuzzer = nullptr;
		uint256 lEventId = *lFrom;
		TransactionPtr lTx = store_->locateTransaction(lEventId);
		if (!lTx) {
			lTransaction.remove(lFrom);
			continue;
		}
		//
		if (lTx->type() == TX_BUZZER_MESSAGE || lTx->type() == TX_BUZZER_MESSAGE_REPLY) {
			//
			uint256 lTxPublisher = lTx->in()[TX_BUZZER_MY_IN].out().tx(); // buzzer allways is the first in
			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lTxPublisher);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			else continue;
			// check for hidden
			uint256 lOwner;
			if (hiddenIdx_.read(*lFrom, lOwner)) continue;
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectMessages]: try to added item ") +
				strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			//
			makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems, uint256());
		} 
	}

	// 3. free-up
	lTransaction.commit();

	// 4. fill reply
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
					if (lBuzzItem->second->type() == TX_BUZZER_MESSAGE_REPLY) {
						lContinue = true;
					} else { 
						lContinue = false;
					}
				}
			} else {
				//
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectMessages]: item is MISSING ") +
					strprintf("%s/%s, %s#", lItem->second.id().toHex(), lItem->second.typeToString(), store_->chain().toHex().substr(0, 10)));
			}
		} else {
			break;
		}
	}

	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectMessages]: ") +
		strprintf("items count = %d, %s#", feed.size(), store_->chain().toHex().substr(0, 10)));	
}

void BuzzerTransactionStoreExtension::fillEventsfeed(unsigned short type, TransactionPtr tx, const uint256& subscriber, std::multimap<uint64_t, EventsfeedItem::Key>& rawBuzzfeed, std::map<EventsfeedItem::Key, EventsfeedItemPtr>& buzzItems) {
	//
	if (!opened_) return;
	//
	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());
	//
	if (type == TX_BUZZ || type == TX_BUZZ_REPLY || type == TX_REBUZZ ||
		type == TX_BUZZER_MESSAGE || type == TX_BUZZER_MESSAGE_REPLY) {
		//
		TxBuzzerPtr lBuzzer = nullptr;
		uint256 lPublisher = tx->in()[TX_BUZZ_REPLY_MY_IN].out().tx();
		if (lPublisher != subscriber) {
			//
			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lPublisher);
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			else return;
			//
			EventsfeedItemPtr lItem = makeEventsfeedItem(lBuzzer, tx, lMainStore, rawBuzzfeed, buzzItems);
			//
			if (lItem && (type == TX_BUZZER_MESSAGE || type == TX_BUZZER_MESSAGE_REPLY)) {
				//
				uint256 lConversationId = tx->in()[TX_BUZZER_CONVERSATION_IN].out().tx();
				uint256 lConversationChain = tx->in()[TX_BUZZER_CONVERSATION_IN].out().chain();
				//
				lItem->beginInfo()->setEventId(lConversationId);
				lItem->beginInfo()->setEventChainId(lConversationChain);
			}
		}
	} else if (type == TX_BUZZ_LIKE) {
		//
		makeEventsfeedLikeItem(tx, lMainStore, rawBuzzfeed, buzzItems);
	} else if (type == TX_BUZZ_REWARD) {
		//
		uint256 lPublisher = tx->in()[TX_BUZZ_MY_BUZZER_IN].out().tx();
		uint256 lBuzzId = tx->in()[TX_BUZZ_REWARD_IN].out().tx(); // buzz_id
		uint256 lBuzzChainId = tx->in()[TX_BUZZ_REWARD_IN].out().chain(); // buzz_id

		EventsfeedItemPtr lItem = EventsfeedItem::instance();
		makeEventsfeedRewardItem(lItem, tx, lBuzzId, lBuzzChainId, lPublisher);
		//
		rawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
		buzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));

	} else if (type == TX_BUZZER_MISTRUST) {
		//
		uint256 lMistruster = tx->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx();
		TxBuzzerMistrustPtr lMistrust = TransactionHelper::to<TxBuzzerMistrust>(tx);
		//
		EventsfeedItemPtr lItem = EventsfeedItem::instance();
		makeEventsfeedTrustScoreItem<TxBuzzerMistrust>(lItem, tx, type, lMistruster, lMistrust->score());
		//
		rawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
		buzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));

	} else if (type == TX_BUZZER_ENDORSE) {
		//
		uint256 lEndorser = tx->in()[TX_BUZZER_ENDORSE_MY_IN].out().tx(); 
		TxBuzzerEndorsePtr lEndorse = TransactionHelper::to<TxBuzzerEndorse>(tx);
		//
		EventsfeedItemPtr lItem = EventsfeedItem::instance();
		makeEventsfeedTrustScoreItem<TxBuzzerEndorse>(lItem, tx, type, lEndorser, lEndorse->score());
		//
		rawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
		buzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));
	} else if (type == TX_BUZZER_SUBSCRIBE) {
		//
		TxBuzzerSubscribePtr lSubscribeTx = TransactionHelper::to<TxBuzzerSubscribe>(tx);
		uint256 lPublisher = lSubscribeTx->in()[0].out().tx(); // in[0] - publisher
		uint256 lSubscriber = lSubscribeTx->in()[1].out().tx(); // in[1] - subscriber

		EventsfeedItemPtr lItem = EventsfeedItem::instance();
		TxBuzzerPtr lBuzzer = nullptr;
		TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lSubscriber);
		if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
		else return;
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
		rawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lItem->timestamp(), lItem->key()));
		buzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lItem->key(), lItem));
		//
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEventsfeed]: ") +
			strprintf("add subscribe = %s, %s, %s#", lPublisher.toHex(), lSubscriber.toHex(), store_->chain().toHex().substr(0, 10)));		
	} else if (type == TX_BUZZER_CONVERSATION) {
		//
		TxBuzzerConversationPtr lConversationTx = TransactionHelper::to<TxBuzzerConversation>(tx);			
		ITransactionStorePtr lStore = store_->storeManager()->locate(lConversationTx->chain()); // conversation store
		if (lStore) {
			//
			uint256 lCreator = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
			uint256 lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx();

			//
			BuzzerTransactionStoreExtensionPtr lCreatorExtension = locateBuzzerExtension(lCreator);
			if (!lCreatorExtension) return;
			BuzzerTransactionStoreExtensionPtr lCounterpartyExtension = locateBuzzerExtension(lCounterparty);
			if (!lCounterpartyExtension) return;

			TxBuzzerInfoPtr lCreatorInfo = lCreatorExtension->readBuzzerInfo(lCreator);
			if (!lCreatorInfo) return;

			TxBuzzerInfoPtr lCounterpartyInfo = lCounterpartyExtension->readBuzzerInfo(lCounterparty);
			if (!lCounterpartyInfo) return;

			BuzzerInfo lCounterpartyScore;
			BuzzerInfo lCreatorScore;
			lCounterpartyExtension->readBuzzerStat(lCounterparty, lCounterpartyScore);
			lCreatorExtension->readBuzzerStat(lCreator, lCreatorScore);

			//
			EventsfeedItemPtr lEvent = EventsfeedItem::instance();

			lEvent->setType(TX_BUZZER_CONVERSATION);
			lEvent->setTimestamp(lConversationTx->timestamp());

			// get score
			BuzzerTransactionStoreExtension::BuzzerInfo lScore;
			lCounterpartyExtension->readBuzzerStat(lCounterparty, lScore);

			// conversation info
			lEvent->setBuzzId(lConversationTx->id());
			lEvent->setBuzzChainId(lConversationTx->chain());
			lEvent->setPublisher(lCreator);
			lEvent->setPublisherInfo(lCreatorInfo->id());
			lEvent->setPublisherInfoChain(lCreatorInfo->chain());
			lEvent->setScore(lCreatorScore.score());

			// add CONVERSATION for verification
			EventsfeedItem::EventInfo lEventInfo(
				lConversationTx->timestamp(), 
				lCounterparty, //lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx(), 
				lCreatorInfo->chain(), 
				lCreatorInfo->id(), 
				lCreatorScore.score());
			
			lEventInfo.setEvent(lConversationTx->chain(), lConversationTx->id(), lConversationTx->signature());
			lEvent->addEventInfo(lEventInfo);

			//
			rawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lEvent->timestamp(), lEvent->key()));
			buzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lEvent->key(), lEvent));
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEventsfeed]: ") +
				strprintf("add conversation = %s, %s, %s#", lConversationTx->id().toHex(), lCounterparty.toHex(), store_->chain().toHex().substr(0, 10)));
		}
		//
	} else if (type == TX_BUZZER_ACCEPT_CONVERSATION) {
		//
		TxBuzzerAcceptConversationPtr lAcceptConversationTx = TransactionHelper::to<TxBuzzerAcceptConversation>(tx);
		// in[0] - buzzer
		uint256 lBuzzer = lAcceptConversationTx->in()[TX_BUZZER_CONVERSATION_ACCEPT_MY_IN].out().tx(); 
		// in[1] - conversation
		uint256 lConversationId = lAcceptConversationTx->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().tx();
		uint256 lConversationChain = lAcceptConversationTx->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().chain();
		//
		ITransactionStorePtr lStore = store_->storeManager()->locate(lAcceptConversationTx->chain()); // conversation store
		if (lStore) {
			//
			TransactionPtr lConversation = lStore->locateTransaction(lConversationId);
			if (!lConversation) return;
			TxBuzzerConversationPtr lConversationTx = TransactionHelper::to<TxBuzzerConversation>(lConversation);
			//
			uint256 lCreator = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
			uint256 lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx();

			//
			BuzzerTransactionStoreExtensionPtr lCreatorExtension = locateBuzzerExtension(lCreator);
			if (!lCreatorExtension) return;
			BuzzerTransactionStoreExtensionPtr lCounterpartyExtension = locateBuzzerExtension(lCounterparty);
			if (!lCounterpartyExtension) return;

			TxBuzzerInfoPtr lCreatorInfo = lCreatorExtension->readBuzzerInfo(lCreator);
			if (!lCreatorInfo) return;

			TxBuzzerInfoPtr lCounterpartyInfo = lCounterpartyExtension->readBuzzerInfo(lCounterparty);
			if (!lCounterpartyInfo) return;

			BuzzerInfo lCounterpartyScore;
			BuzzerInfo lCreatorScore;
			lCounterpartyExtension->readBuzzerStat(lCounterparty, lCounterpartyScore);
			lCreatorExtension->readBuzzerStat(lCreator, lCreatorScore);

			//
			EventsfeedItemPtr lEvent = EventsfeedItem::instance();

			lEvent->setType(TX_BUZZER_ACCEPT_CONVERSATION);
			lEvent->setTimestamp(lConversationTx->timestamp());

			// get score
			BuzzerTransactionStoreExtension::BuzzerInfo lScore;
			lCounterpartyExtension->readBuzzerStat(lCounterparty, lScore);

			// conversation info
			lEvent->setBuzzId(lConversationTx->id());
			lEvent->setBuzzChainId(lConversationTx->chain());
			lEvent->setPublisher(lCreator);
			lEvent->setPublisherInfo(lCreatorInfo->id());
			lEvent->setPublisherInfoChain(lCreatorInfo->chain());
			lEvent->setScore(lCreatorScore.score());

			// add CONVERSATION for verification
			EventsfeedItem::EventInfo lEventInfo(
				lAcceptConversationTx->timestamp(), 
				lAcceptConversationTx->in()[TX_BUZZER_CONVERSATION_ACCEPT_MY_IN].out().tx(), 
				lCounterpartyInfo->chain(), 
				lCounterpartyInfo->id(), 
				lCounterpartyScore.score());

			
			lEventInfo.setEvent(lAcceptConversationTx->chain(), lAcceptConversationTx->id(), lAcceptConversationTx->signature());
			lEvent->addEventInfo(lEventInfo);

			rawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lEvent->timestamp(), lEvent->key()));
			buzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lEvent->key(), lEvent));
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEventsfeed]: ") +
				strprintf("add accept conversation = %s, %s, %s#", lAcceptConversationTx->id().toHex(), lBuzzer.toHex(), store_->chain().toHex().substr(0, 10)));
		}
		//
	} else if (type == TX_BUZZER_DECLINE_CONVERSATION) {
		//
		TxBuzzerDeclineConversationPtr lDeclineConversationTx = TransactionHelper::to<TxBuzzerDeclineConversation>(tx);
		// in[0] - buzzer
		uint256 lBuzzer = lDeclineConversationTx->in()[TX_BUZZER_CONVERSATION_ACCEPT_MY_IN].out().tx(); 
		// in[1] - conversation
		uint256 lConversationId = lDeclineConversationTx->in()[TX_BUZZER_CONVERSATION_DECLINE_IN].out().tx();
		//
		uint256 lConversationChain = lDeclineConversationTx->in()[TX_BUZZER_CONVERSATION_DECLINE_IN].out().chain();
		//
		ITransactionStorePtr lStore = store_->storeManager()->locate(lDeclineConversationTx->chain()); // conversation store
		if (lStore) {
			//
			TransactionPtr lConversation = lStore->locateTransaction(lConversationId);
			if (!lConversation) return;
			TxBuzzerConversationPtr lConversationTx = TransactionHelper::to<TxBuzzerConversation>(lConversation);
			//
			uint256 lCreator = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
			uint256 lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx();

			//
			BuzzerTransactionStoreExtensionPtr lCreatorExtension = locateBuzzerExtension(lCreator);
			if (!lCreatorExtension) return;
			BuzzerTransactionStoreExtensionPtr lCounterpartyExtension = locateBuzzerExtension(lCounterparty);
			if (!lCounterpartyExtension) return;

			TxBuzzerInfoPtr lCreatorInfo = lCreatorExtension->readBuzzerInfo(lCreator);
			if (!lCreatorInfo) return;

			TxBuzzerInfoPtr lCounterpartyInfo = lCounterpartyExtension->readBuzzerInfo(lCounterparty);
			if (!lCounterpartyInfo) return;

			BuzzerInfo lCounterpartyScore;
			BuzzerInfo lCreatorScore;
			lCounterpartyExtension->readBuzzerStat(lCounterparty, lCounterpartyScore);
			lCreatorExtension->readBuzzerStat(lCreator, lCreatorScore);

			//
			EventsfeedItemPtr lEvent = EventsfeedItem::instance();

			lEvent->setType(TX_BUZZER_DECLINE_CONVERSATION);
			lEvent->setTimestamp(lConversationTx->timestamp());

			// get score
			BuzzerTransactionStoreExtension::BuzzerInfo lScore;
			lCounterpartyExtension->readBuzzerStat(lCounterparty, lScore);

			// conversation info
			lEvent->setBuzzId(lConversationTx->id());
			lEvent->setBuzzChainId(lConversationTx->chain());
			lEvent->setPublisher(lCreator);
			lEvent->setPublisherInfo(lCreatorInfo->id());
			lEvent->setPublisherInfoChain(lCreatorInfo->chain());
			lEvent->setScore(lCreatorScore.score());

			// add CONVERSATION for verification
			EventsfeedItem::EventInfo lEventInfo(
				lDeclineConversationTx->timestamp(), 
				lDeclineConversationTx->in()[TX_BUZZER_CONVERSATION_DECLINE_MY_IN].out().tx(), 
				lCounterpartyInfo->chain(), 
				lCounterpartyInfo->id(), 
				lCounterpartyScore.score());

			
			lEventInfo.setEvent(lDeclineConversationTx->chain(), lDeclineConversationTx->id(), lDeclineConversationTx->signature());
			lEvent->addEventInfo(lEventInfo);
			//
			rawBuzzfeed.insert(std::multimap<uint64_t, EventsfeedItem::Key>::value_type(lEvent->timestamp(), lEvent->key()));
			buzzItems.insert(std::map<EventsfeedItem::Key, EventsfeedItemPtr>::value_type(lEvent->key(), lEvent));
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEventsfeed]: ") +
				strprintf("add decline conversation = %s, %s, %s#", lDeclineConversationTx->id().toHex(), lBuzzer.toHex(), store_->chain().toHex().substr(0, 10)));
		}
	}
}

void BuzzerTransactionStoreExtension::selectEventsfeed(uint64_t from, const uint256& subscriber, std::vector<EventsfeedItem>& feed) {
	//
	if (!opened_) return;
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

	uint64_t lActualTime = (lTimeFrom < lSubscriberTime ? lTimeFrom : lSubscriberTime);

	// 0.0. check unconfirmed events
	// try mempool
	IMemoryPoolPtr lMempool = store_->storeManager()->locateMempool(store_->chain());
	if (lMempool) {
		std::list<TransactionContextPtr> lPending;
		lMempool->selectTransactions(subscriber, lPending);
		//
		if (lPending.size()) {
			for (std::list<TransactionContextPtr>::iterator lCtx = lPending.begin(); lCtx != lPending.end(); lCtx++) {
				//
				TxEventPtr lTx = TransactionHelper::to<TxEvent>((*lCtx)->tx());
				if (lTx->timestamp() <= lActualTime) {
					fillEventsfeed(lTx->type(), lTx, subscriber, lRawBuzzfeed, lBuzzItems);
				}
			}
		} else {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectEventsfeed(*)]: NO pending items for ") +
				strprintf("subscriber = %s, from = %d, chain = %s#", subscriber.toHex(), from, store_->chain().toHex().substr(0, 10)));
		}
	}

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
			*lFromEvent == TX_BUZZER_MISTRUST || *lFromEvent == TX_BUZZER_ENDORSE || *lFromEvent == TX_BUZZER_SUBSCRIBE || 
			*lFromEvent == TX_BUZZER_CONVERSATION || *lFromEvent == TX_BUZZER_ACCEPT_CONVERSATION || 
			*lFromEvent == TX_BUZZER_DECLINE_CONVERSATION || *lFromEvent == TX_BUZZER_MESSAGE || *lFromEvent == TX_BUZZER_MESSAGE_REPLY) {
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

				fillEventsfeed(*lFromEvent, lTx, subscriber, lRawBuzzfeed, lBuzzItems);
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

EventsfeedItemPtr BuzzerTransactionStoreExtension::makeEventsfeedItem(TxBuzzerPtr buzzer, TransactionPtr tx, ITransactionStorePtr mainStore, std::multimap<uint64_t, EventsfeedItem::Key>& rawBuzzfeed, std::map<EventsfeedItem::Key, EventsfeedItemPtr>& buzzes) {
	//
	if (tx->type() == TX_BUZZ ||
		tx->type() == TX_REBUZZ ||
		tx->type() == TX_BUZZ_REPLY ||
		tx->type() == TX_BUZZER_MESSAGE ||
		tx->type() == TX_BUZZER_MESSAGE_REPLY) {
		//
		bool lAdd = true;
		TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(tx);
		//
		EventsfeedItemPtr lItem = EventsfeedItem::instance();
		//
		if (tx->type() == TX_BUZZ || tx->type() == TX_BUZZ_REPLY ||
			tx->type() == TX_BUZZER_MESSAGE || tx->type() == TX_BUZZER_MESSAGE_REPLY) {
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
		if (lAdd) return lItem;
	}

	return nullptr;
}

void BuzzerTransactionStoreExtension::makeEventsfeedLikeItem(TransactionPtr tx, ITransactionStorePtr mainStore, std::multimap<uint64_t, EventsfeedItem::Key>& rawBuzzfeed, std::map<EventsfeedItem::Key, EventsfeedItemPtr>& buzzes) {
	//
	if (!opened_) return;
	//
	TxBuzzLikePtr lLike = TransactionHelper::to<TxBuzzLike>(tx);

	uint256 lPublisherId = lLike->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator
	uint256 lBuzzId = lLike->in()[TX_BUZZ_LIKE_IN].out().tx(); // buzz_id
	uint256 lBuzzChainId = lLike->in()[TX_BUZZ_LIKE_IN].out().chain(); // buzz_id

	bool lAdd = true;
	ITransactionStorePtr lStore = store_->storeManager()->locate(lBuzzChainId);
	if (!lStore) return;

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

		if (buzz->type() == TX_BUZZ || buzz->type() == TX_BUZZ_REPLY || buzz->type() == TX_REBUZZ || 
			buzz->type() == TX_BUZZER_MESSAGE || buzz->type() == TX_BUZZER_MESSAGE_REPLY) {
			lInfo.setEvent(eventChainId, eventId, buzz->body(), buzz->mediaPointers(), signature);
		} else {
			lInfo.setEvent(eventChainId, eventId, signature);			
		}

		item.addEventInfo(lInfo);
	}	
}

void BuzzerTransactionStoreExtension::selectBuzzfeedByBuzz(uint64_t from, const uint256& buzz, const uint256& subscriber, std::vector<BuzzfeedItem>& feed) {
	//
	if (!opened_) return;
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
			else return;
			//
			bool lProcessed = false;
			if (lTx->type() == TX_REBUZZ) {
				TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lTx);
				if (lRebuzz->simpleRebuzz()) {
					//
					makeBuzzfeedRebuzzItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
					lProcessed = true;
				}
			}

			if (!lProcessed) {
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzz]: try to add item ") +
					strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

				makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
			}
		}
	}

	// try mempool
	IMemoryPoolPtr lMempool = store_->storeManager()->locateMempool(store_->chain());
	if (lMempool) {
		std::list<TransactionContextPtr> lPending;
		lMempool->selectTransactions(buzz, lPending);
		//
		if (lPending.size()) {
			for (std::list<TransactionContextPtr>::iterator lCtx = lPending.begin(); lCtx != lPending.end(); lCtx++) {
				//
				TxEventPtr lTx = TransactionHelper::to<TxEvent>((*lCtx)->tx());
				if (lTx->type() == TX_BUZZ_REPLY && lTx->timestamp() >= from) {
					//
					TxBuzzerPtr lBuzzer;
					uint256 lTxPublisher = lTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer allways is the first in
					TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lTxPublisher);
					if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
					else continue;
					// check for "trusted"
					BuzzerTransactionStoreExtensionPtr lPublisherExtension = locateBuzzerExtension(lTxPublisher);
					if (lPublisherExtension) {
						BuzzerInfo lPublisherInfo;
						lPublisherExtension->readBuzzerStat(lTxPublisher, lPublisherInfo);
						if (!lPublisherInfo.trusted()) continue;
					}
					//
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzz(*)]: try to added item ") +
						strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

					//
					// TODO: consider to limit uplinkage for parents (at least when from != 0)
					int lContext = 0;
					BuzzfeedItemPtr lItem = makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber, !from);
					if (lItem) {
						lItem->setMempool();
					}
				}
			}
		} else {
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzz(*)]: NO pending items for ") +
				strprintf("buzz = %s, from = %d, chain = %s#", buzz.toHex(), from, store_->chain().toHex().substr(0, 10)));
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
			else continue;
			// check for "trusted"
			BuzzerTransactionStoreExtensionPtr lPublisherExtension = locateBuzzerExtension(lTxPublisher);
			if (lPublisherExtension) {
				BuzzerInfo lPublisherInfo;
				lPublisherExtension->readBuzzerStat(lTxPublisher, lPublisherInfo);
				if (!lPublisherInfo.trusted()) continue;
			}
			/*
			// check for personal trust
			uint256 lTrustTx;
			if (mistrusts_.read(qbit::db::TwoKey<uint256, uint256>(lTxPublisher, subscriber), lTrustTx)) {
				// re-ceck if we have compensation
				if (!endorsements_.read(qbit::db::TwoKey<uint256, uint256>(lTxPublisher, subscriber), lTrustTx))
					continue; // in case if we have mistrusted and NOT endorsed the publisher
			}
			*/
			// check for hidden
			uint256 lOwner;
			if (hiddenIdx_.read(lEventId, lOwner)) continue;
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzz]: try to added item ") +
				strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			//
			// TODO: consider to limit uplinkage for parents (at least when from != 0)
			makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber, !from);
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

void BuzzerTransactionStoreExtension::selectBuzzfeedByBuzzer(uint64_t from, const uint256& buzzer, const uint256& subscriber, std::vector<BuzzfeedItem>& feed) {
	//
	if (!opened_) return;
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
			else continue;
			// check for "trusted"
			BuzzerTransactionStoreExtensionPtr lPublisherExtension = locateBuzzerExtension(lTxPublisher);
			if (lPublisherExtension) {
				BuzzerInfo lPublisherInfo;
				lPublisherExtension->readBuzzerStat(lTxPublisher, lPublisherInfo);
				if (!lPublisherInfo.trusted()) continue;
			}
			// check for hidden
			uint256 lOwner;
			if (hiddenIdx_.read(*lFrom, lOwner)) continue;
			//
			bool lProcessed = false;
			if (lTx->type() == TX_REBUZZ) {
				TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lTx);
				if (lRebuzz->simpleRebuzz()) {
					//
					makeBuzzfeedRebuzzItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
					lProcessed = true;
				}
			}

			if (!lProcessed) {
				if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByBuzzer]: try to add item ") +
					strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

				makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
			}
		} else if (lTx->type() == TX_BUZZ_LIKE) {
			//
			makeBuzzfeedLikeItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
		} else if (lTx->type() == TX_BUZZ_REWARD) {
			//
			makeBuzzfeedRewardItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
		} else if (lTx->type() == TX_BUZZER_MISTRUST) {
			//
			TxBuzzerMistrustPtr lEvent = TransactionHelper::to<TxBuzzerMistrust>(lTx);
			uint256 lMistruster = lEvent->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx(); 
			uint256 lBuzzerId = lTx->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 

			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);
			if (!lBuzzerTx) continue;

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
			if (!lBuzzerTx) continue;

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
	for (std::multimap<uint64_t, BuzzfeedItem::Key>::reverse_iterator lItem = lRawBuzzfeed.rbegin(); lItem != lRawBuzzfeed.rend(); lItem++) {
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
															const uint256& publisher, const uint256& subscriber, std::vector<BuzzfeedItem>& feed) {
	//
	if (!opened_) return;
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
			else continue;

			// check for "trusted"
			BuzzerTransactionStoreExtensionPtr lPublisherExtension = locateBuzzerExtension(lTxPublisher);
			if (lPublisherExtension) {
				BuzzerInfo lPublisherInfo;
				lPublisherExtension->readBuzzerStat(lTxPublisher, lPublisherInfo);
				if (!lPublisherInfo.trusted()) continue;
			}

			// check for hidden
			uint256 lOwner;
			if (hiddenIdx_.read(*lFrom, lOwner)) continue;

			// check for personal trust
			uint256 lTrustTx;
			if (mistrusts_.read(qbit::db::TwoKey<uint256, uint256>(lTxPublisher, subscriber), lTrustTx)) {
				// re-ceck if we have compensation
				if (!endorsements_.read(qbit::db::TwoKey<uint256, uint256>(lTxPublisher, subscriber), lTrustTx))
					continue; // in case if we have mistrusted and NOT endirsed the publisher
			}

			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedGlobal]: try to added item ") +
				strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
		} /*else if (lTx->type() == TX_BUZZER_MISTRUST) {
			//
			TxBuzzerMistrustPtr lEvent = TransactionHelper::to<TxBuzzerMistrust>(lTx);
			uint256 lMistruster = lEvent->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx(); 
			uint256 lBuzzerId = lTx->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 

			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);
			if (!lBuzzerTx) continue;

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
			if (!lBuzzerTx) continue;

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
		}*/
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
									uint64_t scoreFrom, uint64_t timestampFrom, const uint256& publisher, const uint256& subscriber,std::vector<BuzzfeedItem>& feed) {
	//
	if (!opened_) return;
	//
	ITransactionStorePtr lMainStore = store_->storeManager()->locate(MainChain::id());
	std::multimap<uint64_t, BuzzfeedItem::Key> lRawBuzzfeed;
	std::map<BuzzfeedItem::Key, BuzzfeedItemPtr> lBuzzItems;

	std::string lTag(tag); // copy
	UTFStringToLowerCase((unsigned char*)lTag.c_str()); // transform
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
			else continue;
			
			// check for "trusted"
			BuzzerTransactionStoreExtensionPtr lPublisherExtension = locateBuzzerExtension(lTxPublisher);
			if (lPublisherExtension) {
				BuzzerInfo lPublisherInfo;
				lPublisherExtension->readBuzzerStat(lTxPublisher, lPublisherInfo);
				if (!lPublisherInfo.trusted()) continue;
			}

			// check for hidden
			uint256 lOwner;
			if (hiddenIdx_.read(*lFrom, lOwner)) continue;

			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeedByTag]: try to added item ") +
				strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
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
	if (!opened_) return;
	//
	if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectHashTags]: selecting hash tags for ") +
		strprintf("tag = %s, chain = %s#", tag, store_->chain().toHex().substr(0, 10)));
	//
	std::string lLower(tag);
	UTFStringToLowerCase((unsigned char*)lLower.c_str());
	//
	db::DbContainer<std::string, std::string>::Iterator lFrom = hashTags_.find(lLower);
	for (int lCount = 0; lFrom.valid() && feed.size() < 6 && lCount < 100; ++lFrom, ++lCount) {
		std::string lTag;
		if (lFrom.first(lTag) && lTag.find(lLower) != std::string::npos) {
			feed.push_back(Buzzer::HashTag(Hash160(lTag.begin(), lTag.end()), *lFrom));

			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectHashTags]: add hash tag ") +
				strprintf("tag = %s, chain = %s#", *lFrom, store_->chain().toHex().substr(0, 10)));
		}
	}
}

void BuzzerTransactionStoreExtension::selectBuzzfeed(const std::vector<BuzzfeedPublisherFrom>& from, const uint256& subscriber, std::vector<BuzzfeedItem>& feed) {
	//
	if (!opened_) return;
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
		for (; (lBuzzItems.size() - lCurrentPublisher) < 50 /*TODO: settings?*/ && lFrom.valid(); --lFrom) {
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
				else continue;

				// check for "trusted"
				BuzzerTransactionStoreExtensionPtr lPublisherExtension = locateBuzzerExtension(lTxPublisher);
				if (lPublisherExtension) {
					BuzzerInfo lPublisherInfo;
					lPublisherExtension->readBuzzerStat(lTxPublisher, lPublisherInfo);
					if (!lPublisherInfo.trusted()) continue;
				}

				/*
				// check for personal trust
				uint256 lTrustTx;
				if (mistrusts_.read(qbit::db::TwoKey<uint256, uint256>(lTxPublisher, subscriber), lTrustTx)) {
					// re-ceck if we have compensation
					if (!endorsements_.read(qbit::db::TwoKey<uint256, uint256>(lTxPublisher, subscriber), lTrustTx))
						continue; // in case if we have mistrusted and NOT endorsed the publisher
				}
				*/

				// check for hidden
				uint256 lOwner;
				if (hiddenIdx_.read(*lFrom, lOwner)) continue;

				//
				bool lProcessed = false;
				if (lTx->type() == TX_REBUZZ) {
					TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lTx);
					if (lRebuzz->simpleRebuzz()) {
						//
						// if (subscriber != lTxPublisher) 
						makeBuzzfeedRebuzzItem(lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
						lProcessed = true;
					}
				}

				if (!lProcessed) {
					if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: try to add item ") +
						strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

					makeBuzzfeedItem(lContext, lBuzzer, lTx, lMainStore, lRawBuzzfeed, lBuzzItems, subscriber);
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
				if (!lBuzzerTx) continue;

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
				if (!lBuzzerTx) continue;

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
		if (!lContinue && feed.size() < 50 /*max last mixed events*/ || lContinue && feed.size() < 500) {
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
	// else return;

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
			else return;
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/makeBuzzfeedLikeItem]: try to add item ") +
				strprintf("buzzer = %s/%s, %s/%s#", lBuzzer->id().toHex(), lTxPublisher.toHex(), lBuzzTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			int lContext = 0;
			makeBuzzfeedItem(lContext, lBuzzer, lBuzzTx, mainStore, buzzFeed, buzzes, subscriber);

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
			else return;

			//
			BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
			prepareBuzzfeedItem(*lItem, lBuzz, lBuzzer);
			lItem->setType(TX_BUZZ_LIKE);
			lItem->setOrder(lLike->timestamp());
			lItem->addItemInfo(BuzzfeedItem::ItemInfo(
				lLike->timestamp(), lLike->score(), lPublisherId, lLike->buzzerInfoChain(), lLike->buzzerInfo(), lLike->chain(), lLike->id(),
				lLike->signature()
			));

			if (!subscriber.isNull()) {
				// try likes, rebuzzes by subscriber
				db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*liker*/, uint256 /*like_tx*/>::Iterator lLikeIdx = 
					likesIdx_.find(lBuzz->id(), subscriber);
				//
				if (lLikeIdx.valid()) {
					lItem->setLike(); // liked by subscriber
				}

				db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*liker*/, uint256 /*like_tx*/>::Iterator lRebuzzIdx = 
					rebuzzesIdx_.find(lBuzz->id(), subscriber);
				//
				if (lRebuzzIdx.valid()) {
					lItem->setRebuzz(); // rebuzzed by subscriber
				}
			}

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
	// else return;

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
			// else return;

			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/makeBuzzfeedRewardItem]: try to add item ") +
				strprintf("publisher = %s, %s/%s#", lTxPublisher.toHex(), lBuzzTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			int lContext = 0;
			makeBuzzfeedItem(lContext, lBuzzer, lBuzzTx, mainStore, buzzFeed, buzzes, subscriber);

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
			// else return;

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
				strprintf("(%d/%d), %s/%s#", buzzes.size(), buzzFeed.size(), lBuzz->id().toHex(), store_->chain().toHex().substr(0, 10)));

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
	// else return;

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
	if (!subscriber.isNull() && lBuzzTx) {
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
			// else return;
			//
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/makeBuzzfeedRebuzzItem]: try to add item ") +
				strprintf("publisher = %s, %s/%s#", lTxPublisher.toHex(), lBuzzTx->id().toHex(), store_->chain().toHex().substr(0, 10)));

			int lContext = 0;
			makeBuzzfeedItem(lContext, lBuzzer, lBuzzTx, mainStore, buzzFeed, buzzes, subscriber);

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
			// else return;

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

			if (!subscriber.isNull()) {
				// try likes, rebuzzes by subscriber
				db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*liker*/, uint256 /*like_tx*/>::Iterator lLikeIdx = 
					likesIdx_.find(lBuzz->id(), subscriber);
				//
				if (lLikeIdx.valid()) {
					lItem->setLike(); // liked by subscriber
				}

				db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*liker*/, uint256 /*like_tx*/>::Iterator lRebuzzIdx = 
					rebuzzesIdx_.find(lBuzz->id(), subscriber);
				//
				if (lRebuzzIdx.valid()) {
					lItem->setRebuzz(); // rebuzzed by subscriber
				}
			}

			//
			buzzFeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
			buzzes.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));
			
			if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: rebuzzed item added ") +
				strprintf("(%d/%d), %s/%s#", buzzes.size(), buzzFeed.size(), lBuzz->id().toHex(), store_->chain().toHex().substr(0, 10)));

		}
	} else {
		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/selectBuzzfeed]: buzz tx was not found ") +
			strprintf("%s/%s#", lBuzzId.toHex(), store_->chain().toHex().substr(0, 10)));
	}
}

BuzzfeedItemPtr BuzzerTransactionStoreExtension::makeBuzzfeedItem(int& context, TxBuzzerPtr buzzer, TransactionPtr tx, ITransactionStorePtr mainStore, std::multimap<uint64_t, BuzzfeedItem::Key>& rawBuzzfeed, std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>& buzzes, const uint256& subscriber, bool expand) {
	//
	if (tx->type() == TX_BUZZ ||
		tx->type() == TX_REBUZZ ||
		tx->type() == TX_BUZZ_REPLY ||
		tx->type() == TX_BUZZER_MESSAGE ||
		tx->type() == TX_BUZZER_MESSAGE_REPLY) {
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
		if (context > 100) return nullptr;

		//
		BuzzfeedItemPtr lItem = BuzzfeedItem::instance();
		prepareBuzzfeedItem(*lItem, lBuzz, buzzer);

		if (!subscriber.isNull()) {
			// try likes, rebuzzes by subscriber
			db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*liker*/, uint256 /*like_tx*/>::Iterator lLikeIdx = 
				likesIdx_.find(lBuzz->id(), subscriber);
			//
			if (lLikeIdx.valid()) {
				lItem->setLike(); // liked by subscriber
			}

			db::DbTwoKeyContainer<uint256 /*buzz|rebuzz|reply*/, uint256 /*liker*/, uint256 /*like_tx*/>::Iterator lRebuzzIdx = 
				rebuzzesIdx_.find(lBuzz->id(), subscriber);
			//
			if (lRebuzzIdx.valid()) {
				lItem->setRebuzz(); // rebuzzed by subscriber
			}
		}

		if (tx->type() == TX_BUZZ_REPLY || tx->type() == TX_BUZZER_MESSAGE_REPLY) {
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
				else return nullptr;

				// add buzz
				makeBuzzfeedItem(++context, lBuzzer, lOriginalTx, mainStore, rawBuzzfeed, buzzes, subscriber);
			}
		} else 	if (tx->type() == TX_REBUZZ) {
			// NOTE: source buzz can be in different chain, so just add info for postponed loading
			TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lBuzz);
			lItem->setOriginalBuzzId(lRebuzz->buzzId());
			lItem->setOriginalBuzzChainId(lRebuzz->buzzChainId());
		}

		rawBuzzfeed.insert(std::multimap<uint64_t, BuzzfeedItem::Key>::value_type(lItem->actualOrder(), lItem->key()));
		buzzes.insert(std::map<BuzzfeedItem::Key, BuzzfeedItemPtr>::value_type(lItem->key(), lItem));

		if (gLog().isEnabled(Log::STORE)) gLog().write(Log::STORE, std::string("[extension/makeBuzzfeedItem]: item added ") +
			strprintf("(%d/%d), %d/%s/%s#", buzzes.size(), rawBuzzfeed.size(), lBuzz->timestamp(), lBuzz->id().toHex(), store_->chain().toHex().substr(0, 10)));

		return lItem;
	}

	return nullptr;
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
