// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_PEER_EXTENSION_H
#define QBIT_BUZZER_PEER_EXTENSION_H

#include "../../ipeerextension.h"
#include "../../ipeermanager.h"
#include "../../ipeer.h"

namespace qbit {

//
class BuzzerPeerExtension: public IPeerExtension, public std::enable_shared_from_this<BuzzerPeerExtension> {
public:
	BuzzerPeerExtension(IPeerPtr peer, IPeerManagerPtr peerManager): peer_(peer), peerManager_(peerManager) {}

	void processTransaction(TransactionContextPtr /*tx*/);
	bool processMessage(Message& /*message*/, std::list<DataStream>::iterator /*data*/, const boost::system::error_code& /*error*/);

	static IPeerExtensionPtr instance(IPeerPtr peer, IPeerManagerPtr peerManager) {
		return std::make_shared<BuzzerPeerExtension>(peer, peerManager);
	}

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
};

} // qbit

#endif
