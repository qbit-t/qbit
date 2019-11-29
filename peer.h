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
	Peer() : status_(IPeer::UNDEFINED) { 
		quarantine_ = 0; latency_ = 1000000; 
	}

	Peer(SocketPtr socket, const State& state, IPeerManagerPtr peerManager) : 
		socket_(socket), status_(IPeer::UNDEFINED), state_(state), latency_(1000000), quarantine_(0), peerManager_(peerManager) {
		socketStatus_ = CONNECTED;
		socketType_ = SERVER;
	}

	Peer(int contextId, const std::string endpoint, IPeerManagerPtr peerManager) : 
		socket_(nullptr), status_(IPeer::UNDEFINED), latency_(1000000), quarantine_(0), peerManager_(peerManager) {
		contextId_ = contextId;
		endpoint_ = endpoint;
		socketType_ = CLIENT;

		resolver_.reset(new tcp::resolver(peerManager->getContext(contextId))); 
	}

	Peer(int contextId, IPeerManagerPtr peerManager) : 
		socket_(nullptr), status_(IPeer::UNDEFINED), latency_(1000000), quarantine_(0), peerManager_(peerManager) {
		contextId_ = contextId;
		socketStatus_ = CONNECTED;
		socketType_ = SERVER;
		socket_ = std::make_shared<tcp::socket>(peerManager->getContext(contextId_));
	}

	void setState(const State& state) { state_ = state; }
	void setLatency(uint32_t latency) { latency_ = latency; }

	IPeer::Status status() { return status_; }

	PKey address() { return state_.address(); }
	uint32_t latency() { return latency_; }
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
		if (socket != nullptr) {
			if (socketType_ == CLIENT) {
				boost::system::error_code lEndpoint; socket->remote_endpoint(lEndpoint);
				if (!lEndpoint)
					return socket->remote_endpoint().address().to_string() + ":" + std::to_string(socket->remote_endpoint().port());
			} else {
				boost::system::error_code lLocalEndpoint; socket->local_endpoint(lLocalEndpoint);
				boost::system::error_code lRemoteEndpoint; socket->remote_endpoint(lRemoteEndpoint);
				if (!lLocalEndpoint && !lRemoteEndpoint)
					return socket->remote_endpoint().address().to_string() + ":" + std::to_string(socket->remote_endpoint().port());
			}
		}

		return "";
	}

	void connect();
	void ping();
	void sendState();
	void requestPeers();
	void waitForMessage();

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
	// all processX functions - going to waitForMessage
	void processMessage(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processState(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processPing(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processPong(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processSent(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void processRequestPeers();
	void processPeers(std::list<DataStream>::iterator msg, const boost::system::error_code& error);

	// finalize - just remove sent message
	void messageFinalize(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void peerFinalize(std::list<DataStream>::iterator msg, const boost::system::error_code& error);
	void connected(const boost::system::error_code& error, tcp::resolver::iterator endpoint_iterator);
	void resolved(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator);

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
		DataStream lMessage(SER_NETWORK, CLIENT_VERSION);
		return rawOutMessages_.insert(rawOutMessages_.end(), lMessage);
	}

	std::list<DataStream>::iterator emptyOutMessage() {
		return rawOutMessages_.end();
	}

	void eraseOutMessage(std::list<DataStream>::iterator msg) {
		rawOutMessages_.erase(msg);
	}

	void eraseInMessage(std::list<DataStream>::iterator msg) {
		rawInMessages_.erase(msg);
	}

	void eraseInData(std::list<DataStream>::iterator msg) {
		rawInData_.erase(msg);
	}

private:
	SocketPtr socket_;
	IPeer::Status status_;
	State state_;
	uint32_t latency_;
	uint32_t quarantine_;
	std::string endpoint_;
	std::shared_ptr<tcp::resolver> resolver_;
	SocketStatus socketStatus_ = CLOSED;
	SocketType socketType_ = DEFAULT;

	std::list<DataStream> rawInMessages_;
	std::list<DataStream> rawInData_;
	std::list<DataStream> rawOutMessages_;

	IPeerManagerPtr peerManager_;
	int contextId_ = -1;
	int postponeTime_ = 0;
};

}

#endif