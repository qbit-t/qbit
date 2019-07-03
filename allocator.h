#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <new>
#include "jm/include/jet_malloc.h"

#define malloc(a) jm_malloc(a)
#define calloc(a, b) jm_calloc(a, b)
#define realloc(a, b) jm_realloc(a, b)
#define free(a) jm_free(a)

void* operator new(size_t sz);
void* operator new[](size_t sz);
void  operator delete(void* ptr) throw();
void  operator delete[](void* ptr) throw();
void* operator new(size_t sz, const std::nothrow_t&) throw();
void* operator new[](std::size_t sz, const std::nothrow_t&) throw();
void  operator delete(void* ptr, const std::nothrow_t&) throw();
void  operator delete[](void* ptr, const std::nothrow_t&) throw();

#endif // ALLOCATOR_H
