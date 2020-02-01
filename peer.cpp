#include "peer.h"

using namespace qbit;

bool Peer::onQuarantine() {
	return peerManager_->consensusManager()->currentState()->height() < quarantine_;
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

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: ping to ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

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
void Peer::internalSendState(StatePtr state, bool global) {
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// push own current state
		StatePtr lState = state;
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);
		/*
		if (!global || state->addressId() == peerManager_->consensusManager()->mainPKey().id())  {
			lState->serialize<DataStream>(lStateStream, peerManager_->consensusManager()->mainKey());
		} else {
			lState->serialize<DataStream>(lStateStream);
		}
		*/
		lState->serialize<DataStream>(lStateStream, peerManager_->consensusManager()->mainKey());

		Message lMessage((global ? Message::GLOBAL_STATE : Message::STATE), lStateStream.size());

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending ") + (global ? "global ": "") + std::string("state to ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

//
// external repeatable procedure
//
void Peer::synchronizeFullChain(IConsensusPtr consensus, SynchronizationJobPtr job) {
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// register job
		addJob(consensus->chain(), job);

		// prepare message
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);
		size_t lHeight = job->acquireNextJob(shared_from_this());

		// done?
		if (!lHeight) {
			std::map<size_t, IPeerPtr> lPendingJobs = job->pendingJobs();
			if (lPendingJobs.size()) 
				lHeight = job->reacquireJob(lPendingJobs.rbegin()->first, shared_from_this());
			else {
				// log
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: synchronization job is done, root = ") + strprintf("%s", job->block().toHex()));
				
				// cleanup
				removeJob(consensus->chain());

				// reindex
				consensus->doIndex(job->block()/*root block*/);

				// broadcast new state
				StatePtr lState = peerManager_->consensusManager()->currentState();
				peerManager_->consensusManager()->broadcastState(lState, addressId());

				return; // we are done
			}
		}

		lStateStream << consensus->chain();
		lStateStream << lHeight;
		Message lMessage(Message::GET_BLOCK_BY_HEIGHT, lStateStream.size());

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: requesting BLOCK ") + strprintf("%d", lHeight) + std::string(" from ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

//
// external repeatable procedure
//
void Peer::synchronizeFullChainHead(IConsensusPtr consensus, SynchronizationJobPtr job) {
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {

		uint256 lId = job->nextBlock();
		if (lId.isNull()) {
			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: synchronization job is done, root = ") + strprintf("%s", job->block().toHex()));

			// cleanup
			removeJob(consensus->chain());

			// reindex, partial
			if (consensus->doIndex(job->block()/*root block*/, job->lastBlock())) {
				// broadcast new state
				StatePtr lState = peerManager_->consensusManager()->currentState();
				peerManager_->consensusManager()->broadcastState(lState, addressId());
			} else {
				// log
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: partial reindex FAILED skipping subtree switching, root = ") + strprintf("%s", job->block().toHex()));				
			}

			return;
		}

		// register job
		addJob(consensus->chain(), job);

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		lStateStream << consensus->chain();
		lStateStream << lId;
		Message lMessage(Message::GET_BLOCK_BY_ID, lStateStream.size());

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: requesting block ") + lId.toHex() + std::string(" from ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

void Peer::acquireBlock(const NetworkBlockHeader& block) {
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		uint256 lId = const_cast<NetworkBlockHeader&>(block).blockHeader().hash();
		lStateStream << const_cast<NetworkBlockHeader&>(block).blockHeader().chain();
		lStateStream << lId;
		Message lMessage(Message::GET_NETWORK_BLOCK, lStateStream.size());

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: acquiring block ") + lId.toHex() + std::string(" from ") + key());

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

		// prepare peers list
		std::vector<std::string> lPeers;
		peerManager_->activePeers(lPeers);

		// push own current state
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);
		lStateStream << lPeers;

		// make message
		Message lMessage(Message::GET_PEERS, lStateStream.size());
		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: exchanging peers with ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

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
			gLog().write(Log::NET, std::string("[peer/processMessage/error]: empty message from ") + key());
			rawInMessages_.erase(msg);

			waitForMessage(); 
			return; 
		}

		//
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: raw message from ") + key() + " -> " + HexStr(msg->begin(), msg->end()) + ", " + statusString());

		Message lMessage;
		(*msg) >> lMessage; // deserialize
		eraseInMessage(msg); // erase

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: message from ") + key() + " -> " + lMessage.toString() + (lMessage.valid()?" - valid":" - invalid"));

		// process
		if (lMessage.valid() && lMessage.dataSize() < peerManager_->settings()->maxMessageSize()) {
			// new data entry
			std::list<DataStream>::iterator lMsg = newInData();
			lMsg->resize(lMessage.dataSize());

			// sanity check
			if (peerManager_->existsBanned(key())) {
				// log
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: peer ") + key() + std::string(" is BANNED."));
				// terminate session
				socketStatus_ = ERROR;
				// close socket
				socket_->close();

				return;
			}

			//
			// "state" message received
			// 1. push to peers processor
			// 2. reply own state 
			if (lMessage.type() == Message::STATE) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processState, shared_from_this(), lMsg, true,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::GLOBAL_STATE) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGlobalState, shared_from_this(), lMsg,
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
			} else if (lMessage.type() == Message::BLOCK_HEADER) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processBlockHeader, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::BLOCK_HEADER_AND_STATE) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processBlockHeaderAndState, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::TRANSACTION) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processTransaction, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::GET_BLOCK_BY_HEIGHT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetBlockByHeight, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::GET_BLOCK_BY_ID) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetBlockById, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::GET_NETWORK_BLOCK) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetNetworkBlock, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::BLOCK_BY_HEIGHT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processBlockByHeight, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::BLOCK_BY_ID) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processBlockById, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::NETWORK_BLOCK) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processNetworkBlock, shared_from_this(), lMsg,
						boost::asio::placeholders::error));

			} else if (lMessage.type() == Message::BLOCK_BY_HEIGHT_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processBlockByHeightAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::BLOCK_BY_ID_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processBlockByIdAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::NETWORK_BLOCK_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processNetworkBlockAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::GET_PEERS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processRequestPeers, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::PEER_EXISTS) {
				// mark peer
				peerManager_->postpone(shared_from_this());
			} else if (lMessage.type() == Message::PEER_BANNED) {
				// mark peer
				peerManager_->ban(shared_from_this());
			} else {
				eraseInData(lMsg);
			}
		}
		
		waitForMessage();
	} else {
		// log
		if (key() != "EKEY") {
			gLog().write(Log::NET, "[peer/processMessage/error]: closing session " + key() + " -> " + error.message() + ", " + statusString());
			//
			socketStatus_ = ERROR;
			// try to deactivate peer
			peerManager_->deactivatePeer(shared_from_this());
			// close socket
			socket_->close();
		}
	}
}

