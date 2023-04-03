// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_PEER_H
#define QBIT_PEER_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include "isettings.h"
#include "log/log.h"

#include <boost/algorithm/string.hpp>

#include "ipeermanager.h"
#include "message.h"
#include "timestamp.h"

#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/random_device.hpp>

using boost::asio::ip::tcp;

namespace qbit {

class Peer: public IPeer, public std::enable_shared_from_this<Peer> {
public:
	enum SocketType {
		DEFAULT = 0,
		SERVER = 1,
		CLIENT = 2
	};

public:
	class PersistentState {
	public:
		PersistentState() {}
		PersistentState(const State& state, IPeer::Status status, IPeer::Type type) : 
			state_(state), status_(status), type_(type) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(state_);

			if (ser_action.ForRead()) {
				int lStatus;
				s >> lStatus;
				status_ = (IPeer::Status)lStatus;

				int lType;
				s >> lType;
				type_ = (IPeer::Type)lType;
			} else {
				READWRITE((int)status_);
				READWRITE((int)type_);				
			}
		}

		State& state() { return state_; }
		IPeer::Status status() { return status_; }
		IPeer::Type type() { return type_; }

	private:
		State state_;
		IPeer::Status status_;
		IPeer::Type type_;
	};

public:
	explicit Peer() : status_(IPeer::UNDEFINED) { 
		quarantine_ = 0; latencyPrev_ = latency_ = 1000000; time_ = getMicroseconds(); timestamp_ = time_;
		gen_ = boost::random::mt19937(rd_());
		secret_ = SKey::instance();
		secret_->create();
	}

	Peer(int contextId, const std::string endpoint, IPeerManagerPtr peerManager) : 
		socket_(nullptr), status_(IPeer::UNDEFINED), latency_(1000000), latencyPrev_(1000000), quarantine_(0), peerManager_(peerManager) {
		contextId_ = contextId;
		endpoint_ = endpoint;
		socketType_ = CLIENT;
		resolver_.reset(new tcp::resolver(peerManager->getContext(contextId))); 
		strand_.reset(new boost::asio::io_service::strand(peerManager->getContext(contextId)));
		time_ = getMicroseconds();
		timestamp_ = time_;

		peerManager->incPeersCount();

		gen_ = boost::random::mt19937(rd_());

		secret_ = SKey::instance();
		secret_->create();
	}

	Peer(int contextId, IPeerManagerPtr peerManager) : 
		socket_(nullptr), status_(IPeer::UNDEFINED), latency_(1000000), latencyPrev_(1000000), quarantine_(0), peerManager_(peerManager) {
		contextId_ = contextId;
		socketStatus_ = CONNECTED;
		socketType_ = SERVER;
		socket_ = std::make_shared<tcp::socket>(peerManager->getContext(contextId_));
		strand_.reset(new boost::asio::io_service::strand(peerManager->getContext(contextId_)));
		time_ = getMicroseconds();
		timestamp_ = time_;

		peerManager->incPeersCount();

		gen_ = boost::random::mt19937(rd_());

		secret_ = SKey::instance();
		secret_->create();
	}

	~Peer() {
		release();

		{
			boost::unique_lock<boost::recursive_mutex> lLock(readMutex_);
			state_.reset();
		}

		peerManager_->decPeersCount();

		if (gLog().isEnabled(Log::NET)) 
			gLog().write(Log::NET, std::string("[peer]: peer destroyed ") + key());
	}

	void moveToContext(int contextId) {
		//
		// NOTICE: this method should be called from syhcnronously from the previous context thread, between "read" and "wait"
		//		   but there still can be ongoing "write" operation
		//
		if (gLog().isEnabled(Log::NET)) 
			gLog().write(Log::NET, strprintf("[peer]: peer %s moving to the NEW context = %d, old = %d", key(), contextId, contextId_));

		boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
		if (socket_) {
			//
			try {
				auto lDescriptor = socket_->release(); // cancel all async operations
				contextId_ = contextId; // new context
				socket_.reset(new boost::asio::ip::tcp::socket(peerManager_->getContext(contextId_)));
				strand_.reset(new boost::asio::io_service::strand(peerManager_->getContext(contextId_)));
				socket_->assign(boost::asio::ip::tcp::v4(), lDescriptor);
			} catch(const boost::system::system_error& ex) {
				gLog().write(Log::GENERAL_ERROR, strprintf("[peer/moveToContext/error]: move failed for %s | %s", key(), ex.what()));
			}
		}
	}

