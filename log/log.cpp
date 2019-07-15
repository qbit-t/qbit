#include "log.h"
#include <iomanip>

using namespace qbit;

qbit::Log& qbit::gLog() {

	static qbit::Log* pgLog = new qbit::Log("debug.log");
	return *pgLog;
}

std::string _getLogCategoryText(Log::Category category) {
	switch(category) {
		case Log::Category::INFO: 		return "info ";
		case Log::Category::WARNING: 	return "warn ";
		case Log::Category::ERROR: 		return "error";
		case Log::Category::DB: 		return "db   ";
		case Log::Category::ALL: 		return "*    ";
	}

	return "ECAT";
}

void qbit::Log::write(Log::Category category, const std::string& str) {
	if (isEnabled(category)) {
		if (open()) {
			// TODO: fix
			// TODO: timestamp
			std::string lMessage = std::string("[") + _getLogCategoryText(category) + std::string("]") + str;
			if (lMessage.at(lMessage.length()-1) != '\n') lMessage += "\n";
			fwrite(lMessage.data(), 1, lMessage.size(), out_);
		}
	}
}

std::string qbit::Log::timestamp() {
	return std::string();
}

bool qbit::Log::open() {
	if (out_) return true;

	std::lock_guard<std::mutex> scoped_lock(mutex_);

	out_ = fsbridge::fopen(name_, "a");
	if (!out_) return false;

	setbuf(out_, nullptr); // unbuffered

	return true;
}