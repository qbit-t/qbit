// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_LOG_H
#define QBIT_LOG_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../allocator.h"

#include <iostream>

#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/function.hpp>

#include "../fs.h"
#include "../timestamp.h"

#include "../tinyformat.h"

namespace qbit {

typedef boost::function<void (uint32_t, const std::string&)> logEchoFunction;

class Log {
public:
	enum Category: uint32_t {
		NONE			= 0,
		INFO			= (1 <<  0),
		WARNING			= (1 <<  1),
		GENERAL_ERROR 	= (1 <<  2),
		DB 				= (1 <<  3),
		POOL			= (1 <<  4),
		WALLET			= (1 <<  5),
		STORE			= (1 <<  6),
		NET				= (1 <<  7),
		VALIDATOR		= (1 <<  8),
		CONSENSUS		= (1 <<  9),
		HTTP			= (1 << 10),
		BALANCE			= (1 << 11),
		SHARDING		= (1 << 12),
		CLIENT			= (1 << 13),
		DEBUG			= (1 << 14),
		ALL				= ~(uint32_t)0
	};

	Log(const std::string& name) : name_ (name) {}

	void write(Category, const std::string&);
	void writeClient(Category, const std::string&);

	void enable(Category category) { categories_ |= category; }
	void disable(Category category) { categories_ &= ~category; }

	bool isEnabled(Category category) { return (categories_.load(std::memory_order_relaxed) & category) != 0; }

	void enableConsole() { console_ = true; }
	void disableConsole() { console_ = false; }

	void enableFile() { file_ = true; }
	void disableFile() { file_ = false; }

	void setEcho(logEchoFunction echo) {
		echoFunction_ = echo;
	}

private:
	std::string timestamp();
	bool open();

private:	
	FILE* out_ = nullptr;
	boost::recursive_mutex mutex_;
	std::atomic_bool startedNewLine_ {true};
	std::atomic<uint32_t> categories_ {0};
	bool console_ = false;
	bool file_ = true;
	logEchoFunction echoFunction_;
	
	std::string name_;
};

extern Log& gLog();
extern Log& gLog(const std::string&);
extern Log::Category getLogCategory(const std::string&);

} // qbit

#endif
