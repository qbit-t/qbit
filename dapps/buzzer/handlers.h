// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_HANDLERS_H
#define QBIT_BUZZER_HANDLERS_H

#include "../isettings.h"
#include "../iwallet.h"
#include "../../client/handlers.h"

#include <boost/function.hpp>

namespace qbit {

//
// load buzzer trust score handler
class ILoadTrustScoreHandler: public IReplyHandler {
public:
	ILoadTrustScoreHandler() {}
	virtual void handleReply(amount_t, amount_t, uint32_t, uint32_t) = 0;
};
typedef std::shared_ptr<ILoadTrustScoreHandler> ILoadTrustScoreHandlerPtr;

//
// load buzzer endorse\mistrust handler
class ILoadEndorseMistrustHandler: public IReplyHandler {
public:
	ILoadEndorseMistrustHandler() {}
	virtual void handleReply(const uint256&) = 0;
};
typedef std::shared_ptr<ILoadEndorseMistrustHandler> ILoadEndorseMistrustHandlerPtr;

// special callbacks
typedef boost::function<void (amount_t, amount_t, uint32_t, uint32_t)> trustScoreLoadedFunction;
typedef boost::function<void (const uint256&)> endorseMistrustLoadedFunction;

// load trust scrore transaction
class LoadTrustScore: public ILoadTrustScoreHandler, public std::enable_shared_from_this<LoadTrustScore> {
public:
	LoadTrustScore(trustScoreLoadedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ILoadTrustScoreHandler
	void handleReply(amount_t endorses, amount_t mistrusts, uint32_t subscriptions, uint32_t followers) {
		 function_(endorses, mistrusts, subscriptions, followers);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ILoadTrustScoreHandlerPtr instance(trustScoreLoadedFunction function, timeoutFunction timeout) { 
		return std::make_shared<LoadTrustScore>(function, timeout); 
	}

private:
	trustScoreLoadedFunction function_;
	timeoutFunction timeout_;
};

// load endorse tx
class LoadEndorse: public ILoadEndorseMistrustHandler, public std::enable_shared_from_this<LoadEndorse> {
public:
	LoadEndorse(endorseMistrustLoadedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ILoadTrustScoreHandler
	void handleReply(const uint256& tx) {
		 function_(tx);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ILoadEndorseMistrustHandlerPtr instance(endorseMistrustLoadedFunction function, timeoutFunction timeout) { 
		return std::make_shared<LoadEndorse>(function, timeout); 
	}

private:
	endorseMistrustLoadedFunction function_;
	timeoutFunction timeout_;
};

// load mistrust tx
class LoadMistrust: public ILoadEndorseMistrustHandler, public std::enable_shared_from_this<LoadMistrust> {
public:
	LoadMistrust(endorseMistrustLoadedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ILoadTrustScoreHandler
	void handleReply(const uint256& tx) {
		 function_(tx);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ILoadEndorseMistrustHandlerPtr instance(endorseMistrustLoadedFunction function, timeoutFunction timeout) { 
		return std::make_shared<LoadMistrust>(function, timeout); 
	}

private:
	endorseMistrustLoadedFunction function_;
	timeoutFunction timeout_;
};

//
// load buzzer & info interface
class ILoadBuzzerAndInfoHandler: public IReplyHandler {
public:
	ILoadBuzzerAndInfoHandler() {}
	virtual void handleReply(EntityPtr, TransactionPtr) = 0;
};
typedef std::shared_ptr<ILoadBuzzerAndInfoHandler> ILoadBuzzerAndInfoHandlerPtr;

//
typedef boost::function<void (EntityPtr, TransactionPtr)> buzzerAndInfoLoadedFunction;
typedef boost::function<void (EntityPtr, TransactionPtr, const std::string&)> buzzerAndInfoByBuzzerLoadedFunction;
typedef boost::function<void (EntityPtr, TransactionPtr, const std::string&, const ProcessingError&)> buzzerAndInfoDoneWithErrorFunction;
typedef boost::function<void (EntityPtr, TransactionPtr, const std::vector<Transaction::NetworkUnlinkedOut>&, const std::string&, const ProcessingError&)> buzzerInfoAndUtxoDoneWithErrorFunction;

// load buzzer & info
class LoadBuzzerAndInfo: public ILoadBuzzerAndInfoHandler, public std::enable_shared_from_this<LoadBuzzerAndInfo> {
public:
	LoadBuzzerAndInfo(buzzerAndInfoLoadedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ILoadBuzzerAndInfoHandler
	void handleReply(EntityPtr buzzer, TransactionPtr info) {
		 function_(buzzer, info);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ILoadBuzzerAndInfoHandlerPtr instance(buzzerAndInfoLoadedFunction function, timeoutFunction timeout) { 
		return std::make_shared<LoadBuzzerAndInfo>(function, timeout); 
	}

private:
	buzzerAndInfoLoadedFunction function_;
	timeoutFunction timeout_;
};

} // qbit

#endif