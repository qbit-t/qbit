// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IHTTP_CALL_ENDPOINT_H
#define QBIT_IHTTP_CALL_ENDPOINT_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include "httpreply.h"
#include "httprequest.h"
#include "iwallet.h"
#include "json.h"
#include "ipeermanager.h"

namespace qbit {

class IHttpCallEnpoint {
public:
	IHttpCallEnpoint() {}

	virtual void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&) { throw qbit::exception("NOT_IMPL", "IHttpCallEnpoint::process - not implemented."); }
	virtual std::string method() { throw qbit::exception("NOT_IMPL", "IHttpCallEnpoint::method - not implemented."); }
	
	void setWallet(IWalletPtr wallet) { wallet_ = wallet; }
	void setPeerManager(IPeerManagerPtr peerManager) { peerManager_ = peerManager; }

	void pack(HttpReply& reply, json::Document& data) {
		// pack
		data.writeToString(reply.content); // TODO: writeToStringPlain for minimizing output
		reply.content += "\n"; // extra line
	}

	void finalize(HttpReply& reply) {
		reply.status = HttpReply::ok;
		reply.headers.resize(2);
		reply.headers[0].name = "Content-Length";
		reply.headers[0].value = boost::lexical_cast<std::string>(reply.content.size());
		reply.headers[1].name = "Content-Type";
		reply.headers[1].value = "application/json";		
	}

protected:
	IWalletPtr wallet_;
	IPeerManagerPtr peerManager_;
};

typedef std::shared_ptr<IHttpCallEnpoint> IHttpCallEnpointPtr;

} // qbit

#endif