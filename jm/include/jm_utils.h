// Copyright (c) 2016-2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __JM_UTILS__
#define __JM_UTILS__

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>

#if defined(__linux__)
#include <sys/time.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

//
// time measurements
//

#include "jet_malloc_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

longlong_t _jm_get_time();
void _jm_now(char* time);

#endif

#if !(defined(_DEBUG))
JM_INLINE longlong_t _jm_get_time()
{
#ifdef WIN32
	FILETIME ft;
	ULARGE_INTEGER ui;

	GetSystemTimeAsFileTime(&ft);
	ui.LowPart=ft.dwLowDateTime;
	ui.HighPart=ft.dwHighDateTime;

	return ui.QuadPart / 10000;
#elif defined(__linux__)
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return now.tv_sec * 1000 + now.tv_nsec/1000000;
#endif
	return 0;
}

JM_INLINE void _jm_now(char* time)
{
#ifdef WIN32
	SYSTEMTIME lSysTime;
	memset(&lSysTime, 0, sizeof(lSysTime));

	GetLocalTime(&lSysTime);

	sprintf(time, "%d-%d-%d %d:%d:%d.%d", lSysTime.wYear, lSysTime.wMonth, lSysTime.wDay, lSysTime.wHour, lSysTime.wMinute, lSysTime.wSecond, lSysTime.wMilliseconds);
#else
	*time=0;
#endif
}
#endif

JET_MALLOC_DEF void Now(char*);
JET_MALLOC_DEF longlong_t getTime();

#ifdef __cplusplus
} // __cplusplus
#endif

#define BEGIN_MEASURE()\
{ \
	char t1[64] = {0}; Now(t1); \
	longlong_t lBeginTime = getTime(); \

#define END_MEASURE()\
	char t2[64] = {0}; Now(t2); \
	longlong_t lEndTime = getTime(); \

#define PRINT_MEASURE(a,b,c,e)\
	printf("\n%llu, %d, %d, %d, %d\n", (lEndTime-lBeginTime), a, b, c, e); \
}\

#endif

