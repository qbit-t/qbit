#include "peerextension.h"
#include "../../peer.h"
#include "../../log/log.h"

#include "txbuzzer.h"
#include "txbuzz.h"
#include "txbuzzreply.h"
#include "txbuzzlike.h"

using namespace qbit;

void BuzzerPeerExtension::prepareBuzzfeedItem(BuzzfeedItem& item, TxBuzzPtr buzz, TxBuzzerPtr buzzer, BuzzerTransactionStoreExtensionPtr extension) {
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
	BuzzerTransactionStoreExtension::BuzzInfo lInfo;
	if (extension->readBuzzInfo(buzz->id(), lInfo)) {
		item.setReplies(lInfo.replies_);
		item.setRebuzzes(lInfo.rebuzzes_);
		item.setLikes(lInfo.likes_);
	}
}

void BuzzerPeerExtension::processTransaction(TransactionContextPtr ctx) {
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
		uint256 lPublisher = (*lTx->in().begin()).out().tx(); // allways first

		if (lStore && lStore->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());
			// 
			// subscriber (peer) has direct publisher
			// -> new item
			if (lExtension->checkSubscription(peer_->state().dAppInstance(), lPublisher) || 
												peer_->state().dAppInstance() == lPublisher) {
				//
				TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lTx);

				BuzzfeedItem lItem;
				TxBuzzerPtr lBuzzer = nullptr;

				TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lPublisher);
				if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);

				prepareBuzzfeedItem(lItem, lBuzz, lBuzzer, lExtension);

				//
				if (ctx->tx()->type() == TX_REBUZZ || ctx->tx()->type() == TX_BUZZ_REPLY) {
					// NOTE: should be in the same chain
					uint256 lOriginalId = lBuzz->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
					lItem.setOriginalBuzzId(lOriginalId);
				}

				notifyNewBuzz(lItem);

			} else {
				//
				TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lTx);
				std::vector<Transaction::In>& lIns = lBuzz->in(); 
				std::vector<Transaction::In>::iterator lInPtr = lIns.begin(); lInPtr++; // buzzer pointer
				for(; lInPtr != lIns.end(); lInPtr++) {
					//
					Transaction::In& lIn = (*lInPtr);
					ITransactionStorePtr lLocalStore = peerManager_->consensusManager()->storeManager()->locate(lIn.out().chain());
					TransactionPtr lInTx = lLocalStore->locateTransaction(lIn.out().tx());

					if (lInTx != nullptr) {
						//
						// next level - parents
						if (lInTx->type() == TX_BUZZER) {
							// direct parent
							// mentions - when subscriber (peer) was mentioned (replied/rebuzzed direcly)
							// -> new item
							if (peer_->state().dAppInstance() == lInTx->id()) {
								//
								TxBuzzPtr lBuzz = TransactionHelper::to<TxBuzz>(lTx);

								BuzzfeedItem lItem;
								TxBuzzerPtr lBuzzer = nullptr;

								TransactionPtr lBuzzerTx = lMainStore->locateTransaction(lPublisher);
								if (lBuzzerTx) lBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzerTx);

								prepareBuzzfeedItem(lItem, lBuzz, lBuzzer, lExtension);

								//
								if (ctx->tx()->type() == TX_REBUZZ || ctx->tx()->type() == TX_BUZZ_REPLY) {
									// NOTE: should be in the same chain
									uint256 lOriginalId = lBuzz->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
									lItem.setOriginalBuzzId(lOriginalId);
								}

								notifyNewBuzz(lItem);
							}
						} else if (/*lInTx->type() == TX_BUZZ ||*/ lInTx->type() == TX_REBUZZ || lInTx->type() == TX_BUZZ_REPLY) {
							//
							std::vector<BuzzfeedItem::Update> lUpdates;
							lPublisher = (*lInTx->in().begin()).out().tx(); // allways first

							// check
							// if subscriber (peer) has subscription on this new publisher
							if (lExtension->checkSubscription(peer_->state().dAppInstance(), lPublisher) || 
																	peer_->state().dAppInstance() == lPublisher) {

								uint256 lReTxId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
								uint256 lReChainId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();

								BuzzerTransactionStoreExtension::BuzzInfo lInfo;
								if (lExtension->readBuzzInfo(lReTxId, lInfo)) {
									if (ctx->tx()->type() == TX_BUZZ_REPLY) {
										BuzzfeedItem::Update lItem(lReTxId, BuzzfeedItem::Update::REPLIES, ++lInfo.replies_);
										lUpdates.push_back(lItem);
									} else if (ctx->tx()->type() == TX_REBUZZ) {
											BuzzfeedItem::Update lItem(lReTxId, BuzzfeedItem::Update::REBUZZES, ++lInfo.rebuzzes_);
											lUpdates.push_back(lItem);
									}
								}
							}

							if (lInTx->type() == TX_BUZZ_REPLY) {
								//
								uint256 lReTxId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().tx();
								uint256 lReChainId = lInTx->in()[TX_BUZZ_REPLY_BUZZ_IN].out().chain();
								//
								while (true) {
									//
									lLocalStore = peerManager_->consensusManager()->storeManager()->locate(lReChainId);
									TransactionPtr lReTx = lLocalStore->locateTransaction(lReTxId);

									if (lReTx->type() == TX_BUZZ_REPLY || lReTx->type() == TX_REBUZZ ||
											lReTx->type() == TX_BUZZ) {
										// make update
										BuzzerTransactionStoreExtension::BuzzInfo lInfo;
										if (lExtension->readBuzzInfo(lReTxId, lInfo)) {
											// if we have one
											if (ctx->tx()->type() == TX_BUZZ_REPLY) {
												BuzzfeedItem::Update lItem(lReTxId, BuzzfeedItem::Update::REPLIES, ++lInfo.replies_);
												lUpdates.push_back(lItem);
											} else {
												BuzzfeedItem::Update lItem(lReTxId, BuzzfeedItem::Update::REBUZZES, ++lInfo.rebuzzes_);
												lUpdates.push_back(lItem);
											}
										} else {
											// default
											if (ctx->tx()->type() == TX_BUZZ_REPLY) {
												BuzzfeedItem::Update lItem(lReTxId, BuzzfeedItem::Update::REPLIES, 1);
												lUpdates.push_back(lItem); 
											} else {
												BuzzfeedItem::Update lItem(lReTxId, BuzzfeedItem::Update::REBUZZES, 1);
												lUpdates.push_back(lItem); 
											}
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

							if (lUpdates.size()) {
								notifyUpdateBuzz(lUpdates);
							}
						}
					}
				}
			}
		}
	} else if (ctx->tx()->type() == TX_BUZZ_LIKE) {
		//
		// check subscription
		TransactionPtr lTx = ctx->tx();
		// get store
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lTx->chain());
		// process
		if (lStore && lStore->extension()) {
			BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());

			// load buzz
			TransactionPtr lBuzz = lStore->locateTransaction(lTx->in()[TX_BUZZ_LIKE_IN].out().tx());
			if (lBuzz) {
				TxBuzzPtr lBuzzTx = TransactionHelper::to<TxBuzz>(lBuzz);
				uint256 lPublisher = lBuzzTx->in()[TX_BUZZ_MY_IN].out().tx(); // buzzer

				if (lExtension->checkSubscription(peer_->state().dAppInstance(), lPublisher) ||
					lExtension->checkSubscription(lPublisher, peer_->state().dAppInstance()) || 
					peer_->state().dAppInstance() == lPublisher) {
					// make update
					BuzzerTransactionStoreExtension::BuzzInfo lInfo;
					if (lExtension->readBuzzInfo(lBuzz->id(), lInfo)) {
						// if we have one
						BuzzfeedItem::Update lItem(lBuzz->id(), BuzzfeedItem::Update::LIKES, ++lInfo.likes_);
						std::vector<BuzzfeedItem::Update> lUpdates; lUpdates.push_back(lItem);
						notifyUpdateBuzz(lUpdates);
					} else {
						// default
						BuzzfeedItem::Update lItem(lBuzz->id(), BuzzfeedItem::Update::LIKES, 1);
						std::vector<BuzzfeedItem::Update> lUpdates; lUpdates.push_back(lItem);
						notifyUpdateBuzz(lUpdates);
					}
				}
			}
		}
	}
}

