// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_PEER_EXTENSION_H
#define QBIT_BUZZER_PEER_EXTENSION_H

#include "../../ipeerextension.h"
#include "../../ipeermanager.h"
#include "../../ipeer.h"
#include "../../message.h"

namespace qbit {

// register message type
#define GET_BUZZER_SUBSCRIPTION 		Message::CUSTOM_0000
#define BUZZER_SUBSCRIPTION 			Message::CUSTOM_0001
#define BUZZER_SUBSCRIPTION_IS_ABSENT 	Message::CUSTOM_0002

//
class BuzzerPeerExtension: public IPeerExtension, public std::enable_shared_from_this<BuzzerPeerExtension> {
public:
	BuzzerPeerExtension(IPeerPtr peer, IPeerManagerPtr peerManager): peer_(peer), peerManager_(peerManager) {}

	void processTransaction(TransactionContextPtr /*tx*/);
	bool processMessage(Message& /*message*/, std::list<DataStream>::iterator /*data*/, const boost::system::error_code& /*error*/);

	static IPeerExtensionPtr instance(IPeerPtr peer, IPeerManagerPtr peerManager) {
		return std::make_shared<BuzzerPeerExtension>(peer, peerManager);
	}

	//
	void loadSubscription(const uint256& /*chain*/, const uint256& /*subscriber*/, const uint256& /*publisher*/, ILoadTransactionHandlerPtr /*handler*/);

private:
	void processGetSubscription(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processSubscription(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processSubscriptionAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

public:
	IPeerPtr peer_;
	IPeerManagerPtr peerManager_;
};

//
class BuzzerPeerExtensionCreator: public PeerExtensionCreator {
public:
	BuzzerPeerExtensionCreator() {}
	IPeerExtensionPtr create(IPeerPtr peer, IPeerManagerPtr peerManager) {
		return BuzzerPeerExtension::instance(peer, peerManager);
	}

	static PeerExtensionCreatorPtr instance() {
		return std::make_shared<BuzzerPeerExtensionCreator>();
	}	
};

} // qbit

#endif
