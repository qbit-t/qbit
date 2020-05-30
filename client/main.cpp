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
	#include "../dapps/buzzer/txbuzzreply.h"
	#include "../dapps/buzzer/txrebuzz.h"
	#include "../dapps/buzzer/txrebuzznotify.h"
	#include "../dapps/buzzer/txbuzzersubscribe.h"
	#include "../dapps/buzzer/txbuzzerunsubscribe.h"
	#include "../dapps/buzzer/txbuzzerendorse.h"
	#include "../dapps/buzzer/txbuzzermistrust.h"
	#include "../dapps/buzzer/peerextension.h"
	#include "../dapps/buzzer/buzzer.h"
	#include "../dapps/buzzer/buzzfeed.h"
	#include "../dapps/buzzer/eventsfeed.h"
	#include "dapps/buzzer/requestprocessor.h"
	#include "dapps/buzzer/composer.h"
	#include "dapps/buzzer/commands.h"
#endif

#if defined (CUBIX_MOD)
	#include "../dapps/cubix/cubix.h"
	#include "../dapps/cubix/txmediaheader.h"
	#include "../dapps/cubix/txmediadata.h"
	#include "../dapps/cubix/txmediasummary.h"
	#include "../dapps/cubix/peerextension.h"
	#include "dapps/cubix/composer.h"
	#include "dapps/cubix/commands.h"
#endif

using namespace qbit;

#if defined (BUZZER_MOD)
//
// buzzfeed
//
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

//
// eventsfeed
//
void eventsfeedLargeUpdated() {
	//
}

void eventsfeedItemNew(EventsfeedItemPtr buzz) {
	//
	std::cout << std::endl;
	gLog().writeClient(Log::CLIENT, std::string(": new event for ") + buzz->buzzId().toHex());
}

void eventsfeedItemUpdated(EventsfeedItemPtr buzz) {
	//
	std::cout << std::endl;
	gLog().writeClient(Log::CLIENT, std::string(": updated event for ") + buzz->buzzId().toHex());
}

void eventsfeedItemsUpdated(const std::vector<EventsfeedItem>& items) {
	//
	std::cout << std::endl;
	gLog().writeClient(Log::CLIENT, strprintf(": events bulk updates count %d", items.size()));
	for (std::vector<EventsfeedItem>::const_iterator lUpdate = items.begin(); lUpdate != items.end(); lUpdate++)
		gLog().writeClient(Log::CLIENT, std::string(": -> event for ") + lUpdate->buzzId().toHex());
}

void trustScoreUpdated(amount_t edorsements, amount_t mistrusts) {
	//
	std::cout << std::endl;
	gLog().writeClient(Log::CLIENT, strprintf(": ts = %d / %d", edorsements, mistrusts));
}

#endif

bool gCommandDone = false;
void commandDone() {
	//
	gCommandDone = true;
}

void commandDone(TransactionPtr) {
	//
	gCommandDone = true;
}

