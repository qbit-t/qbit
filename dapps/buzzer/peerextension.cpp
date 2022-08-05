#include "peerextension.h"
#include "../../peer.h"
#include "../../log/log.h"

#include "txbuzzer.h"
#include "txbuzz.h"
#include "txrebuzz.h"
#include "txrebuzznotify.h"
#include "txbuzzreply.h"
#include "txbuzzlike.h"
#include "txbuzzhide.h"
#include "txbuzzreward.h"
#include "txbuzzerendorse.h"
#include "txbuzzermistrust.h"
#include "txbuzzersubscribe.h"
#include "txbuzzerconversation.h"
#include "txbuzzeracceptconversation.h"
#include "txbuzzerdeclineconversation.h"
#include "txbuzzermessage.h"
#include "txbuzzermessagereply.h"

using namespace qbit;

void BuzzerPeerExtension::prepareBuzzfeedItem(BuzzfeedItem& item, TxBuzzPtr buzz, TxBuzzerPtr buzzer, BuzzerTransactionStoreExtensionPtr extension) {
	//
	item.setType(buzz->type());
	item.setBuzzId(buzz->id());
	item.setBuzzChainId(buzz->chain());
	item.setBuzzerInfoId(buzz->buzzerInfo());
	item.setBuzzerInfoChainId(buzz->buzzerInfoChain());
	item.setTimestamp(buzz->timestamp());
	item.setScore(buzz->score());
	item.setBuzzBody(buzz->body());
	item.setBuzzMedia(buzz->mediaPointers());
	item.setSignature(buzz->signature());

	if (buzzer) {
		item.setBuzzerId(buzzer->id());
	}

	// read info
	BuzzerTransactionStoreExtension::BuzzInfo lInfo;
	if (extension->readBuzzInfo(buzz->id(), lInfo)) {
		item.setReplies(lInfo.replies_);
		item.setRebuzzes(lInfo.rebuzzes_);
		item.setLikes(lInfo.likes_);
		item.setRewards(lInfo.rewards_);
	}
}

void BuzzerPeerExtension::prepareEventsfeedItem(EventsfeedItem& item, TxBuzzPtr buzz, TxBuzzerPtr buzzer, 
		const uint256& eventChainId, const uint256& eventId, uint64_t eventTimestamp, 
		const uint256& buzzerInfoChain, const uint256& buzzerInfo, uint64_t score, const uint512& signature) {
	//
	if (buzz) {
		item.setType(buzz->type());
		item.setTimestamp(buzz->timestamp());
		item.setBuzzId(buzz->id());
		item.setBuzzChainId(buzz->chain());
	}

	if (buzz && buzz->type() != TX_REBUZZ) {
		item.setBuzzBody(buzz->body());
		item.setBuzzMedia(buzz->mediaPointers());
		item.setSignature(buzz->signature());
	}

	if (buzzer) {
		EventsfeedItem::EventInfo lInfo(eventTimestamp, buzzer->id(), buzzerInfoChain, buzzerInfo, score);

		if (buzz && (buzz->type() == TX_BUZZ || buzz->type() == TX_BUZZ_REPLY || 
						buzz->type() == TX_REBUZZ || buzz->type() == TX_BUZZER_MESSAGE || buzz->type() == TX_BUZZER_MESSAGE_REPLY)) {
			lInfo.setEvent(eventChainId, eventId, buzz->body(), buzz->mediaPointers(), signature);
		} else {
			lInfo.setEvent(eventChainId, eventId, signature);
		}

		item.addEventInfo(lInfo);
	}
}

bool BuzzerPeerExtension::haveBuzzSubscription(TransactionPtr tx, uint512& signature) {
	//
	if (tx->type() == TX_BUZZ_REPLY) {
		//
		uint256 lOriginalId = tx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
		{
			boost::unique_lock<boost::recursive_mutex> lLock(notificationMutex_);
			std::map<uint256 /*buzz_id*/, uint512 /*signature*/>::iterator lSubscription = buzzSubscriptions_.find(lOriginalId);
			if (lSubscription != buzzSubscriptions_.end()) {
				//
				signature = lSubscription->second;
				return true;
			}
		}
	}

	return false;
}

bool BuzzerPeerExtension::processBuzzCommon(TransactionContextPtr ctx, bool processHide, uint64_t timestamp, const uint512& signature) {
	//
	if (ctx->tx()->type() == TX_BUZZ ||
		ctx->tx()->type() == TX_REBUZZ ||
		ctx->tx()->type() == TX_BUZZ_REPLY) {
		//
		if (gLog().isEnabled(Log::NET)) 
			gLog().write(Log::NET, strprintf("[peer/buzzer/extension]: processing for %s type %s, tx %s/%s#", dapp_.instance().toHex(), ctx->tx()->name(), ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));

		// check subscription
		TransactionPtr lTx = ctx->tx();
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		// publisher
		uint256 lPublisher = (*lTx->in().begin()).out().tx(); // allways first
		uint256 lOriginalPublisher = lPublisher;

		if (lStore && lStore->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());
			//
			uint512 lSignature;
			bool lHasDynamicSubscription = haveBuzzSubscription(ctx->tx(), lSignature);
			if (lExtension->checkSubscription(dapp_.instance(), lPublisher) || 
					dapp_.instance() == lPublisher ||
					lHasDynamicSubscription) {
				//
				TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lTx);

				BuzzfeedItem lItem;
				TxBuzzerPtr lBuzzer = nullptr;

				TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lPublisher);
				if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
				//else return true;

				prepareBuzzfeedItem(lItem, lBuzz, lBuzzer, lExtension);

				//
				bool lNotify = true;
				if (ctx->tx()->type() == TX_BUZZ_REPLY) {
					// NOTE: should be in the same chain
					uint256 lOriginalId = lBuzz->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
					lItem.setOriginalBuzzId(lOriginalId);

					if (lHasDynamicSubscription) {
						lItem.setSource(BuzzfeedItem::Source::BUZZ_SUBSCRIPTION);
						lItem.setSubscriptionSignature(lSignature);
					}
				} else if (ctx->tx()->type() == TX_REBUZZ) {
					// NOTE: can be in the different chain 
					TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lBuzz);
					if (!lRebuzz->simpleRebuzz()) {
						lItem.setOriginalBuzzId(lRebuzz->buzzId());
						lItem.setOriginalBuzzChainId(lRebuzz->buzzChainId());
						//
						/*
						lItem.setRebuzzes(lItem.rebuzzes() + 1); // increment rebuzzes
						lItem.addItemInfo(BuzzfeedItem::ItemInfo(
							lRebuzz->timestamp(), lRebuzz->score(), lPublisher, lRebuzz->buzzerInfoChain(), lRebuzz->buzzerInfo(), lRebuzz->chain(), lRebuzz->id(),
							lRebuzz->signature()
						));
						*/

					} else {
						lNotify = false; // will be done at processRebuzz
					}

					/*
					BuzzerTransactionStoreExtension::BuzzInfo lInfo;
					lExtension->readBuzzInfo(lRebuzz->buzzId(), lInfo);
					BuzzfeedItemUpdate lUpdateItem(lRebuzz->buzzId(), lRebuzz->id(), BuzzfeedItemUpdate::REBUZZES, lInfo.rebuzzes_+1);
					std::vector<BuzzfeedItemUpdate> lUpdates; lUpdates.push_back(lUpdateItem);
					notifyUpdateBuzz(lUpdates);
					*/
				}

				// send
				if (lNotify) {
					if (processHide) {
						// clean up
						lItem.setType(TX_BUZZ_HIDE);
						lItem.setBuzzBody(std::vector<unsigned char>());
						lItem.setBuzzMedia(std::vector<BuzzerMediaPointer>());
						lItem.setTimestamp(timestamp);
						lItem.setSignature(signature);
					}

					notifyNewBuzz(lItem);
				}
			}
				
			//
			TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lTx);
			std::vector<Transaction::In>& lIns = lBuzz->in(); 
			std::vector<Transaction::In>::iterator lInPtr = lIns.begin(); lInPtr++; // buzzer pointer
			for(; lInPtr != lIns.end(); lInPtr++) {
				//
				Transaction::In& lIn = (*lInPtr);
				ITransactionStorePtr lLocalStore = peerManager_->consensusManager()->storeManager()->locate(lIn.out().chain());
				if (!lLocalStore) continue;
				TransactionPtr lInTx = lLocalStore->locateTransaction(lIn.out().tx());

				if (lInTx != nullptr) {
					//
					// next level - parents
					if (lInTx->type() == TX_BUZZER) {
						// direct parent
						// mentions - when subscriber (peer) was mentioned (replied/rebuzzed direcly)
						// -> new item
						if (dapp_.instance() == lInTx->id()) {
							//
							BuzzfeedItem lItem;
							TxBuzzerPtr lBuzzer = nullptr;

							TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lPublisher);
							if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
							//else continue;

							prepareBuzzfeedItem(lItem, lBuzz, lBuzzer, lExtension);

							//
							if (ctx->tx()->type() == TX_BUZZ_REPLY) {
								// NOTE: should be in the same chain
								uint256 lOriginalId = lBuzz->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
								lItem.setOriginalBuzzId(lOriginalId);
							} else if (ctx->tx()->type() == TX_REBUZZ) {
								// NOTE: can be in the different chain
								// TODO: that's just a mention - do we need to notify?
								TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lBuzz);
								lItem.setOriginalBuzzId(lRebuzz->buzzId());
								lItem.setOriginalBuzzChainId(lRebuzz->buzzChainId());
							}

							if (processHide) {
								// clean up
								lItem.setType(TX_BUZZ_HIDE);
								lItem.setBuzzBody(std::vector<unsigned char>());
								lItem.setBuzzMedia(std::vector<BuzzerMediaPointer>());
								lItem.setTimestamp(timestamp);
								lItem.setSignature(signature);
							}

							notifyNewBuzz(lItem);
						}
					} else if (lInTx->type() == TX_BUZZ || lInTx->type() == TX_REBUZZ || lInTx->type() == TX_BUZZ_REPLY) {
						//
						std::vector<BuzzfeedItemUpdate> lUpdates;
						lPublisher = (*lInTx->in().begin()).out().tx(); // allways first

						// check
						lHasDynamicSubscription = haveBuzzSubscription(lInTx, lSignature);
						// if subscriber (peer) has subscription on this new publisher
						if (lExtension->checkSubscription(dapp_.instance(), lPublisher) || 
																dapp_.instance() == lPublisher ||
																dapp_.instance() == lOriginalPublisher ||
																lHasDynamicSubscription) {

							uint256 lReTxId = lInTx->id();
							uint256 lReChainId = lInTx->chain();

							BuzzerTransactionStoreExtension::BuzzInfo lInfo;
							lExtension->readBuzzInfo(lReTxId, lInfo);
							if (ctx->tx()->type() == TX_BUZZ_REPLY) {
								BuzzfeedItemUpdate lItem(lReTxId, ctx->tx()->id(), BuzzfeedItemUpdate::REPLIES, lInfo.replies_+1);
								lUpdates.push_back(lItem);
							} else if (ctx->tx()->type() == TX_REBUZZ) {
								BuzzfeedItemUpdate lItem(lReTxId, ctx->tx()->id(), BuzzfeedItemUpdate::REBUZZES, lInfo.rebuzzes_+1);
								lUpdates.push_back(lItem);
							}
						}

						// if we have indirect subscription
						if (lHasDynamicSubscription && ctx->tx()->type() == TX_BUZZ_REPLY) {
							//
							TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lTx);

							BuzzfeedItem lItem;
							TxBuzzerPtr lBuzzer = nullptr;

							TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lOriginalPublisher);
							if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
							//else continue;

							prepareBuzzfeedItem(lItem, lBuzz, lBuzzer, lExtension);

							uint256 lOriginalId = lBuzz->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
							lItem.setOriginalBuzzId(lOriginalId);
							lItem.setSource(BuzzfeedItem::Source::BUZZ_SUBSCRIPTION);
							lItem.setSubscriptionSignature(lSignature);

							if (processHide) {
								// clean up
								lItem.setType(TX_BUZZ_HIDE);
								lItem.setBuzzBody(std::vector<unsigned char>());
								lItem.setBuzzMedia(std::vector<BuzzerMediaPointer>());
								lItem.setTimestamp(timestamp);
								lItem.setSignature(signature);
							}

							notifyNewBuzz(lItem);
						}

						if (lInTx->type() == TX_BUZZ_REPLY) {
							//
							uint256 lReTxId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
							uint256 lReChainId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
							//
							while (true) {
								//
								lLocalStore = peerManager_->consensusManager()->storeManager()->locate(lReChainId);
								if (!lLocalStore) break;
								TransactionPtr lReTx = lLocalStore->locateTransaction(lReTxId);

								if (lReTx && (lReTx->type() == TX_BUZZ_REPLY || lReTx->type() == TX_REBUZZ ||
																						lReTx->type() == TX_BUZZ)) {

									lHasDynamicSubscription = haveBuzzSubscription(lReTx, lSignature);
									lPublisher = (*lInTx->in().begin()).out().tx();
									// make update
									if (lExtension->checkSubscription(dapp_.instance(), lPublisher) || 
																			dapp_.instance() == lPublisher || 
																			dapp_.instance() == lOriginalPublisher || 
																			lHasDynamicSubscription) {
										BuzzerTransactionStoreExtension::BuzzInfo lInfo;
										lExtension->readBuzzInfo(lReTxId, lInfo);
										// if we have one
										if (ctx->tx()->type() == TX_BUZZ_REPLY) {
											BuzzfeedItemUpdate lItem(lReTxId, ctx->tx()->id(), BuzzfeedItemUpdate::REPLIES, lInfo.replies_+1);
											lUpdates.push_back(lItem);
										} else if (ctx->tx()->type() == TX_REBUZZ) {
											BuzzfeedItemUpdate lItem(lReTxId, ctx->tx()->id(), BuzzfeedItemUpdate::REBUZZES, lInfo.rebuzzes_+1);
											lUpdates.push_back(lItem);
										}
									}

									// if have indirect subscription
									if (lHasDynamicSubscription && ctx->tx()->type() == TX_BUZZ_REPLY) {
										//
										TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lTx);

										BuzzfeedItem lItem;
										TxBuzzerPtr lBuzzer = nullptr;

										TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lOriginalPublisher);
										if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
										//else break;

										prepareBuzzfeedItem(lItem, lBuzz, lBuzzer, lExtension);

										uint256 lOriginalId = lBuzz->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
										lItem.setOriginalBuzzId(lOriginalId);
										lItem.setSource(BuzzfeedItem::Source::BUZZ_SUBSCRIPTION);
										lItem.setSubscriptionSignature(lSignature);

										if (processHide) {
											// clean up
											lItem.setType(TX_BUZZ_HIDE);
											lItem.setBuzzBody(std::vector<unsigned char>());
											lItem.setBuzzMedia(std::vector<BuzzerMediaPointer>());
											lItem.setTimestamp(timestamp);
											lItem.setSignature(signature);
										}

										notifyNewBuzz(lItem);
									}

									if (lReTx->type() == TX_BUZZ || lReTx->type() == TX_REBUZZ) { 
										break; // search done
									}

									if (lReTx->in().size() > TX_BUZZ_REPLY_BUZZ_IN) { // move up-next
										lReTxId = lReTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
										lReChainId = lReTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
									}
								} else break; // search done
							}
						}

						if (lUpdates.size() && !processHide) {
							notifyUpdateBuzz(lUpdates);
						}
					}
				}
			}
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::processBuzzHide(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZ_HIDE) {
		//
		// check subscription
		TransactionPtr lTx = ctx->tx();
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());		
		// publisher
		uint256 lOriginalPublisher = (*lTx->in().begin()).out().tx(); // allways first
		// process
		if (lStore && lStore->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());
			//
			TxBuzzHidePtr lHideTx = TransactionHelper::to<TxBuzzHide>(lTx);
			// load buzz
			TransactionPtr lBuzz = lStore->locateTransaction(lTx->in()[TX_BUZZ_HIDE_IN].out().tx());
			if (lBuzz) {
				// make context
				TransactionContextPtr lUpdateContext = TransactionContext::instance(lBuzz);
				// re-process to notify subscribers about buzz hiding
				if (lBuzz->type() != TX_BUZZER_MESSAGE && lBuzz->type() != TX_BUZZER_MESSAGE_REPLY)
					processBuzzCommon(lUpdateContext, true, lHideTx->timestamp(), lHideTx->signature());
				else
					processConversationMessage(lUpdateContext, true, lHideTx->timestamp(), lHideTx->signature());
			}
		}

		return true;
	}

	return false;
}


