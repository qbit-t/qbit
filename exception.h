// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_EXCEPTION_H
#define QBIT_EXCEPTION_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
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

}

#endif