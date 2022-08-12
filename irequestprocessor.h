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
	class KeyOrder {
	public:
		KeyOrder(uint64_t height, uint32_t latency): height_(height), latency_(latency) {}

		friend bool operator < (const KeyOrder& a, const KeyOrder& b) {
			// forward order (less height - on top)
			if (a.height() < b.height()) return true;
			if (a.height() > b.height()) return false;
			// reverse order (less latency - on bottom)
			if (a.latency() > b.latency()) return true;
			if (a.latency() < b.latency()) return false;

			return false;
		}

		friend bool operator == (const KeyOrder& a, const KeyOrder& b) {
			return (a.height() == b.height() && a.latency() == b.latency());
		}

		uint64_t height() const { return height_; }
		uint32_t latency() const { return latency_; }

	private:
		uint64_t height_;
		uint32_t latency_;
	};

public:
	IRequestProcessor() {}

	virtual uint64_t locateHeight(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::locateHeight - not implemented."); }	
	virtual IPeerPtr locatePeer(const uint160& /*peer*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::locatePeer - not implemented."); }	
	virtual void addDAppInstance(const State::DAppInstance& /*instance*/, bool notify = true) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::addDAppInstance - not implemented."); }	
	virtual void addDAppInstance(const std::vector<State::DAppInstance>& /*instance*/, bool notify = true) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::addDAppInstance - not implemented."); }
	virtual void clearDApps() { throw qbit::exception("NOT_IMPL", "IRequestProcessor::clearDApps - not implemented."); }

	virtual void requestState() { throw qbit::exception("NOT_IMPL", "IRequestProcessor::requestState - not implemented."); }	
	virtual bool loadTransaction(const uint256& /*chain*/, const uint256& /*tx*/, ILoadTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::loadTransaction - not implemented."); }
	virtual bool loadTransaction(const uint256& /*chain*/, const uint256& /*tx*/, bool /*tryMempool*/, ILoadTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::loadTransaction - not implemented."); }
	virtual bool loadTransactions(const uint256& /*chain*/, const std::vector<uint256>& /*txs*/, ILoadTransactionsHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::loadTransactions - not implemented."); }
	virtual bool loadEntity(const std::string& /*entityName*/, ILoadEntityHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::loadEntity - not implemented."); }
	virtual bool selectUtxoByAddress(const PKey& /*source*/, const uint256& /*chain*/, ISelectUtxoByAddressHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectUtxoByAddress - not implemented."); }
	virtual bool selectUtxoByAddressAndAsset(const PKey& /*source*/, const uint256& /*chain*/, const uint256& /*asset*/, ISelectUtxoByAddressAndAssetHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectUtxoByAddress - not implemented."); }
	virtual bool selectUtxoByTransaction(const uint256& /*chain*/, const uint256& /*tx*/, ISelectUtxoByTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectUtxoByAddress - not implemented."); }
	virtual bool selectUtxoByEntity(const std::string& /*entityName*/, ISelectUtxoByEntityNameHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectUtxoByEntity - not implemented."); }
	virtual bool selectUtxoByEntityNames(const std::vector<std::string>& /*entityNames*/, ISelectUtxoByEntityNamesHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectUtxoByEntityNames - not implemented."); }
	virtual bool selectEntityCountByShards(const std::string& /*dapp*/, ISelectEntityCountByShardsHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectEntityCountByShards - not implemented."); }
	virtual bool selectEntityCountByDApp(const std::string& /*dapp*/, ISelectEntityCountByDAppHandlerPtr /*handler*/, int& /*destinations*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectEntityCountByDApp - not implemented."); }
	virtual bool selectEntityNames(const std::string& /*pattern*/, ISelectEntityNamesHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::selectEntityNames - not implemented."); }
	virtual bool broadcastTransaction(TransactionContextPtr /*ctx*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::broadcastTransaction - not implemented."); }
	virtual void collectPeersByChain(const uint256& /*chain*/, std::map<KeyOrder, IPeerPtr>& /*order*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::collectPeersByChain - not implemented."); }
	virtual void collectChains(const std::string& /*dApp*/, std::vector<uint256>& /*chains*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::collectChains - not implemented."); }
	virtual void collectPeers(std::map<uint160, IPeerPtr>& /*peers*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::collectPeers - not implemented."); }
	virtual void collectPeersByDApp(const std::string& /*dapp*/, std::map<uint256, std::multimap<uint32_t, IPeerPtr>>& /*order*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::collectPeersByDApp - not implemented."); }
	virtual IPeerPtr sendTransaction(TransactionContextPtr /*ctx*/, ISentTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::sendTransaction - not implemented."); }
	virtual IPeerPtr sendTransaction(const uint256& /*destination*/, TransactionContextPtr /*ctx*/, ISentTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::sendTransaction - not implemented."); }
	virtual IPeerPtr sendTransaction(IPeerPtr /*peer*/, const uint256& /*destination*/, TransactionContextPtr /*ctx*/, ISentTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::sendTransaction - not implemented."); }
	virtual bool sendTransaction(IPeerPtr /*peer*/, TransactionContextPtr /*ctx*/, ISentTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IRequestProcessor::sendTransaction - not implemented."); }
	virtual bool askForQbits() { throw qbit::exception("NOT_IMPL", "IRequestProcessor::askForQbits - not implemented."); }
};

typedef std::shared_ptr<IRequestProcessor> IRequestProcessorPtr;

} // qbit

#endif
