// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IPEER_H
#define QBIT_IPEER_H

#include "state.h"
#include "key.h"
#include "entity.h"

#include <boost/atomic.hpp>

namespace qbit {
	class IConsensus;
	typedef std::shared_ptr<IConsensus> IConsensusPtr;
	class SynchronizationJob;
	typedef std::shared_ptr<SynchronizationJob> SynchronizationJobPtr;
}

#include "synchronizationjob.h"
#include "iconsensus.h"
#include "ipeerextension.h"

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

namespace qbit {

typedef std::shared_ptr<tcp::socket> SocketPtr;

//
// basic handler
class IReplyHandler {
public:
	IReplyHandler() {}
	virtual void timeout() {}
};
typedef std::shared_ptr<IReplyHandler> IReplyHandlerPtr;

//
// basic handler
class RequestWrapper {
public:
	RequestWrapper(IReplyHandlerPtr handler) : timestamp_(qbit::getTime()), handler_(handler) {}

	IReplyHandlerPtr handler() { return handler_; }
	bool timedout() { return qbit::getTime() - timestamp_ > 10; }

private:
	uint64_t timestamp_;
	IReplyHandlerPtr handler_;
};

//
// tx helper
class ReplyHelper {
public:
	ReplyHelper() {}

	template<typename type>
	static std::shared_ptr<type> to(IReplyHandlerPtr handler) { return std::static_pointer_cast<type>(handler); }
};

//
// network block handler with coinbase tx
class INetworkBlockHandlerWithCoinBase: public IReplyHandler {
public:
	INetworkBlockHandlerWithCoinBase() {}
	virtual void handleReply(const NetworkBlockHeader&, TransactionPtr) = 0;
};
typedef std::shared_ptr<INetworkBlockHandlerWithCoinBase> INetworkBlockHandlerWithCoinBasePtr;

//
// load transaction handler
class ILoadTransactionHandler: public IReplyHandler {
public:
	ILoadTransactionHandler() {}
	virtual void handleReply(TransactionPtr) = 0;
};
typedef std::shared_ptr<ILoadTransactionHandler> ILoadTransactionHandlerPtr;

//
// sent transaction handler
class ISentTransactionHandler: public IReplyHandler {
public:
	ISentTransactionHandler() {}
	virtual void handleReply(const uint256&, const std::vector<TransactionContext::Error>&) = 0;
};
typedef std::shared_ptr<ISentTransactionHandler> ISentTransactionHandlerPtr;

//
// select utxo by address
class ISelectUtxoByAddressHandler: public IReplyHandler {
public:
	ISelectUtxoByAddressHandler() {}
	virtual void handleReply(const std::vector<Transaction::NetworkUnlinkedOut>&, const PKey&) = 0;
};
typedef std::shared_ptr<ISelectUtxoByAddressHandler> ISelectUtxoByAddressHandlerPtr;

//
// select utxo by address
class ISelectUtxoByAddressAndAssetHandler: public IReplyHandler {
public:
	ISelectUtxoByAddressAndAssetHandler() {}
	virtual void handleReply(const std::vector<Transaction::NetworkUnlinkedOut>&, const PKey&, const uint256&) = 0;
};
typedef std::shared_ptr<ISelectUtxoByAddressAndAssetHandler> ISelectUtxoByAddressAndAssetHandlerPtr;

//
// select utxo by address
class ISelectUtxoByTransactionHandler: public IReplyHandler {
public:
	ISelectUtxoByTransactionHandler() {}
	virtual void handleReply(const std::vector<Transaction::NetworkUnlinkedOut>&, const uint256&) = 0;
};
typedef std::shared_ptr<ISelectUtxoByTransactionHandler> ISelectUtxoByTransactionHandlerPtr;

//
// load entity handler
class ILoadEntityHandler: public IReplyHandler {
public:
	ILoadEntityHandler() {}
	virtual void handleReply(EntityPtr) = 0;
};
typedef std::shared_ptr<ILoadEntityHandler> ILoadEntityHandlerPtr;

//
// select utxo by entity name
class ISelectUtxoByEntityNameHandler: public IReplyHandler {
public:
	ISelectUtxoByEntityNameHandler() {}
	virtual void handleReply(const std::vector<Transaction::UnlinkedOut>&, const std::string& /*entity*/) = 0;
};
typedef std::shared_ptr<ISelectUtxoByEntityNameHandler> ISelectUtxoByEntityNameHandlerPtr;

//
// select entity count by shards
class ISelectEntityCountByShardsHandler: public IReplyHandler {
public:
	class EntitiesCount {
	public:
		EntitiesCount() {}
		EntitiesCount(const uint256& shard, uint32_t count) : shard_(shard), count_(count) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(shard_);
			READWRITE(count_);
		}

