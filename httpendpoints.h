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
	HttpMallocStats(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
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
	HttpGetKey(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getkey"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetKey>(false);
	}
};

class HttpNewKey: public IHttpCallEnpoint {
public:
	HttpNewKey() {}
	HttpNewKey(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("newkey"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpNewKey>(false);
	}
};

class HttpGetBalance: public IHttpCallEnpoint {
public:
	HttpGetBalance() {}
	HttpGetBalance(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getbalance"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetBalance>(false);
	}
};

class HttpSendToAddress: public IHttpCallEnpoint {
public:
	HttpSendToAddress() {}
	HttpSendToAddress(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("sendtoaddress"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpSendToAddress>(false);
	}
};

class HttpGetPeerInfo: public IHttpCallEnpoint {
public:
	HttpGetPeerInfo() {}
	HttpGetPeerInfo(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getpeerinfo"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetPeerInfo>();
	}
};

class HttpReleasePeer: public IHttpCallEnpoint {
public:
	HttpReleasePeer() {}
	HttpReleasePeer(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("releasepeer"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpReleasePeer>(false);
	}
};

class HttpBanPeer: public IHttpCallEnpoint {
public:
	HttpBanPeer() {}
	HttpBanPeer(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("banpeer"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpBanPeer>(false);
	}
};

class HttpGetState: public IHttpCallEnpoint {
public:
	HttpGetState() {}
	HttpGetState(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getstate"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetState>();
	}
};

class HttpCreateDApp: public IHttpCallEnpoint {
public:
	HttpCreateDApp() {}
	HttpCreateDApp(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("createdapp"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpCreateDApp>(false);
	}
};

class HttpCreateShard: public IHttpCallEnpoint {
public:
	HttpCreateShard() {}
	HttpCreateShard(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("createshard"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpCreateShard>(false);
	}
};

class HttpGetTransaction: public IHttpCallEnpoint {
public:
	HttpGetTransaction() {}
	HttpGetTransaction(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("gettransaction"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetTransaction>();
	}
};

class HttpGetEntity: public IHttpCallEnpoint {
public:
	HttpGetEntity() {}
	HttpGetEntity(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getentity"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetEntity>();
	}
};

class HttpGetBlock: public IHttpCallEnpoint {
public:
	HttpGetBlock() {}
	HttpGetBlock(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getblock"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetBlock>();
	}
};

class HttpGetBlockHeader: public IHttpCallEnpoint {
public:
	HttpGetBlockHeader() {}
	HttpGetBlockHeader(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getblockheader"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetBlockHeader>();
	}
};

class HttpGetUnconfirmedTransactions: public IHttpCallEnpoint {
public:
	HttpGetUnconfirmedTransactions() {}
	HttpGetUnconfirmedTransactions(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getunconfirmedtxs"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetUnconfirmedTransactions>();
	}
};

class HttpGetBlockHeaderByHeight: public IHttpCallEnpoint {
public:
	HttpGetBlockHeaderByHeight() {}
	HttpGetBlockHeaderByHeight(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getblockheaderbyheight"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetBlockHeaderByHeight>();
	}
};

class HttpCreateAsset: public IHttpCallEnpoint {
public:
	HttpCreateAsset() {}
	HttpCreateAsset(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("createasset"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpCreateAsset>(false);
	}
};

class HttpCreateAssetEmission: public IHttpCallEnpoint {
public:
	HttpCreateAssetEmission() {}
	HttpCreateAssetEmission(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("createassetemission"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpCreateAssetEmission>(false);
	}
};

class HttpGetEntitiesCount: public IHttpCallEnpoint {
public:
	HttpGetEntitiesCount() {}
	HttpGetEntitiesCount(bool publicMethod) : IHttpCallEnpoint(publicMethod) {}

	void run(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getentitiescount"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetEntitiesCount>();
	}
};

} // qbit

#endif