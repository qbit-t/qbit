// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_SERVER_H
#define QBIT_UNITTEST_SERVER_H

#include <string>
#include <list>

#include "unittest.h"
#include "../itransactionstore.h"
#include "../iwallet.h"
#include "../ientitystore.h"
#include "../transactionvalidator.h"
#include "../transactionactions.h"
#include "../block.h"
#include "../iconsensus.h"

#include "../txassettype.h"
#include "../txassetemission.h"

#include "../isettings.h"

#include "../wallet.h"

#include "../transactionstore.h"

#include "../server.h"

#include "assets.h"
#include "wallet.h"
#include "memorypool.h"

#include "../memorypoolmanager.h"
#include "../transactionstoremanager.h"
#include "../peermanager.h"
#include "../consensusmanager.h"
#include "../validatormanager.h"

namespace qbit {
namespace tests {

class SettingsS0: public ISettings {
public:
	SettingsS0() {}

	std::string dataPath() { return "/tmp/.qbitS0"; }
	size_t keysCache() { return 0; }

	qunit_t maxFeeRate() { return QUNIT * 10; } // 10 qunits per byte	

	int serverPort() { return 31415; } // main net

	size_t threadPoolSize() { return 2; } // tread pool size

	uint32_t roles() { return State::PeerRoles::FULLNODE|State::PeerRoles::MINER; } // default role		
};

class SettingsS1: public ISettings {
public:
	SettingsS1() {}

	std::string dataPath() { return "/tmp/.qbitS1"; }
	size_t keysCache() { return 0; }

	qunit_t maxFeeRate() { return QUNIT * 10; } // 10 qunits per byte	

	int serverPort() { return 31416; } // main net	

	size_t threadPoolSize() { return 2; } // tread pool size

	uint32_t roles() { return State::PeerRoles::FULLNODE; } // default role	

	bool isMiner() { uint32_t lRoles(roles()); return (lRoles & State::PeerRoles::MINER) != 0; }
	bool isFullNode() { uint32_t lRoles(roles()); return (lRoles & State::PeerRoles::FULLNODE) != 0; }
};

class SettingsS2: public ISettings {
public:
	SettingsS2() {}

	std::string dataPath() { return "/tmp/.qbitS2"; }
	size_t keysCache() { return 0; }

	qunit_t maxFeeRate() { return QUNIT * 10; } // 10 qunits per byte	

	int serverPort() { return 31417; } // main net	

	size_t threadPoolSize() { return 2; } // tread pool size

	uint32_t roles() { return State::PeerRoles::FULLNODE; } // default role	

