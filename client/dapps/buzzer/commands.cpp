#include "commands.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace qbit;

void CreateBuzzerCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 3) {
		// prepare
		IComposerMethodPtr lCreateBuzzer = BuzzerLightComposer::CreateTxBuzzer::instance(composer_, 
			std::string(args[0]), std::string(args[1]), std::string(args[2]),
			boost::bind(&CreateBuzzerCommand::created, shared_from_this(), _1));
		// async process
		lCreateBuzzer->process(boost::bind(&CreateBuzzerCommand::error, shared_from_this(), _1, _2));
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}

void CreateBuzzCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 1) {
		// prepare
		IComposerMethodPtr lCreateBuzz = BuzzerLightComposer::CreateTxBuzz::instance(composer_, args[0],
			boost::bind(&CreateBuzzCommand::created, shared_from_this(), _1));
		// async process
		lCreateBuzz->process(boost::bind(&CreateBuzzCommand::error, shared_from_this(), _1, _2));
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}

void BuzzerSubscribeCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 1) {
		// prepare
		IComposerMethodPtr lCreateBuzz = BuzzerLightComposer::CreateTxBuzzerSubscribe::instance(composer_, args[0],
			boost::bind(&BuzzerSubscribeCommand::created, shared_from_this(), _1));
		// async process
		lCreateBuzz->process(boost::bind(&BuzzerSubscribeCommand::error, shared_from_this(), _1, _2));
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}

void BuzzerUnsubscribeCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 1) {
		// prepare
		IComposerMethodPtr lCreateBuzz = BuzzerLightComposer::CreateTxBuzzerSubscribe::instance(composer_, args[0],
			boost::bind(&BuzzerUnsubscribeCommand::created, shared_from_this(), _1));
		// async process
		lCreateBuzz->process(boost::bind(&BuzzerUnsubscribeCommand::error, shared_from_this(), _1, _2));
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}
