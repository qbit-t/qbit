// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_LEVELDB_H
#define QBIT_LEVELDB_H

#include "leveldb/db.h"
#include "leveldb/cache.h"
#include "leveldb/filter_policy.h"
#include "leveldb/env.h"
#include "leveldb/status.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"
#include "helpers/memenv/memenv.h"

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

class LevelDBLogger: public leveldb::Logger {
public:
	void Logv(const char * format, va_list ap);
};

template<typename key, typename value>
class LevelDBContainer {
public:
	class _iterator {
	public:
		_iterator() {}
		explicit _iterator(leveldb::Iterator* iterator) { iterator_ = std::shared_ptr<leveldb::Iterator>(iterator); valid(); }
		_iterator(const _iterator& other) : iterator_(other.iterator_) { valid(); }
		~_iterator() {}

		bool valid() { return (iterator_ && iterator_->Valid()); }
		void next() { iterator_->Next(); }

		bool first(key& k) {
			DataStream lKeyStream(SER_DISK, CLIENT_VERSION);
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
			DataStream lValueStream(SER_DISK, CLIENT_VERSION);
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

public:
	LevelDBContainer() {}
	~LevelDBContainer() { if (db_) delete db_; }

	bool open(const std::string& name, uint32_t cache = 0) { 
		if (db_ == nullptr) {
			name_ = name;

			if (cache) {
				options_.block_cache = leveldb::NewLRUCache(cache / 2);
				options_.write_buffer_size = cache / 4;
			}

			options_.filter_policy = leveldb::NewBloomFilterPolicy(10);
			options_.compression = leveldb::kNoCompression;
			options_.info_log = new LevelDBLogger();
			options_.create_if_missing = true;

			gLog().write(Log::INFO, std::string("[LevelDBContainer]: Opening ") + name_);

			leveldb::Status lStatus = leveldb::DB::Open(options_, name_, &db_);
			if (!lStatus.ok()) error(lStatus);

			gLog().write(Log::INFO, std::string("[LevelDBContainer]: Opened ") + name_);
		}

		return true;
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
		DataStream lKeyStream(SER_DISK, CLIENT_VERSION);
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
		DataStream lKeyStream(SER_DISK, CLIENT_VERSION);
		lKeyStream << k;

		DataStream lValueStream(SER_DISK, CLIENT_VERSION);
		lValueStream << v;

		return write(lKeyStream, lValueStream, sync);
	}

	bool read(const DataStream& k, DataStream& v) {
		leveldb::ReadOptions lOptions;
		lOptions.verify_checksums = true;

		leveldb::Slice lKey(k.data(), k.size());

		std::string lValue;
		leveldb::Status lStatus = db_->Get(lOptions, lKey, &lValue);
		if (!lStatus.ok()) error(lStatus);

		try {
			v.insert(v.end(), lValue.data(), lValue.data() + lValue.size());
		} 
		catch (const std::exception&) {
			return false;
		}

		return true;
	}

	bool read(const key& k, value& v) {
		DataStream lKeyStream(SER_DISK, CLIENT_VERSION);
		lKeyStream << k;

		DataStream lValueStream(SER_DISK, CLIENT_VERSION);
		if(read(lKeyStream, lValueStream)) {
			lValueStream >> v;
			return true;
		}

		return false;
	}

private:
	void error(const leveldb::Status& status) {
		const std::string lMessage = "[LevelDBContainer/error]: " + status.ToString();
		throw db::exception(lMessage);      
	}

	std::string name_;

private:
	leveldb::DB* db_ {nullptr};
	leveldb::Options options_;
};

} // db
} // qbit

#endif