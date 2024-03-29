#include "allocator.h"

#if defined(JM_MALLOC)

void* operator new(size_t sz)
{
	return jm_malloc(sz);
}

void* operator new[](size_t sz)
{
	return jm_malloc(sz);
}

void operator delete(void* ptr) throw()
{
	jm_free(ptr);
}

void operator delete[](void* ptr) throw()
{
	jm_free(ptr);
}

void* operator new(size_t sz, const std::nothrow_t&) throw()
{
	return jm_malloc(sz);
}

void* operator new[](std::size_t sz, const std::nothrow_t&) throw()
{
	return jm_malloc(sz);
}

void operator delete(void* ptr, const std::nothrow_t&) throw()
{
	jm_free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) throw()
{
	jm_free(ptr);
}

void std::_jm_free(void* address) {
	jm_free(address);
}

#endif