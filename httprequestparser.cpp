#include "httprequestparser.h"
#include "httprequest.h"
#include "log/log.h"

#include <iostream>

using namespace qbit;

HttpRequestParser::HttpRequestParser(): state_(method_start) {
}

void HttpRequestParser::reset() {
	state_ = method_start;
}

boost::tribool HttpRequestParser::consume(HttpRequest& req, char input) {
	//
	switch (state_) {
		case method_start:
			if (!isChar(input) || isCtl(input) || isTSpecial(input)) {
				return false;
			} else {
				state_ = method;
				req.method.push_back(input);
				return boost::indeterminate;
			}
		case method:
			if (input == ' ') {
				state_ = uri;
				return boost::indeterminate;
			} else if (!isChar(input) || isCtl(input) || isTSpecial(input)) {
				return false;
			} else {
				req.method.push_back(input);
				return boost::indeterminate;
			}
		case uri:
			if (input == ' ') {
				state_ = http_version_h;
				return boost::indeterminate;
			} else if (isCtl(input)) {
				return false;
			} else {
				req.uri.push_back(input);
				return boost::indeterminate;
			}
		case http_version_h:
			if (input == 'H') {
				state_ = http_version_t_1;
				return boost::indeterminate;
			} else {
				return false;
			}
		case http_version_t_1:
			if (input == 'T') {
				state_ = http_version_t_2;
				return boost::indeterminate;
			} else {
				return false;
			}
		case http_version_t_2:
			if (input == 'T') {
				state_ = http_version_p;
				return boost::indeterminate;
			} else {
				return false;
			}
		case http_version_p:
			if (input == 'P') {
				state_ = http_version_slash;
				return boost::indeterminate;
			} else {
				return false;
			}
		case http_version_slash:
			if (input == '/') {
				req.http_version_major = 0;
				req.http_version_minor = 0;
				state_ = http_version_major_start;
				return boost::indeterminate;
			} else {
				return false;
			}
		case http_version_major_start:
			if (isDigit(input)) {
				req.http_version_major = req.http_version_major * 10 + input - '0';
				state_ = http_version_major;
				return boost::indeterminate;
			} else {
				return false;
			}
		case http_version_major:
			if (input == '.') {
				state_ = http_version_minor_start;
				return boost::indeterminate;
			} else if (isDigit(input)) {
				req.http_version_major = req.http_version_major * 10 + input - '0';
				return boost::indeterminate;
			} else {
				return false;
			}
		case http_version_minor_start:
			if (isDigit(input)) {
				req.http_version_minor = req.http_version_minor * 10 + input - '0';
				state_ = http_version_minor;
				return boost::indeterminate;
			} else {
				return false;
			}
		case http_version_minor:
			if (input == '\r') {
				state_ = expecting_newline_1;
				return boost::indeterminate;
			} else if (isDigit(input))	{
				req.http_version_minor = req.http_version_minor * 10 + input - '0';
				return boost::indeterminate;
			} else {
				return false;
			}
		case expecting_newline_1:
			if (input == '\n') {
				state_ = header_line_start;
				return boost::indeterminate;
			} else {
				return false;
			}
		case header_line_start:
			if (input == '\r') {
				state_ = expecting_newline_3;
				return boost::indeterminate;
			} else if (!req.headers.empty() && (input == ' ' || input == '\t')) {
				state_ = header_lws;
				return boost::indeterminate;
			} else if (!isChar(input) || isCtl(input) || isTSpecial(input)) {
				return false;
			} else {
				req.headers.push_back(HttpHeader());
				req.headers.back().name.push_back(input);
				state_ = header_name;
				return boost::indeterminate;
			}
		case header_lws:
			if (input == '\r') {
				state_ = expecting_newline_2;
				return boost::indeterminate;
			} else if (input == ' ' || input == '\t') {
				return boost::indeterminate;
			} else if (isCtl(input)) {
				return false;
			} else {
				state_ = header_value;
				req.headers.back().value.push_back(input);
				return boost::indeterminate;
			}
		case header_name:
			if (input == ':') {
				state_ = space_before_header_value;
				return boost::indeterminate;
			} else if (!isChar(input) || isCtl(input) || isTSpecial(input)) {
				return false;
			} else {
				req.headers.back().name.push_back(input);
				return boost::indeterminate;
			}
		case space_before_header_value:
			if (input == ' ') {
				state_ = header_value;
				return boost::indeterminate;
			} else {
				return false;
			}
		case header_value:
			if (input == '\r') {
				state_ = expecting_newline_2;
				return boost::indeterminate;
			} else if (isCtl(input)) {
				return false;
			} else {
				req.headers.back().value.push_back(input);
				return boost::indeterminate;
			}
		case expecting_newline_2:
			if (input == '\n') {
				state_ = header_line_start;
				return boost::indeterminate;
			} else {
				return false;
			}
		case expecting_newline_3: {
			std::string lContentType = req.contentType();
			if (lContentType == "text/plain" || lContentType == "application/json") {
				int lSize = req.contentSize();
				if (lSize < 16*1024/*TODO: settings*/) {
					req.data.resize(lSize);
					state_ = binary_data;
					return boost::indeterminate;
				}
			}
			return (input == '\n');
		}
		case binary_data:
			if (dataSize_ < req.data.size()) {
				req.data[dataSize_++] = (unsigned char)input; 
				state_ = binary_data;
				if (dataSize_ == req.data.size()) return true;
				return boost::indeterminate;
			} else {
				return true;
			} 
		default: return false;
	}
}

bool HttpRequestParser::isChar(int c) {
	return c >= 0 && c <= 127;
}

bool HttpRequestParser::isCtl(int c) {
	return (c >= 0 && c <= 31) || (c == 127);
}

bool HttpRequestParser::isTSpecial(int c) {
	switch (c) {
		case '(': case ')': case '<': case '>': case '@':
		case ',': case ';': case ':': case '\\': case '"':
		case '/': case '[': case ']': case '?': case '=':
		case '{': case '}': case ' ': case '\t':
			return true;
		default:
			return false;
	}
}

bool HttpRequestParser::isDigit(int c) {
	return c >= '0' && c <= '9';
}

