#include "httprequesthandler.h"
#include "httpreply.h"
#include "httprequest.h"
#include "log/log.h"
#include "json.h"
#include "tinyformat.h"
#include "httpendpoints.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace qbit;

void HttpGetBalance::process(const std::string& source, const HttpRequest& request, const json::Document& data, HttpReply& reply) {
	/* request
	{
		"jsonrpc":	"1.0",
		"id":		"curltext",
		"method":	"getbalance",
		"params": [
			"34212ab32920cd029882010232b12098c09809123091" -- (string, optional) asset
		]
	}
	*/
	/* reply
	{
		"result":	"1.0",		-- (string) corresponding asset balance
		"error":	null,		-- (string or null) error description
		"id":		"curltext"	-- (string) request id
	}
	*/

	// id
	json::Value lId;
	if (!(const_cast<json::Document&>(data).find("id", lId) && lId.isString())) {
		reply = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}

	// params
	json::Value lParams;
	if (const_cast<json::Document&>(data).find("params", lParams) && lParams.isArray()) {
		// extract parameters
		uint256 lAsset; // 0
		if (lParams.size()) {
			// param[0]
			json::Value lP0 = lParams[0];
			if (lP0.isString()) lAsset.setHex(lP0.getString());
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }
		}

		// process
		double lBalance = 0.0;
		if (!lAsset.isNull()) lBalance = ((double)wallet_->balance(lAsset)) / QBIT;
		else lBalance = ((double)wallet_->balance()) / QBIT;

		// prepare reply
		json::Document lReply;
		lReply.loadFromString("{}");
		lReply.addString("result", strprintf(QBIT_FORMAT, lBalance));
		lReply.addObject("error").toNull();
		lReply.addString("id", lId.getString());

		// pack
		lReply.writeToString(reply.content); // TODO: writeToStringPlain for minimizing output

		// finalize
		reply.status = HttpReply::ok;
		reply.headers.resize(2);
		reply.headers[0].name = "Content-Length";
		reply.headers[0].value = boost::lexical_cast<std::string>(reply.content.size());
		reply.headers[1].name = "Content-Type";
		reply.headers[1].value = "application/json";
	} else {
		reply = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}
}
