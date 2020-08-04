// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_EXCEPTION_H
#define QBIT_EXCEPTION_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include <string>
#include <stdexcept>

namespace qbit {

class exception : public std::runtime_error
{
public:
	explicit exception(const std::string& code, const std::string& msg) : std::runtime_error(msg) { code_ = code; }
	inline std::string code() { return code_; }

private:
	std::string code_;
};


#define NULL_REFERENCE_EXCEPTION()\
{ \
	char lMsg[512] = {0}; \
	snprintf(lMsg, sizeof(lMsg)-1, "Null reference exception at %s(), %s: %d", __FUNCTION__, __FILE__, __LINE__); \
	throw exception("ENULL", std::string(lMsg)); \
} \

}

#endif