void Peer::processBlockByHeightAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block by height is absent from ") + key());

		// extract
		uint256 lChain;
		size_t lHeight;
		(*msg) >> lChain;
		(*msg) >> lHeight;
		eraseInData(msg);
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block is absent for ") + strprintf("%d/%s#", lHeight, lChain.toHex().substr(0, 10)));

	} else {
		// log
		gLog().write(Log::NET, "[peer/processBlockByHeightAbsent/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processBlockByIdAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error){
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block by id is absent from ") + key());

		// extract
		uint256 lChain;
		uint256 lId;
		(*msg) >> lChain;
		(*msg) >> lId;
		eraseInData(msg);
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block is absent for ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)));

	} else {
		// log
		gLog().write(Log::NET, "[peer/processBlockByIdAbsent/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}	
}

void Peer::processNetworkBlockAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: network block is absent from ") + key());

		// extract
		uint256 lChain;
		uint256 lId;
		(*msg) >> lChain;
		(*msg) >> lId;
		eraseInData(msg);
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: network block is absent for ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)));

	} else {
		// log
		gLog().write(Log::NET, "[peer/processNetworkBlockAbsent/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}	
}

void Peer::processBlockByHeight(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response for block by height from ") + key());

		// extract
		uint256 lChain;
		size_t lHeight;
		(*msg) >> lChain;
		(*msg) >> lHeight;
		BlockPtr lBlock = Block::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		// locate job
		SynchronizationJobPtr lJob = locateJob(lChain);
		if (lJob && lJob->releaseJob(lHeight)) {
			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing BLOCK ") + strprintf("%s/%d", lBlock->hash().toHex(), lHeight) + std::string("..."));
			// save block
			peerManager_->consensusManager()->locate(lChain)->store()->saveBlock(lBlock);
		} else {
			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::WARNING, std::string("[peer]: local job was not found for ") + strprintf("%d/%s#", lHeight, lChain.toHex().substr(0, 10)));
		}

		// go do next job
		if (lJob)  {
			synchronizeFullChain(peerManager_->consensusManager()->locate(lChain), lJob);
		} else {
			// go to read
			//waitForMessage();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBlockByHeight/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}	
}

void Peer::processGetBlockByHeight(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request block by height from ") + key());

		// extract
		uint256 lChain;
		size_t lHeight;
		(*msg) >> lChain;
		(*msg) >> lHeight;
		eraseInData(msg);

		// get block
		BlockPtr lBlock = peerManager_->consensusManager()->locate(lChain)->store()->block(lHeight);
		if (lBlock != nullptr) {
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lChain; // chain
			lStream << lHeight; // height
			Block::Serializer::serialize<DataStream>(lStream, lBlock); // block

			// prepare message
			Message lMessage(Message::BLOCK_BY_HEIGHT, lStream.size());
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending block by height ") + strprintf("%s/%d", lBlock->hash().toHex(), lHeight) + std::string(" for ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		} else {
			// block is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lChain; // chain
			lStream << lHeight; // height

			// prepare message
			Message lMessage(Message::BLOCK_BY_HEIGHT_IS_ABSENT, lStream.size());
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block is absent for chain ") + strprintf("%d/%s#", lHeight, lChain.toHex().substr(0, 10)) + std::string(" -> ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		}

	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBlockByHeight/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}	
}

void Peer::processBlockById(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response for block by id from ") + key());

		// extract
		uint256 lChain;
		uint256 lId;
		(*msg) >> lChain;
		(*msg) >> lId;
		BlockPtr lBlock = Block::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		// locate job
		SynchronizationJobPtr lJob = locateJob(lChain);
		if (lJob) {
			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing block ") + strprintf("%s/%s#", lBlock->hash().toHex(), lChain.toHex().substr(0, 10)) + std::string("..."));
			// save block
			peerManager_->consensusManager()->locate(lChain)->store()->saveBlock(lBlock);
			// extract next block id
			size_t lHeight;
			uint256 lPrev = lBlock->prev();
			if (!peerManager_->consensusManager()->locate(lChain)->store()->blockHeight(lPrev, lHeight)) {
				// go do next job
				lJob->setNextBlock(lPrev);
			} else {
				// we found root
				lJob->setLastBlock(lPrev);
			}

			synchronizeFullChainHead(peerManager_->consensusManager()->locate(lChain), lJob);
		} else {
			// go to read
			//waitForMessage();
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBlockById/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}	
}

void Peer::processGetBlockById(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request block by id from ") + key());

		// extract
		uint256 lChain;
		uint256 lId;
		(*msg) >> lChain;
		(*msg) >> lId;
		eraseInData(msg);

		// get block
		BlockPtr lBlock = peerManager_->consensusManager()->locate(lChain)->store()->block(lId);
		if (lBlock != nullptr) {
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lChain; // chain
			lStream << lId; // id
			Block::Serializer::serialize<DataStream>(lStream, lBlock); // block

			// prepare message
			Message lMessage(Message::BLOCK_BY_ID, lStream.size());
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending block by id ") + strprintf("%s/%s#", lBlock->hash().toHex(), lChain.toHex().substr(0, 10)) + std::string(" for ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		} else {
			// block is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lChain; // chain
			lStream << lId; // id

			// prepare message
			Message lMessage(Message::BLOCK_BY_ID_IS_ABSENT, lStream.size());
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block is absent for chain ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)) + std::string(" -> ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		}

	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBlockById/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}	
}

