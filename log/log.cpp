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
		case Log::Category::INFO: 		return "info  ";
		case Log::Category::WARNING: 	return "warn  ";
		case Log::Category::ERROR: 		return "error ";
		case Log::Category::DB: 		return "db    ";
		case Log::Category::POOL: 		return "pool  ";
		case Log::Category::WALLET: 	return "wallet";
		case Log::Category::STORE: 		return "store ";
		case Log::Category::NET: 		return "net   ";
		case Log::Category::VALIDATOR:	return "val   ";
		case Log::Category::CONSENSUS:	return "cons  ";
		case Log::Category::HTTP:		return "http  ";
		case Log::Category::BALANCE:	return "bal   ";
		case Log::Category::SHARDING:	return "shard ";
		case Log::Category::ALL: 		return "*     ";
	}

	return "ECAT";
}

void qbit::Log::write(Log::Category category, const std::string& str) {
	if (isEnabled(category)) {
		if (open()) {
			uint64_t lMicroSeconds = getMicroseconds();
			std::string lMessage = 
				strprintf("[%d][%s.%06d][%s]%s", boost::this_thread::get_id(), 
					formatISO8601DateTime(lMicroSeconds / 1000000), lMicroSeconds % 1000000, _getLogCategoryText(category), str);

			if (lMessage.at(lMessage.length()-1) != '\n') lMessage += "\n";
			if (console_) std::cout << lMessage;

			{
				boost::unique_lock<boost::mutex> lLock(mutex_);
				fwrite(lMessage.data(), 1, lMessage.size(), out_);
			}
		}
	}
}

std::string qbit::Log::timestamp() {
	return std::string();
}

bool qbit::Log::open() {
	if (out_) return true;

	boost::unique_lock<boost::mutex> lLock(mutex_);

	out_ = fsbridge::fopen(name_, "a");
	if (!out_) return false;

	setbuf(out_, nullptr); // unbuffered

	return true;
}