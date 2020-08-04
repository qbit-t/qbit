// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_LEVELDB_H
#define QBIT_LEVELDB_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../allocator.h"

#include "leveldb/db.h"
#include "leveldb/cache.h"
#include "leveldb/filter_policy.h"
#include "leveldb/env.h"
#include "leveldb/status.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"
#include "leveldb/comparator.h"

#include "db.h"
#include "../log/log.h"
#include "../streams.h"

namespace qbit {
namespace db {

class exception : public std::runtime_error
{
public:
	explicit exception(const std::string& msg) : std::runtime_error(msg) {}
};

#ifndef MOBILE_PLATFORM
class LevelDBLogger: public leveldb::Logger {
public:
	void Logv(const char * format, va_list ap);
};
#endif

#ifndef MOBILE_PLATFORM
template<typename key, typename value>
class LevelDBComparator: public leveldb::Comparator {
public:
	int Compare(const leveldb::Slice& left, const leveldb::Slice& right) const {
		//
		key lLeftKey;
		DataStream lLeftStream(SER_DISK, PROTOCOL_VERSION);
		lLeftStream.insert(lLeftStream.end(), left.data(), left.data() + left.size());
		lLeftStream >> lLeftKey;

		key lRightKey;
		DataStream lRightStream(SER_DISK, PROTOCOL_VERSION);
		lRightStream.insert(lRightStream.end(), right.data(), right.data() + right.size());
		lRightStream >> lRightKey;

		if (lLeftKey < lRightKey) return -1;
		if (lLeftKey > lRightKey) return 1;
		return 0;
	}

	const char* Name() const { return "BacisComparator"; }
	void FindShortestSeparator(std::string*, const leveldb::Slice&) const {}
	void FindShortSuccessor(std::string*) const {}
};
#endif

template<typename key, typename value>
class LevelDBContainer {
public:
	class _iterator {
	public:
		_iterator() {}
		explicit _iterator(leveldb::Iterator* iterator) { iterator_ = std::shared_ptr<leveldb::Iterator>(iterator); }
		_iterator(const _iterator& other) : iterator_(other.iterator_) {}
		~_iterator() {}

		bool valid() { return (iterator_ && iterator_->Valid()); }
		void next() { iterator_->Next(); }
		void prev() { iterator_->Prev(); }

		bool first(key& k) {
			DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
			if(first(lKeyStream)) {
				lKeyStream >> k;
				return true;
			}
			return false;
		}

		bool first(DataStream& k) {
			if (!iterator_) return false;

			try {
				leveldb::Slice lKey = iterator_->key();
				k.insert(k.end(), lKey.data(), lKey.data() + lKey.size());
			}
			catch (const std::exception&) {
				return false;
			}
			return true;
		}

		bool second(value& v) {
			DataStream lValueStream(SER_DISK, PROTOCOL_VERSION);
			if(second(lValueStream)) {
				lValueStream >> v;
				return true;
			}
			return false;
		}

		bool second(DataStream& v) {
			if (!iterator_) return false;

			try {
				leveldb::Slice lValue = iterator_->value();
				v.insert(v.end(), lValue.data(), lValue.data() + lValue.size());
			}
			catch (const std::exception&) {
				return false;
			}
			return true;
		}

	private:
		std::shared_ptr<leveldb::Iterator> iterator_ { nullptr };
	};

	class _transaction {
	public:
		_transaction() {}
		explicit _transaction(std::shared_ptr<leveldb::DB> db, const std::string& name) : db_(db), name_(name) {}

		void write(const DataStream& k, const DataStream& v) {
			leveldb::Slice lKey(k.data(), k.size());
			leveldb::Slice lValue(v.data(), v.size());

			batch_.Put(lKey, lValue);
		}

		void write(const key& k, const value& v) {
			DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
			lKeyStream << k;

			DataStream lValueStream(SER_DISK, PROTOCOL_VERSION);
			lValueStream << v;

			write(lKeyStream, lValueStream);
		}

		void remove(const DataStream& k) {
			leveldb::Slice lKey(k.data(), k.size());
			batch_.Delete(lKey);
		}

		void remove(const key& k) {
			DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
			lKeyStream << k;

			remove(lKeyStream);
		}

		bool commit(bool sync = false) {
			leveldb::WriteOptions lOptions;
			lOptions.sync = sync;

			leveldb::Status lStatus = db_->Write(lOptions, &batch_);

			if (!lStatus.ok()) error(lStatus);

			return true;
		}

	private:
		void error(const leveldb::Status& status) {
			const std::string lMessage = std::string("[Db/") + name_ + std::string("]: ") + status.ToString();
			gLog().write(Log::ERROR, lMessage);
			throw db::exception(lMessage);      
		}

	private:
		leveldb::WriteBatch batch_;
		std::shared_ptr<leveldb::DB> db_;
		std::string name_;
	};

public:
	LevelDBContainer() {}
	~LevelDBContainer() {}

