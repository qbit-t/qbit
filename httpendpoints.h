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

//
// system
//
class HttpMallocStats: public IHttpCallEnpoint {
public:
	HttpMallocStats() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("mallocstats"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpMallocStats>();
	}
};

//
// common
//
class HttpGetKey: public IHttpCallEnpoint {
public:
	HttpGetKey() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getkey"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetKey>();
	}
};

class HttpNewKey: public IHttpCallEnpoint {
public:
	HttpNewKey() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("newkey"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpNewKey>();
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

class HttpGetState: public IHttpCallEnpoint {
public:
	HttpGetState() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getstate"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetState>();
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

class HttpGetBlock: public IHttpCallEnpoint {
public:
	HttpGetBlock() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getblock"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetBlock>();
	}
};

class HttpGetBlockHeader: public IHttpCallEnpoint {
public:
	HttpGetBlockHeader() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getblockheader"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetBlockHeader>();
	}
};

class HttpGetBlockHeaderByHeight: public IHttpCallEnpoint {
public:
	HttpGetBlockHeaderByHeight() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getblockheaderbyheight"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetBlockHeaderByHeight>();
	}
};

class HttpCreateAsset: public IHttpCallEnpoint {
public:
	HttpCreateAsset() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("createasset"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpCreateAsset>();
	}
};

class HttpCreateAssetEmission: public IHttpCallEnpoint {
public:
	HttpCreateAssetEmission() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("createassetemission"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpCreateAssetEmission>();
	}
};

} // qbit

#endif