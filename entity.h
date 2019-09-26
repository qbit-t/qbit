// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ENTITY_H
#define QBIT_ENTITY_H

#include "transaction.h"

namespace qbit {

class Entity : public TxSpend {
public:
	inline static std::string emptyName() { return "empty"; }
public:
	Entity() {}

	virtual bool isValue(UnlinkedOutPtr) { return false; }
	virtual bool isEntity(UnlinkedOutPtr) { return true; }		
	virtual bool isEntity() { return true; }		
	virtual std::string entityName() { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	virtual inline void setChain(const uint256&) { chain_ = MainChain::id(); /* all entities live in mainchain */ }
};

typedef std::shared_ptr<Entity> EntityPtr;

}

#endif
