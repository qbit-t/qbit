// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ICONSENSUS_H
#define QBIT_ICONSENSUS_H

#include "transaction.h"
#include "block.h"
#include "transactioncontext.h"
#include "state.h"
#include "ipeer.h"

#include <memory>

namespace qbit {
// TODO: all checks by active peers: height|block;
// TODO: aditional indexes
class IConsensus {
public:
	IConsensus() {}

	virtual size_t maxBlockSize() { throw qbit::exception("NOT_IMPL", "IConsensus::maxBlockSize - not implemented."); }
	virtual uint64_t currentTime() { throw qbit::exception("NOT_IMPL", "IConsensus::currentTime - not implemented."); }
	virtual StatePtr currentState() { throw qbit::exception("NOT_IMPL", "IConsensus::currentState - not implemented."); }
	virtual size_t quarantineTime() { throw qbit::exception("NOT_IMPL", "IConsensus::quarantineTime - not implemented."); }

	virtual void pushPeer(const std::string /*endpoint*/, IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IConsensus::pushPeer - not implemented."); }
	virtual void popPeer(const std::string /*endpoint*/, IPeerPtr /*peer*/) { throw qbit::exception("NOT_IMPL", "IConsensus::popPeer - not implemented."); }
	virtual SKey mainKey() { throw qbit::exception("NOT_IMPL", "IConsensus::mainKey - not implemented."); }
};

typedef std::shared_ptr<IConsensus> IConsensusPtr;

} // qbit

#endif
