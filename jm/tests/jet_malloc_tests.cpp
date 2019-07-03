//#undef JET_MALLOC_EXPORT

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "../include/jet_malloc.h"
#include "../include/jm_utils.h"

#if defined(__linux__)
	#include <pthread.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/syscall.h>
#endif

#ifdef TEST_JEMALLOC
#include "jemalloc.h"
#pragma comment(lib, "x64/release/jemalloc.lib")
#endif

#ifdef _WIN32
#pragma comment(lib, "../../lib/jet_malloc.lib")
#endif

#define POINTERS_1 5000
#define ITERATIONS_1 10000000

enum _allocator
{
	none,
	cruntime,
	jemalloc,
	jet_malloc
};

#include <string.h>

void test1(int size1, char* allocator)
{
	// init
	char* a = (char*) jm_malloc(2); jm_free(a);
	char* b = (char*) jm_malloc(4); jm_free(b);
	char* c = (char*) jm_malloc(8); jm_free(c);
	char* d = (char*) jm_malloc(16); jm_free(d);
	char* e = (char*) jm_malloc(32); jm_free(e);
	char* f = (char*) jm_malloc(64); jm_free(f);
	char* g = (char*) jm_malloc(128); jm_free(g);
	char* h = (char*) jm_malloc(256); jm_free(h);
	char* i = (char*) jm_malloc(512); jm_free(i);
	char* j = (char*) jm_malloc(1024); jm_free(j);
	char* k = (char*) jm_malloc(2048); jm_free(k);
	char* l = (char*) jm_malloc(4096); jm_free(l);

	// stats buffer
	char stats[10240] = {0};
	_allocator al = none;

	if (!strcmp(allocator, "cruntime")) al = cruntime;
	else if (!strcmp(allocator, "jemalloc")) al = jemalloc;
	else if (!strcmp(allocator, "jet_malloc")) al = jet_malloc;

	BEGIN_MEASURE();

	char* pointers[POINTERS_1+1] = {0};

	for (int i=0; i < ITERATIONS_1; i++)
	{
		size_t size = rand() % size1;

		int idx = rand() % POINTERS_1;

		if (!pointers[idx])
		{
			if (al == jet_malloc) pointers[idx] = (char*)jm_malloc(size);
#ifdef TEST_JEMALLOC
			else if (al == jemalloc) pointers[idx] = (char*)je_malloc(size);
#endif
			else if (al == cruntime) pointers[idx] = (char*)malloc(size);
		}
		else
		{
			if (al == jet_malloc) jm_free(pointers[idx]);
#ifdef TEST_JEMALLOC
			else if (al == jemalloc) je_free(pointers[idx]);
#endif
			else if (al == cruntime) free(pointers[idx]);

			pointers[idx] = 0;
		}
	}

#if defined(__linux__)
	char path[] = ".";
	_jm_thread_dump_chunk(syscall(__NR_gettid), 0, 5, path);
#endif

	for (int i=0; i < POINTERS_1; i++)
	{
		if (pointers[i])
		{
			if (al == jet_malloc) jm_free(pointers[i]);
#ifdef TEST_JEMALLOC
			else if (al == jemalloc) je_free(pointers[i]);
#endif
			else if (al == cruntime) free(pointers[i]);
		}
	}

	END_MEASURE();

	_jm_arena_print_stats(_jm_get_current_arena(), stats, JM_ARENA_BASIC_STATS /*| JM_ARENA_CHUNK_STATS*/, JM_ALLOC_CLASSES);
	printf("%s", stats);

	PRINT_MEASURE(0, 0, 0, 0);

	_jm_free_arena();
}

#ifdef _WIN32
#include <process.h>
#endif

int THREADS = 4;

struct _thread_info
{
	_allocator allocator;
	int class_index;
	int block_size;
	int init_mem;
	int finished;
	int print_stat;
};

struct _thread_info thread_info[128] = { {none, 0, 0, 0, 0, 0} };

