// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_HTTP_REQUESTHANDLER_H
#define QBIT_HTTP_REQUESTHANDLER_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include "isettings.h"
#include "ihttpcallendpoint.h"

#include <string>
#include <boost/noncopyable.hpp>

namespace qbit {

struct HttpReply;
struct HttpRequest;

class HttpRequestHandler;
typedef std::shared_ptr<HttpRequestHandler> HttpRequestHandlerPtr;

class HttpRequestHandler: private boost::noncopyable {
public:
	HttpRequestHandler(ISettingsPtr);

	void handleRequest(const std::string&, const HttpRequest&, HttpReply&);

	static HttpRequestHandlerPtr instance(ISettingsPtr settings) { 
		return std::make_shared<HttpRequestHandler>(settings); 
	}

	void push(IHttpCallEnpointPtr endpoint) {
		methods_.insert(std::map<std::string /*method*/, IHttpCallEnpointPtr>::value_type(endpoint->method(), endpoint));
	}

private:
	static bool urlDecode(const std::string&, std::string&);

private:
	// settings
	ISettingsPtr settings_;

	// Need to be filled BEFORE all processing started
	std::map<std::string /*method*/, IHttpCallEnpointPtr> methods_;
};

typedef std::shared_ptr<HttpRequestHandler> HttpRequestHandlerPtr;

}

#endif