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

class HttpGetBalance: public IHttpCallEnpoint {
public:
	HttpGetBalance() {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("getbalance"); }

	static IHttpCallEnpointPtr instance() {
		return std::make_shared<HttpGetBalance>();
	}
};

} // qbit

#endif