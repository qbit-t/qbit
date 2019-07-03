// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QUARK_CONTAINERS_H
#define QUARK_CONTAINERS_H

#include <list>
#include <vector>
#include <map>

//
// TODO: redefine allocator
//

namespace quark {

template<class type> class vector : public std::vector<type> {};

} // quark

#endif
