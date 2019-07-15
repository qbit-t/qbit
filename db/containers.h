// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_DB_CONTAINERS_H
#define QBIT_DB_CONTAINERS_H

#include "leveldb.h"
#include "../uint256.h"
#include "../transaction.h"
#include "../block.h"

namespace qbit {
namespace db {

template<typename key, typename value>
class DbContainer: public Container<key, value, LevelDBContainer> {
public: 
	DbContainer(const std::string& name) : Container<key, value, LevelDBContainer>(name) {}
};

template<typename key, typename value>
class DbMultiContainer: public MultiContainer<key, value, LevelDBContainer> {
public: 
	DbMultiContainer(const std::string& name) : MultiContainer<key, value, LevelDBContainer>(name) {}
};

template<typename key, typename value>
class DbEntityContainer: public EntityContainer<key, value, LevelDBContainer> {
public: 
	DbEntityContainer(const std::string& name) : EntityContainer<key, value, LevelDBContainer>(name) {}
};

} // db
} // qbit

#endif
