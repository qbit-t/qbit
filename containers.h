// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CONTAINERS_H
#define QBIT_CONTAINERS_H

#include <list>
#include <vector>
#include <map>

//
// TODO: redefine allocator
//

namespace qbit {

template<class type> class vector : public std::vector<type> {};

} // qbit

#endif
