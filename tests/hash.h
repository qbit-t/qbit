#ifndef QBIT_UNITTEST_HASH_H
#define QBIT_UNITTEST_HASH_H

#include "unittest.h"

namespace qbit {
namespace tests {

class HashTest: public Unit {
public:
	HashTest(): Unit("HashTest") {}

	bool execute();
};

}
}


#endif
