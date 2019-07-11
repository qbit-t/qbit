// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_VM_H
#define QBIT_UNITTEST_VM_H

#include <string>
#include <list>

#include "unittest.h"

namespace qbit {
namespace tests {

class VMMovCmp: public Unit {
public:
	VMMovCmp(): Unit("VMMovCmp") {}

	bool execute();
};

class VMMovCmpJmpFalse: public Unit {
public:
	VMMovCmpJmpFalse(): Unit("VMMovCmpJmpFalse") {}

	bool execute();
};

class VMMovCmpJmpTrue: public Unit {
public:
	VMMovCmpJmpTrue(): Unit("VMMovCmpJmpTrue") {}

	bool execute();
};

class VMLoop: public Unit {
public:
	VMLoop(): Unit("VMLoop") {}

	bool execute();
};

class VMCheckLHash256: public Unit {
public:
	VMCheckLHash256(): Unit("VMCheckLHash256") {}

	bool execute();
};

class VMCheckSig: public Unit {
public:
	VMCheckSig(): Unit("VMCheckSig") {}

	bool execute();
};

}
}

#endif