void Peer::processNetworkBlock(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response for network block from ") + key());

		// extract
		uint256 lChain;
		uint256 lId;
		(*msg) >> lChain;
		(*msg) >> lId;
		BlockPtr lBlock = Block::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing network block ") + strprintf("%s/%s#", lBlock->hash().toHex(), lChain.toHex().substr(0, 10)) + std::string("..."));

		BlockHeader lHeader;
		size_t lHeight = peerManager_->consensusManager()->locate(lChain)->store()->currentHeight(lHeader);

		// check sequence
		uint256 lCurrentHash = lHeader.hash();
		if (lCurrentHash != lBlock->prev()) {
			// sequence is broken
			if (lCurrentHash == lBlock->hash()) {
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block already EXISTS ") + 
					strprintf("current height:%d/hash:%s, proposing hash:%s/%s#", 
						lHeight, lHeader.hash().toHex(), lBlock->hash().toHex(), lChain.toHex().substr(0, 10)));
			} else { 
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: blocks sequence is BROKEN ") + 
					strprintf("current height:%d/hash:%s, proposing hash:%s/prev:%s/%s#", 
						lHeight, lHeader.hash().toHex(), lBlock->hash().toHex(), lBlock->prev().toHex(), lChain.toHex().substr(0, 10)));
			}
		} else {
			// save block
			BlockContextPtr lCtx = peerManager_->consensusManager()->locate(lChain)->store()->pushBlock(lBlock);
			if (lCtx) {
				if (!lCtx->errors().size()) {
					// clean-up mempool
					peerManager_->memoryPoolManager()->locate(lChain)->removeTransactions(lBlock);
					// create state and block info
					StatePtr lState = peerManager_->consensusManager()->currentState();
					NetworkBlockHeader lHeader(lCtx->block()->blockHeader(), lCtx->height());
					// broadcast block and state
					peerManager_->consensusManager()->broadcastBlockHeaderAndState(lHeader, lState, lState->addressId());
				} else {
					// mark peer
					peerManager_->ban(shared_from_this());
				}
			}
		}

	} else {
		// log
		gLog().write(Log::NET, "[peer/processNetworkBlock/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}	
}

void Peer::processGetNetworkBlock(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request network block from ") + key());

		// extract
		uint256 lChain;
		uint256 lId;
		(*msg) >> lChain;
		(*msg) >> lId;
		eraseInData(msg);

		// get block
		BlockPtr lBlock = peerManager_->consensusManager()->locate(lChain)->store()->block(lId);
		if (lBlock != nullptr) {
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lChain; // chain
			lStream << lId; // id
			Block::Serializer::serialize<DataStream>(lStream, lBlock); // block

			// prepare message
			Message lMessage(Message::NETWORK_BLOCK, lStream.size());
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending network block ") + strprintf("%s/%s#", lBlock->hash().toHex(), lChain.toHex().substr(0, 10)) + std::string(" for ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		} else {
			// block is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lChain; // chain
			lStream << lId; // id

			// prepare message
			Message lMessage(Message::NETWORK_BLOCK_IS_ABSENT, lStream.size());
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: network block is absent for chain ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)) + std::string(" -> ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		}

	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetNetworkBlock/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}	
}