		inline uint256& shard() { return shard_; }
		inline uint32_t count() { return count_; }

	private:
		uint256 shard_;
		uint32_t count_;
	};

public:
	ISelectEntityCountByShardsHandler() {}
	virtual void handleReply(const std::map<uint32_t, uint256>&, const std::string& /*dapp*/) = 0;
};
typedef std::shared_ptr<ISelectEntityCountByShardsHandler> ISelectEntityCountByShardsHandlerPtr;

//
// p2p protocol processor
class IPeer {
public:
	enum Status {
		UNDEFINED = 0,
		ACTIVE = 1,
		QUARANTINE = 2,
		BANNED = 3,
		PENDING_STATE = 4,
		POSTPONED = 5
	};

	enum UpdatePeerResult {
		SUCCESSED = 0,
		EXISTS = 1,
		BAN = 2,
		DENIED = 3,
		SESSIONS_EXCEEDED = 4
	};	

public:
	IPeer() {}

	virtual std::map<std::string, IPeerExtensionPtr> extensions() { throw qbit::exception("NOT_IMPL", "IPeer::extensions - not implemented."); }
	virtual IPeerExtensionPtr extension(const std::string&) { throw qbit::exception("NOT_IMPL", "IPeer::extension - not implemented."); }
	virtual void setExtension(const std::string&, IPeerExtensionPtr /*extension*/) { throw qbit::exception("NOT_IMPL", "IPeer::setExtension - not implemented."); }

	virtual void release() { throw qbit::exception("NOT_IMPL", "IPeer::release - not implemented."); }

	virtual Status status() { throw qbit::exception("NOT_IMPL", "IPeer::status - not implemented."); }
	virtual State& state() { throw qbit::exception("NOT_IMPL", "IPeer::state - not implemented."); }
	virtual uint160 addressId() { throw qbit::exception("NOT_IMPL", "IPeer::addressId - not implemented."); }
	virtual bool hasRole(State::PeerRoles /*role*/) { throw qbit::exception("NOT_IMPL", "IPeer::hasRole - not implemented."); }

	virtual void close() { throw qbit::exception("NOT_IMPL", "IPeer::close - not implemented."); }

	virtual std::string statusString() { throw qbit::exception("NOT_IMPL", "IPeer::statusString - not implemented."); }

	virtual void setState(const State& /*state*/) { throw qbit::exception("NOT_IMPL", "IPeer::setState - not implemented."); }
	virtual void setLatency(uint32_t /*latency*/) { throw qbit::exception("NOT_IMPL", "IPeer::setLatency - not implemented."); }

	virtual PKey address() { throw qbit::exception("NOT_IMPL", "IPeer::address - not implemented."); }
	virtual uint32_t latency() { throw qbit::exception("NOT_IMPL", "IPeer::latency - not implemented."); }
	virtual uint32_t latencyPrev() { throw qbit::exception("NOT_IMPL", "IPeer::latencyPrev - not implemented."); }
	virtual uint32_t quarantine() { throw qbit::exception("NOT_IMPL", "IPeer::quarantine - not implemented."); }
	virtual int contextId() { throw qbit::exception("NOT_IMPL", "IPeer::contextId - not implemented."); }

	virtual uint32_t inQueueLength() { throw qbit::exception("NOT_IMPL", "IPeer::inQueueLength - not implemented."); }
	virtual uint32_t outQueueLength() { throw qbit::exception("NOT_IMPL", "IPeer::outQueueLength - not implemented."); }
	virtual uint32_t pendingQueueLength() { throw qbit::exception("NOT_IMPL", "IPeer::pendingQueueLength - not implemented."); }

