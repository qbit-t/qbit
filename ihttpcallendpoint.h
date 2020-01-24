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

namespace qbit {

class IHttpCallEnpoint {
public:
	IHttpCallEnpoint() {}

	virtual void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&) { throw qbit::exception("NOT_IMPL", "IHttpCallEnpoint::process - not implemented."); }
	virtual std::string method() { throw qbit::exception("NOT_IMPL", "IHttpCallEnpoint::method - not implemented."); }
	
	void setWallet(IWalletPtr wallet) { wallet_ = wallet; }

protected:
	IWalletPtr wallet_;
};

typedef std::shared_ptr<IHttpCallEnpoint> IHttpCallEnpointPtr;

} // qbit

#endif