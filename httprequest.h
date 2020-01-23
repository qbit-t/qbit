// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_HTTP_REQUEST_H
#define QBIT_HTTP_REQUEST_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include "httpheader.h"

#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace qbit {

struct HttpRequest {
	std::string method;
	std::string uri;
	int http_version_major;
	int http_version_minor;
	std::vector<HttpHeader> headers;
	std::vector<unsigned char> data;

	std::string contentType() {
		for (std::vector<HttpHeader>::iterator lHeader = headers.begin(); lHeader != headers.end(); lHeader++) {
			if (boost::algorithm::to_lower_copy((*lHeader).name) == "content-type")
				return (*lHeader).value;
		}

		return "empty";
	}

	int contentSize() {
		for (std::vector<HttpHeader>::iterator lHeader = headers.begin(); lHeader != headers.end(); lHeader++) {
			if (boost::algorithm::to_lower_copy((*lHeader).name) == "content-length")
				return boost::lexical_cast<int>((*lHeader).value);		
		}

		return 0;
	}

	std::string toString() {
		std::string lOut;
		for (std::vector<HttpHeader>::iterator lHeader = headers.begin(); lHeader != headers.end(); lHeader++) {
			lOut += (*lHeader).name + ":" + (*lHeader).value + "\n";
		}

		return lOut;
	}
};

}

#endif