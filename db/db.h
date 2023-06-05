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

	friend inline bool operator < (const MultiKey<_key>& a, const MultiKey<_key>& b) { 
		return a.key() < b.key(); 
	}

	friend inline bool operator > (const MultiKey<_key>& a, const MultiKey<_key>& b) { 
		return a.key() > b.key(); 
	}

	const _key& key() const { return key_; } 

private:
	_key key_;
	uint64_t timestamp_;
};

template<typename _key1, typename _key2>
class TwoKey {
public:
	TwoKey() {}
	TwoKey(const _key1& k1, const _key2& k2) : key1_(k1), key2_(k2) {}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(key1_);
		READWRITE(key2_);
	}

	const _key1& key1() const { return key1_; } 
	const _key2& key2() const { return key2_; } 

	friend inline bool operator < (const TwoKey<_key1, _key2>& a, const TwoKey<_key1, _key2>& b) { 
		if (a.key1() < b.key1()) return true;
		if (a.key1() > b.key1()) return false;
		if (a.key2() < b.key2()) return true;
		if (a.key2() > b.key2()) return false;

		return false;
	}

	friend inline bool operator > (const TwoKey<_key1, _key2>& a, const TwoKey<_key1, _key2>& b) { 
		if (a.key1() > b.key1()) return true;
		if (a.key1() < b.key1()) return false;
		if (a.key2() > b.key2()) return true;
		if (a.key2() < b.key2()) return false;

		return false;
	}

private:
	_key1 key1_;
	_key2 key2_;
};

template<typename _key1, typename _key2, typename _key3>
class ThreeKey {
public:
	ThreeKey() {}
	ThreeKey(const _key1& k1, const _key2& k2, const _key3& k3) : key1_(k1), key2_(k2), key3_(k3) {}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(key1_);
		READWRITE(key2_);
		READWRITE(key3_);
	}

	const _key1& key1() const { return key1_; } 
	const _key2& key2() const { return key2_; } 
	const _key3& key3() const { return key3_; } 

	friend inline bool operator < (const ThreeKey<_key1, _key2, _key3>& a, const ThreeKey<_key1, _key2, _key3>& b) { 
		if (a.key1() < b.key1()) return true;
		if (a.key1() > b.key1()) return false;
		if (a.key2() < b.key2()) return true;
		if (a.key2() > b.key2()) return false;
		if (a.key3() < b.key3()) return true;
		if (a.key3() > b.key3()) return false;

		return false;
	}

	friend inline bool operator > (const ThreeKey<_key1, _key2, _key3>& a, const ThreeKey<_key1, _key2, _key3>& b) { 
		if (a.key1() > b.key1()) return true;
		if (a.key1() < b.key1()) return false;
		if (a.key2() > b.key2()) return true;
		if (a.key2() < b.key2()) return false;
		if (a.key3() > b.key3()) return true;
		if (a.key3() < b.key3()) return false;

		return false;
	}

private:
	_key1 key1_;
	_key2 key2_;
	_key3 key3_;
};

template<typename _key1, typename _key2, typename _key3, typename _key4>
class FourKey {
public:
	FourKey() {}
	FourKey(const _key1& k1, const _key2& k2, const _key3& k3, const _key4& k4) : key1_(k1), key2_(k2), key3_(k3), key4_(k4) {}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(key1_);
		READWRITE(key2_);
		READWRITE(key3_);
		READWRITE(key4_);
	}

	const _key1& key1() const { return key1_; } 
	const _key2& key2() const { return key2_; } 
	const _key3& key3() const { return key3_; } 
	const _key4& key4() const { return key4_; } 

	friend inline bool operator < (const FourKey<_key1, _key2, _key3, _key4>& a, const FourKey<_key1, _key2, _key3, _key4>& b) { 
		if (a.key1() < b.key1()) return true;
		if (a.key1() > b.key1()) return false;
		if (a.key2() < b.key2()) return true;
		if (a.key2() > b.key2()) return false;
		if (a.key3() < b.key3()) return true;
		if (a.key3() > b.key3()) return false;
		if (a.key4() < b.key4()) return true;
		if (a.key4() > b.key4()) return false;

		return false;
	}

	friend inline bool operator > (const FourKey<_key1, _key2, _key3, _key4>& a, const FourKey<_key1, _key2, _key3, _key4>& b) { 
		if (a.key1() > b.key1()) return true;
		if (a.key1() < b.key1()) return false;
		if (a.key2() > b.key2()) return true;
		if (a.key2() < b.key2()) return false;
		if (a.key3() > b.key3()) return true;
		if (a.key3() < b.key3()) return false;
		if (a.key4() > b.key4()) return true;
		if (a.key4() < b.key4()) return false;

		return false;
	}

private:
	_key1 key1_;
	_key2 key2_;
	_key3 key3_;
	_key4 key4_;
};

template<typename _key1, typename _key2, typename _key3, typename _key4, typename _key5>
class FiveKey {
public:
	FiveKey() {}
	FiveKey(const _key1& k1, const _key2& k2, const _key3& k3, const _key4& k4, const _key5& k5) : key1_(k1), key2_(k2), key3_(k3), key4_(k4), key5_(k5) {}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(key1_);
		READWRITE(key2_);
		READWRITE(key3_);
		READWRITE(key4_);
		READWRITE(key5_);
	}

	const _key1& key1() const { return key1_; } 
	const _key2& key2() const { return key2_; } 
	const _key3& key3() const { return key3_; } 
	const _key4& key4() const { return key4_; } 
	const _key5& key5() const { return key5_; } 

	friend inline bool operator < (const FiveKey<_key1, _key2, _key3, _key4, _key5>& a, const FiveKey<_key1, _key2, _key3, _key4, _key5>& b) { 
		if (a.key1() < b.key1()) return true;
		if (a.key1() > b.key1()) return false;
		if (a.key2() < b.key2()) return true;
		if (a.key2() > b.key2()) return false;
		if (a.key3() < b.key3()) return true;
		if (a.key3() > b.key3()) return false;
		if (a.key4() < b.key4()) return true;
		if (a.key4() > b.key4()) return false;
		if (a.key5() < b.key5()) return true;
		if (a.key5() > b.key5()) return false;

		return false;
	}

	friend inline bool operator > (const FiveKey<_key1, _key2, _key3, _key4, _key5>& a, const FiveKey<_key1, _key2, _key3, _key4, _key5>& b) { 
		if (a.key1() > b.key1()) return true;
		if (a.key1() < b.key1()) return false;
		if (a.key2() > b.key2()) return true;
		if (a.key2() < b.key2()) return false;
		if (a.key3() > b.key3()) return true;
		if (a.key3() < b.key3()) return false;
		if (a.key4() > b.key4()) return true;
		if (a.key4() < b.key4()) return false;
		if (a.key5() > b.key5()) return true;
		if (a.key5() < b.key5()) return false;

		return false;
	}

