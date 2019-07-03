
#include "include/jm_backend.h"

#ifdef WIN32
	#include <windows.h>
#elif defined(__linux__)
	#include <sys/mman.h>
#endif

unsigned char* _jm_backend_alloc(unsigned char* base, size_t size)
{
#ifdef WIN32
	void* lAddress = VirtualAlloc(base, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return (unsigned char*)lAddress;
#elif defined(__linux__)
	void* lAddress = 0;
	//size = !(size % _jm_page_size) ? size : size - (size % _jm_page_size) + _jm_page_size;
	lAddress = mmap(base, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	return (unsigned char*)lAddress;
#endif
	return 0;
}

void _jm_backend_free(unsigned char* address, size_t size)
{
#ifdef WIN32
	VirtualFree(address, 0, MEM_RELEASE);
#elif defined(__linux__)
	munmap(address, size);
#endif
}
