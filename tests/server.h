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
#include "../peermanager.h"

#include "assets.h"
#include "wallet.h"
#include "memorypool.h"

namespace qbit {
namespace tests {

class ConsensusB: public IConsensus {
public:
	ConsensusB() {}

	size_t maxBlockSize() { return 1024 * 1024 * 8; }
	uint64_t getTime() { return getTime(); }

	void pushPeer(const std::string /*endpoint*/, IPeerPtr /*peer*/) { }
	void popPeer(const std::string /*endpoint*/, IPeerPtr /*peer*/) { }

	StatePtr currentState() {
		StatePtr lState = State::instance(qbit::getTime(), wallet_->firstKey().createPKey());

		// TODO: add last block and height by chain
		return lState;
	}

	SKey mainKey() {
		return wallet_->firstKey();
	}

	void setWallet(IWalletPtr wallet) { wallet_ = wallet; } 

	size_t quarantineTime() { return 100; }

private:
	IWalletPtr wallet_;
};	

class SettingsB: public ISettings {
public:
	SettingsB() {}

	std::string dataPath() { return "/tmp/.qbit"; }
	size_t keysCache() { return 0; }

	qunit_t maxFeeRate() { return QUNIT * 10; } // 10 qunits per byte	

	int serverPort() { return 31415; } // main net

	size_t threadPoolSize() { return 2; } // tread pool size
};

class SettingsC: public ISettings {
public:
	SettingsC() {}

	std::string dataPath() { return "/tmp/.qbit1"; }
	size_t keysCache() { return 0; }

	qunit_t maxFeeRate() { return QUNIT * 10; } // 10 qunits per byte	

	int serverPort() { return 31416; } // main net	

	size_t threadPoolSize() { return 2; } // tread pool size
};

class ServerA: public Unit {
public:
	ServerA(): Unit("ServerA") {
		settings_ = std::make_shared<SettingsB>();
		consensus_ = std::make_shared<ConsensusB>();

		store_ = TransactionStore::instance(settings_, consensus_);
		entityStore_ = store_->entityStore();

		mempool_ = MemoryPool::instance(consensus_, store_, entityStore_); 
		wallet_ = Wallet::instance(settings_, mempool_, entityStore_);

		std::static_pointer_cast<MemoryPool>(mempool_)->setWallet(wallet_);
		std::static_pointer_cast<TransactionStore>(store_)->setWallet(wallet_);
		std::static_pointer_cast<Wallet>(wallet_)->setTransactionStore(store_);

		std::static_pointer_cast<ConsensusB>(consensus_)->setWallet(wallet_);

		peerManager_ = PeerManager::instance(settings_, consensus_, mempool_);
		server_ = Server::instance(settings_, consensus_, peerManager_);

		// 31415!!!
		peerManager_->addPeer("192.168.0.72:31416");
		peerManager_->addPeer("192.168.0.72:31415");
		//peerManager_->addPeer("192.168.1.49:31416");
		//peerManager_->addPeer("192.168.1.49:31415");

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

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
	IMemoryPoolPtr mempool_;
	ISettingsPtr settings_;
	IConsensusPtr consensus_;

	IPeerManagerPtr peerManager_;
	ServerPtr server_;

	std::list<std::string> seed_;
};

class ServerB: public Unit {
public:
	ServerB(): Unit("ServerB") {
		settings_ = std::make_shared<SettingsC>();
		consensus_ = std::make_shared<ConsensusB>();

		store_ = TransactionStore::instance(settings_, consensus_);
		entityStore_ = store_->entityStore();

		mempool_ = MemoryPool::instance(consensus_, store_, entityStore_); 
		wallet_ = Wallet::instance(settings_, mempool_, entityStore_);

		std::static_pointer_cast<MemoryPool>(mempool_)->setWallet(wallet_);
		std::static_pointer_cast<TransactionStore>(store_)->setWallet(wallet_);
		std::static_pointer_cast<Wallet>(wallet_)->setTransactionStore(store_);

		std::static_pointer_cast<ConsensusB>(consensus_)->setWallet(wallet_);

		peerManager_ = PeerManager::instance(settings_, consensus_, mempool_);
		server_ = Server::instance(settings_, consensus_, peerManager_);

		// 31416!!!
		peerManager_->addPeer("192.168.0.72:31415");
		peerManager_->addPeer("192.168.0.72:31416");
		//peerManager_->addPeer("192.168.1.49:31415");		
		//peerManager_->addPeer("192.168.1.49:31416");

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

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;
	IMemoryPoolPtr mempool_;
	ISettingsPtr settings_;
	IConsensusPtr consensus_;

	IPeerManagerPtr peerManager_;
	ServerPtr server_;

	std::list<std::string> seed_;
};

}
}

#endif
