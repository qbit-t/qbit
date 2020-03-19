// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_COMMANDS_H
#define QBIT_COMMANDS_H

#include "icommand.h"
#include "../ipeer.h"
#include "../log/log.h"

namespace qbit {

class KeyCommand: public ICommand, public std::enable_shared_from_this<KeyCommand> {
public:
	KeyCommand() {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("key"); 
		lSet.insert("k"); 
		return lSet;
	}

	static ICommandPtr instance() { 
		return std::make_shared<KeyCommand>(); 
	}
};

class BalanceCommand;
typedef std::shared_ptr<BalanceCommand> BalanceCommandPtr;

class BalanceCommand: public ICommand, public std::enable_shared_from_this<BalanceCommand> {
public:
	class LoadAssetType: public ILoadTransactionHandler, public std::enable_shared_from_this<LoadAssetType> {
	public:
		LoadAssetType(BalanceCommandPtr command): command_(command) {}

		// ILoadTransactionHandler
		void handleReply(TransactionPtr tx) {
			command_->assetTypeLoaded(tx);
		}
		// IReplyHandler
		void timeout() {
			gLog().write(Log::CLIENT, std::string(": request timeout..."));
		}

		static ILoadTransactionHandlerPtr instance(BalanceCommandPtr command) { 
			return std::make_shared<LoadAssetType>(command); 
		}

	private:
		BalanceCommandPtr command_;
	};

public:
	BalanceCommand() {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("balance"); 
		lSet.insert("bal"); 
		lSet.insert("b"); 
		return lSet;
	}

	void assetTypeLoaded(TransactionPtr);

	static ICommandPtr instance() { 
		return std::make_shared<BalanceCommand>(); 
	}	
};

class SendToAddressCommand;
typedef std::shared_ptr<SendToAddressCommand> SendToAddressCommandPtr;

class SendToAddressCommand: public ICommand, public std::enable_shared_from_this<SendToAddressCommand> {
public:
	class LoadAssetType: public ILoadTransactionHandler, public std::enable_shared_from_this<LoadAssetType> {
	public:
		LoadAssetType(SendToAddressCommandPtr command): command_(command) {}

		// ILoadTransactionHandler
		void handleReply(TransactionPtr tx) {
			command_->assetTypeLoaded(tx);
		}
		// IReplyHandler
		void timeout() {
			gLog().write(Log::CLIENT, std::string(": request timeout..."));
		}

		static ILoadTransactionHandlerPtr instance(SendToAddressCommandPtr command) { 
			return std::make_shared<LoadAssetType>(command); 
		}

	private:
		SendToAddressCommandPtr command_;
	};

public:
	SendToAddressCommand() {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("sendToAddress"); 
		lSet.insert("send"); 
		lSet.insert("s"); 
		return lSet;
	}

	void assetTypeLoaded(TransactionPtr);

	static ICommandPtr instance() { 
		return std::make_shared<SendToAddressCommand>(); 
	}	

private:
	std::vector<std::string> args_;	
};

} // qbit

#endif