bool BuzzerPeerExtension::processBuzzLike(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZ_LIKE) {
		//
		// check subscription
		TransactionPtr lTx = ctx->tx();
		// like tx
		TxBuzzLikePtr lLike = TransactionHelper::to<TxBuzzLike>(lTx);
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		// publisher
		uint256 lPublisher;
		// initiator
		uint256 lInitiator;

		// process
		if (lStore && lStore->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			//
			// if we have subscription on original publisher - increment\control like count in feed
			// 
			bool lSent = false;
			TransactionPtr lBuzz = lStore->locateTransaction(lTx->in()[TX_BUZZ_LIKE_IN].out().tx());
			if (lBuzz) {
				TxBuzzPtr lBuzzTx = TransactionHelper::to<TxBuzz>(lBuzz);
				lPublisher = lBuzzTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer
				lInitiator = lTx->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator

				if (lPublisher != lInitiator) {
					//
					if (lExtension->checkSubscription(dapp_.instance(), lPublisher) ||
						//lExtension->checkSubscription(lPublisher, dapp_.instance()) || 
						dapp_.instance() == lPublisher || 
						dapp_.instance() == lInitiator) {
						//
						if (!lExtension->checkLike(lTx->in()[TX_BUZZ_LIKE_IN].out().tx(), lTx->in()[TX_BUZZ_MY_BUZZER_IN].out().tx())) {
							// make update
							BuzzerTransactionStoreExtension::BuzzInfo lInfo;
							lExtension->readBuzzInfo(lBuzz->id(), lInfo);

							// if we have one
							BuzzfeedItemUpdate lItem(lBuzz->id(), lLike->id(), BuzzfeedItemUpdate::LIKES, lInfo.likes_+1);
							std::vector<BuzzfeedItemUpdate> lUpdates; lUpdates.push_back(lItem);
							notifyUpdateBuzz(lUpdates);
							
							// make update
							if (dapp_.instance() == lPublisher) {
								// WARNING: this breaks current logic
								/*
								BuzzerTransactionStoreExtension::BuzzerInfo lScore;
								lExtension->readBuzzerStat(lPublisher, lScore);
								lScore.incrementEndorsements(BUZZER_ENDORSE_LIKE);
								notifyUpdateTrustScore(lScore);
								*/
							}
						}

						lSent = true;
					}
				}
			}

			//
			// if we have direct subscription on liker
			//
			lPublisher = (*lTx->in().begin()).out().tx(); // allways first
			//
			if (!lSent && lBuzz && (lExtension->checkSubscription(dapp_.instance(), lPublisher) ||
																	dapp_.instance() == lPublisher)) {
				//
				TxBuzzPtr lBuzzTx = TransactionHelper::to<TxBuzz>(lBuzz);
				uint256 lBuzzerId = lBuzz->in()[TX_BUZZ_MY_BUZZER_IN].out().tx();

				if (lBuzzerId != lPublisher) {
					//
					TxBuzzerPtr lBuzzer = nullptr;
					TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);
					if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
					//else return true;

					if (!lExtension->checkLike(lTx->in()[TX_BUZZ_LIKE_IN].out().tx(), lTx->in()[TX_BUZZ_MY_BUZZER_IN].out().tx())) {
						//
						BuzzfeedItem lItem;
						prepareBuzzfeedItem(lItem, lBuzzTx, lBuzzer, lExtension); // read buzz info
						lItem.setType(TX_BUZZ_LIKE);
						lItem.setOrder(lLike->timestamp());
						lItem.setLikes(lItem.likes() + 1); // increment likes
						lItem.addItemInfo(BuzzfeedItem::ItemInfo(
							lLike->timestamp(), lLike->score(), lPublisher, lLike->buzzerInfoChain(), lLike->buzzerInfo(), lLike->chain(), lLike->id(),
							lLike->signature()
						));

						// send
						notifyNewBuzz(lItem);
					}
				}
			}
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::processBuzzReward(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZ_REWARD) {
		//
		// check subscription
		TransactionPtr lTx = ctx->tx();
		// reward tx
		TxBuzzRewardPtr lReward = TransactionHelper::to<TxBuzzReward>(lTx);
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		// publisher
		uint256 lPublisher;
		// initiator
		uint256 lInitiator;

		// process
		if (lStore && lStore->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			//
			// if we have subscription on original publisher - increment\control rewards in feed
			// 
			bool lSent = false;
			TransactionPtr lBuzz = lStore->locateTransaction(lTx->in()[TX_BUZZ_REWARD_IN].out().tx());
			if (lBuzz) {
				TxBuzzPtr lBuzzTx = TransactionHelper::to<TxBuzz>(lBuzz);
				lPublisher = lBuzzTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer
				lInitiator = lTx->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator

				if (lPublisher != lInitiator) {
					//
					if (lExtension->checkSubscription(dapp_.instance(), lPublisher) ||
						//lExtension->checkSubscription(lPublisher, dapp_.instance()) || 
						dapp_.instance() == lPublisher || 
						dapp_.instance() == lInitiator) {
						// make update
						BuzzerTransactionStoreExtension::BuzzInfo lInfo;
						lExtension->readBuzzInfo(lBuzz->id(), lInfo);

						// if we have one
						BuzzfeedItemUpdate lItem(lBuzz->id(), lReward->id(), BuzzfeedItemUpdate::REWARDS, lInfo.rewards_ + lReward->amount());
						std::vector<BuzzfeedItemUpdate> lUpdates; lUpdates.push_back(lItem);
						notifyUpdateBuzz(lUpdates);
						
						// make update
						if (dapp_.instance() == lPublisher) {
							/*
							BuzzerTransactionStoreExtension::BuzzerInfo lScore;
							lExtension->readBuzzerStat(lPublisher, lScore);
							lScore.incrementEndorsements(BUZZER_ENDORSE_LIKE);
							notifyUpdateTrustScore(lScore);
							*/
						}

						lSent = true;
					}
				}
			}

			//
			// if we have direct subscription on rewardes
			//
			lPublisher = (*lTx->in().begin()).out().tx(); // allways first
			//
			if (!lSent && lBuzz && (lExtension->checkSubscription(dapp_.instance(), lPublisher) ||
																	dapp_.instance() == lPublisher)) {
				//
				TxBuzzPtr lBuzzTx = TransactionHelper::to<TxBuzz>(lBuzz);
				uint256 lBuzzerId = lBuzz->in()[TX_BUZZ_MY_BUZZER_IN].out().tx();
				//

				if (lBuzzerId != lPublisher) {
					//
					TxBuzzerPtr lBuzzer = nullptr;
					TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);
					if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
					//else return true;

					//
					BuzzfeedItem lItem;
					prepareBuzzfeedItem(lItem, lBuzzTx, lBuzzer, lExtension); // read buzz info
					lItem.setType(TX_BUZZ_REWARD);
					lItem.setOrder(lReward->timestamp());
					lItem.setRewards(lItem.rewards() + lReward->amount()); // increment rewards
					lItem.addItemInfo(BuzzfeedItem::ItemInfo(
						lReward->timestamp(), lReward->score(), lReward->amount(), lPublisher, lReward->buzzerInfoChain(), lReward->buzzerInfo(), lReward->chain(), lReward->id(),
						lReward->signature()
					));

					// send
					notifyNewBuzz(lItem);
				}
			}
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::processRebuzz(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_REBUZZ) {
		//
		// check subscription
		TransactionPtr lTx = ctx->tx();

		// rebuzz tx
		TxReBuzzPtr lRebuzz = TransactionHelper::to<TxReBuzz>(lTx);
		uint256 lBuzzId = lRebuzz->buzzId();
		uint256 lBuzzChainId = lRebuzz->buzzChainId();

		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lBuzzChainId);
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		// publisher
		uint256 lPublisher;
		// initiator
		uint256 lInitiator;

		// process
		if (lStore && lStore->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			//
			// if we have subscription on original publisher - increment\control like count in feed
			// 
			bool lSent = false;
			TransactionPtr lBuzz = lStore->locateTransaction(lBuzzId);
			if (lBuzz) {
				TxBuzzPtr lBuzzTx = TransactionHelper::to<TxBuzz>(lBuzz);
				lPublisher = lBuzzTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer
				lInitiator = lTx->in()[TX_BUZZ_MY_BUZZER_IN].out().tx(); // initiator

				if (lExtension->checkSubscription(dapp_.instance(), lPublisher) ||
					//lExtension->checkSubscription(lPublisher, dapp_.instance()) || 
					dapp_.instance() == lPublisher || 
					dapp_.instance() == lInitiator) {

					BuzzerTransactionStoreExtension::BuzzInfo lInfo;
					lExtension->readBuzzInfo(lRebuzz->buzzId(), lInfo);
					BuzzfeedItemUpdate lUpdateItem(lRebuzz->buzzId(), lRebuzz->id(), BuzzfeedItemUpdate::REBUZZES, lInfo.rebuzzes_+1);
					std::vector<BuzzfeedItemUpdate> lUpdates; lUpdates.push_back(lUpdateItem);
					notifyUpdateBuzz(lUpdates);

					// update score
					if (dapp_.instance() == lPublisher) {
						// WARNING: breaks current logic
						/*
						BuzzerTransactionStoreExtension::BuzzerInfo lScore;
						lExtension->readBuzzerStat(lRebuzz->buzzerId(), lScore);
						lScore.incrementEndorsements(BUZZER_ENDORSE_REBUZZ);
						notifyUpdateTrustScore(lScore);
						*/
					}

					lSent = true;
				}
			}

			//
			// if we have direct subscription
			//
			if (/*!lSent &&*/ lBuzz && lRebuzz->simpleRebuzz()) {
				//
				lPublisher = (*lTx->in().begin()).out().tx(); // allways first
				// original store
				ITransactionStorePtr lOriginalStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
				if (!lOriginalStore) return true;
				BuzzerTransactionStoreExtensionPtr lOriginalExtension = 
					std::static_pointer_cast<BuzzerTransactionStoreExtension>(lOriginalStore->extension());

				//
				if (lOriginalExtension->checkSubscription(dapp_.instance(), lPublisher) || 
							lExtension->checkSubscription(dapp_.instance(), lPublisher) ||
								dapp_.instance() == lPublisher) {
					//
					TxBuzzPtr lBuzzTx = TransactionHelper::to<TxBuzz>(lBuzz);
					uint256 lBuzzerId = lBuzz->in()[TX_BUZZ_MY_BUZZER_IN].out().tx();
					//
					TxBuzzerPtr lBuzzer = nullptr;
					TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);
					if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
					// else return true;

					//
					BuzzfeedItem lItem;
					prepareBuzzfeedItem(lItem, lBuzzTx, lBuzzer, lExtension); // read buzz info
					lItem.setType(TX_REBUZZ);
					lItem.setOrder(lRebuzz->timestamp());
					lItem.setRebuzzes(lItem.rebuzzes() + 1); // increment rebuzzes
					lItem.addItemInfo(BuzzfeedItem::ItemInfo(
						lRebuzz->timestamp(), lRebuzz->score(), lPublisher, lRebuzz->buzzerInfoChain(), lRebuzz->buzzerInfo(), lRebuzz->chain(), lRebuzz->id(),
						lRebuzz->signature()
					));

					// send
					notifyNewBuzz(lItem);
				}
			}
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::processEventCommon(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZ ||
		ctx->tx()->type() == TX_REBUZZ ||
		ctx->tx()->type() == TX_BUZZ_REPLY) {
		// check subscription
		TransactionPtr lTx = ctx->tx();
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		// publisher
		uint256 lOriginalPublisher = (*lTx->in().begin()).out().tx(); // allways first
		//
		TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lTx);

		// direct re-buzz of own buzz
		if (ctx->tx()->type() == TX_REBUZZ) {
			//
			TxReBuzzPtr lReBuzz = TransactionHelper::to<TxReBuzz>(lTx);
			if (dapp_.instance() == lReBuzz->buzzerId()) { // if original buzzer_id current id
				//
				EventsfeedItem lItem;
				TxBuzzerPtr lBuzzer = nullptr;
				TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lOriginalPublisher);
				if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);

				prepareEventsfeedItem(lItem, lBuzz, lBuzzer, lTx->chain(), lTx->id(), 
					lBuzz->timestamp(), lBuzz->buzzerInfoChain(), lBuzz->buzzerInfo(), lBuzz->score(), lBuzz->signature());
				lItem.setBuzzId(lReBuzz->buzzId());
				lItem.setBuzzChainId(lReBuzz->buzzChainId());
				notifyNewEvent(lItem);
			}
		}

		// scan ins
		std::vector<Transaction::In>& lIns = lBuzz->in(); 
		std::vector<Transaction::In>::iterator lInPtr = lIns.begin(); lInPtr++; // buzzer pointer
		for(; lInPtr != lIns.end(); lInPtr++) {
			//
			Transaction::In& lIn = (*lInPtr);
			ITransactionStorePtr lLocalStore = peerManager_->consensusManager()->storeManager()->locate(lIn.out().chain());
			if (!lLocalStore) continue;

			TransactionPtr lInTx = lLocalStore->locateTransaction(lIn.out().tx());

			if (lInTx != nullptr) {
				//
				// next level - parents
				if (lInTx->type() == TX_BUZZER) {
					// direct parent
					// mentions - when subscriber (peer) was mentioned (replied/rebuzzed direcly)
					// -> new item
					if (dapp_.instance() == lInTx->id()) {
						//
						EventsfeedItem lItem;
						TxBuzzerPtr lBuzzer = nullptr;
						TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lOriginalPublisher);
						if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
						//else continue;

						// TODO: need to somehow identify that is the mention
						prepareEventsfeedItem(lItem, lBuzz, lBuzzer, lBuzz->chain(), lBuzz->id(), 
							lBuzz->timestamp(), lBuzz->buzzerInfoChain(), lBuzz->buzzerInfo(), lBuzz->score(), lBuzz->signature()); // just mention in re-buzz/buzz/reply, for example

						if (ctx->tx()->type() == TX_REBUZZ) {
							TxReBuzzPtr lReBuzz = TransactionHelper::to<TxReBuzz>(lTx);
							lItem.setBuzzId(lReBuzz->buzzId());
							lItem.setBuzzChainId(lReBuzz->buzzChainId());
							lItem.setMention();
						}

						notifyNewEvent(lItem);
						break;
					}
				} else if (lInTx->type() == TX_BUZZ || lInTx->type() == TX_REBUZZ || lInTx->type() == TX_BUZZ_REPLY) {
					//
					uint256 lInnerPublisher = (*lInTx->in().begin()).out().tx(); // allways first

					// check
					// if subscriber (peer) has subscription on this new publisher
					if (dapp_.instance() == lInnerPublisher && 
							lOriginalPublisher != lInnerPublisher /*self-notification*/) {
						//
						EventsfeedItem lItem;
						TxBuzzerPtr lBuzzer = nullptr;
						TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lOriginalPublisher);
						if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
						//else continue;

						prepareEventsfeedItem(lItem, lBuzz, lBuzzer, lBuzz->chain(), lBuzz->id(), 
							lBuzz->timestamp(), lBuzz->buzzerInfoChain(), lBuzz->buzzerInfo(), lBuzz->score(), lBuzz->signature());
						notifyNewEvent(lItem);
						break;
					}

					if (ctx->tx()->type() == TX_BUZZ_REPLY) {
						//
						uint256 lReTxId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
						uint256 lReChainId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
						//
						while (true) {
							//
							lLocalStore = peerManager_->consensusManager()->storeManager()->locate(lReChainId);
							if (!lLocalStore) break;

							TransactionPtr lReTx = lLocalStore->locateTransaction(lReTxId);

							if (lReTx && (lReTx->type() == TX_BUZZ_REPLY || lReTx->type() == TX_REBUZZ ||
																					lReTx->type() == TX_BUZZ)) {

								lInnerPublisher = (*lReTx->in().begin()).out().tx();
								if (dapp_.instance() == lInnerPublisher &&
										lOriginalPublisher != lInnerPublisher /*self-notification*/) {
									// make event
									EventsfeedItem lItem;
									TxBuzzerPtr lBuzzer = nullptr;
									TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lOriginalPublisher);
									if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
									//else break;

									prepareEventsfeedItem(lItem, lBuzz, lBuzzer, lBuzz->chain(), lBuzz->id(), 
										lBuzz->timestamp(), lBuzz->buzzerInfoChain(), lBuzz->buzzerInfo(), lBuzz->score(), lBuzz->signature());

									if (lReTx->type() != TX_REBUZZ) {
										TxBuzzPtr lInnerBuzz = TransactionHelper::to<TxBuzz>(lReTx);
										lItem.setBuzzBody(lInnerBuzz->body());
										// TODO: signature?
									}

									notifyNewEvent(lItem);
									break;
								}

								if (lReTx->type() == TX_BUZZ || lReTx->type() == TX_REBUZZ) { 
									break; // search done
								}

								if (lReTx->in().size() > TX_BUZZ_REPLY_BUZZ_IN) { // move up-next
									lReTxId = lReTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
									lReChainId = lReTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
								}
							} else break; // search done
						}
					}
				}
			}
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::processEventLike(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZ_LIKE) {
		//
		// check subscription
		TransactionPtr lTx = ctx->tx();
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());		
		// publisher
		uint256 lOriginalPublisher = (*lTx->in().begin()).out().tx(); // allways first
		// process
		if (lStore && lStore->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());
			//
			TxBuzzLikePtr lLikeTx = TransactionHelper::to<TxBuzzLike>(lTx);
			// load buzz
			TransactionPtr lBuzz = lStore->locateTransaction(lTx->in()[TX_BUZZ_LIKE_IN].out().tx());
			if (lBuzz) {
				TxBuzzPtr lBuzzTx = TransactionHelper::to<TxBuzz>(lBuzz);
				uint256 lPublisher = lBuzzTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer

				if (dapp_.instance() == lPublisher) {
					// make update
					EventsfeedItem lItem;
					TxBuzzerPtr lBuzzer = nullptr;
					TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lOriginalPublisher);
					if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
					//else return true;
					//
					prepareEventsfeedItem(lItem, lBuzzTx, lBuzzer, lTx->chain(), lTx->id(), 
						lLikeTx->timestamp(), lLikeTx->buzzerInfoChain(), lLikeTx->buzzerInfo(), lLikeTx->score(), lLikeTx->signature());
					lItem.setType(TX_BUZZ_LIKE);
					notifyNewEvent(lItem);
				}
			}
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::processEventReward(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZ_REWARD) {
		//
		// check subscription
		TransactionPtr lTx = ctx->tx();
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());		
		// publisher
		uint256 lOriginalPublisher = (*lTx->in().begin()).out().tx(); // allways first
		// process
		if (lStore && lStore->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());
			//
			TxBuzzRewardPtr lRewardTx = TransactionHelper::to<TxBuzzReward>(lTx);
			// load buzz
			TransactionPtr lBuzz = lStore->locateTransaction(lTx->in()[TX_BUZZ_REWARD_IN].out().tx());
			if (lBuzz) {
				TxBuzzPtr lBuzzTx = TransactionHelper::to<TxBuzz>(lBuzz);
				uint256 lPublisher = lBuzzTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer

				if (dapp_.instance() == lPublisher) {
					// make update
					EventsfeedItem lItem;
					TxBuzzerPtr lBuzzer = nullptr;
					TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lOriginalPublisher);
					if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
					//else return true;
					//
					prepareEventsfeedItem(lItem, lBuzzTx, lBuzzer, lTx->chain(), lTx->id(), 
						lRewardTx->timestamp(), lRewardTx->buzzerInfoChain(), lRewardTx->buzzerInfo(), lRewardTx->score(), lRewardTx->signature());
					lItem.setType(TX_BUZZ_REWARD);
					lItem.setValue(lRewardTx->amount());
					notifyNewEvent(lItem);
				}
			}
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::processEndorse(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_ENDORSE) {
		//
		if (gLog().isEnabled(Log::NET)) 
			gLog().write(Log::NET, strprintf("[peer/buzzer/extension]: processing for %s type %s, tx %s/%s#", dapp_.instance().toHex(), ctx->tx()->name(), ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));

		// check subscription
		TransactionPtr lTx = ctx->tx();
		//
		TxBuzzerEndorsePtr lEndorse = TransactionHelper::to<TxBuzzerEndorse>(lTx);
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());

		if (lStore && lStore->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			// in[0] - publisher/initiator
			uint256 lEndoser = lTx->in()[TX_BUZZER_ENDORSE_MY_IN].out().tx(); 
			// in[1] - publisher/initiator
			uint256 lBuzzer = lTx->in()[TX_BUZZER_ENDORSE_BUZZER_IN].out().tx(); 

			if (lExtension->checkSubscription(dapp_.instance(), lEndoser) || 
										lBuzzer == dapp_.instance() || 
										lEndoser == dapp_.instance()) {
				//
				BuzzerTransactionStoreExtension::BuzzerInfo lScore;
				lExtension->readBuzzerStat(lBuzzer, lScore);
				//
				if (lMainStore && lTx->in().size() > TX_BUZZER_ENDORSE_FEE_IN) {
					TransactionPtr lFeeTx = lMainStore->locateTransaction(lTx->in()[TX_BUZZER_ENDORSE_FEE_IN].out().tx());
					if (!lFeeTx) return true;
					//
					if (lBuzzer == dapp_.instance()) {
						//
						if (lFeeTx) {
							TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

							amount_t lAmount;
							uint64_t lHeight;
							if (TxBuzzerEndorse::extractLockedAmountAndHeight(lFee, lAmount, lHeight)) {
								//
								lScore.incrementEndorsements(lAmount);
								notifyUpdateTrustScore(lScore);
							}
						} else {
							lScore.incrementEndorsements(lEndorse->amount());
							notifyUpdateTrustScore(lScore);
						}

						// make update
						EventsfeedItem lItem;
						lItem.setType(TX_BUZZER_ENDORSE);
						lItem.setTimestamp(lEndorse->timestamp());
						lItem.setBuzzId(lEndorse->id());
						lItem.setBuzzChainId(lEndorse->chain());
						lItem.setValue(lEndorse->amount());
						lItem.setSignature(lEndorse->signature());
						lItem.setPublisher(lBuzzer);

						EventsfeedItem::EventInfo lInfo(lEndorse->timestamp(), lEndoser, lEndorse->buzzerInfoChain(), lEndorse->buzzerInfo(), lEndorse->score());
						lInfo.setEvent(lEndorse->chain(), lEndorse->id(), lEndorse->signature());
						lItem.addEventInfo(lInfo);

						notifyNewEvent(lItem);

					} else {
						//
						uint256 lBuzzerId = lTx->in()[TX_BUZZER_ENDORSE_BUZZER_IN].out().tx(); 

						TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzer);
						if (!lBuzzerTx) return true;

						ITransactionStorePtr lStore = lMainStore->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
						if (!lStore) return true;

						if (!lStore->extension()) return true;
						BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

						TxBuzzerInfoPtr lInfo = lExtension->readBuzzerInfo(lBuzzerId);
						if (lInfo) {
							//
							BuzzfeedItem lItem;
							lItem.setType(TX_BUZZER_ENDORSE);
							lItem.setBuzzerId(lEndoser);
							lItem.setBuzzId(lEndorse->id());
							lItem.setBuzzerInfoId(lEndorse->buzzerInfo());
							lItem.setBuzzerInfoChainId(lEndorse->buzzerInfoChain());
							lItem.setBuzzChainId(lEndorse->chain());
							lItem.setValue(lEndorse->amount());
							lItem.setTimestamp(lEndorse->timestamp());
							lItem.setScore(lEndorse->score());
							lItem.setSignature(lEndorse->signature());

							lItem.addItemInfo(BuzzfeedItem::ItemInfo(
								lEndorse->timestamp(), lEndorse->score(),
								lBuzzer, lInfo->chain(), lInfo->id(), 
								lEndorse->chain(), lEndorse->id(), lEndorse->signature()
							));						

							notifyNewBuzz(lItem);
						}
					}
				}
			}

			return true;
		}
	}

	return false;
}

