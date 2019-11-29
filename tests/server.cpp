#include "server.h"
#include "../mkpath.h"

using namespace qbit;
using namespace qbit::tests;

bool ServerA::execute() {
	// clean up
	rmpath(settings_->dataPath().c_str());

	// prepare wallet
	if (!wallet_->open()) {
		error_  = "Wallet open failed.";
		return false;
	}

	// prepare store
	if (!store_->open()) {
		error_  = "Storage open failed.";
		return false;
	}

	wallet_->createKey(seed_);

	//peerManager_->run();
	server_->run();

	return true;
}

bool ServerB::execute() {
	// clean up
	rmpath(settings_->dataPath().c_str());

	// prepare wallet
	if (!wallet_->open()) {
		error_  = "Wallet open failed.";
		return false;
	}

	// prepare store
	if (!store_->open()) {
		error_  = "Storage open failed.";
		return false;
	}

	wallet_->createKey(seed_);

	//peerManager_->run();
	server_->run();

	return true;
}
