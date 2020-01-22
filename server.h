// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_SERVER_H
#define QBIT_SERVER_H

#include "iconsensus.h"
#include "isettings.h"
#include "state.h"
#include "ipeermanager.h"
#include "log/log.h"
#include "peer.h"

#include <boost/bind.hpp>

namespace qbit {

class Server;
typedef std::shared_ptr<Server> ServerPtr;

class Server {
public:
	Server(ISettingsPtr settings, IPeerManagerPtr peerManager) : 
		Server(settings, peerManager, settings->serverPort()) {
	}

	Server(ISettingsPtr settings, IPeerManagerPtr peerManager, int port) : 
		settings_(settings), peerManager_(peerManager),
		signals_(peerManager_->getContext(0)),
		endpoint4_(tcp::v4(), port),
		acceptor4_(peerManager_->getContext(0), endpoint4_) {
		//
		signals_.add(SIGINT);
		signals_.add(SIGTERM);
#if defined(SIGQUIT)
		signals_.add(SIGQUIT);
#endif
		signals_.async_wait(boost::bind(&Server::stop, this));

		accept4();
	}

	static ServerPtr instance(ISettingsPtr settings, IPeerManagerPtr peerManager) { 
		return std::make_shared<Server>(settings, peerManager); 
	}

	static ServerPtr instance(ISettingsPtr settings, IPeerManagerPtr peerManager, int port) { 
		return std::make_shared<Server>(settings, peerManager, port); 
	}

	void run() {
		gLog().write(Log::NET, "[server]: starting...");
		peerManager_->run();
	}

	void stop() {
		gLog().write(Log::NET, "[server]: stopping...");
		peerManager_->stop();
	}

private:
	void accept4() {
		gLog().write(Log::NET, "[server]: accepting...");

		IPeerPtr lPeer(new Peer(peerManager_->getContextId(), peerManager_));
		acceptor4_.async_accept(*lPeer->socket(),
			boost::bind(
				&Server::processAccept4, this, lPeer,
				boost::asio::placeholders::error));
	}

	void processAccept4(IPeerPtr peer, const boost::system::error_code& error) {
		//
		if (!error) {
			// log
			gLog().write(Log::NET, "[server]: starting session for " + peer->key());
			peer->waitForMessage();
		}

		accept4();
	}

private:
	ISettingsPtr settings_;
	IPeerManagerPtr peerManager_;

	boost::asio::signal_set signals_;
	tcp::endpoint endpoint4_;
	tcp::acceptor acceptor4_;
};

}

#endif