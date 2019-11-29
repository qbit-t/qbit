// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TIME_H
#define QBIT_TIME_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <string>

#if defined(__linux__)
#include <sys/time.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <cstdint>

namespace qbit {

extern uint64_t getMicroseconds();
extern uint64_t getTime();

/**
 * ISO 8601 formatting is preferred. Use the FormatISO8601{DateTime,Date,Time}
 * helper functions if possible.
 */
extern std::string formatISO8601DateTime(int64_t);
extern std::string formatISO8601Date(int64_t);

}

#endif
