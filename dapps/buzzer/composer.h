// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_COMPOSER_H
#define QBIT_BUZZER_COMPOSER_H

#include "../../wallet.h"

namespace qbit {

// forward
class BuzzerComposer;
typedef std::shared_ptr<BuzzerComposer> BuzzerComposerPtr;

//
class BuzzerComposer: public std::enable_shared_from_this<BuzzerComposer> {
public:
	BuzzerComposer(ISettingsPtr settings, IWalletPtr wallet) : settings_(settings), wallet_(wallet),
		workingSettings_(settings->dataPath() + "/wallet/buzzer/settings") {
		opened_ = false;
	}

	static BuzzerComposerPtr instance(ISettingsPtr settings, IWalletPtr wallet) {
		return std::make_shared<BuzzerComposer>(settings, wallet); 
	}

	// composer management
	bool open();
	bool close();
	bool isOpened() { return opened_; }

	std::string dAppName() { return "buzzer"; }

	// create buzzer tx
	TransactionContextPtr createTxBuzzer(const PKey& /*self*/, const std::string& /*buzzer*/, const std::string& /*alias*/, const std::string& /*description*/);
	TransactionContextPtr createTxBuzzer(const std::string& /*buzzer*/, const std::string& /*alias*/, const std::string& /*description*/);
	// create buzz tx
	TransactionContextPtr createTxBuzz(const PKey& /*self*/, const std::string& /*body*/);
	TransactionContextPtr createTxBuzz(const std::string& /*body*/);

private:
	// various settings, command line args & config file
	ISettingsPtr settings_;
	// wallet
	IWalletPtr wallet_;
	// buzzer tx
	uint256 buzzerTx_;

	// persistent settings
	db::DbContainer<std::string /*name*/, std::string /*data*/> workingSettings_;	

	// flag
	bool opened_;

	// lock
	boost::recursive_mutex mutex_;
};

} // qbit

#endif