void Peer::processBlockHeader(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: raw block header from ") + key() + " -> " + HexStr(msg->begin(), msg->end()));

		// extract block header data
		NetworkBlockHeader lNetworkBlockHeader;
		(*msg) >> lNetworkBlockHeader;
		eraseInData(msg);

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: process block header from ") + key() + " -> " + 
				strprintf("%s/%d/%s#", lNetworkBlockHeader.blockHeader().hash().toHex(), lNetworkBlockHeader.height(), lNetworkBlockHeader.blockHeader().chain().toHex().substr(0, 10)));

		if (lNetworkBlockHeader.blockHeader().origin() == peerManager_->consensusManager()->mainPKey().id()) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, "[peer]: skip re-broadcasting block header " + 
				strprintf("%s/%s/%s#", lNetworkBlockHeader.blockHeader().hash().toHex(), lNetworkBlockHeader.height(), lNetworkBlockHeader.blockHeader().chain().toHex().substr(0, 10)));
		} else if (peerManager_->consensusManager()->pushBlockHeader(lNetworkBlockHeader)) {
			// re-broadcast
			peerManager_->consensusManager()->broadcastBlockHeader(lNetworkBlockHeader, addressId());
		} else {
			// TODO: what to do with the peer? Do we need reason - why blockHeader was not pushed?
		}

	} else {
		// log
		gLog().write(Log::NET, "[peer/processBlockHeader/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processBlockHeaderAndState(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: raw block header and state from ") + key() + " -> " + HexStr(msg->begin(), msg->end()));

		// extract block header data
		NetworkBlockHeader lNetworkBlockHeader;
		(*msg) >> lNetworkBlockHeader;

		// extract state
		State lState;
		lState.deserialize<DataStream>(*msg);
		eraseInData(msg); // erase		

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing block header from ") + key() + " -> " + 
				strprintf("%d/%s/%s#", lNetworkBlockHeader.height(), lNetworkBlockHeader.blockHeader().hash().toHex(), lNetworkBlockHeader.blockHeader().chain().toHex().substr(0, 10)));

		if (lNetworkBlockHeader.blockHeader().origin() == peerManager_->consensusManager()->mainPKey().id()) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, "[peer]: skip re-processing block header " + 
				strprintf("%s/%s/%s#", lNetworkBlockHeader.blockHeader().hash().toHex(), lNetworkBlockHeader.height(), lNetworkBlockHeader.blockHeader().chain().toHex().substr(0, 10)));
		} else {
			//
			IValidator::BlockCheckResult lResult = peerManager_->consensusManager()->pushBlockHeader(lNetworkBlockHeader);
			switch(lResult) {
				case IValidator::SUCCESS:
				case IValidator::BROKEN_CHAIN: {
						// process state
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing state from ") + key() + " -> " + lState.toString());
						StatePtr lStatePtr = State::instance(lState);
						peerManager_->consensusManager()->pushState(lStatePtr);
					}
					break;
				/*
				case IValidator::BROKEN_CHAIN: {
						// synchronize ?
						IConsensusPtr lConsensus = peerManager_->consensusManager()->locate(lNetworkBlockHeader.blockHeader().chain());
						if (lConsensus) lConsensus->toNonSynchronized();
					}
					break;
				*/
				case IValidator::ORIGIN_NOT_ALLOWED:
						// quarantine? - just skip for now
					break;
				case IValidator::INTEGRITY_IS_INVALID:
						// ban
						peerManager_->ban(shared_from_this());
					break;
				case IValidator::ALREADY_PROCESSED:
						// skip
					break;
				case IValidator::VALIDATOR_ABSENT:
						// skip
					break;
				case IValidator::PEERS_IS_ABSENT:
						// skip
					break;
			}
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBlockHeader/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processTransaction(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: raw transaction from ") + key() + " -> " + HexStr(msg->begin(), msg->end()));

		// extract transaction data
		TransactionPtr lTx = Transaction::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: process transaction from ") + key() + " -> " + 
				strprintf("%s/%s#", lTx->id().toHex(), lTx->chain().toHex().substr(0, 10)));

		IMemoryPoolPtr lMempool = peerManager_->consensusManager()->mempoolManager()->locate(lTx->chain());
		if (lMempool) {
			//
			TransactionContextPtr lCtx = lMempool->pushTransaction(lTx);
			if (!lCtx) {
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: transaction is ALREADY exists ") + 
						strprintf("%s/%s#", lTx->id().toHex(), lTx->chain().toHex().substr(0, 10)));
			} else if (lCtx->errors().size()) {
				for (std::list<std::string>::iterator lErr = lCtx->errors().begin(); lErr != lCtx->errors().end(); lErr++) {
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: transaction processing ERROR - ") + (*lErr) +
							strprintf(" -> %s/%s#", lTx->id().toHex(), lTx->chain().toHex().substr(0, 10)));
				}
			} else {
				peerManager_->consensusManager()->broadcastTransaction(lCtx, addressId());
			}
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: chain is NOT SUPPORTED for ") + 
					strprintf("%s/%s#", lTx->id().toHex(), lTx->chain().toHex().substr(0, 10)));
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processBlockHeader/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::broadcastBlockHeader(const NetworkBlockHeader& blockHeader) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; } // TODO: connect will skip current call
	else if (socketStatus_ == CONNECTED) {

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// push blockheader
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);
		lStateStream << blockHeader;

		Message lMessage(Message::BLOCK_HEADER, lStateStream.size());
		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: broadcasting block header to ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

