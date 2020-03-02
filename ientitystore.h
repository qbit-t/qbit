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
	virtual EntityPtr locateEntity(const std::string&) { return nullptr; }
	virtual bool pushEntity(const uint256&, TransactionContextPtr) { return false; }
	virtual bool collectUtxoByEntityName(const std::string& /*name*/, std::vector<Transaction::UnlinkedOutPtr>& /*result*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::collectUtxoByEntityName - not implemented."); }	
	virtual bool entityCountByShards(const std::string& /*name*/, std::map<uint32_t, uint256>& /*result*/) { throw qbit::exception("NOT_IMPL", "ITransactionStore::collectUtxoByEntityName - not implemented."); }	
};

typedef std::shared_ptr<IEntityStore> IEntityStorePtr;

} // qbit

#endif
