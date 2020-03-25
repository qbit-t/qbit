// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IREQUEST_PROCESSOR_H
#define QBIT_IREQUEST_PROCESSOR_H

#include "isettings.h"
#include "ipeer.h"

namespace qbit {

class IRequestProcessor {
public:
	IRequestProcessor() {}

	virtual bool loadTransaction(const uint256& /*chain*/, const uint256& /*tx*/, ILoadTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::loadTransaction - not implemented."); }
	virtual bool loadEntity(const std::string& /*entityName*/, ILoadEntityHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::loadEntity - not implemented."); }
	virtual bool selectUtxoByAddress(const PKey& /*source*/, const uint256& /*chain*/, ISelectUtxoByAddressHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectUtxoByAddress - not implemented."); }
	virtual bool selectUtxoByAddressAndAsset(const PKey& /*source*/, const uint256& /*chain*/, const uint256& /*asset*/, ISelectUtxoByAddressAndAssetHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectUtxoByAddress - not implemented."); }
	virtual bool selectUtxoByTransaction(const uint256& /*chain*/, const uint256& /*tx*/, ISelectUtxoByTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectUtxoByAddress - not implemented."); }
	virtual bool selectUtxoByEntity(const std::string& /*entityName*/, ISelectUtxoByEntityNameHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectUtxoByEntity - not implemented."); }
	virtual bool selectEntityCountByShards(const std::string& /*dapp*/, ISelectEntityCountByShardsHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectEntityCountByShards - not implemented."); }
	virtual bool broadcastTransaction(TransactionContextPtr /*ctx*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::broadcastTransaction - not implemented."); }
	virtual void collectPeersByChain(const uint256& /*chain*/, std::map<uint32_t, IPeerPtr>& /*order*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::collectPeersByChain - not implemented."); }
};

typedef std::shared_ptr<IRequestProcessor> IRequestProcessorPtr;

} // qbit

#endif
