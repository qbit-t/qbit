// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_RANDOM_H
#define QBIT_RANDOM_H

#include "timestamp.h"
#include "hash.h"
#include "uint256.h"

namespace qbit {

// Simple fast random generator
// mainly used as blinding factors

class Random {
public:
	Random() {}

	static uint256 generate() {
		uint64_t lS0 = getMicroseconds();
		uint64_t lS1 = getMicroseconds(); if (lS0 == lS1) lS1++;
		uint64_t lS2 = getMicroseconds(); if (lS1 == lS2) lS2++;
		uint64_t lS3 = getMicroseconds(); if (lS2 == lS3) lS3++;

		std::vector<unsigned char> lSrc; lSrc.resize(4*sizeof(uint64_t));
		unsigned char* lPtr = &lSrc[0];
		memcpy(lPtr, &lS0, sizeof(uint64_t)); lPtr += sizeof(uint64_t);
		memcpy(lPtr, &lS1, sizeof(uint64_t)); lPtr += sizeof(uint64_t);
		memcpy(lPtr, &lS2, sizeof(uint64_t)); lPtr += sizeof(uint64_t);
		memcpy(lPtr, &lS3, sizeof(uint64_t)); lPtr += sizeof(uint64_t);

		return Hash(lSrc.begin(), lSrc.end());
	}

};

}

#endif