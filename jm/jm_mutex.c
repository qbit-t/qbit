//
// stubbed
//

#include "include/jm_mutex.h"

#ifdef _WIN32
__declspec(thread) size_t _jm_current_thread_id = 0;
#endif

#if defined(__linux__)
__thread size_t _jm_current_thread_id;
#endif

#if defined(_WIN32) && defined(_DEBUG)
void _jm_interlocked_increment(volatile size_t* number)
{
	InterlockedIncrement(number);
}

void _jm_interlocked_decrement(volatile size_t* number)
{
	InterlockedDecrement(number);
}

void _jm_interlocked_exchange(volatile size_t* number, volatile size_t dest)
{
	while(InterlockedExchange(number, dest) != dest);
}

size_t _jm_get_current_thread_id()
{
	return _jm_current_thread_id = GetCurrentThreadId();
}

void _jm_mutex_init(struct _jm_mutex* mutex)
{

}

JM_INLINE void _jm_mutex_free(struct _jm_mutex* mutex)
{

}

void _jm_mutex_lock(struct _jm_mutex* mutex)
{
#ifdef JM_USE_MUTEX
	size_t lThreadId = _jm_get_current_thread_id(), lSpin;
	if(InterlockedCompareExchange(&(mutex->owner), lThreadId, 0) == lThreadId) return;
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

void _jm_mutex_unlock(struct _jm_mutex* mutex)
{
#ifdef JM_USE_MUTEX
	size_t lThreadId = _jm_get_current_thread_id();
	while(InterlockedCompareExchange(&(mutex->owner), 0, lThreadId) == lThreadId);
#endif
}
#elif defined(__linux__) && defined(_DEBUG)
void _jm_interlocked_increment(volatile size_t* number)
{
        /*return*/ __sync_fetch_and_add(number, 1) + 1;
}

void _jm_interlocked_decrement(volatile size_t* number)
{
        /*return*/ __sync_fetch_and_sub(number, 1) - 1;
}

void _jm_interlocked_exchange(volatile size_t* number, volatile size_t dest)
{
        while(__sync_lock_test_and_set(number, dest) != dest);
}

size_t _jm_get_current_thread_id()
{
        return _jm_current_thread_id = syscall(__NR_gettid);
}

void _jm_mutex_init(struct _jm_mutex* mutex)
{
        pthread_mutex_init(&(mutex->lock), 0);
}

void _jm_mutex_free(struct _jm_mutex* mutex)
{
	pthread_mutex_destroy(&(mutex->lock));
}

void _jm_mutex_lock(struct _jm_mutex* mutex)
{
#ifdef JM_USE_MUTEX
        if(!mutex->initialized)
        {
                pthread_mutexattr_t lAttr;
                pthread_mutexattr_init(&lAttr);
                pthread_mutexattr_settype(&lAttr, PTHREAD_MUTEX_RECURSIVE);
                pthread_mutex_init(&(mutex->lock), &lAttr);
                pthread_mutexattr_destroy(&lAttr);
		mutex->initialized = 1;
        }

        pthread_mutex_lock(&(mutex->lock));
#endif
}

void _jm_mutex_unlock(struct _jm_mutex* mutex)
{
#ifdef JM_USE_MUTEX
        pthread_mutex_unlock(&(mutex->lock));
#endif
}

#endif
