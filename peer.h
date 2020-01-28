// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_PEER_H
#define QBIT_PEER_H

#include "isettings.h"
#include "log/log.h"

#include <boost/algorithm/string.hpp>

#include "ipeermanager.h"
#include "message.h"
#include "timestamp.h"

#include <boost/bind.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

namespace qbit {

class Peer: public IPeer, public std::enable_shared_from_this<Peer> {
public:
	enum SocketStatus {
		CLOSED = 0,
		CONNECTING = 1,
		CONNECTED = 2,
		ERROR = 3
	};

	enum SocketType {
		DEFAULT = 0,
		SERVER = 1,
		CLIENT = 2
	};

public:
	class PersistentState {
	public:
		PersistentState() {}
		PersistentState(const State& state, IPeer::Status status) : state_(state), status_(status) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(state_);
			READWRITE((int)status_);
		}

		State& state() { return state_; }
		IPeer::Status status() { return status_; }

	private:
		State state_;
		IPeer::Status status_;
	};

public:
	Peer() : status_(IPeer::UNDEFINED) { 
		quarantine_ = 0; latencyPrev_ = latency_ = 1000000; 
	}

	Peer(SocketPtr socket, const State& state, IPeerManagerPtr peerManager) : 
		socket_(socket), status_(IPeer::UNDEFINED), state_(state), latency_(1000000), latencyPrev_(1000000), quarantine_(0), peerManager_(peerManager) {
		socketStatus_ = CONNECTED;
		socketType_ = SERVER;
	}

	Peer(int contextId, const std::string endpoint, IPeerManagerPtr peerManager) : 
		socket_(nullptr), status_(IPeer::UNDEFINED), latency_(1000000), latencyPrev_(1000000), quarantine_(0), peerManager_(peerManager) {
		contextId_ = contextId;
		endpoint_ = endpoint;
		socketType_ = CLIENT;

		resolver_.reset(new tcp::resolver(peerManager->getContext(contextId))); 
	}

	Peer(int contextId, IPeerManagerPtr peerManager) : 
		socket_(nullptr), status_(IPeer::UNDEFINED), latency_(1000000), latencyPrev_(1000000), quarantine_(0), peerManager_(peerManager) {
		contextId_ = contextId;
		socketStatus_ = CONNECTED;
		socketType_ = SERVER;
		socket_ = std::make_shared<tcp::socket>(peerManager->getContext(contextId_));
	}

	void setState(const State& state) { state_ = state; }
	void setLatency(uint32_t latency) { latencyPrev_ = latency_; latency_ = latency; }

	bool hasRole(State::PeerRoles role) { return (state_.roles() & role) != 0; }

	IPeer::Status status() { return status_; }
	State& state() { return state_; }
	void close() {
		if (socket_) {
			socket_->close();
			socket_ = nullptr;
		}
	}

	PKey address() { return state_.address(); }
	uint32_t latency() { return latency_; }
	uint32_t latencyPrev() { return latencyPrev_; }
	uint32_t quarantine() { return quarantine_; }
	int contextId() { return contextId_; }
	bool isOutbound() { return socketType_ == CLIENT; }

	SocketPtr socket() { return socket_; }
	uint160 addressId() { return state_.address().id(); }

	void toQuarantine(uint32_t block) { quarantine_ = block; status_ = IPeer::QUARANTINE; }
	void toBan() { quarantine_ = 0; status_ = IPeer::BANNED; }
	void toActive() { quarantine_ = 0; status_ = IPeer::ACTIVE; }
	void deactivate() { status_ = IPeer::UNDEFINED; }
	void toPendingState() { status_ = IPeer::PENDING_STATE; }
	void toPostponed() {
		status_ = IPeer::POSTPONED;
		postponeTime_ = 5; // TODO: settings!
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
		std::string lKey = key(socket_); 
		if (lKey.size()) return lKey;
		return endpoint_;
	}

