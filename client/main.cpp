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

#include "../txbase.h"
#include "../txblockbase.h"

#if defined (BUZZER_MOD)
	#include "../dapps/buzzer/txbuzzer.h"
	#include "../dapps/buzzer/txbuzz.h"
	#include "../dapps/buzzer/txbuzzlike.h"
	#include "../dapps/buzzer/txbuzzersubscribe.h"
	#include "../dapps/buzzer/txbuzzerunsubscribe.h"
	#include "../dapps/buzzer/peerextension.h"
	#include "../dapps/buzzer/buzzfeed.h"
	#include "dapps/buzzer/requestprocessor.h"
	#include "dapps/buzzer/composer.h"
	#include "dapps/buzzer/commands.h"
#endif

using namespace qbit;

#if defined (BUZZER_MOD)

void buzzfeedLargeUpdated() {
	//
}

void buzzfeedItemNew(BuzzfeedItemPtr buzz) {
	//
	std::cout << std::endl;
	gLog().writeClient(Log::CLIENT, std::string(": new buzz ") + buzz->buzzId().toHex());
}

void buzzfeedItemUpdated(BuzzfeedItemPtr buzz) {
	//
	std::cout << std::endl;
	gLog().writeClient(Log::CLIENT, std::string(": updated buzz ") + buzz->buzzId().toHex());
}

void buzzfeedItemsUpdated(const std::vector<BuzzfeedItem::Update>& items) {
	//
	std::cout << std::endl;
	gLog().writeClient(Log::CLIENT, strprintf(": buzz bulk updates count %d", items.size()));
	for (std::vector<BuzzfeedItem::Update>::const_iterator lUpdate = items.begin(); lUpdate != items.end(); lUpdate++)
		gLog().writeClient(Log::CLIENT, std::string(": -> buzz ") + lUpdate->buzzId().toHex());
}

void buzzfeedItemAbsent(const uint256& chain, const uint256& buzz) {
	//
	std::cout << std::endl;
	gLog().writeClient(Log::CLIENT, strprintf(": missing buzz %s/%s#", buzz.toHex(), chain.toHex().substr(0, 10)));
}

#endif

bool gCommandDone = false;
void commandDone() {
	//
	gCommandDone = true;
}

