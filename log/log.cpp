#include "log.h"
#include "../tinyformat.h"

#include <iomanip>
#include <boost/thread.hpp>

using namespace qbit;

static qbit::Log* pgLog = nullptr;

qbit::Log& qbit::gLog() {
	if (!pgLog) pgLog = new qbit::Log("debug.log");
	return *pgLog;
}

qbit::Log& qbit::gLog(const std::string& name) {
	pgLog = new qbit::Log(name);
	return *pgLog;
}

std::string _getLogCategoryText(Log::Category category) {
	switch(category) {
		case Log::Category::INFO: 			return "info  ";
		case Log::Category::WARNING: 		return "warn  ";
		case Log::Category::GENERAL_ERROR: 	return "error ";
		case Log::Category::DB: 			return "db    ";
		case Log::Category::POOL: 			return "pool  ";
		case Log::Category::WALLET: 		return "wallet";
		case Log::Category::STORE: 			return "store ";
		case Log::Category::NET: 			return "net   ";
		case Log::Category::VALIDATOR:		return "val   ";
		case Log::Category::CONSENSUS:		return "cons  ";
		case Log::Category::HTTP:			return "http  ";
		case Log::Category::BALANCE:		return "bal   ";
		case Log::Category::SHARDING:		return "shard ";
		case Log::Category::CLIENT:			return "client";
		case Log::Category::DEBUG:			return "debug ";
		case Log::Category::ALL: 			return "*     ";
	}

	return "ECAT";
}

Log::Category qbit::getLogCategory(const std::string& category) {
	if (category == "info") return Log::Category::INFO;
	else if (category == "warn")	return Log::Category::WARNING;
	else if (category == "error")	return Log::Category::GENERAL_ERROR;
	else if (category == "db")		return Log::Category::DB;
	else if (category == "pool")	return Log::Category::POOL;
	else if (category == "wallet")	return Log::Category::WALLET;
	else if (category == "store")	return Log::Category::STORE;
	else if (category == "net")		return Log::Category::NET;

	else if (category == "val")		return Log::Category::VALIDATOR;
	else if (category == "cons")	return Log::Category::CONSENSUS;
	else if (category == "http")	return Log::Category::HTTP;

	else if (category == "bal")		return Log::Category::BALANCE;
	else if (category == "shard")	return Log::Category::SHARDING;
	else if (category == "client")	return Log::Category::CLIENT;

	else if (category == "debug")	return Log::Category::DEBUG;
	return Log::Category::ALL;
}

void qbit::Log::write(Log::Category category, const std::string& str) {
	if (isEnabled(category)) {
		//
		if (open()) {
			boost::unique_lock<boost::recursive_mutex> lLock(mutex_);

			uint64_t lMicroSeconds = getMicroseconds();
			std::string lMessage = 
				strprintf("[%-12d][%s.%06d][%s]%s", boost::this_thread::get_id(), 
					formatISO8601DateTime(lMicroSeconds / 1000000), lMicroSeconds % 1000000, _getLogCategoryText(category), str);

			if (echoFunction_) echoFunction_(category, lMessage);

			if (lMessage.at(lMessage.length()-1) != '\n') lMessage += "\n";
			if (console_) std::cout << lMessage;

			if (file_) {
				fwrite(lMessage.data(), 1, lMessage.size(), out_);
			}
		} else {
			if (echoFunction_) {
				echoFunction_(category, str);
			}
		}
	}
}

void qbit::Log::writeClient(Log::Category category, const std::string& str) {
	if (isEnabled(category)) {
		if (open()) {
			boost::unique_lock<boost::recursive_mutex> lLock(mutex_);

			uint64_t lMicroSeconds = getMicroseconds();
			std::string lMessage = 
				strprintf("[%-12d][%s.%06d][%s]%s", boost::this_thread::get_id(), 
					formatISO8601DateTime(lMicroSeconds / 1000000), lMicroSeconds % 1000000, _getLogCategoryText(category), str);

			std::string lConsoleMessage = 
				strprintf("[%-12d]%s", _getLogCategoryText(category), str);

			if (echoFunction_) echoFunction_(category, lMessage);	

			if (lMessage.at(lMessage.length()-1) != '\n') { lMessage += "\n"; lConsoleMessage += "\n"; }
			std::cout << lConsoleMessage;

			if (file_) {
				fwrite(lMessage.data(), 1, lMessage.size(), out_);
			}
		} else {
			if (echoFunction_) {
				echoFunction_(category, str);
			}
		}
	}
}

std::string qbit::Log::timestamp() {
	return std::string();
}

bool qbit::Log::open() {
	if (out_) return true;

	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);

	out_ = fsbridge::fopen(name_, "a");
	if (!out_) return false;

	setbuf(out_, nullptr); // unbuffered

	return true;
}
