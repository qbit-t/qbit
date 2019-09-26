// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IENTITY_STORE_H
#define QBIT_IENTITY_STORE_H

#include "transaction.h"
#include "transactioncontext.h"
#include "entity.h"

#include <memory>

namespace qbit {

class IEntityStore {
public:
	IEntityStore() {}

	virtual EntityPtr locateEntity(const uint256&) { return nullptr; }
	virtual bool pushEntity(const uint256&, TransactionContextPtr) { return false; }
};

typedef std::shared_ptr<IEntityStore> IEntityStorePtr;

} // qbit

#endif
