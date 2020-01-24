#include "httprequesthandler.h"
#include "httpreply.h"
#include "httprequest.h"
#include "log/log.h"
#include "json.h"
#include "tinyformat.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace qbit;

HttpRequestHandler::HttpRequestHandler(ISettingsPtr settings, IWalletPtr wallet) : settings_(settings), wallet_(wallet) {
}

void HttpRequestHandler::handleRequest(const std::string& endpoint, const HttpRequest& request, HttpReply& reply) {
	// Decode url to path
	std::string lRequestPath;
	if (!urlDecode(request.uri, lRequestPath)) {
		reply = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}

	// Request path must be absolute and not contain "..".
	if (lRequestPath.empty() || lRequestPath[0] != '/' || lRequestPath.find("..") != std::string::npos) {
		reply = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}

	// log
	gLog().write(Log::HTTP, std::string("[handleRequest]: ") + 
			strprintf("host = %s, url = %s, method = %s, headers ->\n%s", 
				endpoint, lRequestPath, request.method, const_cast<HttpRequest&>(request).toString()));

	// deserialize data
	json::Document lData;
	if (!lData.loadFromStream(request.data)) {
		// error
		gLog().write(Log::HTTP, std::string("[handleRequest/error]: ") + lData.lastError());
		reply = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}

	// log
	if (gLog().isEnabled(Log::HTTP)) {
		std::string lJson;
		lData.writeToString(lJson);
		gLog().write(Log::HTTP, std::string("[handleRequest]: ") + strprintf("host = %s, json request ->\n%s", endpoint, lData.toString()));
	}

	json::Value lMethod;
	if (lData.find("method", lMethod) && lMethod.isString()) {
		std::map<std::string /*method*/, IHttpCallEnpointPtr>::iterator lEndpoint = methods_.find(lMethod.getString());
		if (lEndpoint != methods_.end()) {
			lEndpoint->second->process(endpoint, request, lData, reply);
		} else {
			reply = HttpReply::stockReply(HttpReply::not_found);
			return;
		}
	} else {
		reply = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}

	// log
	if (gLog().isEnabled(Log::HTTP)) {
		gLog().write(Log::HTTP, std::string("[handleRequest]: ") + strprintf("host = %s, json reply ->\n%s", endpoint, reply.content));
	}
}

bool HttpRequestHandler::urlDecode(const std::string& in, std::string& out) {
	out.clear();
	out.reserve(in.size());
	for (std::size_t i = 0; i < in.size(); ++i) {
		if (in[i] == '%') {
			if (i + 3 <= in.size()) {
				int value = 0;
				std::istringstream is(in.substr(i + 1, 2));
				if (is >> std::hex >> value) {
					out += static_cast<char>(value);
					i += 2;
				} else {
					return false;
				}
			} else {
				return false;
			}
		} else if (in[i] == '+') {
			out += ' ';
		} else {
			out += in[i];
		}
	}
	return true;
}
