#include "peerextension.h"
#include "transactionstoreextension.h"
#include "../../peer.h"
#include "../../log/log.h"

using namespace qbit;

void BuzzerPeerExtension::processTransaction(TransactionContextPtr ctx) {
	// TODO: process under subscriptions and prepare updates if any
}

bool BuzzerPeerExtension::processMessage(Message& message, std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lSkipWaitForMessage = false;

	//
	// WARNING: for short requests, but for long - we _must_ to go to wait until we read and process data
	//
	lSkipWaitForMessage = 
		message.type() == BUZZER_SUBSCRIPTION;
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
	} else {
		return false;
	}

	if (!lSkipWaitForMessage) 
		peer_->waitForMessage();

	return true;
}

void BuzzerPeerExtension::processGetSubscription(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: checksum is INVALID for message from ") + peer_->key());
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

	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetSubscription/error]: closing session " + peer_->key() + " -> " + error.message());
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

//
void BuzzerPeerExtension::loadSubscription(const uint256& chain, const uint256& subscriber, const uint256& publisher, ILoadTransactionHandlerPtr handler) {
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
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/buzzer]: loading subscription ") + strprintf("sub = %s, pub = %s", subscriber.toHex(), publisher.toHex()) + std::string(" from ") + peer_->key());

		// write
		boost::asio::async_write(*peer_->socket(),
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, std::static_pointer_cast<Peer>(peer_), lMsg,
				boost::asio::placeholders::error));
	}
}