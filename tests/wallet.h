// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_WALLET_H
#define QBIT_UNITTEST_WALLET_H

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

#include "../wallet.h"

#include "assets.h"

namespace qbit {
namespace tests {

class SettingsA: public ISettings {
public:
	SettingsA() {}

	std::string dataPath() { return "/tmp/.qbit"; }
	size_t keysCache() { return 0; }

	qunit_t maxFeeRate() { return QUNIT * 10; } // 10 qunits per byte	
};

class MemoryPoolA: public IMemoryPool {
public:
	MemoryPoolA(IConsensusPtr consensus) : consensus_(consensus) {}	
	MemoryPoolA() {}

	qunit_t estimateFeeRateByLimit(TransactionContextPtr ctx, qunit_t feeRateLimit) {
		return QUNIT * 1;
	}

	qunit_t estimateFeeRateByBlock(TransactionContextPtr ctx, uint32_t targetBlock) {
		return QUNIT * 5;
	}

	bool isUnlinkedOutUsed(const uint256&) { return false; }
	bool isUnlinkedOutExists(const uint256&) { return false; }

	IConsensusPtr consensus() { return consensus_; }

	IConsensusPtr consensus_;
};

class WalletQbitCreateSpend: public Unit {
public:
	WalletQbitCreateSpend(): Unit("WalletQbitCreateSpend") {
		consensus_ = std::make_shared<ConsensusAA>();
		store_ = std::make_shared<TxStoreA>(); 
		entityStore_ = std::make_shared<EntityStoreA>(); 
		mempool_ = std::make_shared<MemoryPoolA>(consensus_);
		settings_ = std::make_shared<SettingsA>(); 

		wallet_ = Wallet::instance(settings_, mempool_, entityStore_);
		std::static_pointer_cast<Wallet>(wallet_)->setTransactionStore(store_);

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

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
	IMemoryPoolPtr mempool_;
	ISettingsPtr settings_;
	IConsensusPtr consensus_;

	std::list<std::string> seedMy_;
	std::list<std::string> seedBob_;
};

class WalletQbitCreateSpendRollback: public Unit {
public:
	WalletQbitCreateSpendRollback(): Unit("WalletQbitCreateSpendRollback") {
		store_ = std::make_shared<TxStoreA>(); 
		entityStore_ = std::make_shared<EntityStoreA>(); 
		mempool_ = std::make_shared<MemoryPoolA>(); 
		settings_ = std::make_shared<SettingsA>(); 

		wallet_ = Wallet::instance(settings_, mempool_, entityStore_); 
		std::static_pointer_cast<Wallet>(wallet_)->setTransactionStore(store_);

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

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
	IMemoryPoolPtr mempool_;
	ISettingsPtr settings_;

	std::list<std::string> seedMy_;
	std::list<std::string> seedBob_;
};

class WalletAssetCreateAndSpend: public Unit {
public:
	WalletAssetCreateAndSpend(): Unit("WalletAssetCreateAndSpend") {
		store_ = std::make_shared<TxStoreA>(); 
		entityStore_ = std::make_shared<EntityStoreA>(); 
		mempool_ = std::make_shared<MemoryPoolA>(); 
		settings_ = std::make_shared<SettingsA>(); 

		wallet_ = Wallet::instance(settings_, mempool_, entityStore_);
		std::static_pointer_cast<Wallet>(wallet_)->setTransactionStore(store_);

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

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
	IMemoryPoolPtr mempool_;
	ISettingsPtr settings_;

	std::list<std::string> seedMy_;
	std::list<std::string> seedBob_;
};

}
}

#endif