bool BuzzerPeerExtension::processMistrust(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_MISTRUST) {
		//
		if (gLog().isEnabled(Log::NET)) 
			gLog().write(Log::NET, strprintf("[peer/buzzer/extension]: processing for %s type %s, tx %s/%s#", dapp_.instance().toHex(), ctx->tx()->name(), ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));

		// check subscription
		TransactionPtr lTx = ctx->tx();
		//
		TxBuzzerMistrustPtr lMistrust = TransactionHelper::to<TxBuzzerMistrust>(lTx);
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());

		if (lStore && lStore->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			// in[0] - publisher/initiator
			uint256 lMistruster = lTx->in()[TX_BUZZER_MISTRUST_MY_IN].out().tx(); 
			// in[1] - publisher/initiator
			uint256 lBuzzer = lTx->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 

			if (lExtension->checkSubscription(dapp_.instance(), lMistruster) || 
														lBuzzer == dapp_.instance() ||
														lMistruster == dapp_.instance()) {
				//
				BuzzerTransactionStoreExtension::BuzzerInfo lScore;
				lExtension->readBuzzerStat(lBuzzer, lScore);
				//
				if (lMainStore && lTx->in().size() > TX_BUZZER_MISTRUST_FEE_IN) {
					TransactionPtr lFeeTx = lMainStore->locateTransaction(lTx->in()[TX_BUZZER_MISTRUST_FEE_IN].out().tx());
					if (!lFeeTx) return true;

					if (lBuzzer == dapp_.instance()) {
						//
						if (lFeeTx) {
							TxFeePtr lFee = TransactionHelper::to<TxFee>(lFeeTx);

							amount_t lAmount;
							uint64_t lHeight;
							if (TxBuzzerMistrust::extractLockedAmountAndHeight(lFee, lAmount, lHeight)) {
								//
								lScore.incrementMistrusts(lAmount);
								notifyUpdateTrustScore(lScore);
							}
						} else {
							lScore.incrementMistrusts(lMistrust->amount());
							notifyUpdateTrustScore(lScore);
						}
						
						// make update
						EventsfeedItem lItem;
						lItem.setType(TX_BUZZER_MISTRUST);
						lItem.setTimestamp(lMistrust->timestamp());
						lItem.setBuzzId(lMistrust->id());
						lItem.setBuzzChainId(lMistrust->chain());
						lItem.setValue(lMistrust->amount());
						lItem.setSignature(lMistrust->signature());
						lItem.setPublisher(lBuzzer);

						EventsfeedItem::EventInfo lInfo(lMistrust->timestamp(), lMistruster, lMistrust->buzzerInfoChain(), lMistrust->buzzerInfo(), lMistrust->score());
						lInfo.setEvent(lMistrust->chain(), lMistrust->id(), lMistrust->signature());
						lItem.addEventInfo(lInfo);

						notifyNewEvent(lItem);

					} else {
						//
						uint256 lBuzzerId = lTx->in()[TX_BUZZER_MISTRUST_BUZZER_IN].out().tx(); 

						TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzer);
						if (!lBuzzerTx) return true;

						ITransactionStorePtr lStore = lMainStore->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
						if (!lStore) return true;

						if (!lStore->extension()) return true;
						BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

						TxBuzzerInfoPtr lInfo = lExtension->readBuzzerInfo(lBuzzerId);
						if (lInfo) {
							//
							BuzzfeedItem lItem;
							lItem.setType(TX_BUZZER_MISTRUST);
							lItem.setBuzzerId(lMistruster);
							lItem.setBuzzId(lMistrust->id());
							lItem.setBuzzerInfoId(lMistrust->buzzerInfo());
							lItem.setBuzzerInfoChainId(lMistrust->buzzerInfoChain());
							lItem.setBuzzChainId(lMistrust->chain());
							lItem.setValue(lMistrust->amount());
							lItem.setTimestamp(lMistrust->timestamp());
							lItem.setScore(lMistrust->score());
							lItem.setSignature(lMistrust->signature());

							lItem.addItemInfo(BuzzfeedItem::ItemInfo(
								lMistrust->timestamp(), lMistrust->score(),
								lBuzzer, lInfo->chain(), lInfo->id(), 
								lMistrust->chain(), lMistrust->id(), lMistrust->signature()
							));

							notifyNewBuzz(lItem);
						}
					}
				}
			}

			return true;
		}
	}

	return false;
}

