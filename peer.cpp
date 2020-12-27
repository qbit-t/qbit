#include "peer.h"

using namespace qbit;

qbit::PeerExtensions qbit::gPeerExtensions;

bool qbit::gLightDaemon = false;

bool Peer::onQuarantine() {
	return peerManager_->consensusManager()->currentState()->height() < quarantine_;
}

void Peer::sendMessageAsync(std::list<DataStream>::iterator msg) {
	//
	std::list<OutMessage>::iterator lMsg;
	{
		boost::unique_lock<boost::mutex> lLock(rawOutMutex_);
		bool lSkip = outQueue_.size() > 0;
		lMsg = outQueue_.insert(outQueue_.end(), OutMessage(msg, (lSkip ? OutMessage::POSTPONED : OutMessage::QUEUED)));
		if (lSkip) return;
	}

	{
		boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
		if (socketStatus_ == CONNECTED) {
			boost::asio::async_write(*socket_,
				boost::asio::buffer(lMsg->msg()->data(), lMsg->msg()->size()),
				strand_->wrap(boost::bind(
					&Peer::messageSentAsync, shared_from_this(), lMsg,
					boost::asio::placeholders::error)));
		}
	}
}

void Peer::sendMessage(std::list<DataStream>::iterator msg) {
	//
	sendMessageAsync(msg);
}

void Peer::processPendingMessagesQueue() {
	//
	std::list<OutMessage>::iterator lMsg;
	bool lFound = false;
	{
		boost::unique_lock<boost::mutex> lLock(rawOutMutex_);
		bool lProcess = outQueue_.size() > 0;

		if (lProcess){
			lMsg = outQueue_.begin();
			for (; lMsg != outQueue_.end(); lMsg++) {
				if (lMsg->type() == OutMessage::POSTPONED) {
					lMsg->toQueued(); // we are ready to re-queue
					lFound = true;
					break;
				}
			}
		}
	}

	if (lFound) {
		bool lProcessed = false;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
			if (socketStatus_ == CONNECTED) {
				boost::asio::async_write(*socket_,
					boost::asio::buffer(lMsg->msg()->data(), lMsg->msg()->size()),
					strand_->wrap(boost::bind(
						&Peer::messageSentAsync, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
				//
				lProcessed = true;
			}
		}

		if (!lProcessed) {
			boost::unique_lock<boost::mutex> lLock(rawOutMutex_);
			outQueue_.clear();
			rawOutMessages_.clear();
		}
	}
	else 
		waitForMessage();
}

void Peer::messageSentAsync(std::list<OutMessage>::iterator msg, const boost::system::error_code& error) {
	//
	if (!eraseOutMessage(msg)) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/messageSentAsync]: queue is EMPTY"));
		return;
	}

	//
	if (error) {
		processError("messageSentAsync", error);
	} else {
		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/messageSentAsync]: for %s", key()));
		// re-process pending items
		processPendingMessagesQueue();
	}
}

void Peer::processError(const std::string& context, const boost::system::error_code& error) {
	// log
	if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer/%s/error]: closing session ", context) + key() + " -> " + error.message());
	//
	if (context != "messageSentAsync") { 
		boost::unique_lock<boost::recursive_mutex> lLock(readMutex_);
		reading_ = false;
	}

	//
	{
		boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
		//
		socketStatus_ = ERROR;		
		// try to deactivate peer
		peerManager_->deactivatePeer(shared_from_this());
		// close socket
		socket_->close();
	}
}

//
// external procedure: just finalize message we need
//
void Peer::ping() {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED && waitingForMessage_ &&
									!hasPendingRequests() && !hasActiveJobs()) {

		uint64_t lTimestamp = getMicroseconds(); // time 1580721029 | microseconds 1580721029.120664

		Message lMessage(Message::PING, sizeof(uint64_t), uint160());
		std::list<DataStream>::iterator lMsg = newOutMessage();

		(*lMsg) << lMessage;
		(*lMsg) << lTimestamp;

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: ping ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		sendMessageAsync(lMsg);
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
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);
		/*
		if (!global || state->addressId() == peerManager_->consensusManager()->mainPKey().id())  {
			lState->serialize<DataStream>(lStateStream, peerManager_->consensusManager()->mainKey());
		} else {
			lState->serialize<DataStream>(lStateStream);
		}
		*/
		lState->serialize<DataStream>(lStateStream, *(peerManager_->consensusManager()->mainKey()));

		Message lMessage((global ? Message::GLOBAL_STATE : Message::STATE), lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending ") + (global ? "global ": "") + std::string("state to ") + key() + " -> " + lState->toString());

		// write
		sendMessageAsync(lMsg);
	}
}

//
// external repeatable procedure
//
void Peer::synchronizeFullChain(IConsensusPtr consensus, SynchronizationJobPtr job, const boost::system::error_code& error) {
	//
	if (error == boost::asio::error::operation_aborted) return;

	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		// register job
		addJob(consensus->chain(), job);

		// prepare message
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);
		uint64_t lHeight = job->acquireNextJob(shared_from_this());

		// done?
		if (!lHeight) {
			std::map<uint64_t, SynchronizationJob::Agent> lPendingJobs = job->pendingJobs();
			if (lPendingJobs.size()) {
				//
				lHeight = job->reacquireJob(lPendingJobs.rbegin()->first, shared_from_this());
			} else {
				// log
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer]: synchronization job is done, root = ") + strprintf("%s, %s#", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));
				
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

		// double check
		if (!lHeight) {
			if (gLog().isEnabled(Log::CONSENSUS))
				gLog().write(Log::CONSENSUS, std::string("[peer]: synchronization job is done EARLIER, root = ") + strprintf("%s, %s#", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));
			return;
		}

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		lStateStream << consensus->chain();
		lStateStream << lHeight;
		Message lMessage(Message::GET_BLOCK_BY_HEIGHT, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer]: requesting BLOCK ") + strprintf("%d/%s#", lHeight, consensus->chain().toHex().substr(0, 10)) + std::string(" from ") + key());

		// write
		sendMessageAsync(lMsg);
	}
}

//
// NOTICE: the last and only WORKING proto, external repeatable procedure
//
void Peer::synchronizePartialTree(IConsensusPtr consensus, SynchronizationJobPtr job, const boost::system::error_code& error) {
	//
	if (error == boost::asio::error::operation_aborted) return;

	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		// register job
		addJob(consensus->chain(), job);
		//
		uint256 lId = job->nextBlock();
		if (lId.isNull()) {
			// log
			if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer]: partial synchronization job is done, root = ") + strprintf("%s, %s#", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));

			// cleanup
			removeJob(consensus->chain());
			// finish
			consensus->finishJob(job);

			// reindex, partial
			if (consensus->doIndex(job->block()/*root block*/, job->lastBlock())) {
				// broadcast new state
				StatePtr lState = peerManager_->consensusManager()->currentState();
				peerManager_->consensusManager()->broadcastState(lState, addressId());
			} else {
				// log
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer]: partial reindex FAILED skipping subtree switching, root = ") + strprintf("%s, %s#", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));
				//
				consensus->toNonSynchronized();
			}

			return;
		}

		/*
		// check if block exists
		uint256 lRootId = BlockHeader().hash();
		// end-of-chain OR last_block
		while ((lId != lRootId || lId != job->lastBlock()) && peerManager_->consensusManager()->locate(consensus->chain())->store()->blockExists(lId)) {
			//
			BlockHeader lHeader;
			if (peerManager_->consensusManager()->locate(consensus->chain())->store()->blockHeader(lId, lHeader)) {
				//
				lId = lHeader.prev();
			} else {
				break;
			}
		}

		// root found?
		if (lId == lRootId) {
			// log
			if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer/root]: patial synchronization job is done, root = ") + strprintf("%s, %s", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));

			// cleanup
			removeJob(consensus->chain());
			// finish
			consensus->finishJob(job);

			// reindex, partial
			if (consensus->doIndex(job->block(), job->lastBlock())) {
				// broadcast new state
				StatePtr lState = peerManager_->consensusManager()->currentState();
				peerManager_->consensusManager()->broadcastState(lState, addressId());
			} else {
				// log
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer/root]: partial reindex FAILED skipping subtree switching, root = ") + strprintf("%s, %s#", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));
				//
				consensus->toNonSynchronized();
			}

			return;
		}
		*/

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		lStateStream << consensus->chain();
		lStateStream << lId;
		Message lMessage(Message::GET_BLOCK_BY_ID, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, strprintf("[peer]: partial requesting block %s/%s#", lId.toHex(), consensus->chain().toHex().substr(0, 10)) + std::string(" from ") + key());

		// write
		sendMessageAsync(lMsg);
	}
}

