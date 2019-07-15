// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TIME_H
#define QBIT_TIME_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>

#if defined(__linux__)
#include <sys/time.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <cstdint>

namespace qbit {

extern uint64_t getMicroseconds();

}

#endif