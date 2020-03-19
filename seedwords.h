// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_SEED_WORDS_H
#define QBIT_SEED_WORDS_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"

#include "uint256.h"
#include <list>
#include <set>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace qbit {

class SeedPhrase {
public:
	SeedPhrase() {}

	std::list<std::string> generate();
};

}

#endif