	inline std::string key(SocketPtr socket) {
		// TODO: we need to consider removing :port part from key for production
		if (socket != nullptr) {
			if (socketType_ == CLIENT) {
				boost::system::error_code lEndpoint; socket->remote_endpoint(lEndpoint);
				if (!lEndpoint)
					return (endpoint_ = socket->remote_endpoint().address().to_string() + ":" + std::to_string(socket->remote_endpoint().port()));
			} else {
				boost::system::error_code lLocalEndpoint; socket->local_endpoint(lLocalEndpoint);
				boost::system::error_code lRemoteEndpoint; socket->remote_endpoint(lRemoteEndpoint);
				if (!lLocalEndpoint && !lRemoteEndpoint)
					return (endpoint_ = socket->remote_endpoint().address().to_string() + ":" + std::to_string(socket->remote_endpoint().port()));
			}
		}

		return "";
	}

	void connect();
	void ping();
	void sendState() { internalSendState(peerManager_->consensusManager()->currentState(), false /*global_state*/); }
	void broadcastState(StatePtr state) { internalSendState(state, true /*global_state*/); }
	void requestPeers();
	void waitForMessage();
	void broadcastBlockHeader(const NetworkBlockHeader& /*blockHeader*/);
	void broadcastTransaction(TransactionContextPtr /*ctx*/);

	void synchronizeFullChain(IConsensusPtr, SynchronizationJobPtr /*job*/);
	void synchronizeFullChainHead(IConsensusPtr, SynchronizationJobPtr /*job*/);
	void acquireBlock(const NetworkBlockHeader& /*block*/);

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

private:
	// internal processing
	void processMessage(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processState(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGlobalState(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processPing(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processPong(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processSent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processRequestPeers(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processPeers(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBlockHeader(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processTransaction(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	void processGetBlockByHeight(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetBlockById(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processGetNetworkBlock(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	void processBlockByHeight(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBlockById(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processNetworkBlock(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	void processBlockByHeightAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processBlockByIdAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processNetworkBlockAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	// finalize - just remove sent message
	void messageFinalize(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void peerFinalize(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void connected(const boost::system::error_code& error, tcp::resolver::iterator endpoint_iterator);
	void resolved(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator);

	void internalSendState(StatePtr state, bool global);	

private:
	// service methods
	std::list<DataStream>::iterator newInMessage() {
		DataStream lMessage(SER_NETWORK, CLIENT_VERSION);
		lMessage.resize(Message::size());

		return rawInMessages_.insert(rawInMessages_.end(), lMessage);
	}

	std::list<DataStream>::iterator newInData() {
		DataStream lMessage(SER_NETWORK, CLIENT_VERSION);
		return rawInData_.insert(rawInData_.end(), lMessage);
	}

	std::list<DataStream>::iterator newOutMessage() {
		boost::unique_lock<boost::mutex> lLock(rawOutMutex_);
		DataStream lMessage(SER_NETWORK, CLIENT_VERSION);
		return rawOutMessages_.insert(rawOutMessages_.end(), lMessage);
	}

	std::list<DataStream>::iterator emptyOutMessage() {
		boost::unique_lock<boost::mutex> lLock(rawOutMutex_);
		return rawOutMessages_.end();
	}

	void eraseOutMessage(std::list<DataStream>::iterator msg) {
		boost::unique_lock<boost::mutex> lLock(rawOutMutex_);
		rawOutMessages_.erase(msg);
	}

	void eraseInMessage(std::list<DataStream>::iterator msg) {
		rawInMessages_.erase(msg);
	}

	void eraseInData(std::list<DataStream>::iterator msg) {
		rawInData_.erase(msg);
	}

	void addJob(const uint256& chain, SynchronizationJobPtr job) {
		boost::unique_lock<boost::mutex> lLock(jobsMutex_);
		if (jobs_.find(chain) == jobs_.end()) jobs_[chain] = job;
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

private:
	SocketPtr socket_;
	IPeer::Status status_;
	State state_;
	uint32_t latency_;
	uint32_t latencyPrev_;
	uint32_t quarantine_;
	std::string endpoint_;
	std::shared_ptr<tcp::resolver> resolver_;
	SocketStatus socketStatus_ = CLOSED;
	SocketType socketType_ = DEFAULT;

	std::list<DataStream> rawInMessages_;
	std::list<DataStream> rawInData_;
	std::list<DataStream> rawOutMessages_;
	boost::mutex rawOutMutex_;

	std::map<uint256, SynchronizationJobPtr> jobs_;
	boost::mutex jobsMutex_;

	IPeerManagerPtr peerManager_;
	int contextId_ = -1;
	int postponeTime_ = 0;

	uint64_t peersPoll_ = 0;
};

}

#endif