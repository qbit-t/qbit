#include "httprequesthandler.h"
#include "httpreply.h"
#include "httprequest.h"
#include "log/log.h"
#include "json.h"
#include "tinyformat.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace qbit;

HttpRequestHandler::HttpRequestHandler(ISettingsPtr settings) : settings_(settings) {
}

void HttpRequestHandler::handleRequest(const std::string& endpoint, const HttpRequest& req, HttpReply& rep) {
	// Decode url to path
	std::string request_path;
	if (!urlDecode(req.uri, request_path)) {
		rep = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}

	// Request path must be absolute and not contain "..".
	if (request_path.empty() || request_path[0] != '/' || request_path.find("..") != std::string::npos) {
		rep = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}

	// extract data
	// req.method = "POST" | "GET"

	// log
	gLog().write(Log::HTTP, std::string("[handleRequest]: ") + 
			strprintf("host = %s, url = %s, method = %s, headers ->\n%s", 
				endpoint, request_path, req.method, const_cast<HttpRequest&>(req).toString()));

	// deserialize data
	json::Document lData;
	if (!lData.loadFromStream(req.data)) {
		// error
		gLog().write(Log::HTTP, std::string("[handleRequest/error]: ") + lData.lastError());
		rep = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}

	// log
	if (gLog().isEnabled(Log::HTTP)) {
		std::string lJson;
		lData.writeToString(lJson);
		gLog().write(Log::HTTP, std::string("[handleRequest]: ") + strprintf("host = %s, json ->\n%s", endpoint, lData.toString()));
	}

	// select by method from methods_ map...
	// 

	// Fill out the reply to be sent to the client.
	rep.status = HttpReply::ok;
	//char buf[512];
	//while (is.read(buf, sizeof(buf)).gcount() > 0)
	//rep.content.append(buf, is.gcount());
	
	rep.headers.resize(2);
	rep.headers[0].name = "Content-Length";
	rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
	rep.headers[1].name = "Content-Type";
	rep.headers[1].value = "application/json";
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
