// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_SERVER_H
#define QBIT_SERVER_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
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

class Server: public std::enable_shared_from_this<Server> {
public:
	Server(ISettingsPtr settings, IPeerManagerPtr peerManager) : 
		Server(settings, peerManager, settings->serverPort()) {
	}

	Server(ISettingsPtr settings, IPeerManagerPtr peerManager, int port) : 
		settings_(settings), peerManager_(peerManager),
		signals_(peerManager_->getContext(0)), port_(port) {
		//
		signals_.add(SIGINT);
		signals_.add(SIGTERM);
#if defined(SIGQUIT)
		signals_.add(SIGQUIT);
#endif
		signals_.async_wait(boost::bind(&Server::stop, this));
	}

	static ServerPtr instance(ISettingsPtr settings, IPeerManagerPtr peerManager) { 
		return std::make_shared<Server>(settings, peerManager); 
	}

	static ServerPtr instance(ISettingsPtr settings, IPeerManagerPtr peerManager, int port) { 
		return std::make_shared<Server>(settings, peerManager, port); 
	}

	void run() {
		gLog().write(Log::NET, "[server]: starting...");
		timer_.reset(new boost::asio::steady_timer(
				peerManager_->getContext(0), 
				boost::asio::chrono::seconds(20))); // time to warm-up (validator & sharding managers)
		timer_->async_wait(boost::bind(&Server::startEndpoint, shared_from_this()));			
		peerManager_->run();
	}

	void stop() {
		gLog().write(Log::NET, "[server]: stopping...");
		peerManager_->stop();
	}

private:	
	void startEndpoint() {
		//
		if (settings_->ipV6Enabled()) {
			gLog().write(Log::NET, "[server]: starting V6 endpoint...");
			boost::system::error_code lError;
			endpoint6_.reset(new tcp::endpoint(tcp::v6(), port_));
			acceptor6_.reset(new tcp::acceptor(peerManager_->getContext(0)));
			acceptor6_->open(endpoint6_->protocol());
			acceptor6_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			acceptor6_->set_option(boost::asio::ip::v6_only(true), lError); // ONLY IPV6 connections accepting
			if (lError) gLog().write(Log::NET, "[server/v6/error]: acceptor set option - " + lError.message());
			else {
				acceptor6_->bind(*endpoint6_);
				acceptor6_->listen();
				accept6();
			}
		}

		gLog().write(Log::NET, "[server]: starting V4 endpoint...");
		endpoint4_.reset(new tcp::endpoint(tcp::v4(), port_));
		acceptor4_.reset(new tcp::acceptor(peerManager_->getContext(0), (tcp::endpoint&)(*endpoint4_)));
		accept4();
	}

	void accept4() {
		gLog().write(Log::NET, "[server]: V4 accepting...");

		IPeerPtr lPeer(new Peer(peerManager_->getContextId(), peerManager_));
		acceptor4_->async_accept(*lPeer->socket(),
			boost::bind(
				&Server::processAccept4, this, lPeer,
				boost::asio::placeholders::error));
	}

	void accept6() {
		gLog().write(Log::NET, "[server]: V6 accepting...");

		IPeerPtr lPeer(new Peer(peerManager_->getContextId(), peerManager_));
		acceptor6_->async_accept(*lPeer->socket(),
			boost::bind(
				&Server::processAccept6, this, lPeer,
				boost::asio::placeholders::error));
	}

	void processAccept4(IPeerPtr peer, const boost::system::error_code& error) {
		//
		if (!error) {
			// log
			gLog().write(Log::NET, "[server]: starting V4 session for " + peer->key());
			peer->waitForMessage();
		} else {
			gLog().write(Log::NET, "[server/v4/error]: accept - " + error.message());
		}

		accept4();
	}

	void processAccept6(IPeerPtr peer, const boost::system::error_code& error) {
		//
		if (!error) {
			// log
			gLog().write(Log::NET, "[server]: starting V6 session for " + peer->key());
			peer->waitForMessage();
		} else {
			gLog().write(Log::NET, "[server/v6/error]: accept - " + error.message());
		}

		accept6();
	}

private:
	typedef std::shared_ptr<boost::asio::steady_timer> TimerPtr;
	typedef std::shared_ptr<tcp::endpoint> EndpointPtr;
	typedef std::shared_ptr<tcp::acceptor> AcceptorPtr;

	ISettingsPtr settings_;
	IPeerManagerPtr peerManager_;
	TimerPtr timer_;

	int port_;
	boost::asio::signal_set signals_;
	EndpointPtr endpoint4_;
	AcceptorPtr acceptor4_;
	EndpointPtr endpoint6_;
	AcceptorPtr acceptor6_;
};

}

#endif