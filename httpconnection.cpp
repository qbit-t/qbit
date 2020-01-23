#include "httpconnection.h"
#include "log/log.h"

#include <iostream>

using namespace qbit;

void HttpConnection::waitForRequest() {
	// log
	gLog().write(Log::HTTP, std::string("[connection]: waiting for request..."));

	socket_->async_read_some(boost::asio::buffer(buffer_),
		boost::bind(&HttpConnection::handleRead, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void HttpConnection::handleRead(const boost::system::error_code& error, std::size_t bytesTransferred) {
	if (!error)	{
		// log
		gLog().write(Log::HTTP, std::string("[connection]: processing request from ") + key());

		boost::tribool lResult;
		boost::tie(lResult, boost::tuples::ignore) = 
			requestParser_.parse(request_, buffer_.data(), buffer_.data() + bytesTransferred);

		if (lResult) {
			requestHandler_->handleRequest(key(), request_, reply_);
			boost::asio::async_write(*socket_, reply_.toBuffers(),
				boost::bind(&HttpConnection::handleWrite, shared_from_this(),
					boost::asio::placeholders::error));
		} else if (!lResult) {
			reply_ = HttpReply::stockReply(HttpReply::bad_request);
			boost::asio::async_write(*socket_, reply_.toBuffers(),
				boost::bind(&HttpConnection::handleWrite, shared_from_this(),
					boost::asio::placeholders::error));
		} else {
			socket_->async_read_some(boost::asio::buffer(buffer_),
				boost::bind(&HttpConnection::handleRead, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
	}
}

void HttpConnection::handleWrite(const boost::system::error_code& error) {
	//
	if (!error)	{
		// Initiate graceful connection closure.
		boost::system::error_code ignored_ec;
		socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
	}
}