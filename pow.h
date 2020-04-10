#ifndef QBIT_POW_H
#define QBIT_POW_H

#include "./hash/cuckoo.h"
#include "block.h"

namespace qbit {

uint32_t GetNextWorkRequired(BlockHeader &current);

}

#endif
