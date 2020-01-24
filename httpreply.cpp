#include "httpreply.h"
#include "log/log.h"

#include <iostream>
#include <boost/lexical_cast.hpp>

using namespace qbit;

namespace status_strings {

const std::string ok = "HTTP/1.0 200 OK\r\n";
const std::string created = "HTTP/1.0 201 Created\r\n";
const std::string accepted = "HTTP/1.0 202 Accepted\r\n";
const std::string no_content = "HTTP/1.0 204 No Content\r\n";
const std::string multiple_choices = "HTTP/1.0 300 Multiple Choices\r\n";
const std::string moved_permanently = "HTTP/1.0 301 Moved Permanently\r\n";
const std::string moved_temporarily = "HTTP/1.0 302 Moved Temporarily\r\n";
const std::string not_modified = "HTTP/1.0 304 Not Modified\r\n";
const std::string bad_request = "HTTP/1.0 400 Bad Request\r\n";
const std::string unauthorized = "HTTP/1.0 401 Unauthorized\r\n";
const std::string forbidden = "HTTP/1.0 403 Forbidden\r\n";
const std::string not_found = "HTTP/1.0 404 Not Found\r\n";
const std::string internal_server_error = "HTTP/1.0 500 Internal Server Error\r\n";
const std::string not_implemented = "HTTP/1.0 501 Not Implemented\r\n";
const std::string bad_gateway = "HTTP/1.0 502 Bad Gateway\r\n";
const std::string service_unavailable = "HTTP/1.0 503 Service Unavailable\r\n";

boost::asio::const_buffer toBuffer(HttpReply::StatusType status) {
	switch (status) {
		case HttpReply::ok:	return boost::asio::buffer(ok);
		case HttpReply::created: return boost::asio::buffer(created);
		case HttpReply::accepted: return boost::asio::buffer(accepted);
		case HttpReply::no_content:	return boost::asio::buffer(no_content);
		case HttpReply::multiple_choices: return boost::asio::buffer(multiple_choices);
		case HttpReply::moved_permanently: return boost::asio::buffer(moved_permanently);
		case HttpReply::moved_temporarily: return boost::asio::buffer(moved_temporarily);
		case HttpReply::not_modified: return boost::asio::buffer(not_modified);
		case HttpReply::bad_request: return boost::asio::buffer(bad_request);
		case HttpReply::unauthorized: return boost::asio::buffer(unauthorized);
		case HttpReply::forbidden: return boost::asio::buffer(forbidden);
		case HttpReply::not_found: return boost::asio::buffer(not_found);
		case HttpReply::internal_server_error: return boost::asio::buffer(internal_server_error);
		case HttpReply::not_implemented: return boost::asio::buffer(not_implemented);
		case HttpReply::bad_gateway: return boost::asio::buffer(bad_gateway);
		case HttpReply::service_unavailable: return boost::asio::buffer(service_unavailable);
		default: return boost::asio::buffer(internal_server_error);
	}
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

std::vector<boost::asio::const_buffer> HttpReply::toBuffers() {
	//
	std::vector<boost::asio::const_buffer> buffers;
	buffers.push_back(status_strings::toBuffer(status));
	
	for (std::size_t i = 0; i < headers.size(); ++i) {
		HttpHeader& h = headers[i];
		buffers.push_back(boost::asio::buffer(h.name));
		buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	}

	buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	buffers.push_back(boost::asio::buffer(content));
	return buffers;
}

namespace stock_replies {

const char ok[] = "";
const char created[] =
	"{"
		"\"result\": \"201 Created\""
	"}";
const char accepted[] =
	"{"
		"\"result\": \"202 Accepted\""
	"}";
const char no_content[] =
	"{"
		"\"result\": \"204 No Content\""
	"}";
const char multiple_choices[] =
	"{"
		"\"result\": \"300 Multiple Choices\""
	"}";
const char moved_permanently[] =
	"{"
		"\"result\": \"301 Moved Permanently\""
	"}";
const char moved_temporarily[] =
	"{"
		"\"result\": \"302 Moved Temporarily\""
	"}";
const char not_modified[] =
	"{"
		"\"result\": \"304 Not Modified\""
	"}";
const char bad_request[] =
	"{"
		"\"result\": null, "
		"\"error\": {\"code\": 400, \"message\": \"Bad Request\"}"
	"}";
const char unauthorized[] =
	"{"
		"\"result\": null, "
		"\"error\": {\"code\": 401, \"message\": \"Unauthorized\"}"
	"}";
const char forbidden[] =
	"{"
		"\"result\": null, "
		"\"error\": {\"code\": 403, \"message\": \"Forbidden\"}"
	"}";
const char not_found[] =
	"{"
		"\"result\": null, "
		"\"error\": {\"code\": 404, \"message\": \"Not Found\"}"
	"}";
const char internal_server_error[] =
	"{"
		"\"result\": null, "
		"\"error\": {\"code\": 405, \"message\": \"Internal Server Error\"}"
	"}";
const char not_implemented[] =
	"{"
		"\"result\": null, "
		"\"error\": {\"code\": 501, \"message\": \"Not Implemented\"}"
	"}";
const char bad_gateway[] =
	"{"
		"\"result\": null, "
		"\"error\": {\"code\": 502, \"message\": \"Bad Gateway\"}"
	"}";
const char service_unavailable[] =
	"{"
		"\"result\": null, "
		"\"error\": {\"code\": 503, \"message\": \"Service Unavailable\"}"
	"}";

std::string toString(HttpReply::StatusType status) {
 	switch (status) {
		case HttpReply::ok: return ok;
		case HttpReply::created: return created;
		case HttpReply::accepted: return accepted;
		case HttpReply::no_content: return no_content;
		case HttpReply::multiple_choices: return multiple_choices;
		case HttpReply::moved_permanently: return moved_permanently;
		case HttpReply::moved_temporarily: return moved_temporarily;
		case HttpReply::not_modified: return not_modified;
		case HttpReply::bad_request: return bad_request;
		case HttpReply::unauthorized: return unauthorized;
		case HttpReply::forbidden: return forbidden;
		case HttpReply::not_found: return not_found;
		case HttpReply::internal_server_error: return internal_server_error;
		case HttpReply::not_implemented: return not_implemented;
		case HttpReply::bad_gateway: return bad_gateway;
		case HttpReply::service_unavailable: return service_unavailable;
		default: return internal_server_error;
	}
}

} // namespace stock_replies

HttpReply HttpReply::stockReply(HttpReply::StatusType status) {
	HttpReply lReply;
	lReply.status = status;
	lReply.content = stock_replies::toString(status);
	lReply.headers.resize(2);
	lReply.headers[0].name = "Content-Length";
	lReply.headers[0].value = boost::lexical_cast<std::string>(lReply.content.size());
	lReply.headers[1].name = "Content-Type";
	lReply.headers[1].value = "application/json";
	return lReply;
}
