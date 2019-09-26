// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_MEMORYPOOL_H
#define QBIT_UNITTEST_MEMORYPOOL_H

#include <string>
#include <list>

#include "unittest.h"
#include "../itransactionstore.h"
#include "../iwallet.h"
#include "../ientitystore.h"
#include "../transactionvalidator.h"
#include "../transactionactions.h"
#include "../block.h"

#include "../txassettype.h"
#include "../txassetemission.h"

#include "../isettings.h"
#include "../iconsensus.h"

#include "../wallet.h"
#include "../memorypool.h"

#include "wallet.h"

namespace qbit {
namespace tests {

class ConsensusA: public IConsensus {
public:
	ConsensusA() {}

	size_t getMaxBlockSize() { return 1024 * 1024 * 8; }
};	

class MemoryPoolQbitCreateSpend: public Unit {
public:
	MemoryPoolQbitCreateSpend(): Unit("MemoryPoolQbitCreateSpend") {
		persistentStore_ = std::make_shared<TxStoreA>(); 
		entityStore_ = std::make_shared<EntityStoreA>(); 
		settings_ = std::make_shared<SettingsA>(); 
		consensus_ = std::make_shared<ConsensusA>(); 

		mempool_ = MemoryPool::instance(consensus_, persistentStore_, entityStore_); 
		wallet_  = Wallet::instance(settings_, mempool_, entityStore_); 
		std::static_pointer_cast<Wallet>(wallet_)->setTransactionStore(persistentStore_);
		std::static_pointer_cast<MemoryPool>(mempool_)->setWallet(wallet_);

		seedMy_.push_back(std::string("fitness"));
		seedMy_.push_back(std::string("exchange"));
		seedMy_.push_back(std::string("glance"));
		seedMy_.push_back(std::string("diamond"));
		seedMy_.push_back(std::string("crystal"));
		seedMy_.push_back(std::string("cinnamon"));
		seedMy_.push_back(std::string("border"));
		seedMy_.push_back(std::string("arrange"));
		seedMy_.push_back(std::string("attitude"));
		seedMy_.push_back(std::string("between"));
		seedMy_.push_back(std::string("broccoli"));
		seedMy_.push_back(std::string("cannon"));
		seedMy_.push_back(std::string("crane"));
		seedMy_.push_back(std::string("double"));
		seedMy_.push_back(std::string("eyebrow"));
		seedMy_.push_back(std::string("frequent"));
		seedMy_.push_back(std::string("gravity"));
		seedMy_.push_back(std::string("hidden"));
		seedMy_.push_back(std::string("identify"));
		seedMy_.push_back(std::string("innocent"));
		seedMy_.push_back(std::string("jealous"));
		seedMy_.push_back(std::string("language"));
		seedMy_.push_back(std::string("leopard"));
		seedMy_.push_back(std::string("lobster"));		

		seedBob_.push_back(std::string("fitness"));
		seedBob_.push_back(std::string("exchange"));
		seedBob_.push_back(std::string("glance"));
		seedBob_.push_back(std::string("diamond"));
		seedBob_.push_back(std::string("crystal"));
		seedBob_.push_back(std::string("cinnamon"));
		seedBob_.push_back(std::string("border"));
		seedBob_.push_back(std::string("arrange"));
		seedBob_.push_back(std::string("attitude"));
		seedBob_.push_back(std::string("between"));
		seedBob_.push_back(std::string("broccoli"));
		seedBob_.push_back(std::string("cannon"));
		seedBob_.push_back(std::string("crane"));
		seedBob_.push_back(std::string("double"));
		seedBob_.push_back(std::string("eyebrow"));
		seedBob_.push_back(std::string("frequent"));
		seedBob_.push_back(std::string("gravity"));
		seedBob_.push_back(std::string("hidden"));
		seedBob_.push_back(std::string("identify"));
		seedBob_.push_back(std::string("innocent"));
		seedBob_.push_back(std::string("jealous"));
		seedBob_.push_back(std::string("language"));
		seedBob_.push_back(std::string("leopard"));
		seedBob_.push_back(std::string("catfish"));		
	}

	bool execute();

	ITransactionStorePtr persistentStore_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
	IMemoryPoolPtr mempool_;
	ISettingsPtr settings_;
	IConsensusPtr consensus_;

	std::list<std::string> seedMy_;
	std::list<std::string> seedBob_;
};

}
}

#endif
