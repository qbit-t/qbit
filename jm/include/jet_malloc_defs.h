#ifndef __JET_MALLOC_DEFS__
#define __JET_MALLOC_DEFS__

#include <stdlib.h>

#ifdef _MSC_VER
	#ifdef JET_MALLOC_EXPORT
		#define JET_MALLOC_DEF __declspec(dllexport)
	#else
		#define JET_MALLOC_DEF __declspec(dllimport)
	#endif
#elif defined(_GCC)
        #ifdef JET_MALLOC_EXPORT
                #define JET_MALLOC_DEF __attribute__((visibility("default")))
        #else
                #define JET_MALLOC_DEF
        #endif
#else
	#define JET_MALLOC_DEF
#endif

#define longlong_t long long
#define ulonglong_t unsigned long long

#ifdef _DEBUG
	#define JM_INLINE
	#define inline
	#ifdef _MSC_VER
		#define inline _inline
	#endif
#else
	#define JM_INLINE static inline
	#ifdef _MSC_VER
		#define inline _inline
	#endif
#endif

//
// options
//
#undef	JM_TOTAL_STAT
#define JM_USE_MUTEX  1
#define JM_ALLOCATION_INFO 1

//
#ifdef __cplusplus
extern "C" {
#endif

//
// arenas
//
#define JM_INITIAL_ARENAS	512
#define JM_MAX_ARENAS		65535

// options
#define JM_ARENA_RECYCLE_MAX_FREE 0
#define JM_ARENA_RECYCLE_MIN_FREE 1
#define JM_ARENA_RECYCLE_FIRST_FREE 2

//
// arena statistics
//
#define JM_ARENA_BASIC_STATS		0x01
#define JM_ARENA_CHUNK_STATS		0x02
#define JM_ARENA_TOTAL_STATS		0x04
#define JM_ARENA_DIRTY_BLOCKS_STATS 0x08
#define JM_ARENA_FREEE_BLOCKS_STATS 0x10

//
// arena classes
//
#define JM_ALLOC_CLASSES 31

// config
struct _jm_config
{
	void (*arena_init)(size_t*, size_t*);
};

JET_MALLOC_DEF extern struct _jm_config _jm_global_config;
JET_MALLOC_DEF extern int _jm_page_size;

#ifdef __cplusplus
}; // __cplusplus
#endif

#endif
