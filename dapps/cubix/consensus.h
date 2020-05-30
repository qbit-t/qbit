// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CUBIX_CONSENSUS_H
#define QBIT_CUBIX_CONSENSUS_H

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
namespace cubix {

class DefaultConsensus: public Consensus {
public:
	DefaultConsensus(const uint256& chain, IConsensusManagerPtr consensusManager, ISettingsPtr settings, IWalletPtr wallet, ITransactionStorePtr store) :
		Consensus(chain, consensusManager, settings, wallet, store) {}

	//
	// max block size
	virtual uint32_t maxBlockSize() { 
		return 1024 * 1024 * 8; 
	}

	//
	// block time for main chain, ms
	// TODO: settings
	virtual uint32_t blockTime() { 
		return 5000; 
	}

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
	virtual uint32_t simpleNetworkSize() { return 5; }

	//
	// mini-tree for sync
	// TODO: settings
	virtual uint32_t partialTreeThreshold() { return 5; }

	static IConsensusPtr instance(const uint256& chain, IConsensusManagerPtr consensusManager, ISettingsPtr settings, IWalletPtr wallet, ITransactionStorePtr store) { 
		return std::make_shared<BuzzerConsensus>(chain, consensusManager, settings, wallet, store);
	}
};

class DefaultConsensusCreator: public ConsensusCreator {
public:
	DefaultConsensusCreator() {}
	IConsensusPtr create(const uint256& chain, IConsensusManagerPtr consensusManager, ISettingsPtr settings, IWalletPtr wallet, ITransactionStorePtr store) { 
		return std::make_shared<DefaultConsensus>(chain, consensusManager, settings, wallet, store); 
	}

	static ConsensusCreatorPtr instance() { return std::make_shared<DefaultConsensusCreator>(); }
};

} // cubix
} // qbit

#endif