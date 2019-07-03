
#include "include/jm_utils.h"

#ifdef WIN32
#include <windows.h>
#endif

#if defined(_DEBUG)
longlong_t _jm_get_time()
{
#ifdef WIN32
	FILETIME ft;
	SYSTEMTIME st;
	ULARGE_INTEGER ui;

	GetSystemTime(&st);

	SystemTimeToFileTime(&st, &ft);  // converts to file time format
	ui.LowPart=ft.dwLowDateTime;
	ui.HighPart=ft.dwHighDateTime;

	return ui.QuadPart / 10000;
#elif defined(__linux__)
        struct timeval now;
        gettimeofday(&now, NULL);
        return now.tv_sec * 1000 + now.tv_usec / 1000;
#endif
	return 0;
}

void _jm_now(char* time)
{
#ifdef WIN32
	SYSTEMTIME lSysTime;
	memset(&lSysTime, 0, sizeof(lSysTime));

	GetLocalTime(&lSysTime);

	sprintf(time, "%d-%d-%d %d:%d:%d.%d", lSysTime.wYear, lSysTime.wMonth, lSysTime.wDay, lSysTime.wHour, lSysTime.wMinute, lSysTime.wSecond, lSysTime.wMilliseconds);
#endif
}
#endif

void Now(char* time)
{
	_jm_now(time);
}

longlong_t getTime()
{
	return _jm_get_time();
}
