// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_HTTP_SERVER_H
#define QBIT_HTTP_SERVER_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include "isettings.h"
#include "log/log.h"
#include "httpconnection.h"
#include "httpconnectionmanager.h"

#include <boost/bind.hpp>

namespace qbit {

class HttpServer;
typedef std::shared_ptr<HttpServer> HttpServerPtr;

class HttpServer {
public:
	HttpServer(ISettingsPtr settings, HttpConnectionManagerPtr connectionManager) : 
		HttpServer(settings, connectionManager, settings->httpServerPort()) {
	}

	HttpServer(ISettingsPtr settings, HttpConnectionManagerPtr connectionManager, int port) : 
		settings_(settings), connectionManager_(connectionManager),
		signals_(connectionManager_->getContext(0)),
		endpoint4_(tcp::v4(), port),
		acceptor4_(connectionManager_->getContext(0), endpoint4_) {
		//
		signals_.add(SIGINT);
		signals_.add(SIGTERM);
#if defined(SIGQUIT)
		signals_.add(SIGQUIT);
#endif
		signals_.async_wait(boost::bind(&HttpServer::stop, this));

		// SO_REUSEADDR
		acceptor4_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

		accept4();
	}

	static HttpServerPtr instance(ISettingsPtr settings, HttpConnectionManagerPtr connectionManager) { 
		return std::make_shared<HttpServer>(settings, connectionManager); 
	}

	static HttpServerPtr instance(ISettingsPtr settings, HttpConnectionManagerPtr connectionManager, int port) { 
		return std::make_shared<HttpServer>(settings, connectionManager, port); 
	}

	void runWait() {
		gLog().write(Log::HTTP, "[server]: starting...");
		connectionManager_->run();
		connectionManager_->wait();
	}

	void run() {
		gLog().write(Log::HTTP, "[server]: starting...");
		connectionManager_->run();
	}

	void stop() {
		gLog().write(Log::HTTP, "[server]: stopping...");
		connectionManager_->stop();
	}

private:
	void accept4() {
		gLog().write(Log::HTTP, "[server]: accepting...");

		HttpConnectionPtr lConnection(new HttpConnection(connectionManager_->getContext(), connectionManager_->requestHandler()));
		acceptor4_.async_accept(*lConnection->socket(),
			boost::bind(
				&HttpServer::processAccept4, this, lConnection,
				boost::asio::placeholders::error));
	}

	void processAccept4(HttpConnectionPtr connection, const boost::system::error_code& error) {
		//
		if (!error) {
			// log
			gLog().write(Log::HTTP, "[server]: starting connection for " + connection->key());
			connection->waitForRequest();
		}

		accept4();
	}

private:
	ISettingsPtr settings_;
	HttpConnectionManagerPtr connectionManager_;

	boost::asio::signal_set signals_;
	tcp::endpoint endpoint4_;
	tcp::acceptor acceptor4_;
};

}

#endif