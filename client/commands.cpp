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

	SKeyPtr lKey;
	if (lAddress.size()) {
		PKey lPKey; 
		if (!lPKey.fromString(lAddress)) {
			throw qbit::exception("E_PKEY_INVALID", "Public key is invalid.");
		}

		lKey = wallet_->findKey(lPKey);
		if (!lKey || !lKey->valid()) {
			throw qbit::exception("E_SKEY_NOT_FOUND", "Key was not found."); 
		}
	} else {
		lKey = wallet_->firstKey();
		if (!lKey->valid()) {
			throw qbit::exception("E_SKEY_IS_ABSENT", "Key is absent."); 
		}
	}

	PKey lPFoundKey = lKey->createPKey();

	std::cout << "address = " << lPFoundKey.toString() << std::endl;
	std::cout << "id      = " << lPFoundKey.id().toHex() << std::endl;
	std::cout << "pkey    = " << lPFoundKey.toHex() << std::endl;
	std::cout << "skey    = " << lKey->toHex() << std::endl;
	std::cout << "seed    ->" << std::endl;

	for (std::vector<SKey::Word>::iterator lWord = lKey->seed().begin(); lWord != lKey->seed().end(); lWord++) {
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
		boost::bind(&BalanceCommand::balance, shared_from_this(), _1, _2, _3));
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

void SearchEntityNamesCommand::process(const std::vector<std::string>& args) {
	// process
	if (args.size() == 1) {
		//
		composer_->requestProcessor()->selectEntityNames(args[0], 
			SelectEntityNames::instance(
				boost::bind(&SearchEntityNamesCommand::assetNamesLoaded, shared_from_this(), _1, _2),
				boost::bind(&SearchEntityNamesCommand::timeout, shared_from_this()))
		);		
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments.");
	}
}

void AskForQbitsCommand::process(const std::vector<std::string>& args) {
	// process
	if (!composer_->requestProcessor()->askForQbits())
		error("E_ASK_FOR_QBITS", "There is no peers able to process your request.");

	done_(ProcessingError());
}


void LoadTransactionCommand::process(const std::vector<std::string>& args) {
	//
	// process
	if (args.size() == 1) {
		//
		uint256 lTx;
		uint256 lChain;

		//
		std::vector<std::string> lParts;
		boost::split(lParts, args[0], boost::is_any_of("/"));

		if (lParts.size() < 2) {
			error("E_LOAD_TRANSACTION", "Transaction info is incorrect");
			return;	
		}

		lTx.setHex(lParts[0]);
		lChain.setHex(lParts[1]);

		if (!composer_->requestProcessor()->loadTransaction(lChain, lTx, 
				LoadTransaction::instance(
					boost::bind(&LoadTransactionCommand::transactionLoaded, shared_from_this(), _1),
					boost::bind(&LoadTransactionCommand::timeout, shared_from_this()))
			))	error("E_TRANSACTION_NOT_LOADED", "Transaction is not loaded.");
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments.");
	}
}
