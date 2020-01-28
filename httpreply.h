// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_HTTP_REPLY_H
#define QBIT_HTTP_REPLY_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include "httpheader.h"

#include <string>
#include <vector>
#include <boost/asio.hpp>

namespace qbit {

struct HttpReply {
	enum StatusType
	{
		ok = 200,
		created = 201,
		accepted = 202,
		no_content = 204,
		multiple_choices = 300,
		moved_permanently = 301,
		moved_temporarily = 302,
		not_modified = 304,
		bad_request = 400,
		unauthorized = 401,
		forbidden = 403,
		not_found = 404,
		internal_server_error = 500,
		not_implemented = 501,
		bad_gateway = 502,
		service_unavailable = 503
	} status;

	std::vector<HttpHeader> headers;
	std::string content;
	std::vector<boost::asio::const_buffer> toBuffers();

	static HttpReply stockReply(StatusType status);
	static HttpReply stockReply(const std::string&, const std::string&);
};

}

#endif