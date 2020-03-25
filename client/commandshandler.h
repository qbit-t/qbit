// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_COMMANDS_HANDLER_H
#define QBIT_COMMANDS_HANDLER_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "icommand.h"
#include "../isettings.h"

#include <string>
#include <boost/noncopyable.hpp>

namespace qbit {

class CommandsHandler;
typedef std::shared_ptr<CommandsHandler> CommandsHandlerPtr;

class CommandsHandler: private boost::noncopyable {
public:
	CommandsHandler(ISettingsPtr, IWalletPtr, IRequestProcessorPtr);

	void handleCommand(const std::string&, const std::vector<std::string>&);

	static CommandsHandlerPtr instance(ISettingsPtr settings, IWalletPtr wallet, IRequestProcessorPtr requestProcessor) { 
		return std::make_shared<CommandsHandler>(settings, wallet, requestProcessor); 
	}

	void push(ICommandPtr command) {
		command->setWallet(wallet_);
		command->setRequestProcessor(requestProcessor_);

		std::set<std::string> lNames = command->name();
		for (std::set<std::string>::iterator lName = lNames.begin(); lName != lNames.end(); lName++) {
			commands_.insert(std::map<std::string /*command*/, ICommandPtr>::value_type(*lName, command));
		}
		
		list_.push_back(command);
	}

	void showHelp() {
		//
		for (std::list<ICommandPtr>::iterator lCommand = list_.begin(); lCommand != list_.end(); lCommand++) {
			(*lCommand)->help();
		}	
	}

private:
	// settings
	ISettingsPtr settings_;
	// wallet
	IWalletPtr wallet_;
	// request processor
	IRequestProcessorPtr requestProcessor_;

	// Need to be filled BEFORE all processing started
	std::map<std::string /*method*/, ICommandPtr> commands_;
	std::list<ICommandPtr> list_;
};

}

#endif