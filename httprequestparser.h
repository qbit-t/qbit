// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_HTTP_REQUESTPARSER_H
#define QBIT_HTTP_REQUESTPARSER_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"

#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>

#include <iostream>

namespace qbit {

struct HttpRequest;

class HttpRequestParser {
public:
	HttpRequestParser();

	void reset();

	template <typename InputIterator>
	boost::tuple<boost::tribool, InputIterator> parse(HttpRequest& req,	InputIterator begin, InputIterator end) {
		while (begin != end) {
			boost::tribool lResult = consume(req, *begin++);
			if (lResult || !lResult)
				return boost::make_tuple(lResult, begin);
		}

		boost::tribool lResult = boost::indeterminate;
		return boost::make_tuple(lResult, begin);
	}

private:
	boost::tribool consume(HttpRequest& req, char input);
	static bool isChar(int c);
	static bool isCtl(int c);
	static bool isTSpecial(int c);
	static bool isDigit(int c);

	enum state {
		method_start,
		method,
		uri,
		http_version_h,
		http_version_t_1,
		http_version_t_2,
		http_version_p,
		http_version_slash,
		http_version_major_start,
		http_version_major,
		http_version_minor_start,
		http_version_minor,
		expecting_newline_1,
		header_line_start,
		header_lws,
		header_name,
		space_before_header_value,
		header_value,
		expecting_newline_2,
		expecting_newline_3,
		binary_data
	} state_;

	int dataSize_ = 0;
};

}

#endif