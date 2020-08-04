// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MKPATH_H
#define MKPATH_H
//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"

#include <sys/types.h>

int mkpath(const char *path, mode_t mode);
int rmpath(const char *path);

#endif