//
// NOTICE: the last and only WORKING proto, external repeatable procedure
//
void Peer::synchronizeLargePartialTree(IConsensusPtr consensus, SynchronizationJobPtr job) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {

		uint256 lId = job->nextBlock();
		if (lId.isNull()) {
			// log
			if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer]: large headers synchronization job is done, root = ") + strprintf("%s, %s#", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));

			if (!consensus->processPartialTreeHeaders(job)) {
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer]: large pending blocks synchronization is not possible, root = ") + strprintf("%s, %s#", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));
			}

			return;
		}

		// register job
		addJob(consensus->chain(), job);

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		lStateStream << consensus->chain();
		lStateStream << lId;
		Message lMessage(Message::GET_BLOCK_HEADER, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, strprintf("[peer]: requesting large block header %s/%s#", lId.toHex(), consensus->chain().toHex().substr(0, 10)) + std::string(" from ") + key());

		// write
		sendMessageAsync(lMsg);
	}
}

//
// NOTICE: the last and only WORKING proto, external repeatable procedure
//
void Peer::synchronizePendingBlocks(IConsensusPtr consensus, SynchronizationJobPtr job, const boost::system::error_code& error) {
	//
	if (error == boost::asio::error::operation_aborted) return;

	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		// register job
		addJob(consensus->chain(), job);

		// try to expand current job to active peers
		consensus->expandJob(job);

		// next job
		uint256 lBlockHeader = job->acquireNextPendingBlockJob(shared_from_this());

		// done?
		if (lBlockHeader.isNull()) {
			//
			if (job->hasPendingBlocks()) {
				// log
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer]: pending blocks synchronization is NOT done, waiting for root = ") + strprintf("%s, %d, %s#", job->block().toHex(), job->pendingBlocks() + job->activeWorkers(), consensus->chain().toHex().substr(0, 10)));
				// cleanup
				removeJob(consensus->chain());
				//
				return;
			} else {
				// log
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer]: pending blocks synchronization job is done, root = ") + strprintf("%s, %s#", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));
				
				// cleanup
				removeJob(consensus->chain());
				// finish
				consensus->finishJob(job);

				// reindex, partial
				if (consensus->doIndex(job->block()/*root block*/, job->lastBlock())) {
					// broadcast new state
					StatePtr lState = peerManager_->consensusManager()->currentState();
					peerManager_->consensusManager()->broadcastState(lState, addressId());
				} else {
					// log
					if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer]: partial reindex FAILED skipping subtree switching, root = ") + strprintf("%s, %s#", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));
					//
					consensus->toNonSynchronized();
				}

				return;
			}
		}

		// make target - last
		if (job->lastBlock().isNull()) job->setLastBlockInstant(job->lastPendingBlock());

		// check if block exists
		while (!lBlockHeader.isNull() && 
					peerManager_->consensusManager()->locate(consensus->chain())->store()->blockExists(lBlockHeader)) {
			//
			job->releasePendingBlockJob(lBlockHeader);
			lBlockHeader = job->acquireNextPendingBlockJob(shared_from_this());
		}

		// check next job, and if job absent - call handler after a little while
		if (lBlockHeader.isNull()) {
			if (job->hasPendingBlocks()) {
				// log
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer/root]: pending blocks synchronization in NOT done, waiting for root = ") + strprintf("%s, %d, %s#", job->block().toHex(), job->pendingBlocks() + job->activeWorkers(), consensus->chain().toHex().substr(0, 10)));
				// cleanup
				removeJob(consensus->chain());
				//
				return;
			} else {
				// log
				if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer/root]: pending blocks synchronization job is done, root = ") + strprintf("%s, %s#", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));
				
				// cleanup
				removeJob(consensus->chain());
				// finish
				consensus->finishJob(job);

				// reindex, partial
				if (consensus->doIndex(job->block()/*root block*/, job->lastBlock())) {
					// broadcast new state
					StatePtr lState = peerManager_->consensusManager()->currentState();
					peerManager_->consensusManager()->broadcastState(lState, addressId());
				} else {
					// log
					if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer/root]: partial reindex FAILED skipping subtree switching, root = ") + strprintf("%s, %s#", job->block().toHex(), consensus->chain().toHex().substr(0, 10)));
					//
					consensus->toNonSynchronized();
				}

				return;
			}
		}

		// new message
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);
		std::list<DataStream>::iterator lMsg = newOutMessage();

		lStateStream << consensus->chain();
		lStateStream << lBlockHeader;
		Message lMessage(Message::GET_BLOCK_DATA, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[peer]: requesting pending BLOCK data ") + strprintf("%s/%s#", lBlockHeader.toHex(), consensus->chain().toHex().substr(0, 10)) + std::string(" from ") + key());

		// write
		sendMessageAsync(lMsg);
	}
}

void Peer::tryAskForQbits() {
	//
	StatePtr lState = peerManager_->consensusManager()->currentState();
	tryAskForQbits(lState->address());
}

void Peer::tryAskForQbits(const PKey& pkey) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);
		lStateStream << pkey;

		Message lMessage(Message::GET_SOME_QBITS, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: requesting some qbits ") + std::string(" from ") + key());

		// write
		sendMessageAsync(lMsg);
	}	
}

void Peer::requestState() {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		//
		StatePtr lState = peerManager_->consensusManager()->currentState();

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);
		lStateStream << lState->addressId();

		Message lMessage(Message::GET_STATE, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: requesting state ") + std::string(" from ") + key());

		// write
		sendMessageAsync(lMsg);
	}
}

void Peer::acquireBlock(const NetworkBlockHeader& block) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lId = const_cast<NetworkBlockHeader&>(block).blockHeader().hash();
		lStateStream << const_cast<NetworkBlockHeader&>(block).blockHeader().chain();
		lStateStream << lId;
		Message lMessage(Message::GET_NETWORK_BLOCK, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: acquiring block ") + lId.toHex() + std::string(" from ") + key());

		// write
		sendMessageAsync(lMsg);
	}
}

void Peer::acquireBlockHeaderWithCoinbase(const uint256& block, const uint256& chain, INetworkBlockHandlerWithCoinBasePtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

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
		sendMessageAsync(lMsg);
	}
}

void Peer::loadTransaction(const uint256& chain, const uint256& tx, ILoadTransactionHandlerPtr handler) {
	loadTransaction(chain, tx, false, handler);
}

void Peer::loadTransaction(const uint256& chain, const uint256& tx, bool tryMempool, ILoadTransactionHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << tx;
		lStateStream << tryMempool;
		Message lMessage(Message::GET_TRANSACTION_DATA, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: loading transaction ") + tx.toHex() + std::string(" from ") + key());

		// write
		sendMessageAsync(lMsg);
	}
}

void Peer::loadTransactions(const uint256& chain, const std::vector<uint256>& txs, ILoadTransactionsHandlerPtr handler) {
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = addRequest(handler);
		lStateStream << lRequestId;
		lStateStream << chain;
		lStateStream << txs;
		Message lMessage(Message::GET_TRANSACTIONS_DATA, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer]: loading transactions %d", txs.size()) + std::string(" from ") + key());

		// write
		sendMessageAsync(lMsg);
	}
}

void Peer::loadEntity(const std::string& name, ILoadEntityHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

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
		sendMessageAsync(lMsg);
	}
}

void Peer::sendTransaction(TransactionContextPtr ctx, ISentTransactionHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; } // TODO: connect will skip current call
	else if (socketStatus_ == CONNECTED) {
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// id
		uint256 lRequestId = addRequest(handler);

		// push tx
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);
		lStateStream << lRequestId;		
		Transaction::Serializer::serialize<DataStream>(lStateStream, ctx->tx());

		Message lMessage(Message::PUSH_TRANSACTION, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) 
			gLog().write(Log::NET, strprintf("[peer]: sending transaction %s to %s", ctx->tx()->id().toHex(), key()));

		// write
		sendMessageAsync(lMsg);
	}
}

void Peer::selectUtxoByAddress(const PKey& source, const uint256& chain, ISelectUtxoByAddressHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

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
		sendMessageAsync(lMsg);
	}	
}

void Peer::selectUtxoByAddressAndAsset(const PKey& source, const uint256& chain, const uint256& asset, ISelectUtxoByAddressAndAssetHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

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
		sendMessageAsync(lMsg);
	}	
}

