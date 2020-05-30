#include "server.h"
#include "../mkpath.h"

using namespace qbit;
using namespace qbit::tests;

bool ServerS0::execute() {
	// clean up
	// rmpath(settings_->dataPath().c_str());

	// prepare main store
	if (!storeManager_->locate(MainChain::id())->open()) {
		error_  = "Storage open failed.";
		return false;
	}

	// prepare wallet
	if (!wallet_->open()) {
		error_  = "Wallet open failed.";
		return false;
	}

	wallet_->createKey(seed_);

	peerManager_->addPeerExplicit("127.0.0.1:31415");
	peerManager_->addPeerExplicit("127.0.0.1:31416");
	peerManager_->addPeerExplicit("127.0.0.1:31417");
	//peerManager_->addPeer("127.0.0.1:31418");

	validatorManager_->run();
	shardingManager_->run();
	httpServer_->run();
	server_->run();

	return true;
}

bool ServerS1::execute() {
	// clean up
	// rmpath(settings_->dataPath().c_str());

	// prepare store
	if (!storeManager_->locate(MainChain::id())->open()) {
		error_  = "Storage open failed.";
		return false;
	}

	// prepare wallet
	if (!wallet_->open()) {
		error_  = "Wallet open failed.";
		return false;
	}

	wallet_->createKey(seed_);

	peerManager_->addPeerExplicit("127.0.0.1:31415");
	peerManager_->addPeerExplicit("127.0.0.1:31416");
	peerManager_->addPeerExplicit("127.0.0.1:31417");
	//peerManager_->addPeer("127.0.0.1:31418");

	validatorManager_->run();
	shardingManager_->run();
	httpServer_->run();
	server_->run();

	return true;
}

bool ServerS2::execute() {
	// clean up
	// rmpath(settings_->dataPath().c_str());

	// prepare store
	if (!storeManager_->locate(MainChain::id())->open()) {
		error_  = "Storage open failed.";
		return false;
	}

	// prepare wallet
	if (!wallet_->open()) {
		error_  = "Wallet open failed.";
		return false;
	}

	wallet_->createKey(seed_);

	peerManager_->addPeerExplicit("127.0.0.1:31415");
	peerManager_->addPeerExplicit("127.0.0.1:31416");
	peerManager_->addPeerExplicit("127.0.0.1:31417");
	//peerManager_->addPeer("127.0.0.1:31418");

	validatorManager_->run();
	shardingManager_->run();
	httpServer_->run();
	server_->run();

	return true;
}

bool ServerS3::execute() {
	// clean up
	// rmpath(settings_->dataPath().c_str());

	// prepare store
	if (!storeManager_->locate(MainChain::id())->open()) {
		error_  = "Storage open failed.";
		return false;
	}

	// prepare wallet
	if (!wallet_->open()) {
		error_  = "Wallet open failed.";
		return false;
	}

	wallet_->createKey(seed_);

	peerManager_->addPeerExplicit("127.0.0.1:31415");
	peerManager_->addPeerExplicit("127.0.0.1:31416");
	peerManager_->addPeerExplicit("127.0.0.1:31417");
	peerManager_->addPeerExplicit("127.0.0.1:31418");

	validatorManager_->run();
	shardingManager_->run();
	httpServer_->run();
	server_->run();

	return true;
}
