// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ENTITY_H
#define QBIT_ENTITY_H

#include "transaction.h"

namespace qbit {

class Entity : public TxSpend {
public:
	Entity() {}
};

typedef std::shared_ptr<Entity> EntityPtr;

}

#endif
