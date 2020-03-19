#include "commandshandler.h"
#include "../log/log.h"
#include "../tinyformat.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace qbit;

CommandsHandler::CommandsHandler(ISettingsPtr settings, IWalletPtr wallet, IRequestProcessorPtr requestProcessor) : 
	settings_(settings), wallet_(wallet), requestProcessor_(requestProcessor) {
}

void CommandsHandler::handleCommand(const std::string& name, const std::vector<std::string>& args) {
	//
	std::map<std::string /*command*/, ICommandPtr>::iterator lCommand = commands_.find(name);
	if (lCommand != commands_.end()) {
		lCommand->second->process(args);
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": command not found"));
	}
}
