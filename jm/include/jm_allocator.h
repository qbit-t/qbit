// Copyright (c) 2016-2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __JM_ALLOCATOR__
#define __JM_ALLOCATOR__

#include "jm_mutex.h"
#include "jet_malloc_defs.h"

// type for arena index
#define arena_index_t unsigned short

// type for block info
#define block_info_t unsigned short

#define JM_CLASS_INDEX 5
#define JM_BLOCK_INDEX sizeof(block_info_t)-JM_CLASS_INDEX

// data block
struct _jm_data_block
{
	// metainfo
	//
	// [15       -           6] [5   -   0]
	//   1 1 1 1 1 1 1 1 1 1 1   1 1 1 1 1
	//           b_idx             c_idx
	// where:
	// - b_idx - 11 bit - block index [0 .. 2047]
	// - s_idx -  5 bit - class index [0 .. 31]
	block_info_t info;

	// index of arena - owner
	arena_index_t arena_index;

	//
	// actual data just behind this structure
	//
#if defined(__linux__)
	unsigned char pad[4];
#endif //__linux__
};

#define JM_ALIGNED_MARKER 0xCC

// forward
struct _jm_arena;
struct _jm_chunk;

// free block flags
#define JM_BLOCK_FLAG_REMOVE 0x01 // marked to be free

#define _jm_flag_set(flags, flag) flags |= 1 << flag
#define _jm_flag_reset(flags, flag) flags &= ~(1 << flag)
#define _jm_flag_toggle(flags, flag) flags ^= 1 << flag
#define _jm_is_flag_set(flags, flag) (flags >> flag) & 1

// free block
struct _jm_free_block
{
	struct _jm_data_block*	block; // pointer to the data block
	struct _jm_free_block*	next; // pointer to the next free block
	struct _jm_free_block*	prev; // pointer to the prev free block
	volatile size_t			flags; // flags
	//
	// we can add extra allocated block info:
	// - module
	// - address location
	// - source filename, function, line
	// ...
#if defined(JM_ALLOCATION_INFO)
	unsigned char*	file;
	unsigned char*	func;
	size_t			line;
#endif
};

// chunk flags
#define JM_CHUNK_FLAG_LINKED 0x01 // chunk is linked
#define JM_CHUNK_FLAG_PUSHED_DIRTY 0x02 // chunk is dirty and totally free

// chunk
struct _jm_chunk
{
	size_t 	class_index; // 0, 1, 2, 3, ...
	size_t	block_size; // 4, 8, 16, 32, ...
	size_t	blocks_count; // 2048, ...

	// all chunks list
	struct _jm_chunk*		next_chunk;
	struct _jm_chunk*		prev_chunk;
	
	// free blocks list
	struct _jm_free_block*	first_free_block;
	size_t					free_blocks_count;
	volatile size_t			dirty_blocks_count;

	// arena
	struct _jm_arena*		arena;

	// blocks metadata
	struct _jm_free_block*	blocks; 

	// free chunks list
	struct _jm_chunk*		next_free_chunk;
	struct _jm_chunk*		prev_free_chunk;

	// chunk flags
	volatile size_t			flags;

	// dirty flag
	volatile size_t			dirty;

	//
	// | b_idx[0]      | b_idx[1]
	// +----+----------+----+----------
	// | 4b | data	   | 4b | data
	// | mi |		   | mi |
	// +----+----------+----+----------
	//
	struct _jm_data_block*	data; // extra "virtual" pointer - data exactly after the sizeof(struct _jm_chunk)

#if defined(_WIN64) || defined(__LP64__) || defined(_LP64) // 64
	unsigned char pad[8];
#else // 32
	unsigned char pad[4];
#endif
};

// free arena
struct _jm_free_arena
{
	arena_index_t			index;
	struct _jm_free_arena*	next;
};

// arena sweep timeout
#define JM_ARENA_SWEEP_TIMEOUT 5000


#define JM_CHUNK_TIMELINE_ITEMS 10
struct _jm_chunk_class_info
{
	size_t		count;
	size_t		current;
	size_t		timeline[JM_CHUNK_TIMELINE_ITEMS];
	longlong_t	last_time;
};

// arena actioons
#define JM_NORMAL 0
#define JM_FORCE 1

// arena
struct _jm_arena
{
	// thread id
	size_t					owner;
	// pointer to the free arena list item
	struct _jm_free_arena*	origin;

	struct _jm_chunk*	root_chunks[JM_ALLOC_CLASSES];
	struct _jm_chunk*	free_chunks[JM_ALLOC_CLASSES];
	struct _jm_chunk*	current_chunks[JM_ALLOC_CLASSES];
	struct _jm_chunk	chunks_template[JM_ALLOC_CLASSES];
	
	// preserved chunk info and stats
	struct _jm_chunk_class_info	allocs_chunks[JM_ALLOC_CLASSES];
	struct _jm_chunk_class_info	deallocs_chunks[JM_ALLOC_CLASSES];
	
	// stat
	size_t				chunks_count[JM_ALLOC_CLASSES];
	size_t				free_chunks_count[JM_ALLOC_CLASSES];
	size_t				total_free_chunks_count[JM_ALLOC_CLASSES];
	size_t				dirty_blocks_count[JM_ALLOC_CLASSES]; // ++
	size_t				dirty_chunks_removed[JM_ALLOC_CLASSES]; // ++
	size_t				total_chunks_count[JM_ALLOC_CLASSES]; // ++
	size_t				max_chunks_count[JM_ALLOC_CLASSES];
	size_t				max_allocs_count[JM_ALLOC_CLASSES];
	size_t				max_deallocs_count[JM_ALLOC_CLASSES];
	ulonglong_t			total_allocated[JM_ALLOC_CLASSES];

	// strategy
	int					recycle_strategy;

	// if arena has dirty chunks
	volatile size_t		dirty;

	// last sweep'n'clean time
	longlong_t			last_sweep_time;

	// if arena has been locked for sweep'n'clean operations
	volatile size_t		sweep_locked;

	// if arena in stats_processing state
	volatile size_t		stats_processing;

	// if arena is sweeping now
	volatile size_t		sweeping;

	// mutex
	struct _jm_mutex	mutex;
};

#endif