bool BuzzerPeerExtension::processMessage(Message& message, std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (message.type() == GET_BUZZER_SUBSCRIPTION) {
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			boost::bind(
				&BuzzerPeerExtension::processGetSubscription, shared_from_this(), msg,
				boost::asio::placeholders::error));
	} else if (message.type() == BUZZER_SUBSCRIPTION) {
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			boost::bind(
				&BuzzerPeerExtension::processSubscription, shared_from_this(), msg,
				boost::asio::placeholders::error));
	} else if (message.type() == BUZZER_SUBSCRIPTION_IS_ABSENT) {
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			boost::bind(
				&BuzzerPeerExtension::processSubscriptionAbsent, shared_from_this(), msg,
				boost::asio::placeholders::error));
	} else if (message.type() == GET_BUZZ_FEED) {
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			boost::bind(
				&BuzzerPeerExtension::processGetBuzzfeed, shared_from_this(), msg,
				boost::asio::placeholders::error));
	} else if (message.type() == BUZZ_FEED) {
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			boost::bind(
				&BuzzerPeerExtension::processBuzzfeed, shared_from_this(), msg,
				boost::asio::placeholders::error));
	} else if (message.type() == NEW_BUZZ_NOTIFY) {
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			boost::bind(
				&BuzzerPeerExtension::processNewBuzzNotify, shared_from_this(), msg,
				boost::asio::placeholders::error));
	} else if (message.type() == BUZZ_UPDATE_NOTIFY) {
		boost::asio::async_read(*peer_->socket(),
			boost::asio::buffer(msg->data(), message.dataSize()),
			boost::bind(
				&BuzzerPeerExtension::processBuzzUpdateNotify, shared_from_this(), msg,
				boost::asio::placeholders::error));
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
				DataStream lStream(SER_NETWORK, CLIENT_VERSION);
				lStream << lRequestId;
				Transaction::Serializer::serialize<DataStream>(lStream, lSubscription); // tx

				// prepare message
				Message lMessage(BUZZER_SUBSCRIPTION, lStream.size(), Hash160(lStream.begin(), lStream.end()));
				(*lMsg) << lMessage;
				lMsg->write(lStream.data(), lStream.size());

				// log
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: sending subscription transacion ") + strprintf("%s", lSubscription->id().toHex()) + std::string(" for ") + peer_->key());

				// write
				boost::asio::async_write(*peer_->socket(),
					boost::asio::buffer(lMsg->data(), lMsg->size()),
					boost::bind(
						&Peer::messageFinalize, std::static_pointer_cast<Peer>(peer_), lMsg,
						boost::asio::placeholders::error));
			}
		} 

		// not found
		if (!lSubscription) {
			// tx is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lRequestId;

			// prepare message
			Message lMessage(BUZZER_SUBSCRIPTION_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: subscription is absent for ") + strprintf("%s/%s", lSubscriber.toHex(), lPublisher.toHex()) + std::string(" -> ") + peer_->key());

			// write
			boost::asio::async_write(*peer_->socket(),
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, std::static_pointer_cast<Peer>(peer_), lMsg,
					boost::asio::placeholders::error));
		}

		//
		peer_->waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetSubscription/error]: closing session " + peer_->key() + " -> " + error.message());
		// try to deactivate peer
		peerManager_->deactivatePeer(peer_);
		// close socket
		peer_->close();
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
			std::vector<BuzzfeedItem> lFeed;
			lExtension->selectBuzzfeed(lFrom, lSubscriber, lFeed);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = peer_->newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
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
			boost::asio::async_write(*peer_->socket(),
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, std::static_pointer_cast<Peer>(peer_), lMsg,
					boost::asio::placeholders::error));
		}

		//
		peer_->waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBuzzfeed/error]: closing session " + peer_->key() + " -> " + error.message());
		// try to deactivate peer
		peerManager_->deactivatePeer(peer_);
		// close socket
		peer_->close();
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

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadTransactionHandler>(lHandler)->handleReply(nullptr);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND, ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: subscription is absent."));

		//
		peer_->waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processSubscriptionAbsent/error]: closing session " + peer_->key() + " -> " + error.message());
		// try to deactivate peer
		peerManager_->deactivatePeer(peer_);
		// close socket
		peer_->close();
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

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing subscription transaction ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadTransactionHandler>(lHandler)->handleReply(lTx);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for transaction ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));
		}

		// WARNING: in case of async_read for large data
		peer_->waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processSubscription/error]: closing session " + peer_->key() + " -> " + error.message());
		// try to deactivate peer
		peerManager_->deactivatePeer(peer_);
		// close socket
		peer_->close();
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

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing buzzfeed ") + strprintf("r = %s, %d/%s#", lRequestId.toHex(), lFeed.size(), lChain.toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = peer_->locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectBuzzFeedHandler>(lHandler)->handleReply(lFeed, lChain);
			peer_->removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: request was NOT FOUND for buzzfeed ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}

		// WARNING: in case of async_read for large data
		peer_->waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzfeed/error]: closing session " + peer_->key() + " -> " + error.message());
		// try to deactivate peer
		peerManager_->deactivatePeer(peer_);
		// close socket
		peer_->close();
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

		// update
		buzzfeed_->merge(lBuzz);

		// WARNING: in case of async_read for large data
		peer_->waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzfeed/error]: closing session " + peer_->key() + " -> " + error.message());
		// try to deactivate peer
		peerManager_->deactivatePeer(peer_);
		// close socket
		peer_->close();
	}
}