	virtual uint32_t receivedMessagesCount() { throw qbit::exception("NOT_IMPL", "IPeer::receivedMessagesCount - not implemented."); }
	virtual uint32_t sentMessagesCount() { throw qbit::exception("NOT_IMPL", "IPeer::sentMessagesCount - not implemented."); }
	virtual uint64_t bytesReceived() { throw qbit::exception("NOT_IMPL", "IPeer::bytesReceived - not implemented."); }
	virtual uint64_t bytesSent() { throw qbit::exception("NOT_IMPL", "IPeer::bytesSent - not implemented."); }

	virtual uint64_t time() { throw qbit::exception("NOT_IMPL", "IPeer::time - not implemented."); }
	virtual uint64_t timestamp() { throw qbit::exception("NOT_IMPL", "IPeer::timestamp - not implemented."); }

	virtual void toQuarantine(uint32_t /*block*/) { throw qbit::exception("NOT_IMPL", "IPeer::toQuarantine - not implemented."); }
	virtual void toBan() { throw qbit::exception("NOT_IMPL", "IPeer::toBan - not implemented."); }
	virtual void toActive() { throw qbit::exception("NOT_IMPL", "IPeer::toActive - not implemented."); }
	virtual void toPendingState() { throw qbit::exception("NOT_IMPL", "IPeer::toPendingState - not implemented."); }
	virtual void toPostponed() { throw qbit::exception("NOT_IMPL", "IPeer::toPostponed - not implemented."); }
	virtual void deactivate() { throw qbit::exception("NOT_IMPL", "IPeer::deactivate - not implemented."); }
	virtual bool isOutbound() { throw qbit::exception("NOT_IMPL", "IPeer::isOutbound - not implemented."); }
	virtual bool postponedTick() { throw qbit::exception("NOT_IMPL", "IPeer::postponedTick - not implemented."); }

	virtual bool onQuarantine() { throw qbit::exception("NOT_IMPL", "IPeer::onQuarantine - not implemented."); }

	virtual SocketPtr socket() { throw qbit::exception("NOT_IMPL", "IPeer::socket - not implemented."); }
	virtual std::string key() { throw qbit::exception("NOT_IMPL", "IPeer::key - not implemented."); }

	virtual void waitForMessage() { throw qbit::exception("NOT_IMPL", "IPeer::waitForMessage - not implemented."); }
	virtual void ping() { throw qbit::exception("NOT_IMPL", "IPeer::ping - not implemented."); }
	virtual void connect() { throw qbit::exception("NOT_IMPL", "IPeer::connect - not implemented."); }
	virtual void requestPeers() { throw qbit::exception("NOT_IMPL", "IPeer::requestPeers - not implemented."); }

	virtual void broadcastState(StatePtr /*state*/) { throw qbit::exception("NOT_IMPL", "IPeer::broadcastState - not implemented."); }
	virtual void broadcastBlockHeader(const NetworkBlockHeader& /*blockHeader*/) { throw qbit::exception("NOT_IMPL", "IPeer::broadcastBlockHeader - not implemented."); }
	virtual void broadcastTransaction(TransactionContextPtr /*ctx*/) { throw qbit::exception("NOT_IMPL", "IPeer::broadcastTransaction - not implemented."); }
	virtual void broadcastBlockHeaderAndState(const NetworkBlockHeader& /*block*/, StatePtr /*state*/) { throw qbit::exception("NOT_IMPL", "IPeer::broadcastBlockHeaderAndState - not implemented."); }

