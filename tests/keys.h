// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_KEYS_H
#define QBIT_UNITTEST_KEYS_H

#include <string>
#include <list>

#include "unittest.h"

namespace qbit {
namespace tests {

class RandomTest: public Unit {
public:
	RandomTest(): Unit("RandomTest") {}

	bool execute();
};

class CreateKeySignAndVerify: public Unit {
public:
	CreateKeySignAndVerify(): Unit("CreateKeySignAndVerify") {}

	bool execute();
};

class CreateKeySignAndVerifyMix: public Unit {
public:
	CreateKeySignAndVerifyMix(): Unit("CreateKeySignAndVerifyMix") {}

	bool execute();
};

}
}

#endif