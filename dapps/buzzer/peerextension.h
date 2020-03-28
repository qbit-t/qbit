// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_PEER_EXTENSION_H
#define QBIT_BUZZER_PEER_EXTENSION_H

#include "../../ipeerextension.h"
#include "../../ipeermanager.h"
#include "../../ipeer.h"
#include "../../message.h"

#include "buzzfeed.h"

namespace qbit {

// register message type
#define GET_BUZZER_SUBSCRIPTION 		Message::CUSTOM_0000
#define BUZZER_SUBSCRIPTION 			Message::CUSTOM_0001
#define BUZZER_SUBSCRIPTION_IS_ABSENT 	Message::CUSTOM_0002
#define GET_BUZZ_FEED					Message::CUSTOM_0003
#define BUZZ_FEED						Message::CUSTOM_0004
#define NEW_BUZZ_NOTIFY					Message::CUSTOM_0005
#define BUZZ_INFO_NOTIFY				Message::CUSTOM_0006

//
class BuzzerPeerExtension: public IPeerExtension, public std::enable_shared_from_this<BuzzerPeerExtension> {
public:
	BuzzerPeerExtension(IPeerPtr peer, IPeerManagerPtr peerManager, BuzzfeedPtr buzzfeed): peer_(peer), peerManager_(peerManager), buzzfeed_(buzzfeed) {}

	void processTransaction(TransactionContextPtr /*tx*/);
	bool processMessage(Message& /*message*/, std::list<DataStream>::iterator /*data*/, const boost::system::error_code& /*error*/);

	static IPeerExtensionPtr instance(IPeerPtr peer, IPeerManagerPtr peerManager, BuzzfeedPtr buzzfeed) {
		return std::make_shared<BuzzerPeerExtension>(peer, peerManager, buzzfeed);
	}

	//
	// client-side facade methods
	bool loadSubscription(const uint256& /*chain*/, const uint256& /*subscriber*/, const uint256& /*publisher*/, ILoadTransactionHandlerPtr /*handler*/);
	bool selectBuzzfeed(const uint256& /*chain*/, uint64_t /*from*/, const uint256& /*subscriber*/, ISelectBuzzFeedHandlerPtr /*handler*/);

	//
	// node-to-peer notifications  
	bool notifyNewBuzz(const BuzzfeedItem& /*buzz*/);

private:
	// server-side processing
	void processGetSubscription(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBuzzfeed(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	// client-side answer processing
	void processSubscription(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBuzzfeed(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processNewBuzzNotify(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	//
	void processSubscriptionAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

public:
	IPeerPtr peer_;
	IPeerManagerPtr peerManager_;
	BuzzfeedPtr buzzfeed_;
};

//
class BuzzerPeerExtensionCreator: public PeerExtensionCreator {
public:
	BuzzerPeerExtensionCreator() {}
	BuzzerPeerExtensionCreator(BuzzfeedPtr buzzfeed): buzzfeed_(buzzfeed) {}
	IPeerExtensionPtr create(IPeerPtr peer, IPeerManagerPtr peerManager) {
		return BuzzerPeerExtension::instance(peer, peerManager, buzzfeed_);
	}

	static PeerExtensionCreatorPtr instance(BuzzfeedPtr buzzfeed) {
		return std::make_shared<BuzzerPeerExtensionCreator>(buzzfeed);
	}

	static PeerExtensionCreatorPtr instance() {
		return std::make_shared<BuzzerPeerExtensionCreator>();
	}

private:
	BuzzfeedPtr buzzfeed_ = nullptr;
};

} // qbit

#endif
