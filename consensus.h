// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CONSENSUS_H
#define QBIT_CONSENSUS_H

#include "iconsensus.h"
#include "iwallet.h"
#include "timestamp.h"

namespace qbit {

// TODO: all checks by active peers: height|block;
// TODO: aditional indexes
class Consensus: public IConsensus {
public:
	Consensus(ISettingsPtr settings, IWalletPtr wallet, ITransactionStorePtr store) :
		settings_(settings), wallet_(wallet), store_(store) {}

	//
	// max block size
	size_t maxBlockSize() { return 1024 * 1024 * 1; }
	//
	// current time (adjusted? averaged?)
	uint64_t currentTime() { return qbit::getTime(); }
	
	StatePtr currentState() {
		StatePtr lState = State::instance(qbit::getTime());

		// TODO: add last block and height by chain
		return lState;
	}

	//
	// block count (10 sec * 100 blocks)
	size_t quarantineTime() { return 100; }

	//
	// use peer for consensus participation
	void pushPeer(const std::string /*endpoint*/, IPeerPtr /*peer*/) {

	}

	//
	// remove peer from consensus participation
	void popPeer(const std::string /*endpoint*/, IPeerPtr /*peer*/) {

	}

	//
	// main key
	SKey mainKey() {
		return wallet_->firstKey();
	}

	static IConsensusPtr instance(ISettingsPtr settings, IWalletPtr wallet, ITransactionStorePtr store) { 
		return std::make_shared<Consensus>(settings, wallet, store);
	}

private:
	ISettingsPtr settings_;
	IWalletPtr wallet_;
	ITransactionStorePtr store_;

};

} // qbit

#endif