void
#if defined(__linux__)
*
#endif
test2_thread(void* arg)
{
	char stats[10240] = {0};
	char* pointers[POINTERS_1+1] = {0};

	//std::minstd_rand generator;
	//std::ranlux24_base generator;

	for (int i=0; i < ITERATIONS_1; i++)
	{
		size_t size = /*generator()*/ rand() % ((struct _thread_info*)arg)->block_size; 

		int idx = rand() % POINTERS_1;

		if (!pointers[idx])
		{
			if (((struct _thread_info*)arg)->allocator == jet_malloc) pointers[idx] = (char*)jm_malloc(size);
#ifdef TEST_JEMALLOC
			else if (((struct _thread_info*)arg)->allocator == jemalloc) pointers[idx] = (char*)je_malloc(size);
#endif
			else if (((struct _thread_info*)arg)->allocator == cruntime) pointers[idx] = (char*)malloc(size);
			
			if (((struct _thread_info*)arg)->init_mem) memset(pointers[idx], 1, size);
		}
		else
		{
			if ((rand() % 3) == 1)
			{
				if (((struct _thread_info*)arg)->allocator == jet_malloc) pointers[idx] = (char*)jm_realloc(pointers[idx], size); 
#ifdef TEST_JEMALLOC
				else if (((struct _thread_info*)arg)->allocator == jemalloc) pointers[idx] = (char*)je_realloc(pointers[idx], size);
#endif
				else if (((struct _thread_info*)arg)->allocator == cruntime) pointers[idx] = (char*)realloc(pointers[idx], size);

				if (((struct _thread_info*)arg)->init_mem) memset(pointers[idx], 2, size);
			}
			else
			{
				if (((struct _thread_info*)arg)->allocator == jet_malloc) jm_free(pointers[idx]);
#ifdef TEST_JEMALLOC
				else if (((struct _thread_info*)arg)->allocator == jemalloc) je_free(pointers[idx]);
#endif
				else if (((struct _thread_info*)arg)->allocator == cruntime) free(pointers[idx]);
			
				pointers[idx] = 0;
			}
		}
	}

	for (int i=0; i < POINTERS_1; i++)
	{
		if (pointers[i])
		{
			if (((struct _thread_info*)arg)->allocator == jet_malloc) jm_free(pointers[i]);
#ifdef TEST_JEMALLOC
			else if (((struct _thread_info*)arg)->allocator == jemalloc) je_free(pointers[i]);
#endif
			else if (((struct _thread_info*)arg)->allocator == cruntime) free(pointers[i]);
		}
	}

	if (((struct _thread_info*)arg)->print_stat)
	{
		_jm_arena_print_stats(_jm_get_current_arena(), stats, JM_ARENA_BASIC_STATS | JM_ARENA_CHUNK_STATS, JM_ALLOC_CLASSES);
		printf("%s", stats);
	}

	((struct _thread_info*)arg)->finished = 1;

	_jm_free_arena();

#if defined(__linux__)
	return 0;
#endif
}

#ifdef _WIN32
#include <windows.h>
#endif

void test2(int threads, int class_index, int block_size, int init_mem, char* allocator, int print_stat)
{
	THREADS = threads;

	BEGIN_MEASURE();

	for(int i=0; i < THREADS; i++)
	{
		if (!strcmp(allocator, "cruntime")) thread_info[i].allocator = cruntime;
		else if (!strcmp(allocator, "jemalloc")) thread_info[i].allocator = jemalloc;
		else if (!strcmp(allocator, "jet_malloc")) thread_info[i].allocator = jet_malloc;
		thread_info[i].class_index = class_index;
		thread_info[i].block_size = block_size;
		thread_info[i].init_mem = init_mem;
		thread_info[i].print_stat = print_stat;
#if defined(__linux__)
		pthread_t lThread;
		pthread_create(&lThread, 0, test2_thread, &thread_info[i]);
		pthread_detach(lThread);
#endif

#ifdef _WIN32
		_beginthread(test2_thread, 0, &thread_info[i]);
#endif
	}

	int lCount = 0;
	do
	{
		lCount = 0;

		for (int i=0; i < THREADS; i++)
		{
			if (thread_info[i].finished) lCount++;
		}

#ifdef _WIN32
		Sleep(1);
#endif

#if defined(__linux__)
		usleep(1);
#endif
	}
	while(lCount != THREADS);

	END_MEASURE();

	PRINT_MEASURE(threads, class_index, block_size, init_mem);
}

void test3()
{
	unsigned char* p = (unsigned char*)jm_aligned_malloc(24, 16);
	if(!(((size_t)p) & 15))
	{
		memset(p, 1, 24);
		jm_aligned_free(p);
	}
	else
	{
		printf("test3/error: memory is unaligned\n");
	}
}

#if defined(__linux__)
#include <unistd.h>
#endif

int main(int, char** argv)
{
	//_jm_initialize(); - this function must be ld'ed with "-init,_jm_initialize" options

	printf("The page size for this system is %d bytes.\n", _jm_page_size);

	// 
	if (!strcmp(argv[1], "test1")) test1(atoi(argv[2]), argv[3]);
	else if (!strcmp(argv[1], "test2")) test2(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), argv[7], atoi(argv[6]));
	else if (!strcmp(argv[1], "test3")) test3();
	
	//
	return 0;
}