void Peer::selectUtxoByTransaction(const uint256& chain, const uint256& tx, ISelectUtxoByTransactionHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

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
		sendMessageAsync(lMsg);
	}	
}

void Peer::selectUtxoByEntity(const std::string& name, ISelectUtxoByEntityNameHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

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
		sendMessageAsync(lMsg);
	}
}

void Peer::selectUtxoByEntityNames(const std::vector<std::string>& entityNames, ISelectUtxoByEntityNamesHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		// pack req
		uint256 lRequestId = addRequest(handler);
		lStateStream << lRequestId;

		// pack names
		std::string lNamesString;
		std::vector<std::vector<unsigned char>> lNames; lNames.resize(entityNames.size());
		std::vector<std::string>& lEntityNames = const_cast<std::vector<std::string>&>(entityNames);
		int lIdx = 0;
		for (std::vector<std::string>::iterator lName = lEntityNames.begin(); lName != lEntityNames.end(); lName++, lIdx++) {
			lNames[lIdx].insert(lNames[lIdx].end(), lName->begin(), lName->end());

			if (gLog().isEnabled(Log::NET)) {
				if (lNamesString.size()) lNamesString += ",";
				lNamesString += *lName;
			}
		}
		lStateStream << lNames;

		// make message
		Message lMessage(Message::GET_UTXO_BY_ENTITY_NAMES, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: selecting utxo by entity names '") + lNamesString + std::string("' from ") + key());

		// write
		sendMessageAsync(lMsg);
	}
}

void Peer::selectEntityCountByShards(const std::string& name, ISelectEntityCountByShardsHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

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
		sendMessageAsync(lMsg);
	}
}

void Peer::selectEntityCountByDApp(const std::string& name, ISelectEntityCountByDAppHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = addRequest(handler);
		std::string lName(name);
		entity_name_t lLimitedName(lName);
		lStateStream << lRequestId;
		lStateStream << lLimitedName;
		Message lMessage(Message::GET_ENTITY_COUNT_BY_DAPP, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: selecting entity count by dapp '") + name + std::string("' from ") + key());

		// write
		sendMessageAsync(lMsg);
	}
}

void Peer::selectEntityNames(const std::string& pattern, ISelectEntityNamesHandlerPtr handler) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; }
	else if (socketStatus_ == CONNECTED) {
		
		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);

		uint256 lRequestId = addRequest(handler);
		std::string lName(pattern);
		entity_name_t lLimitedName(lName);
		lStateStream << lRequestId;
		lStateStream << lLimitedName;
		Message lMessage(Message::GET_ENTITY_NAMES, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));

		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: selecting entity names '") + pattern + std::string("' from ") + key());

		// write
		sendMessageAsync(lMsg);
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
		StatePtr lOwnState = peerManager_->consensusManager()->currentState();
		if (!lOwnState->client()) { 
			// active & explicit
			peerManager_->activePeers(lPeers);
		}

		// push own current state
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);
		lStateStream << lPeers;

		// make message
		Message lMessage(Message::GET_PEERS, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: exchanging peers with ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		// write
		sendMessageAsync(lMsg);
	}
}

void Peer::waitForMessage() {
	//
	{
		boost::unique_lock<boost::recursive_mutex> lLock(readMutex_);

		//
		if (waitingForMessage_) {
			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer]: %s ALREADY waiting for message, skip...", key()));
			return;
		}

		if (reading_) {
			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer]: %s ALREADY reading message, skip...", key()));
			return;
		}

		// mark
		waitingForMessage_ = true;
	}

	// log
	if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer]: %s - waiting for message...", key()));

	// new message entry
	std::list<DataStream>::iterator lMsg = newInMessage();

	{
		boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
		if (socketStatus_ == CONNECTED)
			boost::asio::async_read(*socket_,
				boost::asio::buffer(lMsg->data(), Message::size()),
				strand_->wrap(boost::bind(
					&Peer::processMessage, shared_from_this(), lMsg,
					boost::asio::placeholders::error)));
	}
}

void Peer::processMessage(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	{
		boost::unique_lock<boost::recursive_mutex> lLock(readMutex_);
		// unmark
		waitingForMessage_ = false;
		// going to read
		reading_ = true;
	}

	{
		//
		boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);

		//
		if (socketStatus_ != CONNECTED) {
			//
			gLog().write(Log::NET, "[peer/processMessage/error]: session is invalid, cancel reading...");
			//
			socketStatus_ = ERROR;
			//
			reading_ = false;
			//
			return;
		}
	}

	//
	if (!error)	{
		if (!msg->size()) {
			// log
			gLog().write(Log::NET, std::string("[peer/processMessage/error]: empty message from ") + key());
			rawInMessages_.erase(msg);

			processed(); 
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
			std::list<DataStream>::iterator lMsg = newInData(lMessage);
			lMsg->resize(lMessage.dataSize());
			bytesReceived_ += lMessage.dataSize() + Message::size();

			// sanity check
			if (peerManager_->existsBanned(key())) {
				// log
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: peer ") + key() + std::string(" is BANNED."));
				eraseInData(lMsg);
				//
				boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
				//
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

			if (lMessage.type() == Message::STATE) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processState, shared_from_this(), lMsg, true,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::GLOBAL_STATE) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGlobalState, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::PING) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processPing, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::PONG) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processPong, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::PEERS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processPeers, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::BLOCK_HEADER) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processBlockHeader, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::BLOCK_HEADER_AND_STATE) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processBlockHeaderAndState, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::TRANSACTION) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processTransaction, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::GET_BLOCK_BY_HEIGHT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetBlockByHeight, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::GET_BLOCK_BY_ID) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetBlockById, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::GET_NETWORK_BLOCK) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetNetworkBlock, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::GET_NETWORK_BLOCK_HEADER) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetNetworkBlockHeader, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::GET_BLOCK_HEADER) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetBlockHeader, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::GET_BLOCK_DATA) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetBlockData, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::BLOCK_BY_HEIGHT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processBlockByHeight, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::BLOCK_BY_ID) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processBlockById, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::BLOCK) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processBlock, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::NETWORK_BLOCK) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processNetworkBlock, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::NETWORK_BLOCK_HEADER) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processNetworkBlockHeader, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::BLOCK_BY_HEIGHT_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processBlockByHeightAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::BLOCK_BY_ID_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processBlockByIdAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::NETWORK_BLOCK_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processNetworkBlockAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::NETWORK_BLOCK_HEADER_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processNetworkBlockHeaderAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::BLOCK_HEADER_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processBlockHeaderAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::BLOCK_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processBlockAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::GET_PEERS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processRequestPeers, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			// transactions & utxo
			} else if (lMessage.type() == Message::GET_TRANSACTION_DATA) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetTransactionData, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::TRANSACTION_DATA) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processTransactionData, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::TRANSACTION_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processTransactionAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::GET_UTXO_BY_ADDRESS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetUtxoByAddress, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::UTXO_BY_ADDRESS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processUtxoByAddress, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::GET_UTXO_BY_ADDRESS_AND_ASSET) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetUtxoByAddressAndAsset, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::UTXO_BY_ADDRESS_AND_ASSET) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processUtxoByAddressAndAsset, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::GET_UTXO_BY_TX) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetUtxoByTransaction, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::UTXO_BY_TX) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processUtxoByTransaction, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::GET_ENTITY) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetEntity, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::ENTITY) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processEntity, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::ENTITY_IS_ABSENT) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processEntityAbsent, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::GET_UTXO_BY_ENTITY) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetUtxoByEntity, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::GET_ENTITY_COUNT_BY_SHARDS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetEntityCountByShards, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::UTXO_BY_ENTITY) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processUtxoByEntity, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::ENTITY_COUNT_BY_SHARDS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processEntityCountByShards, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::PUSH_TRANSACTION) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processPushTransaction, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::TRANSACTION_PUSHED) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processTransactionPushed, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::GET_UTXO_BY_ENTITY_NAMES) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetUtxoByEntityNames, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::UTXO_BY_ENTITY_NAMES) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processUtxoByEntityNames, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::GET_STATE) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetState, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::GET_ENTITY_COUNT_BY_DAPP) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetEntityCountByDApp, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::ENTITY_COUNT_BY_DAPP) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processEntityCountByDApp, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::GET_TRANSACTIONS_DATA) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetTransactionsData, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::TRANSACTIONS_DATA) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processTransactionsData, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::GET_ENTITY_NAMES) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processGetEntityNames, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
			} else if (lMessage.type() == Message::ENTITY_NAMES) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processEntityNames, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::GET_SOME_QBITS) {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processAskForQbits, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));

			} else if (lMessage.type() == Message::CLIENT_SESSIONS_EXCEEDED) {
				// postpone peer
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/msg]: node sessions exceeded, postponing connections for ") + key());
				peerManager_->postpone(shared_from_this());
				eraseInData(lMsg);
				processed();
			} else if (lMessage.type() == Message::PEER_EXISTS) {
				// mark peer
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/msg]: peer already exists ") + key());				
				// keep as is
				// peerManager_->postpone(shared_from_this());
				eraseInData(lMsg);
				processed();
			} else if (lMessage.type() == Message::PEER_BANNED) {
				// mark peer
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer/msg]: peer banned ") + key());				
				peerManager_->ban(shared_from_this());
				eraseInData(lMsg);
				processed();
			} else {
				boost::asio::async_read(*socket_,
					boost::asio::buffer(lMsg->data(), lMessage.dataSize()),
					strand_->wrap(boost::bind(
						&Peer::processUnknownMessage, shared_from_this(), lMsg,
						boost::asio::placeholders::error)));
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
				{
					boost::unique_lock<boost::recursive_mutex> lLock(readMutex_);
					reading_ = false;
				}

				//
				{
					boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
					//
					socketStatus_ = ERROR;
					// try to deactivate peer
					peerManager_->deactivatePeer(shared_from_this());
					// close socket
					socket_->close();
				}
			}

			return;
		}
		
	} else {
		// log
		if (key() != "EKEY") {
			gLog().write(Log::NET, "[peer/processMessage/error]: closing session " + key() + " -> " + error.message() + ", " + statusString());
			//
			{
				boost::unique_lock<boost::recursive_mutex> lLock(readMutex_);
				reading_ = false;
			}

			//
			{
				boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
				//
				socketStatus_ = ERROR;
				// try to deactivate peer
				peerManager_->deactivatePeer(shared_from_this());
				// close socket
				socket_->close();
			}
		}
	}
}

