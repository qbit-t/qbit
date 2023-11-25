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

class DbContainerSpace: public LevelDbContainerSpace {
public: 
	DbContainerSpace(const std::string& name) : LevelDbContainerSpace() { name_ = name; }
	bool open() {
		return LevelDbContainerSpace::open(name_, true, 0 /*cache*/);
	}
	bool repair() {
		return LevelDbContainerSpace::repair(name_, true, 0 /*cache*/);
	}
private:
	std::string name_;
};
typedef std::shared_ptr<DbContainerSpace> DbContainerSpacePtr;

template<typename key, typename value>
class DbContainerShared: public Container<key, value, LevelDbContainerProxy> {
public: 
	DbContainerShared(const std::string& name, DbContainerSpacePtr space) : Container<key, value, LevelDbContainerProxy>(name) { space_ = space; }
	void attach() {
		//
		this->attachTo(space_);
	}

private:
	DbContainerSpacePtr space_;
};

template<typename key, typename value>
class DbMultiContainerShared: public MultiContainer<key, value, LevelDbContainerProxy> {
public: 
	DbMultiContainerShared(const std::string& name, DbContainerSpacePtr space) : MultiContainer<key, value, LevelDbContainerProxy>(name) { space_ = space; }
	void attach() {
		//
		this->attachTo(space_);
	}

private:
	DbContainerSpacePtr space_;
};

template<typename key, typename value>
class DbEntityContainerShared: public EntityContainer<key, value, LevelDbContainerProxy> {
public: 
	DbEntityContainerShared(const std::string& name, DbContainerSpacePtr space) : EntityContainer<key, value, LevelDbContainerProxy>(name) { space_ = space; }
	void attach() {
		//
		this->attachTo(space_);
	}

private:
	DbContainerSpacePtr space_;
};

template<typename key1, typename key2, typename value>
class DbTwoKeyContainerShared: public TwoKeyContainer<key1, key2, value, LevelDbContainerProxy> {
public: 
	DbTwoKeyContainerShared(const std::string& name, DbContainerSpacePtr space) : TwoKeyContainer<key1, key2, value, LevelDbContainerProxy>(name) { space_ = space; }
	void attach() {
		//
		this->attachTo(space_);
	}

private:
	DbContainerSpacePtr space_;
};

template<typename key1, typename key2, typename key3, typename value>
class DbThreeKeyContainerShared: public ThreeKeyContainer<key1, key2, key3, value, LevelDbContainerProxy> {
public: 
	DbThreeKeyContainerShared(const std::string& name, DbContainerSpacePtr space) : ThreeKeyContainer<key1, key2, key3, value, LevelDbContainerProxy>(name) { space_ = space; }
	void attach() {
		//
		this->attachTo(space_);
	}

private:
	DbContainerSpacePtr space_;
};

template<typename key1, typename key2, typename key3, typename key4, typename value>
class DbFourKeyContainerShared: public FourKeyContainer<key1, key2, key3, key4, value, LevelDbContainerProxy> {
public: 
	DbFourKeyContainerShared(const std::string& name, DbContainerSpacePtr space) : FourKeyContainer<key1, key2, key3, key4, value, LevelDbContainerProxy>(name) { space_ = space; }
	void attach() {
		//
		this->attachTo(space_);
	}

private:
	DbContainerSpacePtr space_;
};

template<typename key1, typename key2, typename key3, typename key4, typename key5, typename value>
class DbFiveKeyContainerShared: public FiveKeyContainer<key1, key2, key3, key4, key5, value, LevelDbContainerProxy> {
public: 
	DbFiveKeyContainerShared(const std::string& name, DbContainerSpacePtr space) : FiveKeyContainer<key1, key2, key3, key4, key5, value, LevelDbContainerProxy>(name) { space_ = space; }
	void attach() {
		//
		this->attachTo(space_);
	}

private:
	DbContainerSpacePtr space_;
};

template<typename value>
class DbList: public Container<uint64_t, value, LevelDbContainerProxy> {
public: 
	DbList(const std::string& name, DbContainerSpacePtr space) : Container<uint64_t, value, LevelDbContainerProxy>(name) { space_ = space; }
	void attach() {
		//
		this->attachTo(space_);
	}

	inline bool write(const value& v) {
		//
		return Container<uint64_t, value, LevelDbContainerProxy>::write(++idx_, v, sync);
	}

private:
	DbContainerSpacePtr space_;
	uint64_t idx_ = 0;
};

template<typename key>
class DbSet: public Container<key, char, LevelDbContainerProxy> {
public: 
	DbSet(const std::string& name, DbContainerSpacePtr space) : Container<key, char, LevelDbContainerProxy>(name) { space_ = space; }
	void attach() {
		//
		this->attachTo(space_);
	}

	inline bool write(const key& k) {
		//
		return Container<key, char, LevelDbContainerProxy>::write(k, (char)0, sync);
	}

	inline bool read(const key& k) {
		//
		char lV;
		return Container<key, char, LevelDbContainerProxy>::read(k, lV);
	}

private:
	DbContainerSpacePtr space_;
};

//

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

template<typename key1, typename key2, typename value>
class DbTwoKeyContainer: public TwoKeyContainer<key1, key2, value, LevelDBContainer> {
public: 
	DbTwoKeyContainer(const std::string& name) : TwoKeyContainer<key1, key2, value, LevelDBContainer>(name) {}
};

template<typename key1, typename key2, typename key3, typename value>
class DbThreeKeyContainer: public ThreeKeyContainer<key1, key2, key3, value, LevelDBContainer> {
public: 
	DbThreeKeyContainer(const std::string& name) : ThreeKeyContainer<key1, key2, key3, value, LevelDBContainer>(name) {}
};

template<typename key1, typename key2, typename key3, typename key4, typename value>
class DbFourKeyContainer: public FourKeyContainer<key1, key2, key3, key4, value, LevelDBContainer> {
public: 
	DbFourKeyContainer(const std::string& name) : FourKeyContainer<key1, key2, key3, key4, value, LevelDBContainer>(name) {}
};

template<typename key1, typename key2, typename key3, typename key4, typename key5, typename value>
class DbFiveKeyContainer: public FiveKeyContainer<key1, key2, key3, key4, key5, value, LevelDBContainer> {
public: 
	DbFiveKeyContainer(const std::string& name) : FiveKeyContainer<key1, key2, key3, key4, key5, value, LevelDBContainer>(name) {}
};

template<typename value>
class DbListContainer: public Container<uint64_t, value, LevelDBContainer> {
public: 
	DbListContainer(const std::string& name) : Container<uint64_t, value, LevelDBContainer>(name) {}
	inline bool write(const value& v) {
		//
		return Container<uint64_t, value, LevelDBContainer>::write(++idx_, v, sync);
	}

private:
	uint64_t idx_ = 0;
};

} // db
} // qbit

#endif
