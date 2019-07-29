// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_AMOUNT_H
#define QBIT_AMOUNT_H

namespace qbit {

typedef uint64_t amount_t;

static const amount_t QBIT = 100000000;
static const amount_t QBIT_MIN = 1 * QBIT;
static const amount_t QBIT_MAX = 10 * QBIT;

} // qbit

#endif