void Peer::processUnknownMessage(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	eraseInData(msg);
	processed();
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
		bool lTryMempool = false;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lTxId;
		(*msg) >> lTryMempool;
		eraseInData(msg);

		// tx info
		uint256 lBlock; lBlock.setNull();
		uint64_t lConfirms = 0;
		uint64_t lHeight = 0;
		uint32_t lIndex = 0;
		bool lCoinbase = false;

		// try storages
		TransactionPtr lTx = nullptr;
		if (lChain.isNull()) {
			std::vector<ITransactionStorePtr> lStorages = peerManager_->consensusManager()->storeManager()->storages();
			for (std::vector<ITransactionStorePtr>::iterator lStore = lStorages.begin(); lStore != lStorages.end(); lStore++) {
				lTx = (*lStore)->locateTransaction(lTxId);
				if (lTx) { 
					(*lStore)->transactionInfo(lTxId, lBlock, lHeight, lConfirms, lIndex, lCoinbase);
					break;
				}
			}
		} else {
			ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
			lTx = lStorage->locateTransaction(lTxId);
			if (lTx) {
				lStorage->transactionInfo(lTxId, lBlock, lHeight, lConfirms, lIndex, lCoinbase);
			}
		}

		// try mempool
		if (!lTx && lTryMempool) {
			IMemoryPoolManagerPtr lMempoolManager = peerManager_->consensusManager()->mempoolManager();
			std::vector<IMemoryPoolPtr> lMempools = lMempoolManager->pools();
			for (std::vector<IMemoryPoolPtr>::iterator lPool = lMempools.begin(); lPool != lMempools.end(); lPool++) {
				lTx = (*lPool)->locateTransaction(lTxId);
				if (lTx) {
					lTx->setMempool(true);
					break;
				}
			}
		}

		if (lTx != nullptr) {
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// extra info
			lTx->setBlock(lBlock);
			lTx->setHeight(lHeight);
			lTx->setConfirms(lConfirms);
			lTx->setIndex(lIndex);

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lTxId;
			Transaction::Serializer::serialize<DataStream>(lStream, lTx); // tx

			// prepare message
			Message lMessage(Message::TRANSACTION_DATA, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending transacion data for tx = ") + strprintf("%s", lTx->id().toHex()) + std::string(" for ") + key());

			// write
			sendMessage(lMsg);
		} else {
			// tx is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lTxId; // tx

			// prepare message
			Message lMessage(Message::TRANSACTION_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: transaction is absent ") + strprintf("%s", lTxId.toHex()) + std::string(" -> ") + key());

			// write
			sendMessage(lMsg);
		}

		//
		//waitForMessage();
	} else {
		processError("processGetTransactionData", error);
	}
}

void Peer::processGetTransactionsData(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request transacions data from ") + key());

		// extract
		uint256 lRequestId;
		uint256 lChain;
		std::vector<uint256> lTxs;
		(*msg) >> lRequestId;
		(*msg) >> lChain;
		(*msg) >> lTxs;
		eraseInData(msg);

		BlockTransactionsPtr lTransactions = BlockTransactions::instance();
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(lChain);
		if (lStorage) {
			for (std::vector<uint256>::iterator lTx = lTxs.begin(); lTx != lTxs.end(); lTx++) {
				//
				TransactionPtr lTransaction = lStorage->locateTransaction(*lTx);
				lTransactions->append(lTransaction);
			}
		}

		// make message, serialize, send back
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// fill data
		DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
		lStream << lRequestId;
		BlockTransactions::Serializer::serialize<DataStream>(lStream, lTransactions); //

		// prepare message
		Message lMessage(Message::TRANSACTIONS_DATA, lStream.size(), Hash160(lStream.begin(), lStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStream.data(), lStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending transacions data ") + strprintf("%d", lTransactions->transactions().size()) + std::string(" for ") + key());

		// write
		sendMessage(lMsg);
	} else {
		processError("processGetTransactionsData", error);
	}
}

void Peer::processGetState(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request get state from ") + key());

		// extract
		uint160 lAddressId;
		(*msg) >> lAddressId;
		eraseInData(msg);

		//
		sendState();

		//
		processed();		
	} else {
		processError("processGetState", error);
	}
}

void Peer::processAskForQbits(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request get some qbits from ") + key());

		// extract
		PKey lAddress;
		(*msg) >> lAddress;
		eraseInData(msg);

		//
		processed();

		//
		if (peerManager_->settings()->supportAirdrop()) {
			// 1. check if address already has some qbits
			ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
			if (lStorage) {
				if (
#ifdef PRODUCTION_MOD					
					!lStorage->airdropped(lAddress.id(), keyId())
#else
					true
#endif
					) {
					//
					// 2. make airdrop tx
					try {
						TransactionContextPtr lCtx = peerManager_->consensusManager()->wallet()->createTxSpend(
							TxAssetType::qbitAsset(),
							lAddress,
							QBIT/2);

						IMemoryPoolPtr lMempool = peerManager_->memoryPoolManager()->locate(MainChain::id()); // all spend txs - to the main chain
						if (lMempool) {
							//
							if (lMempool->pushTransaction(lCtx)) {
								// check for errors
								if (lCtx->errors().size()) {
									// rollback transaction
									peerManager_->consensusManager()->wallet()->rollback(lCtx);
									// error
									if (gLog().isEnabled(Log::ERROR)) 
										gLog().write(Log::ERROR, strprintf("[peer]: airdrop tx creation failed: E_TX_MEMORYPOOL - %s", *lCtx->errors().begin()));
									//
									lCtx = nullptr;
								} else if (!lMempool->consensus()->broadcastTransaction(lCtx, peerManager_->consensusManager()->wallet()->firstKey()->createPKey().id())) {
									if (gLog().isEnabled(Log::WARNING)) 
										gLog().write(Log::WARNING, strprintf("[peer]: airdrop tx broadcast failed - %s", lCtx->tx()->id().toHex()));
								}

								// we good
								if (lCtx) {
									// find and broadcast for active clients
									peerManager_->notifyTransaction(lCtx);
									// mark
									lStorage->pushAirdropped(lAddress.id(), keyId(), lCtx->tx()->id());
								}
							} else {
								if (gLog().isEnabled(Log::ERROR)) 
									gLog().write(Log::ERROR, std::string("[peer]: airdrop tx creation failed: E_TX_EXISTS - Transaction already exists"));
								// rollback transaction
								peerManager_->consensusManager()->wallet()->rollback(lCtx);
							}
						}
					}
					catch(qbit::exception& ex) {
						if (gLog().isEnabled(Log::ERROR)) 
							gLog().write(Log::ERROR, strprintf("[peer]: airdrop tx creation failed: %s - %s", ex.code(), ex.what()));
					}
				} else {
					if (gLog().isEnabled(Log::ERROR)) 
						gLog().write(Log::ERROR, strprintf("[peer]: already aidropped for: %s - %s", lAddress.id().toHex(), keyId().toHex()));
				}
			}

		} else {
			// WARNING: in case of broadcasting we can't control peer, just address; that is why this technique is not safe
			//StatePtr lState = peerManager_->consensusManager()->currentState();
			//peerManager_->consensusManager()->broadcastAirdropRequest(lAddress, lState->addressId());
		}
		
	} else {
		processError("processAskForQbits", error);
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

		if (!lTx) {
			// try mempool
			IMemoryPoolPtr lMempool = peerManager_->consensusManager()->mempoolManager()->locate(MainChain::id());
			lTx = lMempool->locateEntity(lName);
		}

		if (lTx != nullptr) {
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			Transaction::Serializer::serialize<DataStream>(lStream, lTx); // tx

			// prepare message
			Message lMessage(Message::ENTITY, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending entity ") + strprintf("'%s'/%s", lName, lTx->id().toHex()) + std::string(" for ") + key());

			// write
			sendMessage(lMsg);
		} else {
			// tx is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lLimitedName; // tx

			// prepare message
			Message lMessage(Message::ENTITY_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: entity is absent ") + strprintf("'%s'", lName) + std::string(" -> ") + key());

			// write
			sendMessage(lMsg);
		}

		//
		//waitForMessage();
	} else {
		processError("processGetEntity", error);
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
		DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
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
		sendMessage(lMsg);
	} else {
		processError("processGetUtxoByEntity", error);
	}
}

void Peer::processGetUtxoByEntityNames(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request utxo by entity names from ") + key());

		// extract
		uint256 lRequestId;
		std::vector<std::vector<unsigned char>> lNames;

		(*msg) >> lRequestId;
		(*msg) >> lNames;
		eraseInData(msg);

		// collect
		std::string lEntityNames;
		std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo> lEntityUtxos;
		for (std::vector<std::vector<unsigned char>>::iterator lName = lNames.begin(); lName != lNames.end(); lName++) {
			//
			std::string lEntity;
			lEntity.insert(lEntity.end(), lName->begin(), lName->end());

			if (gLog().isEnabled(Log::NET)) {
				if (lEntityNames.size()) lEntityNames += ",";
				lEntityNames += lEntity;
			}

			std::vector<Transaction::UnlinkedOutPtr> lUtxos;
			ISelectUtxoByEntityNamesHandler::EntityUtxo lEntityUtxo(lEntity);

			ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
			lStorage->entityStore()->collectUtxoByEntityName(lEntity, lUtxos);
			for (std::vector<Transaction::UnlinkedOutPtr>::iterator lItem = lUtxos.begin(); lItem != lUtxos.end(); lItem++) {
				lEntityUtxo.add(*(*lItem));
			}
		}

		// make message, serialize, send back
		std::list<DataStream>::iterator lMsg = newOutMessage();
		// fill data
		DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
		lStream << lRequestId;
		lStream << lEntityUtxos;

		// prepare message
		Message lMessage(Message::UTXO_BY_ENTITY_NAMES, lStream.size(), Hash160(lStream.begin(), lStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStream.data(), lStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending utxo by entity names ") + strprintf("'%s'/%d", lEntityNames, lEntityUtxos.size()) + std::string(" for ") + key());

		// write
		sendMessage(lMsg);
	} else {
		processError("processGetUtxoByEntityNames", error);
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
		DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
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
		sendMessage(lMsg);
	} else {
		processError("processGetEntityCountByShards", error);
	}
}

void Peer::processGetEntityCountByDApp(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request entity count by dapp from ") + key());

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
		DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
		lStream << lRequestId;
		lStream << lLimitedName;

		std::map<uint256, uint32_t> lShardInfo;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		//
		std::vector<ISelectEntityCountByDAppHandler::EntitiesCount> lEntitiesCount;
		if (lStorage->entityStore()->entityCountByDApp(lName, lShardInfo)) {
			for (std::map<uint256, uint32_t>::iterator lItem = lShardInfo.begin(); lItem != lShardInfo.end(); lItem++) {
				//
				lEntitiesCount.push_back(ISelectEntityCountByDAppHandler::EntitiesCount(lItem->first, lItem->second));
			}
		}
		lStream << lEntitiesCount;

		// prepare message
		Message lMessage(Message::ENTITY_COUNT_BY_DAPP, lStream.size(), Hash160(lStream.begin(), lStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStream.data(), lStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending entity count by dapp ") + strprintf("'%s'/%d", lName, lShardInfo.size()) + std::string(" for ") + key());

		// write
		sendMessage(lMsg);
	} else {
		processError("processGetEntityCountByDApp", error);
	}
}

void Peer::processGetEntityNames(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing request entity names ") + key());

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
		DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
		lStream << lRequestId;
		lStream << lLimitedName;

		std::map<uint256, uint32_t> lShardInfo;
		ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(MainChain::id());
		//
		std::vector<IEntityStore::EntityName> lNames;
		lStorage->entityStore()->selectEntityNames(lName, lNames);
		lStream << lNames;

		// prepare message
		Message lMessage(Message::ENTITY_NAMES, lStream.size(), Hash160(lStream.begin(), lStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStream.data(), lStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending entity names ") + strprintf("'%s'/%d", lName, lNames.size()) + std::string(" for ") + key());

		// write
		sendMessage(lMsg);
	} else {
		processError("processGetEntityNames", error);
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

		// go to read
		processed();

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadTransactionHandler>(lHandler)->handleReply(nullptr);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND, ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: transaction is absent ") + strprintf("%s#", lTxId.toHex()));

	} else {
		processError("processTransactionAbsent", error);
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
		uint256 lTxId;
		TransactionPtr lTx;

		(*msg) >> lRequestId;
		(*msg) >> lTxId;
		lTx = Transaction::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing transaction ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));

		// sanity
		if (lTx->id() != lTxId) lTx = nullptr;

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadTransactionHandler>(lHandler)->handleReply(lTx);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for transaction ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));
		}
	} else {
		processError("processTransactionData", error);
	}
}