	bool isMiner() { uint32_t lRoles(roles()); return (lRoles & State::PeerRoles::MINER) != 0; }
	bool isFullNode() { uint32_t lRoles(roles()); return (lRoles & State::PeerRoles::FULLNODE) != 0; }
};

class ServerS0: public Unit {
public:
	ServerS0(): Unit("ServerS0") {

		settings_ = std::make_shared<SettingsS0>();

		// main working configuration
		wallet_ = Wallet::instance(settings_);
		consensusManager_ = ConsensusManager::instance(settings_);
		mempoolManager_ = MemoryPoolManager::instance(settings_);
		storeManager_ = TransactionStoreManager::instance(settings_);
		peerManager_ = PeerManager::instance(settings_, consensusManager_, mempoolManager_);		
		validatorManager_ = ValidatorManager::instance(settings_, consensusManager_, mempoolManager_, storeManager_);

		// store
		std::static_pointer_cast<TransactionStoreManager>(storeManager_)->setWallet(wallet_);
		// push main chain to store manager
		storeManager_->push(MainChain::id());

		// consensus
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setStoreManager(storeManager_);
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setValidatorManager(validatorManager_);
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setWallet(wallet_);
		// mempool
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setStoreManager(storeManager_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setConsensusManager(consensusManager_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setWallet(wallet_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setEntityStore(storeManager_->locate(MainChain::id())->entityStore());
		// wallet
		std::static_pointer_cast<Wallet>(wallet_)->setStoreManager(storeManager_);
		std::static_pointer_cast<Wallet>(wallet_)->setMemoryPoolManager(mempoolManager_);
		std::static_pointer_cast<Wallet>(wallet_)->setEntityStore(storeManager_->locate(MainChain::id())->entityStore());

		// push main chain
		consensusManager_->push(MainChain::id());
		mempoolManager_->push(MainChain::id());
		validatorManager_->push(MainChain::id());

		//
		server_ = Server::instance(settings_, peerManager_);

		seed_.push_back(std::string("fitness"));
		seed_.push_back(std::string("exchange"));
		seed_.push_back(std::string("glance"));
		seed_.push_back(std::string("diamond"));
		seed_.push_back(std::string("crystal"));
		seed_.push_back(std::string("cinnamon"));
		seed_.push_back(std::string("border"));
		seed_.push_back(std::string("arrange"));
		seed_.push_back(std::string("attitude"));
		seed_.push_back(std::string("between"));
		seed_.push_back(std::string("broccoli"));
		seed_.push_back(std::string("cannon"));
		seed_.push_back(std::string("crane"));
		seed_.push_back(std::string("double"));
		seed_.push_back(std::string("eyebrow"));
		seed_.push_back(std::string("frequent"));
		seed_.push_back(std::string("gravity"));
		seed_.push_back(std::string("hidden"));
		seed_.push_back(std::string("identify"));
		seed_.push_back(std::string("innocent"));
		seed_.push_back(std::string("jealous"));
		seed_.push_back(std::string("language"));
		seed_.push_back(std::string("leopard"));
		seed_.push_back(std::string("lobster"));
	}

	bool execute();

	ITransactionStoreManagerPtr storeManager_;
	IMemoryPoolManagerPtr mempoolManager_;
	IPeerManagerPtr peerManager_;
	IConsensusManagerPtr consensusManager_;
	IValidatorManagerPtr validatorManager_;
	ServerPtr server_;

	IWalletPtr wallet_;
	ISettingsPtr settings_;

	std::list<std::string> seed_;
};

class ServerS1: public Unit {
public:
	ServerS1(): Unit("ServerS1") {
		settings_ = std::make_shared<SettingsS1>();

		// main working configuration
		wallet_ = Wallet::instance(settings_);
		consensusManager_ = ConsensusManager::instance(settings_);
		mempoolManager_ = MemoryPoolManager::instance(settings_);
		storeManager_ = TransactionStoreManager::instance(settings_);
		peerManager_ = PeerManager::instance(settings_, consensusManager_, mempoolManager_);		
		validatorManager_ = ValidatorManager::instance(settings_, consensusManager_, mempoolManager_, storeManager_);

		// store
		std::static_pointer_cast<TransactionStoreManager>(storeManager_)->setWallet(wallet_);
		// push main chain to store manager
		storeManager_->push(MainChain::id());

		// consensus
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setStoreManager(storeManager_);
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setValidatorManager(validatorManager_);
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setWallet(wallet_);
		// mempool
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setStoreManager(storeManager_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setConsensusManager(consensusManager_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setWallet(wallet_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setEntityStore(storeManager_->locate(MainChain::id())->entityStore());
		// wallet
		std::static_pointer_cast<Wallet>(wallet_)->setStoreManager(storeManager_);
		std::static_pointer_cast<Wallet>(wallet_)->setMemoryPoolManager(mempoolManager_);
		std::static_pointer_cast<Wallet>(wallet_)->setEntityStore(storeManager_->locate(MainChain::id())->entityStore());

		// push main chain
		consensusManager_->push(MainChain::id());
		mempoolManager_->push(MainChain::id());
		validatorManager_->push(MainChain::id());

		//
		server_ = Server::instance(settings_, peerManager_);

		seed_.push_back(std::string("fitness"));
		seed_.push_back(std::string("exchange"));
		seed_.push_back(std::string("glance"));
		seed_.push_back(std::string("diamond"));
		seed_.push_back(std::string("crystal"));
		seed_.push_back(std::string("cinnamon"));
		seed_.push_back(std::string("border"));
		seed_.push_back(std::string("arrange"));
		seed_.push_back(std::string("attitude"));
		seed_.push_back(std::string("between"));
		seed_.push_back(std::string("broccoli"));
		seed_.push_back(std::string("cannon"));
		seed_.push_back(std::string("crane"));
		seed_.push_back(std::string("double"));
		seed_.push_back(std::string("eyebrow"));
		seed_.push_back(std::string("frequent"));
		seed_.push_back(std::string("gravity"));
		seed_.push_back(std::string("hidden"));
		seed_.push_back(std::string("identify"));
		seed_.push_back(std::string("innocent"));
		seed_.push_back(std::string("jealous"));
		seed_.push_back(std::string("language"));
		seed_.push_back(std::string("leopard"));
		seed_.push_back(std::string("catfish"));		
	}

	bool execute();

	ITransactionStoreManagerPtr storeManager_;
	IMemoryPoolManagerPtr mempoolManager_;
	IPeerManagerPtr peerManager_;
	IConsensusManagerPtr consensusManager_;
	IValidatorManagerPtr validatorManager_;
	ServerPtr server_;

	IWalletPtr wallet_;
	ISettingsPtr settings_;

	std::list<std::string> seed_;
};

class ServerS2: public Unit {
public:
	ServerS2(): Unit("ServerS2") {
		settings_ = std::make_shared<SettingsS2>();

		// main working configuration
		wallet_ = Wallet::instance(settings_);
		consensusManager_ = ConsensusManager::instance(settings_);
		mempoolManager_ = MemoryPoolManager::instance(settings_);
		storeManager_ = TransactionStoreManager::instance(settings_);
		peerManager_ = PeerManager::instance(settings_, consensusManager_, mempoolManager_);		
		validatorManager_ = ValidatorManager::instance(settings_, consensusManager_, mempoolManager_, storeManager_);

		// store
		std::static_pointer_cast<TransactionStoreManager>(storeManager_)->setWallet(wallet_);
		// push main chain to store manager
		storeManager_->push(MainChain::id());

		// consensus
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setStoreManager(storeManager_);
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setValidatorManager(validatorManager_);
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setWallet(wallet_);
		// mempool
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setStoreManager(storeManager_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setConsensusManager(consensusManager_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setWallet(wallet_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setEntityStore(storeManager_->locate(MainChain::id())->entityStore());
		// wallet
		std::static_pointer_cast<Wallet>(wallet_)->setStoreManager(storeManager_);
		std::static_pointer_cast<Wallet>(wallet_)->setMemoryPoolManager(mempoolManager_);
		std::static_pointer_cast<Wallet>(wallet_)->setEntityStore(storeManager_->locate(MainChain::id())->entityStore());

		// push main chain
		consensusManager_->push(MainChain::id());
		mempoolManager_->push(MainChain::id());
		validatorManager_->push(MainChain::id());

		//
		server_ = Server::instance(settings_, peerManager_);

		seed_.push_back(std::string("fitness"));
		seed_.push_back(std::string("exchange"));
		seed_.push_back(std::string("glance"));
		seed_.push_back(std::string("diamond"));
		seed_.push_back(std::string("crystal"));
		seed_.push_back(std::string("cinnamon"));
		seed_.push_back(std::string("border"));
		seed_.push_back(std::string("arrange"));
		seed_.push_back(std::string("attitude"));
		seed_.push_back(std::string("between"));
		seed_.push_back(std::string("broccoli"));
		seed_.push_back(std::string("cannon"));
		seed_.push_back(std::string("crane"));
		seed_.push_back(std::string("double"));
		seed_.push_back(std::string("eyebrow"));
		seed_.push_back(std::string("frequent"));
		seed_.push_back(std::string("gravity"));
		seed_.push_back(std::string("hidden"));
		seed_.push_back(std::string("identify"));
		seed_.push_back(std::string("innocent"));
		seed_.push_back(std::string("jealous"));
		seed_.push_back(std::string("language"));
		seed_.push_back(std::string("leopard"));
		seed_.push_back(std::string("rakoon"));		
	}

	bool execute();

	ITransactionStoreManagerPtr storeManager_;
	IMemoryPoolManagerPtr mempoolManager_;
	IPeerManagerPtr peerManager_;
	IConsensusManagerPtr consensusManager_;
	IValidatorManagerPtr validatorManager_;
	ServerPtr server_;

	IWalletPtr wallet_;
	ISettingsPtr settings_;

	std::list<std::string> seed_;
};

}
}

#endif