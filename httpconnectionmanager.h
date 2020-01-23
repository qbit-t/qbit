// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_HTTP_CONNECIONMANAGER_H
#define QBIT_HTTP_CONNECIONMANAGER_H

#include "httpconnection.h"
#include "isettings.h"
#include "log/log.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/chrono.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace qbit {

class HttpConnectionManager;
typedef std::shared_ptr<HttpConnectionManager> HttpConnectionManagerPtr;

class HttpConnectionManager: public std::enable_shared_from_this<HttpConnectionManager> {
public:
	HttpConnectionManager(ISettingsPtr settings, HttpRequestHandlerPtr requestHandler) : 
		settings_(settings), requestHandler_(requestHandler) {

		// prepare pool contexts
		for (std::size_t lIdx = 0; lIdx < settings_->httpThreadPoolSize(); ++lIdx) {
			IOContextPtr lContext(new boost::asio::io_context());
			contexts_.push_back(lContext);
			work_.push_back(boost::asio::make_work_guard(*lContext));

			connections_[lIdx] = ConnectionsMap();
			contextMutex_.push_back(new boost::mutex());
		}
	}

	inline bool exists(const std::string& endpoint) {
		// traverse
		for (std::map<int, ConnectionsMap>::iterator lChannel = connections_.begin(); lChannel != connections_.end(); lChannel++) {
			boost::unique_lock<boost::mutex> lLock(contextMutex_[lChannel->first]);
			std::map<std::string /*endpoint*/, HttpConnectionPtr>::iterator lPeerIter = lChannel->second.find(endpoint);
			if (lPeerIter != lChannel->second.end()) return true;
		}

		return false;
	}

	static HttpConnectionManagerPtr instance(ISettingsPtr, HttpRequestHandlerPtr);

	void run() {
		// create a pool of threads to run all of the io_contexts
		gLog().write(Log::INFO, std::string("[connectionManager]: starting contexts..."));
		for (std::vector<IOContextPtr>::iterator lCtx = contexts_.begin(); lCtx != contexts_.end(); lCtx++)	{
			boost::shared_ptr<boost::thread> lThread(
				new boost::thread(
					boost::bind(&HttpConnectionManager::processor, shared_from_this(), *lCtx)));
			threads_.push_back(lThread);
		}
	}

	void stop() {
		gLog().write(Log::INFO, std::string("[connectionManager]: stopping contexts..."));
		for (std::vector<IOContextPtr>::iterator lCtx = contexts_.begin(); lCtx != contexts_.end(); lCtx++)
			(*lCtx)->stop();

		wait();
	}

	void wait() {
		if (!waiting_) {
			//
			waiting_ = true;

			// wait for all threads in the pool to exit
			for (std::vector<boost::shared_ptr<boost::thread> >::iterator lThread = threads_.begin(); lThread != threads_.end(); lThread++)
				(*lThread)->join();		
		}
	}

	HttpRequestHandlerPtr requestHandler() { return requestHandler_; }

	boost::asio::io_context& getContext(int id) {
		return *contexts_[id];
	}

	boost::asio::io_context& getContext() {
		return *contexts_[getContextId()];
	}

	int getContextId() {
		// lock peers - use the same mutex as for the peers
		boost::unique_lock<boost::mutex> lLock(nextContextMutex_);

		int lId = nextContext_++;
		
		if (nextContext_ == contexts_.size()) nextContext_ = 0;
		return lId;
	}

private:
	void processor(std::shared_ptr<boost::asio::io_context> ctx) {
		gLog().write(Log::INFO, std::string("[connectionManager]: context run..."));
		while(true) {
			try {
				ctx->run();
				break;
			} 
			catch(boost::system::system_error& ex) {
				gLog().write(Log::ERROR, std::string("[connectionManager]: context error -> ") + ex.what());
			}
		}
		gLog().write(Log::NET, std::string("[connectionManager]: context stop."));
	}


private:
	typedef std::map<std::string /*endpoint*/, HttpConnectionPtr> ConnectionsMap;
	typedef std::set<std::string /*endpoint*/> PeersSet;
	typedef std::shared_ptr<boost::asio::io_context> IOContextPtr;
	typedef std::shared_ptr<boost::asio::steady_timer> TimerPtr;
	typedef boost::asio::executor_work_guard<boost::asio::io_context::executor_type> IOContextWork;

	boost::mutex nextContextMutex_;
	std::map<int, ConnectionsMap> connections_; // connections by context id's

	boost::ptr_vector<boost::mutex> contextMutex_;

	std::vector<IOContextPtr> contexts_;
	std::list<IOContextWork> work_;
	std::vector<boost::shared_ptr<boost::thread> > threads_;
	int nextContext_ = 0;
	bool waiting_ = false;

	ISettingsPtr settings_;
	HttpRequestHandlerPtr requestHandler_;
};

}

#endif