bool BuzzerPeerExtension::processEventSubscribe(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_SUBSCRIBE) {
		//
		// check subscription
		TransactionPtr lTx = ctx->tx();
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		// process
		if (lStore && lStore->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());
			//
			TxBuzzerSubscribePtr lSubscribeTx = TransactionHelper::to<TxBuzzerSubscribe>(lTx);
			uint256 lPublisher = lSubscribeTx->in()[0].out().tx(); // in[0] - publisher
			uint256 lSubscriber = lSubscribeTx->in()[1].out().tx(); // in[1] - subscriber

			if (dapp_.instance() == lPublisher) {
				// make update
				EventsfeedItem lItem;
				TxBuzzerPtr lBuzzer = nullptr;
				TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lSubscriber);
				if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
				else return true;
				//
				lItem.setType(TX_BUZZER_SUBSCRIBE);
				lItem.setTimestamp(lSubscribeTx->timestamp());
				lItem.setBuzzId(lSubscribeTx->id());
				lItem.setBuzzChainId(lSubscribeTx->chain());
				lItem.setSignature(lSubscribeTx->signature());
				lItem.setPublisher(lPublisher);

				EventsfeedItem::EventInfo lInfo(lSubscribeTx->timestamp(), lBuzzer->id(), lSubscribeTx->buzzerInfoChain(), lSubscribeTx->buzzerInfo(), lSubscribeTx->score());
				lInfo.setEvent(lSubscribeTx->chain(), lSubscribeTx->id(), lSubscribeTx->signature());
				lItem.addEventInfo(lInfo);
				notifyNewEvent(lItem);
			}
		}

		return true;
	}

	return false;
}

//
BuzzerTransactionStoreExtensionPtr BuzzerPeerExtension::locateBuzzerExtension(const uint256& buzzer) {
	//
	ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
	TransactionPtr lBuzzerTx = lMainStore->locateTransaction(buzzer);
	if (!lBuzzerTx) return nullptr;

	ITransactionStorePtr lStore = lMainStore->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
	if (!lStore) return nullptr;

	if (!lStore->extension()) return nullptr;
	BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());
	return lExtension;
}

