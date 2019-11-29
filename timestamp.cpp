#include "timestamp.h"
#include "tinyformat.h"
#include <ctime>

uint64_t qbit::getMicroseconds() {
#ifdef WIN32
		FILETIME ft;
		ULARGE_INTEGER ui;

		GetSystemTimeAsFileTime(&ft);
		ui.LowPart=ft.dwLowDateTime;
		ui.HighPart=ft.dwHighDateTime;

		return ui.QuadPart / 10;
#elif defined(__linux__)
		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		return now.tv_sec * 1000000 + now.tv_nsec/1000;
#endif
		return 0;
}

uint64_t qbit::getTime() {
	time_t lNow = time(nullptr);
	return lNow;
}

std::string qbit::formatISO8601DateTime(int64_t nTime) {
	struct tm ts;
	time_t time_val = nTime;
#ifdef _MSC_VER
	gmtime_s(&ts, &time_val);
#else
	gmtime_r(&time_val, &ts);
#endif
	return strprintf("%04i-%02i-%02iT%02i:%02i:%02iZ", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
}

std::string qbit::formatISO8601Date(int64_t nTime) {
	struct tm ts;
	time_t time_val = nTime;
#ifdef _MSC_VER
	gmtime_s(&ts, &time_val);
#else
	gmtime_r(&time_val, &ts);
#endif
	return strprintf("%04i-%02i-%02i", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday);
}
