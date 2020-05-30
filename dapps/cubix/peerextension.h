// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CUBIX_PEER_EXTENSION_H
#define QBIT_CUBIX_PEER_EXTENSION_H

#include "../../ipeerextension.h"
#include "../../ipeermanager.h"
#include "../../ipeer.h"
#include "../../message.h"

#include "cubix.h"

namespace qbit {
namespace cubix {

//
class PeerExtension: public IPeerExtension, public std::enable_shared_from_this<PeerExtension> {
public:
	PeerExtension(const State::DAppInstance& dapp, IPeerPtr peer, IPeerManagerPtr peerManager): 
		peer_(peer), peerManager_(peerManager) { dapp_ = dapp; }

	void processTransaction(TransactionContextPtr /*tx*/) {}
	bool processMessage(Message& /*message*/, std::list<DataStream>::iterator /*data*/, const boost::system::error_code& /*error*/) {
		return false;
	}

	static IPeerExtensionPtr instance(const State::DAppInstance& dapp, IPeerPtr peer, IPeerManagerPtr peerManager) {
		return std::make_shared<PeerExtension>(dapp, peer, peerManager);
	}

	void release() {
		peer_.reset();
		peerManager_.reset();
	}

private:
	IPeerPtr peer_;
	IPeerManagerPtr peerManager_;
};

//
class DefaultPeerExtensionCreator: public PeerExtensionCreator {
public:
	DefaultPeerExtensionCreator() {}
	IPeerExtensionPtr create(const State::DAppInstance& dapp, IPeerPtr peer, IPeerManagerPtr peerManager) {
		return PeerExtension::instance(dapp, peer, peerManager);
	}

	static PeerExtensionCreatorPtr instance() {
		return std::make_shared<DefaultPeerExtensionCreator>();
	}
};

} // cubix
} // qbit

#endif