	void release() {
		//
		{
			boost::unique_lock<boost::mutex> lLock(extensionsMutex_);
			for (std::map<std::string, IPeerExtensionPtr>::iterator lExtension = extension_.begin(); lExtension != extension_.end(); lExtension++) {
				//
				lExtension->second->release();
			}

			extension_.clear();
		}

		close(IPeer::CLOSED);

		/*
		{
			// NOTICE: peer pop\push in this case will not work - we need LAST actual state
			boost::unique_lock<boost::recursive_mutex> lLock(readMutex_);
			state_.reset();
		}
		*/

		if (gLog().isEnabled(Log::NET)) 
			gLog().write(Log::NET, std::string("[peer]: peer released ") + key());
	}

	void setState(StatePtr state) { 
		//
		boost::unique_lock<boost::recursive_mutex> lLock(readMutex_);
		state_ = state;
		address_ = state_->address();
		addressId_ = state_->address().id();
		roles_ = state->roles();

		// refresh dapp info
		if (state_->client()) {
			for (std::vector<State::DAppInstance>::const_iterator 
					lInstance = state_->dApps().begin();
					lInstance != state_->dApps().end(); lInstance++) {
				//
				std::map<std::string, IPeerExtensionPtr>::iterator lExtension = extension_.find(lInstance->name());
				if (lExtension != extension_.end()) {
					lExtension->second->setDApp(*lInstance);
				} else { // create if new one found
					//
					PeerExtensionCreatorPtr lCreator = peerManager_->locateExtensionCreator(lInstance->name());
					if (lCreator) {
						setExtension(lInstance->name(), lCreator->create(*lInstance, shared_from_this(), peerManager_));
					}
				}
			}
		}
	}

	void setLatency(uint32_t latency) { latencyPrev_ = latency_; latency_ = latency; }

	bool hasRole(State::PeerRoles role) { return (roles_ & role) != 0; }
	uint64_t time() { return time_; }	
	uint64_t timestamp() { return timestamp_; }	

	IPeer::Status status() { return status_; }
	StatePtr state() { 
		boost::unique_lock<boost::recursive_mutex> lLock(readMutex_);
		if (!state_) { PKey lPKey; state_ = State::instance(0, 0, lPKey); }
		return state_; 
	}
	void close(SocketStatus status) {
		bool lDeactivate = false;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
			if (socket_ && socketStatus_ != GENERAL_ERROR) {
				try {
					if (socket_->is_open()) {
						try {
							socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
						} catch(const boost::system::system_error& ex) {
							gLog().write(Log::GENERAL_ERROR, strprintf("[peer/close/error]: socket shutdown failed for %s | %s", key(), ex.what()));
						}
						socket_->close(); // close and cancel
					} else {
						socket_->cancel(); // cancels any awating operation
					}
				} catch(const boost::system::system_error& ex) {
					gLog().write(Log::GENERAL_ERROR, strprintf("[peer/close/error]: socket close failed for %s | %s", key(), ex.what()));
				}

				socketStatus_ = status;
				if (status == GENERAL_ERROR)
					lDeactivate = true;
			}
		}

