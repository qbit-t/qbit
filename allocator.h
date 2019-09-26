// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#if defined(JM_MALLOC)

#include <new>
#include "jm/include/jet_malloc.h"

namespace std {
void _jm_free(void* address);
}

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

#endif

#endif // ALLOCATOR_H
