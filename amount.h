// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_AMOUNT_H
#define QBIT_AMOUNT_H

namespace qbit {

typedef uint64_t amount_t;
typedef uint64_t qunit_t;
typedef uint64_t qbit_t;

static const amount_t QBIT  = 100000000;
static const amount_t QUNIT = 1;
static const amount_t QBIT_MIN = QUNIT;
static const amount_t QBIT_MAX = QBIT;
static const amount_t QBIT_DECIMALS = 8;
static const char* QBIT_FORMAT = "%.8f";

} // qbit

#endif