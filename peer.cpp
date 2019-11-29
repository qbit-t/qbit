#include "peer.h"

using namespace qbit;

bool Peer::onQuarantine() {
	return peerManager_->consensus()->currentState()->height() < quarantine_;
}

//
// external procedure: just finalize message we need
//
void Peer::ping() {
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED /*&& socketType_ == CLIENT*/) {

		uint64_t lTimestamp = getMicroseconds();

		Message lMessage(Message::PING, sizeof(uint64_t));
		std::list<DataStream>::iterator lMsg = newOutMessage();

		(*lMsg) << lMessage;
		(*lMsg) << lTimestamp;

		gLog().write(Log::NET, std::string("[peer]: ping to ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

//
// external procedure: just finalize message we need
//
void Peer::sendState() {
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// push own current state
		StatePtr lState = peerManager_->consensus()->currentState();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);
		lState->serialize<DataStream>(lStateStream, peerManager_->consensus()->mainKey());

		Message lMessage(Message::STATE, lStateStream.size());

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		gLog().write(Log::NET, std::string("[peer]: sending state to ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

void Peer::requestPeers() {	
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		// make message
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);
		Message lMessage(Message::GET_PEERS, lStateStream.size());
		(*lMsg) << lMessage;

		// log
		gLog().write(Log::NET, std::string("[peer]: requesting peers from ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

void Peer::waitForMessage() {
	// log
	gLog().write(Log::NET, std::string("[peer]: waiting for message..."));

	// new message entry
	std::list<DataStream>::iterator lMsg = newInMessage();

	boost::asio::async_read(*socket_,
		boost::asio::buffer(lMsg->data(), Message::size()),
		boost::bind(
			&Peer::processMessage, shared_from_this(), lMsg,
			boost::asio::placeholders::error));
}

void Peer::processMessage(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error)	{
		if (!msg->size()) {
			// log
			gLog().write(Log::ERROR, std::string("[peer]: empty message from ") + key());
			rawInMessages_.erase(msg);

			waitForMessage(); 
			return; 
		}

		//
		gLog().write(Log::NET, std::string("[peer]: raw message from ") + key() + " -> " + HexStr(msg->begin(), msg->end()) + ", " + statusString());

		Message lMessage;
		(*msg) >> lMessage; // deserialize
		eraseInMessage(msg); // erase

		// log
		gLog().write(Log::NET, std::string("[peer]: message from ") + key() + " -> " + lMessage.toString() + (lMessage.valid()?" - valid":" - invalid"));

		// process
		if (lMessage.valid() && lMessage.dataSize() < peerManager_->settings()->maxMessageSize()) {
			// new data entry
			std::list<DataStream>::iterator lMsg = newInData();
			lMsg->resize(lMessage.dataSize());

			//
			// "state" message received
			// 1. push to peers processor
			// 2. reply own state 
			if (lMessage.type() == Message::STATE) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processState, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::PING) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processPing, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::PONG) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processPong, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::PEERS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processPeers, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::GET_PEERS) {
				// call processor
				processRequestPeers();
			} else if (lMessage.type() == Message::PEER_EXISTS) {
				// mark peer
				toPostponed();
				peerManager_->postpone(shared_from_this());
				waitForMessage();
			} else if (lMessage.type() == Message::PEER_BANNED) {
				// mark peer
				toBan();
				peerManager_->ban(shared_from_this());
				waitForMessage();
			} else {
				eraseInData(lMsg);
				waitForMessage();
			}
		} else {
			waitForMessage();
		}
	} else {
		// log
		if (key().size()) {
			gLog().write(Log::NET, "[peer/processMessage]: closing session " + key() + " -> " + error.message() + ", " + statusString());
			//
			socketStatus_ = ERROR;
			// try to deactivate peer
			peerManager_->deactivatePeer(shared_from_this());
			// close socket
			socket_->close();
		}
	}
}

void Peer::processRequestPeers() {
	// new message
	std::list<DataStream>::iterator lMsg = newOutMessage();

	// prepare peers list
	std::vector<std::string> lPeers;
	peerManager_->activePeers(lPeers);

	// push own current state
	DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);
	lStateStream << lPeers;

	// make message
	Message lMessage(Message::PEERS, lStateStream.size());
	(*lMsg) << lMessage;
	lMsg->write(lStateStream.data(), lStateStream.size());

	// log
	gLog().write(Log::NET, std::string("[peer]: sending peers list to ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

	// write
	boost::asio::async_write(*socket_,
		boost::asio::buffer(lMsg->data(), lMsg->size()),
		boost::bind(
			&Peer::processSent, shared_from_this(), lMsg,
			boost::asio::placeholders::error));
}

void Peer::processPeers(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		gLog().write(Log::NET, std::string("[peer]: peers list from ") + Peer::key(socket()));

		// extract peers
		std::vector<std::string> lPeers;
		(*msg) >> lPeers;
		eraseInData(msg);

		for (std::vector<std::string>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			peerManager_->addPeer(*lPeer);	
		}

		waitForMessage();
	} else {
		// log
		gLog().write(Log::ERROR, "[peer/processPeers]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processPing(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		gLog().write(Log::NET, std::string("[peer]: ping from ") + Peer::key(socket()));

		uint64_t lTimestamp;
		(*msg) >> lTimestamp;
		eraseInData(msg);

		Message lMessage(Message::PONG, sizeof(uint64_t));

		std::list<DataStream>::iterator lMsg = newOutMessage();
		(*lMsg) << lMessage;
		(*lMsg) << lTimestamp;

		gLog().write(Log::NET, std::string("[peer]: pong to ") + Peer::key(socket()));

		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::processSent, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	} else {
		// log
		gLog().write(Log::ERROR, "[peer/processPing]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processPong(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		gLog().write(Log::NET, std::string("[peer]: pong from ") + key());

		uint64_t lTimestamp;
		(*msg) >> lTimestamp;
		eraseInData(msg);

		// TODO: update
		//peerManager_->updatePeerLatency(getMicroseconds() - lTimestamp);

		waitForMessage();
	} else {
		// log
		gLog().write(Log::ERROR, "[peer/processPong]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processState(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		// log
		gLog().write(Log::NET, std::string("[peer]: raw state message from ") + key() + " -> " + HexStr(msg->begin(), msg->end()));

		State lState;
		lState.deserialize<DataStream>(*msg);
		eraseInData(msg); // erase

		gLog().write(Log::NET, std::string("[peer]: processing state from ") + key() + " -> " + lState.toString());

		IPeer::UpdatePeerResult lPeerResult;
		if (!peerManager_->updatePeerState(shared_from_this(), lState, lPeerResult)) {
			// if peer aleady exists
			if (lPeerResult == IPeer::EXISTS) {
				Message lMessage(Message::PEER_EXISTS, sizeof(uint64_t));
				std::list<DataStream>::iterator lMsg = newOutMessage();

				(*lMsg) << lMessage;

				gLog().write(Log::NET, std::string("[peer]: peer already exists ") + key());

				boost::asio::async_write(*socket_,
					boost::asio::buffer(lMsg->data(), lMsg->size()),
					boost::bind(
						&Peer::peerFinalize, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lPeerResult == IPeer::BAN) {
				Message lMessage(Message::PEER_BANNED, sizeof(uint64_t));
				std::list<DataStream>::iterator lMsg = newOutMessage();

				(*lMsg) << lMessage;

				gLog().write(Log::NET, std::string("[peer]: peer banned ") + key());

				boost::asio::async_write(*socket_,
					boost::asio::buffer(lMsg->data(), lMsg->size()),
					boost::bind(
						&Peer::peerFinalize, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else {
				// log
				gLog().write(Log::NET, "[peer]: closing session " + key() + " -> " + error.message());
				//
				socketStatus_ = ERROR;
				// try to deactivate peer
				peerManager_->deactivatePeer(shared_from_this());
				// close socket
				socket_->close();
			}
		} else {
			// send state only for inbound peers
			if (!isOutbound()) sendState();
			// goto start
			waitForMessage();
		}
	} else {
		// log
		gLog().write(Log::ERROR, "[peer/processState]: closing session " + Peer::key(socket()) + " -> " + error.message());
		//
		socketStatus_ = ERROR;		
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();				
	}
}

void Peer::processSent(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	if (msg != emptyOutMessage()) eraseOutMessage(msg);
	waitForMessage();
}

void Peer::messageFinalize(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	if (msg != emptyOutMessage()) eraseOutMessage(msg);
}

void Peer::peerFinalize(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	if (msg != emptyOutMessage()) eraseOutMessage(msg);
	// log
	gLog().write(Log::NET, "[peer]: finalizing session " + key());
	//
	socketStatus_ = ERROR;
	// try to deactivate peer
	peerManager_->deactivatePeer(shared_from_this());
	// close socket
	socket_->close();	
}

void Peer::connect() {
	if (socketType_ == SERVER) { 
		gLog().write(Log::NET, std::string("[peer]: server endpoint - connect() not allowed"));
		return;
	}

	if (socketType_ == CLIENT && (socketStatus_ == CLOSED || socketStatus_ == ERROR)) {
		gLog().write(Log::NET, std::string("[peer]: connecting ") + key());

		// change status
		socketStatus_ = CONNECTING;

		// make socket
		socket_.reset(new boost::asio::ip::tcp::socket(peerManager_->getContext(contextId_)));

		std::vector<std::string> lParts;
		boost::split(lParts, endpoint_, boost::is_any_of(":"), boost::token_compress_on);

		if (lParts.size() == 2) {
			resolver_->async_resolve(tcp::resolver::query(lParts[0], lParts[1]),
				boost::bind(&Peer::resolved, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::iterator));			
		}
	}
}

void Peer::resolved(const boost::system::error_code& error, tcp::resolver::iterator endpoint_iterator) {
	//
	if (!error) {
		tcp::endpoint lEndpoint = *endpoint_iterator;
		socket_->async_connect(lEndpoint,
			boost::bind(&Peer::connected, shared_from_this(),
				boost::asio::placeholders::error, ++endpoint_iterator));
	} else {
		gLog().write(Log::NET, std::string("[peer/resolve]: resolve failed for ") + key() + " -> " + error.message());
		socketStatus_ = ERROR;
		socket_ = nullptr;
	}
}

void Peer::connected(const boost::system::error_code& error, tcp::resolver::iterator endpoint_iterator) {
	if (!error) {
		socketStatus_ = CONNECTED;

		// connected
		gLog().write(Log::NET, std::string("[peer]: connected to ") + key());

		// connected - send our state
		sendState();

		// connected - request peers
		requestPeers();

		// go to read
		waitForMessage();
	} else if (endpoint_iterator != tcp::resolver::iterator()) {
		socket_->close();
		
		tcp::endpoint lEndpoint = *endpoint_iterator;
		socket_->async_connect(lEndpoint,
			boost::bind(&Peer::connected, this,
				boost::asio::placeholders::error, ++endpoint_iterator));
	} else {
		gLog().write(Log::ERROR, std::string("[peer/connect]: connection failed for ") + key() + " -> " + error.message());
		socketStatus_ = ERROR;
		socket_ = nullptr;
	}
}
