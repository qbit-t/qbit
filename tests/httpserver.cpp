#include "httpserver.h"

using namespace qbit;
using namespace qbit::tests;

bool ServerHttp::execute() {
	// prepare wallet
	if (!wallet_->open("")) {
		error_  = "Wallet open failed.";
		return false;
	}

	server_->runWait();
	return true;
}