void Peer::processTransactionsData(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response transactions data from ") + key());

		// extract
		uint256 lRequestId;
		BlockTransactionsPtr lTransactions;

		(*msg) >> lRequestId;
		lTransactions = BlockTransactions::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing transactions ") + strprintf("r = %s, s = %s", lRequestId.toHex(), lTransactions->transactions().size()) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadTransactionsHandler>(lHandler)->handleReply(lTransactions->transactions());
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		processError("processTransactionsData", error);
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

		// go to read
		processed();

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadEntityHandler>(lHandler)->handleReply(nullptr);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND, ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: entity is absent - ") + strprintf("'%s'", lName));
	} else {
		processError("processTransactionAbsent", error);
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
		eraseInData(msg);

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing entity ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ILoadEntityHandler>(lHandler)->handleReply(TransactionHelper::to<Entity>(lTx));
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for entity ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx->id().toHex()) + std::string("..."));
		}
	} else {
		processError("processTransactionData", error);
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
		DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
		lStream << lRequestId;
		lStream << lPKey;
		lStream << lChain;
		lStream << lOuts;

		// prepare message
		Message lMessage(Message::UTXO_BY_ADDRESS, lStream.size(), Hash160(lStream.begin(), lStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStream.data(), lStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending utxo by address ") + strprintf("count = %d, address = %s/%s#", lOuts.size(), lPKey.toString(), lChain.toHex().substr(0, 10)) + std::string(" for ") + key());

		// write
		sendMessage(lMsg);
	} else {
		processError("processGetUtxoByAddress", error);
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
		DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
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
		sendMessage(lMsg);
	} else {
		processError("processGetUtxoByAddressAndAsset", error);
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
		DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
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
		sendMessage(lMsg);
	} else {
		processError("processGetUtxoByTransaction", error);
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
		eraseInData(msg);

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing utxo by address ") + strprintf("r = %s, %d/%s", lRequestId.toHex(), lOuts.size(), lPKey.toString()) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectUtxoByAddressHandler>(lHandler)->handleReply(lOuts, lPKey);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for utxo by address ") + strprintf("r = %s, %s", lRequestId.toHex(), lPKey.toString()) + std::string("..."));
		}
	} else {
		processError("processUtxoByAddress", error);
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
		eraseInData(msg);

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing utxo by address and asset ") + strprintf("r = %s, %d/%s/%s", lRequestId.toHex(), lOuts.size(), lPKey.toString(), lAsset.toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectUtxoByAddressAndAssetHandler>(lHandler)->handleReply(lOuts, lPKey, lAsset);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for utxo by asset ") + strprintf("r = %s, %s", lRequestId.toHex(), lAsset.toHex()) + std::string("..."));
		}
	} else {
		processError("processUtxoByAddressAndAsset", error);
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
		eraseInData(msg);

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing utxo by transaction ") + strprintf("r = %s, %d/%s", lRequestId.toHex(), lOuts.size(), lTx.toHex()) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectUtxoByTransactionHandler>(lHandler)->handleReply(lOuts, lTx);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for utxo by tx ") + strprintf("r = %s, %s", lRequestId.toHex(), lTx.toHex()) + std::string("..."));
		}
	} else {
		processError("processUtxoByTransaction", error);
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
		eraseInData(msg);

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing utxo by entity ") + strprintf("r = %s, s = %d, e = '%s'", lRequestId.toHex(), lOuts.size(), lName) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectUtxoByEntityNameHandler>(lHandler)->handleReply(lOuts, lName);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for utxo by entity ") + strprintf("r = %s, e = '%s'", lRequestId.toHex(), lName) + std::string("..."));
		}
	} else {
		processError("processUtxoByEntity", error);
	}
}

