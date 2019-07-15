// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_DB_H
#define QBIT_DB_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../allocator.h"
#include "../containers.h"
#include "../streams.h"
#include "../version.h"
#include "../timestamp.h"
#include "../serialize.h"

#include <iostream>

#include <memory>

namespace qbit {
namespace db {

enum Comparer {
	DEFAULT = 0x01,
	UINT32 	= 0x02,
	UINT64 	= 0x03
};

template<typename _key>
class MultiKey {
public:
	MultiKey() {}
	MultiKey(const _key& k) : key_(k) { timestamp_ = qbit::getMicroseconds(); }
	MultiKey(const _key& k, uint64_t timestamp) : key_(k), timestamp_(timestamp) {}

	ADD_SERIALIZE_METHODS;

	inline static char separator() { return 0x00; }

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(key_);
		READWRITE(timestamp_);
	}

	_key& key() { return key_; } 

private:
	_key key_;
	uint64_t timestamp_;
};

//
// Reqular container for any key or value
template<typename key, typename value, template<typename, typename> typename impl >
class Container: public impl<key, value> {
public:
	class Iterator {
	public:
		Iterator(const typename impl<key, value>::_iterator& i) : i_(i) {}

		virtual inline bool valid() { return i_.valid(); }

		inline Iterator& operator++() {
			i_.next();
			return *this;
		}

		inline Iterator& operator++(int) {
			i_.next();
			return *this;
		}

		inline void next() { i_.next(); }

		inline bool first(key& k) { return i_.first(k); }
		inline bool first(DataStream& k) { return i_.first(k); }

		inline bool second(value& v) { return i_.second(v); }
		inline bool second(DataStream& v) { return i_.second(v); }

		inline value operator* () {
			value lValue;
			i_.second(lValue);
			return lValue; 
		}

	private:
		typename impl<key, value>::_iterator i_;
	};

public:
	Container(const std::string& name) : name_(name) {}

	inline bool open(uint32_t cache = 0) { return impl<key, value>::open(name_, cache); }

	inline bool write(const key& k, const value& v, bool sync = false) { return impl<key, value>::write(k, v, sync); }
	inline bool write(const DataStream& k, const DataStream& v, bool sync = false) { return impl<key, value>::write(k, v, sync); }
	
	inline bool read(const key& k, value& v) { return impl<key, value>::read(k, v); }
	inline bool read(const DataStream& k, DataStream& v) { return impl<key, value>::read(k, v); }

	inline Iterator find(const key& k) { return Iterator(impl<key, value>::find(k)); }
	inline Iterator find(const DataStream& k) { return Iterator(impl<key, value>::find(k)); }

protected:
	std::string name_;
};

//
// Multi-key container
template<typename key, typename value, template<typename, typename> typename impl >
class MultiContainer: public Container<MultiKey<key>, value, impl> {
public:
	class Iterator {
	public:
		Iterator(const key& k, const typename Container<MultiKey<key>, value, impl>::Iterator& i) : key_(k), i_(i) {}

		inline bool valid() {
			MultiKey<key> lKey;
			if (i_.valid() && i_.first(lKey) && lKey.key() == key_) {
				return true;
			}

			return false;
		}

		inline Iterator& operator++() {
			i_.next();
			return *this;
		}

		inline Iterator& operator++(int) {
			i_.next();
			return *this;
		}

		inline value operator* () {
			value lValue;
			i_.second(lValue);
			return lValue; 
		}		

	private:
		key key_;
		typename Container<MultiKey<key>, value, impl>::Iterator i_;
	};

public:
	MultiContainer(const std::string& name) : Container<MultiKey<key>, value, impl>(name) {}

	inline bool open(uint32_t cache = 0) { return Container<MultiKey<key>, value, impl>::open(cache); }

	bool write(const key& k, const value& v, bool sync = false) {
		MultiKey<key> lKey(k, 0);

		typename Container<MultiKey<key>, value, impl>::Iterator lBegin = Container<MultiKey<key>, value, impl>::find(lKey);
		if (!lBegin.valid()) return Container<MultiKey<key>, value, impl>::write(lKey, v, sync);
		return Container<MultiKey<key>, value, impl>::write(MultiKey<key>(k), v, sync);
	}

	inline Iterator find(const key& k) {
		return MultiContainer::Iterator(k, Container<MultiKey<key>, value, impl>::find(MultiKey<key>(k, 0))); 
	}

	inline Iterator find(const DataStream& k) {
		return MultiContainer::Iterator(k, Container<MultiKey<key>, value, impl>::find(MultiKey<key>(k, 0)));
	}
};

//
// Entity container for any key and special serialization\desesialization scheme 
template<typename key, typename value, template<typename, typename> typename impl >
class EntityContainer: public Container<key, value, impl> {
public:
	EntityContainer(const std::string& name) : Container<key, value, impl>(name) {}

	inline bool open(uint32_t cache = 0) { return Container<key, value, impl>::open(cache); }

	bool write(const key& k, std::shared_ptr<value> v, bool sync = false) {
		DataStream lKeyStream(SER_DISK, CLIENT_VERSION);
		lKeyStream << k;

		DataStream lValueStream(SER_DISK, CLIENT_VERSION);
		value::Serializer::serialize(lValueStream, v);

		return Container<key, value, impl>::write(lKeyStream, lValueStream, sync);
	}

	std::shared_ptr<value> read(const key& k) {
		DataStream lKeyStream(SER_DISK, CLIENT_VERSION);
		lKeyStream << k;

		DataStream lValueStream(SER_DISK, CLIENT_VERSION);
		
		if (Container<key, value, impl>::read(lKeyStream, lValueStream)) {
			return value::Deserializer::deserialize(lValueStream);
		}

		return nullptr;
	}
};

} // db
} // qbit

#endif