// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_HTTP_ENDPOINTS_H
#define QBIT_HTTP_ENDPOINTS_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include "ihttpcallendpoint.h"
#include "iwallet.h"

#include <string>

namespace qbit {

class HttpGetKey: public IHttpCallEnpoint {
public:
	HttpGetKey() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getkey"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetKey>();
	}
};

class HttpGetBalance: public IHttpCallEnpoint {
public:
	HttpGetBalance() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getbalance"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetBalance>();
	}
};

class HttpSendToAddress: public IHttpCallEnpoint {
public:
	HttpSendToAddress() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("sendtoaddress"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpSendToAddress>();
	}
};

class HttpGetPeerInfo: public IHttpCallEnpoint {
public:
	HttpGetPeerInfo() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getpeerinfo"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetPeerInfo>();
	}
};

class HttpCreateDApp: public IHttpCallEnpoint {
public:
	HttpCreateDApp() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("createdapp"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpCreateDApp>();
	}
};

class HttpCreateShard: public IHttpCallEnpoint {
public:
	HttpCreateShard() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("createshard"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpCreateShard>();
	}
};

class HttpGetTransaction: public IHttpCallEnpoint {
public:
	HttpGetTransaction() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("gettransaction"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetTransaction>();
	}
};

} // qbit

#endif