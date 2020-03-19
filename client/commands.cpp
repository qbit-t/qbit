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
}

void BalanceCommand::process(const std::vector<std::string>& args) {
	//
	if (args.size()) {
		// load asset tx
		uint256 lAsset; lAsset.setHex(args[0]);
		requestProcessor_->loadTransaction(MainChain::id(), lAsset, LoadAssetType::instance(shared_from_this()));
	} else {
		assetTypeLoaded(nullptr);
	}

}

void BalanceCommand::assetTypeLoaded(TransactionPtr asset) {
	//
	amount_t lScale = QBIT;
	uint256 lAsset = TxAssetType::qbitAsset();

	if (asset) {
		if (asset->type() == Transaction::ASSET_TYPE) {
			TxAssetTypePtr lAssetType = TransactionHelper::to<TxAssetType>(asset);
			lScale = lAssetType->scale();			
		}
	}

	double lBalance = ((double)wallet_->balance(lAsset)) / lScale;
	std::cout << strprintf(TxAssetType::scaleFormat(lScale), lBalance) << std::endl;
}

void SendToAddressCommand::process(const std::vector<std::string>& args) {
	//
	// copy args
	args_ = args;

	// process
	if (args.size() == 3) {
		// 0 - asset or *
		// 1 - address
		// 2 - amount

		if (args[0] != "*") {
			uint256 lAsset; lAsset.setHex(args[0]);
			requestProcessor_->loadTransaction(MainChain::id(), lAsset, LoadAssetType::instance(shared_from_this()));
		} else {
			assetTypeLoaded(nullptr);
		}
		
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}

void SendToAddressCommand::assetTypeLoaded(TransactionPtr asset) {
	//
	amount_t lScale = QBIT;
	// 0
	uint256 lAsset = TxAssetType::qbitAsset();
	if (args_[0] != "*") lAsset.setHex(args_[0]);

	// 1
	PKey lAddress;
	lAddress.fromString(args_[1]);

	// 2
	double lAmount = (double)(boost::lexical_cast<double>(args_[2]));

	if (asset) {
		if (asset->type() == Transaction::ASSET_TYPE) {
			TxAssetTypePtr lAssetType = TransactionHelper::to<TxAssetType>(asset);
			lScale = lAssetType->scale();			
		}
	}

	TransactionContextPtr lCtx = wallet_->createTxSpend(lAsset, lAddress, (amount_t)(lAmount * (double)lScale));
	if (lCtx) {
		if (requestProcessor_->broadcastTransaction(lCtx)) {
			std::cout << lCtx->tx()->id().toHex() << std::endl;		
		} else {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			wallet_->resetCache();
			wallet_->prepareCache();
		}
	}
}
