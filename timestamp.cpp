#include "timestamp.h"

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
