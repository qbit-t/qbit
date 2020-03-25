#include "commands.h"
#include "../txassettype.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace qbit;

void KeyCommand::process(const std::vector<std::string>& args) {
	//
	std::string lAddress;
	if (args.size()) {
		lAddress = args[0];
	}

	SKey lKey;
	if (lAddress.size()) {
		PKey lPKey; 
		if (!lPKey.fromString(lAddress)) {
			throw qbit::exception("E_PKEY_INVALID", "Public key is invalid.");
		}

		lKey = wallet_->findKey(lPKey);
		if (!lKey.valid()) {
			throw qbit::exception("E_SKEY_NOT_FOUND", "Key was not found."); 
		}
	} else {
		lKey = wallet_->firstKey();
		if (!lKey.valid()) {
			throw qbit::exception("E_SKEY_IS_ABSENT", "Key is absent."); 
		}
	}

	PKey lPFoundKey = lKey.createPKey();

	std::cout << "address = " << lPFoundKey.toString() << std::endl;
	std::cout << "id      = " << lPFoundKey.id().toHex() << std::endl;
	std::cout << "pkey    = " << lPFoundKey.toHex() << std::endl;
	std::cout << "skey    = " << lKey.toHex() << std::endl;
	std::cout << "seed    ->" << std::endl;

	for (std::vector<SKey::Word>::iterator lWord = lKey.seed().begin(); lWord != lKey.seed().end(); lWord++) {
		std::cout << "\t" << (*lWord).wordA() << std::endl;
	}

	done_();
}

void BalanceCommand::process(const std::vector<std::string>& args) {
	//
	uint256 lAsset;
	if (args.size()) {
		lAsset.setHex(args[0]);
	}

	// prepare
	IComposerMethodPtr lBalance = LightComposer::Balance::instance(composer_, lAsset, 
		boost::bind(&BalanceCommand::balance, shared_from_this(), _1, _2));
	// async process
	lBalance->process(boost::bind(&BalanceCommand::error, shared_from_this(), _1, _2));
}

void SendToAddressCommand::process(const std::vector<std::string>& args) {
	// process
	if (args.size() == 3) {
		//
		amount_t lScale = QBIT;
		// 0
		uint256 lAsset = TxAssetType::qbitAsset();
		if (args[0] != "*") lAsset.setHex(args[0]);

		// 1
		PKey lAddress;
		lAddress.fromString(args[1]);

		// 2
		double lAmount = (double)(boost::lexical_cast<double>(args[2]));

		// prepare
		IComposerMethodPtr lSendToAddress = LightComposer::SendToAddress::instance(composer_, lAsset, lAddress, lAmount,
			boost::bind(&SendToAddressCommand::created, shared_from_this(), _1));
		// async process
		lSendToAddress->process(boost::bind(&SendToAddressCommand::error, shared_from_this(), _1, _2));
		
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}
