
#include "include/jm_backend.h"
#include "include/jm_allocator.h"
#include "include/jm_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

//
// prediction optimization
//
#if defined(__linux__)
	#define likely(x)   __builtin_expect(!!(x), 1)
	#define unlikely(x) __builtin_expect(!!(x), 0)
#else
	#define likely(x)   (x)
	#define unlikely(x) (x)
#endif

//
// assert
//
#define	_jm_assert(e) {\
	if (unlikely(!(e))) \
	{ \
		printf("[jet.malloc]: %s:%d - Failed assertion: \"%s\"\n", __FILE__, __LINE__, #e); \
		fflush(stdout); \
		abort(); \
	} \
}

// arena - initial chunks table
struct _jm_chunk _jm_chunk_template[] = 
{
	{ 0,          4, 2048, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // sensless

	{ 1,          8, 2048, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 2,         16, 2048, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 3,         32, 1024, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 2048
	{ 4,         64, 1024, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 2048
	{ 5,        128,  512, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 2048
	{ 6,        256,  512, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 7,        512,  128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 8,       1024,  128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

	{ 9,       2048,   64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{10,       4096,   64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{11,       8192,   32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{12,      16384,   16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{13,      32768,    8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{14,      65536,    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{15,     131072,    2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

	{16,     262144,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{17,     524288,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{18,    1048576,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{19,    2097152,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{20,    4194304,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{21,    8388608,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{22,   16777216,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{23,   33554432,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

	{24,   67108864,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{25,  134217728,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{26,  268435456,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{27,  536870912,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{28, 1073741824,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{29, 2147483648,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{30, 4294967295,    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } //6
};

size_t _jm_arena_preserve_chunks[] = 
{
	1, // class  0
	5, // class  1
	5, // class  2
	5, // class  3
	5, // class  4
	4, // class  5
	4, // class  6
	3, // class  7
	3, // class  8
	2, // class  9
	2, // class 10
	2, // class 11
	1, // class 12
	1, // class 13
	1, // class 14
	1, // class 15
	0, // class 16
	0, // class 17
	0, // class 18
	0, // class 19
	0, // class 20
	0, // class 21
	0, // class 22
	0, // class 23
	0, // class 24
	0, // class 25
	0, // class 26
	0, // class 27
	0, // class 28
	0, // class 29
	0  // class 30
};

#define JM_CLASS_SIZE_0			1024 
#define JM_CLASS_SIZE_0_START	0
#define JM_CLASS_SIZE_0_MIDDLE	4 
#define JM_CLASS_SIZE_0_END		9

#define JM_CLASS_SIZE_1			131072 
#define JM_CLASS_SIZE_1_START	9
#define JM_CLASS_SIZE_1_MIDDLE	12 
#define JM_CLASS_SIZE_1_END		16

#define JM_CLASS_SIZE_2			33554432
#define JM_CLASS_SIZE_2_START	16
#define JM_CLASS_SIZE_2_MIDDLE	19 
#define JM_CLASS_SIZE_2_END		24

#define JM_CLASS_SIZE_3			4294967296
#define JM_CLASS_SIZE_3_START	24
#define JM_CLASS_SIZE_3_MIDDLE	27 
#define JM_CLASS_SIZE_3_END		30

//
// block info
//
JM_INLINE void _jm_block_set_info(struct _jm_data_block* block, arena_index_t arena_index, unsigned short block_index, unsigned short class_index)
{
	block->info = block_index;
	block->info = block->info << JM_CLASS_INDEX;
	block->info += class_index;
	block->arena_index = arena_index;
}

JM_INLINE void _jm_block_get_info(struct _jm_data_block* block, arena_index_t* arena_index, unsigned short* block_index, unsigned short* class_index)
{
	*block_index = block->info >> JM_CLASS_INDEX;
	*class_index = block->info & 0x1f;
	*arena_index = block->arena_index;
}

//
// arenas
//
struct _jm_config _jm_global_config = { 0 };
struct _jm_arena** _jm_arenas = 0;
struct _jm_free_arena* _jm_free_arenas = 0;
struct _jm_free_arena* _jm_free_arena_root = 0;

struct _jm_mutex _jm_arenas_mutex = { 0 };

int _jm_arenas_count = 0;
int _jm_arenas_max = JM_INITIAL_ARENAS;

// class sizes index
#define JM_MAX_CLASS_SIZE 65535
int _jm_alloc_class_index_initialized = 0;
unsigned short _jm_alloc_class_index[JM_MAX_CLASS_SIZE] = { 0 };

//
// arenas management
//
struct _jm_free_arena* _jm_arena_prealloc()
{
	unsigned short lIdx = 0, lClassIdx = 0;
	unsigned int lArenaIdx = 0;
	struct _jm_arena** lNewArenas = 0;
	struct _jm_free_arena* lFreeArena = 0;

	// lock arenas
	_jm_mutex_lock(&_jm_arenas_mutex);

	if (!_jm_arenas)
	{
		_jm_arenas = (struct _jm_arena**)_jm_backend_alloc(0, sizeof(struct _jm_arena*) * JM_INITIAL_ARENAS);
		_jm_free_arenas = (struct _jm_free_arena*)_jm_backend_alloc(0, sizeof(struct _jm_free_arena) * JM_MAX_ARENAS);
		
		for (lArenaIdx = 0; lArenaIdx < JM_MAX_ARENAS; lArenaIdx++) 
		{ 
			_jm_free_arenas[lArenaIdx].index = lArenaIdx; // init indeces
			if (lArenaIdx+1 < JM_MAX_ARENAS) _jm_free_arenas[lArenaIdx].next = &_jm_free_arenas[lArenaIdx+1]; // init list
		}

		_jm_free_arena_root = &_jm_free_arenas[0]; // init root
	}

	if (!_jm_alloc_class_index_initialized) // init sizes index
	{
		for(lIdx = 0; lIdx < JM_MAX_CLASS_SIZE; lIdx++)
		{
			for(lClassIdx = 0; lClassIdx < JM_ALLOC_CLASSES; lClassIdx++)
			{
				if(lIdx <= _jm_chunk_template[lClassIdx].block_size) { _jm_alloc_class_index[lIdx] = lClassIdx; break; }
			}
		}

		_jm_alloc_class_index_initialized = 1;
	}

	if (_jm_arenas_count + 1 >= _jm_arenas_max)
	{
		_jm_assert(_jm_arenas_count + 1 < JM_MAX_ARENAS); 

		lNewArenas = (struct _jm_arena**)_jm_backend_alloc(0, sizeof(struct _jm_arena*) * (_jm_arenas_max + JM_INITIAL_ARENAS));
		memcpy(lNewArenas, _jm_arenas, sizeof(struct _jm_arena*) * _jm_arenas_count);
		_jm_backend_free((unsigned char*)_jm_arenas, sizeof(struct _jm_arena*) * _jm_arenas_count);

		_jm_arenas = lNewArenas;
		_jm_arenas_max += JM_INITIAL_ARENAS;
	}

	// inc current arenas count
	_jm_arenas_count++;

	// get free root
	lFreeArena = _jm_free_arena_root;

	// move next
	_jm_free_arena_root = _jm_free_arena_root->next; 

	// unlink
	lFreeArena->next = 0; 

	_jm_assert(_jm_free_arena_root); 

	// unlock arenas
	_jm_mutex_unlock(&_jm_arenas_mutex);

	return lFreeArena;
}

//
// arena and chunks management
//
void _jm_arena_init(struct _jm_arena* arena, size_t* chunk_sizes, size_t* chunk_count)
{
	int lIdx;

	if (!chunk_sizes && !chunk_count) return;

	for(lIdx = 0; lIdx < JM_ALLOC_CLASSES; lIdx++)
	{
		if (chunk_sizes) arena->chunks_template[lIdx].blocks_count = chunk_sizes[lIdx];
		if (chunk_count) arena->allocs_chunks[lIdx].count = chunk_count[lIdx];
	}
}

struct _jm_arena* _jm_arena_alloc()
{
	int lIdx = 0;

	size_t lChunksTemplate[JM_ALLOC_CLASSES] = {0};
	size_t lChunksCount[JM_ALLOC_CLASSES] = {0};

	struct _jm_arena* lArena = (struct _jm_arena*)_jm_backend_alloc(0, sizeof(struct _jm_arena));
	_jm_assert(lArena);

	memset(lArena, 0, sizeof(struct _jm_arena));

	lArena->origin = _jm_arena_prealloc(); // assign arena origin
	_jm_arenas[lArena->origin->index] = lArena; // register arena

	lArena->owner = _jm_get_current_thread_id();

	_jm_mutex_init(&lArena->mutex); // init arena mutex

	// init chunks template
	memcpy(lArena->chunks_template, &_jm_chunk_template[0], sizeof(_jm_chunk_template));

	// init preserved chunks count by class
	for(lIdx = 0; lIdx < JM_ALLOC_CLASSES; lIdx++)
	{
		lArena->allocs_chunks[lIdx].count = _jm_arena_preserve_chunks[lIdx];
	}
	
	// default arena recycle strategy is "JM_ARENA_RECYCLE_FIRST_FREE"
	lArena->recycle_strategy = JM_ARENA_RECYCLE_FIRST_FREE;

	if (_jm_global_config.arena_init)
	{
		_jm_global_config.arena_init(&lChunksTemplate[0], &lChunksCount[0]); // call application specific global init
		_jm_arena_init(lArena, &lChunksTemplate[0], &lChunksCount[0]); // fill specific settings
	}

	return lArena;
}

void _jm_arena_free(struct _jm_arena* arena)
{
	// lock arenas
	_jm_mutex_lock(&_jm_arenas_mutex);

	// free arena slot
	_jm_arenas[arena->origin->index] = 0;

	// push free index
	arena->origin->next = _jm_free_arena_root;
	_jm_free_arena_root = arena->origin;

	// dec current arenas count
	_jm_arenas_count--;

	// destroy arena mutex
	_jm_mutex_free(&arena->mutex);

	// unlock arenas
	_jm_mutex_unlock(&_jm_arenas_mutex);

	_jm_backend_free((unsigned char*)arena, sizeof(struct _jm_arena));
}

JM_INLINE void _jm_arena_chunk_free(struct _jm_arena* arena, struct _jm_chunk* chunk)
{
	// stat
	size_t lChunkSize = sizeof(struct _jm_chunk) + chunk->blocks_count * chunk->block_size + chunk->blocks_count * sizeof(struct _jm_free_block);

	arena->total_allocated[chunk->class_index] -= lChunkSize;

	arena->chunks_count[chunk->class_index]--;
	if (chunk->free_blocks_count == chunk->blocks_count) arena->total_free_chunks_count[chunk->class_index]--; // descrease

	_jm_backend_free((unsigned char*)chunk, lChunkSize);
}

JM_INLINE void _jm_arena_link_chunk(struct _jm_arena* arena, struct _jm_chunk* chunk)
{
	size_t lClassIndex = chunk->class_index;

	if (likely(_jm_is_flag_set(chunk->flags, JM_CHUNK_FLAG_LINKED))) return;

	if (unlikely(!arena->free_chunks[lClassIndex])) 
	{ 
		arena->free_chunks[lClassIndex] = chunk; 
		chunk->next_free_chunk = chunk->prev_free_chunk = 0;
	}
	else
	{
		arena->free_chunks[lClassIndex]->prev_free_chunk = chunk;
		chunk->next_free_chunk = arena->free_chunks[lClassIndex];
		chunk->prev_free_chunk = 0;

		arena->free_chunks[lClassIndex] = chunk;
	}

	_jm_flag_set(chunk->flags, JM_CHUNK_FLAG_LINKED);

	arena->free_chunks_count[lClassIndex]++;
}

JM_INLINE struct _jm_chunk* _jm_arena_unlink_chunk(struct _jm_arena* arena, struct _jm_chunk* chunk)
{
	struct _jm_chunk* lNext = chunk;

	if (unlikely(!_jm_is_flag_set(chunk->flags, JM_CHUNK_FLAG_LINKED))) return chunk;

	if (!chunk->prev_free_chunk && !chunk->next_free_chunk) // single
	{
		lNext = arena->free_chunks[chunk->class_index] = 0;
	}
	else if (!chunk->prev_free_chunk && chunk->next_free_chunk) // begin
	{
		chunk->next_free_chunk->prev_free_chunk = 0;
		lNext = arena->free_chunks[chunk->class_index] = chunk->next_free_chunk;
	}
	else if (chunk->prev_free_chunk && !chunk->next_free_chunk) // end
	{
		lNext = chunk->prev_free_chunk->next_free_chunk = 0;
	}
	else // middle
	{
		lNext = chunk->prev_free_chunk->next_free_chunk = chunk->next_free_chunk;
		chunk->next_free_chunk->prev_free_chunk = chunk->prev_free_chunk;
	}

	chunk->next_free_chunk = chunk->prev_free_chunk = 0;
	
	_jm_flag_reset(chunk->flags, JM_CHUNK_FLAG_LINKED);
	
	arena->free_chunks_count[chunk->class_index]--;

	return lNext;
}

JM_INLINE struct _jm_chunk* _jm_arena_push_single_chunk(struct _jm_arena* arena, struct _jm_chunk* chunk, struct _jm_chunk** next_free, int force)
{
	struct _jm_chunk* lBegin = 0; 
	struct _jm_chunk* lRemove = 0;
	struct _jm_chunk* lNext = chunk;
	struct _jm_chunk* lFree = 0;
	
	size_t lClassIndex = 0;

	_jm_arena_link_chunk(arena, chunk); // link chunk

	if (unlikely(next_free)) *next_free = lFree; // by default

	// chunk is free?
	if (unlikely(chunk->free_blocks_count == chunk->blocks_count || force))
	{
		lClassIndex = chunk->class_index;

		// if chunk is freed by other threads
		// force used only from clean_and_sweep
		if (force)
		{
			chunk->dirty = 0; // reset
			chunk->dirty_blocks_count = 0; // reset
			chunk->free_blocks_count = chunk->blocks_count; // reset - chunk may be freed by next _jm_arena_chunk_free and stats should be intect
			_jm_flag_set(chunk->flags, JM_CHUNK_FLAG_PUSHED_DIRTY); // mark it
		}

		// check and remove unneeded chunks
		if (unlikely(arena->chunks_count[lClassIndex] > arena->allocs_chunks[lClassIndex].count)) // threshold
		{
			if (arena->stats_processing) return lNext; // skip chunks removing

			lFree = _jm_arena_unlink_chunk(arena, chunk); // unlink "chunk" from free list
			if (next_free) *next_free = lFree;

			lBegin = arena->root_chunks[lClassIndex];
			if (chunk == lBegin) // first = root_chunk
			{
				lNext = chunk->next_chunk; // move to the next
				if (lNext) lNext->prev_chunk = 0; // cut link

				lRemove = lBegin;
				if (lRemove == arena->current_chunks[lClassIndex]) arena->current_chunks[lClassIndex] = 0; // reset - force recycle

				_jm_arena_chunk_free(arena, lRemove);

				lBegin = lNext;
				arena->root_chunks[lClassIndex] = lBegin;
			}
			else if (!chunk->next_chunk) // last chunk
			{
				chunk->prev_chunk->next_chunk = 0;
				
				lRemove = chunk;
				if (lRemove == arena->current_chunks[lClassIndex]) arena->current_chunks[lClassIndex] = 0; // reset - force recycle

				_jm_arena_chunk_free(arena, lRemove);

				lNext = 0;
			}
			else // in the middle of nowhere
			{
				chunk->prev_chunk->next_chunk = chunk->next_chunk;
				chunk->next_chunk->prev_chunk = chunk->prev_chunk;

				lNext = chunk->next_chunk;
				lRemove = chunk;
				if (lRemove == arena->current_chunks[lClassIndex]) arena->current_chunks[lClassIndex] = 0; // reset - force recycle

				_jm_arena_chunk_free(arena, lRemove);
			}
		}
	}

	return lNext;
}

// forward
JM_INLINE int _jm_arena_calculate_chunks_speed(struct _jm_arena* arena, struct _jm_chunk_class_info* info, int class_index, size_t base);
JM_INLINE struct _jm_chunk* _jm_arena_calc_stat_and_push_chunks(struct _jm_arena* arena, struct _jm_chunk* chunk, int class_index)
{
	struct _jm_chunk* lCurrent = 0;
	struct _jm_chunk* lNext = 0;

	// recalc memory consumption speed - deallocations
	_jm_arena_calculate_chunks_speed(arena, arena->deallocs_chunks, class_index, 0 /* base */);

	// if we can reprocess unneded total free chunks
	if (arena->deallocs_chunks[class_index].count < arena->total_free_chunks_count[class_index])
	{
		// calculate delta, culculate upper limit and release unneded
		arena->allocs_chunks[class_index].count = arena->chunks_count[class_index] - (arena->total_free_chunks_count[class_index] - arena->deallocs_chunks[class_index].count);
		// check default template
		if (arena->allocs_chunks[class_index].count < _jm_arena_preserve_chunks[class_index]) arena->allocs_chunks[class_index].count = _jm_arena_preserve_chunks[class_index];

		lCurrent = arena->free_chunks[class_index];
		while(lCurrent && arena->allocs_chunks[class_index].count < arena->chunks_count[class_index])
		{
			if (lCurrent != chunk)
			{
				_jm_arena_push_single_chunk(arena, lCurrent, &lNext, 0 /* not forced */);

				if (lCurrent != lNext) 
				{
					lCurrent = lNext;
					continue;
				}
			}

			lCurrent = lCurrent->next_free_chunk;
		}
	}
}

JM_INLINE struct _jm_chunk* _jm_arena_push_chunk(struct _jm_arena* arena, struct _jm_chunk* chunk, int force)
{
	struct _jm_chunk* lCurrent = 0;
	struct _jm_chunk* lNext = 0;

	if (unlikely(chunk->free_blocks_count == chunk->blocks_count || force)) 
	{
		_jm_arena_calc_stat_and_push_chunks(arena, chunk /* avoid push this chunk */, chunk->class_index);
	}

	return _jm_arena_push_single_chunk(arena, chunk, 0, force); // force may be
}

JM_INLINE struct _jm_chunk* _jm_arena_recycle_chunks(struct _jm_arena* arena, int class_index)
{
	struct _jm_chunk* lCurrent = arena->free_chunks[class_index];
	struct _jm_chunk* lCandidate = lCurrent;

	switch(arena->recycle_strategy)
	{
		case JM_ARENA_RECYCLE_MAX_FREE:
			{
				while(lCurrent)
				{
					if (lCurrent->free_blocks_count > /* max */ lCandidate->free_blocks_count) lCandidate = lCurrent;
					lCurrent = lCurrent->next_free_chunk;
				}
			}
			break;
		case JM_ARENA_RECYCLE_MIN_FREE:
			{
				while(lCurrent)
				{
					if (lCurrent->free_blocks_count < /* min */ lCandidate->free_blocks_count) lCandidate = lCurrent;
					lCurrent = lCurrent->next_free_chunk;
				}
			}
			break;
		default: // JM_ARENA_RECYCLE_FIRST_FREE
			break;
	}

	arena->current_chunks[class_index] = lCandidate;
	return lCandidate;
}

void _jm_chunk_init(struct _jm_chunk* chunk)
{
	size_t lIdx = 0;
	struct _jm_free_block* lCurrBlock = 0;
	struct _jm_free_block* lNextBlock = 0;
	size_t lBlocksSize = 0;

	// initial free block count
	chunk->free_blocks_count = chunk->blocks_count;

	// reset blocks data
	memset(chunk->blocks, 0, chunk->blocks_count * sizeof(struct _jm_free_block));

	// init free blocks list
	for (lIdx = 0; lIdx < chunk->blocks_count; lIdx++)
	{
		lCurrBlock = &chunk->blocks[lIdx];
		lNextBlock = lIdx+1 < chunk->blocks_count ? &chunk->blocks[lIdx+1] : 0;
			
		lCurrBlock->block = (struct _jm_data_block*)((unsigned char*)chunk->data + (lIdx * chunk->block_size));
			
		_jm_block_set_info(lCurrBlock->block, chunk->arena->origin->index, lIdx, chunk->class_index);

		lCurrBlock->next = lNextBlock;
		if (lNextBlock) lNextBlock->prev = lCurrBlock;
	}

	chunk->first_free_block = &chunk->blocks[0];
	chunk->first_free_block->block = chunk->data;

	chunk->dirty_blocks_count = 0;
	chunk->dirty = 0;
}

JM_INLINE int _jm_arena_calculate_chunks_speed(struct _jm_arena* arena, struct _jm_chunk_class_info* info, int class_index, size_t base)
{
	int lIdx = 0, lForwardCount = 0;
	longlong_t lTime = 0, lDelta = 0;
	size_t lSpeed = 0;

	// calc current count
	if (!info[class_index].last_time) 
	{ 
		info[class_index].last_time = _jm_get_time();
	}
	else
	{
		lTime = _jm_get_time();
		lDelta = lTime - info[class_index].last_time;
		lForwardCount = (int)(lDelta / 1000); // steps

		if (lForwardCount < 0 || lForwardCount >= JM_CHUNK_TIMELINE_ITEMS) // too much time in past
		{
			memset(&info[class_index].timeline[0], 0, sizeof(size_t) * JM_CHUNK_TIMELINE_ITEMS); // reset all stat
			info[class_index].current = 0; // reset current mark
			info[class_index].last_time = lTime; // set last time
		}
		else // we are in flow
		{
			for (lIdx = 0; lIdx < lForwardCount; lIdx++) 
			{
				// round robin
				if (info[class_index].current + 1 >= JM_CHUNK_TIMELINE_ITEMS) info[class_index].current = 0;
				else info[class_index].current++;

				info[class_index].timeline[info[class_index].current] = 0; // reset
				info[class_index].last_time = lTime; // set last time
			}
		}
	}

	info[class_index].timeline[info[class_index].current]++; // calc mark stat

	// calc speed
	for (lIdx = 0; lIdx < JM_CHUNK_TIMELINE_ITEMS; lIdx++)
	{
		lSpeed += info[class_index].timeline[lIdx];
	}

	// calc avg speed
	lSpeed /= JM_CHUNK_TIMELINE_ITEMS;

	// calc current needs
	lSpeed += base;

	// check
	if (lSpeed >= info[class_index].count) 
	{
		info[class_index].count = lSpeed; // increase size
	}
	else // descrease - speed going down
	{
		if (lSpeed > _jm_arena_preserve_chunks[class_index]) // default template
		{
			info[class_index].count = lSpeed; // signal to free "free" some chunks
		}
		else
		{
			info[class_index].count = _jm_arena_preserve_chunks[class_index]; // set template settings
		}

		return 1; // recycle chunks
	}

	return 0; // no recycling
}

JM_INLINE struct _jm_chunk* _jm_arena_chunk_alloc(struct _jm_arena* arena, int class_index)
{
	// [sizeof(struct _jm_chunk)][... class data ...][... class metadata ...]
	size_t lNewChunkSize = 
		sizeof(struct _jm_chunk) + // chunk size
		arena->chunks_template[class_index].blocks_count * arena->chunks_template[class_index].block_size + // data block size
		arena->chunks_template[class_index].blocks_count * sizeof(struct _jm_free_block); // metadata block size
	
	// new chunk
	struct _jm_chunk* lNewChunk = (struct _jm_chunk*)_jm_backend_alloc(0, lNewChunkSize);
	_jm_assert(lNewChunk);

	// stat
	arena->total_allocated[class_index] += lNewChunkSize;

	// chunk template from arena preset
	memcpy(lNewChunk, &arena->chunks_template[class_index], sizeof(struct _jm_chunk));
	
	lNewChunk->arena = arena;
	lNewChunk->data = (struct _jm_data_block*)(((unsigned char*)lNewChunk) + sizeof(struct _jm_chunk)); // offset to real data
	lNewChunk->blocks = (struct _jm_free_block*)(((unsigned char*)lNewChunk) + sizeof(struct _jm_chunk) + lNewChunk->blocks_count * lNewChunk->block_size); // offset to metadata
	
	// stat
	arena->chunks_count[class_index]++;
	arena->total_chunks_count[class_index]++;
	if (arena->chunks_count[class_index] > arena->max_chunks_count[class_index]) arena->max_chunks_count[class_index] = arena->chunks_count[class_index];

	if (arena->root_chunks[class_index])
	{
		arena->root_chunks[class_index]->prev_chunk = lNewChunk;
		lNewChunk->next_chunk = arena->root_chunks[class_index];
	}

	arena->root_chunks[class_index] = lNewChunk; // new chunk became root
	arena->current_chunks[class_index] = lNewChunk; // current fresh one
	arena->total_free_chunks_count[class_index]++;

	// calc memory consumption speed
	_jm_arena_calculate_chunks_speed(arena, arena->allocs_chunks, class_index,  arena->chunks_count[class_index] /* base */);

	// init new chunk
	_jm_chunk_init(lNewChunk);

	// link new chunk to the free chunks list
	_jm_arena_link_chunk(arena, lNewChunk);

	return lNewChunk;
}

JM_INLINE struct _jm_data_block* _jm_chunk_block_alloc
(
#if defined(JM_ALLOCATION_INFO)	
	struct _jm_chunk* chunk, const char* file, const char* func, int line
#else
	struct _jm_chunk* chunk
#endif
)
{
	struct _jm_free_block* lBlock = chunk->first_free_block;

	if (unlikely(chunk->free_blocks_count == chunk->blocks_count)) chunk->arena->total_free_chunks_count[chunk->class_index]--; // total free chunks count

	chunk->first_free_block = lBlock->next;
	chunk->free_blocks_count--;

	if (likely(chunk->first_free_block)) chunk->first_free_block->prev = 0; // unlink first_free_block and lBlock
	else 
	{
		_jm_arena_unlink_chunk(chunk->arena, chunk); // unlink from free chunks list
		if (chunk->arena->current_chunks[chunk->class_index] == chunk) chunk->arena->current_chunks[chunk->class_index] = 0; // reset
	}
		
#ifdef JM_TOTAL_STAT
	// stat
	chunk->arena->max_allocs_count[chunk->class_index]++;
#endif

	lBlock->next = lBlock->prev = 0; // unlink from free_block_list

#ifdef JM_ALLOCATION_INFO
	lBlock->file = (unsigned char*)file;
	lBlock->func = (unsigned char*)func;
	lBlock->line = line;
#endif

	return lBlock->block;
}

JM_INLINE void _jm_chunk_block_free(struct _jm_chunk* chunk, int block_index)
{
	struct _jm_free_block* lBlock = &chunk->blocks[block_index];
	
	if (likely(chunk->first_free_block))
	{
		lBlock->next = chunk->first_free_block;
		chunk->first_free_block->prev = lBlock;
	}

	chunk->first_free_block = lBlock;
	chunk->free_blocks_count++;

	if (unlikely(chunk->free_blocks_count == chunk->blocks_count)) chunk->arena->total_free_chunks_count[chunk->class_index]++; // total free chunks count

#ifdef JM_TOTAL_STAT
	// stat
	chunk->arena->max_deallocs_count[chunk->class_index]++;
#endif

#ifdef JM_ALLOCATION_INFO
	lBlock->file = 0;
	lBlock->func = 0;
	lBlock->line = 0;
#endif
}

JM_INLINE void _jm_chunk_block_set_dirty(struct _jm_chunk* chunk, int block_index)
{
	struct _jm_free_block* lBlock = &chunk->blocks[block_index];
	
	_jm_flag_set(lBlock->flags, JM_BLOCK_FLAG_REMOVE);
	
#ifdef JM_TOTAL_STAT
	// stat
	chunk->arena->dirty_blocks_count[chunk->class_index]++;
#endif

	chunk->dirty = 1;
	chunk->arena->dirty = 1;

	// finish
	_jm_interlocked_increment(&chunk->dirty_blocks_count);
}

JM_INLINE struct _jm_chunk* _jm_arena_pop_chunk(struct _jm_arena* arena, size_t size /* requested size + sizeof(struct _jm_data_block) */)
{
	unsigned int lSize = (unsigned int) size;
	
	int lStartIndex = JM_CLASS_SIZE_0_START, lClassIndex = JM_CLASS_SIZE_0_MIDDLE, lEndIndex = JM_CLASS_SIZE_0_END;
	size_t lCurrentSize = 0; 

	struct _jm_chunk* lChunk = 0;

	if (likely(lSize < JM_MAX_CLASS_SIZE)) // use fast index
	{
		lClassIndex = _jm_alloc_class_index[lSize];
	}
	else // doing hard: calculate size and find allocation class
	{
		// calculate next near size based on "size of power of 2"
		lSize--;
		lSize |= lSize >> 1;
		lSize |= lSize >> 2;
		lSize |= lSize >> 4;
		lSize |= lSize >> 8;
		lSize |= lSize >> 16;
		lSize++;

		// little optimization
		if (lSize <= JM_CLASS_SIZE_1) { lStartIndex = JM_CLASS_SIZE_1_START; lClassIndex = JM_CLASS_SIZE_1_MIDDLE; lEndIndex = JM_CLASS_SIZE_1_END; }
		else if (lSize <= JM_CLASS_SIZE_2) { lStartIndex = JM_CLASS_SIZE_2_START; lClassIndex = JM_CLASS_SIZE_2_MIDDLE; lEndIndex = JM_CLASS_SIZE_2_END; }
		else if (lSize <= JM_CLASS_SIZE_3) { lStartIndex = JM_CLASS_SIZE_3_START; lClassIndex = JM_CLASS_SIZE_3_MIDDLE; lEndIndex = JM_CLASS_SIZE_3_END; }

		// get middle
		lCurrentSize = arena->chunks_template[lClassIndex].block_size;

		// check allowed size
		_jm_assert(lSize <= arena->chunks_template[JM_ALLOC_CLASSES-1].block_size);

		// lookup - fast search
		while(1)
		{
			if (lCurrentSize == lSize) break; // lCurrentIndex is what we want
			else if (lCurrentSize > lSize) lEndIndex = lClassIndex;
			else lStartIndex = lClassIndex;

			lClassIndex = lStartIndex + ((lEndIndex - lStartIndex) >> 1);
			lCurrentSize = arena->chunks_template[lClassIndex].block_size;
		}
	}

	lChunk = arena->current_chunks[lClassIndex];

	if (unlikely(!lChunk /*|| !lChunk->free_blocks_count*/)) // no free chunks in list
	{
		lChunk = _jm_arena_recycle_chunks(arena, lClassIndex);
		if (unlikely(!lChunk /*|| !lChunk->free_blocks_count*/))
		{
			lChunk = _jm_arena_chunk_alloc(arena, lClassIndex);
		}
	}
	
	if (_jm_is_flag_set(lChunk->flags, JM_CHUNK_FLAG_PUSHED_DIRTY)) // chunk was previously freed by others
	{
		_jm_chunk_init(lChunk); // re-init
		_jm_flag_reset(lChunk->flags, JM_CHUNK_FLAG_PUSHED_DIRTY); // rseset flag
	}

	return lChunk;
}

JM_INLINE void _jm_arena_clean_and_sweep(struct _jm_arena* arena, int action)
{
	int lClassIdx;
	size_t lBlockIdx, lDirtyBlocksCount;
	struct _jm_chunk* lCurrent = 0;
	struct _jm_chunk* lNext = 0;
	struct _jm_free_block* lCurrBlock = 0;

	longlong_t lCurrentTime = 0; 

	// little tricky
	if (likely(!arena->stats_processing && !arena->sweep_locked && (arena->dirty || action == JM_FORCE)))
	{
		_jm_interlocked_exchange(&arena->sweeping, 1); // going to sweep

		lCurrentTime = _jm_get_time();
		if (unlikely((lCurrentTime - arena->last_sweep_time) > JM_ARENA_SWEEP_TIMEOUT))
		{
			arena->dirty = 0;
			arena->last_sweep_time = lCurrentTime;
			for (lClassIdx = 0; lClassIdx < JM_ALLOC_CLASSES; lClassIdx++)
			{
				lCurrent = arena->root_chunks[lClassIdx];

				while(lCurrent)
				{
					lDirtyBlocksCount = lCurrent->dirty_blocks_count; // get current dirty blocks count

					if (lCurrent->dirty || lDirtyBlocksCount) // if chunk is dirty
					{
						// next is current - reset "previously", may be invalid, lNext 
						lNext = lCurrent; 

						// chunk is totaly free
						if (lDirtyBlocksCount > 0 && (lDirtyBlocksCount == lCurrent->blocks_count || lDirtyBlocksCount == lCurrent->blocks_count - lCurrent->free_blocks_count))
						{
							// total free block stat correction
							lCurrent->arena->total_free_chunks_count[lCurrent->class_index]++;

							// remove chunk in any way
							lNext = _jm_arena_push_chunk(arena, lCurrent, 1 /* force push */); // next can be really "next"

							arena->dirty_chunks_removed[lClassIdx]++;
						}
						else // doing hard
						{
							if (lDirtyBlocksCount > 0 && lDirtyBlocksCount + lCurrent->free_blocks_count < lCurrent->blocks_count)
							{
								for (lBlockIdx = 0; lBlockIdx < lCurrent->blocks_count; lBlockIdx++)
								{
									lCurrBlock = &lCurrent->blocks[lBlockIdx];
									if (_jm_is_flag_set(lCurrBlock->flags, JM_BLOCK_FLAG_REMOVE))
									{
										_jm_chunk_block_free(lCurrent, lBlockIdx);
										_jm_flag_reset(lCurrBlock->flags, JM_BLOCK_FLAG_REMOVE);
										_jm_interlocked_decrement(&lCurrent->dirty_blocks_count);
									}
								}
							}

							lCurrent->dirty = 0;
							if (lCurrent->free_blocks_count) // only if we have free blocks - chunk must be linked and pushed
							{ 
								lNext = _jm_arena_push_chunk(arena, lCurrent, 0 /* normal push */); // next can be really "next"
							}
						}

						// if next is REALLY changed - continue with new current
						if (lNext != lCurrent) { lCurrent = lNext; continue; }
					}
					else // deallocation speed may be changed
					{
						if (lCurrent->free_blocks_count)
						{
							lNext = _jm_arena_push_chunk(arena, lCurrent, 0 /* normal push */); // next can be really "next"
							// if next is REALLY changed - continue with new current
							if (lNext != lCurrent) { lCurrent = lNext; continue; }
						}
					}

					lCurrent = lCurrent->next_chunk;
				}
			}
		}

		_jm_interlocked_exchange(&arena->sweeping, 0); // sweep is done
	}
}

//
// facade functions
//

#ifdef WIN32
__declspec(thread) struct _jm_arena* _jm_current_thread_arena = 0;
#else
__thread struct _jm_arena* _jm_current_thread_arena = 0;
#endif

JET_MALLOC_DEF void* _jm_malloc
(
#if defined(JM_ALLOCATION_INFO)	
	size_t size, const char* file, const char* func, int line
#else
	size_t size
#endif
)
{
	struct _jm_chunk* lChunk = 0;
	struct _jm_data_block* lBlock = 0;
	struct _jm_arena* lArena = _jm_current_thread_arena; //get this from thread storage

	if (unlikely(!lArena))
	{
		_jm_current_thread_arena = _jm_arena_alloc();
		lArena = _jm_current_thread_arena; 
	}

	lChunk = _jm_arena_pop_chunk(lArena,  (size ? size : 1) + sizeof(struct _jm_data_block) /* add metainfo */);
	lBlock = _jm_chunk_block_alloc
	(
#if defined(JM_ALLOCATION_INFO)	
		lChunk, file, func, line
#else
		lChunk
#endif
	);

	return (unsigned char*)lBlock + sizeof(struct _jm_data_block);
}

JET_MALLOC_DEF void* _jm_aligned_malloc
(
#if defined(JM_ALLOCATION_INFO)	
	size_t size, size_t alignment, const char* file, const char* func, int line
#else
	size_t size, size_t alignment
#endif
)
{
	size_t lOffset = (alignment > sizeof(struct _jm_data_block) ? alignment - sizeof(struct _jm_data_block) : 0);
	unsigned char* lAddress = (unsigned char*)_jm_malloc
	(
#if defined(JM_ALLOCATION_INFO)	
		size + lOffset, file, func, line
#else
		size + lOffset
#endif
	);
	memset(lAddress, JM_ALIGNED_MARKER, lOffset);
	memset(lAddress + lOffset, 0, size);
	return lAddress + lOffset;
}

JET_MALLOC_DEF void _jm_free(void* address)
{
	struct _jm_data_block* lBlock = 0;
	struct _jm_chunk* lChunk = 0;
	struct _jm_arena* lArena = 0;
	struct _jm_arena* lCurrentArena = _jm_current_thread_arena; //get this from thread storage
	
	unsigned short lBlockIndex = 0, lClassSize = 0;
	arena_index_t lArenaIndex = 0;

	if (!address) return;
		
	lBlock = (struct _jm_data_block*)((unsigned char*)address - sizeof(struct _jm_data_block));

	_jm_block_get_info(lBlock, &lArenaIndex, &lBlockIndex, &lClassSize);

	// backtracing...
	lChunk = (struct _jm_chunk*)((unsigned char*)lBlock - ((_jm_arenas[lArenaIndex]->chunks_template[lClassSize].block_size * lBlockIndex) + sizeof(struct _jm_chunk)));
	
	// get arena
	lArena = lChunk->arena;

	// check for origin: if owner of block is not current arena - just mark block and chunk for sweeping later
	if (lArena != lCurrentArena)
	{
		_jm_chunk_block_set_dirty(lChunk, lBlockIndex);
	}
	else
	{
		_jm_chunk_block_free(lChunk, lBlockIndex); // free block
		_jm_arena_push_chunk(lArena, lChunk, 0 /* normal push */); // push - check, unlink, delete

		// try to clean up
		_jm_arena_clean_and_sweep(lArena, JM_NORMAL);
	}
}

JET_MALLOC_DEF void _jm_free_direct(void* address)
{
	_jm_free(address);
}

JET_MALLOC_DEF void _jm_aligned_free(void* address)
{
	unsigned char* lAddress = (unsigned char*)address;
	while(*(--lAddress) == JM_ALIGNED_MARKER);
	
	_jm_free(++lAddress);
}

JET_MALLOC_DEF void* _jm_realloc
(
#if defined(JM_ALLOCATION_INFO)	
	void* address, size_t size, const char* file, const char* func, int line
#else
	void* address, size_t size
#endif
)
{
	struct _jm_data_block* lBlock = 0;
	size_t lBlockSize = 0;
	void* lNewAddress = 0;

	unsigned short lBlockIndex = 0, lClassSize = 0;
	arena_index_t lArenaIndex = 0;

	// just alloc if address == 0
	if (!address) return _jm_malloc
	(
#if defined(JM_ALLOCATION_INFO)	
		size, file, func, line
#else
		size
#endif
	);
	// free if size == 0
	else if (!size) { _jm_free(address); return 0; }

	// get block info
	lBlock = (struct _jm_data_block*)((unsigned char*)address - sizeof(struct _jm_data_block));
	_jm_block_get_info(lBlock, &lArenaIndex, &lBlockIndex, &lClassSize);

	// get old address block size
	lBlockSize = _jm_arenas[lArenaIndex]->chunks_template[lClassSize].block_size;

	if (size <= lBlockSize - sizeof(struct _jm_data_block)) return address; // we have enough space

	// alloc new block
	lNewAddress = _jm_malloc
	(
#if defined(JM_ALLOCATION_INFO)	
		size, file, func, line
#else
		size
#endif
	);

	// copy contents
	memcpy(lNewAddress, address, lBlockSize - sizeof(struct _jm_data_block));

	// free address
	_jm_free(address);
	
	// return new one
	return lNewAddress;
}

JET_MALLOC_DEF void* _jm_calloc
(
#if defined(JM_ALLOCATION_INFO)	
	size_t num, size_t size, const char* file, const char* func, int line
#else
	size_t num, size_t size
#endif
)
{
	unsigned char* lAddress = (unsigned char*)_jm_malloc
	(
#if defined(JM_ALLOCATION_INFO)	
		num * size, file, func, line
#else
		num * size
#endif
	);
	memset(lAddress, 0, num * size);
	return lAddress;
}

//
// facade arena functions
//
JET_MALLOC_DEF void _jm_free_arena()
{
	int lIdx = 0;
	struct _jm_chunk* lCurrent = 0;
	struct _jm_chunk* lRemove = 0;
	struct _jm_arena* lArena = _jm_current_thread_arena; //get this from thread storage

	_jm_current_thread_arena = 0; // reset

	if (lArena)
	{
		_jm_mutex_lock(&lArena->mutex);

		for (lIdx = 0; lIdx < JM_ALLOC_CLASSES; lIdx++)
		{
			lCurrent = lArena->root_chunks[lIdx];
			while(lCurrent)
			{
				lRemove = lCurrent;
				lCurrent = lCurrent->next_chunk;
				_jm_arena_chunk_free(lArena, lRemove);
			}
		}

		_jm_mutex_unlock(&lArena->mutex);

		_jm_arena_free(lArena);
	}
}

JET_MALLOC_DEF void _jm_create_arena(size_t* chunk_sizes, size_t* chunk_count)
{
	if (_jm_current_thread_arena)
	{
		_jm_free_arena();
	}

	_jm_current_thread_arena = _jm_arena_alloc();

	_jm_arena_init(_jm_current_thread_arena, chunk_sizes, chunk_count);
}

void _jm_arena_set_chunks_count_internal(struct _jm_arena* arena, int class_size, int count)
{
	_jm_mutex_lock(&arena->mutex);
	
	if (class_size < JM_ALLOC_CLASSES)
	{
		arena->allocs_chunks[class_size].count = count;
	}

	_jm_mutex_unlock(&arena->mutex);
}

void _jm_arena_set_strategy_internal(struct _jm_arena* arena, int strategy)
{
	_jm_mutex_lock(&arena->mutex);
	
	arena->recycle_strategy = strategy;

	_jm_mutex_unlock(&arena->mutex);
}

JET_MALLOC_DEF void _jm_arena_set_chunks_count(int arena_index, int class_size, int count)
{
	if (arena_index >= 0 && arena_index < JM_MAX_ARENAS && _jm_arenas[arena_index] != 0)
	{
		_jm_arena_set_chunks_count_internal(_jm_arenas[arena_index], class_size, count);
	}
}

JET_MALLOC_DEF void _jm_arena_set_strategy(int arena_index, int strategy)
{
	if (arena_index >= 0 && arena_index < JM_MAX_ARENAS && _jm_arenas[arena_index] != 0)
	{
		_jm_arena_set_strategy_internal(_jm_arenas[arena_index], strategy);
	}
}

JET_MALLOC_DEF void _jm_thread_set_chunks_count(size_t thread_id, int class_size, int count)
{
	int lArenaIdx = 0;

	// lock arenas
	_jm_mutex_lock(&_jm_arenas_mutex);

	for(lArenaIdx = 0; lArenaIdx < _jm_arenas_max; lArenaIdx++)
	{
		if (_jm_arenas[lArenaIdx] != 0 && _jm_arenas[lArenaIdx]->owner == thread_id)
		{
			_jm_arena_set_chunks_count_internal(_jm_arenas[lArenaIdx], class_size, count);
			break;
		}
	}

	// ulock arenas
	_jm_mutex_unlock(&_jm_arenas_mutex);
}

JET_MALLOC_DEF void _jm_thread_set_arena_strategy(size_t thread_id, int strategy)
{
	int lArenaIdx = 0;

	// lock arenas
	_jm_mutex_lock(&_jm_arenas_mutex);

	for(lArenaIdx = 0; lArenaIdx < _jm_arenas_max; lArenaIdx++)
	{
		if (_jm_arenas[lArenaIdx] != 0 && _jm_arenas[lArenaIdx]->owner == thread_id)
		{
			_jm_arena_set_strategy_internal(_jm_arenas[lArenaIdx], strategy);
			break;
		}
	}

	// ulock arenas
	_jm_mutex_unlock(&_jm_arenas_mutex);
}

JET_MALLOC_DEF int _jm_get_current_arena()
{
	if (!_jm_current_thread_arena) return -1;
	return _jm_current_thread_arena->origin->index;
}

//
// arena statistics
//
ulonglong_t _jm_arena_print_stats_internal(struct _jm_arena* arena, char* out, unsigned int options, int class_index)
{
	int lLen = 0, lIdx = 0;
	struct _jm_chunk* lCurrent = 0;
	ulonglong_t lTotals = 0;

	if (arena)
	{
		_jm_mutex_lock(&arena->mutex);

		if (options & JM_ARENA_TOTAL_STATS) 
		{
#if defined(__linux__)
			lLen = sprintf(out, "arena [0x%X], thread [%zd], sweep_locks=%zd, ", arena, arena->owner, arena->sweep_locked);
#else
			lLen = sprintf(out, "arena [0x%X], thread [%d], sweep_locks=%d, ", arena, arena->owner, arena->sweep_locked);
#endif
		}
		else
		{
#if defined(__linux__)
			lLen = sprintf(out, "arena [0x%X], thread [%zd], sweep_locks=%zd\n", arena, arena->owner, arena->sweep_locked);
#else
			lLen = sprintf(out, "arena [0x%X], thread [%d], sweep_locks=%d\n", arena, arena->owner, arena->sweep_locked);
#endif
		}

		for (lIdx = 0; lIdx < JM_ALLOC_CLASSES; lIdx++)
		{
			if (arena->sweeping) break;

			if (class_index == JM_ALLOC_CLASSES || lIdx == class_index)
			{
				lCurrent = arena->root_chunks[lIdx];

				if (lCurrent) // arena stats
				{
					if (options & JM_ARENA_BASIC_STATS)
					{
#if defined(__linux__)
						lLen += sprintf(out+lLen, "\tclass=%ld, block=%zd, blocks=%zd, pre_chunks=%zd, cur_chunks=%zd, free_chunks=%zd, total_free_chunks=%zd, max_chunks=%zd, total_allocated=%llu, total_chunks=%zd, dirty_chunks_removed=%zd, root=[0x%X], current=[0x%X]\n", 
#else
						lLen += sprintf(out+lLen, "\tclass=%d, block=%d, blocks=%d, pre_chunks=%d, cur_chunks=%d, free_chunks=%d, total_free_chunks=%d, max_chunks=%d, total_allocated=%llu, total_chunks=%d, dirty_chunks_removed=%d, root=[0x%X], current=[0x%X]\n", 
#endif
							lCurrent->class_index, lCurrent->block_size, lCurrent->blocks_count, arena->allocs_chunks[lIdx].count, arena->chunks_count[lIdx], arena->free_chunks_count[lIdx], arena->total_free_chunks_count[lIdx], arena->max_chunks_count[lIdx], arena->total_allocated[lIdx],  
							arena->total_chunks_count[lIdx],
							arena->dirty_chunks_removed[lIdx],
							arena->root_chunks[lIdx], arena->current_chunks[lIdx]);
					}
				}

				lTotals += arena->total_allocated[lIdx];

				if (options & JM_ARENA_CHUNK_STATS || options & JM_ARENA_DIRTY_BLOCKS_STATS || options & JM_ARENA_FREEE_BLOCKS_STATS)
				{
					while(lCurrent)
					{
						if ((options & JM_ARENA_DIRTY_BLOCKS_STATS && lCurrent->dirty_blocks_count) || 
							(options & JM_ARENA_FREEE_BLOCKS_STATS && lCurrent->free_blocks_count) || options & JM_ARENA_CHUNK_STATS)
						{
#if defined(__linux__)
							lLen += sprintf(out+lLen, "\t\tchunk [0x%X]: free_blocks=%zd, dirty_blocks=%zd\n", lCurrent, lCurrent->free_blocks_count, lCurrent->dirty_blocks_count);
#else
							lLen += sprintf(out+lLen, "\t\tchunk [0x%X]: free_blocks=%d, dirty_blocks=%d\n", lCurrent, lCurrent->free_blocks_count, lCurrent->dirty_blocks_count);
#endif
						}

						lCurrent = lCurrent->next_chunk;
					}
				}
			}
		}

		lLen += sprintf(out+lLen, "total_allocated=%llu\n", lTotals);

		_jm_mutex_unlock(&arena->mutex);
	}

	return lTotals;
}

JET_MALLOC_DEF void _jm_arena_print_stats(int arena_index, char* out, unsigned int options, int class_index)
{
	if (arena_index >= 0 && arena_index < JM_MAX_ARENAS && _jm_arenas[arena_index] != 0)
	{
		_jm_interlocked_increment(&_jm_arenas[arena_index]->stats_processing);
		if (!_jm_arenas[arena_index]->sweeping) _jm_arena_print_stats_internal(_jm_arenas[arena_index], out, options, class_index);
		_jm_interlocked_decrement(&_jm_arenas[arena_index]->stats_processing);
	}
}

JET_MALLOC_DEF void _jm_thread_print_stats(size_t thread_id, char* out, unsigned int options, int class_index)
{
	int lArenaIdx = 0;

	// lock arenas
	_jm_mutex_lock(&_jm_arenas_mutex);

	for(lArenaIdx = 0; lArenaIdx < _jm_arenas_max; lArenaIdx++)
	{
		if (_jm_arenas[lArenaIdx] != 0 && _jm_arenas[lArenaIdx]->owner == thread_id)
		{
			_jm_interlocked_increment(&_jm_arenas[lArenaIdx]->stats_processing);
			if (!_jm_arenas[lArenaIdx]->sweeping) _jm_arena_print_stats_internal(_jm_arenas[lArenaIdx], out, options, class_index);
			_jm_interlocked_decrement(&_jm_arenas[lArenaIdx]->stats_processing);
			break;
		}
	}

	// ulock arenas
	_jm_mutex_unlock(&_jm_arenas_mutex);
}

JET_MALLOC_DEF void _jm_arena_sweep_lock()
{
	struct _jm_arena* lArena = _jm_current_thread_arena; //get this from thread storage

	if (!lArena)
	{
		_jm_current_thread_arena = _jm_arena_alloc();
		lArena = _jm_current_thread_arena; 
	}

	_jm_interlocked_increment(&lArena->sweep_locked); // lock
}

JET_MALLOC_DEF void _jm_arena_sweep_unlock()
{
	struct _jm_arena* lArena = _jm_current_thread_arena; //get this from thread storage
	if (lArena) _jm_interlocked_decrement(&lArena->sweep_locked); // unlock
}

JET_MALLOC_DEF void _jm_arena_force_sweep()
{
	struct _jm_arena* lArena = _jm_current_thread_arena; //get this from thread storage
	if (lArena)
	{
		_jm_arena_clean_and_sweep(lArena, JM_FORCE); // sweep it!
	}
}

JET_MALLOC_DEF void _jm_arena_force_unlocked_sweep()
{
	struct _jm_arena* lArena = _jm_current_thread_arena; //get this from thread storage
	volatile size_t lSweepCount = 0;

	if (lArena)
	{
		lSweepCount = lArena->sweep_locked;
		if (lSweepCount) _jm_interlocked_exchange(&lArena->sweep_locked, 0); // unlock sweep

		_jm_arena_clean_and_sweep(lArena, JM_FORCE); // sweep it!

		if (lSweepCount) _jm_interlocked_exchange(&lArena->sweep_locked, lSweepCount); // lock back
	}
}

void _jm_arena_reset_stats_internal(struct _jm_arena* arena)
{
	memset(arena->total_chunks_count, 0, sizeof(size_t) * JM_ALLOC_CLASSES);
	memset(arena->dirty_blocks_count, 0, sizeof(size_t) * JM_ALLOC_CLASSES);
	memset(arena->dirty_chunks_removed, 0, sizeof(size_t) * JM_ALLOC_CLASSES);
}

JET_MALLOC_DEF void _jm_arena_reset_stats(int arena_index)
{
	if (arena_index >= 0 && arena_index < JM_MAX_ARENAS && _jm_arenas[arena_index] != 0)
	{
		_jm_arena_reset_stats_internal(_jm_arenas[arena_index]);
	}
}

JET_MALLOC_DEF void _jm_thread_reset_stats(size_t thread_id)
{
	int lArenaIdx = 0;

	// lock arenas
	_jm_mutex_lock(&_jm_arenas_mutex);

	for(lArenaIdx = 0; lArenaIdx < _jm_arenas_max; lArenaIdx++)
	{
		if (_jm_arenas[lArenaIdx] != 0 && _jm_arenas[lArenaIdx]->owner == thread_id)
		{
			_jm_arena_reset_stats_internal(_jm_arenas[lArenaIdx]);
			break;
		}
	}

	// ulock arenas
	_jm_mutex_unlock(&_jm_arenas_mutex);
}

JET_MALLOC_DEF void _jm_global_reset_stats()
{
	int lArenaIdx = 0;

	// lock arenas
	_jm_mutex_lock(&_jm_arenas_mutex);

	for(lArenaIdx = 0; lArenaIdx < _jm_arenas_max; lArenaIdx++)
	{
		if (_jm_arenas[lArenaIdx] != 0)
			_jm_arena_reset_stats_internal(_jm_arenas[lArenaIdx]);
	}

	// ulock arenas
	_jm_mutex_unlock(&_jm_arenas_mutex);
}

JET_MALLOC_DEF void _jm_threads_print_stats(char* out)
{
	int lArenaIdx = 0;
	ulonglong_t lTotal = 0;

	// lock arenas
	_jm_mutex_lock(&_jm_arenas_mutex);

	for(lArenaIdx = 0; lArenaIdx < _jm_arenas_max; lArenaIdx++)
	{
		if (_jm_arenas[lArenaIdx] != 0)
		{
			_jm_interlocked_increment(&_jm_arenas[lArenaIdx]->stats_processing);
			if (!_jm_arenas[lArenaIdx]->sweeping) lTotal += _jm_arena_print_stats_internal(_jm_arenas[lArenaIdx], out + strlen(out), JM_ARENA_TOTAL_STATS, JM_ALLOC_CLASSES);
			_jm_interlocked_decrement(&_jm_arenas[lArenaIdx]->stats_processing);
		}
	}

	sprintf(out + strlen(out), "total_allocated=%llu\n", lTotal);

	// ulock arenas
	_jm_mutex_unlock(&_jm_arenas_mutex);
}

int _jm_page_size = 0;

JET_MALLOC_DEF void _jm_initialize()
{
#if defined(__linux__)
	_jm_page_size = getpagesize();
#else
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	_jm_page_size = si.dwPageSize;
#endif

	_jm_mutex_init(&_jm_arenas_mutex);
}

#if defined(__linux__)
	#define JM_DUMP_FILE "%s/dump-%d-%d.bin"
#else
	#define JM_DUMP_FILE "%s\\dump-%d-%d.bin"
#endif

#if defined(__linux__)
	#include <sys/stat.h> 
	#include <fcntl.h>

	#define _jm_file_open(file) open(file, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
	#define _jm_file_close(h) close(h)
	#define _jm_file_write(h, buf, size) write(h, buf, size)
#else
	#include <fcntl.h>
	#include <io.h>
	#include <sys\types.h>
	#include <sys\stat.h>

	#define _jm_file_open(file) open(file, O_CREAT | O_RDWR, _S_IREAD | _S_IWRITE)
	#define _jm_file_close(h) close(h)
	#define _jm_file_write(h, buf, size) write(h, buf, size)
#endif

void _jm_arena_dump_chunk_internal(struct _jm_arena* arena, size_t chunk, int class_index, char* path)
{
	char lFile[0x200] = {0};
	char lInfo[0x200] = {0};
	
	int lHeader;
	size_t lIdx;
	
	struct _jm_free_block* lBlock;
	struct _jm_chunk* lCurrent = arena->root_chunks[class_index];

	sprintf(lFile, JM_DUMP_FILE, path, arena->owner, class_index);
	lHeader = _jm_file_open(lFile);

	while(lCurrent)
	{
		if (chunk == 0 || chunk == (size_t)lCurrent)
		{
			sprintf(lInfo, "\n[chunk = 0x%X, class = %d, block_size = %ld, blocks_count = %ld, free_blocks = %ld, dirty_blocks = %ld]\n", lCurrent, class_index, lCurrent->block_size, lCurrent->blocks_count, lCurrent->free_blocks_count, lCurrent->dirty_blocks_count);
			_jm_file_write(lHeader, lInfo, strlen(lInfo));
			for(lIdx = 0; lIdx < lCurrent->blocks_count; lIdx++)
			{
				lBlock = (struct _jm_free_block*)((unsigned char*)lCurrent->blocks + lIdx * sizeof(struct _jm_free_block));
				if (lBlock->next || lBlock->prev) continue;
				
				sprintf(lInfo, "\n[block = %ld]\n", lIdx);
				_jm_file_write(lHeader, lInfo, strlen(lInfo));

#if defined(JM_ALLOCATION_INFO)
				sprintf(lInfo, "%s - %s:%ld\n", lBlock->file, lBlock->func, lBlock->line);
				_jm_file_write(lHeader, lInfo, strlen(lInfo));
#endif

				_jm_file_write(lHeader, "<[", 2);
				_jm_file_write(lHeader, (unsigned char*)lBlock->block, lCurrent->block_size);
				_jm_file_write(lHeader, "]>", 2);
			}
		}

		lCurrent = lCurrent->next_chunk;
	}

	_jm_file_close(lHeader);
}

JET_MALLOC_DEF void _jm_thread_dump_chunk(size_t thread_id, size_t chunk, int class_index, char* path)
{
	int lArenaIdx = 0;

	// lock arenas
	_jm_mutex_lock(&_jm_arenas_mutex);

	for(lArenaIdx = 0; lArenaIdx < _jm_arenas_max; lArenaIdx++)
	{
		if (_jm_arenas[lArenaIdx] != 0 && _jm_arenas[lArenaIdx]->owner == thread_id)
		{
			_jm_interlocked_increment(&_jm_arenas[lArenaIdx]->stats_processing);
			if (!_jm_arenas[lArenaIdx]->sweeping) _jm_arena_dump_chunk_internal(_jm_arenas[lArenaIdx], chunk, class_index, path);
			_jm_interlocked_decrement(&_jm_arenas[lArenaIdx]->stats_processing);
			break;
		}
	}

	// ulock arenas
	_jm_mutex_unlock(&_jm_arenas_mutex);
}