	virtual void synchronizeFullChain(IConsensusPtr, SynchronizationJobPtr /*job*/) { throw qbit::exception("NOT_IMPL", "IPeer::synchronizeFullChain - not implemented."); }
	virtual void synchronizePartialTree(IConsensusPtr, SynchronizationJobPtr /*job*/) { throw qbit::exception("NOT_IMPL", "IPeer::synchronizePartialTree - not implemented."); }
	virtual void synchronizeLargePartialTree(IConsensusPtr, SynchronizationJobPtr /*job*/) { throw qbit::exception("NOT_IMPL", "IPeer::synchronizeLargePartialTree - not implemented."); }
	virtual void synchronizePendingBlocks(IConsensusPtr, SynchronizationJobPtr /*job*/) { throw qbit::exception("NOT_IMPL", "IPeer::synchronizePendingBlocks - not implemented."); }
	virtual bool jobExists(const uint256& /*chain*/) { throw qbit::exception("NOT_IMPL", "IPeer::jobExists - not implemented."); }
	virtual void acquireBlock(const NetworkBlockHeader& /*block*/) { throw qbit::exception("NOT_IMPL", "IPeer::acquireBlock - not implemented."); }

	virtual uint256 addRequest(IReplyHandlerPtr /*replyHandler*/) { throw qbit::exception("NOT_IMPL", "IPeer::addRequest - not implemented."); }
	virtual void removeRequest(const uint256& /*id*/) { throw qbit::exception("NOT_IMPL", "IPeer::removeRequest - not implemented."); }
	virtual IReplyHandlerPtr locateRequest(const uint256& /*id*/) { throw qbit::exception("NOT_IMPL", "IPeer::locateRequest - not implemented."); }
	
	virtual std::list<DataStream>::iterator newInMessage() { throw qbit::exception("NOT_IMPL", "IPeer::newInMessage - not implemented."); }
	virtual std::list<DataStream>::iterator newInData(const Message&) { throw qbit::exception("NOT_IMPL", "IPeer::newInData - not implemented."); }
	virtual std::list<DataStream>::iterator newOutMessage() { throw qbit::exception("NOT_IMPL", "IPeer::newOutMessage - not implemented."); }
	virtual std::list<DataStream>::iterator emptyOutMessage() { throw qbit::exception("NOT_IMPL", "IPeer::emptyOutMessage - not implemented."); }
	virtual void eraseOutMessage(std::list<DataStream>::iterator) { throw qbit::exception("NOT_IMPL", "IPeer::eraseOutMessage - not implemented."); }
	virtual void eraseInMessage(std::list<DataStream>::iterator) { throw qbit::exception("NOT_IMPL", "IPeer::eraseInMessage - not implemented."); }
	virtual void eraseInData(std::list<DataStream>::iterator) { throw qbit::exception("NOT_IMPL", "IPeer::eraseInData - not implemented."); }

	// open requests
	virtual void acquireBlockHeaderWithCoinbase(const uint256& /*block*/, const uint256& /*chain*/, INetworkBlockHandlerWithCoinBasePtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IPeer::acquireBlockHeaderWithCoinbase - not implemented."); }
	virtual void loadTransaction(const uint256& /*chain*/, const uint256& /*tx*/, ILoadTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IPeer::loadTransaction - not implemented."); }
	virtual void loadEntity(const std::string& /*entityName*/, ILoadEntityHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IPeer::loadEnity - not implemented."); }
	virtual void selectUtxoByAddress(const PKey& /*source*/, const uint256& /*chain*/, ISelectUtxoByAddressHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IPeer::selectUtxoByAddress - not implemented."); }
	virtual void selectUtxoByAddressAndAsset(const PKey& /*source*/, const uint256& /*chain*/, const uint256& /*asset*/, ISelectUtxoByAddressAndAssetHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IPeer::selectUtxoByAddressAndAsset - not implemented."); }
	virtual void selectUtxoByTransaction(const uint256& /*chain*/, const uint256& /*tx*/, ISelectUtxoByTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IPeer::selectUtxoByTransaction - not implemented."); }
	virtual void selectUtxoByEntity(const std::string& /*entityName*/, ISelectUtxoByEntityNameHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IPeer::selectUtxoByEntity - not implemented."); }
	virtual void selectEntityCountByShards(const std::string& /*dapp*/, ISelectEntityCountByShardsHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IPeer::selectEntityCountByShards - not implemented."); }
	virtual void sendTransaction(TransactionContextPtr /*ctx*/, ISentTransactionHandlerPtr /*handler*/) { throw qbit::exception("NOT_IMPL", "IPeer::sendTransaction - not implemented."); }
};

typedef std::shared_ptr<IPeer> IPeerPtr;

} // qbit

#endif