	bool open(const std::string& name, bool useTypedComparer = true, uint32_t cache = 0) { 
		if (db_ == nullptr) {
			name_ = name;

			if (cache) {
				options_.block_cache = leveldb::NewLRUCache(cache / 2);
				options_.write_buffer_size = cache / 4;
			}

			options_.filter_policy = leveldb::NewBloomFilterPolicy(10);
			options_.compression = leveldb::kNoCompression;
			options_.create_if_missing = true;

#ifndef MOBILE_PLATFORM
			options_.info_log = new LevelDBLogger();
			if (useTypedComparer) options_.comparator = &defaultComparator;
#endif
			gLog().write(Log::DB, std::string("[leveldb]: Opening container ") + name_);

			leveldb::DB* lDB;
			leveldb::Status lStatus = leveldb::DB::Open(options_, name_, &lDB);
			if (!lStatus.ok()) error(lStatus);

			db_ = std::shared_ptr<leveldb::DB>(lDB);

			gLog().write(Log::DB, std::string("[leveldb]: Opened container ") + name_);
		}

		return true;
	}

	bool opened() { return db_ != nullptr; }

	void close() {
		db_ = nullptr; // release
	}

	_transaction transaction() {
		return _transaction(db_, name_);
	}

	_iterator begin() {
		leveldb::ReadOptions lOptions;
		lOptions.verify_checksums = true;

		leveldb::Iterator* lIterator = db_->NewIterator(lOptions);

		try {
			lIterator->SeekToFirst();	
		}
		catch(std::exception&) {
			delete lIterator;
			lIterator = nullptr;
		}

		return _iterator(lIterator);
	}

	_iterator last() {
		leveldb::ReadOptions lOptions;
		lOptions.verify_checksums = true;

		leveldb::Iterator* lIterator = db_->NewIterator(lOptions);

		try {
			lIterator->SeekToLast();	
		}
		catch(std::exception&) {
			delete lIterator;
			lIterator = nullptr;
		}

		return _iterator(lIterator);
	}

	_iterator find(const DataStream& k) {
		leveldb::ReadOptions lOptions;
		lOptions.verify_checksums = true;

		leveldb::Iterator* lIterator = db_->NewIterator(lOptions);

		try {

			leveldb::Slice lKey(k.data(), k.size());
			lIterator->Seek(lKey);	
		}
		catch(std::exception&) {
			delete lIterator;
			lIterator = nullptr;
		}

		return _iterator(lIterator);
	}

	_iterator find(const key& k) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		lKeyStream << k;

		return find(lKeyStream);
	}

	bool write(const DataStream& k, const DataStream& v, bool sync = false) {
		leveldb::Slice lKey(k.data(), k.size());
		leveldb::Slice lValue(v.data(), v.size());

		leveldb::WriteOptions lOptions;
		lOptions.sync = sync;

		leveldb::WriteBatch lBatch;
		lBatch.Put(lKey, lValue);

		leveldb::Status lStatus = db_->Write(lOptions, &lBatch);

		if (!lStatus.ok()) error(lStatus);

		return true;
	}

	bool write(const key& k, const value& v, bool sync = false) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		lKeyStream << k;

		DataStream lValueStream(SER_DISK, PROTOCOL_VERSION);
		lValueStream << v;

		return write(lKeyStream, lValueStream, sync);
	}

	bool read(const DataStream& k, DataStream& v) {
		leveldb::ReadOptions lOptions;
		lOptions.verify_checksums = true;

		leveldb::Slice lKey(k.data(), k.size());

		std::string lValue;
		leveldb::Status lStatus = db_->Get(lOptions, lKey, &lValue);
		if (!lStatus.ok()) { if (lStatus.IsNotFound()) return false; error(lStatus); }

		try {
			v.insert(v.end(), lValue.data(), lValue.data() + lValue.size());
		} 
		catch (const std::exception&) {
			return false;
		}

		return true;
	}

	bool read(const key& k, value& v) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		lKeyStream << k;

		DataStream lValueStream(SER_DISK, PROTOCOL_VERSION);
		if(read(lKeyStream, lValueStream)) {
			lValueStream >> v;
			return true;
		}

		return false;
	}

	bool remove(const DataStream& k, bool sync = false) {
		leveldb::Slice lKey(k.data(), k.size());

		leveldb::WriteOptions lOptions;
		lOptions.sync = sync;

		leveldb::WriteBatch lBatch;
		lBatch.Delete(lKey);

		leveldb::Status lStatus = db_->Write(lOptions, &lBatch);

		if (!lStatus.ok()) { if (lStatus.IsNotFound()) return false; error(lStatus); }

		return true;
	}

	bool remove(const key& k, bool sync = false) {
		DataStream lKeyStream(SER_DISK, PROTOCOL_VERSION);
		lKeyStream << k;

		return remove(lKeyStream, sync);
	}

private:
	void error(const leveldb::Status& status) {
		const std::string lMessage = std::string("[Db/") + name_ + std::string("]: ") + status.ToString();
		gLog().write(Log::ERROR, lMessage);
		throw db::exception(lMessage);      
	}

	std::string name_;

private:
#ifndef MOBILE_PLATFORM
	LevelDBComparator<key, value> defaultComparator;
#endif
	std::shared_ptr<leveldb::DB> db_ {nullptr};
	leveldb::Options options_;
};

} // db
} // qbit

#endif
