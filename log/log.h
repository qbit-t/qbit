// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_LOG_H
#define QBIT_LOG_H

#include <iostream>
#include <atomic>
#include <mutex>

#include "../fs.h"

namespace qbit {

class Log {
public:
	enum Category : uint32_t {
		NONE	= 0,
		INFO	= (1 <<  0),
		WARNING	= (1 <<  1),
		ERROR	= (1 <<  2),
		DB 		= (1 <<  3),
		POOL	= (1 <<  4),
		WALLET	= (1 <<  5),
		STORE	= (1 <<  6),
		ALL		= ~(uint32_t)0
	};

	Log(const std::string& name) : name_ (name) {}

	void write(Category, const std::string&);

	void enable(Category category) { categories_ |= category; }
	void disable(Category category) { categories_ &= ~category; }

	bool isEnabled(Category category) { return (categories_.load(std::memory_order_relaxed) & category) != 0; }

private:
	std::string timestamp();
	bool open();

private:	
	FILE* out_ = nullptr;
	std::mutex mutex_;
	std::atomic_bool startedNewLine_ {true};
	std::atomic<uint32_t> categories_ {0};
	
	std::string name_;
};

extern Log& gLog();

} // qbit

#endif