private:
	_key1 key1_;
	_key2 key2_;
	_key3 key3_;
	_key4 key4_;
	_key5 key5_;
};

//
// Reqular container for any key or value
template<typename key, typename value, template<typename, typename> typename impl >
class Container: public impl<key, value> {
public:
	class Iterator {
	public:
		Iterator() {}
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

		inline Iterator& operator--() {
			i_.prev();
			return *this;
		}

		inline Iterator& operator--(int) {
			i_.prev();
			return *this;
		}

		inline void next() { i_.next(); }
		inline void prev() { i_.prev(); }

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

	class Transaction {
	public:
		Transaction(const typename impl<key, value>::_transaction& t) : t_(t) {}

		inline void write(const key& k, const value& v) { t_.write(k, v); }
		inline void write(const DataStream& k, const DataStream& v) { t_.write(k, v); }

		inline void remove(const key& k) { t_.remove(k); }
		inline void remove(const DataStream& k) { t_.remove(k); }

		inline bool commit(bool sync = false) { return t_.commit(sync); }

	private:
		typename impl<key, value>::_transaction t_;
	};

public:
	Container(const std::string& name) : name_(name) {}

	inline bool open(bool useTypedComparer = true, uint32_t cache = 0) { return impl<key, value>::open(name_, useTypedComparer, cache); }
	inline bool repair(bool useTypedComparer = true, uint32_t cache = 0) { return impl<key, value>::repair(name_, useTypedComparer, cache); }

	inline void close() { impl<key, value>::close(); }
	inline bool opened() { return impl<key, value>::opened(); }

	inline bool write(const key& k, const value& v, bool sync = false) { return impl<key, value>::write(k, v, sync); }
	inline bool write(const DataStream& k, const DataStream& v, bool sync = false) { return impl<key, value>::write(k, v, sync); }
	
	inline bool read(const key& k, value& v) { return impl<key, value>::read(k, v); }
	inline bool read(const DataStream& k, DataStream& v) { return impl<key, value>::read(k, v); }

	//
	// any find* methods locating _near_ key/value: >=
	inline Iterator find(const key& k) { return Iterator(impl<key, value>::find(k)); }
	inline Iterator find(const DataStream& k) { return Iterator(impl<key, value>::find(k)); }

	inline Iterator begin() { return Iterator(impl<key, value>::begin()); }
	inline Iterator last() { return Iterator(impl<key, value>::last()); }

	inline bool remove(const key& k, bool sync = false) { return impl<key, value>::remove(k, sync); }
	inline bool remove(const DataStream& k, bool sync = false) { return impl<key, value>::remove(k, sync); }

	inline bool remove(const Iterator& iter, bool sync = false) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		const_cast<Iterator&>(iter).first(lKeyStream);
		return remove(lKeyStream, sync);
	}

	inline Transaction transaction() { return Transaction(impl<key, value>::transaction()); }

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
		Iterator() {}
		Iterator(const key& k, const typename Container<MultiKey<key>, value, impl>::Iterator& i) : keyEmpty_(false), key_(k), i_(i) {}
		Iterator(const typename Container<MultiKey<key>, value, impl>::Iterator& i) : keyEmpty_(true), i_(i) {}

		inline bool valid() {
			MultiKey<key> lKey;
			if (i_.valid() && i_.first(lKey) && (!keyEmpty_ && lKey.key() == key_ || keyEmpty_)) {
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

		inline Iterator& operator--() {
			i_.prev();
			return *this;
		}

		inline Iterator& operator--(int) {
			i_.prev();
			return *this;
		}

		inline value operator* () {
			value lValue;
			i_.second(lValue);
			return lValue; 
		}

		inline bool first(key& k) {
			MultiKey<key> lKey;
			bool lResult = i_.first(lKey);
			k = lKey.key();
			return lResult; 
		}

		inline bool first(DataStream& k) { return i_.first(k); }

		inline bool second(value& v) { return i_.second(v); }
		inline bool second(DataStream& v) { return i_.second(v); }

	private:
		bool keyEmpty_;
		key key_;
		typename Container<MultiKey<key>, value, impl>::Iterator i_;
	};

	class Transaction {
	public:
		Transaction(const typename Container<MultiKey<key>, value, impl>::Transaction& t) : t_(t) {}

		inline void write(const key& k, const value& v) { t_.write(k, v); }
		inline void write(const DataStream& k, const DataStream& v) { t_.write(k, v); }

		inline void remove(const key& k) { t_.remove(k); }
		inline void remove(const DataStream& k) { t_.remove(k); }

		inline void remove(const MultiContainer::Iterator& iter) {
			DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
			const_cast<MultiContainer::Iterator&>(iter).first(lKeyStream);
			remove(lKeyStream);
		}

		inline bool commit(bool sync = false) { return t_.commit(sync); }

	private:
		typename Container<MultiKey<key>, value, impl>::Transaction t_;
	};	

public:
	MultiContainer(const std::string& name) : Container<MultiKey<key>, value, impl>(name) {}

	inline bool open(uint32_t cache = 0) { return Container<MultiKey<key>, value, impl>::open(false /*typed comparer for multikey is not suitable*/, cache); }
	inline void close() { Container<MultiKey<key>, value, impl>::close(); }
	inline bool opened() { return Container<MultiKey<key>, value, impl>::opened(); }

	bool write(const key& k, const value& v, bool sync = false) {
		MultiKey<key> lKey(k, 0);

		typename Container<MultiKey<key>, value, impl>::Iterator lBegin = Container<MultiKey<key>, value, impl>::find(lKey);
		if (!lBegin.valid()) return Container<MultiKey<key>, value, impl>::write(lKey, v, sync);
		return Container<MultiKey<key>, value, impl>::write(MultiKey<key>(k), v, sync);
	}

	inline Iterator find(const key& k) {
		return MultiContainer::Iterator(k, Container<MultiKey<key>, value, impl>::find(MultiKey<key>(k, 0))); 
	}

	inline Iterator begin() { return MultiContainer::Iterator(Container<MultiKey<key>, value, impl>::begin()); }
	inline Iterator last() { return MultiContainer::Iterator(Container<MultiKey<key>, value, impl>::last()); }

	inline bool remove(const MultiContainer::Iterator& iter, bool sync = false) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		const_cast<MultiContainer::Iterator&>(iter).first(lKeyStream);
		return Container<MultiKey<key>, value, impl>::remove(lKeyStream, sync);
	}

