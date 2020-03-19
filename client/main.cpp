//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../allocator.h"

#include <iostream>
#include <boost/algorithm/string.hpp>

#include "../key.h"
#include "../transaction.h"
#include "../lightwallet.h"
#include "../requestprocessor.h"
#include "../iconsensusmanager.h"
#include "../ipeermanager.h"
#include "../ipeer.h"
#include "../peer.h"
#include "../peermanager.h"

#include "libsecp256k1-config.h"
#include "scalar_impl.h"

#include "../utilstrencodings.h"
#include "../jm/include/jm_utils.h"

#include "settings.h"
#include "commandshandler.h"
#include "commands.h"

using namespace qbit;

int main(int argv, char** argc) {
	//
	std::cout << std::endl << "qbit-cli (" << 
		CLIENT_VERSION_MAJOR << "." <<
		CLIENT_VERSION_MINOR << "." <<
		CLIENT_VERSION_REVISION << "." <<
		CLIENT_VERSION_BUILD << ") | (c) 2020 q-bit.technology | MIT license" << std::endl;

	// setings
	ISettingsPtr lSettings = ClientSettings::instance();

	// logs
	gLog(lSettings->dataPath() + "/debug.log"); // setup

	// command line
	bool lDebugFound = false;
	std::vector<std::string> lPeers;
	for (int lIdx = 1; lIdx < argv; lIdx++) {
		//
		if (std::string(argc[lIdx]) == std::string("-debug")) {
			std::vector<std::string> lCategories; 
			boost::split(lCategories, std::string(argc[++lIdx]), boost::is_any_of(","));

			for (std::vector<std::string>::iterator lCategory = lCategories.begin(); lCategory != lCategories.end(); lCategory++) {
				gLog().enable(getLogCategory(*lCategory));				
			}

			lDebugFound = true;
		} else if (std::string(argc[lIdx]) == std::string("-peers")) {
			boost::split(lPeers, std::string(argc[++lIdx]), boost::is_any_of(","));
		}
	}

	if (!lDebugFound) {
		gLog().enable(Log::INFO);
		gLog().enable(Log::ERROR);
		gLog().enable(Log::CLIENT);
	}

	// request processor
	IRequestProcessorPtr lRequestProcessor = RequestProcessor::instance(lSettings);

	// wallet
	IWalletPtr lWallet = LightWallet::instance(lSettings, lRequestProcessor);
	std::static_pointer_cast<RequestProcessor>(lRequestProcessor)->setWallet(lWallet);
	lWallet->open();

	// peer manager
	IPeerManagerPtr lPeerManager = PeerManager::instance(lSettings, std::static_pointer_cast<RequestProcessor>(lRequestProcessor)->consensusManager());

	// commands jandler
	CommandsHandlerPtr lCommandsHandler = CommandsHandler::instance(lSettings, lWallet, lRequestProcessor);
	lCommandsHandler->push(KeyCommand::instance());
	lCommandsHandler->push(BalanceCommand::instance());
	lCommandsHandler->push(SendToAddressCommand::instance());

	// peers
	for (std::vector<std::string>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
		lPeerManager->addPeer(*lPeer);
	}

	// control thread
	lPeerManager->run();

	// request data
	bool lPreparingCache = false;
	bool lExit = false;
	bool lOpeningWallet = false;
	while(!lExit) {
		//
		if (lWallet->status() == IWallet::UNKNOWN || !lPreparingCache) {
			lPreparingCache = lWallet->prepareCache();
			if (!lOpeningWallet) { std::cout << "synchronizing wallet ["; lOpeningWallet = true; }
			std::cout << "." << std::flush;
			boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
			continue;
		} else if (lWallet->status() != IWallet::OPENED) {
			std::cout << "." << std::flush;
			boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
			continue;
		} else if (lWallet->status() == IWallet::OPENED && lOpeningWallet) {
			std::cout << "]" << std::endl << std::flush;
			lOpeningWallet = false;
		}

		//
		std::string lInput;

		std::cout << "\nqbit>";
		std::getline(std::cin, lInput);

		if (!lInput.size()) continue;
		if (lWallet->status() != IWallet::OPENED) continue;

		std::string lCommand;
		std::vector<std::string> lArgs;

		std::vector<std::string> lRawArgs; 
		boost::split(lRawArgs, lInput, boost::is_any_of(" "));

		bool lCat = false;
		lCommand = *lRawArgs.begin(); lRawArgs.erase(lRawArgs.begin());
		for(std::vector<std::string>::iterator lArg = lRawArgs.begin(); lArg != lRawArgs.end(); lArg++) {
			if (!lCat) lArgs.push_back(*lArg);
			else *(lArgs.rbegin()) += *lArg;

			if (*(lArg->begin()) == '\"' && *(lArg->rbegin()) == '\"') {
				lArgs.rbegin()->erase(lArgs.rbegin()->begin());
				lArgs.rbegin()->erase(--(lArgs.rbegin()->end()));
			} else if (*(lArg->begin()) == '\"') { 
				lCat = true; 
			} else if (*(lArg->rbegin()) == '\"') { 
				lCat = false;
				lArgs.rbegin()->erase(lArgs.rbegin()->begin());
				lArgs.rbegin()->erase(--(lArgs.rbegin()->end()));
			}
		}

		if (lCommand == "quit" || lCommand == "q") { lExit = true; continue; }

		try {
			lCommandsHandler->handleCommand(lCommand, lArgs);
		}
		catch(qbit::exception& ex) {
			gLog().writeClient(Log::ERROR, std::string(": ") + ex.code() + " | " + ex.what());
		}
		catch(std::exception& ex) {
			gLog().writeClient(Log::ERROR, std::string(": ") + ex.what());
		}
	}

	return 0;
}
