#include "timestamp.h"
#include "tinyformat.h"
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

boost::atomic<uint64_t> qbit::gMedianTime(0);
boost::atomic<uint64_t> qbit::gMedianMicroseconds(0);

uint64_t qbit::getMedianMicroseconds() {
	uint64_t lMicroseconds = qbit::getMicroseconds();
	uint64_t lMedianMicroseconds = gMedianMicroseconds.load(boost::memory_order_relaxed); 
	if (!lMedianMicroseconds) lMedianMicroseconds = qbit::getMicroseconds();

	return (lMicroseconds + lMedianMicroseconds)/2;
}

uint64_t qbit::getMedianTime() {
	uint64_t lTime = qbit::getTime();
	uint64_t lMedianTime = (gMedianTime == 0 ? qbit::getTime() : gMedianTime.load(boost::memory_order_relaxed));

	return (lTime + lMedianTime)/2; 
}

uint64_t qbit::getMicroseconds() {
#ifdef _WIN32
		FILETIME lFt;
		// Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC)
		GetSystemTimeAsFileTime(&lFt);
		uint64_t lMicroseconds = static_cast<uint64_t>(lFt.dwHighDateTime) << 32 | lFt.dwLowDateTime;
		lMicroseconds -= 116444736000000000LL;
    	return lMicroseconds / 10; // interval in microseconds
#else //if defined(__linux__)
		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		uint64_t lMicroseconds = ((uint64_t)now.tv_sec) * 1000000;
		lMicroseconds += ((uint64_t)now.tv_nsec)/1000;
		return lMicroseconds;
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
#ifdef _WIN32
	_gmtime64_s(&ts, &time_val);
#else
	gmtime_r(&time_val, &ts);
#endif

	return strprintf("%04i-%02i-%02iT%02i:%02i:%02iZ", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
}

std::string qbit::formatISO8601Date(int64_t nTime) {
	struct tm ts;
	time_t time_val = nTime;
#ifdef _WIN32
	_gmtime64_s(&ts, &time_val);
#else
	gmtime_r(&time_val, &ts);
#endif

	return strprintf("%04i-%02i-%02i", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday);
}

std::string qbit::formatLocalDateTime(int64_t nTime) {
	struct tm ts;
	time_t time_val = nTime;
#ifdef _WIN32
	_localtime64_s(&ts, &time_val);
#else
	localtime_r(&time_val, &ts);
#endif
	return strprintf("%02i:%02i %02i/%02i/%04i", ts.tm_hour, ts.tm_min, ts.tm_mday, ts.tm_mon + 1, ts.tm_year + 1900);
}

std::string qbit::formatLocalDateTimeLong(int64_t nTime) {
	struct tm ts;
	time_t time_val = nTime;
#ifdef _WIN32
	_localtime64_s(&ts, &time_val);
#else
	localtime_r(&time_val, &ts);
#endif
	return strprintf("%02i:%02i:%02i %02i/%02i/%04i", ts.tm_hour, ts.tm_min, ts.tm_sec, ts.tm_mday, ts.tm_mon + 1, ts.tm_year + 1900);
}