	inline Transaction transaction() { return Transaction(Container<MultiKey<key>, value, impl>::transaction()); }	
};

//
// Two-key container
template<typename key1, typename key2, typename value, template<typename, typename> typename impl >
class TwoKeyContainer: public Container<TwoKey<key1, key2>, value, impl> {
public:
	class Iterator {
	public:
		Iterator() {}
		Iterator(const key1& k1, const key2& k2, const typename Container<TwoKey<key1, key2>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(false), key1_(k1), key2_(k2), i_(i) {}
		Iterator(const key1& k1, const typename Container<TwoKey<key1, key2>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(true), key1_(k1), i_(i) {}
		Iterator(const typename Container<TwoKey<key1, key2>, value, impl>::Iterator& i) : key1Empty_(true), key2Empty_(true), i_(i) {}

		inline bool valid() {
			TwoKey<key1, key2> lKey;
			if (i_.valid() && i_.first(lKey) && 
					(!key1Empty_ && lKey.key1() == key1_ || key1Empty_) && 
					(!key2Empty_ && lKey.key2() == key2_ || key2Empty_)) {
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

		inline Iterator& operator--() {
			i_.prev();
			return *this;
		}

		inline Iterator& operator--(int) {
			i_.prev();
			return *this;
		}

		inline value operator* () {
			value lValue;
			i_.second(lValue);
			return lValue; 
		}

		inline bool first(key1& k1, key2& k2) {
			TwoKey<key1, key2> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			k2 = lKey.key2();
			return lResult; 
		}

		inline bool first(key1& k1) {
			TwoKey<key1, key2> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			return lResult; 
		}

		inline bool first(TwoKey<key1, key2>& k) {
			bool lResult = i_.first(k);
			return lResult; 
		}

		inline bool first(DataStream& k) { return i_.first(k); }

		inline bool second(value& v) { return i_.second(v); }
		inline bool second(DataStream& v) { return i_.second(v); }

		inline void setKey2Empty() { key2Empty_ = true; }
		inline void setKey1Empty() { key2Empty_ = key1Empty_ = true; }

	private:
		bool key1Empty_;
		bool key2Empty_;
		key1 key1_;
		key2 key2_;
		typename Container<TwoKey<key1, key2>, value, impl>::Iterator i_;
	};

	class Transaction {
	public:
		Transaction(const typename Container<TwoKey<key1, key2>, value, impl>::Transaction& t) : t_(t) {}

		inline void write(const key1& k1, const key2& k2, const value& v) { t_.write(TwoKey<key1, key2>(k1, k2), v); }
		inline void write(const TwoKey<key1, key2>& k, const value& v) { t_.write(k, v); }
		inline void write(const DataStream& k, const DataStream& v) { t_.write(k, v); }

		inline void remove(const key1& k1, const key2& k2) { t_.remove(TwoKey<key1, key2>(k1, k2)); }
		inline void remove(const TwoKey<key1, key2>& k) { t_.remove(k); }
		inline void remove(const DataStream& k) { t_.remove(k); }

		inline void remove(const TwoKeyContainer::Iterator& iter) {
			DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
			const_cast<TwoKeyContainer::Iterator&>(iter).first(lKeyStream);
			remove(lKeyStream);
		}

		inline bool commit(bool sync = false) { return t_.commit(sync); }

	private:
		typename Container<TwoKey<key1, key2>, value, impl>::Transaction t_;
	};	

public:
	TwoKeyContainer(const std::string& name) : Container<TwoKey<key1, key2>, value, impl>(name) {}

	inline bool open(bool useTypedComparer = true, uint32_t cache = 0) { return Container<TwoKey<key1, key2>, value, impl>::open(useTypedComparer, cache); }
	inline bool repair(bool useTypedComparer = true, uint32_t cache = 0) { return Container<TwoKey<key1, key2>, value, impl>::repair(useTypedComparer, cache); }
	inline void close() { Container<TwoKey<key1, key2>, value, impl>::close(); }
	inline bool opened() { return Container<TwoKey<key1, key2>, value, impl>::opened(); }

	bool write(const key1& k1, const value& v, bool sync = false) {
		TwoKey<key1, key2> lKey(k1, key2());
		return Container<TwoKey<key1, key2>, value, impl>::write(lKey, v, sync);
	}

	bool write(const key1& k1, const key2& k2, const value& v, bool sync = false) {
		TwoKey<key1, key2> lKey(k1, k2);
		return Container<TwoKey<key1, key2>, value, impl>::write(lKey, v, sync);
	}

	bool remove(const key1& k1, const key2& k2, bool sync = false) {
		TwoKey<key1, key2> lKey(k1, k2);
		return Container<TwoKey<key1, key2>, value, impl>::remove(lKey, sync);
	}	

	inline Iterator find(const key1& k1) {
		return TwoKeyContainer::Iterator(k1, Container<TwoKey<key1, key2>, value, impl>::find(TwoKey<key1, key2>(k1, key2()))); 
	}

	inline Iterator find(const key1& k1, const key2& k2) {
		return TwoKeyContainer::Iterator(k1, k2, Container<TwoKey<key1, key2>, value, impl>::find(TwoKey<key1, key2>(k1, k2))); 
	}

	inline Iterator begin() { return TwoKeyContainer::Iterator(Container<TwoKey<key1, key2>, value, impl>::begin()); }
	inline Iterator last() { return TwoKeyContainer::Iterator(Container<TwoKey<key1, key2>, value, impl>::last()); }

	inline bool remove(const TwoKeyContainer::Iterator& iter, bool sync = false) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		const_cast<TwoKeyContainer::Iterator&>(iter).first(lKeyStream);
		return Container<TwoKey<key1, key2>, value, impl>::remove(lKeyStream, sync);
	}

	inline Transaction transaction() { return Transaction(Container<TwoKey<key1, key2>, value, impl>::transaction()); }	
};

//
// Tree-key container
template<typename key1, typename key2, typename key3, typename value, template<typename, typename> typename impl >
class ThreeKeyContainer: public Container<ThreeKey<key1, key2, key3>, value, impl> {
public:
	class Iterator {
	public:
		Iterator() {}
		Iterator(const key1& k1, const key2& k2, const key3& k3, const typename Container<ThreeKey<key1, key2, key3>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(false), key3Empty_(false), key1_(k1), key2_(k2), key3_(k3), i_(i) {}
		Iterator(const key1& k1, const key2& k2, const typename Container<ThreeKey<key1, key2, key3>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(false), key3Empty_(true), key1_(k1), key2_(k2), i_(i) {}
		Iterator(const key1& k1, const typename Container<ThreeKey<key1, key2, key3>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(true), key3Empty_(true), key1_(k1), i_(i) {}
		Iterator(const typename Container<ThreeKey<key1, key2, key3>, value, impl>::Iterator& i) : key1Empty_(true), key2Empty_(true), key3Empty_(true), i_(i) {}

		inline bool valid() {
			ThreeKey<key1, key2, key3> lKey;
			if (i_.valid() && i_.first(lKey) && 
					(!key1Empty_ && lKey.key1() == key1_ || key1Empty_) && 
					(!key2Empty_ && lKey.key2() == key2_ || key2Empty_) &&
					(!key3Empty_ && lKey.key3() == key3_ || key3Empty_)) {
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

		inline Iterator& operator--() {
			i_.prev();
			return *this;
		}

		inline Iterator& operator--(int) {
			i_.prev();
			return *this;
		}

		inline value operator* () {
			value lValue;
			i_.second(lValue);
			return lValue; 
		}

		inline bool first(key1& k1, key2& k2, key3& k3) {
			ThreeKey<key1, key2, key3> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			k2 = lKey.key2();
			k3 = lKey.key3();
			return lResult; 
		}

		inline bool first(key1& k1, key2& k2) {
			ThreeKey<key1, key2, key3> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			k2 = lKey.key2();
			return lResult; 
		}

		inline bool first(key1& k1) {
			ThreeKey<key1, key2, key3> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			return lResult; 
		}

		inline bool first(ThreeKey<key1, key2, key3>& k) {
			bool lResult = i_.first(k);
			return lResult; 
		}

		inline bool first(DataStream& k) { return i_.first(k); }

		inline bool second(value& v) { return i_.second(v); }
		inline bool second(DataStream& v) { return i_.second(v); }

		inline void setKey3Empty() { key3Empty_ = true; }
		inline void setKey2Empty() { key3Empty_ = key2Empty_ = true; }
		inline void setKey1Empty() { key3Empty_ = key2Empty_ = key1Empty_ = true; }		

	private:
		bool key1Empty_;
		bool key2Empty_;
		bool key3Empty_;
		key1 key1_;
		key2 key2_;
		key3 key3_;
		typename Container<ThreeKey<key1, key2, key3>, value, impl>::Iterator i_;
	};

	class Transaction {
	public:
		Transaction(const typename Container<ThreeKey<key1, key2, key3>, value, impl>::Transaction& t) : t_(t) {}

		inline void write(const key1& k1, const key2& k2, const key3& k3, const value& v) { t_.write(ThreeKey<key1, key2, key3>(k1, k2, k3), v); }
		inline void write(const ThreeKey<key1, key2, key3>& k, const value& v) { t_.write(k, v); }
		inline void write(const DataStream& k, const DataStream& v) { t_.write(k, v); }

		inline void remove(const key1& k1, const key2& k2, const key3& k3) { t_.remove(ThreeKey<key1, key2, key3>(k1, k2, k3)); }
		inline void remove(const ThreeKey<key1, key2, key3>& k) { t_.remove(k); }
		inline void remove(const DataStream& k) { t_.remove(k); }

		inline void remove(const ThreeKeyContainer::Iterator& iter) {
			DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
			const_cast<ThreeKeyContainer::Iterator&>(iter).first(lKeyStream);
			remove(lKeyStream);
		}

		inline bool commit(bool sync = false) { return t_.commit(sync); }

	private:
		typename Container<ThreeKey<key1, key2, key3>, value, impl>::Transaction t_;
	};	

public:
	ThreeKeyContainer(const std::string& name) : Container<ThreeKey<key1, key2, key3>, value, impl>(name) {}

	inline bool open(bool useTypedComparer = true, uint32_t cache = 0) { return Container<ThreeKey<key1, key2, key3>, value, impl>::open(useTypedComparer, cache); }
	inline bool repair(bool useTypedComparer = true, uint32_t cache = 0) { return Container<ThreeKey<key1, key2, key3>, value, impl>::repair(useTypedComparer, cache); }
	inline void close() { Container<ThreeKey<key1, key2, key3>, value, impl>::close(); }
	inline bool opened() { return Container<ThreeKey<key1, key2, key3>, value, impl>::opened(); }	

	bool write(const key1& k1, const value& v, bool sync = false) {
		ThreeKey<key1, key2, key3> lKey(k1, key2(), key3());
		return Container<ThreeKey<key1, key2, key3>, value, impl>::write(lKey, v, sync);
	}

	bool write(const key1& k1, const key2& k2, const value& v, bool sync = false) {
		ThreeKey<key1, key2, key3> lKey(k1, k2, key3());
		return Container<ThreeKey<key1, key2, key3>, value, impl>::write(lKey, v, sync);
	}

	bool write(const key1& k1, const key2& k2, const key3& k3, const value& v, bool sync = false) {
		ThreeKey<key1, key2, key3> lKey(k1, k2, k3);
		return Container<ThreeKey<key1, key2, key3>, value, impl>::write(lKey, v, sync);
	}

	bool remove(const key1& k1, const key2& k2, const key3& k3, bool sync = false) {
		ThreeKey<key1, key2, key3> lKey(k1, k2, k3);
		return Container<ThreeKey<key1, key2, key3>, value, impl>::remove(lKey, sync);
	}

	inline Iterator find(const key1& k1) {
		return ThreeKeyContainer::Iterator(k1, Container<ThreeKey<key1, key2, key3>, value, impl>::find(ThreeKey<key1, key2, key3>(k1, key2(), key3()))); 
	}

	inline Iterator find(const key1& k1, const key2& k2) {
		return ThreeKeyContainer::Iterator(k1, k2, Container<ThreeKey<key1, key2, key3>, value, impl>::find(ThreeKey<key1, key2, key3>(k1, k2, key3()))); 
	}

	inline Iterator find(const key1& k1, const key2& k2, const key3& k3) {
		return ThreeKeyContainer::Iterator(k1, k2, k3, Container<ThreeKey<key1, key2, key3>, value, impl>::find(ThreeKey<key1, key2, key3>(k1, k2, k3))); 
	}

	inline Iterator begin() { return ThreeKeyContainer::Iterator(Container<ThreeKey<key1, key2, key3>, value, impl>::begin()); }
	inline Iterator last() { return ThreeKeyContainer::Iterator(Container<ThreeKey<key1, key2, key3>, value, impl>::last()); }

	inline bool remove(const ThreeKeyContainer::Iterator& iter, bool sync = false) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		const_cast<ThreeKeyContainer::Iterator&>(iter).first(lKeyStream);
		return Container<ThreeKey<key1, key2, key3>, value, impl>::remove(lKeyStream, sync);
	}

	inline Transaction transaction() { return Transaction(Container<ThreeKey<key1, key2, key3>, value, impl>::transaction()); }	
};

//
// Four-key container
template<typename key1, typename key2, typename key3, typename key4, typename value, template<typename, typename> typename impl >
class FourKeyContainer: public Container<FourKey<key1, key2, key3, key4>, value, impl> {
public:
	class Iterator {
	public:
		Iterator() {}
		Iterator(const key1& k1, const key2& k2, const key3& k3, const key4& k4, const typename Container<FourKey<key1, key2, key3, key4>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(false), key3Empty_(false), key4Empty_(false), key1_(k1), key2_(k2), key3_(k3), key4_(k4), i_(i) {}
		Iterator(const key1& k1, const key2& k2, const key3& k3, const typename Container<FourKey<key1, key2, key3, key4>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(false), key3Empty_(false), key4Empty_(true), key1_(k1), key2_(k2), key3_(k3), i_(i) {}
		Iterator(const key1& k1, const key2& k2, const typename Container<FourKey<key1, key2, key3, key4>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(false), key3Empty_(true), key4Empty_(true), key1_(k1), key2_(k2), i_(i) {}
		Iterator(const key1& k1, const typename Container<FourKey<key1, key2, key3, key4>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(true), key3Empty_(true), key4Empty_(true), key1_(k1), i_(i) {}
		Iterator(const typename Container<FourKey<key1, key2, key3, key4>, value, impl>::Iterator& i) : key1Empty_(true), key2Empty_(true), key3Empty_(true), key4Empty_(true), i_(i) {}

		inline bool valid() {
			FourKey<key1, key2, key3, key4> lKey;
			if (i_.valid() && i_.first(lKey) && 
					(!key1Empty_ && lKey.key1() == key1_ || key1Empty_) && 
					(!key2Empty_ && lKey.key2() == key2_ || key2Empty_) &&
					(!key3Empty_ && lKey.key3() == key3_ || key3Empty_) &&
					(!key4Empty_ && lKey.key4() == key4_ || key4Empty_)) {
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

		inline Iterator& operator--() {
			i_.prev();
			return *this;
		}

		inline Iterator& operator--(int) {
			i_.prev();
			return *this;
		}

		inline value operator* () {
			value lValue;
			i_.second(lValue);
			return lValue; 
		}

		inline bool first(key1& k1, key2& k2, key3& k3, key4& k4) {
			FourKey<key1, key2, key3, key4> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			k2 = lKey.key2();
			k3 = lKey.key3();
			k4 = lKey.key4();
			return lResult; 
		}

		inline bool first(key1& k1, key2& k2, key3& k3) {
			FourKey<key1, key2, key3, key4> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			k2 = lKey.key2();
			k3 = lKey.key3();
			return lResult; 
		}

		inline bool first(key1& k1, key2& k2) {
			FourKey<key1, key2, key3, key4> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			k2 = lKey.key2();
			return lResult; 
		}

		inline bool first(key1& k1) {
			FourKey<key1, key2, key3, key4> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			return lResult; 
		}

		inline bool first(FourKey<key1, key2, key3, key4>& k) {
			bool lResult = i_.first(k);
			return lResult; 
		}

		inline bool first(DataStream& k) { return i_.first(k); }

		inline bool second(value& v) { return i_.second(v); }
		inline bool second(DataStream& v) { return i_.second(v); }

		inline void setKey4Empty() { key4Empty_ = true; }
		inline void setKey3Empty() { key4Empty_ = key3Empty_ = true; }
		inline void setKey2Empty() { key4Empty_ = key3Empty_ = key2Empty_ = true; }
		inline void setKey1Empty() { key4Empty_ = key3Empty_ = key2Empty_ = key1Empty_ = true; }

	private:
		bool key1Empty_;
		bool key2Empty_;
		bool key3Empty_;
		bool key4Empty_;
		key1 key1_;
		key2 key2_;
		key3 key3_;
		key4 key4_;
		typename Container<FourKey<key1, key2, key3, key4>, value, impl>::Iterator i_;
	};

	class Transaction {
	public:
		Transaction(const typename Container<FourKey<key1, key2, key3, key4>, value, impl>::Transaction& t) : t_(t) {}

		inline void write(const key1& k1, const key2& k2, const key3& k3, const key4& k4, const value& v) { t_.write(FourKey<key1, key2, key3, key4>(k1, k2, k3, k4), v); }
		inline void write(const FourKey<key1, key2, key3, key4>& k, const value& v) { t_.write(k, v); }
		inline void write(const DataStream& k, const DataStream& v) { t_.write(k, v); }

		inline void remove(const key1& k1, const key2& k2, const key3& k3, const key4& k4) { t_.remove(FourKey<key1, key2, key3, key4>(k1, k2, k3, k4)); }
		inline void remove(const FourKey<key1, key2, key3, key4>& k) { t_.remove(k); }
		inline void remove(const DataStream& k) { t_.remove(k); }

		inline void remove(const FourKeyContainer::Iterator& iter) {
			DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
			const_cast<FourKeyContainer::Iterator&>(iter).first(lKeyStream);
			remove(lKeyStream);
		}

		inline bool commit(bool sync = false) { return t_.commit(sync); }

	private:
		typename Container<FourKey<key1, key2, key3, key4>, value, impl>::Transaction t_;
	};	

public:
	FourKeyContainer(const std::string& name) : Container<FourKey<key1, key2, key3, key4>, value, impl>(name) {}

	inline bool open(bool useTypedComparer = true, uint32_t cache = 0) { return Container<FourKey<key1, key2, key3, key4>, value, impl>::open(useTypedComparer, cache); }
	inline bool repair(bool useTypedComparer = true, uint32_t cache = 0) { return Container<FourKey<key1, key2, key3, key4>, value, impl>::repair(useTypedComparer, cache); }
	inline void close() { Container<FourKey<key1, key2, key3, key4>, value, impl>::close(); }
	inline bool opened() { return Container<FourKey<key1, key2, key3, key4>, value, impl>::opened(); }	

	bool write(const key1& k1, const value& v, bool sync = false) {
		FourKey<key1, key2, key3, key4> lKey(k1, key2(), key3(), key4());
		return Container<FourKey<key1, key2, key3, key4>, value, impl>::write(lKey, v, sync);
	}

	bool write(const key1& k1, const key2& k2, const value& v, bool sync = false) {
		FourKey<key1, key2, key3, key4> lKey(k1, k2, key3(), key4());
		return Container<FourKey<key1, key2, key3, key4>, value, impl>::write(lKey, v, sync);
	}

	bool write(const key1& k1, const key2& k2, const key3& k3, const value& v, bool sync = false) {
		FourKey<key1, key2, key3, key4> lKey(k1, k2, k3, key4());
		return Container<FourKey<key1, key2, key3, key4>, value, impl>::write(lKey, v, sync);
	}

	bool write(const key1& k1, const key2& k2, const key3& k3, const key4& k4, const value& v, bool sync = false) {
		FourKey<key1, key2, key3, key4> lKey(k1, k2, k3, k4);
		return Container<FourKey<key1, key2, key3, key4>, value, impl>::write(lKey, v, sync);
	}

	bool remove(const key1& k1, const key2& k2, const key3& k3, const key4& k4, bool sync = false) {
		FourKey<key1, key2, key3, key4> lKey(k1, k2, k3, k4);
		return Container<FourKey<key1, key2, key3, key4>, value, impl>::remove(lKey, sync);
	}

	inline Iterator find(const key1& k1) {
		return FourKeyContainer::Iterator(k1, Container<FourKey<key1, key2, key3, key4>, value, impl>::find(FourKey<key1, key2, key3, key4>(k1, key2(), key3(), key4()))); 
	}

	inline Iterator find(const key1& k1, const key2& k2) {
		return FourKeyContainer::Iterator(k1, k2, Container<FourKey<key1, key2, key3, key4>, value, impl>::find(FourKey<key1, key2, key3, key4>(k1, k2, key3(), key4())));
	}

	inline Iterator find(const key1& k1, const key2& k2, const key3& k3) {
		return FourKeyContainer::Iterator(k1, k2, k3, Container<FourKey<key1, key2, key3, key4>, value, impl>::find(FourKey<key1, key2, key3, key4>(k1, k2, k3, key4())));
	}

	inline Iterator find(const key1& k1, const key2& k2, const key3& k3, const key4& k4) {
		return FourKeyContainer::Iterator(k1, k2, k3, k4, Container<FourKey<key1, key2, key3, key4>, value, impl>::find(FourKey<key1, key2, key3, key4>(k1, k2, k3, k4)));
	}

	inline Iterator begin() { return FourKeyContainer::Iterator(Container<FourKey<key1, key2, key3, key4>, value, impl>::begin()); }
	inline Iterator last() { return FourKeyContainer::Iterator(Container<FourKey<key1, key2, key3, key4>, value, impl>::last()); }

	inline bool remove(const FourKeyContainer::Iterator& iter, bool sync = false) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		const_cast<FourKeyContainer::Iterator&>(iter).first(lKeyStream);
		return Container<FourKey<key1, key2, key3, key4>, value, impl>::remove(lKeyStream, sync);
	}

	inline Transaction transaction() { return Transaction(Container<FourKey<key1, key2, key3, key4>, value, impl>::transaction()); }
};

//
// Five-key container
template<typename key1, typename key2, typename key3, typename key4, typename key5, typename value, template<typename, typename> typename impl >
class FiveKeyContainer: public Container<FiveKey<key1, key2, key3, key4, key5>, value, impl> {
public:
	class Iterator {
	public:
		Iterator() {}
		Iterator(const key1& k1, const key2& k2, const key3& k3, const key4& k4, const key5& k5, const typename Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(false), key3Empty_(false), key4Empty_(false), key5Empty_(false), key1_(k1), key2_(k2), key3_(k3), key4_(k4), key5_(k5), i_(i) {}
		Iterator(const key1& k1, const key2& k2, const key3& k3, const key4& k4, const typename Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(false), key3Empty_(false), key4Empty_(false), key5Empty_(true), key1_(k1), key2_(k2), key3_(k3), key4_(k4), i_(i) {}
		Iterator(const key1& k1, const key2& k2, const key3& k3, const typename Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(false), key3Empty_(false), key4Empty_(true), key5Empty_(true), key1_(k1), key2_(k2), key3_(k3), i_(i) {}
		Iterator(const key1& k1, const key2& k2, const typename Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(false), key3Empty_(true), key4Empty_(true), key5Empty_(true), key1_(k1), key2_(k2), i_(i) {}
		Iterator(const key1& k1, const typename Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::Iterator& i) : key1Empty_(false), key2Empty_(true), key3Empty_(true), key4Empty_(true), key5Empty_(true), key1_(k1), i_(i) {}
		Iterator(const typename Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::Iterator& i) : key1Empty_(true), key2Empty_(true), key3Empty_(true), key4Empty_(true), key5Empty_(true), i_(i) {}

		inline bool valid() {
			FiveKey<key1, key2, key3, key4, key5> lKey;
			if (i_.valid() && i_.first(lKey) && 
					(!key1Empty_ && lKey.key1() == key1_ || key1Empty_) && 
					(!key2Empty_ && lKey.key2() == key2_ || key2Empty_) &&
					(!key3Empty_ && lKey.key3() == key3_ || key3Empty_) &&
					(!key4Empty_ && lKey.key4() == key4_ || key4Empty_) &&
					(!key5Empty_ && lKey.key5() == key5_ || key5Empty_)) {
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

		inline Iterator& operator--() {
			i_.prev();
			return *this;
		}

		inline Iterator& operator--(int) {
			i_.prev();
			return *this;
		}

		inline value operator* () {
			value lValue;
			i_.second(lValue);
			return lValue; 
		}

		inline bool first(key1& k1, key2& k2, key3& k3, key4& k4, key5& k5) {
			FiveKey<key1, key2, key3, key4, key5> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			k2 = lKey.key2();
			k3 = lKey.key3();
			k4 = lKey.key4();
			k5 = lKey.key5();
			return lResult; 
		}

		inline bool first(key1& k1, key2& k2, key3& k3, key4& k4) {
			FiveKey<key1, key2, key3, key4, key5> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			k2 = lKey.key2();
			k3 = lKey.key3();
			k4 = lKey.key4();
			return lResult; 
		}

		inline bool first(key1& k1, key2& k2, key3& k3) {
			FiveKey<key1, key2, key3, key4, key5> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			k2 = lKey.key2();
			k3 = lKey.key3();
			return lResult; 
		}

		inline bool first(key1& k1, key2& k2) {
			FiveKey<key1, key2, key3, key4, key5> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			k2 = lKey.key2();
			return lResult; 
		}

		inline bool first(key1& k1) {
			FiveKey<key1, key2, key3, key4, key5> lKey;
			bool lResult = i_.first(lKey);
			k1 = lKey.key1();
			return lResult; 
		}

		inline bool first(FiveKey<key1, key2, key3, key4, key5>& k) {
			bool lResult = i_.first(k);
			return lResult; 
		}

		inline bool first(DataStream& k) { return i_.first(k); }

		inline bool second(value& v) { return i_.second(v); }
		inline bool second(DataStream& v) { return i_.second(v); }

		inline void setKey5Empty() { key5Empty_ = true; }
		inline void setKey4Empty() { key5Empty_ = key4Empty_ = true; }
		inline void setKey3Empty() { key5Empty_ = key4Empty_ = key3Empty_ = true; }
		inline void setKey2Empty() { key5Empty_ = key4Empty_ = key3Empty_ = key2Empty_ = true; }
		inline void setKey1Empty() { key5Empty_ = key4Empty_ = key3Empty_ = key2Empty_ = key1Empty_ = true; }

	private:
		bool key1Empty_;
		bool key2Empty_;
		bool key3Empty_;
		bool key4Empty_;
		bool key5Empty_;
		key1 key1_;
		key2 key2_;
		key3 key3_;
		key4 key4_;
		key5 key5_;
		typename Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::Iterator i_;
	};

	class Transaction {
	public:
		Transaction(const typename Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::Transaction& t) : t_(t) {}

		inline void write(const key1& k1, const key2& k2, const key3& k3, const key4& k4, const key5& k5, const value& v) { t_.write(FiveKey<key1, key2, key3, key4, key5>(k1, k2, k3, k4, k5), v); }
		inline void write(const FiveKey<key1, key2, key3, key4, key5>& k, const value& v) { t_.write(k, v); }
		inline void write(const DataStream& k, const DataStream& v) { t_.write(k, v); }

		inline void remove(const key1& k1, const key2& k2, const key3& k3, const key4& k4, const key5& k5) { t_.remove(FiveKey<key1, key2, key3, key4, key5>(k1, k2, k3, k4, k5)); }
		inline void remove(const FiveKey<key1, key2, key3, key4, key5>& k) { t_.remove(k); }
		inline void remove(const DataStream& k) { t_.remove(k); }

		inline void remove(const FiveKeyContainer::Iterator& iter) {
			DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
			const_cast<FiveKeyContainer::Iterator&>(iter).first(lKeyStream);
			remove(lKeyStream);
		}

		inline bool commit(bool sync = false) { return t_.commit(sync); }

	private:
		typename Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::Transaction t_;
	};	

public:
	FiveKeyContainer(const std::string& name) : Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>(name) {}

	inline bool open(bool useTypedComparer = true, uint32_t cache = 0) { return Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::open(useTypedComparer, cache); }
	inline bool repair(bool useTypedComparer = true, uint32_t cache = 0) { return Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::repair(useTypedComparer, cache); }
	inline void close() { Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::close(); }
	inline bool opened() { return Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::opened(); }	

	bool write(const key1& k1, const value& v, bool sync = false) {
		FiveKey<key1, key2, key3, key4, key5> lKey(k1, key2(), key3(), key4(), key5());
		return Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::write(lKey, v, sync);
	}

	bool write(const key1& k1, const key2& k2, const value& v, bool sync = false) {
		FiveKey<key1, key2, key3, key4, key5> lKey(k1, k2, key3(), key4(), key5());
		return Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::write(lKey, v, sync);
	}

	bool write(const key1& k1, const key2& k2, const key3& k3, const value& v, bool sync = false) {
		FiveKey<key1, key2, key3, key4, key5> lKey(k1, k2, k3, key4(), key5());
		return Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::write(lKey, v, sync);
	}

	bool write(const key1& k1, const key2& k2, const key3& k3, const key4& k4, const value& v, bool sync = false) {
		FiveKey<key1, key2, key3, key4, key5> lKey(k1, k2, k3, k4, key5());
		return Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::write(lKey, v, sync);
	}

	bool write(const key1& k1, const key2& k2, const key3& k3, const key4& k4, const key5& k5, const value& v, bool sync = false) {
		FiveKey<key1, key2, key3, key4, key5> lKey(k1, k2, k3, k4, k5);
		return Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::write(lKey, v, sync);
	}

	bool remove(const key1& k1, const key2& k2, const key3& k3, const key4& k4, const key5& k5, bool sync = false) {
		FiveKey<key1, key2, key3, key4, key5> lKey(k1, k2, k3, k4, k5);
		return Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::remove(lKey, sync);
	}

	inline Iterator find(const key1& k1) {
		return FiveKeyContainer::Iterator(k1, Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::find(FiveKey<key1, key2, key3, key4, key5>(k1, key2(), key3(), key4(), key5()))); 
	}

	inline Iterator find(const key1& k1, const key2& k2) {
		return FiveKeyContainer::Iterator(k1, k2, Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::find(FiveKey<key1, key2, key3, key4, key5>(k1, k2, key3(), key4(), key5())));
	}

	inline Iterator find(const key1& k1, const key2& k2, const key3& k3) {
		return FiveKeyContainer::Iterator(k1, k2, k3, Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::find(FiveKey<key1, key2, key3, key4, key5>(k1, k2, k3, key4(), key5())));
	}

	inline Iterator find(const key1& k1, const key2& k2, const key3& k3, const key4& k4) {
		return FiveKeyContainer::Iterator(k1, k2, k3, k4, Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::find(FiveKey<key1, key2, key3, key4, key5>(k1, k2, k3, k4, key5())));
	}

	inline Iterator find(const key1& k1, const key2& k2, const key3& k3, const key4& k4, const key5& k5) {
		return FiveKeyContainer::Iterator(k1, k2, k3, k4, Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::find(FiveKey<key1, key2, key3, key4, key5>(k1, k2, k3, k4, k5)));
	}

	inline Iterator begin() { return FiveKeyContainer::Iterator(Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::begin()); }
	inline Iterator last() { return FiveKeyContainer::Iterator(Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::last()); }

	inline bool remove(const FiveKeyContainer::Iterator& iter, bool sync = false) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		const_cast<FiveKeyContainer::Iterator&>(iter).first(lKeyStream);
		return Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::remove(lKeyStream, sync);
	}

	inline Transaction transaction() { return Transaction(Container<FiveKey<key1, key2, key3, key4, key5>, value, impl>::transaction()); }
};

//
// Entity container for any key and special serialization\desesialization scheme 
template<typename key, typename value, template<typename, typename> typename impl >
class EntityContainer: public Container<key, value, impl> {
public:
	class Iterator {
	public:
		Iterator(const typename Container<key, value, impl>::Iterator& i) : i_(i) {}

		virtual inline bool valid() { return i_.valid(); }

		inline Iterator& operator++() {
			i_.next();
			return *this;
		}

		inline Iterator& operator++(int) {
			i_.next();
			return *this;
		}

		inline Iterator& operator--() {
			i_.prev();
			return *this;
		}

		inline Iterator& operator--(int) {
			i_.prev();
			return *this;
		}

		inline void next() { i_.next(); }
		inline void prev() { i_.prev(); }

		inline bool first(key& k) { return i_.first(k); }
		inline bool first(DataStream& k) { return i_.first(k); }

		inline std::shared_ptr<value> operator* () {
			DataStream lValueStream(SER_DISK, PROTOCOL_VERSION);
			if (i_.second(lValueStream)) {
				return value::Deserializer::deserialize(lValueStream);
			}

			return nullptr;
		}

	private:
		typename Container<key, value, impl>::Iterator i_;
	};
public:
	class Transaction {
	public:
		Transaction(const typename Container<key, value, impl>::Transaction& t) : t_(t) {}

		inline void write(const key& k, std::shared_ptr<value> v) {
			DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
			lKeyStream << k;

			DataStream lValueStream(SER_DISK, PROTOCOL_VERSION);
			value::Serializer::serialize(lValueStream, v);

			t_.write(lKeyStream, lValueStream);
		}

		inline void remove(const key& k) { t_.remove(k); }
		inline bool commit(bool sync = false) { return t_.commit(sync); }

	private:
		typename Container<key, value, impl>::Transaction t_;
	};

public:
	EntityContainer(const std::string& name) : Container<key, value, impl>(name) {}

	inline bool open(bool useTypedComparer = true, uint32_t cache = 0) { return Container<key, value, impl>::open(useTypedComparer, cache); }
	inline bool repair(bool useTypedComparer = true, uint32_t cache = 0) { return Container<key, value, impl>::repair(useTypedComparer, cache); }
	inline void close() { Container<key, value, impl>::close(); }
	inline bool opened() { return Container<key, value, impl>::opened(); }	

	bool write(const key& k, std::shared_ptr<value> v, bool sync = false) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		lKeyStream << k;

		DataStream lValueStream(SER_DISK, PROTOCOL_VERSION);
		value::Serializer::serialize(lValueStream, v);

		return Container<key, value, impl>::write(lKeyStream, lValueStream, sync);
	}

	std::shared_ptr<value> read(const key& k) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		lKeyStream << k;

		DataStream lValueStream(SER_DISK, PROTOCOL_VERSION);
		
		if (Container<key, value, impl>::read(lKeyStream, lValueStream)) {
			return value::Deserializer::deserialize(lValueStream);
		}

		return nullptr;
	}

	inline bool exists(const key& k) {
		//
		key lKey;
		EntityContainer::Iterator lItem(Container<key, value, impl>::find(k));
		return (lItem.valid() && lItem.first(lKey) && lKey == k);
	}

	inline Iterator begin() { return EntityContainer::Iterator(Container<key, value, impl>::begin()); }
	inline Iterator last() { return EntityContainer::Iterator(Container<key, value, impl>::last()); }

	inline Transaction transaction() { return Transaction(Container<key, value, impl>::transaction()); }
};

} // db
} // qbit

#endif