void Peer::processUtxoByEntityNames(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response utxo by entity names from ") + key());

		// extract
		uint256 lRequestId;
		std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo> lEntityUtxos;

		(*msg) >> lRequestId;
		(*msg) >> lEntityUtxos;
		eraseInData(msg);

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing utxo by entity names ") + strprintf("r = %s, s = %d", lRequestId.toHex(), lEntityUtxos.size()) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectUtxoByEntityNamesHandler>(lHandler)->handleReply(lEntityUtxos);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for utxo by entity ") + strprintf("r = %s", lRequestId.toHex()) + std::string("..."));
		}
	} else {
		processError("processUtxoByEntityNames", error);
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

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing entity count by shards ") + strprintf("r = %s, s = %d, e = '%s'", lRequestId.toHex(), lShardInfo.size(), lName) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectEntityCountByShardsHandler>(lHandler)->handleReply(lShardInfo, lName);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for entity count by shards ") + strprintf("r = %s, e = '%s'", lRequestId.toHex(), lName) + std::string("..."));
		}
	} else {
		processError("processEntityCountByShards", error);
	}
}

void Peer::processEntityCountByDApp(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response entity count by dapp from ") + key());

		// extract
		uint256 lRequestId;
		std::string lName;
		entity_name_t lLimitedName(lName);
		std::vector<ISelectEntityCountByDAppHandler::EntitiesCount> lEntitiesCount;

		(*msg) >> lRequestId;
		(*msg) >> lLimitedName;
		(*msg) >> lEntitiesCount;
		eraseInData(msg);

		std::map<uint256, uint32_t> lShardInfo;
		for (std::vector<ISelectEntityCountByDAppHandler::EntitiesCount>::iterator lItem = lEntitiesCount.begin(); lItem != lEntitiesCount.end(); lItem++) {
			//
			lShardInfo.insert(std::map<uint256, uint32_t>::value_type((*lItem).shard(), (*lItem).count()));
		}

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing entity count by dapp ") + strprintf("r = %s, s = %d, e = '%s'", lRequestId.toHex(), lShardInfo.size(), lName) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectEntityCountByDAppHandler>(lHandler)->handleReply(lShardInfo, lName);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for entity count by shards ") + strprintf("r = %s, e = '%s'", lRequestId.toHex(), lName) + std::string("..."));
		}
	} else {
		processError("processEntityCountByDApp", error);
	}
}

void Peer::processEntityNames(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing response entity names from ") + key());

		// extract
		uint256 lRequestId;
		std::string lName;
		entity_name_t lLimitedName(lName);
		std::vector<IEntityStore::EntityName> lNames;

		(*msg) >> lRequestId;
		(*msg) >> lLimitedName;
		(*msg) >> lNames;
		eraseInData(msg);

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing entity names ") + strprintf("r = %s, s = %d, e = '%s'", lRequestId.toHex(), lNames.size(), lName) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISelectEntityNamesHandler>(lHandler)->handleReply(lName, lNames);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for entity names ") + strprintf("r = %s, e = '%s'", lRequestId.toHex(), lName) + std::string("..."));
		}
	} else {
		processError("processEntityNames", error);
	}
}

void Peer::processTransactionPushed(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing transaction pushed from ") + key());

		// extract
		uint256 lRequestId;
		uint256 lTxId;
		std::vector<TransactionContext::Error> lErrors;

		(*msg) >> lRequestId;
		(*msg) >> lTxId;
		(*msg) >> lErrors;
		eraseInData(msg);

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing transaction pushed ") + strprintf("r = %s, tx = %s, e = %d", lRequestId.toHex(), lTxId.toHex(), lErrors.size()) + std::string("..."));

		// go to read
		processed();

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<ISentTransactionHandler>(lHandler)->handleReply(lTxId, lErrors);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND ") + strprintf("r = %s, tx = %s", lRequestId.toHex(), lTxId.toHex()) + std::string("..."));
		}
	} else {
		processError("processTransactionPushed", error);
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

		//
		processed();
	} else {
		processError("processBlockByHeightAbsent", error);
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

		//
		processed();
	} else {
		processError("processBlockByIdAbsent", error);
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

		//
		processed();
	} else {
		processError("processNetworkBlockAbsent", error);
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

		//
		processed();

		// clean-up
		removeRequest(lRequestId);
		
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: network block header is absent for ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)));
	} else {
		processError("processNetworkBlockHeaderAbsent", error);
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

		//
		processed();
	} else {
		processError("processBlockHeaderAbsent", error);
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

		//
		processed();
	} else {
		processError("processBlockAbsent", error);
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

		//
		processed();

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

	} else {
		processError("processBlockByHeight", error);
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
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lHeight;
			Block::Serializer::serialize<DataStream>(lStream, lBlock); // block

			// prepare message
			Message lMessage(Message::BLOCK_BY_HEIGHT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending block by height ") + strprintf("%s/%d", lBlock->hash().toHex(), lHeight) + std::string(" for ") + key());

			// write
			sendMessage(lMsg);
		} else {
			// block is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lChain; // chain
			lStream << lHeight; // height

			// prepare message
			Message lMessage(Message::BLOCK_BY_HEIGHT_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block is absent for chain ") + strprintf("%d/%s#", lHeight, lChain.toHex().substr(0, 10)) + std::string(" -> ") + key());

			// write
			sendMessage(lMsg);
		}

	} else {
		processError("processGetBlockByHeight", error);
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

		processed();

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

	} else {
		processError("processBlockById", error);
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

		//
		processed();

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

	} else {
		processError("processBlock", error);
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
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			Block::Serializer::serialize<DataStream>(lStream, lBlock); // block

			// prepare message
			Message lMessage(Message::BLOCK_BY_ID, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending block by id ") + strprintf("%s/%s#", lBlock->hash().toHex(), lChain.toHex().substr(0, 10)) + std::string(" for ") + key());

			// write
			sendMessage(lMsg);
		} else {
			// block is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lChain; // chain
			lStream << lId; // id

			// prepare message
			Message lMessage(Message::BLOCK_BY_ID_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block is absent for chain ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)) + std::string(" -> ") + key());

			// write
			sendMessage(lMsg);
		}
	} else {
		processError("processGetBlockById", error);
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
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			Block::Serializer::serialize<DataStream>(lStream, lBlock); // block

			// prepare message
			Message lMessage(Message::BLOCK, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending block data ") + strprintf("%s/%s#", lBlock->hash().toHex(), lChain.toHex().substr(0, 10)) + std::string(" for ") + key());

			// write
			sendMessage(lMsg);
		} else {
			// block is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lChain; // chain
			lStream << lId; // id

			// prepare message
			Message lMessage(Message::BLOCK_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block data is absent for chain ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)) + std::string(" -> ") + key());

			// write
			sendMessage(lMsg);
		}
	} else {
		processError("processGetBlockData", error);
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

		//
		processed();

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

	} else {
		processError("processNetworkBlock", error);
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
		eraseInData(msg);

		// go to read
		processed();

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing network block header ") + strprintf("r = %s, %s/%s#", lRequestId.toHex(), lNetworkBlockHeader.blockHeader().hash().toHex(), lNetworkBlockHeader.blockHeader().chain().toHex().substr(0, 10)) + std::string("..."));

		IReplyHandlerPtr lHandler = locateRequest(lRequestId);
		if (lHandler) {
			ReplyHelper::to<INetworkBlockHandlerWithCoinBase>(lHandler)->handleReply(lNetworkBlockHeader, lBase);
			removeRequest(lRequestId);
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: request was NOT FOUND for network block header ") + strprintf("r = %s, %s/%s#", lRequestId.toHex(), lNetworkBlockHeader.blockHeader().hash().toHex(), lNetworkBlockHeader.blockHeader().chain().toHex().substr(0, 10)) + std::string("..."));
		}
	} else {
		processError("processNetworkBlockHeader", error);
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
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			Block::Serializer::serialize<DataStream>(lStream, lBlock); // block

			// prepare message
			Message lMessage(Message::NETWORK_BLOCK, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending network block ") + strprintf("%s/%s#", lBlock->hash().toHex(), lChain.toHex().substr(0, 10)) + std::string(" for ") + key());

			// write
			sendMessage(lMsg);
		} else {
			// block is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lChain; // chain
			lStream << lId; // id

			// prepare message
			Message lMessage(Message::NETWORK_BLOCK_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: network block is absent for chain ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)) + std::string(" -> ") + key());

			// write
			sendMessage(lMsg);
		}
	} else {
		processError("processGetNetworkBlock", error);
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
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
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
			sendMessage(lMsg);
		} else {
			// block is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
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
			sendMessage(lMsg);
		}
	} else {
		processError("processGetNetworkBlockHeader", error);
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
			} while(
				peerManager_->consensusManager()->locate(lChain)->store()->blockHeaderHeight(lCurrent, lHeight, lHeader) && 
				(++lCount) < 500 // NOTICE: header size is around 300b, so 500 headers per request ~150k
			);

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lHeaders;

			// prepare message
			Message lMessage(Message::BLOCK_HEADER, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending block header(s) ") + strprintf("%d/%s/%s#", lHeaders.size(), lId.toHex(), lChain.toHex().substr(0, 10)) + std::string(" for ") + key());

			// write
			sendMessage(lMsg);
		} else {
			// block is absent
			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lChain; // chain
			lStream << lId; // id

			// prepare message
			Message lMessage(Message::BLOCK_HEADER_IS_ABSENT, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block header is absent for chain ") + strprintf("%s/%s#", lId.toHex(), lChain.toHex().substr(0, 10)) + std::string(" -> ") + key());

			// write
			sendMessage(lMsg);
		}
	} else {
		processError("processGetBlockHeader", error);
	}	
}

