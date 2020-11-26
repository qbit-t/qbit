#ifndef QBIT_POW_H
#define QBIT_POW_H

#include "./hash/cuckoo.h"
#include "block.h"

namespace qbit {

class ITransactionStore;
typedef std::shared_ptr<ITransactionStore> ITransactionStorePtr;

uint32_t getNextWorkRequired(ITransactionStorePtr store, BlockPtr current, uint64_t blockTime);

}

#endif
