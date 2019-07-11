// Copyright (c) 2016-2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __JM_MUTEX__
#define __JM_MUTEX__

#include "jet_malloc_defs.h"

#ifdef _WIN32
	#include <windows.h>
	#include <process.h>
#elif defined(__linux__)
	#include <sys/types.h>
	#include <sys/syscall.h>
	#include <pthread.h>
#endif

struct _jm_mutex
{
#ifdef _WIN32
	volatile size_t 	owner;
#else
	pthread_mutex_t		lock;
#endif
};

#ifdef _WIN32

void _jm_mutex_init(struct _jm_mutex* mutex);
void _jm_mutex_free(struct _jm_mutex* mutex);
void _jm_mutex_lock(struct _jm_mutex* mutex);
void _jm_mutex_unlock(struct _jm_mutex* mutex);
size_t _jm_get_current_thread_id();

void _jm_interlocked_increment(volatile size_t* number);
void _jm_interlocked_decrement(volatile size_t* number);
void _jm_interlocked_exchange(volatile size_t* number, volatile size_t dest);

#endif

/////////////////////////////////////////
#if defined(_WIN32) && !(defined(_DEBUG))

extern __declspec(thread) size_t _jm_current_thread_id;

JM_INLINE void _jm_interlocked_increment(volatile size_t* number)
{
	InterlockedIncrement(number);
}

JM_INLINE void _jm_interlocked_decrement(volatile size_t* number)
{
	InterlockedDecrement(number);
}

JM_INLINE void _jm_interlocked_exchange(volatile size_t* number, volatile size_t dest)
{
	while(InterlockedExchange(number, dest) != dest);
}

JM_INLINE size_t _jm_get_current_thread_id()
{
	return _jm_current_thread_id = GetCurrentThreadId();
}

JM_INLINE void _jm_mutex_init(struct _jm_mutex* mutex)
{

}

JM_INLINE void _jm_mutex_free(struct _jm_mutex* mutex)
{

}

JM_INLINE void _jm_mutex_lock(struct _jm_mutex* mutex)
{
#ifdef JM_USE_MUTEX
	size_t lThreadId = _jm_get_current_thread_id(), lSpin;
	do
	{
		for(lSpin = 0; lSpin < 20 /*MUTEX_SPIN_COUNT*/; lSpin++)
		{
			if(InterlockedCompareExchange(&(mutex->owner), lThreadId, 0) == lThreadId)
			{
				return;
			}
		}
		Sleep(1);
	}
	while(1);
#endif
}

JM_INLINE void _jm_mutex_unlock(struct _jm_mutex* mutex)
{
#ifdef JM_USE_MUTEX
	size_t lThreadId = _jm_get_current_thread_id();
	while(InterlockedCompareExchange(&(mutex->owner), 0, lThreadId) == lThreadId);
#endif
}
//////////////////////////////////////////////
#elif defined(__linux__) && !(defined(_DEBUG))

extern __thread size_t _jm_current_thread_id;

JM_INLINE void _jm_interlocked_increment(volatile size_t* number)
{
	/*return*/ __sync_fetch_and_add(number, 1) + 1;
}

JM_INLINE void _jm_interlocked_decrement(volatile size_t* number)
{
	/*return*/ __sync_fetch_and_sub(number, 1) - 1;
}

JM_INLINE void _jm_interlocked_exchange(volatile size_t* number, volatile size_t dest)
{
	while(__sync_lock_test_and_set(number, dest) != dest);
}

JM_INLINE size_t _jm_get_current_thread_id()
{
	return _jm_current_thread_id = syscall(__NR_gettid);
}

JM_INLINE void _jm_mutex_init(struct _jm_mutex* mutex)
{
	pthread_mutex_init(&(mutex->lock), 0);
}

JM_INLINE void _jm_mutex_free(struct _jm_mutex* mutex)
{
	pthread_mutex_destroy(&(mutex->lock));
}

JM_INLINE void _jm_mutex_lock(struct _jm_mutex* mutex)
{
#ifdef JM_USE_MUTEX
	pthread_mutex_lock(&(mutex->lock));
#endif
}

JM_INLINE void _jm_mutex_unlock(struct _jm_mutex* mutex)
{
#ifdef JM_USE_MUTEX
	pthread_mutex_unlock(&(mutex->lock));
#endif
}

#endif // _WIN32 | __linux__

#endif