void Peer::processBlockHeader(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, strprintf("[peer]: checksum %s/%d is INVALID for message from %s", (*msg).calculateCheckSum().toHex(), (*msg).size(), key()));
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: raw block header(s) from ") + key() + " -> " + HexStr(msg->begin(), msg->end()).substr(0, 100) + "#");

		// extract block header data
		std::vector<NetworkBlockHeader> lHeaders;
		(*msg) >> lHeaders;
		eraseInData(msg);

		if (!lHeaders.size()) { 
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: block header(s) is EMPTY from ") + key() + " -> " + HexStr(msg->begin(), msg->end()));
			processed();
			return;
		}

		//
		processed();

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
		}
	} else {
		processError("processBlockHeader", error);
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

		//
		processed();

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing block header from ") + key() + " -> " + 
				strprintf("%d/%s/%s#", lNetworkBlockHeader.height(), lNetworkBlockHeader.blockHeader().hash().toHex(), lNetworkBlockHeader.blockHeader().chain().toHex().substr(0, 10)));

		if (lNetworkBlockHeader.blockHeader().origin() == peerManager_->consensusManager()->mainPKey().id()) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, "[peer]: skip re-processing block header " + 
				strprintf("%s/%s/%s#", lNetworkBlockHeader.blockHeader().hash().toHex(), lNetworkBlockHeader.height(), lNetworkBlockHeader.blockHeader().chain().toHex().substr(0, 10)));
			
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing state from ") + key() + " -> " + lState.toString());
			
			StatePtr lStatePtr = State::instance(lState);
			IPeer::UpdatePeerResult lPeerResult;
			if (!peerManager_->updatePeerState(shared_from_this(), lStatePtr, lPeerResult)) {
				setState(lStatePtr);
				peerManager_->consensusManager()->pushPeer(shared_from_this());
			}
			peerManager_->consensusManager()->pushState(lStatePtr);
		} else {
			//
			IValidator::BlockCheckResult lResult = peerManager_->consensusManager()->pushBlockHeader(lNetworkBlockHeader);
			switch(lResult) {
				case IValidator::SUCCESS: {
					if (!peerManager_->settings()->isMiner()) {
						StatePtr lLocalState = peerManager_->consensusManager()->currentState();
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: notifying state changes ") + key() + " -> " + lLocalState->toString());
						peerManager_->consensusManager()->broadcastState(lLocalState, peerManager_->consensusManager()->mainPKey().id());
					}	
				}
				case IValidator::BROKEN_CHAIN: {
						// process state
						if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing state from ") + key() + " -> " + lState.toString());
						StatePtr lStatePtr = State::instance(lState);
						IPeer::UpdatePeerResult lPeerResult;
						if (!peerManager_->updatePeerState(shared_from_this(), lStatePtr, lPeerResult)) {
							setState(lStatePtr);
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

	} else {
		processError("processBlockHeaderAndState", error);
	}
}

void Peer::processTransaction(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: raw transaction from ") + key());

		// extract transaction data
		TransactionPtr lTx = Transaction::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		//
		processed();	

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

	} else {
		processError("processTransaction", error);
	}
}

void Peer::processPushTransaction(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	bool lMsgValid = (*msg).valid();
	if (!lMsgValid) if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: checksum is INVALID for message from ") + key());
	if (!error && lMsgValid) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: raw send transaction from ") + key());

		uint256 lRequestId;
		(*msg) >> lRequestId;

		// extract transaction data
		TransactionPtr lTx = Transaction::Deserializer::deserialize<DataStream>(*msg);
		eraseInData(msg);

		if (!lTx) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: raw send transaction from ") + key() + " is INVALID. Probably tx type is not known.");
			processed();
			return;
		}

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: process send transaction from ") + key() + " -> " + 
				strprintf("%s/%s#", lTx->id().toHex(), lTx->chain().toHex().substr(0, 10)));

		if (peerManager_->settings()->isClient()) {
			// client-side
			peerManager_->consensusManager()->processTransaction(lTx);
			processed();
		} else {
			// node-side
			std::vector<TransactionContext::Error> lErrors;
			// ge mempool
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

					lCtx->packErrors(lErrors);
				} else {
					// broadcast by nodes and fullnodes
					peerManager_->consensusManager()->broadcastTransaction(lCtx, addressId());

					// find and broadcast for active clients
					peerManager_->notifyTransaction(lCtx);
				}

			} else {
				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: chain is NOT SUPPORTED for ") + 
						strprintf("%s/%s#", lTx->id().toHex(), lTx->chain().toHex().substr(0, 10)));
				lErrors.push_back(TransactionContext::Error("chain is NOT SUPPORTED"));
			}

			// make message, serialize, send back
			std::list<DataStream>::iterator lMsg = newOutMessage();

			// fill data
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			lStream << lRequestId;
			lStream << lTx->id();
			lStream << lErrors;

			// prepare message
			Message lMessage(Message::TRANSACTION_PUSHED, lStream.size(), Hash160(lStream.begin(), lStream.end()));
			(*lMsg) << lMessage;
			lMsg->write(lStream.data(), lStream.size());

			// log
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending transaction pushed result: ") + strprintf("errors = %d, tx = %s", lErrors.size(), lTx->id().toHex()) + std::string(" for ") + key());

			// write
			sendMessage(lMsg);
		}
	} else {
		processError("processPushTransaction", error);
	}
}

void Peer::broadcastBlockHeader(const NetworkBlockHeader& blockHeader) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; } // TODO: connect will skip current call
	else if (socketStatus_ == CONNECTED) {

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// push blockheader
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);
		lStateStream << blockHeader;

		Message lMessage(Message::BLOCK_HEADER, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: broadcasting block header to ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		// write
		sendMessageAsync(lMsg);
	}
}

