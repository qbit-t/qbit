// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CLIENT_HANDLERS_H
#define QBIT_CLIENT_HANDLERS_H

#include "../isettings.h"
#include "../iwallet.h"

#include <boost/function.hpp>

namespace qbit {

//
class ProcessingError {
public:
	ProcessingError() {}
	ProcessingError(const std::string& error, const std::string& message):
		errorCode_(error), errorMessage_(message) {}

	bool success() const {
		return !errorCode_.size();
	}

	const std::string& error() const { return errorCode_; }
	const std::string& message() const { return errorMessage_; }

private:
	std::string errorCode_;
	std::string errorMessage_;
};

//
typedef boost::function<void (void)> timeoutFunction;
typedef boost::function<void (void)> doneFunction;

//
typedef boost::function<void (const ProcessingError& /*error*/)> doneWithErrorFunction;
typedef boost::function<void (bool, const ProcessingError& /*error*/)> doneSentWithErrorFunction;
typedef boost::function<void (double, double, amount_t, const ProcessingError& /*error*/)> doneBalanceWithErrorFunction;
typedef boost::function<void (TransactionPtr, const ProcessingError& /*error*/)> doneTransactionWithErrorFunction;
typedef boost::function<void (TransactionPtr, TransactionPtr, const ProcessingError& /*error*/)> doneTransactionsWithErrorFunction;
typedef boost::function<void (amount_t, amount_t, uint32_t, uint32_t, const ProcessingError& /*error*/)> doneTrustScoreWithErrorFunction;
typedef boost::function<void (uint64_t, uint64_t)> progressFunction;
typedef boost::function<void (const std::string&, uint64_t, uint64_t)> progressUploadFunction;
typedef boost::function<void (const std::string&, const std::vector<IEntityStore::EntityName>&, const ProcessingError& /*error*/)> doneEntityNamesSelectedFunction;

//
typedef boost::function<void (TransactionPtr)> entityCreatedFunction;
typedef boost::function<void (TransactionPtr)> doneWithResultFunction;
typedef boost::function<void (EntityPtr)> entityLoadedFunction;
typedef boost::function<void (TransactionPtr)> transactionLoadedFunction;
typedef boost::function<void (const std::vector<TransactionPtr>&)> transactionsLoadedFunction;
typedef boost::function<void (const std::vector<Transaction::UnlinkedOut>&, const std::string&)> utxoByEntityNameSelectedFunction;
typedef boost::function<void (const std::vector<Transaction::NetworkUnlinkedOut>&, const uint256&)> utxoByTransactionSelectedFunction;
typedef boost::function<void (const std::map<uint32_t, uint256>&, const std::string&)> entityCountByShardsSelectedFunction;
typedef boost::function<void (const std::map<uint256, uint32_t>&, const std::string&)> entityCountByDAppSelectedFunction;
typedef boost::function<void (const std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>&)> utxoByEntityNamesSelectedFunction;
typedef boost::function<void (const std::string&, const std::vector<IEntityStore::EntityName>&)> entityNamesSelectedFunction;

typedef boost::function<void (TransactionContextPtr)> transactionCreatedFunction;
typedef boost::function<void (TransactionContextPtr, Transaction::UnlinkedOutPtr)> transactionCreatedWithUTXOFunction;
typedef boost::function<void (TransactionContextPtr, Transaction::UnlinkedOutPtr)> transactionCreatedWithOutFunction;
typedef boost::function<void (double, double, amount_t)> balanceReadyFunction;
typedef boost::function<void (const std::string&, const std::string&)> errorFunction;

typedef boost::function<void (const uint256&, const std::vector<TransactionContext::Error>&)> sentTransactionFunction;

typedef boost::function<void (const uint256&)> instanceChangedFunction;

// common method
class IComposerMethod {
public:
	virtual void process(errorFunction) {}
};
typedef std::shared_ptr<IComposerMethod> IComposerMethodPtr;

// load transaction
class LoadTransaction: public ILoadTransactionHandler, public std::enable_shared_from_this<LoadTransaction> {
public:
	LoadTransaction(transactionLoadedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ILoadEntityHandler
	void handleReply(TransactionPtr tx) {
		 function_(tx);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ILoadTransactionHandlerPtr instance(transactionLoadedFunction function, timeoutFunction timeout) { 
		return std::make_shared<LoadTransaction>(function, timeout); 
	}

private:
	transactionLoadedFunction function_;
	timeoutFunction timeout_;
};

// load transactions
class LoadTransactions: public ILoadTransactionsHandler, public std::enable_shared_from_this<LoadTransactions> {
public:
	LoadTransactions(transactionsLoadedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ILoadTransactionsHandler
	void handleReply(const std::vector<TransactionPtr>& txs) {
		 function_(txs);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ILoadTransactionsHandlerPtr instance(transactionsLoadedFunction function, timeoutFunction timeout) { 
		return std::make_shared<LoadTransactions>(function, timeout); 
	}

private:
	transactionsLoadedFunction function_;
	timeoutFunction timeout_;
};

// sent transaction
class SentTransaction: public ISentTransactionHandler, public std::enable_shared_from_this<SentTransaction> {
public:
	SentTransaction(sentTransactionFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ILoadEntityHandler
	void handleReply(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		 function_(tx, errors);
	}
	// ISentTransactionHandler
	void timeout() {
		timeout_();
	}

	static ISentTransactionHandlerPtr instance(sentTransactionFunction function, timeoutFunction timeout) { 
		return std::make_shared<SentTransaction>(function, timeout); 
	}

private:
	sentTransactionFunction function_;
	timeoutFunction timeout_;
};

// load entity 
class LoadEntity: public ILoadEntityHandler, public std::enable_shared_from_this<LoadEntity> {
public:
	LoadEntity(entityLoadedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ILoadEntityHandler
	void handleReply(EntityPtr entity) {
		 function_(entity);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ILoadEntityHandlerPtr instance(entityLoadedFunction function, timeoutFunction timeout) { 
		return std::make_shared<LoadEntity>(function, timeout); 
	}

private:
	entityLoadedFunction function_;
	timeoutFunction timeout_;
};

// select utxo by transaction 
class SelectUtxoByTransaction: public ISelectUtxoByTransactionHandler, public std::enable_shared_from_this<SelectUtxoByTransaction> {
public:
	SelectUtxoByTransaction(utxoByTransactionSelectedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ILoadEntityHandler
	void handleReply(const std::vector<Transaction::NetworkUnlinkedOut>& utxo, const uint256& tx) {
		function_(utxo, tx);
	}
	// IReplyHandler
	void timeout() {
		timeout_();	
	}

	static ISelectUtxoByTransactionHandlerPtr instance(utxoByTransactionSelectedFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectUtxoByTransaction>(function, timeout); 
	}

private:
	utxoByTransactionSelectedFunction function_;
	timeoutFunction timeout_;
};

// select utxo by entity name 
class SelectUtxoByEntityName: public ISelectUtxoByEntityNameHandler, public std::enable_shared_from_this<SelectUtxoByEntityName> {
public:
	SelectUtxoByEntityName(utxoByEntityNameSelectedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ILoadEntityHandler
	void handleReply(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& entity) {
		function_(utxo, entity);
	}
	// IReplyHandler
	void timeout() {
		timeout_();		
	}

	static ISelectUtxoByEntityNameHandlerPtr instance(utxoByEntityNameSelectedFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectUtxoByEntityName>(function, timeout); 
	}

private:
	utxoByEntityNameSelectedFunction function_;
	timeoutFunction timeout_;
};

// select utxo by entity names 
class SelectUtxoByEntityNames: public ISelectUtxoByEntityNamesHandler, public std::enable_shared_from_this<SelectUtxoByEntityNames> {
public:
	SelectUtxoByEntityNames(utxoByEntityNamesSelectedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ILoadEntityHandler
	void handleReply(const std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>& entityUtxos) {
		function_(entityUtxos);
	}
	// IReplyHandler
	void timeout() {
		timeout_();
	}

	static ISelectUtxoByEntityNamesHandlerPtr instance(utxoByEntityNamesSelectedFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectUtxoByEntityNames>(function, timeout); 
	}

private:
	utxoByEntityNamesSelectedFunction function_;
	timeoutFunction timeout_;
};

// select entity count by shards
class SelectEntityCountByShards: public ISelectEntityCountByShardsHandler, public std::enable_shared_from_this<SelectEntityCountByShards> {
public:
	SelectEntityCountByShards(entityCountByShardsSelectedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ISelectEntityCountByShardsHandler
	void handleReply(const std::map<uint32_t, uint256>& info, const std::string& dapp) {
		function_(info, dapp);
	}
	// IReplyHandler
	void timeout() {
		timeout_();		
	}

	static ISelectEntityCountByShardsHandlerPtr instance(entityCountByShardsSelectedFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectEntityCountByShards>(function, timeout); 
	}

private:
	entityCountByShardsSelectedFunction function_;
	timeoutFunction timeout_;
};

// select entity count by dapp
class SelectEntityCountByDApp: public ISelectEntityCountByDAppHandler, public std::enable_shared_from_this<SelectEntityCountByDApp> {
public:
	SelectEntityCountByDApp(entityCountByDAppSelectedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ISelectEntityCountByShardsHandler
	void handleReply(const std::map<uint256, uint32_t>& info, const std::string& dapp) {
		function_(info, dapp);
	}
	// IReplyHandler
	void timeout() {
		timeout_();		
	}

	static ISelectEntityCountByDAppHandlerPtr instance(entityCountByDAppSelectedFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectEntityCountByDApp>(function, timeout); 
	}

private:
	entityCountByDAppSelectedFunction function_;
	timeoutFunction timeout_;
};

// select entity names
class SelectEntityNames: public ISelectEntityNamesHandler, public std::enable_shared_from_this<SelectEntityNames> {
public:
	SelectEntityNames(entityNamesSelectedFunction function, timeoutFunction timeout): function_(function), timeout_(timeout) {}

	// ISelectEntityCountByShardsHandler
	void handleReply(const std::string& name, const std::vector<IEntityStore::EntityName>& names) {
		function_(name, names);
	}
	// IReplyHandler
	void timeout() {
		timeout_();		
	}

	static ISelectEntityNamesHandlerPtr instance(entityNamesSelectedFunction function, timeoutFunction timeout) { 
		return std::make_shared<SelectEntityNames>(function, timeout); 
	}

private:
	entityNamesSelectedFunction function_;
	timeoutFunction timeout_;
};

} // qbit

#endif