void Peer::broadcastBlockHeaderAndState(const NetworkBlockHeader& blockHeader, StatePtr state) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; } // TODO: connect will skip current call
	else if (socketStatus_ == CONNECTED) {

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// push blockheader
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);
		lStateStream << blockHeader;

		if (state->addressId() == peerManager_->consensusManager()->mainPKey().id()) {
			// own state
			state->serialize<DataStream>(lStateStream, peerManager_->consensusManager()->mainKey());
		} else {
			// impossible
			state->serialize<DataStream>(lStateStream);
		}

		Message lMessage(Message::BLOCK_HEADER_AND_STATE, lStateStream.size());
		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: broadcasting block header and state to ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

void Peer::broadcastTransaction(TransactionContextPtr ctx) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; } // TODO: connect will skip current call
	else if (socketStatus_ == CONNECTED) {

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// push tx
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);
		Transaction::Serializer::serialize<DataStream>(lStateStream, ctx->tx());

		Message lMessage(Message::TRANSACTION, lStateStream.size());
		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: broadcasting transaction to ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}	
}

void Peer::processRequestPeers(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		// extract peers
		std::vector<std::string> lOuterPeers;
		(*msg) >> lOuterPeers;
		eraseInData(msg);

		// push peers
		for (std::vector<std::string>::iterator lPeer = lOuterPeers.begin(); lPeer != lOuterPeers.end(); lPeer++) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: trying to add proposed peer - ") + (*lPeer));
			peerManager_->addPeer(*lPeer);
		}

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
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending peers list to ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::processSent, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	} else {
		// log
		gLog().write(Log::NET, "[peer/processRequestPeers/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processPeers(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: peers list from ") + key());

		// extract peers
		std::vector<std::string> lPeers;
		(*msg) >> lPeers;
		eraseInData(msg);

		for (std::vector<std::string>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: trying to add proposed peer - ") + (*lPeer));
			peerManager_->addPeer(*lPeer);	
		}

		//waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processPeers/error]: closing session " + key() + " -> " + error.message());
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
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: ping from ") + key());

		uint64_t lTimestamp;
		(*msg) >> lTimestamp;
		eraseInData(msg);

		Message lMessage(Message::PONG, sizeof(uint64_t));

		std::list<DataStream>::iterator lMsg = newOutMessage();
		(*lMsg) << lMessage;
		(*lMsg) << lTimestamp;

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: pong to ") + key());

		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::processSent, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	} else {
		// log
		gLog().write(Log::NET, "[peer/processPing/error]: closing session " + key() + " -> " + error.message());
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
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: pong from ") + key());

		uint64_t lTimestamp;
		(*msg) >> lTimestamp;
		eraseInData(msg);

		// update
		peerManager_->updatePeerLatency(shared_from_this(), (uint32_t)(getMicroseconds() - lTimestamp));

		//waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processPong/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processGlobalState(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: raw global state message from ") + key() + " -> " + HexStr(msg->begin(), msg->end()));

		State lState;
		lState.deserialize<DataStream>(*msg);
		eraseInData(msg); // erase

		StatePtr lOwnState = peerManager_->consensusManager()->currentState();
		if (lOwnState->address().id() == lState.address().id()) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, "[peer/processGlobalState]: skip re-broadcasting state for " + 
				strprintf("%s", lOwnState->address().toHex()));
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing global state from ") + key() + " -> " + lState.toString());

			if (lState.valid()) {
				StatePtr lStatePtr = State::instance(lState);
				peerManager_->consensusManager()->pushState(lStatePtr);
			} else {
				peerManager_->ban(shared_from_this());
			}
		}

		//waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGlobalState/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();				
	}
}