void Peer::broadcastBlockHeaderAndState(const NetworkBlockHeader& blockHeader, StatePtr state) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; } // TODO: connect will skip current call
	else if (socketStatus_ == CONNECTED) {

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// push blockheader
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);
		lStateStream << blockHeader;

		if (state->addressId() == peerManager_->consensusManager()->mainPKey().id()) {
			// own state
			state->serialize<DataStream>(lStateStream, *(peerManager_->consensusManager()->mainKey()));
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
		sendMessageAsync(lMsg);
	}
}

void Peer::broadcastTransaction(TransactionContextPtr ctx) {
	//
	if (socketStatus_ == CLOSED || socketStatus_ == ERROR) { connect(); return; } // TODO: connect will skip current call
	else if (socketStatus_ == CONNECTED) {
		//
		if (peerManager_->consensusManager()->storeManager()) {
			// get tx info
			uint64_t lConfirms = 0;
			uint64_t lHeight = 0;
			bool lCoinbase = false;
			//
			ITransactionStorePtr lStorage = peerManager_->consensusManager()->storeManager()->locate(ctx->tx()->chain());
			if (lStorage) {
				lStorage->transactionHeight(ctx->tx()->id(), lHeight, lConfirms, lCoinbase);
				//
				// NOTICE: memory & network only
				//
				ctx->tx()->setConfirms(lConfirms);
			}
		}

		// new message
		std::list<DataStream>::iterator lMsg = newOutMessage();

		// push tx
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);
		Transaction::Serializer::serialize<DataStream>(lStateStream, ctx->tx());

		Message lMessage(Message::TRANSACTION, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: broadcasting transaction to ") + key());

		// write
		sendMessageAsync(lMsg);
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
		DataStream lStateStream(SER_NETWORK, PROTOCOL_VERSION);
		lStateStream << lPeers;

		// make message
		Message lMessage(Message::PEERS, lStateStream.size(), Hash160(lStateStream.begin(), lStateStream.end()));
		(*lMsg) << lMessage;
		lMsg->write(lStateStream.data(), lStateStream.size());

		// log
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: sending peers list to ") + key() + " -> " + HexStr(lMsg->begin(), lMsg->end()));

		// write
		sendMessage(lMsg);
	} else {
		processError("processRequestPeers", error);
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

		//
		processed();

		for (std::vector<std::string>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: trying to add proposed peer - ") + (*lPeer));
			peerManager_->addPeer(*lPeer);	
		}

	} else {
		processError("processPeers", error);
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

		sendMessage(lMsg);
	} else {
		processError("processPing", error);
	}
}

void Peer::processPong(std::list<DataStream>::iterator msg, const boost::system::error_code& error) {
	//
	if (!error) {
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: pong from ") + key());

		uint64_t lTimestamp;
		(*msg) >> lTimestamp;
		eraseInData(msg);

		//
		processed();

		time_ = lTimestamp;
		timestamp_ = getMicroseconds();

		// update and median time
		peerManager_->updatePeerLatency(shared_from_this(), (uint32_t)(getMicroseconds() - lTimestamp));
		if (!state()->client()) peerManager_->updateMedianTime();

	} else {
		processError("processPong", error);
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

		//
		processed();

		StatePtr lOwnState = peerManager_->consensusManager()->currentState();
		if (lOwnState->address().id() == lState.address().id()) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, "[peer/processGlobalState]: skip re-broadcasting state for " + 
				strprintf("%s", lOwnState->address().toHex()));
		} else {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing global state from ") + key() + " -> " + lState.toString());

			if (lState.valid()) {
				StatePtr lStatePtr = State::instance(lState);
				peerManager_->consensusManager()->pushState(lStatePtr);
				setState(lStatePtr);

				/*
				if (peerManager_->settings()->isClient()) {
					// update state
					setState(lState);
				}

				if (lState.client()) {
					// update state
					setState(lState);
				}
				*/
			} else {
				peerManager_->ban(shared_from_this());
			}
		}

	} else {
		processError("processGlobalState", error);
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
		eraseInData(msg); // 

		processed();		

		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: processing state from ") + key() + " -> " + lState.toString());

		IPeer::UpdatePeerResult lPeerResult;
		if (!peerManager_->updatePeerState(shared_from_this(), State::instance(lState), lPeerResult)) {
			// if peer aleady exists
			if (lPeerResult == IPeer::EXISTS) {
				Message lMessage(Message::PEER_EXISTS, sizeof(uint64_t), uint160());
				std::list<DataStream>::iterator lMsg = newOutMessage();

				(*lMsg) << lMessage;

				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: peer already exists ") + key());

				sendMessage(lMsg);
			} else if (lPeerResult == IPeer::BAN) {
				Message lMessage(Message::PEER_BANNED, sizeof(uint64_t), uint160());
				std::list<DataStream>::iterator lMsg = newOutMessage();

				(*lMsg) << lMessage;

				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: peer banned ") + key());

				sendMessage(lMsg);
			} else if (lPeerResult == IPeer::SESSIONS_EXCEEDED) {
				Message lMessage(Message::CLIENT_SESSIONS_EXCEEDED, sizeof(uint64_t), uint160());
				std::list<DataStream>::iterator lMsg = newOutMessage();

				(*lMsg) << lMessage;

				if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: node overloaded ") + key());

				sendMessage(lMsg);
			} else {
				// log
				gLog().write(Log::NET, "[peer]: closing session " + key() + " -> " + error.message());
				//
				{
					boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
					//
					socketStatus_ = ERROR;
					// try to deactivate peer
					peerManager_->deactivatePeer(shared_from_this());
					// close socket
					socket_->close();
				}
			}
		} else if (broadcast) {
			// send state only for inbound peers
			if (!isOutbound()) sendState();
		}
	} else {
		processError("processState", error);
	}
}

void Peer::connect() {
	//
	if (socketType_ == SERVER) { 
		if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: server endpoint - connect() not allowed"));
		return;
	}

	//
	{
		boost::unique_lock<boost::recursive_mutex> lLock(socketMutex_);
		if (socketType_ == CLIENT && (socketStatus_ == CLOSED || socketStatus_ == ERROR) && peerManager_) {
			if (gLog().isEnabled(Log::NET)) gLog().write(Log::NET, std::string("[peer]: connecting ") + key());

			// change status
			socketStatus_ = CONNECTING;

			// make socket
			socket_.reset(new boost::asio::ip::tcp::socket(peerManager_->getContext(contextId_)));
			strand_.reset(new boost::asio::io_service::strand(peerManager_->getContext(contextId_)));

			std::vector<std::string> lParts;
			boost::split(lParts, endpoint_, boost::is_any_of(":"), boost::token_compress_on);

			if (lParts.size() >= 2) {
				resolver_->async_resolve(tcp::resolver::query(lParts[0], lParts[1]),
					strand_->wrap(boost::bind(&Peer::resolved, shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::iterator)));			
			}
		}
	}
}

void Peer::resolved(const boost::system::error_code& error, tcp::resolver::iterator endpoint_iterator) {
	//
	if (!error && socket_ != nullptr) {
		tcp::endpoint lEndpoint = *endpoint_iterator;
		socket_->async_connect(lEndpoint,
			strand_->wrap(boost::bind(&Peer::connected, shared_from_this(),
				boost::asio::placeholders::error, ++endpoint_iterator)));
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

		// activate dapp extension
		if (peerManager_->settings()->isClient()) {
			for (std::vector<State::DAppInstance>::const_iterator 
					lInstance = peerManager_->consensusManager()->dApps().begin();
					lInstance != peerManager_->consensusManager()->dApps().end(); lInstance++) {
				//
				PeerExtensionCreatorPtr lCreator = peerManager_->locateExtensionCreator(lInstance->name());
				if (lCreator) {
					setExtension(lInstance->name(), lCreator->create(*lInstance, shared_from_this(), peerManager_));
				}
			}
		}

		// connected - send our state
		sendState();

		// connected - request peers
		requestPeers();

		// go to read
		processed();
	} else if (endpoint_iterator != tcp::resolver::iterator()) {
		socket_->close();
		
		tcp::endpoint lEndpoint = *endpoint_iterator;
		socket_->async_connect(lEndpoint,
			strand_->wrap(boost::bind(&Peer::connected, this,
				boost::asio::placeholders::error, ++endpoint_iterator)));
	} else {
		gLog().write(Log::NET, std::string("[peer/connect/error]: connection failed for ") + key() + " -> " + error.message());
		socketStatus_ = ERROR;
		socket_ = nullptr;
	}
}
