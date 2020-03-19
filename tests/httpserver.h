// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_HTTPSERVER_H
#define QBIT_UNITTEST_HTTPSERVER_H

#include <string>
#include <list>

#include "unittest.h"
#include "../httpserver.h"
#include "../wallet.h"
#include "../httpendpoints.h"

namespace qbit {
namespace tests {

class SettingsHttp: public ISettings {
public:
	SettingsHttp() {}

	std::string dataPath() { return "/tmp/.qbitHttp"; }
	size_t keysCache() { return 0; }

	qunit_t maxFeeRate() { return QUNIT * 10; } // 10 qunits per byte	

	int serverPort() { return 31415; } // main net

	size_t threadPoolSize() { return 2; } // tread pool size

	uint32_t roles() { return State::PeerRoles::FULLNODE|State::PeerRoles::MINER; } // default role		
};

class ServerHttp: public Unit {
public:
	ServerHttp(): Unit("HttpServer") {

		settings_ = std::make_shared<SettingsHttp>();
		wallet_ = Wallet::instance(settings_);

		requestHandler_ = HttpRequestHandler::instance(settings_, wallet_, nullptr);
		requestHandler_->push(HttpGetBalance::instance());

		connectionManager_ = HttpConnectionManager::instance(settings_, requestHandler_);
		server_ = HttpServer::instance(settings_, connectionManager_);
	}

	bool execute();

	HttpRequestHandlerPtr requestHandler_;
	HttpConnectionManagerPtr connectionManager_;
	HttpServerPtr server_;

	IWalletPtr wallet_;
	ISettingsPtr settings_;
};

}
}

#endif