void BuzzerPeerExtension::processBuzzUpdateNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: processing buzz update notification from ") + peer_->key());

		// extract
		std::vector<BuzzfeedItem::Update> lBuzzUpdates;
		(*msg) >> lBuzzUpdates;
		peer_->eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) {
			for (std::vector<BuzzfeedItem::Update>::iterator lUpdate = lBuzzUpdates.begin(); lUpdate != lBuzzUpdates.end(); lUpdate++) {
				gLog().write(Log::NET, std::string("[peer/buzzer]: processing buzz update notification ") + 
				strprintf("%s/%s/%d from %s", lUpdate->buzzId().toHex(), lUpdate->fieldString(), lUpdate->count(), peer_->key()));
			}
		}

		// update
		buzzfeed_->merge(lBuzzUpdates);

		// WARNING: in case of async_read for large data
		peer_->waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBuzzfeed/error]: closing session " + peer_->key() + " -> " + error.message());
		// try to deactivate peer
		peerManager_->deactivatePeer(peer_);
		// close socket
		peer_->close();
	}
}

//
bool BuzzerPeerExtension::loadSubscription(const uint256& chain, const uint256& subscriber, const uint256& publisher, ILoadTransactionHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

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
		boost::asio::async_write(*peer_->socket(),
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, std::static_pointer_cast<Peer>(peer_), lMsg,
				boost::asio::placeholders::error));

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::selectBuzzfeed(const uint256& chain, uint64_t from, const uint256& subscriber, ISelectBuzzFeedHandlerPtr handler) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

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
		boost::asio::async_write(*peer_->socket(),
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, std::static_pointer_cast<Peer>(peer_), lMsg,
				boost::asio::placeholders::error));

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::notifyNewBuzz(const BuzzfeedItem& buzz) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		lStateStream << buzz;
		Message lMessage(NEW_BUZZ_NOTIFY, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: new buzz notification ") + 
			strprintf("%s/%s# for %s -> %d/%s", const_cast<BuzzfeedItem&>(buzz).buzzId().toHex(), const_cast<BuzzfeedItem&>(buzz).buzzChainId().toHex().substr(0, 10), peer_->key(), lStateStream.size(), HexStr(lStateStream.begin(), lStateStream.end())));

		// write
		boost::asio::async_write(*peer_->socket(),
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, std::static_pointer_cast<Peer>(peer_), lMsg,
				boost::asio::placeholders::error));

		return true;
	}

	return false;
}