int main(int argv, char** argc) {
	//

	uint32_t _QBIT_VERSION = 
			((((QBIT_VERSION_MAJOR << 16) + QBIT_VERSION_MINOR) << 8) + QBIT_VERSION_REVISION);

	std::cout << std::endl << "qbit-cli (" << 
		QBIT_VERSION_MAJOR << "." <<
		QBIT_VERSION_MINOR << "." <<
		QBIT_VERSION_REVISION << "." <<
		QBIT_VERSION_BUILD << ") | (c) 2020 q-bit.technology | MIT license" << std::endl;

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
	lRequestProcessor = RequestProcessor::instance(lSettings);

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
	lCommandsHandler->push(SearchEntityNamesCommand::instance(lComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(AskForQbitsCommand::instance(lComposer, boost::bind(&commandDone)));

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
	Transaction::registerTransactionType(TX_REBUZZ, TxReBuzzCreator::instance());
	Transaction::registerTransactionType(TX_BUZZ_REBUZZ_NOTIFY, TxReBuzzNotifyCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_ENDORSE, TxBuzzerEndorseCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_MISTRUST, TxBuzzerMistrustCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_INFO, TxBuzzerInfoCreator::instance());

	// buzzer message types
	Message::registerMessageType(GET_BUZZER_SUBSCRIPTION, "GET_BUZZER_SUBSCRIPTION");
	Message::registerMessageType(BUZZER_SUBSCRIPTION, "BUZZER_SUBSCRIPTION");
	Message::registerMessageType(BUZZER_SUBSCRIPTION_IS_ABSENT, "BUZZER_SUBSCRIPTION_IS_ABSENT");
	Message::registerMessageType(GET_BUZZ_FEED, "GET_BUZZ_FEED");
	Message::registerMessageType(BUZZ_FEED, "BUZZ_FEED");
	Message::registerMessageType(NEW_BUZZ_NOTIFY, "NEW_BUZZ_NOTIFY");
	Message::registerMessageType(BUZZ_UPDATE_NOTIFY, "BUZZ_UPDATE_NOTIFY");
	Message::registerMessageType(GET_BUZZES, "GET_BUZZES");
	Message::registerMessageType(GET_EVENTS_FEED, "GET_EVENTS_FEED");
	Message::registerMessageType(EVENTS_FEED, "EVENTS_FEED");
	Message::registerMessageType(NEW_EVENT_NOTIFY, "NEW_EVENT_NOTIFY");
	Message::registerMessageType(EVENT_UPDATE_NOTIFY, "EVENT_UPDATE_NOTIFY");
	Message::registerMessageType(GET_BUZZER_TRUST_SCORE, "GET_BUZZER_TRUST_SCORE");
	Message::registerMessageType(BUZZER_TRUST_SCORE, "BUZZER_TRUST_SCORE"); 
	Message::registerMessageType(BUZZER_TRUST_SCORE_UPDATE, "BUZZER_TRUST_SCORE_UPDATE");
	Message::registerMessageType(GET_BUZZER_MISTRUST_TX, "GET_BUZZER_MISTRUST_TX");
	Message::registerMessageType(BUZZER_MISTRUST_TX, "BUZZER_MISTRUST_TX");
	Message::registerMessageType(GET_BUZZER_ENDORSE_TX, "GET_BUZZER_ENDORSE_TX");
	Message::registerMessageType(BUZZER_ENDORSE_TX, "BUZZER_ENDORSE_TX");
	Message::registerMessageType(GET_BUZZ_FEED_BY_BUZZ, "GET_BUZZ_FEED_BY_BUZZ");
	Message::registerMessageType(BUZZ_FEED_BY_BUZZ, "BUZZ_FEED_BY_BUZZ");
	Message::registerMessageType(GET_BUZZ_FEED_BY_BUZZER, "GET_BUZZ_FEED_BY_BUZZER");
	Message::registerMessageType(BUZZ_FEED_BY_BUZZER, "BUZZ_FEED_BY_BUZZER");
	Message::registerMessageType(GET_MISTRUSTS_FEED_BY_BUZZER, "GET_MISTRUSTS_FEED_BY_BUZZER");
	Message::registerMessageType(MISTRUSTS_FEED_BY_BUZZER, "MISTRUSTS_FEED_BY_BUZZER");
	Message::registerMessageType(GET_ENDORSEMENTS_FEED_BY_BUZZER, "GET_ENDORSEMENTS_FEED_BY_BUZZER");
	Message::registerMessageType(ENDORSEMENTS_FEED_BY_BUZZER, "ENDORSEMENTS_FEED_BY_BUZZER");
	Message::registerMessageType(GET_BUZZER_SUBSCRIPTIONS, "GET_BUZZER_SUBSCRIPTIONS");
	Message::registerMessageType(BUZZER_SUBSCRIPTIONS, "BUZZER_SUBSCRIPTIONS");
	Message::registerMessageType(GET_BUZZER_FOLLOWERS, "GET_BUZZER_FOLLOWERS");
	Message::registerMessageType(BUZZER_FOLLOWERS, "BUZZER_FOLLOWERS");
	Message::registerMessageType(GET_BUZZ_FEED_GLOBAL, "GET_BUZZ_FEED_GLOBAL");
	Message::registerMessageType(BUZZ_FEED_GLOBAL, "BUZZ_FEED_GLOBAL");
	Message::registerMessageType(GET_BUZZ_FEED_BY_TAG, "GET_BUZZ_FEED_BY_TAG");
	Message::registerMessageType(BUZZ_FEED_BY_TAG, "BUZZ_FEED_BY_TAG");
	Message::registerMessageType(GET_HASH_TAGS, "GET_HASH_TAGS");
	Message::registerMessageType(HASH_TAGS, "HASH_TAGS");

	// buzzer
	BuzzerPtr lBuzzer = Buzzer::instance(lRequestProcessor, boost::bind(&trustScoreUpdated, _1, _2));

	// buzzer request processor
	BuzzerRequestProcessorPtr lBuzzerRequestProcessor = BuzzerRequestProcessor::instance(lRequestProcessor);

	// buzzer composer
	BuzzerLightComposerPtr lBuzzerComposer = BuzzerLightComposer::instance(lSettings, lBuzzer, lWallet, lRequestProcessor, lBuzzerRequestProcessor);

	// buzzfeed
	BuzzfeedPtr lBuzzfeed = Buzzfeed::instance(lBuzzer,
		boost::bind(&BuzzerLightComposer::verifyPublisherStrict, lBuzzerComposer, _1),
		boost::bind(&buzzfeedLargeUpdated), 
		boost::bind(&buzzfeedItemNew, _1), 
		boost::bind(&buzzfeedItemUpdated, _1),
		boost::bind(&buzzfeedItemsUpdated, _1),
		boost::bind(&buzzfeedItemAbsent, _1, _2),
		BuzzfeedItem::Merge::UNION
	);

	lBuzzfeed->prepare();
	lBuzzer->setBuzzfeed(lBuzzfeed); // main feed

	// global buzzfeed
	BuzzfeedPtr lGlobalBuzzfeed = Buzzfeed::instance(lBuzzer,
		boost::bind(&BuzzerLightComposer::verifyPublisherStrict, lBuzzerComposer, _1),
		boost::bind(&buzzfeedLargeUpdated), 
		boost::bind(&buzzfeedItemNew, _1), 
		boost::bind(&buzzfeedItemUpdated, _1),
		boost::bind(&buzzfeedItemsUpdated, _1),
		boost::bind(&buzzfeedItemAbsent, _1, _2),
		BuzzfeedItem::Merge::INTERSECT
	);

	lGlobalBuzzfeed->prepare();

	// buzzfeed for contexted selects
	BuzzfeedPtr lContextBuzzfeed = Buzzfeed::instance(lBuzzer,
		boost::bind(&BuzzerLightComposer::verifyPublisherStrict, lBuzzerComposer, _1),
		boost::bind(&buzzfeedLargeUpdated), 
		boost::bind(&buzzfeedItemNew, _1), 
		boost::bind(&buzzfeedItemUpdated, _1),
		boost::bind(&buzzfeedItemsUpdated, _1),
		boost::bind(&buzzfeedItemAbsent, _1, _2),
		BuzzfeedItem::Merge::INTERSECT
	);

	lContextBuzzfeed->prepare();

	// eventsfeed
	EventsfeedPtr lEventsfeed = Eventsfeed::instance(lBuzzer,
		boost::bind(&BuzzerLightComposer::verifyEventPublisher, lBuzzerComposer, _1),
		boost::bind(&eventsfeedLargeUpdated), 
		boost::bind(&eventsfeedItemNew, _1), 
		boost::bind(&eventsfeedItemUpdated, _1),
		boost::bind(&eventsfeedItemsUpdated, _1),
		EventsfeedItem::Merge::UNION
	);

	lEventsfeed->prepare();

	// eventsfeed for contexted selects
	EventsfeedPtr lContextEventsfeed = Eventsfeed::instance(lBuzzer,
		boost::bind(&BuzzerLightComposer::verifyEventPublisher, lBuzzerComposer, _1),
		boost::bind(&eventsfeedLargeUpdated), 
		boost::bind(&eventsfeedItemNew, _1), 
		boost::bind(&eventsfeedItemUpdated, _1),
		boost::bind(&eventsfeedItemsUpdated, _1),
		EventsfeedItem::Merge::INTERSECT
	);

	lContextEventsfeed->prepare();

	// buzzer peer extention
	PeerManager::registerPeerExtension("buzzer", BuzzerPeerExtensionCreator::instance(lBuzzer, lBuzzfeed, lEventsfeed));

	// buzzer commands
	CreateBuzzerCommandPtr lBuzzerCommand = std::static_pointer_cast<CreateBuzzerCommand>(
		CreateBuzzerCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(lBuzzerCommand);

	CreateBuzzCommandPtr lBuzzCommand = std::static_pointer_cast<CreateBuzzCommand>(
		CreateBuzzCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(lBuzzCommand);

	CreateBuzzReplyCommandPtr lBuzzReplyCommand = std::static_pointer_cast<CreateBuzzReplyCommand>(
		CreateBuzzReplyCommand::instance(lBuzzerComposer, lBuzzfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(lBuzzReplyCommand);

	CreateReBuzzCommandPtr lRebuzzCommand = std::static_pointer_cast<CreateReBuzzCommand>(
		CreateReBuzzCommand::instance(lBuzzerComposer, lBuzzfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(lRebuzzCommand);


	lCommandsHandler->push(BuzzerSubscribeCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(BuzzerUnsubscribeCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadBuzzfeedCommand::instance(lBuzzerComposer, lBuzzfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(BuzzfeedListCommand::instance(lBuzzerComposer, lBuzzfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(BuzzLikeCommand::instance(lBuzzerComposer, lBuzzfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadEventsfeedCommand::instance(lBuzzerComposer, lEventsfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(EventsfeedListCommand::instance(lBuzzerComposer, lEventsfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadBuzzerTrustScoreCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(BuzzerEndorseCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(BuzzerMistrustCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadHashTagsCommand::instance(lBuzzerComposer, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadBuzzesGlobalCommand::instance(lBuzzerComposer, lGlobalBuzzfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadBuzzfeedByTagCommand::instance(lBuzzerComposer, lContextBuzzfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadBuzzfeedByBuzzCommand::instance(lBuzzerComposer, lContextBuzzfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadBuzzfeedByBuzzerCommand::instance(lBuzzerComposer, lContextBuzzfeed, boost::bind(&commandDone)));	
	lCommandsHandler->push(LoadMistrustsByBuzzerCommand::instance(lBuzzerComposer, lContextEventsfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadEndorsementsByBuzzerCommand::instance(lBuzzerComposer, lContextEventsfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadSubscriptionsByBuzzerCommand::instance(lBuzzerComposer, lContextEventsfeed, boost::bind(&commandDone)));
	lCommandsHandler->push(LoadFollowersByBuzzerCommand::instance(lBuzzerComposer, lContextEventsfeed, boost::bind(&commandDone)));
#endif

#if defined (CUBIX_MOD)
	std::cout << "enabling 'cubix' module" << std::endl;

	// buzzer transactions
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_HEADER, cubix::TxMediaHeaderCreator::instance());
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_DATA, cubix::TxMediaDataCreator::instance());
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_SUMMARY, cubix::TxMediaSummaryCreator::instance());

	// cubix composer
	cubix::CubixLightComposerPtr lCubixComposer = cubix::CubixLightComposer::instance(lSettings, lWallet, lRequestProcessor);

	// peer extention
	PeerManager::registerPeerExtension("cubix", cubix::DefaultPeerExtensionCreator::instance());	

	// cubix commands
	lCommandsHandler->push(cubix::UploadMediaCommand::instance(lCubixComposer, boost::bind(&commandDone, _1)));
	lCommandsHandler->push(cubix::DownloadMediaCommand::instance(lCubixComposer, boost::bind(&commandDone, _1)));

	// dapp instance - shared
	lBuzzerComposer->setInstanceChangedCallback(boost::bind(&cubix::CubixLightComposer::setDAppSharedInstance, lCubixComposer, _1));

	// link uploads for buzzer info
	lBuzzerCommand->setUploadAvatar(
		cubix::UploadMediaCommand::instance(lCubixComposer, boost::bind(&CreateBuzzerCommand::avatarUploaded, lBuzzerCommand, _1))
	);
	lBuzzerCommand->setUploadHeader(
		cubix::UploadMediaCommand::instance(lCubixComposer, boost::bind(&CreateBuzzerCommand::headerUploaded, lBuzzerCommand, _1))
	);

	lBuzzCommand->setUploadMedia(
		cubix::UploadMediaCommand::instance(lCubixComposer, boost::bind(&CreateBuzzCommand::mediaUploaded, lBuzzCommand, _1))
	);

	lBuzzReplyCommand->setUploadMedia(
		cubix::UploadMediaCommand::instance(lCubixComposer, boost::bind(&CreateBuzzReplyCommand::mediaUploaded, lBuzzReplyCommand, _1))
	);

	lRebuzzCommand->setUploadMedia(
		cubix::UploadMediaCommand::instance(lCubixComposer, boost::bind(&CreateReBuzzCommand::mediaUploaded, lRebuzzCommand, _1))
	);
#endif

#if defined (BUZZER_MOD)
	// prepare composers
	lCubixComposer->open();
	lBuzzerComposer->open();
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
