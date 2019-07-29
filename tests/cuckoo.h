#ifndef QBIT_UNITTEST_CUCKOO_H
#define QBIT_UNITTEST_CUCKOO_H

#include "../hash/cuckoo.h"

#include "unittest.h"

namespace qbit {
namespace tests {

	class CuckooHash: public Unit {
	public:
		CuckooHash(): Unit("CuckooHash") {}

		bool execute();
	};

}
}

#endif