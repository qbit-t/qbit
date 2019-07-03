#ifndef __JM_BACKEND__
#define __JM_BACKEND__

#include "jet_malloc_defs.h"

//
// pages mgmt
//

unsigned char* _jm_backend_alloc(unsigned char* base, size_t size);
void _jm_backend_free(unsigned char* address, size_t size);

#endif