		if (lDeactivate) peerManager_->deactivatePeer(shared_from_this());
	}

	PKey address() { boost::unique_lock<boost::recursive_mutex> lLock(readMutex_); return address_; }
	uint32_t latency() { return latency_; }
	uint32_t latencyPrev() { return latencyPrev_; }
	uint32_t quarantine() { return quarantine_; }
	int contextId() { return contextId_; }
	bool isOutbound() { return socketType_ == CLIENT; }

	SocketPtr socket() { return socket_; }
	uint160 addressId() { boost::unique_lock<boost::recursive_mutex> lLock(readMutex_); return addressId_; }

	void toQuarantine(uint32_t block) { quarantine_ = block; status_ = IPeer::QUARANTINE; }
	void toBan() { quarantine_ = 0; status_ = IPeer::BANNED; }
	void toActive() { quarantine_ = 0; status_ = IPeer::ACTIVE; }
	void deactivate() { status_ = IPeer::UNDEFINED; }
	void toPendingState() { status_ = IPeer::PENDING_STATE; }
	void toPostponed() {
		status_ = IPeer::POSTPONED;
		postponeTime_ = 30; // TODO: settings!
	}

	bool postponedTick() {
		postponeTime_--;
		if (postponeTime_ <= 0) {
			postponeTime_ = 0;
			deactivate();
			return true;
		}

		return false;
	}

	bool onQuarantine();

	static IPeerPtr instance(int contextId, const std::string endpoint, IPeerManagerPtr peerManager) {
		return std::make_shared<Peer>(contextId, endpoint, peerManager); 
	}

	inline std::string key() {
		boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
		std::string lKey = key(socket_); 
		if (lKey.size()) return lKey;
		return endpoint_.size() ? endpoint_ : "(?)";
	}

	inline std::string key(SocketPtr socket) {
		// TODO: we need to consider removing :port part from key for production
		boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
		if (socket != nullptr) {
			if (socketType_ == CLIENT) {
				boost::system::error_code lErr; 
				boost::asio::ip::tcp::endpoint lEndpoint = socket->remote_endpoint(lErr);
				if (!lErr)
					return (endpoint_ = (lEndpoint.address().is_v4() ? strprintf("%s:%s", lEndpoint.address().to_string(), std::to_string(lEndpoint.port())) :
																		strprintf("[%s]:%s", lEndpoint.address().to_string(), std::to_string(lEndpoint.port()))));
			} else {
				boost::system::error_code lLocalErr;
				boost::asio::ip::tcp::endpoint lLocalEndpoint = socket->local_endpoint(lLocalErr);
				boost::system::error_code lRemoteErr;
				boost::asio::ip::tcp::endpoint lRemoteEndpoint = socket->remote_endpoint(lRemoteErr);

				if (!lLocalErr && !lRemoteErr)
					return (endpoint_ = (lRemoteEndpoint.address().is_v4() ? strprintf("%s:%s", lRemoteEndpoint.address().to_string(), std::to_string(lRemoteEndpoint.port())) :
																		strprintf("[%s]:%s", lRemoteEndpoint.address().to_string(), std::to_string(lRemoteEndpoint.port()))));
			}
		}

		return "";
	}

	virtual bool isLocal() {
		boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
		if (socket_ != nullptr) {
			boost::system::error_code lLocalErr;
			boost::asio::ip::tcp::endpoint lLocalEndpoint = socket_->local_endpoint(lLocalErr);
			boost::system::error_code lRemoteErr;
			boost::asio::ip::tcp::endpoint lRemoteEndpoint = socket_->remote_endpoint(lRemoteErr);
			if (!lLocalErr && !lRemoteErr)
				return lLocalEndpoint.address().to_string() == lRemoteEndpoint.address().to_string();
		}

		return false;
	}

	inline uint160 keyId() {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
		std::string lAddress;
		if (socket_ != nullptr) {
			if (socketType_ == CLIENT) {
				boost::system::error_code lEndpoint; socket_->remote_endpoint(lEndpoint);
				if (!lEndpoint)
					lAddress = socket_->remote_endpoint().address().to_string();
			} else {
				boost::system::error_code lLocalEndpoint; socket_->local_endpoint(lLocalEndpoint);
				boost::system::error_code lRemoteEndpoint; socket_->remote_endpoint(lRemoteEndpoint);
				if (!lLocalEndpoint && !lRemoteEndpoint)
					lAddress = socket_->remote_endpoint().address().to_string();
			}

			return Hash160(lAddress.begin(), lAddress.end());
		}

		return uint160();		
	}

	void connect();
	void ping();
	void sendState() { internalSendState(peerManager_->consensusManager()->currentState(), false /*global_state*/); }
	void requestState();
	void broadcastState(StatePtr state) { internalSendState(state, true /*global_state*/); }
	void requestPeers();
	void waitForMessage();
	void broadcastBlockHeader(const NetworkBlockHeader& /*blockHeader*/);
	void broadcastTransaction(TransactionContextPtr /*ctx*/);
	void broadcastBlockHeaderAndState(const NetworkBlockHeader& /*block*/, StatePtr /*state*/);
	void keyExchange();

	void synchronizeFullChain(IConsensusPtr, SynchronizationJobPtr /*job*/, const boost::system::error_code& error = boost::system::error_code());
	void synchronizePartialTree(IConsensusPtr, SynchronizationJobPtr /*job*/, const boost::system::error_code& error = boost::system::error_code());
	void synchronizeLargePartialTree(IConsensusPtr, SynchronizationJobPtr /*job*/);
	void synchronizePendingBlocks(IConsensusPtr, SynchronizationJobPtr /*job*/, const boost::system::error_code& error = boost::system::error_code());
	void acquireBlock(const NetworkBlockHeader& /*block*/);

	IPeerExtensionPtr extension(const std::string& dapp) { 
		boost::unique_lock<boost::mutex> lLock(extensionsMutex_);
		std::map<std::string, IPeerExtensionPtr>::iterator lExt = extension_.find(dapp);
		if (lExt != extension_.end()) return lExt->second;
		return nullptr;
	}
	void setExtension(const std::string& dapp, IPeerExtensionPtr extension) { 
		boost::unique_lock<boost::mutex> lLock(extensionsMutex_);
		extension_[dapp] = extension; 
	}	
	std::map<std::string, IPeerExtensionPtr> extensions() { 
		boost::unique_lock<boost::mutex> lLock(extensionsMutex_);
		return extension_; 
	}

	IPeer::Type type() { return type_; }
	void setType(IPeer::Type type) { type_ = type; }

	uint256 sharedSecret();
	uint160 encryptionId() { return other_.id(); }

	// open requests
	void acquireBlockHeaderWithCoinbase(const uint256& /*block*/, const uint256& /*chain*/, INetworkBlockHandlerWithCoinBasePtr /*handler*/);
	void loadTransaction(const uint256& /*chain*/, const uint256& /*tx*/, ILoadTransactionHandlerPtr /*handler*/);
	void loadTransaction(const uint256& /*chain*/, const uint256& /*tx*/, bool /*tryMempool*/, ILoadTransactionHandlerPtr /*handler*/);
	void loadTransactions(const uint256& /*chain*/, const std::vector<uint256>& /*txs*/, ILoadTransactionsHandlerPtr /*handler*/);
	void loadEntity(const std::string& /*entityName*/, ILoadEntityHandlerPtr /*handler*/);
	void selectUtxoByAddress(const PKey& /*source*/, const uint256& /*chain*/, ISelectUtxoByAddressHandlerPtr /*handler*/);
	void selectUtxoByAddressAndAsset(const PKey& /*source*/, const uint256& /*chain*/, const uint256& /*asset*/, ISelectUtxoByAddressAndAssetHandlerPtr /*handler*/);
	void selectUtxoByTransaction(const uint256& /*chain*/, const uint256& /*tx*/, ISelectUtxoByTransactionHandlerPtr /*handler*/);
	void selectUtxoByEntity(const std::string& /*entityName*/, ISelectUtxoByEntityNameHandlerPtr /*handler*/);
	void selectUtxoByEntityNames(const std::vector<std::string>& /*entityNames*/, ISelectUtxoByEntityNamesHandlerPtr /*handler*/);
	void selectEntityCountByShards(const std::string& /*dapp*/, ISelectEntityCountByShardsHandlerPtr /*handler*/);
	void selectEntityCountByDApp(const std::string& /*dapp*/, ISelectEntityCountByDAppHandlerPtr /*handler*/);
	bool sendTransaction(TransactionContextPtr /*ctx*/, ISentTransactionHandlerPtr /*handler*/);
	void selectEntityNames(const std::string& /*pattern*/, ISelectEntityNamesHandlerPtr /*handler*/);
	void tryAskForQbits();
	void tryAskForQbits(const PKey& /*key*/);

	std::string statusString() {
		switch(status_) {
			case IPeer::ACTIVE: return "ACTIVE";
			case IPeer::QUARANTINE: return "QUARANTINE";
			case IPeer::BANNED: return "BANNED";
			case IPeer::PENDING_STATE: return "PENDING_STATE";
			case IPeer::POSTPONED: return "POSTPONED";
			default: return "UNDEFINED";
		}

		return "ESTATUS";
	}

	std::string socketStatusString() {
		switch(socketStatus_) {
			case CLOSED: return "CLOSED";
			case CONNECTING: return "CONNECTING";
			case CONNECTED: return "CONNECTED";
			case GENERAL_ERROR: return "GENERAL_ERROR";
			default: return "UNDEFINED";
		}

		return "ESTATUS";
	}


	void processed() {
		//
		waitForMessage();
	}
	// error processing
	void processError(const std::string& context, std::list<DataStream>::iterator msg, const boost::system::error_code& error);	
	bool sendMessageAsync(std::list<DataStream>::iterator msg);
	void postMessageAsync(std::list<DataStream>::iterator msg);
	void sendMessage(std::list<DataStream>::iterator msg);
	void processPendingMessagesQueue();
	StrandPtr strand() { return strand_; }

	int syncRequestsBlocks() {
		return reqBlocks_.load(std::memory_order_relaxed);
	}

	int syncRequestsHeaders() {
		return reqHeaders_.load(std::memory_order_relaxed);
	}

	int syncRequestsBalance() {
		return syncRequestsBlocks() + syncRequestsHeaders();
	}

