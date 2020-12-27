// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_CONSENSUS_H
#define QBIT_BUZZER_CONSENSUS_H

#include "../../iconsensus.h"
#include "../../iwallet.h"
#include "../../timestamp.h"
#include "../../ivalidator.h"
#include "../../ivalidatormanager.h"
#include "../../itransactionstoremanager.h"
#include "../../log/log.h"
#include "../../consensus.h"

#include <iterator>

namespace qbit {

class BuzzerConsensus: public Consensus {
public:
	BuzzerConsensus(const uint256& chain, IConsensusManagerPtr consensusManager, ISettingsPtr settings, IWalletPtr wallet, ITransactionStorePtr store) :
		Consensus(chain, consensusManager, settings, wallet, store) {}

	//
	// max block size
	virtual uint32_t maxBlockSize() { return 1024 * 1024 * 1; }

	//
	// block time for main chain, ms
	// TODO: settings
	virtual uint32_t blockTime() { return 5000; }

	//
	// block count (100 blocks)
	// TODO: settings
	virtual uint32_t quarantineTime() { return 100; }

	//
	// maturity period (blocks)
	virtual uint32_t maturity() {
		return settings_->mainChainMaturity(); 
	}

	//
	// coinbase maturity period (blocks)
	virtual uint32_t coinbaseMaturity() {
		return settings_->mainChainCoinbaseMaturity(); 
	}

	//
	// minimum network size
	// TODO: settings
	virtual uint32_t simpleNetworkSize() { return 4; }

	//
	// mini-tree for sync
	// TODO: settings
	virtual uint32_t partialTreeThreshold() { return 30; }

	virtual bool checkBalance(amount_t /*coinbaseAmount*/, amount_t /*blockFee*/, uint64_t /*height*/) {
		//
		return true;
	}	

	static IConsensusPtr instance(const uint256& chain, IConsensusManagerPtr consensusManager, ISettingsPtr settings, IWalletPtr wallet, ITransactionStorePtr store) { 
		return std::make_shared<BuzzerConsensus>(chain, consensusManager, settings, wallet, store);
	}
};

class BuzzerConsensusCreator: public ConsensusCreator {
public:
	BuzzerConsensusCreator() {}
	IConsensusPtr create(const uint256& chain, IConsensusManagerPtr consensusManager, ISettingsPtr settings, IWalletPtr wallet, ITransactionStorePtr store) { 
		return std::make_shared<BuzzerConsensus>(chain, consensusManager, settings, wallet, store); 
	}

	static ConsensusCreatorPtr instance() { return std::make_shared<BuzzerConsensusCreator>(); }
};

} // qbit

#endif