// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_HTTP_CONNECTION_H
#define QBIT_HTTP_CONNECTION_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include "log/log.h"
#include "httpreply.h"
#include "httprequest.h"
#include "httprequesthandler.h"
#include "httprequestparser.h"

#include <boost/algorithm/string.hpp>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

using boost::asio::ip::tcp;

namespace qbit {

typedef std::shared_ptr<tcp::socket> SocketPtr;

class HttpConnection: public std::enable_shared_from_this<HttpConnection> {
public:
	HttpConnection() {}

	HttpConnection(boost::asio::io_context& context, HttpRequestHandlerPtr requestHandler) {
		requestHandler_ = requestHandler;
		socket_ = std::make_shared<tcp::socket>(context);
	}

	SocketPtr socket() { return socket_; }

	void waitForRequest();

	inline std::string key() {
		std::string lKey = key(socket_); 
		if (lKey.size()) return lKey;
		return endpoint_;
	}

private:
	inline std::string key(SocketPtr socket) {
		if (socket != nullptr) {
			boost::system::error_code lLocalEndpoint; socket->local_endpoint(lLocalEndpoint);
			boost::system::error_code lRemoteEndpoint; socket->remote_endpoint(lRemoteEndpoint);
			if (!lLocalEndpoint && !lRemoteEndpoint)
				return (endpoint_ = socket->remote_endpoint().address().to_string() + ":" + std::to_string(socket->remote_endpoint().port()));
		}

		return "";
	}	

	void handleRead(const boost::system::error_code& error, std::size_t bytesTransferred);
	void handleWrite(const boost::system::error_code& error);

private:
	SocketPtr socket_;
	std::string endpoint_;

	/// The handler used to process the incoming request.
	HttpRequestHandlerPtr requestHandler_;

	/// Buffer for incoming data.
	boost::array<char, 8192> buffer_;

	/// The incoming request.
	HttpRequest request_;

	/// The parser for the incoming request.
	HttpRequestParser requestParser_;

	/// The reply to be sent back to the client.
	HttpReply reply_;
};

typedef std::shared_ptr<HttpConnection> HttpConnectionPtr;

}

#endif