bool BuzzerPeerExtension::notifyUpdateBuzz(const std::vector<BuzzfeedItem::Update>& updates) {
	//
	if (peer_->status() == IPeer::ACTIVE) {
		
		// new message
		std::list<DataStream>::iterator lMsg = peer_->newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		lStateStream << updates;
		Message lMessage(BUZZ_UPDATE_NOTIFY, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) {
			for (std::vector<BuzzfeedItem::Update>::const_iterator lUpdate = updates.begin(); lUpdate != updates.end(); lUpdate++) {
				gLog().write(Log::NET, std::string("[peer/buzzer]: buzz update notification ") + 
					strprintf("%s/%s/%d for %s -> %d/%s", 
						const_cast<BuzzfeedItem::Update&>(*lUpdate).buzzId().toHex(), 
						const_cast<BuzzfeedItem::Update&>(*lUpdate).fieldString(),
						const_cast<BuzzfeedItem::Update&>(*lUpdate).count(), peer_->key(), lStateStream.size(), HexStr(lStateStream.begin(), lStateStream.end())));
			}

			gLog().write(Log::NET, std::string("[peer/buzzer]: buzz update notification size ") + 
				strprintf("%d/%s", lStateStream.size(), HexStr(lStateStream.begin(), lStateStream.end())));
		}

		// write
		boost::asio::async_write(*peer_->socket(),
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, std::static_pointer_cast<Peer>(peer_), lMsg,
				boost::asio::placeholders::error));

		return true;
	}

	return false;
}
