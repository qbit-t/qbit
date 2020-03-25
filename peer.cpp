#include "peer.h"

using namespace qbit;

qbit::PeerExtensions qbit::gPeerExtensions;

bool Peer::onQuarantine() {
	return peerManager_->consensusManager()->currentState()->height() < quarantine_;
}

//
// external procedure: just finalize message we need
//
void Peer::ping() {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED /*&& socketType_ == CLIENT*/) {

		uint64_t lTimestamp = getMicroseconds(); // time 1580721029 | microseconds 1580721029.120664

		Message lMessage(Message::PING, sizeof(uint64_t), uint160());
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
	//
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

		Message lMessage((global ? Message::GLOBAL_STATE : Message::STATE), lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending ") + (global ? "global ": "") + std::string("state to ") + key() + " -> " + lState->toString());

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
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// register job
		addJob(consensus->chain(), job);

		// prepare message
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);
		uint64_t lHeight = job->acquireNextJob(shared_from_this());

		// done?
		if (!lHeight) {
			std::map<uint64_t, IPeerPtr> lPendingJobs = job->pendingJobs();
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
		Message lMessage(Message::GET_BLOCK_BY_HEIGHT, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

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
void Peer::synchronizePartialTree(IConsensusPtr consensus, SynchronizationJobPtr job) {
	//
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
				//
				consensus->toNonSynchronized();
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
		Message lMessage(Message::GET_BLOCK_BY_ID, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

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

//
// external repeatable procedure
//
void Peer::synchronizeLargePartialTree(IConsensusPtr consensus, SynchronizationJobPtr job) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {

		uint256 lId = job->nextBlock();
		if (lId.isNull()) {
			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: headers synchronization job is done, root = ") + strprintf("%s", job->block().toHex()));

			if (!consensus->processPartialTreeHeaders(job)) {
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: pending blocks synchronization is not possible, root = ") + strprintf("%s", job->block().toHex()));
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
		Message lMessage(Message::GET_BLOCK_HEADER, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: requesting block header ") + lId.toHex() + std::string(" from ") + key());

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
void Peer::synchronizePendingBlocks(IConsensusPtr consensus, SynchronizationJobPtr job) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// register job
		addJob(consensus->chain(), job);

		// prepare message
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);
		uint256 lBlockHeader = job->acquireNextPendingBlockJob(shared_from_this());

		// done?
		if (lBlockHeader.isNull()) {
			std::map<uint256, IPeerPtr> lPendingJobs = job->pendingBlockJobs();
			if (lPendingJobs.size()) 
				lBlockHeader = job->reacquirePendingBlockJob(lPendingJobs.rbegin()->first, shared_from_this());
			else {
				// log
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: pending blocks synchronization job is done, root = ") + strprintf("%s", job->block().toHex()));
				
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
					//
					consensus->toNonSynchronized();				
				}

				return;
			}
		}

		lStateStream << consensus->chain();
		lStateStream << lBlockHeader;
		Message lMessage(Message::GET_BLOCK_DATA, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: requesting BLOCK data ") + strprintf("%s", lBlockHeader.toHex()) + std::string(" from ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

void Peer::acquireBlock(const NetworkBlockHeader& block) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		uint256 lId = const_cast<NetworkBlockHeader&>(block).blockHeader().hash();
		lStateStream << const_cast<NetworkBlockHeader&>(block).blockHeader().chain();
		lStateStream << lId;
		Message lMessage(Message::GET_NETWORK_BLOCK, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

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

void Peer::acquireBlockHeaderWithCoinbase(const uint256& block, const uint256& chain, INetworkBlockHandlerWithCoinBasePtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		uint256 lRequestId = addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << block;
		Message lMessage(Message::GET_NETWORK_BLOCK_HEADER, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: acquiring block header ") + block.toHex() + std::string(" from ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

void Peer::loadTransaction(const uint256& chain, const uint256& tx, ILoadTransactionHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		uint256 lRequestId = addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << tx;
		Message lMessage(Message::GET_TRANSACTION_DATA, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: loading transaction ") + tx.toHex() + std::string(" from ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

void Peer::loadEntity(const std::string& name, ILoadEntityHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		uint256 lRequestId = addRequest(handler);
		std::string lName(name);
		entity_name_t lLimitedName(lName);
		lStateStream << lRequestId;
		lStateStream << lLimitedName;
		Message lMessage(Message::GET_ENTITY, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: loading entity '") + name + std::string("' from ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

void Peer::selectUtxoByAddress(const PKey& source, const uint256& chain, ISelectUtxoByAddressHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		uint256 lRequestId = addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << source;
		lStateStream << chain;
		Message lMessage(Message::GET_UTXO_BY_ADDRESS, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: selecting utxos by address ") + const_cast<PKey&>(source).toString() + std::string(" from ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}	
}

void Peer::selectUtxoByAddressAndAsset(const PKey& source, const uint256& chain, const uint256& asset, ISelectUtxoByAddressAndAssetHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		uint256 lRequestId = addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << source;
		lStateStream << chain;
		lStateStream << asset;
		Message lMessage(Message::GET_UTXO_BY_ADDRESS_AND_ASSET, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: selecting utxos by address and asset ") + strprintf("%s/%s", const_cast<PKey&>(source).toString(), asset.toHex()) + std::string(" from ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}	
}

void Peer::selectUtxoByTransaction(const uint256& chain, const uint256& tx, ISelectUtxoByTransactionHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		uint256 lRequestId = addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << tx;
		Message lMessage(Message::GET_UTXO_BY_TX, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: selecting utxos by transaction ") + strprintf("%s", tx.toHex()) + std::string(" from ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}	
}

void Peer::selectUtxoByEntity(const std::string& name, ISelectUtxoByEntityNameHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		uint256 lRequestId = addRequest(handler);
		std::string lName(name);
		entity_name_t lLimitedName(lName);
		lStateStream << lRequestId;
		lStateStream << lLimitedName;
		Message lMessage(Message::GET_UTXO_BY_ENTITY, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: selecting utxo by entity '") + name + std::string("' from ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

void Peer::selectEntityCountByShards(const std::string& name, ISelectEntityCountByShardsHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, CLIENT_VERSION);

		uint256 lRequestId = addRequest(handler);
		std::string lName(name);
		entity_name_t lLimitedName(lName);
		lStateStream << lRequestId;
		lStateStream << lLimitedName;
		Message lMessage(Message::GET_ENTITY_COUNT_BY_SHARDS, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: selecting entity count by shards for dapp '") + name + std::string("' from ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	}
}

void Peer::requestPeers() {	
	//
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
		Message lMessage(Message::GET_PEERS, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
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
	//
	if (waitingForMessage_) {
		// log
		gLog().write(Log::NET, std::string("[peer]: ALREADY waiting for message..."));
		return;
	}

	// mark
	waitingForMessage_ = true;

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
	// unmark
	waitingForMessage_ = false;

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

		//
		bool lSkipWaitForMessage = false;

		// process
		if (lMessage.valid() && lMessage.dataSize() < peerManager_->settings()->maxMessageSize()) {
			// new data entry
			std::list<DataStream>::iterator lMsg = newInData(lMessage);
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

			// try proto extension
			bool lHandled = false;
			std::map<std::string, IPeerExtensionPtr> lExtensions = extensions();
			for (std::map<std::string, IPeerExtensionPtr>::iterator lExtension = lExtensions.begin(); lExtension != lExtensions.end(); lExtension++) {
				lHandled = lExtension->second->processMessage(lMessage, lMsg, error);
				if (lHandled) break;
			}

			if (lHandled) { // handled in proto extension, just continue
				return;
			}

			//
			// WARNING: for short requests, but for long - we _must_ to go to wait until we read and process data
			//
			lSkipWaitForMessage = 
				lMessage.type() == Message::BLOCK_HEADER || 
				lMessage.type() == Message::BLOCK_BY_HEIGHT || 
				lMessage.type() == Message::BLOCK_BY_ID || 
				lMessage.type() == Message::BLOCK ||
				lMessage.type() == Message::NETWORK_BLOCK ||
				lMessage.type() == Message::NETWORK_BLOCK_HEADER ||
				lMessage.type() == Message::TRANSACTION ||
				lMessage.type() == Message::BLOCK_HEADER_AND_STATE ||
				lMessage.type() == Message::STATE ||
				lMessage.type() == Message::GLOBAL_STATE ||
				lMessage.type() == Message::TRANSACTION_DATA ||
				lMessage.type() == Message::UTXO_BY_ADDRESS ||
				lMessage.type() == Message::UTXO_BY_ADDRESS_AND_ASSET ||
				lMessage.type() == Message::UTXO_BY_TX ||
				lMessage.type() == Message::UTXO_BY_ENTITY ||
				lMessage.type() == Message::ENTITY;

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
			} else if (lMessage.type() == Message::GET_NETWORK_BLOCK_HEADER) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetNetworkBlockHeader, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::GET_BLOCK_HEADER) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetBlockHeader, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::GET_BLOCK_DATA) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetBlockData, shared_from_this(), lMsg,
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
			} else if (lMessage.type() == Message::BLOCK) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processBlock, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::NETWORK_BLOCK) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processNetworkBlock, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::NETWORK_BLOCK_HEADER) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processNetworkBlockHeader, shared_from_this(), lMsg,
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
			} else if (lMessage.type() == Message::NETWORK_BLOCK_HEADER_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processNetworkBlockHeaderAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::BLOCK_HEADER_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processBlockHeaderAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::BLOCK_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processBlockAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::GET_PEERS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processRequestPeers, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			// transactions & utxo
			} else if (lMessage.type() == Message::GET_TRANSACTION_DATA) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetTransactionData, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::TRANSACTION_DATA) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processTransactionData, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::TRANSACTION_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processTransactionAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error));

			} else if (lMessage.type() == Message::GET_UTXO_BY_ADDRESS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetUtxoByAddress, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::UTXO_BY_ADDRESS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processUtxoByAddress, shared_from_this(), lMsg,
						boost::asio::placeholders::error));

			} else if (lMessage.type() == Message::GET_UTXO_BY_ADDRESS_AND_ASSET) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetUtxoByAddressAndAsset, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::UTXO_BY_ADDRESS_AND_ASSET) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processUtxoByAddressAndAsset, shared_from_this(), lMsg,
						boost::asio::placeholders::error));

			} else if (lMessage.type() == Message::GET_UTXO_BY_TX) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetUtxoByTransaction, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::UTXO_BY_TX) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processUtxoByTransaction, shared_from_this(), lMsg,
						boost::asio::placeholders::error));

			} else if (lMessage.type() == Message::GET_ENTITY) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetEntity, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::ENTITY) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processEntity, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::ENTITY_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processEntityAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error));

			} else if (lMessage.type() == Message::GET_UTXO_BY_ENTITY) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetUtxoByEntity, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::GET_ENTITY_COUNT_BY_SHARDS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processGetEntityCountByShards, shared_from_this(), lMsg,
						boost::asio::placeholders::error));

			} else if (lMessage.type() == Message::UTXO_BY_ENTITY) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processUtxoByEntity, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lMessage.type() == Message::ENTITY_COUNT_BY_SHARDS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					boost::bind(
						&Peer::processEntityCountByShards, shared_from_this(), lMsg,
						boost::asio::placeholders::error));

			} else if (lMessage.type() == Message::CLIENT_SESSIONS_EXCEEDED) {
				// postpone peer
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: node sessions exceeded, postponing connections for ") + key());
				peerManager_->postpone(shared_from_this());
				eraseInData(lMsg);
			} else if (lMessage.type() == Message::PEER_EXISTS) {
				// mark peer
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: peer already exists ") + key());				
				peerManager_->postpone(shared_from_this());
				eraseInData(lMsg);
			} else if (lMessage.type() == Message::PEER_BANNED) {
				// mark peer
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: peer banned ") + key());				
				peerManager_->ban(shared_from_this());
				eraseInData(lMsg);
			} else {
				eraseInData(lMsg);
			}
		} else {
			// message too large or invalid
			// TODO: quarantine?
			// peerManager_->ban(shared_from_this());
			// close
			if (key() != "EKEY") {
				gLog().write(Log::NET, "[peer/processMessage/error]: closing session " + key() + 
					strprintf(" -> Message size >= %d or invalid, ", peerManager_->settings()->maxMessageSize()) + statusString());
				//
				socketStatus_ = ERROR;
				// try to deactivate peer
				peerManager_->deactivatePeer(shared_from_this());
				// close socket
				socket_->close();
			}

			return;
		}
		
		if (!lSkipWaitForMessage) 
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