//
bool BuzzerPeerExtension::processConversation(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_CONVERSATION) {
		//
		TransactionPtr lTx = ctx->tx();
		//
		if (lTx->in().size() <= TX_BUZZER_CONVERSATION_BUZZER_IN) return false;

		// buzzer event
		bool lFound = false;
		if (lTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx() == dapp_.instance()) {
			lFound = true;
		} else if (lTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx() == dapp_.instance()) {
			lFound = true;
		}

		if (lFound) {
			//
			TxBuzzerConversationPtr lConversationTx = TransactionHelper::to<TxBuzzerConversation>(lTx);
			uint256 lCreator = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
			uint256 lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx();

			//
			BuzzerTransactionStoreExtensionPtr lCreatorExtension = locateBuzzerExtension(lCreator);
			if (!lCreatorExtension) return true;
			BuzzerTransactionStoreExtensionPtr lCounterpartyExtension = locateBuzzerExtension(lCounterparty);
			if (!lCounterpartyExtension) return true;

			ConversationItemPtr lItem = ConversationItem::instance();

			lItem->setTimestamp(lConversationTx->timestamp()); // last update
			lItem->setConversationId(lConversationTx->id());
			lItem->setConversationChainId(lConversationTx->chain());

			TxBuzzerInfoPtr lCreatorInfo = lCreatorExtension->readBuzzerInfo(lCreator);
			if (!lCreatorInfo) return true;
			lItem->setCreatorId(lCreator);
			lItem->setCreatorInfoId(lCreatorInfo->id());
			lItem->setCreatorInfoChainId(lCreatorInfo->chain());

			TxBuzzerInfoPtr lCounterpartyInfo = lCounterpartyExtension->readBuzzerInfo(lCounterparty);
			if (!lCounterpartyInfo) return true;
			lItem->setCounterpartyId(lCounterparty);
			lItem->setCounterpartyInfoId(lCounterpartyInfo->id());
			lItem->setCounterpartyInfoChainId(lCounterpartyInfo->chain());

			lItem->setSignature(lConversationTx->signature());

			BuzzerTransactionStoreExtension::BuzzerInfo lCounterpartyScore;
			BuzzerTransactionStoreExtension::BuzzerInfo lCreatorScore;
			if (dapp_.instance() == lCreator) {
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

			//
			// make event
			if (lTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx() == dapp_.instance()) {
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
					lCounterparty, // trick
					lCreatorInfo->chain(), 
					lCreatorInfo->id(), 
					lCreatorScore.score());
				
				lEventInfo.setEvent(lConversationTx->chain(), lConversationTx->id(), lConversationTx->signature());
				lEvent->addEventInfo(lEventInfo);

				//
				notifyNewEvent(*lEvent);
			}

			//
			// make conversation notification
			notifyNewConversation(*lItem);
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::processAcceptConversation(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_ACCEPT_CONVERSATION) {
		//
		TransactionPtr lTx = ctx->tx();
		// in[0] - buzzer
		uint256 lBuzzer = lTx->in()[TX_BUZZER_CONVERSATION_ACCEPT_MY_IN].out().tx(); 
		uint256 lBuzzerChain = lTx->in()[TX_BUZZER_CONVERSATION_ACCEPT_MY_IN].out().chain();
		// in[1] - conversation
		uint256 lConversationId = lTx->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().tx();
		uint256 lConversationChain = lTx->in()[TX_BUZZER_CONVERSATION_ACCEPT_IN].out().chain();
		//
		ITransactionStorePtr lConversationStore = peerManager_->consensusManager()->storeManager()->locate(lConversationChain);
		if (!lConversationStore) return false;
		TransactionPtr lConversationTx = lConversationStore->locateTransaction(lConversationId);
		if (!lConversationTx) return false;

		// buzzer event
		bool lFound = false;
		if (lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx() == dapp_.instance()) {
			lFound = true;
		} else if (lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx() == dapp_.instance()) {
			lFound = true;
		}

		if (lFound) {
			//
			uint256 lCreator = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
			uint256 lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx();

			//
			BuzzerTransactionStoreExtensionPtr lCreatorExtension = locateBuzzerExtension(lCreator);
			if (!lCreatorExtension) return true;
			BuzzerTransactionStoreExtensionPtr lCounterpartyExtension = locateBuzzerExtension(lCounterparty);
			if (!lCounterpartyExtension) return true;

			TxBuzzerInfoPtr lCreatorInfo = lCreatorExtension->readBuzzerInfo(lCreator);
			if (!lCreatorInfo) return true;
			TxBuzzerInfoPtr lCounterpartyInfo = lCounterpartyExtension->readBuzzerInfo(lCounterparty);
			if (!lCounterpartyInfo) return true;

			BuzzerTransactionStoreExtension::BuzzerInfo lCounterpartyScore;
			lCounterpartyExtension->readBuzzerStat(lCounterparty, lCounterpartyScore);

			TxBuzzerAcceptConversationPtr lAccept = TransactionHelper::to<TxBuzzerAcceptConversation>(lTx);
			ConversationItem::EventInfo lItem(
				lAccept->type(),
				lAccept->timestamp(),
				lConversationId,
				lAccept->id(), 
				lAccept->chain(), 
				lCounterparty,
				lCounterpartyInfo->chain(),
				lCounterpartyInfo->id(),
				lCounterpartyScore.score(),
				lAccept->signature());

			//
			// make conversation update
			notifyUpdateConversation(lItem);

			//
			// create event
			if (lCreator == dapp_.instance()) {
				//
				EventsfeedItemPtr lEvent = EventsfeedItem::instance();

				lEvent->setType(TX_BUZZER_ACCEPT_CONVERSATION);
				lEvent->setTimestamp(lAccept->timestamp());

				// get score
				BuzzerTransactionStoreExtension::BuzzerInfo lCounterpartyScore;
				lCounterpartyExtension->readBuzzerStat(lCounterparty, lCounterpartyScore);

				BuzzerTransactionStoreExtension::BuzzerInfo lCreatorScore;
				lCreatorExtension->readBuzzerStat(lCreator, lCreatorScore);

				// conversation info
				lEvent->setBuzzId(lConversationTx->id());
				lEvent->setBuzzChainId(lConversationTx->chain());
				lEvent->setPublisher(lCreator);
				lEvent->setPublisherInfo(lCreatorInfo->id());
				lEvent->setPublisherInfoChain(lCreatorInfo->chain());
				lEvent->setScore(lCreatorScore.score());

				// add CONVERSATION for verification
				EventsfeedItem::EventInfo lEventInfo(
					lAccept->timestamp(), 
					lAccept->in()[TX_BUZZER_CONVERSATION_ACCEPT_MY_IN].out().tx(), 
					lCounterpartyInfo->chain(), 
					lCounterpartyInfo->id(), 
					lCounterpartyScore.score());
				
				lEventInfo.setEvent(lAccept->chain(), lAccept->id(), lAccept->signature());
				lEvent->addEventInfo(lEventInfo);

				//
				notifyNewEvent(*lEvent);				
			}
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::processDeclineConversation(TransactionContextPtr ctx) {
	//
	if (ctx->tx()->type() == TX_BUZZER_DECLINE_CONVERSATION) {
		//
		TransactionPtr lTx = ctx->tx();
		// in[0] - buzzer
		uint256 lBuzzer = lTx->in()[TX_BUZZER_CONVERSATION_DECLINE_MY_IN].out().tx(); 
		uint256 lBuzzerChain = lTx->in()[TX_BUZZER_CONVERSATION_DECLINE_MY_IN].out().chain();
		// in[1] - conversation
		uint256 lConversationId = lTx->in()[TX_BUZZER_CONVERSATION_DECLINE_IN].out().tx();
		uint256 lConversationChain = lTx->in()[TX_BUZZER_CONVERSATION_DECLINE_IN].out().chain();
		//
		ITransactionStorePtr lConversationStore = peerManager_->consensusManager()->storeManager()->locate(lConversationChain);
		if (!lConversationStore) return false;
		TransactionPtr lConversationTx = lConversationStore->locateTransaction(lConversationId);
		if (!lConversationTx) return false;

		// buzzer event
		bool lFound = false;
		if (lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx() == dapp_.instance()) {
			lFound = true;
		} else if (lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx() == dapp_.instance()) {
			lFound = true;
		}

		if (lFound) {
			//
			uint256 lCreator = lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx();
			uint256 lCounterparty = lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx();

			//
			BuzzerTransactionStoreExtensionPtr lCreatorExtension = locateBuzzerExtension(lCreator);
			if (!lCreatorExtension) return true;
			BuzzerTransactionStoreExtensionPtr lCounterpartyExtension = locateBuzzerExtension(lCounterparty);
			if (!lCounterpartyExtension) return true;

			TxBuzzerInfoPtr lCreatorInfo = lCreatorExtension->readBuzzerInfo(lCreator);
			if (!lCreatorInfo) return true;
			TxBuzzerInfoPtr lCounterpartyInfo = lCounterpartyExtension->readBuzzerInfo(lCounterparty);
			if (!lCounterpartyInfo) return true;

			BuzzerTransactionStoreExtension::BuzzerInfo lCounterpartyScore;
			lCounterpartyExtension->readBuzzerStat(lCounterparty, lCounterpartyScore);

			TxBuzzerDeclineConversationPtr lDecline = TransactionHelper::to<TxBuzzerDeclineConversation>(lTx);
			ConversationItem::EventInfo lItem(
				lDecline->type(),
				lDecline->timestamp(),
				lConversationId,
				lDecline->id(), 
				lDecline->chain(), 
				lCounterparty,
				lCounterpartyInfo->chain(),
				lCounterpartyInfo->id(),
				lCounterpartyScore.score(),
				lDecline->signature());

			//
			// make conversation update
			notifyUpdateConversation(lItem);

			//
			// create event
			if (lCreator == dapp_.instance()) {
				//
				EventsfeedItemPtr lEvent = EventsfeedItem::instance();

				lEvent->setType(TX_BUZZER_DECLINE_CONVERSATION);
				lEvent->setTimestamp(lDecline->timestamp());

				// get score
				BuzzerTransactionStoreExtension::BuzzerInfo lCounterpartyScore;
				lCounterpartyExtension->readBuzzerStat(lCounterparty, lCounterpartyScore);

				BuzzerTransactionStoreExtension::BuzzerInfo lCreatorScore;
				lCreatorExtension->readBuzzerStat(lCreator, lCreatorScore);

				// conversation info
				lEvent->setBuzzId(lConversationTx->id());
				lEvent->setBuzzChainId(lConversationTx->chain());
				lEvent->setPublisher(lCreator);
				lEvent->setPublisherInfo(lCreatorInfo->id());
				lEvent->setPublisherInfoChain(lCreatorInfo->chain());
				lEvent->setScore(lCreatorScore.score());

				// add CONVERSATION for verification
				EventsfeedItem::EventInfo lEventInfo(
					lDecline->timestamp(), 
					lDecline->in()[TX_BUZZER_CONVERSATION_DECLINE_MY_IN].out().tx(), 
					lCounterpartyInfo->chain(), 
					lCounterpartyInfo->id(), 
					lCounterpartyScore.score());
				
				lEventInfo.setEvent(lDecline->chain(), lDecline->id(), lDecline->signature());
				lEvent->addEventInfo(lEventInfo);

				//
				notifyNewEvent(*lEvent);
			}
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::processConversationMessage(TransactionContextPtr ctx, bool processHide, uint64_t timestamp, const uint512& signature) {
	//
	if (ctx->tx()->type() == TX_BUZZER_MESSAGE) {
		//
		TransactionPtr lTx = ctx->tx();
		// sanity
		if (lTx->in().size() <= TX_BUZZER_CONVERSATION_IN) return false;
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		if (!lStore) return false;
		//
		ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		//
		uint256 lCounterparty;
		uint256 lCounterpartyChain;
		// in[0] - buzzer
		uint256 lBuzzerId = lTx->in()[TX_BUZZER_MY_IN].out().tx(); 
		// in[1] - conversation
		uint256 lConversationId = lTx->in()[TX_BUZZER_CONVERSATION_IN].out().tx();
		uint256 lConversationChain = lTx->in()[TX_BUZZER_CONVERSATION_IN].out().chain();
		//
		ITransactionStorePtr lConversationStore = peerManager_->consensusManager()->storeManager()->locate(lConversationChain);
		if (!lConversationStore) return false;
		TransactionPtr lConversationTx = lConversationStore->locateTransaction(lConversationId);
		if (!lConversationTx) return false;

		// buzzer event
		bool lFound = false;
		if (lConversationTx->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx() == dapp_.instance()) {
			lFound = true;
		} else if (lConversationTx->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx() == dapp_.instance()) {
			lFound = true;
		}

		if (lFound) {
			//
			ITransactionStorePtr lMainStore = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());

			TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lBuzzerId);
			if (!lBuzzerTx) return true;

			ITransactionStorePtr lStore = lMainStore->storeManager()->locate(lBuzzerTx->in()[TX_BUZZER_SHARD_IN].out().tx());
			if (!lStore) return true;

			if (!lStore->extension()) return true;
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			TxBuzzerMessagePtr lMessage = TransactionHelper::to<TxBuzzerMessage>(lTx);

			BuzzfeedItem lItem;
			TxBuzzerPtr lBuzzer = nullptr;
			if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);
			else return true;

			//
			// send new message
			prepareBuzzfeedItem(lItem, TransactionHelper::to<TxBuzz>(lTx), lBuzzer, lExtension);
			lItem.setRootBuzzId(lConversationId);

			if (processHide) {
				// clean up
				lItem.setType(TX_BUZZ_HIDE);
				lItem.setBuzzBody(std::vector<unsigned char>());
				lItem.setBuzzMedia(std::vector<BuzzerMediaPointer>());
				lItem.setTimestamp(timestamp);
				lItem.setSignature(signature);
			}

			notifyNewMessage(lItem);

			//
			// send event
			if (lBuzzerId != dapp_.instance() && !processHide) {
				EventsfeedItem lEventItem;
				prepareEventsfeedItem(lEventItem, lMessage, lBuzzer, lMessage->chain(), lMessage->id(), 
					lMessage->timestamp(), lMessage->buzzerInfoChain(), lMessage->buzzerInfo(), lMessage->score(), lMessage->signature());
				// conversation is root
				lEventItem.beginInfo()->setEventId(lConversationId);
				lEventItem.beginInfo()->setEventChainId(lConversationChain);
				notifyNewEvent(lEventItem);
			}

			BuzzerTransactionStoreExtensionPtr lCounterpartyExtension = locateBuzzerExtension(lBuzzerId);
			if (!lCounterpartyExtension) return true;

			TxBuzzerInfoPtr lBuzzerInfo = lCounterpartyExtension->readBuzzerInfo(lBuzzerId);
			if (!lBuzzerInfo) return true;

			BuzzerTransactionStoreExtension::BuzzerInfo lBuzzerScore;
			lCounterpartyExtension->readBuzzerStat(lBuzzerId, lBuzzerScore);

			//
			if (!processHide) {
				// notify update conversations
				ConversationItem::EventInfo lMessageItem(
					lMessage->type(),
					lMessage->timestamp(),
					lConversationId,
					lMessage->id(), 
					lMessage->chain(), 
					lBuzzerId,
					lBuzzerInfo->chain(),
					lBuzzerInfo->id(),
					lMessage->mediaPointers(),
					lMessage->body(),
					lBuzzerScore.score(),
					lMessage->signature());

				//
				// make conversation update
				notifyUpdateConversation(lMessageItem);
			}
		}
	}

	return true;
}

bool BuzzerPeerExtension::processConversationMessageReply(TransactionContextPtr ctx) {
	return true;
}

//
// external entry point
void BuzzerPeerExtension::processTransaction(TransactionContextPtr ctx) {
	//
	// check pending updates
	if (pendingScore_) {
		notifyUpdateTrustScore(lastScore_); // just re-push recent
	}

	if (pendingBuzzfeedItems_) {
		notifyUpdateBuzz(std::vector<BuzzfeedItemUpdate>()); // already have collected
	}

	if (pendingEventsfeedItems_) {
		notifyNewEvent(EventsfeedItem(), true); // already have collected
	}

	//
	if (!processBuzzCommon(ctx)) {
		if (!processBuzzLike(ctx))
			processBuzzReward(ctx);
	}

	if (ctx->tx()->type() == TX_REBUZZ) 
		processRebuzz(ctx);

	if (!processEventCommon(ctx)) {
		if (!processEventLike(ctx))
			processEventReward(ctx);
	}

	//
	if (ctx->tx()->type() == TX_BUZZER_ENDORSE) {
		processEndorse(ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_MISTRUST) {
		processMistrust(ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_SUBSCRIBE) {
		processEventSubscribe(ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_CONVERSATION) {
		processConversation(ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_ACCEPT_CONVERSATION) {
		processAcceptConversation(ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_DECLINE_CONVERSATION) {
		processDeclineConversation(ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_MESSAGE) {
		processConversationMessage(ctx);
	} else if (ctx->tx()->type() == TX_BUZZER_MESSAGE_REPLY) {
		processConversationMessageReply(ctx);
	} else if (ctx->tx()->type() == TX_BUZZ_HIDE) {
		processBuzzHide(ctx);
	}
}

//
// external entry point
bool BuzzerPeerExtension::processMessage(Message& message, std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (message.type() == GET_BUZZER_SUBSCRIPTION) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetSubscription, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZER_SUBSCRIPTION) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processSubscription, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZER_SUBSCRIPTION_IS_ABSENT) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processSubscriptionAbsent, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == GET_BUZZ_FEED) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetBuzzfeed, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZ_FEED) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzfeed, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == NEW_BUZZ_NOTIFY) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processNewBuzzNotify, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZ_UPDATE_NOTIFY) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzUpdateNotify, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == GET_BUZZES) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetBuzzes, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == GET_EVENTS_FEED) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetEventsfeed, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == EVENTS_FEED) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processEventsfeed, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == NEW_EVENT_NOTIFY) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processNewEventNotify, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == EVENT_UPDATE_NOTIFY) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processEventUpdateNotify, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == GET_BUZZER_TRUST_SCORE) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetBuzzerTrustScore, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZER_TRUST_SCORE) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzerTrustScore, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZER_TRUST_SCORE_UPDATE) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzerTrustScoreUpdateNotify, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == GET_BUZZER_MISTRUST_TX) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetBuzzerMistrustTx, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZER_MISTRUST_TX) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzerMistrustTx, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == GET_BUZZER_ENDORSE_TX) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetBuzzerEndorseTx, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZER_ENDORSE_TX) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzerEndorseTx, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == GET_BUZZ_FEED_BY_BUZZ) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetBuzzfeedByBuzz, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZ_FEED_BY_BUZZ) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzfeedByBuzz, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == GET_BUZZ_FEED_BY_BUZZER) {
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetBuzzfeedByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZ_FEED_BY_BUZZER) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzfeedByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == GET_MISTRUSTS_FEED_BY_BUZZER) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetMistrustsByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == MISTRUSTS_FEED_BY_BUZZER) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processMistrustsByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == GET_ENDORSEMENTS_FEED_BY_BUZZER) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetEndorsementsByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == ENDORSEMENTS_FEED_BY_BUZZER) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processEndorsementsByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == GET_BUZZER_SUBSCRIPTIONS) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetSubscriptionsByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZER_SUBSCRIPTIONS) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processSubscriptionsByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == GET_BUZZER_FOLLOWERS) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetFollowersByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZER_FOLLOWERS) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processFollowersByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == GET_BUZZ_FEED_GLOBAL) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetBuzzfeedGlobal, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZ_FEED_GLOBAL) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzfeedGlobal, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == GET_BUZZ_FEED_BY_TAG) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetBuzzfeedByTag, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZ_FEED_BY_TAG) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzfeedByTag, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == GET_HASH_TAGS) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetHashTags, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == HASH_TAGS) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processHashTags, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == BUZZ_SUBSCRIBE) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processSubscribeBuzzThread, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZ_UNSUBSCRIBE) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processUnsubscribeBuzzThread, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == GET_BUZZER_AND_INFO) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetBuzzerAndInfo, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZER_AND_INFO) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzerAndInfo, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == BUZZER_AND_INFO_ABSENT) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processBuzzerAndInfoAbsent, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == GET_CONVERSATIONS_FEED_BY_BUZZER) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetConversationsFeedByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == CONVERSATIONS_FEED_BY_BUZZER) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processConversationsFeedByBuzzer, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == GET_MESSAGES_FEED_BY_CONVERSATION) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processGetMessagesFeedByConversation, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == MESSAGES_FEED_BY_CONVERSATION) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processMessagesFeedByConversation, shared_from_this(), msg,
				boost::asio::placeholders::error)));

	} else if (message.type() == NEW_BUZZER_CONVERSATION_NOTIFY) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processNewConversationNotify, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == UPDATE_BUZZER_CONVERSATION_NOTIFY) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processUpdateConversationNotify, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else if (message.type() == NEW_BUZZER_MESSAGE_NOTIFY) {
		if (peerManager_->settings()->reindex()) return true;
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			peer_->strand()->wrap(boost::bind(
				&BuzzerPeerExtension::processNewMessageNotify, shared_from_this(), msg,
				boost::asio::placeholders::error)));
	} else {
		return false;
	}

	return true;
}

