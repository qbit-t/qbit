#ifndef QBIT_POW_H
#define QBIT_POW_H

#include "./hash/cuckoo.h"
#include "block.h"
#include "itransactionstore.h"

namespace qbit {

uint32_t GetNextWorkRequired(ITransactionStorePtr store, BlockHeader &current);

}

#endif