private:
	class OutMessage {
	public:
		enum Type {
			QUEUED = 0,
			POSTPONED = 1,
			EMPTY = 2
		};
	public:
		OutMessage() {}
		OutMessage(std::list<DataStream>::iterator msg, OutMessage::Type type, unsigned short epoch) : msg_(msg), type_(type), epoch_(epoch) {}

		std::list<DataStream>::iterator	msg() { return msg_; }
		Type type() { return type_; }
		void toQueued() { type_ = OutMessage::QUEUED; }
		void toPostponed() { type_ = OutMessage::POSTPONED; }
		unsigned short epoch() { return epoch_; }

	private:
		std::list<DataStream>::iterator msg_;
		Type type_;
		unsigned short epoch_ = 0;
	};

	void messageSentAsync(std::list<OutMessage>::iterator msg, const boost::system::error_code& error);
	void sendTimeout(int seconds);
	void postTimeout(boost::system::error_code error);

private:
	// internal processing
	void processMessage(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processState(std::list<DataStream>::iterator msg, bool broadcast, const boost::system::error_code& error);
	void processGlobalState(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processPing(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processPong(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	//void processSent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processRequestPeers(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processPeers(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBlockHeader(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processTransaction(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBlockHeaderAndState(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBlock(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	void processGetBlockByHeight(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBlockById(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetNetworkBlock(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetNetworkBlockHeader(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBlockHeader(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBlockData(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetTransactionData(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetTransactionsData(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetUtxoByAddress(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetUtxoByAddressAndAsset(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetUtxoByTransaction(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetEntity(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetUtxoByEntity(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetEntityCountByShards(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetEntityCountByDApp(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processPushTransaction(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetUtxoByEntityNames(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetState(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetEntityNames(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	
	void processAskForQbits(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	void processBlockByHeight(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBlockById(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processNetworkBlock(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processNetworkBlockHeader(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processTransactionData(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processTransactionsData(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processUtxoByAddress(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processUtxoByAddressAndAsset(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processUtxoByTransaction(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processEntity(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processUtxoByEntity(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processEntityCountByShards(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processEntityCountByDApp(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processTransactionPushed(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processUtxoByEntityNames(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processEntityNames(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	void processBlockByHeightAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBlockByIdAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processNetworkBlockAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processNetworkBlockHeaderAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBlockHeaderAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBlockAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processTransactionAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processEntityAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processUnknownMessage(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processKeyExchange(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	//void peerFinalize(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void connected(const boost::system::error_code& error, tcp::resolver::iterator endpoint_iterator);
	void resolved(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator);

	void internalSendState(StatePtr state, bool global);

	bool hasPendingRequests() {
		boost::unique_lock<boost::mutex> lLock(replyHandlersMutex_);
		return replyHandlers_.size();
	}

	bool hasActiveJobs() {
		boost::unique_lock<boost::mutex> lLock(jobsMutex_);
		return jobs_.size();
	}

	uint256 addRequest(IReplyHandlerPtr replyHandler) {
		//
		boost::random::uniform_int_distribution<> lDistribute(1, 1024*1024);
		uint256 lId = Random::generate(lDistribute(gen_));
		//
		boost::unique_lock<boost::mutex> lLock(replyHandlersMutex_);
		replyHandlers_.insert(std::map<uint256 /*request*/, RequestWrapper>::value_type(lId, RequestWrapper(replyHandler)));
		return lId;
	}

	void removeRequest(const uint256& id) {
		boost::unique_lock<boost::mutex> lLock(replyHandlersMutex_);
		replyHandlers_.erase(id);

		// cleanup
		std::map<uint256 /*request*/, RequestWrapper> lCurrentHandlers = replyHandlers_;
		for (std::map<uint256 /*request*/, RequestWrapper>::iterator lItem = lCurrentHandlers.begin(); lItem != lCurrentHandlers.end(); lItem++) {
			if (lItem->second.timedout()) {
				replyHandlers_.erase(lItem->first);
			}
		}
	}

	IReplyHandlerPtr locateRequest(const uint256& id) {
		boost::unique_lock<boost::mutex> lLock(replyHandlersMutex_);
		std::map<uint256 /*request*/, RequestWrapper>::iterator lRequest = replyHandlers_.find(id);
		if (lRequest != replyHandlers_.end()) return lRequest->second.handler();

		return nullptr;
	}

	// service methods
	std::list<DataStream>::iterator newInMessage() {
		boost::unique_lock<boost::mutex> lLock(rawInMutex_);
		DataStream lMessage(SER_NETWORK, PROTOCOL_VERSION);
		lMessage.resize(Message::size());
		receivedMessagesCount_++;
		return rawInMessages_.insert(rawInMessages_.end(), lMessage);
	}

	std::list<DataStream>::iterator newInData(const Message& msg) {
		boost::unique_lock<boost::mutex> lLock(rawInMutex_);
		DataStream lMessage(SER_NETWORK, PROTOCOL_VERSION);
		lMessage.setCheckSum(const_cast<Message&>(msg).checkSum());
		if (const_cast<Message&>(msg).encrypted()) lMessage.setSecret(sharedSecret());
		return rawInData_.insert(rawInData_.end(), lMessage);
	}

	std::list<DataStream>::iterator newOutMessage() {
		boost::unique_lock<boost::mutex> lLock(rawOutMutex_);
		DataStream lMessage(SER_NETWORK, PROTOCOL_VERSION);
		return rawOutMessages_.insert(rawOutMessages_.end(), lMessage);
	}

	std::list<DataStream>::iterator emptyOutMessage() {
		boost::unique_lock<boost::mutex> lLock(rawOutMutex_);
		return rawOutMessages_.end();
	}

	void removeUnqueuedOutMessage(std::list<DataStream>::iterator msg) {
		//
		boost::unique_lock<boost::mutex> lLock(rawOutMutex_);
		if (rawOutMessages_.size()) rawOutMessages_.erase(msg);
	}

	bool eraseOutMessage(std::list<OutMessage>::iterator msg) {
		boost::unique_lock<boost::mutex> lLock(rawOutMutex_);
		if (outQueue_.size()) {
			sentMessagesCount_++;
			msg->msg()->reset();
			bytesSent_ += msg->msg()->size();
			rawOutMessages_.erase(msg->msg());
			outQueue_.erase(msg);

			return true;
		}

		return false;
	}

	void clearQueues();

	void eraseInMessage(std::list<DataStream>::iterator msg) {
		boost::unique_lock<boost::mutex> lLock(rawInMutex_);
		if (rawInMessages_.size() && msg != rawInMessages_.end()) rawInMessages_.erase(msg);
	}

	void eraseInData(std::list<DataStream>::iterator msg) {
		{
			// all data was read, unlock reading
			boost::unique_lock<boost::recursive_mutex> lLock(readMutex_);
			reading_ = false;
		}

		{
			boost::unique_lock<boost::mutex> lLock(rawInMutex_);
			if (msg != rawInData_.end()) rawInData_.erase(msg);
		}
	}

	bool jobExists(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(jobsMutex_);
		return jobs_.find(chain) != jobs_.end();
	}

	bool addJob(const uint256& chain, SynchronizationJobPtr job) {
		boost::unique_lock<boost::mutex> lLock(jobsMutex_);
		std::map<uint256, SynchronizationJobPtr>::iterator lJob = jobs_.find(chain);
		if (lJob == jobs_.end()) { jobs_[chain] = job; return true; }
		else {
			return lJob->second->unique() == job->unique();
		}
		
		//jobs_.erase(chain);
		//jobs_[chain] = job;
		return false;
	}

	void removeJob(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(jobsMutex_);
		jobs_.erase(chain);
	}

	SynchronizationJobPtr locateJob(const uint256& chain) {
		boost::unique_lock<boost::mutex> lLock(jobsMutex_);
		std::map<uint256, SynchronizationJobPtr>::iterator lJob = jobs_.find(chain);
		if (lJob != jobs_.end()) return lJob->second;

		return nullptr;
	}

	std::vector<SynchronizationJobPtr> synchronizationJobs() {
		//
		std::vector<SynchronizationJobPtr> lResult;
		boost::unique_lock<boost::mutex> lLock(jobsMutex_);
		for (std::map<uint256, SynchronizationJobPtr>::iterator lJob = jobs_.begin(); lJob != jobs_.end(); lJob++) {
			//
			lResult.push_back(lJob->second);
		}

		return lResult;
	}

	void incReqBlocks() {
		reqBlocks_.fetch_add(1, std::memory_order_relaxed);
	}

	void incReqHeaders() {
		reqHeaders_.fetch_add(1, std::memory_order_relaxed);
	}

	void decReqBlocks() {
		//reqBlocks_.fetch_sub(1, std::memory_order_relaxed);
		reqBlocks_.store(0, std::memory_order_relaxed);
	}

	void decReqHeaders() {
		//reqHeaders_.fetch_sub(1, std::memory_order_relaxed);
		reqHeaders_.store(0, std::memory_order_relaxed);
	}

	void resetStat() {
		decReqBlocks();
		decReqHeaders();
	}

	uint32_t inQueueLength() {

		boost::unique_lock<boost::mutex> lLock(rawInMutex_);
		return rawInMessages_.size();
	}

	uint32_t outQueueLength() {
		boost::unique_lock<boost::mutex> lLock(rawOutMutex_);
		return rawOutMessages_.size();
	}

	uint32_t pendingQueueLength() {
		boost::unique_lock<boost::mutex> lLock(rawInMutex_);
		return rawInData_.size();
	}

	uint32_t receivedMessagesCount() {
		return receivedMessagesCount_;
	}

	uint32_t sentMessagesCount() {
		return sentMessagesCount_;
	}

	uint64_t bytesReceived() {
		return bytesReceived_;
	}

	uint64_t bytesSent() {
		return bytesSent_;
	}

	void reset();

private:
	SocketPtr socket_ = nullptr;
	StrandPtr strand_;
	IPeer::Status status_;
	StatePtr state_;
	uint32_t latency_;
	uint32_t latencyPrev_;
	uint32_t quarantine_;
	std::string endpoint_;
	std::shared_ptr<tcp::resolver> resolver_;
	SocketStatus socketStatus_ = CLOSED;
	SocketType socketType_ = DEFAULT;
	uint64_t time_;
	uint64_t timestamp_;

	uint32_t receivedMessagesCount_ = 0;
	uint32_t sentMessagesCount_ = 0;

	uint64_t bytesReceived_ = 0;
	uint64_t bytesSent_ = 0;

	uint160 addressId_;
	PKey address_;
	uint32_t roles_;

	std::list<DataStream> rawInMessages_;
	std::list<DataStream> rawInData_;
	std::list<DataStream> rawOutMessages_;
	std::list<OutMessage> outQueue_;
	boost::mutex rawOutMutex_;
	boost::mutex rawInMutex_;
	boost::recursive_mutex socketMutex_;

	std::map<uint256 /*request*/, RequestWrapper> replyHandlers_;
	boost::mutex replyHandlersMutex_;

	std::map<uint256, SynchronizationJobPtr> jobs_;
	boost::mutex jobsMutex_;

	std::set<uint160> alreadyRelayed_;

	IPeerManagerPtr peerManager_;
	int contextId_ = -1;
	int postponeTime_ = 0;

	std::atomic<int> reqBlocks_ { 0 };
	std::atomic<int> reqHeaders_ { 0 };

	bool waitingForMessage_ = false;
	bool reading_ = false;

	uint64_t peersPoll_ = 0;
	unsigned short epoch_ = 0;

	IPeer::Type type_ = IPeer::Type::IMPLICIT;

	// lock
	boost::recursive_mutex readMutex_;

	boost::mutex extensionsMutex_;
	std::map<std::string, IPeerExtensionPtr> extension_;

	//
	boost::random_device rd_;
	boost::random::mt19937 gen_;

	//
	typedef std::shared_ptr<boost::asio::high_resolution_timer> TimerPtr;
	TimerPtr controlTimer_;

	//
	SKeyPtr secret_;
	PKey other_;
};

}

#endif
