#ifndef __JET_MALLOC__
#define __JET_MALLOC__

#include "jet_malloc_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// facade functions
//
JET_MALLOC_DEF void  _jm_initialize();
JET_MALLOC_DEF void* _jm_malloc
(
#if defined(JM_ALLOCATION_INFO)
	size_t size, const char* file, const char* func, int line
#else
	size_t size
#endif
);
JET_MALLOC_DEF void* _jm_realloc
(
#if defined(JM_ALLOCATION_INFO)
	void* address, size_t size, const char* file, const char* func, int line
#else
	void* address, size_t size
#endif
);
JET_MALLOC_DEF void* _jm_calloc
(
#if defined(JM_ALLOCATION_INFO)
	size_t num, size_t size, const char* file, const char* func, int line
#else
	size_t num, size_t size
#endif
);
JET_MALLOC_DEF void  _jm_free(void* address);

JET_MALLOC_DEF void* _jm_aligned_malloc
(
#if defined(JM_ALLOCATION_INFO)
	size_t size, size_t alignment, const char* file, const char* func, int line
#else
	size_t size, size_t alignment
#endif
);
JET_MALLOC_DEF void* _jm_aligned_free(void* address);

JET_MALLOC_DEF void _jm_create_arena(size_t* chunk_sizes, size_t* chunk_count);
JET_MALLOC_DEF void _jm_arena_set_chunks_count(int arena_index, int class_size, int count);
JET_MALLOC_DEF void _jm_arena_set_strategy(int arena_index, int strategy);
JET_MALLOC_DEF void _jm_thread_set_chunks_count(size_t thread_id, int class_size, int count);
JET_MALLOC_DEF void _jm_thread_set_arena_strategy(size_t thread_id, int strategy);
JET_MALLOC_DEF void _jm_free_arena();
JET_MALLOC_DEF void _jm_arena_sweep_lock();
JET_MALLOC_DEF void _jm_arena_sweep_unlock();
JET_MALLOC_DEF void _jm_arena_force_sweep();
JET_MALLOC_DEF void _jm_arena_force_unlocked_sweep();

JET_MALLOC_DEF int  _jm_get_current_arena();
JET_MALLOC_DEF void _jm_arena_print_stats(int arena_index, char* out, unsigned int options, int class_index);
JET_MALLOC_DEF void _jm_arena_reset_stats(int arena_index);
JET_MALLOC_DEF void _jm_thread_print_stats(size_t thread_id, char* out, unsigned int options, int class_index);
JET_MALLOC_DEF void _jm_threads_print_stats(char* out);
JET_MALLOC_DEF void _jm_thread_reset_stats(size_t thread_id);
JET_MALLOC_DEF void _jm_thread_dump_chunk(size_t thread_id, size_t chunk, int class_index, char* path);

JET_MALLOC_DEF void _jm_global_reset_stats();

#if defined(JM_ALLOCATION_INFO)
	#define jm_malloc(size) _jm_malloc(size, __FILE__, __FUNCTION__, __LINE__)
	#define jm_realloc(address, size) _jm_realloc(address, size, __FILE__, __FUNCTION__, __LINE__)
	#define jm_calloc(num, size) _jm_calloc(num, size, __FILE__, __FUNCTION__, __LINE__)
	#define jm_aligned_malloc(size, alignment) _jm_aligned_malloc(size, alignment, __FILE__, __FUNCTION__, __LINE__)
#else
	#define jm_malloc(size) _jm_malloc(size)
	#define jm_realloc(address, size) _jm_realloc(address, size)
	#define jm_calloc(num, size) _jm_calloc(num, size)
	#define jm_aligned_malloc(size, alignment) _jm_aligned_malloc(size, alignment)
#endif

#define jm_free(address) _jm_free(address)
#define jm_aligned_free(address) _jm_aligned_free(address)

#ifdef __cplusplus
}; // __cplusplus
#endif

#endif
