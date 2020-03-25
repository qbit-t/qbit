// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_COMMANDS_H
#define QBIT_COMMANDS_H

#include "icommand.h"
#include "../ipeer.h"
#include "../log/log.h"
#include "lightcomposer.h"

namespace qbit {

class KeyCommand: public ICommand, public std::enable_shared_from_this<KeyCommand> {
public:
	KeyCommand(doneFunction done): done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("key"); 
		lSet.insert("k"); 
		return lSet;
	}

	void help() {
		std::cout << "key | k [address]" << std::endl;
		std::cout << "\tPrints key pair information, including: qbit-address, qbit-id, qbit-private key, seed phrase." << std::endl;
		std::cout << "\t[address] - optional, qbit-address for display" << std::endl << std::endl;
	}	

	static ICommandPtr instance(doneFunction done) { 
		return std::make_shared<KeyCommand>(done); 
	}

private:
	doneFunction done_;
};

class BalanceCommand;
typedef std::shared_ptr<BalanceCommand> BalanceCommandPtr;

class BalanceCommand: public ICommand, public std::enable_shared_from_this<BalanceCommand> {
public:
	BalanceCommand(LightComposerPtr composer, doneFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("balance"); 
		lSet.insert("bal"); 
		lSet.insert("b"); 
		return lSet;
	}

	void help() {
		std::cout << "balance | bal | b [asset]" << std::endl;
		std::cout << "\tPrints balance by asset." << std::endl;
		std::cout << "\t[asset] - optional, asset in hex to show balance for" << std::endl << std::endl;
	}

	static ICommandPtr instance(LightComposerPtr composer, doneFunction done) { 
		return std::make_shared<BalanceCommand>(composer, done); 
	}

	// callbacks
	void balance(double amount, amount_t scale) {
		std::cout << strprintf(TxAssetType::scaleFormat(scale), amount) << std::endl;
		done_();
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_();
	}

private:
	LightComposerPtr composer_;
	doneFunction done_;
};

class SendToAddressCommand;
typedef std::shared_ptr<SendToAddressCommand> SendToAddressCommandPtr;

class SendToAddressCommand: public ICommand, public std::enable_shared_from_this<SendToAddressCommand> {
public:
	SendToAddressCommand(LightComposerPtr composer, doneFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("sendToAddress"); 
		lSet.insert("send"); 
		lSet.insert("s"); 
		return lSet;
	}

	void help() {
		std::cout << "sendToAddress | send | s <asset or *> <address> <amount>" << std::endl;
		std::cout << "\tMake regular send transaction for specified asset and amount to given address." << std::endl;
		std::cout << "\t<asset or *> - required, asset in hex or qbit-asset - (*)" << std::endl;
		std::cout << "\t<address>    - required, recipient's address, qbit-address" << std::endl;
		std::cout << "\t<amount>     - required, amount to send, in asset units" << std::endl;
		std::cout << "\texample:\n\t\t>send * 523pXWffBi7Hgeyi6VSdhxSUJ1sYU1xunZ5bfnwBhy1dx6WG7v 0.5" << std::endl << std::endl;
	}	

	static ICommandPtr instance(LightComposerPtr composer, doneFunction done) { 
		return std::make_shared<SendToAddressCommand>(composer, done); 
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		if (composer_->requestProcessor()->broadcastTransaction(ctx)) {
			std::cout << ctx->tx()->id().toHex() << std::endl;		
		} else {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
		}

		done_();
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_();
	}	

private:
	LightComposerPtr composer_;
	doneFunction done_;
};

} // qbit

#endif