void BuzzerPeerExtension::processGetSubscription(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load subscription from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lSubscriber;
		uint256 lPublisher;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lSubscriber;
		(*msg) >> lPublisher;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			lSubscription = lExtension->locateSubscription(lSubscriber, lPublisher);
			if (lSubscription) {
				// make message, serialize, send back
				std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

				// fill data
				DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
				lStream << lRequestId;
				Transaction::Serializer::serialize<DataStream>(lStream, lSubscription); // tx

				// prepare message
				Message lMessage(BUZZER_SUBSCRIPTION, lStream.size(), Hash160(lStream.begin(), lStream.end()));
				(*lMsg) << lMessage;
				lMsg->write(lStream.data(), lStream.size());

				// log
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending subscription transacion ") + strprintf("%s", lSubscription->id().toHex()) + std::string(" for ") + peer_->key());

				// write
				peer_->sendMessage(lMsg);
			}
		} 

		// not found
		if (!lSubscription) {
			// tx is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;

			// prepare message
			Message lMessage(BUZZER_SUBSCRIPTION_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: subscription is absent for ") + strprintf("%s/%s", lSubscriber.toHex(), lPublisher.toHex()) + std::string(" -> ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetSubscription/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetBuzzerTrustScore(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load trust score from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lBuzzer;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lBuzzer;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			BuzzerTransactionStoreExtension::BuzzerInfo lScore;
			if (!lExtension->readBuzzerStat(lBuzzer, lScore)) {
				// try to reconstruct
				lExtension->processBuzzerStat(lBuzzer);
				// re-read
				lExtension->readBuzzerStat(lBuzzer, lScore);
			}

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lScore;

			// prepare message
			Message lMessage(BUZZER_TRUST_SCORE, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending trust score transacion ") + strprintf("%d/%d", lScore.endorsements(), lScore.mistrusts()) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();	
		}

		//
		//peer_->waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzerTrustScore/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetBuzzerEndorseTx(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load endorse from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lActor;
		uint256 lBuzzer;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lActor;
		(*msg) >> lBuzzer;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			
			uint256 lTx;
			lExtension->selectBuzzerEndorseTx(lActor, lBuzzer, lTx);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lTx;

			// prepare message
			Message lMessage(BUZZER_ENDORSE_TX, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending endorse tx ") + strprintf("%s/%s#", lTx.toHex(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}

		//
		//peer_->waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzerEndorseTx/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetBuzzerMistrustTx(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load mistrust from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lActor;
		uint256 lBuzzer;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lActor;
		(*msg) >> lBuzzer;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			
			uint256 lTx;
			lExtension->selectBuzzerMistrustTx(lActor, lBuzzer, lTx);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lTx;

			// prepare message
			Message lMessage(BUZZER_MISTRUST_TX, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending mistrust tx ") + strprintf("%s/%s#", lTx.toHex(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}

		//
		//peer_->waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzerMistrustTx/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processSubscribeBuzzThread(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request subscribe buzz thread ") + peer_->key());

		// extract
		uint256 lChain;
		uint256 lBuzzId;
		uint512 lSignature;
		(*msg) >> lChain;
		(*msg) >> lBuzzId;
		(*msg) >> lSignature;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage) {
			//
			boost::unique_lock<boost::recursive_mutex> lLock(notificationMutex_);
			buzzSubscriptions_[lBuzzId] = lSignature; // put temporary subscription
		}

		peer_->processed();

	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzfeed/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processUnsubscribeBuzzThread(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request unsubscribe buzz thread ") + peer_->key());

		// extract
		uint256 lChain;
		uint256 lBuzzId;
		uint512 lSignature;
		(*msg) >> lChain;
		(*msg) >> lBuzzId;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage) {
			//
			boost::unique_lock<boost::recursive_mutex> lLock(notificationMutex_);
			buzzSubscriptions_.erase(lBuzzId);
		}

		peer_->processed();

	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzfeed/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetConversationsFeedByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load conversations from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lBuzzer;
		uint64_t lFrom;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lBuzzer;
		(*msg) >> lFrom;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<ConversationItem> lFeed;
			lExtension->selectConversations(lFrom, lBuzzer, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lBuzzer;
			lStream << lFeed;

			// prepare message
			Message lMessage(CONVERSATIONS_FEED_BY_BUZZER, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending conversations ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetConversationsFeedByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetMessagesFeedByConversation(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load conversation messages from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lConversation;
		uint64_t lFrom;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lConversation;
		(*msg) >> lFrom;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<BuzzfeedItem> lFeed;
			lExtension->selectMessages(lFrom, lConversation, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lConversation;
			lStream << lFeed;

			// prepare message
			Message lMessage(MESSAGES_FEED_BY_CONVERSATION, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending conversation messages ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetMessagesFeedByConversation/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetBuzzfeed(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load buzzfeed from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		std::vector<BuzzfeedPublisherFrom> lFrom;
		uint256 lSubscriber;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFrom;
		(*msg) >> lSubscriber;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<BuzzfeedItem> lFeed;
			lExtension->selectBuzzfeed(lFrom, lSubscriber, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lFeed;

			// prepare message
			Message lMessage(BUZZ_FEED, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending buzzfeed ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzfeed/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetBuzzfeedGlobal(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load buzzfeed global from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint64_t lTimeframeFrom;
		uint64_t lScoreFrom;
		uint64_t lTimestampFrom;
		uint256 lPublisher;
		uint256 lSubscriber;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lTimeframeFrom;
		(*msg) >> lScoreFrom;
		(*msg) >> lTimestampFrom;
		(*msg) >> lPublisher;
		if ((*msg).size() >= uint256().size()) (*msg) >> lSubscriber;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<BuzzfeedItem> lFeed;
			lExtension->selectBuzzfeedGlobal(lTimeframeFrom, lScoreFrom, lTimestampFrom, lPublisher, lSubscriber, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lFeed;

			// prepare message
			Message lMessage(BUZZ_FEED, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending buzzfeed global ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzfeedGlobal/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetBuzzfeedByTag(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load buzzfeed by tag from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		std::string lTag;
		uint64_t lTimeframeFrom;
		uint64_t lScoreFrom;
		uint64_t lTimestampFrom;
		uint256 lPublisher;
		uint256 lSubscriber;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lTag;		
		(*msg) >> lTimeframeFrom;
		(*msg) >> lScoreFrom;
		(*msg) >> lTimestampFrom;
		(*msg) >> lPublisher;
		if ((*msg).size() >= uint256().size()) (*msg) >> lSubscriber;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<BuzzfeedItem> lFeed;
			lExtension->selectBuzzfeedByTag(lTag, lTimeframeFrom, lScoreFrom, lTimestampFrom, lPublisher, lSubscriber, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lFeed;

			// prepare message
			Message lMessage(BUZZ_FEED, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending buzzfeed by tag ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzfeedByTag/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetHashTags(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load hash tags from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		std::string lTag;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lTag;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<Buzzer::HashTag> lFeed;
			lExtension->selectHashTags(lTag, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lFeed;

			// prepare message
			Message lMessage(HASH_TAGS, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending buzzfeed global ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetHashTags/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetBuzzfeedByBuzz(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load buzzfeed by buzz from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint64_t lFrom;
		uint256 lBuzz;
		uint256 lSubscriber;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFrom;
		(*msg) >> lBuzz;
		if ((*msg).size() >= uint256().size()) (*msg) >> lSubscriber;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<BuzzfeedItem> lFeed;
			lExtension->selectBuzzfeedByBuzz(lFrom, lBuzz, lSubscriber, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lBuzz;
			lStream << lFeed;

			// prepare message
			Message lMessage(BUZZ_FEED_BY_BUZZ, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending buzzfeed by buzz ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzfeedByBuzz/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetBuzzfeedByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load buzzfeed by buzzer from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint64_t lFrom;
		uint256 lBuzzer;
		uint256 lSubscriber;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFrom;
		(*msg) >> lBuzzer;
		if ((*msg).size() >= uint256().size()) (*msg) >> lSubscriber;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<BuzzfeedItem> lFeed;
			lExtension->selectBuzzfeedByBuzzer(lFrom, lBuzzer, lSubscriber, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lBuzzer;
			lStream << lFeed;

			// prepare message
			Message lMessage(BUZZ_FEED_BY_BUZZER, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending buzzfeed by buzzer ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzfeedByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetMistrustsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load mistrusts from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lFrom;
		uint256 lBuzzer;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFrom;
		(*msg) >> lBuzzer;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<EventsfeedItem> lFeed;
			lExtension->selectMistrusts(lFrom, lBuzzer, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lBuzzer;
			lStream << lFeed;

			// prepare message
			Message lMessage(MISTRUSTS_FEED_BY_BUZZER, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending mistrusts ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetMistrustsByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetEndorsementsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load endorsements from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lFrom;
		uint256 lBuzzer;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFrom;
		(*msg) >> lBuzzer;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<EventsfeedItem> lFeed;
			lExtension->selectEndorsements(lFrom, lBuzzer, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lBuzzer;
			lStream << lFeed;

			// prepare message
			Message lMessage(ENDORSEMENTS_FEED_BY_BUZZER, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending endorsements ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetEndorsementsByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetSubscriptionsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load subscriptions from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lFrom;
		uint256 lBuzzer;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFrom;
		(*msg) >> lBuzzer;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<EventsfeedItem> lFeed;
			lExtension->selectSubscriptions(lFrom, lBuzzer, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lBuzzer;
			lStream << lFeed;

			// prepare message
			Message lMessage(MISTRUSTS_FEED_BY_BUZZER, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending subscriptions ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetSubscriptionsByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetFollowersByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load followers from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lFrom;
		uint256 lBuzzer;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFrom;
		(*msg) >> lBuzzer;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<EventsfeedItem> lFeed;
			lExtension->selectFollowers(lFrom, lBuzzer, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lBuzzer;
			lStream << lFeed;

			// prepare message
			Message lMessage(MISTRUSTS_FEED_BY_BUZZER, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending followers ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetFollowersByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetEventsfeed(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load eventsfeed from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint64_t lFrom;
		uint256 lSubscriber;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFrom;
		(*msg) >> lSubscriber;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			std::vector<EventsfeedItem> lFeed;
			lExtension->selectEventsfeed(lFrom, lSubscriber, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lFeed;

			// prepare message
			Message lMessage(EVENTS_FEED, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending eventsfeed ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetEventsfeed/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetBuzzes(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load buzzes from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		std::vector<uint256> lBuzzes;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lBuzzes;
		peer_->eraseInData(msg);

		// locate subscription
		TransactionPtr lSubscription = nullptr;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage && lStorage->extension()) {
			//
			std::vector<BuzzfeedItem> lFeed;
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
			for (std::vector<uint256>::iterator lId = lBuzzes.begin(); lId != lBuzzes.end(); lId++) {
				//
				BuzzfeedItem lItem;
				if (lExtension->selectBuzzItem(*lId, lItem)) {
					lFeed.push_back(lItem);
				}
			}

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lChain;
			lStream << lFeed;

			// prepare message
			Message lMessage(BUZZ_FEED, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending buzzes list ") + strprintf("%d/%s#", lFeed.size(), lChain.toHex().substr(0, 10)) + std::string(" for ") + peer_->key());

			// write
			peer_->sendMessage(lMsg);
		} else {
			peer_->processed();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzes/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processGetBuzzerAndInfo(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/buzzer]: checksum %s is INVALID for message from %s -> %s", (*msg).calculateCheckSum().toHex(), peer_->key(), HexStr((*msg).begin(), (*msg).end())));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing request load buzzer and info from ") + peer_->key());

		// extract
		uint256 lRequestId;
		std::string lBuzzer;
		entity_name_t lLimitedName(lBuzzer);

		(*msg) >> lRequestId;
		(*msg) >> lLimitedName;
		peer_->eraseInData(msg);

		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		EntityPtr lTx = lStorage->entityStore()->locateEntity(lBuzzer);

		bool lSent = false;
		if (lTx != nullptr) {
			//
			// extract chain

			ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lTx->in()[TX_BUZZER_SHARD_IN].out().tx());
			if (lStorage && lStorage->extension()) {
				//
				BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStorage->extension());
				// load info
				TxBuzzerInfoPtr lInfo = lExtension->readBuzzerInfo(lTx->id());
				if (lInfo) {
					// make message, serialize, send back
					std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

					// fill data
					DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
					lStream << lRequestId;
					Transaction::Serializer::serialize<DataStream>(lStream, lTx); // buzzer
					if (lInfo != nullptr)
						Transaction::Serializer::serialize<DataStream>(lStream, lInfo); // info

					// prepare message
					Message lMessage(BUZZER_AND_INFO, lStream.size(), Hash160(lStream.begin(), lStream.end()));
					(*lMsg) << lMessage;
					lMsg->write(lStream.data(), lStream.size());

					// log
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending buzzer and info for ") + strprintf("%s, buzzer = %s, info = %s", lBuzzer, lTx->id().toHex(), lInfo->id().toHex()) + std::string(" for ") + peer_->key());

					// write
					peer_->sendMessage(lMsg);
					//
					lSent = true;
				}
			}
		}

		if (!lSent) {
			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
			// prepare message
			Message lMessage(BUZZER_AND_INFO_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// write
			peer_->sendMessage(lMsg);
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzerAndInfo/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processBuzzerAndInfo(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load buzzer and info from ") + peer_->key());

		// extract
		uint256 lRequestId;
		TransactionPtr lBuzzer;
		TransactionPtr lInfo;

		(*msg) >> lRequestId;
		lBuzzer = Transaction::Deserializer::deserialize<DataStream>(*msg);
		lInfo = Transaction::Deserializer::deserialize<DataStream>(*msg);
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing load buzzer and info ") + strprintf("r = %s, %s", lRequestId.toHex(), lBuzzer->id().toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadBuzzerAndInfoHandler>(lHandler)->handleReply(TransactionHelper::to<Entity>(lBuzzer), lInfo);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for transaction ") + strprintf("r = %s, %s", lRequestId.toHex(), lBuzzer->id().toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzerAndInfo/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processBuzzerAndInfoAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load buzzer and info from ") + peer_->key());

		// extract
		uint256 lRequestId;
		(*msg) >> lRequestId;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing load buzzer and info is absent ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadBuzzerAndInfoHandler>(lHandler)->handleReply(nullptr, nullptr);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for transaction ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzerAndInfo/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processSubscriptionAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: subscription is absent from ") + peer_->key());

		// extract
		uint256 lRequestId;
		(*msg) >> lRequestId;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadTransactionHandler>(lHandler)->handleReply(nullptr);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND, ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: subscription is absent."));
	} else {
		// log
		gLog().write(Log::NET, "[peer/processSubscriptionAbsent/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processSubscription(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response subscription transaction from ") + peer_->key());

		// extract
		uint256 lRequestId;
		TransactionPtr lTx;

		(*msg) >> lRequestId;
		lTx = Transaction::Deserializer::deserialize<DataStream>(*msg);
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing subscription transaction ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadTransactionHandler>(lHandler)->handleReply(lTx);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for transaction ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processSubscription/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processBuzzerTrustScore(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response for trust score from ") + peer_->key());

		// extract
		uint256 lRequestId;
		BuzzerTransactionStoreExtension::BuzzerInfo lScore;

		(*msg) >> lRequestId;
		(*msg) >> lScore;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing trust score ") + strprintf("%d/%d", lScore.endorsements(), lScore.mistrusts()) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadTrustScoreHandler>(lHandler)->handleReply(lScore.endorsements(), lScore.mistrusts(), lScore.subscriptions(), lScore.followers());
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzerTrustScore/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processBuzzerEndorseTx(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response for endorse tx from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lTx;

		(*msg) >> lRequestId;
		(*msg) >> lTx;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing endorse tx ") + strprintf("%s", lTx.toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadEndorseMistrustHandler>(lHandler)->handleReply(lTx);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzerEndorseTx/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processBuzzerMistrustTx(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response for mistrust tx from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lTx;

		(*msg) >> lRequestId;
		(*msg) >> lTx;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing mistrust tx ") + strprintf("%s", lTx.toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadEndorseMistrustHandler>(lHandler)->handleReply(lTx);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzerMistrustTx/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processBuzzfeed(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load buzzfeed from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		std::vector<BuzzfeedItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing buzzfeed ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectBuzzFeedHandler>(lHandler)->handleReply(lFeed, lChain);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzfeed/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processBuzzfeedGlobal(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load buzzfeed global from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		std::vector<BuzzfeedItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing buzzfeed global ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectBuzzFeedHandler>(lHandler)->handleReply(lFeed, lChain);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzfeedGlobal/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processBuzzfeedByTag(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load buzzfeed by tag from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		std::vector<BuzzfeedItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing buzzfeed by tag ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectBuzzFeedHandler>(lHandler)->handleReply(lFeed, lChain);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzfeedByTag/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processHashTags(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load hash tags from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		std::vector<Buzzer::HashTag> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing hash tags ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectHashTagsHandler>(lHandler)->handleReply(lFeed, lChain);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processHashTags/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processBuzzfeedByBuzz(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load buzzfeed by buzz from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lBuzz;
		std::vector<BuzzfeedItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lBuzz;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing buzzfeed by buzz ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectBuzzFeedByEntityHandler>(lHandler)->handleReply(lFeed, lChain, lBuzz);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed by buzz ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzfeedByBuzz/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processBuzzfeedByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load buzzfeed by buzzer from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lBuzzer;
		std::vector<BuzzfeedItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lBuzzer;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing buzzfeed by buzzer ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectBuzzFeedByEntityHandler>(lHandler)->handleReply(lFeed, lChain, lBuzzer);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed by buzzer ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzfeedByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processMistrustsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load mistrusts by buzzer from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lBuzzer;
		std::vector<EventsfeedItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lBuzzer;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing mistrusts by buzzer ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectEventsFeedByEntityHandler>(lHandler)->handleReply(lFeed, lChain, lBuzzer);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed by buzzer ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processMistrustsByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processEndorsementsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load endorsements by buzzer from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lBuzzer;
		std::vector<EventsfeedItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lBuzzer;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing endorsements` by buzzer ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectEventsFeedByEntityHandler>(lHandler)->handleReply(lFeed, lChain, lBuzzer);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed by buzzer ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processEndorsementsByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processSubscriptionsByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load subscriptions by buzzer from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lBuzzer;
		std::vector<EventsfeedItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lBuzzer;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing subscriptions by buzzer ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectEventsFeedByEntityHandler>(lHandler)->handleReply(lFeed, lChain, lBuzzer);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed by buzzer ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processSubscriptionsByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processFollowersByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load followers by buzzer from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lBuzzer;
		std::vector<EventsfeedItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lBuzzer;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing followers by buzzer ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectEventsFeedByEntityHandler>(lHandler)->handleReply(lFeed, lChain, lBuzzer);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed by buzzer ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processFollowersByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processConversationsFeedByBuzzer(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load conversations by buzzer from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lBuzzer;
		std::vector<ConversationItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lBuzzer;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing conversations by buzzer ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectConversationsFeedByEntityHandler>(lHandler)->handleReply(lFeed, lChain, lBuzzer);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for conversations by buzzer ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processConversationsFeedByBuzzer/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processMessagesFeedByConversation(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load messages by conversation from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lConversation;
		std::vector<BuzzfeedItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lConversation;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing messages by conversations` ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			// mixup pending messages
			std::list<BuzzfeedItemPtr> lItems;
			buzzer_->pendingMessages(peer_->addressId(), lChain, lConversation, lItems);
			//
			for (std::list<BuzzfeedItemPtr>::iterator lItem = lItems.begin(); lItem != lItems.end(); lItem++) {
				lFeed.push_back((*lItem)->self());
			}

			ReplyHelper::to<ISelectBuzzFeedByEntityHandler>(lHandler)->handleReply(lFeed, lChain, lConversation);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processMessagesFeedByConversation/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processEventsfeed(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing response load eventsfeed from ") + peer_->key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		std::vector<EventsfeedItem> lFeed;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lFeed;
		peer_->eraseInData(msg);

		// read
		peer_->processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing eventsfeed ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectEventsFeedHandler>(lHandler)->handleReply(lFeed, lChain);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processEventsfeed/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processNewBuzzNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing new buzz notification from ") + peer_->key());

		// extract
		BuzzfeedItem lBuzz;
		(*msg) >> lBuzz;
		peer_->eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing new buzz notification ") + 
			strprintf("%s/%s# from %s", lBuzz.buzzId().toHex(), lBuzz.buzzChainId().toHex().substr(0, 10), peer_->key()));
		else if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[peer/buzzer]: processing new buzz notification ") + 
			strprintf("%s/%s# from %s", lBuzz.buzzId().toHex(), lBuzz.buzzChainId().toHex().substr(0, 10), peer_->key()));

		if (buzzer_->buzzfeed()) {
			// update subscriptions
			buzzer_->processSubscriptions(lBuzz, peer_->addressId());
			// update (if necessary) current buzzfeed
			buzzer_->buzzfeed()->push(lBuzz, peer_->addressId());
			// try to process pending items (in case of rebuzz)
			buzzer_->resolvePendingItems();
		}

		// WARNING: in case of async_read for large data
		peer_->processed();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processNewBuzzNotify/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processNewMessageNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing new message notification from ") + peer_->key());

		// extract
		BuzzfeedItem lBuzz;
		(*msg) >> lBuzz;
		peer_->eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing new message notification ") + 
			strprintf("%s/%s# from %s", lBuzz.buzzId().toHex(), lBuzz.buzzChainId().toHex().substr(0, 10), peer_->key()));
		else if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[peer/buzzer]: processing message buzz notification ") + 
			strprintf("%s/%s# from %s", lBuzz.buzzId().toHex(), lBuzz.buzzChainId().toHex().substr(0, 10), peer_->key()));

		if (buzzer_->processConversations(lBuzz, peer_->addressId())) {
			// try to process pending items (in case of rebuzz)
			buzzer_->resolvePendingItems();
		} else {
			buzzer_->pushPendingMessage(peer_->addressId(), lBuzz.buzzChainId(), lBuzz.rootBuzzId(), BuzzfeedItem::instance(lBuzz));
		}

		// WARNING: in case of async_read for large data
		peer_->processed();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processNewMessageNotify/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processNewEventNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing new event notification from ") + peer_->key());

		// extract
		std::vector<EventsfeedItem> lEvents;
		(*msg) >> lEvents;
		peer_->eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing new event notifications ") +
			strprintf("%d from %s", lEvents.size(), peer_->key()));
		else if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[peer/buzzer]: processing new event notifications ") +
			strprintf("%d from %s", lEvents.size(), peer_->key()));

		// update
		eventsfeed_->mergeUpdate(lEvents, peer_->addressId());
		// try to process pending items (in case of rebuzz)
		buzzer_->resolvePendingEventsItems();
		// resolve info
		buzzer_->resolveBuzzerInfos(); //

		// WARNING: in case of async_read for large data
		peer_->processed();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processNewEventNotify/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processNewConversationNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing new conversation notification from ") + peer_->key());

		// extract
		std::vector<ConversationItem> lEvents;
		(*msg) >> lEvents;
		peer_->eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing new conversation notifications ") +
			strprintf("%d from %s", lEvents.size(), peer_->key()));
		else if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[peer/buzzer]: processing new conversation notifications ") +
			strprintf("%d from %s", lEvents.size(), peer_->key()));

		// update
		if (buzzer_->conversations()) { 
			//
			buzzer_->conversations()->mergeUpdate(lEvents, peer_->addressId());
			// try to process pending items (in case of rebuzz)
			buzzer_->resolvePendingEventsItems();
			// resolve info
			buzzer_->resolveBuzzerInfos(); //
		}

		// WARNING: in case of async_read for large data
		peer_->processed();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processNewConversationNotify/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processUpdateConversationNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing update conversation notification from ") + peer_->key());

		// extract
		std::vector<ConversationItem::EventInfo> lEvents;
		(*msg) >> lEvents;
		peer_->eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing update conversation notifications ") +
			strprintf("%d from %s", lEvents.size(), peer_->key()));
		else if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[peer/buzzer]: processing update conversation notifications ") +
			strprintf("%d from %s", lEvents.size(), peer_->key()));

		// update
		if (buzzer_->conversations()) {
			//
			buzzer_->conversations()->mergeUpdate(lEvents, peer_->addressId());
			// try to process pending items (in case of rebuzz)
			buzzer_->resolvePendingEventsItems();
			// resolve info
			buzzer_->resolveBuzzerInfos(); //
		}

		// WARNING: in case of async_read for large data
		peer_->processed();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processUpdateConversationNotify/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processBuzzUpdateNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing buzz update notification from ") + peer_->key());

		// extract
		std::vector<BuzzfeedItemUpdate> lBuzzUpdates;
		(*msg) >> lBuzzUpdates;
		peer_->eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) {
			for (std::vector<BuzzfeedItemUpdate>::iterator lUpdate = lBuzzUpdates.begin(); lUpdate != lBuzzUpdates.end(); lUpdate++) {
				gLog().write(Log::NET, std::string("[peer/buzzer]: processing buzz update notification ") + 
				strprintf("%s/%s/%d from %s", lUpdate->buzzId().toHex(), lUpdate->fieldString(), lUpdate->count(), peer_->key()));
			}
		} else if (gLog().isEnabled(Log::CLIENT)) {
			for (std::vector<BuzzfeedItemUpdate>::iterator lUpdate = lBuzzUpdates.begin(); lUpdate != lBuzzUpdates.end(); lUpdate++) {
				gLog().write(Log::CLIENT, std::string("[peer/buzzer]: processing buzz update notification ") + 
				strprintf("%s/%s/%d from %s", lUpdate->buzzId().toHex(), lUpdate->fieldString(), lUpdate->count(), peer_->key()));
			}
		}

		// update
		buzzer_->broadcastUpdate<std::vector<BuzzfeedItemUpdate>>(lBuzzUpdates);

		// WARNING: in case of async_read for large data
		peer_->processed();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzUpdateNotify/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

void BuzzerPeerExtension::processEventUpdateNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing events update notification from ") + peer_->key());

		// extract
		std::vector<EventsfeedItem> lBuzzUpdates;
		(*msg) >> lBuzzUpdates;
		peer_->eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) {
				gLog().write(Log::NET, std::string("[peer/buzzer]: processing event updates notification ") + 
														strprintf("%d from %s", lBuzzUpdates.size(), peer_->key()));
		}

		// update
		eventsfeed_->merge(lBuzzUpdates, -1);

		// WARNING: in case of async_read for large data
		peer_->processed();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processEventUpdateNotify/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

//
bool BuzzerPeerExtension::loadSubscription(const uint256& chain, const uint256& subscriber, const uint256& publisher, ILoadTransactionHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << subscriber;
		lStateStream << publisher;
		Message lMessage(GET_BUZZER_SUBSCRIPTION, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading subscription ") + strprintf("sub = %s, pub = %s from %s -> %s", subscriber.toHex(), publisher.toHex(), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectBuzzfeed(const uint256& chain, const std::vector<BuzzfeedPublisherFrom>& from, const uint256& subscriber, ISelectBuzzFeedHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << from;
		lStateStream << subscriber;
		Message lMessage(GET_BUZZ_FEED, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading buzzfeed for ") + strprintf("%s/%s# from %s -> %s", subscriber.toHex(), chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectHashTags(const uint256& chain, const std::string& tag, ISelectHashTagsHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << tag;
		Message lMessage(GET_HASH_TAGS, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading hash tags for ") + strprintf("%s/%s# from %s -> %s", tag, chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectBuzzfeedGlobal(const uint256& chain, uint64_t timeframeFrom, uint64_t scoreFrom, uint64_t timestampFrom, const uint256& publisher, const uint256& subscriber, ISelectBuzzFeedHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << timeframeFrom;
		lStateStream << scoreFrom;
		lStateStream << timestampFrom;
		lStateStream << publisher;
		lStateStream << subscriber;
		Message lMessage(GET_BUZZ_FEED_GLOBAL, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading buzzfeed global for ") + strprintf("%d/%s# from %s -> %s", timeframeFrom, chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectBuzzfeedByTag(const uint256& chain, const std::string& tag, uint64_t timeframeFrom, uint64_t scoreFrom, uint64_t timestampFrom, const uint256& publisher, const uint256& subscriber, ISelectBuzzFeedHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE || tag.size() < 256 /*sanity*/) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << tag;
		lStateStream << timeframeFrom;
		lStateStream << scoreFrom;
		lStateStream << timestampFrom;
		lStateStream << publisher; //?, check remaining size
		lStateStream << subscriber;
		Message lMessage(GET_BUZZ_FEED_BY_TAG, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading buzzfeed by tag for ") + strprintf("%d/%s# from %s -> %s", timeframeFrom, chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectBuzzfeedByBuzz(const uint256& chain, uint64_t from, const uint256& buzz, const uint256& subscriber, ISelectBuzzFeedByEntityHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << from;
		lStateStream << buzz; // 32+32+8+32
		lStateStream << subscriber;
		Message lMessage(GET_BUZZ_FEED_BY_BUZZ, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading buzzfeed by buzz ") + strprintf("%s/%s# from %s -> %s", buzz.toHex(), chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectBuzzfeedByBuzzer(const uint256& chain, uint64_t from, const uint256& buzzer, const uint256& subscriber, ISelectBuzzFeedByEntityHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << from;
		lStateStream << buzzer; // 32+32+8+32
		lStateStream << subscriber;
		Message lMessage(GET_BUZZ_FEED_BY_BUZZER, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading buzzfeed by buzzer ") + strprintf("%s/%s# from %s -> %s", buzzer.toHex(), chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectMistrustsByBuzzer(const uint256& chain, const uint256& from, const uint256& buzzer, ISelectEventsFeedByEntityHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << from;
		lStateStream << buzzer;
		Message lMessage(GET_MISTRUSTS_FEED_BY_BUZZER, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading mistrusts by buzzer ") + strprintf("%s/%s# from %s -> %s", buzzer.toHex(), chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectEndorsementsByBuzzer(const uint256& chain, const uint256& from, const uint256& buzzer, ISelectEventsFeedByEntityHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << from;
		lStateStream << buzzer;
		Message lMessage(GET_ENDORSEMENTS_FEED_BY_BUZZER, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading endorsements by buzzer ") + strprintf("%s/%s# from %s -> %s", buzzer.toHex(), chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectSubscriptionsByBuzzer(const uint256& chain, const uint256& from, const uint256& buzzer, ISelectEventsFeedByEntityHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << from;
		lStateStream << buzzer;
		Message lMessage(GET_BUZZER_SUBSCRIPTIONS, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading subscribers by buzzer ") + strprintf("%s/%s# from %s -> %s", buzzer.toHex(), chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectFollowersByBuzzer(const uint256& chain, const uint256& from, const uint256& buzzer, ISelectEventsFeedByEntityHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << from;
		lStateStream << buzzer;
		Message lMessage(GET_BUZZER_FOLLOWERS, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading followers by buzzer ") + strprintf("%s/%s# from %s -> %s", buzzer.toHex(), chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectEventsfeed(const uint256& chain, uint64_t from, const uint256& subscriber, ISelectEventsFeedHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << from;
		lStateStream << subscriber;
		Message lMessage(GET_EVENTS_FEED, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading eventsfeed for ") + strprintf("%s/%s# from %s -> %s", subscriber.toHex(), chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectBuzzes(const uint256& chain, const std::vector<uint256>& buzzes, ISelectBuzzFeedHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << buzzes;
		Message lMessage(GET_BUZZES, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading buzz list for ") + strprintf("%d/%s# from %s -> %s", buzzes.size(), chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectConversations(const uint256& chain, uint64_t from, const uint256& buzzer, ISelectConversationsFeedByEntityHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << buzzer;
		lStateStream << from;
		Message lMessage(GET_CONVERSATIONS_FEED_BY_BUZZER, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading conversations by buzzer ") + strprintf("%s/%s# from %s -> %s", buzzer.toHex(), chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectMessages(const uint256& chain, uint64_t from, const uint256& conversation, ISelectBuzzFeedByEntityHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << conversation;
		lStateStream << from;
		Message lMessage(GET_MESSAGES_FEED_BY_CONVERSATION, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading messages by conversation ") + strprintf("%s/%s# from %s -> %s", conversation.toHex(), chain.toHex().substr(0, 10), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::notifyNewBuzz(const BuzzfeedItem& buzz) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		lStateStream << buzz;
		Message lMessage(NEW_BUZZ_NOTIFY, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: new buzz notification ") + 
			strprintf("%s/%s# for %s -> %d/%s", 
				const_cast<BuzzfeedItem&>(buzz).buzzId().toHex(), 
				const_cast<BuzzfeedItem&>(buzz).buzzChainId().toHex().substr(0, 10), peer_->key(), lStateStream.size(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::notifyNewMessage(const BuzzfeedItem& buzz) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		lStateStream << buzz;
		Message lMessage(NEW_BUZZER_MESSAGE_NOTIFY, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: new message notification ") + 
			strprintf("%s/%s# for %s -> %d/%s", 
				const_cast<BuzzfeedItem&>(buzz).buzzId().toHex(), 
				const_cast<BuzzfeedItem&>(buzz).buzzChainId().toHex().substr(0, 10), peer_->key(), lStateStream.size(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::notifyNewEvent(const EventsfeedItem& buzz, bool tryForward) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		bool lProcess = false;
		uint64_t lTimestamp = getMicroseconds();
		std::vector<EventsfeedItem> lUpdates;
		//
		{
			boost::unique_lock<boost::recursive_mutex> lLock(notificationMutex_);
			if (lTimestamp - lastEventsfeedItemTimestamp_ > 1000000) {
				if (!tryForward) lastEventsfeedItems_.push_back(buzz);
				lUpdates.insert(lUpdates.end(), lastEventsfeedItems_.begin(), lastEventsfeedItems_.end());

				lastEventsfeedItems_.clear();
				lastEventsfeedItemTimestamp_ = lTimestamp;
				pendingEventsfeedItems_ = false;
				lProcess = true;
			} else {
				//
				lastEventsfeedItems_.push_back(buzz);
				pendingEventsfeedItems_ = true;
			}
		}

		if (lProcess) {
			// new message
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
			DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

			lStateStream << lUpdates;
			Message lMessage(NEW_EVENT_NOTIFY, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

			(*lMsg) << lMessage;
			lMsg->write(lStateStream.data(), lStateStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: new event notifications ") + 
				strprintf("%d for %s", lUpdates.size(), peer_->key()));

			// write
			peer_->sendMessageAsync(lMsg);
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::notifyNewConversation(const ConversationItem& conversation) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		//
		std::vector<ConversationItem> lUpdates;
		lUpdates.push_back(conversation);

		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		lStateStream << lUpdates;
		Message lMessage(NEW_BUZZER_CONVERSATION_NOTIFY, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: new conversation notification ") + 
			strprintf("%d for %s", conversation.conversationId().toHex(), peer_->key()));

		// write
		peer_->sendMessageAsync(lMsg);

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::notifyUpdateConversation(const ConversationItem::EventInfo& conversation) {
	//
	if (peer_->status() == IPeer::ACTIVE) {		
		//
		std::vector<ConversationItem::EventInfo> lUpdates;
		lUpdates.push_back(conversation);

		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		lStateStream << lUpdates;
		Message lMessage(UPDATE_BUZZER_CONVERSATION_NOTIFY, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: update conversation notification ") + 
			strprintf("%d for %s", conversation.conversationId().toHex(), peer_->key()));

		// write
		peer_->sendMessageAsync(lMsg);

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::notifyUpdateBuzz(const std::vector<BuzzfeedItemUpdate>& updates) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		//
		bool lProcess = false;
		uint64_t lTimestamp = getMicroseconds();
		std::vector<BuzzfeedItemUpdate> lUpdates;
		//
		{
			boost::unique_lock<boost::recursive_mutex> lLock(notificationMutex_);
			if (lTimestamp - lastBuzzfeedItemTimestamp_ > 1000000) {
				lUpdates.insert(lUpdates.end(), updates.begin(), updates.end());
				lUpdates.insert(lUpdates.end(), lastBuzzfeedItems_.begin(), lastBuzzfeedItems_.end());

				lastBuzzfeedItems_.clear();
				lastBuzzfeedItemTimestamp_ = lTimestamp;
				pendingBuzzfeedItems_ = false;
				lProcess = true;			
			} else {
				//
				lastBuzzfeedItems_.insert(lastBuzzfeedItems_.end(), updates.begin(), updates.end());
				pendingBuzzfeedItems_ = true;
			}
		}

		if (lProcess) {
			// new message
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
			DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

			lStateStream << lUpdates;
			Message lMessage(BUZZ_UPDATE_NOTIFY, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

			(*lMsg) << lMessage;
			lMsg->write(lStateStream.data(), lStateStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) {
				for (std::vector<BuzzfeedItemUpdate>::const_iterator lUpdate = lUpdates.begin(); lUpdate != lUpdates.end(); lUpdate++) {
					gLog().write(Log::NET, std::string("[peer/buzzer]: buzz update notification ") + 
						strprintf("%s/%s/%d for %s -> %d/%s", 
							const_cast<BuzzfeedItemUpdate&>(*lUpdate).buzzId().toHex(), 
							const_cast<BuzzfeedItemUpdate&>(*lUpdate).fieldString(),
							const_cast<BuzzfeedItemUpdate&>(*lUpdate).count(), peer_->key(), lStateStream.size(), HexStr(lStateStream.begin(), lStateStream.end())));
				}

				gLog().write(Log::NET, std::string("[peer/buzzer]: buzz update notification size ") + 
					strprintf("%d/%s", lStateStream.size(), HexStr(lStateStream.begin(), lStateStream.end())));
			}

			// write
			peer_->sendMessageAsync(lMsg);
		}

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::loadTrustScore(const uint256& chain, const uint256& buzzer, ILoadTrustScoreHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << buzzer;
		Message lMessage(GET_BUZZER_TRUST_SCORE, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading trust score ") + strprintf("%s from %s -> %s", buzzer.toHex(), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::notifyUpdateTrustScore(const BuzzerTransactionStoreExtension::BuzzerInfo& score) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		//
		bool lProcess = false;
		if (getMicroseconds() - lastScoreTimestamp_ > 1000000) {
			lastScore_ = score;
			lProcess = true;
		} else {
			//
			lastScore_ = score;
			pendingScore_ = true;
		}

		if (lProcess) {
			// new message
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
			DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

			lStateStream << lastScore_;
			Message lMessage(BUZZER_TRUST_SCORE_UPDATE, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

			(*lMsg) << lMessage;
			lMsg->write(lStateStream.data(), lStateStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: trust score update notification ") + 
				strprintf("%d/%d for %s -> %d/%s", score.endorsements(), score.mistrusts(), peer_->key(), lStateStream.size(), HexStr(lStateStream.begin(), lStateStream.end())));

			// write
			peer_->sendMessageAsync(lMsg);
			//
			lastScoreTimestamp_ = getMicroseconds();
			pendingScore_ = false;
		}

		return true;
	}

	return false;
}

void BuzzerPeerExtension::processBuzzerTrustScoreUpdateNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing trust score update notification from ") + peer_->key());

		// extract
		BuzzerTransactionStoreExtension::BuzzerInfo lScore;
		(*msg) >> lScore;
		peer_->eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing trust score notification ") + 
			strprintf("%d/%d from %s", lScore.endorsements(), lScore.mistrusts(), peer_->key()));

		// update
		buzzer_->updateTrustScore(lScore.endorsements(), lScore.mistrusts());

		// WARNING: in case of async_read for large data
		peer_->processed();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzerTrustScoreUpdateNotify/error]: closing session " + peer_->key() + " -> " + error.message());
		// releasing message
		peer_->eraseInData(msg);
		// close socket
		peer_->close(IPeer::GENERAL_ERROR);
	}
}

bool BuzzerPeerExtension::selectBuzzerEndorse(const uint256& chain, const uint256& actor, const uint256& buzzer, ILoadEndorseMistrustHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << actor;
		lStateStream << buzzer;
		Message lMessage(GET_BUZZER_ENDORSE_TX, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading endorse tx ") + strprintf("%s from %s -> %s", buzzer.toHex(), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectBuzzerMistrust(const uint256& chain, const uint256& actor, const uint256& buzzer, ILoadEndorseMistrustHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << actor;
		lStateStream << buzzer;
		Message lMessage(GET_BUZZER_MISTRUST_TX, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading mistrust tx ") + strprintf("%s from %s -> %s", buzzer.toHex(), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::subscribeBuzzThread(const uint256& chain, const uint256& buzzId, const uint512& signature) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		lStateStream << chain;
		lStateStream << buzzId;
		lStateStream << signature;
		Message lMessage(BUZZ_SUBSCRIBE, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: subscribing on buzz thread ") + strprintf("%s from %s -> %s", buzzId.toHex(), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::unsubscribeBuzzThread(const uint256& chain, const uint256& buzzId) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		lStateStream << chain;
		lStateStream << buzzId;
		Message lMessage(BUZZ_UNSUBSCRIBE, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: unsubscribing on buzz thread ") + strprintf("%s from %s -> %s", buzzId.toHex(), peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}

bool BuzzerPeerExtension::loadBuzzerAndInfo(const std::string& buzzer, ILoadBuzzerAndInfoHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = peer_->addRequest(handler);
		std::string lName(buzzer);
		entity_name_t lBuzzer(lName);
		lStateStream << lRequestId;
		lStateStream << lBuzzer;
		Message lMessage(GET_BUZZER_AND_INFO, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading buzzer and info ") + strprintf("%s from %s -> %s", buzzer, peer_->key(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		peer_->sendMessageAsync(lMsg);
		return true;
	}

	return false;
}