void Peer::processGetTransactionData(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request transacion data from ") + key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lTxId;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lTxId;
		eraseInData(msg);

		TransactionPtr lTx = nullptr;
		if (lChain.isNull()) {
			std::vector<ITransactionStorePtr> lStorages = peerManager_->consensusManager()->storeManager()->storages();
			for (std::vector<ITransactionStorePtr>::iterator lStore = lStorages.begin(); lStore != lStorages.end(); lStore++) {
				lTx = (*lStore)->locateTransaction(lTxId);
				if (lTx) break;
			}
		} else {
			ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
			lTx = lStorage->locateTransaction(lTxId);
		}

		if (lTx != nullptr) {
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lRequestId;
			Transaction::Serializer::serialize<DataStream>(lStream, lTx); // tx

			// prepare message
			Message lMessage(Message::TRANSACTION_DATA, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending transacion data for tx = ") + strprintf("%s", lTx->id().toHex()) + std::string(" for ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		} else {
			// tx is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lRequestId;
			lStream << lTxId; // tx

			// prepare message
			Message lMessage(Message::TRANSACTION_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: transaction is absent ") + strprintf("%s", lTxId.toHex()) + std::string(" -> ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		}

	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetTransactionData/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processGetEntity(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request entity from ") + key());

		// extract
		uint256 lRequestId;
		std::string lName;
		entity_name_t lLimitedName(lName);

		(*msg) >> lRequestId;
		(*msg) >> lLimitedName;
		eraseInData(msg);

		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		EntityPtr lTx = lStorage->entityStore()->locateEntity(lName);

		if (lTx != nullptr) {
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lRequestId;
			Transaction::Serializer::serialize<DataStream>(lStream, lTx); // tx

			// prepare message
			Message lMessage(Message::ENTITY, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending entity ") + strprintf("'%s'/%s", lName, lTx->id().toHex()) + std::string(" for ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		} else {
			// tx is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lRequestId;
			lStream << lLimitedName; // tx

			// prepare message
			Message lMessage(Message::ENTITY_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: entity is absent ") + strprintf("'%s'", lName) + std::string(" -> ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		}

	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetTransactionData/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processGetUtxoByEntity(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request utxo by entity from ") + key());

		// extract
		uint256 lRequestId;
		std::string lName;
		entity_name_t lLimitedName(lName);

		(*msg) >> lRequestId;
		(*msg) >> lLimitedName;
		eraseInData(msg);

		// make message, serialize, send back
		std::list<DataStream>::iterator lMsg = newOutMessage();
		// fill data
		DataStream lStream(SER_NETWORK, CLIENT_VERSION);
		lStream << lRequestId;
		lStream << lLimitedName;

		std::vector<Transaction::UnlinkedOutPtr> lUtxos;
		std::vector<Transaction::UnlinkedOut> lUtxosObj;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		lStorage->entityStore()->collectUtxoByEntityName(lName, lUtxos);
		for (std::vector<Transaction::UnlinkedOutPtr>::iterator lItem = lUtxos.begin(); lItem != lUtxos.end(); lItem++) {
			lUtxosObj.push_back(*(*lItem));
		}
		lStream << lUtxosObj;

		// prepare message
		Message lMessage(Message::UTXO_BY_ENTITY, lStream.size(), Hash160(lStream.begin(), lStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStream.data(), lStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending utxo by entity ") + strprintf("'%s'/%d", lName, lUtxos.size()) + std::string(" for ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetUtxoByEntity/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processGetEntityCountByShards(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request entity count by shards from ") + key());

		// extract
		uint256 lRequestId;
		std::string lName;
		entity_name_t lLimitedName(lName);

		(*msg) >> lRequestId;
		(*msg) >> lLimitedName;
		eraseInData(msg);

		// make message, serialize, send back
		std::list<DataStream>::iterator lMsg = newOutMessage();
		// fill data
		DataStream lStream(SER_NETWORK, CLIENT_VERSION);
		lStream << lRequestId;
		lStream << lLimitedName;

		std::map<uint32_t, uint256> lShardInfo;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		//
		std::vector<ISelectEntityCountByShardsHandler::EntitiesCount> lEntitiesCount;
		if (lStorage->entityStore()->entityCountByShards(lName, lShardInfo)) {
			for (std::map<uint32_t, uint256>::iterator lItem = lShardInfo.begin(); lItem != lShardInfo.end(); lItem++) {
				//
				lEntitiesCount.push_back(ISelectEntityCountByShardsHandler::EntitiesCount(lItem->second, lItem->first));
			}
		}
		lStream << lEntitiesCount;

		// prepare message
		Message lMessage(Message::ENTITY_COUNT_BY_SHARDS, lStream.size(), Hash160(lStream.begin(), lStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStream.data(), lStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending entity count by shards ") + strprintf("'%s'/%d", lName, lShardInfo.size()) + std::string(" for ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetEntityCountByShards/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processTransactionAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: transaction is absent from ") + key());

		// extract
		uint256 lRequestId;
		uint256 lTxId;
		(*msg) >> lRequestId;
		(*msg) >> lTxId;
		eraseInData(msg);

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadTransactionHandler>(lHandler)->handleReply(nullptr);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND, ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: transaction is absent ") + strprintf("%s#", lTxId.toHex()));

	} else {
		// log
		gLog().write(Log::NET, "[peer/processTransactionAbsent/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processTransactionData(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response transaction data from ") + key());

		// extract
		uint256 lRequestId;
		TransactionPtr lTx;

		(*msg) >> lRequestId;
		lTx = Transaction::Deserializer::deserialize<DataStream>(*msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing transaction ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadTransactionHandler>(lHandler)->handleReply(lTx);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for transaction ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processTransactionData/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processEntityAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: entity is absent from ") + key());

		// extract
		uint256 lRequestId;
		std::string lName;
		entity_name_t lLimitedName(lName);
		(*msg) >> lRequestId;
		(*msg) >> lLimitedName;
		eraseInData(msg);

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadEntityHandler>(lHandler)->handleReply(nullptr);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND, ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: entity is absent - ") + strprintf("'%s'", lName));

	} else {
		// log
		gLog().write(Log::NET, "[peer/processTransactionAbsent/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processEntity(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response entity from ") + key());

		// extract
		uint256 lRequestId;
		TransactionPtr lTx;

		(*msg) >> lRequestId;
		lTx = Transaction::Deserializer::deserialize<DataStream>(*msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing entity ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadEntityHandler>(lHandler)->handleReply(TransactionHelper::to<Entity>(lTx));
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for entity ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processTransactionData/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processGetUtxoByAddress(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request for utxo by address from ") + key());

		// extract
		uint256 lRequestId;
		PKey lPKey;
		uint256 lChain;
		(*msg) >> lRequestId;
		(*msg) >> lPKey;
		(*msg) >> lChain;
		eraseInData(msg);

		// with assets
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lChain);
		std::vector<Transaction::NetworkUnlinkedOut> lOuts;

		lStore->selectUtxoByAddress(lPKey, lOuts);

		// make message, serialize, send back
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// fill data
		DataStream lStream(SER_NETWORK, CLIENT_VERSION);
		lStream << lRequestId;
		lStream << lPKey;
		lStream << lChain;
		lStream << lOuts;

		// prepare message
		Message lMessage(Message::UTXO_BY_ADDRESS, lStream.size(), Hash160(lStream.begin(), lStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStream.data(), lStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending utxo by address ") + strprintf("count = %d, address = %s", lOuts.size(), lPKey.toString()) + std::string(" for ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetUtxoByAddress/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processGetUtxoByAddressAndAsset(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request for utxo by address and asset from ") + key());

		// extract
		uint256 lRequestId;
		PKey lPKey;
		uint256 lChain;
		uint256 lAsset;
		(*msg) >> lRequestId;
		(*msg) >> lPKey;
		(*msg) >> lChain;
		(*msg) >> lAsset;
		eraseInData(msg);

		// with assets
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lChain);
		std::vector<Transaction::NetworkUnlinkedOut> lOuts;

		lStore->selectUtxoByAddressAndAsset(lPKey, lAsset, lOuts);

		// make message, serialize, send back
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// fill data
		DataStream lStream(SER_NETWORK, CLIENT_VERSION);
		lStream << lRequestId;
		lStream << lPKey;
		lStream << lChain;
		lStream << lAsset;
		lStream << lOuts;

		// prepare message
		Message lMessage(Message::UTXO_BY_ADDRESS_AND_ASSET, lStream.size(), Hash160(lStream.begin(), lStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStream.data(), lStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending utxo by address and asset ") + strprintf("count = %d, address = %s, asset = %s", lOuts.size(), lPKey.toString(), lAsset.toHex()) + std::string(" for ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetUtxoByAddressAndAsset/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processGetUtxoByTransaction(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request for utxo by transaction from ") + key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lTx;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lTx;
		eraseInData(msg);

		// with assets - only mainchain
		ITransactionStorePtr lStore = peerManager_->consensusManager()->storeManager()->locate(lChain);
		std::vector<Transaction::NetworkUnlinkedOut> lOuts;

		lStore->selectUtxoByTransaction(lTx, lOuts);

		// make message, serialize, send back
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// fill data
		DataStream lStream(SER_NETWORK, CLIENT_VERSION);
		lStream << lRequestId;
		lStream << lChain;
		lStream << lTx;
		lStream << lOuts;

		// prepare message
		Message lMessage(Message::UTXO_BY_TX, lStream.size(), Hash160(lStream.begin(), lStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStream.data(), lStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending utxo by transaction ") + strprintf("count = %d, tx = %s", lOuts.size(), lTx.toHex()) + std::string(" for ") + key());

		// write
		boost::asio::async_write(*socket_,
			boost::asio::buffer(lMsg->data(), lMsg->size()),
			boost::bind(
				&Peer::messageFinalize, shared_from_this(), lMsg,
				boost::asio::placeholders::error));
	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetUtxoByTransaction/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processUtxoByAddress(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response utxo by address from ") + key());

		// extract
		uint256 lRequestId;
		PKey lPKey;
		uint256 lChain;
		std::vector<Transaction::NetworkUnlinkedOut> lOuts;

		(*msg) >> lRequestId;
		(*msg) >> lPKey;
		(*msg) >> lChain;
		(*msg) >> lOuts;

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing utxo by address ") + strprintf("r = %s, %d/%s", lRequestId.toHex(), lOuts.size(), lPKey.toString()) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectUtxoByAddressHandler>(lHandler)->handleReply(lOuts, lPKey);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for utxo by address ") + strprintf("r = %s, %s", lRequestId.toHex(), lPKey.toString()) + std::string("..."));
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processUtxoByAddress/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processUtxoByAddressAndAsset(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response utxo by address and asset from ") + key());

		// extract
		uint256 lRequestId;
		PKey lPKey;
		uint256 lChain;
		uint256 lAsset;
		std::vector<Transaction::NetworkUnlinkedOut> lOuts;

		(*msg) >> lRequestId;
		(*msg) >> lPKey;
		(*msg) >> lChain;
		(*msg) >> lAsset;		
		(*msg) >> lOuts;

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing utxo by address and asset ") + strprintf("r = %s, %d/%s/%s", lRequestId.toHex(), lOuts.size(), lPKey.toString(), lAsset.toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectUtxoByAddressAndAssetHandler>(lHandler)->handleReply(lOuts, lPKey, lAsset);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for utxo by asset ") + strprintf("r = %s, %s", lRequestId.toHex(), lAsset.toHex()) + std::string("..."));
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processUtxoByAddressAndAsset/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processUtxoByTransaction(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response utxo by transaction from ") + key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lTx;
		std::vector<Transaction::NetworkUnlinkedOut> lOuts;

		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lTx;		
		(*msg) >> lOuts;

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing utxo by transaction ") + strprintf("r = %s, %d/%s", lRequestId.toHex(), lOuts.size(), lTx.toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectUtxoByTransactionHandler>(lHandler)->handleReply(lOuts, lTx);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for utxo by tx ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx.toHex()) + std::string("..."));
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processUtxoByTransaction/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processUtxoByEntity(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response utxo by entity from ") + key());

		// extract
		uint256 lRequestId;
		std::string lName;
		entity_name_t lLimitedName(lName);
		std::vector<Transaction::UnlinkedOut> lOuts;

		(*msg) >> lRequestId;
		(*msg) >> lLimitedName;
		(*msg) >> lOuts;

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing utxo by entity ") + strprintf("r = %s, s = %d, e = '%s'", lRequestId.toHex(), lOuts.size(), lName) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectUtxoByEntityNameHandler>(lHandler)->handleReply(lOuts, lName);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for utxo by entity ") + strprintf("r = %s, e = '%s'", lRequestId.toHex(), lName) + std::string("..."));
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processUtxoByEntity/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processEntityCountByShards(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response entity count by shards from ") + key());

		// extract
		uint256 lRequestId;
		std::string lName;
		entity_name_t lLimitedName(lName);
		std::vector<ISelectEntityCountByShardsHandler::EntitiesCount> lEntitiesCount;

		(*msg) >> lRequestId;
		(*msg) >> lLimitedName;
		(*msg) >> lEntitiesCount;
		eraseInData(msg);

		std::map<uint32_t, uint256> lShardInfo;
		for (std::vector<ISelectEntityCountByShardsHandler::EntitiesCount>::iterator lItem = lEntitiesCount.begin(); lItem != lEntitiesCount.end(); lItem++) {
			//
			lShardInfo.insert(std::map<uint32_t, uint256>::value_type((*lItem).count(), (*lItem).shard()));
		}

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing entity count by shards ") + strprintf("r = %s, s = %d, e = '%s'", lRequestId.toHex(), lShardInfo.size(), lName) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectEntityCountByShardsHandler>(lHandler)->handleReply(lShardInfo, lName);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for entity count by shards ") + strprintf("r = %s, e = '%s'", lRequestId.toHex(), lName) + std::string("..."));
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processEntityCountByShards/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processBlockByHeightAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block by height is absent from ") + key());

		// extract
		uint256 lChain;
		uint64_t lHeight;
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

void Peer::processNetworkBlockHeaderAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: network block header is absent from ") + key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lId;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lId;
		eraseInData(msg);

		// clean-up
		removeRequest(lRequestId);
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: network block header is absent for ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)));

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

void Peer::processBlockHeaderAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block header is absent from ") + key());

		// extract
		uint256 lChain;
		uint256 lId;
		(*msg) >> lChain;
		(*msg) >> lId;
		eraseInData(msg);
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block header is absent for ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)));

	} else {
		// log
		gLog().write(Log::NET, "[peer/processBlockHeaderAbsent/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

void Peer::processBlockAbsent(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block is absent from ") + key());

		// extract
		uint256 lChain;
		uint256 lId;
		(*msg) >> lChain;
		(*msg) >> lId;
		eraseInData(msg);
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block is absent for ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)));

	} else {
		// log
		gLog().write(Log::NET, "[peer/processBlockAbsent/error]: closing session " + key() + " -> " + error.message());
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response for block by height from ") + key());

		// extract
		uint64_t lHeight;
		(*msg) >> lHeight;
		BlockPtr lBlock = Block::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		// locate job
		SynchronizationJobPtr lJob = locateJob(lBlock->chain());
		if (lJob && lJob->releaseJob(lHeight)) {
			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing BLOCK ") + strprintf("%s/%d", lBlock->hash().toHex(), lHeight) + std::string("..."));
			// save block
			peerManager_->consensusManager()->locate(lBlock->chain())->store()->saveBlock(lBlock);
		} else {
			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::WARNING, std::string("[peer]: local job was not found for ") + strprintf("%d/%s#", lHeight, lBlock->chain().toHex().substr(0, 10)));
		}

		// go do next job
		if (lJob)  {
			synchronizeFullChain(peerManager_->consensusManager()->locate(lBlock->chain()), lJob);
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request block by height from ") + key());

		// extract
		uint256 lChain;
		uint64_t lHeight;
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
			lStream << lHeight;
			Block::Serializer::serialize<DataStream>(lStream, lBlock); // block

			// prepare message
			Message lMessage(Message::BLOCK_BY_HEIGHT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
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
			Message lMessage(Message::BLOCK_BY_HEIGHT_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response for block by id from ") + key());

		// extract
		BlockPtr lBlock = Block::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		// locate job
		SynchronizationJobPtr lJob = locateJob(lBlock->chain());
		if (lJob) {
			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing block ") + strprintf("%s/%s#", lBlock->hash().toHex(), lBlock->chain().toHex().substr(0, 10)) + std::string("..."));
			// save block
			peerManager_->consensusManager()->locate(lBlock->chain())->store()->saveBlock(lBlock);
			// extract next block id
			uint64_t lHeight;
			uint256 lPrev = lBlock->prev();
			if (lPrev != BlockHeader().hash()) { // BlockHeader().hash() - final/absent link 
				if (!peerManager_->consensusManager()->locate(lBlock->chain())->store()->blockHeight(lPrev, lHeight)) {
					// go do next job
					lJob->setNextBlock(lPrev);
				} else {
					// we found root
					lJob->setLastBlock(lPrev);
				}
			} else {
				// we have new shiny full chain
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: full chain found, switching to ") + 
					strprintf("head = %s, root = %s/%s#", lBlock->hash().toHex(), lJob->block().toHex(), lBlock->chain().toHex().substr(0, 10)) + std::string("..."));
				lJob->setLastBlock(lPrev);
			}
			
			synchronizePartialTree(peerManager_->consensusManager()->locate(lBlock->chain()), lJob);
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
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

void Peer::processBlock(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response for block from ") + key());

		// extract
		BlockPtr lBlock = Block::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		// locate job
		SynchronizationJobPtr lJob = locateJob(lBlock->chain());
		if (lJob) {
			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing block ") + strprintf("%s/%s#", lBlock->hash().toHex(), lBlock->chain().toHex().substr(0, 10)) + std::string("..."));
			// save block
			peerManager_->consensusManager()->locate(lBlock->chain())->store()->saveBlock(lBlock);
			// release job
			lJob->releasePendingBlockJob(lBlock->hash());
			// move to the next job
			synchronizePendingBlocks(peerManager_->consensusManager()->locate(lBlock->chain()), lJob);
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
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
			Block::Serializer::serialize<DataStream>(lStream, lBlock); // block

			// prepare message
			Message lMessage(Message::BLOCK_BY_ID, lStream.size(), Hash160(lStream.begin(), lStream.end()));
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
			Message lMessage(Message::BLOCK_BY_ID_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
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

void Peer::processGetBlockData(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request block data from ") + key());

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
			Block::Serializer::serialize<DataStream>(lStream, lBlock); // block

			// prepare message
			Message lMessage(Message::BLOCK, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending block data ") + strprintf("%s/%s#", lBlock->hash().toHex(), lChain.toHex().substr(0, 10)) + std::string(" for ") + key());

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
			Message lMessage(Message::BLOCK_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block data is absent for chain ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)) + std::string(" -> ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		}

	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBlockData/error]: closing session " + key() + " -> " + error.message());
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response for network block from ") + key());

		// extract
		BlockPtr lBlock = Block::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing network block ") + strprintf("%s/%s#", lBlock->hash().toHex(), lBlock->chain().toHex().substr(0, 10)) + std::string("..."));

		BlockHeader lHeader;
		uint64_t lHeight = peerManager_->consensusManager()->locate(lBlock->chain())->store()->currentHeight(lHeader);

		// check sequence
		uint256 lCurrentHash = lHeader.hash();
		if (lCurrentHash != lBlock->prev()) {
			// sequence is broken
			if (lCurrentHash == lBlock->hash()) {
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block already EXISTS ") + 
					strprintf("current height:%d/hash:%s, proposing hash:%s/%s#", 
						lHeight, lHeader.hash().toHex(), lBlock->hash().toHex(), lBlock->chain().toHex().substr(0, 10)));
			} else { 
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: blocks sequence is BROKEN ") + 
					strprintf("current height:%d/hash:%s, proposing hash:%s/prev:%s/%s#", 
						lHeight, lHeader.hash().toHex(), lBlock->hash().toHex(), lBlock->prev().toHex(), lBlock->chain().toHex().substr(0, 10)));
			}
		} else {
			// save block
			BlockContextPtr lCtx = peerManager_->consensusManager()->locate(lBlock->chain())->store()->pushBlock(lBlock);
			if (lCtx) {
				if (!lCtx->errors().size()) {
					// clean-up mempool
					peerManager_->memoryPoolManager()->locate(lBlock->chain())->removeTransactions(lBlock);
					// create state and block info
					StatePtr lState = peerManager_->consensusManager()->currentState();
					NetworkBlockHeader lHeader(lCtx->block()->blockHeader(), lCtx->height());
					// broadcast block and state
					peerManager_->consensusManager()->broadcastBlockHeaderAndState(lHeader, lState, lState->addressId());
				} else {
					// TODO: quarantine
					//peerManager_->ban(shared_from_this());
				}
			}
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
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

void Peer::processNetworkBlockHeader(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response for network block header from ") + key());

		// extract
		uint256 lRequestId;
		NetworkBlockHeader lNetworkBlockHeader;
		TransactionPtr lBase;

		(*msg) >> lRequestId;
		(*msg) >> lNetworkBlockHeader;
		lBase = Transaction::Deserializer::deserialize<DataStream>(*msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing network block header ") + strprintf("r = %s, %s/%s#", lRequestId.toHex(), lNetworkBlockHeader.blockHeader().hash().toHex(), lNetworkBlockHeader.blockHeader().chain().toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<INetworkBlockHandlerWithCoinBase>(lHandler)->handleReply(lNetworkBlockHeader, lBase);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for network block header ") + strprintf("r = %s, %s/%s#", lRequestId.toHex(), lNetworkBlockHeader.blockHeader().hash().toHex(), lNetworkBlockHeader.blockHeader().chain().toHex().substr(0, 10)) + std::string("..."));
		}

		// WARNING: in case of async_read for large data
		waitForMessage();
	} else {
		// log
		gLog().write(Log::NET, "[peer/processNetworkBlockHeader/error]: closing session " + key() + " -> " + error.message());
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
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
			Block::Serializer::serialize<DataStream>(lStream, lBlock); // block

			// prepare message
			Message lMessage(Message::NETWORK_BLOCK, lStream.size(), Hash160(lStream.begin(), lStream.end()));
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
			Message lMessage(Message::NETWORK_BLOCK_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
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

void Peer::processGetNetworkBlockHeader(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request network block header from ") + key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		uint256 lId;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lId;
		eraseInData(msg);

		// get block header
		uint64_t lHeight;
		BlockHeader lBlockHeader;
		ITransactionStorePtr lStore = peerManager_->consensusManager()->locate(lChain)->store();
		//
		if (lStore->blockHeaderHeight(lId, lHeight, lBlockHeader)) {
			//
			BlockHeader lCurrentBlockHeader;
			uint64_t lCurrentHeight = lStore->currentHeight(lCurrentBlockHeader);

			uint64_t lConfirms = 0;
			if (lCurrentHeight > lHeight) lConfirms = lCurrentHeight - lHeight;

			// load block and extract ...base (coinbase\base) transaction
			BlockPtr lBlock = lStore->block(lId);
			TransactionPtr lTx = *lBlock->transactions().begin();

			// make network block header
			NetworkBlockHeader lNetworkBlockHeader(lBlockHeader, lHeight, lConfirms);

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lRequestId;
			lStream << lNetworkBlockHeader;
			Transaction::Serializer::serialize<DataStream>(lStream, lTx);

			// prepare message
			Message lMessage(Message::NETWORK_BLOCK_HEADER, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending network block header ") + strprintf("%s/%s#", lBlock->hash().toHex(), lChain.toHex().substr(0, 10)) + std::string(" for ") + key());

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
			lStream << lRequestId;
			lStream << lChain; // chain
			lStream << lId; // id

			// prepare message
			Message lMessage(Message::NETWORK_BLOCK_HEADER_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: network block header is absent for chain ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)) + std::string(" -> ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		}

	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetNetworkBlockHeader/error]: closing session " + key() + " -> " + error.message());
		//
		socketStatus_ = ERROR;
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}	
}

void Peer::processGetBlockHeader(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request for block header(s) from ") + key());

		// extract
		uint256 lChain;
		uint256 lId;
		(*msg) >> lChain;
		(*msg) >> lId;
		eraseInData(msg);

		// get header
		uint64_t lHeight;
		uint256 lCurrent = lId;
		BlockHeader lHeader;
		if (peerManager_->consensusManager()->locate(lChain)->store()->blockHeaderHeight(lCurrent, lHeight, lHeader)) {
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			size_t lCount = 0;
			std::vector<NetworkBlockHeader> lHeaders;
			do {
				//
				lHeaders.push_back(NetworkBlockHeader(lHeader, lHeight));
				lCurrent = lHeader.prev();
			} while(peerManager_->consensusManager()->locate(lChain)->store()->blockHeaderHeight(lCurrent, lHeight, lHeader) && (++lCount) < 1000);

			// fill data
			DataStream lStream(SER_NETWORK, CLIENT_VERSION);
			lStream << lHeaders;

			// prepare message
			Message lMessage(Message::BLOCK_HEADER, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending block header(s) ") + strprintf("%d/%s/%s#", lHeaders.size(), lId.toHex(), lChain.toHex().substr(0, 10)) + std::string(" for ") + key());

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
			Message lMessage(Message::BLOCK_HEADER_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block header is absent for chain ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)) + std::string(" -> ") + key());

			// write
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->data(), lMsg->size()),
				boost::bind(
					&Peer::messageFinalize, shared_from_this(), lMsg,
					boost::asio::placeholders::error));
		}

	} else {
		// log
		gLog().write(Log::NET, "[peer/processGetBlockHeader/error]: closing session " + key() + " -> " + error.message());
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: raw block header(s) from ") + key() + " -> " + HexStr(msg->begin(), msg->end()).substr(0, 100) + "#");

		// extract block header data
		std::vector<NetworkBlockHeader> lHeaders;
		(*msg) >> lHeaders;
		eraseInData(msg);

		if (!lHeaders.size()) { 
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block header(s) is EMPTY from ") + key() + " -> " + HexStr(msg->begin(), msg->end()));
			return;
		}

		//
		// locate job
		SynchronizationJobPtr lJob = locateJob(lHeaders.begin()->blockHeader().chain());
		IConsensusPtr lConsensus = peerManager_->consensusManager()->locate(lHeaders.begin()->blockHeader().chain());
		if (lJob) {
			//
			for (std::vector<NetworkBlockHeader>::iterator lHeader = lHeaders.begin(); lHeader != lHeaders.end(); lHeader++) {
				//
				BlockHeader lBlockHeader = (*lHeader).blockHeader();
				// log
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: process block header from ") + key() + " -> " + 
					strprintf("%s/%s#", lBlockHeader.hash().toHex(), lBlockHeader.chain().toHex().substr(0, 10)));

				// save
				lConsensus->store()->saveBlockHeader(lBlockHeader);
				lJob->pushPendingBlock(lBlockHeader.hash());				

				// extract next block id
				uint64_t lHeight;
				uint256 lPrev = lBlockHeader.prev();
				if (lPrev != BlockHeader().hash()) { // BlockHeader().hash() - final/absent link 
					if (!peerManager_->consensusManager()->locate(lBlockHeader.chain())->store()->blockHeight(lPrev, lHeight)) {
						// go do next job
						lJob->setNextBlock(lPrev);
					} else {
						// we found root
						lJob->setLastBlock(lPrev);
						break;
					}
				} else {
					// we have new shiny full chain
					if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: full chain found, try switching to ") + 
						strprintf("head = %s, root = %s/%s#", lBlockHeader.hash().toHex(), lJob->block().toHex(), lBlockHeader.chain().toHex().substr(0, 10)) + std::string("..."));
					lJob->setLastBlock(lPrev);
					break;
				}
			}
			
			if (lConsensus != nullptr) synchronizeLargePartialTree(lConsensus, lJob);

			// WARNING: exception - long reading procedure in async_read
			waitForMessage();
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
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
			
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing state from ") + key() + " -> " + lState.toString());
			
			StatePtr lStatePtr = State::instance(lState);
			IPeer::UpdatePeerResult lPeerResult;
			if (!peerManager_->updatePeerState(shared_from_this(), lState, lPeerResult)) {
				setState(lState);
				peerManager_->consensusManager()->pushPeer(shared_from_this());
			}
			peerManager_->consensusManager()->pushState(lStatePtr);
		} else {
			//
			IValidator::BlockCheckResult lResult = peerManager_->consensusManager()->pushBlockHeader(lNetworkBlockHeader);
			switch(lResult) {
				case IValidator::SUCCESS:
				case IValidator::BROKEN_CHAIN: {
						// process state
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing state from ") + key() + " -> " + lState.toString());
						StatePtr lStatePtr = State::instance(lState);
						IPeer::UpdatePeerResult lPeerResult;
						if (!peerManager_->updatePeerState(shared_from_this(), lState, lPeerResult)) {
							setState(lState);
							peerManager_->consensusManager()->pushPeer(shared_from_this());
						}
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

		// WARNING: in case of async_read for large data
		waitForMessage();		
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: raw transaction from ") + key() + " -> " + HexStr(msg->begin(), msg->end()));

		// extract transaction data
		TransactionPtr lTx = Transaction::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: process transaction from ") + key() + " -> " + 
				strprintf("%s/%s#", lTx->id().toHex(), lTx->chain().toHex().substr(0, 10)));

		if (peerManager_->settings()->isClient()) {
			// client-side
			peerManager_->consensusManager()->processTransaction(lTx);
		} else {
			// node-side
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
					// broadcast by nodes and fullnodes
					peerManager_->consensusManager()->broadcastTransaction(lCtx, addressId());

					// find and broadcast for active clients
					peerManager_->notifyTransaction(lCtx);
				}
			} else {
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: chain is NOT SUPPORTED for ") + 
						strprintf("%s/%s#", lTx->id().toHex(), lTx->chain().toHex().substr(0, 10)));
			}
		}

		// WARNING: in case of async_read for large data
		waitForMessage();	
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

		Message lMessage(Message::BLOCK_HEADER, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
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

		Message lMessage(Message::BLOCK_HEADER_AND_STATE, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
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

		Message lMessage(Message::TRANSACTION, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
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
		Message lMessage(Message::PEERS, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
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

		Message lMessage(Message::PONG, sizeof(uint64_t), uint160());

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

		time_ = lTimestamp;
		timestamp_ = getMicroseconds();

		// update and median time
		peerManager_->updatePeerLatency(shared_from_this(), (uint32_t)(getMicroseconds() - lTimestamp));
		peerManager_->updateMedianTime();

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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
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

		// WARNING: in case of async_read for large data
		waitForMessage();
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
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		//
		State lState;
		lState.deserialize<DataStream>(*msg);
		eraseInData(msg); // erase

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing state from ") + key() + " -> " + lState.toString());

		IPeer::UpdatePeerResult lPeerResult;
		if (!peerManager_->updatePeerState(shared_from_this(), lState, lPeerResult)) {
			// if peer aleady exists
			if (lPeerResult == IPeer::EXISTS) {
				Message lMessage(Message::PEER_EXISTS, sizeof(uint64_t), uint160());
				std::list<DataStream>::iterator lMsg = newOutMessage();

				(*lMsg) << lMessage;

				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: peer already exists ") + key());

				boost::asio::async_write(*socket_,
					boost::asio::buffer(lMsg->data(), lMsg->size()),
					boost::bind(
						&Peer::peerFinalize, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lPeerResult == IPeer::BAN) {
				Message lMessage(Message::PEER_BANNED, sizeof(uint64_t), uint160());
				std::list<DataStream>::iterator lMsg = newOutMessage();

				(*lMsg) << lMessage;

				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: peer banned ") + key());

				boost::asio::async_write(*socket_,
					boost::asio::buffer(lMsg->data(), lMsg->size()),
					boost::bind(
						&Peer::peerFinalize, shared_from_this(), lMsg,
						boost::asio::placeholders::error));
			} else if (lPeerResult == IPeer::SESSIONS_EXCEEDED) {
				Message lMessage(Message::CLIENT_SESSIONS_EXCEEDED, sizeof(uint64_t), uint160());
				std::list<DataStream>::iterator lMsg = newOutMessage();

				(*lMsg) << lMessage;

				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: node overloaded ") + key());

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
		}

		// WARNING: in case of async_read for large data
		waitForMessage();		
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