void Peer::processState(std::list<DataStream>::iterator msg, bool broadcast, const boost::system::error_code& error) {
	//
	if (!error) {
		//
		State lState;
		lState.deserialize<DataStream>(*msg);
		eraseInData(msg); // erase

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing state from ") + key() + " -> " + lState.toString());

		IPeer::UpdatePeerResult lPeerResult;
		if (!peerManager_->updatePeerState(shared_from_this(), lState, lPeerResult)) {
			// if peer aleady exists
			if (lPeerResult == IPeer::EXISTS) {
				Message lMessage(Message::PEER_EXISTS, sizeof(uint64_t));
				std::list<DataStream>::iterator lMsg = newOutMessage();

				(*lMsg) << lMessage;

				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: peer already exists ") + key());

				boost::asio::async_write(*socket_,
					boost::asio::buffer(lMsg->data(), lMsg->size()),
					boost::bind(
						&Peer::peerFinalize, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lPeerResult == IPeer::BAN) {
				Message lMessage(Message::PEER_BANNED, sizeof(uint64_t));
				std::list<DataStream>::iterator lMsg = newOutMessage();

				(*lMsg) << lMessage;

				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: peer banned ") + key());

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
		} else if (broadcast) {
			// send state only for inbound peers
			if (!isOutbound()) sendState();
			// broadcast state
			// peerManager_->consensusManager()->broadcastState(State::instance(lState), addressId());
		}
	} else {
		// log
		gLog().write(Log::NET, "[peer/processState/error]: closing session " + key() + " -> " + error.message());
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
	//waitForMessage();

	if (error) {
		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, "[peer/processSent/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;		
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::messageFinalize(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	if (msg != emptyOutMessage()) eraseOutMessage(msg);

	if (error) {
		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, "[peer/messageFinalize/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;		
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::peerFinalize(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	if (msg != emptyOutMessage()) eraseOutMessage(msg);
	// log
	if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, "[peer/peerFinalize/error]: finalizing session " + key());
	//
	socketStatus_ = ERROR;
	// try to deactivate peer
	peerManager_->deactivatePeer(shared_from_this());
	// close socket
	socket_->close();	
}

void Peer::connect() {
	if (socketType_ == SERVER) { 
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: server endpoint - connect() not allowed"));
		return;
	}

	if (socketType_ == CLIENT && (socketStatus_ == CLOSED || socketStatus_ == ERROR)) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: connecting ") + key());

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
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/resolve]: resolve failed for ") + key() + " -> " + error.message());
		socketStatus_ = ERROR;
		socket_ = nullptr;
	}
}

void Peer::connected(const boost::system::error_code& error, tcp::resolver::iterator endpoint_iterator) {
	if (!error) {
		socketStatus_ = CONNECTED;

		// connected
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: connected to ") + key());

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
		gLog().write(Log::NET, std::string("[peer/connect/error]: connection failed for ") + key() + " -> " + error.message());
		socketStatus_ = ERROR;
		socket_ = nullptr;
	}
}
