// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_H
#define QBIT_UNITTEST_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../allocator.h"

#include <iostream>
#include "../key.h"
#include "../transaction.h"
#include "../vm/vm.h"
#include "libsecp256k1-config.h"
#include "scalar_impl.h"
#include "../utilstrencodings.h"
#include "../jm/include/jm_utils.h"

#include <string>
#include <list>

namespace qbit {
namespace tests {

enum UnitState {
	SUCCESS = 0x01,
	ERROR 	= 0x00,
	READY	= 0x02
};

// forward
class TestSuit;

class Unit {
friend class TestSuit;
public:
	Unit(std::string name) : name_(name) { state_ = UnitState::READY; timeLimit_ = 0; }

	virtual bool execute() = 0;
	std::string name() { return name_; }
	UnitState state() { return state_; }
	uint32_t time() { return time_; }

protected:
	std::string name_;
	std::string error_;
	UnitState state_;
	int64_t time_;
	int64_t timeLimit_;
};

class TestSuit {
public:
	TestSuit() {}

	void operator << (Unit* unit) {
		tests_.push_back(unit);
	}

	void execute();

private:
	std::list<Unit*> tests_;
};

} // tests
} // qbit

#endif

