// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_MAINCHAIN_H
#define QBIT_MAINCHAIN_H

#include "exception.h"
#include "uint256.h"

namespace qbit {

class MainChain {
public:
	// main chain id
	static inline uint256 id() { return uint256S("0000000000000000000000000000000000000000000000000000000000000001"); }
};

} // qbit

#endif
