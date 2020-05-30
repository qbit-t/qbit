#ifndef QBIT_UNITTEST_POW_H
#define QBIT_UNITTEST_POW_H

#include "unittest.h"
#include "../pow.h"

namespace qbit {
namespace tests {

class PowTest: public Unit {
public:
	PowTest() : Unit("PowTest") {}
	bool execute();
};

class PowTestOne: public Unit {
public:
	PowTestOne() : Unit("PowTestOne") {}
	bool execute();
};

class PowTestTwo: public Unit {
public:
	PowTestTwo() : Unit("PowTestTwo") {}
	bool execute();
};

}
}

#endif
