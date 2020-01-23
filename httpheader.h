// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_HTTP_HEADER_H
#define QBIT_HTTP_HEADER_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include <string>

namespace qbit {

struct HttpHeader {
	std::string name;
	std::string value;
};

}

#endif