int main(int argv, char** argc) {
	//
	std::cout << std::endl << "qbit-cli (" << 
		CLIENT_VERSION_MAJOR << "." <<
		CLIENT_VERSION_MINOR << "." <<
		CLIENT_VERSION_REVISION << "." <<
		CLIENT_VERSION_BUILD << ") | (c) 2020 q-bit.technology | MIT license" << std::endl;

	// home
	ISettingsPtr lSettings = ClientSettings::instance();
	bool lIsLogConfigured = false;

	// command line
	bool lDebugFound = false;
	std::vector<std::string> lPeers;
	for (int lIdx = 1; lIdx < argv; lIdx++) {
		//
		if (std::string(argc[lIdx]) == std::string("-debug")) {
			std::vector<std::string> lCategories; 
			boost::split(lCategories, std::string(argc[++lIdx]), boost::is_any_of(","));

			if (!lIsLogConfigured) { 
				gLog(lSettings->dataPath() + "/debug.log"); // setup 
				lIsLogConfigured = true;
			}

			for (std::vector<std::string>::iterator lCategory = lCategories.begin(); lCategory != lCategories.end(); lCategory++) {
				gLog().enable(getLogCategory(*lCategory));				
			}

			lDebugFound = true;
		} else if (std::string(argc[lIdx]) == std::string("-peers")) {
			boost::split(lPeers, std::string(argc[++lIdx]), boost::is_any_of(","));
		} else if (std::string(argc[lIdx]) == std::string("-home")) {
			std::string lHome = std::string(argc[++lIdx]);
			lSettings = ClientSettings::instance(lHome); // re-create
			gLog(lSettings->dataPath() + "/debug.log"); // setup
			lIsLogConfigured = true;
		}
	}

	if (!lDebugFound) {
		gLog().enable(Log::INFO);
		gLog().enable(Log::ERROR);
		gLog().enable(Log::CLIENT);
	}

	// request processor
	IRequestProcessorPtr lRequestProcessor = nullptr;
#if defined (BUZZER_MOD)
	lRequestProcessor = RequestProcessor::instance(lSettings, "buzzer");
#else
	lRequestProcessor = RequestProcessor::instance(lSettings);
#endif

	// wallet
	IWalletPtr lWallet = LightWallet::instance(lSettings, lRequestProcessor);
	std::static_pointer_cast<RequestProcessor>(lRequestProcessor)->setWallet(lWallet);
	lWallet->open();

	// composer
	LightComposerPtr lComposer = LightComposer::instance(lSettings, lWallet, lRequestProcessor);

	// peer manager
	IPeerManagerPtr lPeerManager = PeerManager::instance(lSettings, std::static_pointer_cast<RequestProcessor>(lRequestProcessor)->consensusManager());

	// commands handler
	CommandsHandlerPtr lCommandsHandler = CommandsHandler::instance(lSettings, lWallet, lRequestProcessor);
	lCommandsHandler->push(KeyCommand::instance(boost::bind(&commandDone)));
	lCommandsHandler->push(BalanceCommand::instance(lComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(SendToAddressCommand::instance(lComposer, boost::bind(&commandDone)));

	Transaction::registerTransactionType(Transaction::ASSET_TYPE, TxAssetTypeCreator::instance());
	Transaction::registerTransactionType(Transaction::ASSET_EMISSION, TxAssetEmissionCreator::instance());
	Transaction::registerTransactionType(Transaction::DAPP, TxDAppCreator::instance());
	Transaction::registerTransactionType(Transaction::SHARD, TxShardCreator::instance());
	Transaction::registerTransactionType(Transaction::BASE, TxBaseCreator::instance());
	Transaction::registerTransactionType(Transaction::BLOCKBASE, TxBlockBaseCreator::instance());

#if defined (BUZZER_MOD)
	std::cout << "enabling 'buzzer' module" << std::endl;

	// buzzer transactions
	Transaction::registerTransactionType(TX_BUZZER, TxBuzzerCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_SUBSCRIBE, TxBuzzerSubscribeCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_UNSUBSCRIBE, TxBuzzerUnsubscribeCreator::instance());
	Transaction::registerTransactionType(TX_BUZZ, TxBuzzCreator::instance());
	Transaction::registerTransactionType(TX_BUZZ_LIKE, TxBuzzLikeCreator::instance());
	Transaction::registerTransactionType(TX_BUZZ_REPLY, TxBuzzReplyCreator::instance());

	// buzzer message types
	Message::registerMessageType(GET_BUZZER_SUBSCRIPTION, "GET_BUZZER_SUBSCRIPTION");
	Message::registerMessageType(BUZZER_SUBSCRIPTION, "BUZZER_SUBSCRIPTION");
	Message::registerMessageType(BUZZER_SUBSCRIPTION_IS_ABSENT, "BUZZER_SUBSCRIPTION_IS_ABSENT");
	Message::registerMessageType(GET_BUZZ_FEED, "GET_BUZZ_FEED");
	Message::registerMessageType(BUZZ_FEED, "BUZZ_FEED");
	Message::registerMessageType(NEW_BUZZ_NOTIFY, "NEW_BUZZ_NOTIFY");
	Message::registerMessageType(BUZZ_UPDATE_NOTIFY, "BUZZ_UPDATE_NOTIFY");

	// buzzer request processor
	BuzzerRequestProcessorPtr lBuzzerRequestProcessor = BuzzerRequestProcessor::instance(lRequestProcessor);

	// buzzer composer
	BuzzerLightComposerPtr lBuzzerComposer = BuzzerLightComposer::instance(lSettings, lWallet, lRequestProcessor, lBuzzerRequestProcessor);
	lBuzzerComposer->open();

	// buzzfed
	BuzzfeedPtr lBuzzfeed = Buzzfeed::instance(
		boost::bind(&buzzfeedLargeUpdated), 
		boost::bind(&buzzfeedItemNew, _1), 
		boost::bind(&buzzfeedItemUpdated, _1),
		boost::bind(&buzzfeedItemsUpdated, _1),
		boost::bind(&buzzfeedItemAbsent, _1, _2)
	);

	// buzzer peer extention
	PeerManager::registerPeerExtension("buzzer", BuzzerPeerExtensionCreator::instance(lBuzzfeed));

	// buzzer commands
	lCommandsHandler->push(CreateBuzzerCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(CreateBuzzCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(BuzzerSubscribeCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(BuzzerUnsubscribeCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadBuzzfeedCommand::instance(lBuzzerComposer, lBuzzfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(BuzzfeedListCommand::instance(lBuzzerComposer, lBuzzfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(BuzzLikeCommand::instance(lBuzzerComposer, lBuzzfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(CreateBuzzReplyCommand::instance(lBuzzerComposer, lBuzzfeed, boost::bind(&commandDone)));
#endif

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
			else { *(lArgs.rbegin()) += " "; *(lArgs.rbegin()) += *lArg; }

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
		else if (lCommand == "help" || lCommand == "h") {
			lCommandsHandler->showHelp();
			continue;
		}

		try {
			gCommandDone = false;
			lCommandsHandler->handleCommand(lCommand, lArgs);

			// wating for signal
			while (!gCommandDone) {
				boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
			}
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
