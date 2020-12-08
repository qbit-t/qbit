#include <QFileInfo>

#include <boost/function.hpp>

#include "logger.h"
#include "client.h"
#include "settings.h"
#include "error.h"
#include "log/log.h"
#include "transaction.h"

#include "lightwallet.h"
#include "peer.h"
#include "peermanager.h"

#include "buzztexthighlighter.h"
#include "wallettransactionslistmodel.h"

#include <QQuickImageProvider>

#if defined (BUZZER_MOD)
	#include "dapps/buzzer/txbuzzer.h"
	#include "dapps/buzzer/txbuzz.h"
	#include "dapps/buzzer/txbuzzlike.h"
	#include "dapps/buzzer/txbuzzreward.h"
	#include "dapps/buzzer/txbuzzreply.h"
	#include "dapps/buzzer/txrebuzz.h"
	#include "dapps/buzzer/txrebuzznotify.h"
	#include "dapps/buzzer/txbuzzersubscribe.h"
	#include "dapps/buzzer/txbuzzerunsubscribe.h"
	#include "dapps/buzzer/txbuzzerendorse.h"
	#include "dapps/buzzer/txbuzzermistrust.h"
	#include "dapps/buzzer/peerextension.h"
	#include "dapps/buzzer/buzzer.h"
	#include "dapps/buzzer/buzzfeed.h"
	#include "dapps/buzzer/eventsfeed.h"
	#include "dapps/buzzer/composer.h"
	#include "client/dapps/buzzer/buzzerrequestprocessor.h"
	#include "client/dapps/buzzer/buzzercommands.h"
#endif

#if defined (CUBIX_MOD)
	#include "dapps/cubix/cubix.h"
	#include "dapps/cubix/txmediaheader.h"
	#include "dapps/cubix/txmediadata.h"
	#include "dapps/cubix/txmediasummary.h"
	#include "dapps/cubix/peerextension.h"
	#include "client/dapps/cubix/cubixcomposer.h"
	#include "client/dapps/cubix/cubixcommands.h"
#endif

#include "commandwrappers.h"
#include "imagelisting.h"
#include "applicationpath.h"

using namespace buzzer;
using namespace qbit;

//
// Client
//
Client::Client(QObject *parent): QObject(parent) {
	//
    theme_  = "Darkmatter";
    themeSelector_ = "dark";
    locale_ = "EN";

	back_.loadFromString("{}");

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

Client::~Client() {
	qDebug() << "Client::~Client";
	delete settings_;
}

int Client::openSettings() {
	try {
		// make settings
		settings_ = SettingsFactory::get();
		settings_->link(this);

		settings_->open();
	} catch (buzzer::Exception& ex) {
		qDebug() << "[Client/openSettings/error]: " << ex.message().c_str();
		if (ex.code() == 1 /* document is empty */) {
            return 1002; // settings is unavailable
        }
        return 1000; // generic exception
    }

	return 0;
}

int Client::open(QString secret) {
	// setup log
	qbit::gLog(settings_->dataPath() + "/debug.log");

	// setup categories
	std::vector<std::string> lCategories;
	boost::split(lCategories, std::string(application_->getLogCategories()), boost::is_any_of(","));
	for (std::vector<std::string>::iterator lCategory = lCategories.begin(); lCategory != lCategories.end(); lCategory++) {
		qbit::gLog().enable(qbit::getLogCategory(*lCategory));
	}

	// setup testnet
	qbit::gTestNet = application_->getTestNet();

	// intercept qbit log - only in debug mode
	if (application_->getDebug())
		qbit::gLog().setEcho(boost::bind(&Client::echoLog, this, _1, _2));

	// rebind qapplication message handler to intercept qInfo(), qError() output messages
	buzzer::gLogger.reset(new buzzer::Logger());

	//
	Transaction::registerTransactionType(Transaction::ASSET_TYPE, TxAssetTypeCreator::instance());
	Transaction::registerTransactionType(Transaction::ASSET_EMISSION, TxAssetEmissionCreator::instance());
	Transaction::registerTransactionType(Transaction::DAPP, TxDAppCreator::instance());
	Transaction::registerTransactionType(Transaction::SHARD, TxShardCreator::instance());
	Transaction::registerTransactionType(Transaction::BASE, TxBaseCreator::instance());
	Transaction::registerTransactionType(Transaction::BLOCKBASE, TxBlockBaseCreator::instance());

	// request processor
	requestProcessor_ = RequestProcessor::instance(
		settings_->shared(),
		boost::bind(&Client::peerPushed, this, _1, _2, _3),
		boost::bind(&Client::peerPopped, this, _1, _2),
		boost::bind(&Client::chainStateChanged, this, _1, _2, _3, _4)
	);

	// wallet
	wallet_ = LightWallet::instance(settings_->shared(), requestProcessor_);
	wallet_->setWalletReceiveTransactionFunction("common", boost::bind(&Client::transactionReceived, this, _1, _2));

	std::static_pointer_cast<RequestProcessor>(requestProcessor_)->setWallet(wallet_);
	wallet_->open(secret.toStdString()); // secret - pin or keystore inlocked pin

	// wallet logs
	walletLog_ = new WalletTransactionsListModel("log", wallet_);
	walletReceivedLog_ = new WalletTransactionsListModelReceived(wallet_);
	walletSentLog_ = new WalletTransactionsListModelSent(wallet_);

	// peers logs
	peersActive_ = new PeersActiveListModel();
	peersAll_ = new PeersListModel();
	peersAdded_ = new PeersAddedListModel();

	// composer
	composer_ = LightComposer::instance(settings_->shared(), wallet_, requestProcessor_);

	// peer manager
	peerManager_ = PeerManager::instance(settings_->shared(), std::static_pointer_cast<RequestProcessor>(requestProcessor_)->consensusManager());

	// commands handler
	commandsHandler_ = CommandsHandler::instance(settings_->shared(), wallet_, requestProcessor_);

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
	Transaction::registerTransactionType(TX_BUZZ_REWARD, TxBuzzRewardCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_CONVERSATION, TxBuzzerConversationCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_ACCEPT_CONVERSATION, TxBuzzerAcceptConversationCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_DECLINE_CONVERSATION, TxBuzzerDeclineConversationCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_MESSAGE, TxBuzzerMessageCreator::instance());

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
	Message::registerMessageType(BUZZ_SUBSCRIBE, "BUZZ_SUBSCRIBE");
	Message::registerMessageType(BUZZ_UNSUBSCRIBE, "BUZZ_UNSUBSCRIBE");
	Message::registerMessageType(GET_BUZZER_AND_INFO, "GET_BUZZER_AND_INFO");
	Message::registerMessageType(BUZZER_AND_INFO, "BUZZER_AND_INFO");
	Message::registerMessageType(BUZZER_AND_INFO_ABSENT, "BUZZER_AND_INFO");
	Message::registerMessageType(GET_CONVERSATIONS_FEED_BY_BUZZER, "GET_CONVERSATIONS_FEED_BY_BUZZER");
	Message::registerMessageType(CONVERSATIONS_FEED_BY_BUZZER, "CONVERSATIONS_FEED_BY_BUZZER");
	Message::registerMessageType(GET_MESSAGES_FEED_BY_CONVERSATION, "GET_MESSAGES_FEED_BY_CONVERSATION");
	Message::registerMessageType(MESSAGES_FEED_BY_CONVERSATION, "MESSAGES_FEED_BY_CONVERSATION");
	Message::registerMessageType(NEW_BUZZER_CONVERSATION_NOTIFY, "NEW_BUZZER_CONVERSATION_NOTIFY");
	Message::registerMessageType(NEW_BUZZER_MESSAGE_NOTIFY, "NEW_BUZZER_MESSAGE_NOTIFY");
	Message::registerMessageType(UPDATE_BUZZER_CONVERSATION_NOTIFY, "UPDATE_BUZZER_CONVERSATION_NOTIFY");

	// buzzer request processor
	buzzerRequestProcessor_ = BuzzerRequestProcessor::instance(requestProcessor_);

	// buzzer
	buzzer_ = Buzzer::instance(requestProcessor_, buzzerRequestProcessor_, wallet_, boost::bind(&Client::trustScoreUpdated, this, _1, _2));

	// buzzer composer
	buzzerComposer_ = BuzzerLightComposer::instance(settings_->shared(), buzzer_, wallet_, requestProcessor_, buzzerRequestProcessor_);

	if (secret.size()) {
		// try secret
	}

	// create basic models
	buzzfeedList_ = new BuzzfeedListModelPersonal();
	globalBuzzfeedList_ = new BuzzfeedListModelGlobal();
	conversationsList_ = new ConversationsListModel();

	// initial
	buzzer_->setBuzzfeed(buzzfeedList_->buzzfeed());
	// register for updates
	buzzer_->registerUpdate(buzzfeedList_->buzzfeed());
	buzzer_->registerUpdate(globalBuzzfeedList_->buzzfeed());

	// events feed
	eventsfeedList_ = new EventsfeedListModelPersonal();
	buzzer_->setEventsfeed(eventsfeedList_->eventsfeed());
	// get event feed updates
	connect(eventsfeedList_, SIGNAL(eventsfeedUpdated(unsigned long long)),
								this, SLOT(eventsfeedUpdated(unsigned long long)));

	// conversations feed
	buzzer_->setConversations(conversationsList_->conversations());

	// get conversation updates
	connect(conversationsList_, SIGNAL(conversationsUpdated(unsigned long long)),
								this, SLOT(conversationsUpdated(unsigned long long)));

	// buzzer peer extention
	PeerManager::registerPeerExtension(
		"buzzer",
		BuzzerPeerExtensionCreator::instance(buzzer_, buzzfeedList_->buzzfeed(), eventsfeedList_->eventsfeed())
	);

#endif

#if defined (CUBIX_MOD)
	std::cout << "enabling 'cubix' module" << std::endl;

	// buzzer transactions
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_HEADER, cubix::TxMediaHeaderCreator::instance());
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_DATA, cubix::TxMediaDataCreator::instance());
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_SUMMARY, cubix::TxMediaSummaryCreator::instance());

	// cubix composer
	cubixComposer_ = cubix::CubixLightComposer::instance(settings_->shared(), wallet_, requestProcessor_);

	// peer extention
	PeerManager::registerPeerExtension("cubix", cubix::DefaultPeerExtensionCreator::instance());

	// dapp instance - shared
	buzzerComposer_->setInstanceChangedCallback(boost::bind(&cubix::CubixLightComposer::setDAppSharedInstance, cubixComposer_, _1));
#endif

#if defined (BUZZER_MOD)
	// prepare composers
	cubixComposer_->open();
	buzzerComposer_->open();
#endif

	// registed command wrappers
	qmlRegisterType<buzzer::ImageListing>("app.buzzer.components", 1, 0, "ImageListing");
	qmlRegisterType<buzzer::BuzzTextHighlighter>("app.buzzer.components", 1, 0, "BuzzTextHighlighter");
	qmlRegisterType<buzzer::WebSourceInfo>("app.buzzer.components", 1, 0, "WebSourceInfo");

	qmlRegisterType<buzzer::AskForQbitsCommand>("app.buzzer.commands", 1, 0, "AskForQbitsCommand");
	qmlRegisterType<buzzer::BalanceCommand>("app.buzzer.commands", 1, 0, "BalanceCommand");
	qmlRegisterType<buzzer::CreateBuzzerCommand>("app.buzzer.commands", 1, 0, "CreateBuzzerCommand");
	qmlRegisterType<buzzer::CreateBuzzerInfoCommand>("app.buzzer.commands", 1, 0, "CreateBuzzerInfoCommand");
	qmlRegisterType<buzzer::LoadBuzzerTrustScoreCommand>("app.buzzer.commands", 1, 0, "LoadBuzzerTrustScoreCommand");
	qmlRegisterType<buzzer::LoadBuzzesGlobalCommand>("app.buzzer.commands", 1, 0, "LoadBuzzesGlobalCommand");
	qmlRegisterType<buzzer::LoadBuzzfeedCommand>("app.buzzer.commands", 1, 0, "LoadBuzzfeedCommand");
	qmlRegisterType<buzzer::LoadBuzzfeedByBuzzCommand>("app.buzzer.commands", 1, 0, "LoadBuzzfeedByBuzzCommand");
	qmlRegisterType<buzzer::LoadBuzzfeedByBuzzerCommand>("app.buzzer.commands", 1, 0, "LoadBuzzfeedByBuzzerCommand");
	qmlRegisterType<buzzer::LoadBuzzfeedByTagCommand>("app.buzzer.commands", 1, 0, "LoadBuzzfeedByTagCommand");
	qmlRegisterType<buzzer::BuzzerSubscribeCommand>("app.buzzer.commands", 1, 0, "BuzzerSubscribeCommand");
	qmlRegisterType<buzzer::BuzzerUnsubscribeCommand>("app.buzzer.commands", 1, 0, "BuzzerUnsubscribeCommand");
	qmlRegisterType<buzzer::BuzzSubscribeCommand>("app.buzzer.commands", 1, 0, "BuzzSubscribeCommand");
	qmlRegisterType<buzzer::BuzzUnsubscribeCommand>("app.buzzer.commands", 1, 0, "BuzzUnsubscribeCommand");
	qmlRegisterType<buzzer::DownloadMediaCommand>("app.buzzer.commands", 1, 0, "DownloadMediaCommand");
	qmlRegisterType<buzzer::UploadMediaCommand>("app.buzzer.commands", 1, 0, "UploadMediaCommand");
	qmlRegisterType<buzzer::BuzzLikeCommand>("app.buzzer.commands", 1, 0, "BuzzLikeCommand");
	qmlRegisterType<buzzer::BuzzRewardCommand>("app.buzzer.commands", 1, 0, "BuzzRewardCommand");
	qmlRegisterType<buzzer::BuzzCommand>("app.buzzer.commands", 1, 0, "BuzzCommand");
	qmlRegisterType<buzzer::ReBuzzCommand>("app.buzzer.commands", 1, 0, "ReBuzzCommand");
	qmlRegisterType<buzzer::ReplyCommand>("app.buzzer.commands", 1, 0, "ReplyCommand");
	qmlRegisterType<buzzer::BuzzerEndorseCommand>("app.buzzer.commands", 1, 0, "BuzzerEndorseCommand");
	qmlRegisterType<buzzer::BuzzerMistrustCommand>("app.buzzer.commands", 1, 0, "BuzzerMistrustCommand");
	qmlRegisterType<buzzer::SearchEntityNamesCommand>("app.buzzer.commands", 1, 0, "SearchEntityNamesCommand");
	qmlRegisterType<buzzer::LoadHashTagsCommand>("app.buzzer.commands", 1, 0, "LoadHashTagsCommand");
	qmlRegisterType<buzzer::LoadBuzzerInfoCommand>("app.buzzer.commands", 1, 0, "LoadBuzzerInfoCommand");
	qmlRegisterType<buzzer::LoadEventsfeedCommand>("app.buzzer.commands", 1, 0, "LoadEventsfeedCommand");
	qmlRegisterType<buzzer::LoadEndorsementsByBuzzerCommand>("app.buzzer.commands", 1, 0, "LoadEndorsementsByBuzzerCommand");
	qmlRegisterType<buzzer::LoadMistrustsByBuzzerCommand>("app.buzzer.commands", 1, 0, "LoadMistrustsByBuzzerCommand");
	qmlRegisterType<buzzer::LoadFollowingByBuzzerCommand>("app.buzzer.commands", 1, 0, "LoadFollowingByBuzzerCommand");
	qmlRegisterType<buzzer::LoadFollowersByBuzzerCommand>("app.buzzer.commands", 1, 0, "LoadFollowersByBuzzerCommand");
	qmlRegisterType<buzzer::LoadEntityCommand>("app.buzzer.commands", 1, 0, "LoadEntityCommand");
	qmlRegisterType<buzzer::LoadTransactionCommand>("app.buzzer.commands", 1, 0, "LoadTransactionCommand");
	qmlRegisterType<buzzer::SendToAddressCommand>("app.buzzer.commands", 1, 0, "SendToAddressCommand");

	qmlRegisterType<buzzer::LoadConversationsCommand>("app.buzzer.commands", 1, 0, "LoadConversationsCommand");
	qmlRegisterType<buzzer::CreateConversationCommand>("app.buzzer.commands", 1, 0, "CreateConversationCommand");
	qmlRegisterType<buzzer::AcceptConversationCommand>("app.buzzer.commands", 1, 0, "AcceptConversationCommand");
	qmlRegisterType<buzzer::DeclineConversationCommand>("app.buzzer.commands", 1, 0, "DeclineConversationCommand");
	qmlRegisterType<buzzer::DecryptMessageBodyCommand>("app.buzzer.commands", 1, 0, "DecryptMessageBodyCommand");
	qmlRegisterType<buzzer::DecryptBuzzerMessageCommand>("app.buzzer.commands", 1, 0, "DecryptBuzzerMessageCommand");
	qmlRegisterType<buzzer::LoadConversationMessagesCommand>("app.buzzer.commands", 1, 0, "LoadConversationMessagesCommand");
	qmlRegisterType<buzzer::ConversationMessageCommand>("app.buzzer.commands", 1, 0, "ConversationMessageCommand");
	qmlRegisterType<buzzer::LoadCounterpartyKeyCommand>("app.buzzer.commands", 1, 0, "LoadCounterpartyKeyCommand");

	/*
	qmlRegisterType<buzzer::SelectConversationCommand>("app.buzzer.commands", 1, 0, "SelectConversationCommand");
	*/

	qmlRegisterType<buzzer::BuzzfeedListModel>("app.buzzer.commands", 1, 0, "BuzzfeedListModel");
	qmlRegisterType<buzzer::BuzzfeedListModelPersonal>("app.buzzer.commands", 1, 0, "BuzzfeedListModelPersonal");
	qmlRegisterType<buzzer::BuzzfeedListModelGlobal>("app.buzzer.commands", 1, 0, "BuzzfeedListModelGlobal");
	qmlRegisterType<buzzer::BuzzfeedListModelBuzzer>("app.buzzer.commands", 1, 0, "BuzzfeedListModelBuzzer");
	qmlRegisterType<buzzer::BuzzfeedListModelBuzzes>("app.buzzer.commands", 1, 0, "BuzzfeedListModelBuzzes");
	qmlRegisterType<buzzer::BuzzfeedListModelTag>("app.buzzer.commands", 1, 0, "BuzzfeedListModelTag");

	qmlRegisterType<buzzer::EventsfeedListModel>("app.buzzer.commands", 1, 0, "EventsfeedListModel");
	qmlRegisterType<buzzer::EventsfeedListModelPersonal>("app.buzzer.commands", 1, 0, "EventsfeedListModelPersonal");
	qmlRegisterType<buzzer::EventsfeedListModelEndorsements>("app.buzzer.commands", 1, 0, "EventsfeedListModelEndorsements");
	qmlRegisterType<buzzer::EventsfeedListModelMistrusts>("app.buzzer.commands", 1, 0, "EventsfeedListModelMistrusts");
	qmlRegisterType<buzzer::EventsfeedListModelFollowing>("app.buzzer.commands", 1, 0, "EventsfeedListModelFollowing");
	qmlRegisterType<buzzer::EventsfeedListModelFollowers>("app.buzzer.commands", 1, 0, "EventsfeedListModelFollowers");

	qmlRegisterType<buzzer::WalletTransactionsListModel>("app.buzzer.commands", 1, 0, "WalletTransactionsListModel");
	qmlRegisterType<buzzer::WalletTransactionsListModelReceived>("app.buzzer.commands", 1, 0, "WalletTransactionsListModelReceived");
	qmlRegisterType<buzzer::WalletTransactionsListModelSent>("app.buzzer.commands", 1, 0, "WalletTransactionsListModelSent");

	qmlRegisterType<buzzer::PeersListModel>("app.buzzer.commands", 1, 0, "PeersListModel");
	qmlRegisterType<buzzer::PeersActiveListModel>("app.buzzer.commands", 1, 0, "PeersActiveListModel");
	qmlRegisterType<buzzer::PeersAddedListModel>("app.buzzer.commands", 1, 0, "PeersAddedListModel");

	qmlRegisterType<buzzer::ConversationsfeedListModel>("app.buzzer.commands", 1, 0, "ConversationsfeedListModel");
	qmlRegisterType<buzzer::ConversationsListModel>("app.buzzer.commands", 1, 0, "ConversationsListModel");

	qRegisterMetaType<qbit::BuzzfeedProxy>("qbit::BuzzfeedProxy");
	qRegisterMetaType<qbit::BuzzfeedItemProxy>("qbit::BuzzfeedItemProxy");
	qRegisterMetaType<qbit::BuzzfeedItemUpdatesProxy>("qbit::BuzzfeedItemUpdatesProxy");

	qRegisterMetaType<qbit::EventsfeedProxy>("qbit::EventsfeedProxy");
	qRegisterMetaType<qbit::ConversationsfeedProxy>("qbit::ConversationsfeedProxy");

	qRegisterMetaType<qbit::ConversationItemProxy>("qbit::ConversationItemProxy");
	qRegisterMetaType<qbit::EventsfeedItemProxy>("qbit::EventsfeedItemProxy");

	qRegisterMetaType<buzzer::UnlinkedOutProxy>("buzzer::UnlinkedOutProxy");
	qRegisterMetaType<buzzer::NetworkUnlinkedOutProxy>("buzzer::NetworkUnlinkedOutProxy");
	qRegisterMetaType<buzzer::TransactionProxy>("buzzer::TransactionProxy");
	qRegisterMetaType<buzzer::PeerProxy>("buzzer::PeerProxy");

	qRegisterMetaType<buzzer::Contact>("buzzer::Contact");

	//
	/*
	peerManager_->addPeerExplicit("192.168.1.109:31415");
	peerManager_->addPeerExplicit("192.168.1.108:31415");
	peerManager_->addPeerExplicit("192.168.1.107:31415");
	peerManager_->addPeerExplicit("192.168.1.106:31415");
	peerManager_->addPeerExplicit("192.168.1.105:31415");
	peerManager_->addPeerExplicit("192.168.1.104:31415");
	peerManager_->addPeerExplicit("192.168.1.49:31415");
	*/

	/*
	peerManager_->addPeerExplicit("178.79.128.112:31415");
	peerManager_->addPeerExplicit("85.90.245.180:31416");
	*/

	// fill up peers
	peerManager_->run();

	// peers
	std::vector<std::string> lPeers;
	boost::split(lPeers, std::string(application_->getPeers()), boost::is_any_of(","));
	for (std::vector<std::string>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
		peerManager_->addPeer(*lPeer);
	}

	// prepare cache timer
	syncTimer_ = new QTimer(this);
	connect(syncTimer_, &QTimer::timeout, this, &Client::prepareCache);
	syncTimer_->start(500);

	return 1;
}

void Client::cleanUpBuzzerCache() {
	// TODO: do we need to clear local containers?
	if (wallet_) {
		prepareCache();
	}
}

void Client::preregisterBuzzerInstance() {
	//
	getBuzzerComposer()->proposeDAppInstance();
}

QVariant Client::locateConversation(QString id) {
	//
	uint256 lConversationId; lConversationId.setHex(id.toStdString());
	if (conversationsList_) {
		ConversationItemPtr lItem = conversationsList_->conversations()->locateItem(lConversationId);
		return QVariant::fromValue(new qbit::QConversationItem(lItem));
	}

	return QVariant();
}

void Client::addContact(QString buzzer, QString pkey) {
	getBuzzerComposer()->addContact(buzzer.toStdString(), pkey.toStdString());
	emit contactsChanged();
}

void Client::removeContact(QString buzzer) {
	getBuzzerComposer()->removeContact(buzzer.toStdString());
	emit contactsChanged();
}

bool Client::addPeerExplicit(QString endpoint) {
	//
	if (peerManager_) {
		return peerManager_->addPeerExplicit(endpoint.toStdString()) != nullptr;
	}

	return false;
}

void Client::removePeer(QString endpoint) {
	//
	if (peerManager_) {
		peerManager_->removePeer(endpoint.toStdString());
	}
}

QList<buzzer::Contact*> Client::selectContacts() {
	//
	QList<buzzer::Contact*> lList;
	std::map<std::string, std::string> lMap;
	getBuzzerComposer()->selectContacts(lMap);

	for (std::map<std::string, std::string>::iterator lItem = lMap.begin(); lItem != lMap.end(); lItem++) {
		//
		lList.push_back(new buzzer::Contact(QString::fromStdString(lItem->first), QString::fromStdString(lItem->second)));
	}

	return lList;
}

long Client::getQbitRate() {
	return settings_->maxFeeRate();
}

bool Client::isTimelockReached(const QString& utxo) {
	//
	uint256 lUtxo; lUtxo.setHex(utxo.toStdString());
	return wallet_->isTimelockReached(lUtxo);
}

QString Client::extractLastUrl(const QString& body) {
	//
	QString lUrl;
	QString lResult = body;
	QRegularExpression lRule = QRegularExpression("((?:https?|ftp)://\\S+)");
	QRegularExpressionMatchIterator lMatchIterator = lRule.globalMatch(lResult);
	while (lMatchIterator.hasNext()) {
		QRegularExpressionMatch lMatch = lMatchIterator.next();
		lUrl = lResult.mid(lMatch.capturedStart(), lMatch.capturedLength());
	}

	return lUrl;
}

QString Client::decorateBuzzBody(const QString& body) {
	//
	// already decorated
	if (body.indexOf("<a") != -1) return body;
	//
	QString lPattern = QString("<a href='\\1' style='text-decoration:none;color:") +
			gApplication->getColor(theme(), themeSelector(), "Material.link.rgb") +
			QString("'>\\1</a>");

	QString lUrlPattern = QString("<a href='\\1' style='text-decoration:none;color:") +
			gApplication->getColor(theme(), themeSelector(), "Material.link.rgb") +
			QString("'>\\2</a>");

	QString lResult = body;
	lResult.replace(QRegExp("(@[A-Za-z0-9]+)"), lPattern);
	lResult.replace(QRegExp("(#[\\w\u0400-\u04FF]+)"), lPattern);

	QRegularExpression lRule = QRegularExpression("((?:https?|ftp)://\\S+)");
	QRegularExpressionMatchIterator lMatchIterator = lRule.globalMatch(lResult);
	while (lMatchIterator.hasNext()) {
		QRegularExpressionMatch lMatch = lMatchIterator.next();
		QString lUrl = lResult.mid(lMatch.capturedStart(), lMatch.capturedLength());
		QString lUrlPatternDest = lUrlPattern;
		lUrlPatternDest.replace("\\1", lUrl);

		//
		if (lUrl.length() > 25) {
			lUrl.truncate(25); lUrl += "...";
		}
		lUrlPatternDest.replace("\\2", lUrl);
		lResult.replace(lMatch.capturedStart(), lMatch.capturedLength(), lUrlPatternDest);
	}

	if (lResult.size() && lResult[lResult.size()-1] == 0x0a) lResult[lResult.size()-1] = 0x20;
	lResult.replace(QRegExp("\n"), QString("<br>"));

	return lResult;
}

QString Client::internalTimestampAgo(long long timestamp, long long fraction) {
	//
	gApplication->getLocalization(locale(), "Buzzer.now");

	uint64_t lBuzzTime = timestamp / fraction;
	uint64_t lCurrentTime = qbit::getMicroseconds() / 1000000;
	const uint64_t lHour = 3600;
	const uint64_t lDay = 3600*24;

	if (lCurrentTime > lBuzzTime) {
		uint64_t lDiff = (lCurrentTime - lBuzzTime); // seconds
		if (lDiff < 5) return gApplication->getLocalization(locale(), "Buzzer.now");
		else if (lDiff < 60) {
			std::string lMark = gApplication->getLocalization(locale(), "Buzzer.seconds").toStdString();
			return QString::fromStdString(strprintf(std::string("%d")+lMark, lDiff));
		} else if (lDiff < lHour) {
			std::string lMark = gApplication->getLocalization(locale(), "Buzzer.minutes").toStdString();
			return QString::fromStdString(strprintf(std::string("%d")+lMark, lDiff/60));
		} else if (lDiff < lDay) {
			std::string lMark = gApplication->getLocalization(locale(), "Buzzer.hours").toStdString();
			return QString::fromStdString(strprintf(std::string("%d")+lMark, lDiff/(lHour)));
		} else {
			std::string lMark = gApplication->getLocalization(locale(), "Buzzer.days").toStdString();
			return QString::fromStdString(strprintf(std::string("%d")+lMark, lDiff/(lDay)));
		}
	}

	return gApplication->getLocalization(locale(), "Buzzer.now");
}

QString Client::timestampAgo(long long timestamp) {
	return internalTimestampAgo(timestamp, 1000000);
}

QString Client::timeAgo(long long timestamp) {
	return internalTimestampAgo(timestamp, 1);
}

ulong Client::getTrustScoreBase() {
	return BUZZER_TRUST_SCORE_BASE;
}

ulong Client::getQbitBase() {
	return QBIT;
}

QString Client::getTempFilesPath() {
	//
	return ApplicationPath::tempFilesDir();
}

QString Client::getBuzzerDescription(QString id) {
	//
	uint256 lId;
	lId.setHex(id.toStdString());

	TxBuzzerInfoPtr lInfo = buzzer_->locateBuzzerInfo(lId);
	if (lInfo) {
		std::string lDescription(lInfo->description().begin(), lInfo->description().end());
		return QString::fromStdString(lDescription);
	}

	return QString();
}

QString Client::getBuzzerName(QString id) {
	//
	uint256 lId;
	lId.setHex(id.toStdString());

	TxBuzzerInfoPtr lInfo = buzzer_->locateBuzzerInfo(lId);
	if (lInfo) {
		return QString::fromStdString(lInfo->myName());
	}

	return QString();
}

QString Client::getBuzzerAlias(QString id) {
	//
	uint256 lId;
	lId.setHex(id.toStdString());

	TxBuzzerInfoPtr lInfo = buzzer_->locateBuzzerInfo(lId);
	if (lInfo) {
		std::string lAlias(lInfo->alias().begin(), lInfo->alias().end());
		return QString::fromStdString(lAlias);
	}

	return QString();
}

QString Client::getBuzzerAvatarUrl(QString id) {
	//
	uint256 lId;
	lId.setHex(id.toStdString());

	BuzzerMediaPointer lMedia;
	if (buzzer_->locateAvatarMedia(lId, lMedia)) {
		return QString::fromStdString(lMedia.url());
	}

	return QString();
}

QString Client::getBuzzerHeaderUrl(QString id) {
	//
	uint256 lId;
	lId.setHex(id.toStdString());

	BuzzerMediaPointer lMedia;
	if (buzzer_->locateHeaderMedia(lId, lMedia)) {
		return QString::fromStdString(lMedia.url());
	}

	return QString();
}

QString Client::getBuzzerAvatarUrlHeader(QString id) {
	//
	uint256 lId;
	lId.setHex(id.toStdString());

	BuzzerMediaPointer lMedia;
	if (buzzer_->locateAvatarMedia(lId, lMedia)) {
		return QString::fromStdString(lMedia.tx().toHex());
	}

	return QString();
}

QString Client::getBuzzerHeaderUrlHeader(QString id) {
	//
	uint256 lId;
	lId.setHex(id.toStdString());

	BuzzerMediaPointer lMedia;
	if (buzzer_->locateHeaderMedia(lId, lMedia)) {
		return QString::fromStdString(lMedia.tx().toHex());
	}

	return QString();
}

bool Client::subscriptionExists(QString publisher) {
	//
	uint256 lPublisher; lPublisher.setHex(publisher.toStdString());
	return buzzerComposer_->subscriptionExists(lPublisher);
}


QString Client::resolveBuzzerDescription(QString buzzerInfoId) {
	//
	uint256 lInfoId; lInfoId.setHex(buzzerInfoId.toStdString());
	TxBuzzerInfoPtr lInfo = buzzer_->locateBuzzerInfo(lInfoId);
	if (lInfo) {
		std::string lDescription(lInfo->description().begin(), lInfo->description().end());
		return QString::fromStdString(lDescription);
	}

	return QString();
}

void Client::peerPushed(qbit::IPeerPtr peer, bool update, int count) {
	//
	if (count == 1) {
		setNetworkReady(true);
		emit networkLimited();
	} else {
		setNetworkReady(true);
		emit networkReady();
	}

	setBuzzerDAppReady();

	if (!suspended_)
		emit peerPushedSignal(PeerProxy(peer), update, count);
}

void Client::peerPopped(qbit::IPeerPtr peer, int count) {
	//
	if (count == 1) {
		setNetworkReady(true);
		emit networkLimited();
	} else if (count > 1) {
		setNetworkReady(true);
		emit networkReady();
	} else {
		setNetworkReady(false);
		emit networkUnavailable();
	}

	setBuzzerDAppReady();

	if (!suspended_)
		emit peerPoppedSignal(PeerProxy(peer), count);
}

void Client::prepareCache() {
	//
	if (wallet_->prepareCache()) {
		if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[Client::prepareCache]: cache is ready"));
		syncTimer_->stop();
		setCacheReady();
		setBuzzerDAppReady();
	}
}

void Client::setBuzzerDAppReady() {
	//
	std::map<uint256, std::map<uint32_t, IPeerPtr>> lMap;
	requestProcessor_->collectPeersByDApp("buzzer", lMap);

	//
	int lSupportNodes = 0;
	for (std::map<uint256, std::map<uint32_t, IPeerPtr>>::iterator lChain = lMap.begin(); lChain != lMap.end(); lChain++) {
		//if (gLog().isEnabled(Log::CLIENT))
		//	gLog().write(Log::CLIENT, strprintf("[Client::setBuzzerDAppReady]: chain = %s, count = %d", lChain->first.toHex(), lChain->second.size()));
		if (lChain->second.size() > 1) lSupportNodes += lChain->second.size();
	}

	// at least two nodes
	if (lSupportNodes / lMap.size() > 1) {
		bool lNotify = !buzzerDAppReady_;
		buzzerDAppReady_ = true;
		if (lNotify) {
			if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[Client::setBuzzerDAppReady]: buzzer is ready"));
			emit buzzerDAppReadyChanged();
		}
	} else {
		bool lNotify = buzzerDAppReady_;
		buzzerDAppReady_ = false;
		if (lNotify) emit buzzerDAppReadyChanged();
	}
}

QString Client::firstPKey() {
	//
	if (wallet_) {
		SKeyPtr lSKey = wallet_->firstKey();
		return QString::fromStdString(lSKey->createPKey().toString());
	}

	return QString();
}

QString Client::firstSKey() {
	//
	if (wallet_) {
		SKeyPtr lSKey = wallet_->firstKey();
		return QString::fromStdString(lSKey->toHex());
	}

	return QString();
}

QStringList Client::firstSeedWords() {
	//
	if (wallet_) {
		SKeyPtr lSKey = wallet_->firstKey();
		//
		QStringList lList;
		for (std::vector<SKey::Word>::iterator lWord = lSKey->seed().begin(); lWord != lSKey->seed().end(); lWord++) {
			lList.push_back(QString::fromStdString(lWord->wordA()));
		}

		return lList;
	}

	return QStringList();
}

void Client::removeAllKeys() {
	//
	if (wallet_) {
		wallet_->removeAllKeys();
	}
}

bool Client::importKey(QStringList words) {
	//
	std::list<std::string> lWords;
	for (QStringList::iterator lWord = words.begin(); lWord != words.end(); lWord++) {
		lWords.push_back(lWord->toStdString());
	}

	if (wallet_) {
		qbit::SKeyPtr lSKey = wallet_->createKey(lWords);
		return lSKey->valid();
	}

	return false;
}

bool Client::checkKey(QStringList words) {
	//
	std::list<std::string> lWords;
	for (QStringList::iterator lWord = words.begin(); lWord != words.end(); lWord++) {
		lWords.push_back(lWord->toStdString());
	}

	qbit::SKey lSKey(lWords);
	lSKey.create();
	return lSKey.valid();
}

void Client::unlink() {
    // reset current device_id
	setProperty("Client.deviceId", "");

    // reset acces by fingerprint
	setProperty("Client.fingerprintAccessEnabled", "");

	// reset configured flag
	setProperty("Client.configured", "");
}

bool Client::configured() {
	QString lValue = getProperty("Client.configured");
	if (lValue == "true") return true;
	return false;
}

bool Client::pinAccessConfigured() {
	QString lValue = getProperty("Client.pinAccessConfigured");
	if (lValue == "true") return true;
	return false;
}

bool Client::isFingerprintAccessConfigured() {
	QString lValue = getProperty("Client.fingerprintAccessEnabled");
    if (lValue == "true") return true;
    return false;
}

void Client::revertChanges() {
	qbit::json::Value lName;
	if (back_.find("name", lName)) {
		setName(QString::fromStdString(back_["name"].getString()));
		setAlias(QString::fromStdString(back_["alias"].getString()));
		setDescription(QString::fromStdString(back_["description"].getString()));
		setAvatar(QString::fromStdString(back_["avatar"].getString()));
		setHeader(QString::fromStdString(back_["header"].getString()));
		setLocale(QString::fromStdString(back_["locale"].getString()));
		setTheme(QString::fromStdString(back_["theme"].getString()));
		setThemeSelector(QString::fromStdString(back_["themeSelector"].getString()));
    }
}

void Client::settingsFromJSON(qbit::json::Value& root) {
	setName(QString::fromStdString(root["name"].getString()));
	setAlias(QString::fromStdString(root["alias"].getString()));
	setDescription(QString::fromStdString(root["description"].getString()));
	setAvatar(QString::fromStdString(root["avatar"].getString()));
	setHeader(QString::fromStdString(root["header"].getString()));
	setLocale(QString::fromStdString(root["locale"].getString()));
	setTheme(QString::fromStdString(root["theme"].getString()));
	setThemeSelector(QString::fromStdString(root["themeSelector"].getString()));

    properties_.clear();

	qbit::json::Value lProperties = root["properties"];
	if (root.find("properties", lProperties)) {
		for (qbit::json::ValueMemberIterator lProperty = lProperties.begin(); lProperty != lProperties.end(); lProperty++) {
			properties_.insert(std::map<std::string, std::string>::value_type(lProperty.getName(), lProperty.getValue().getString()));
        }
    }

	back_.loadFromString("{}");
	root.clone(back_);
}

void Client::settingsToJSON(qbit::json::Value& root) {
	root.addString("name", name().toStdString());
	root.addString("alias", alias().toStdString());
	root.addString("description", description().toStdString());
	root.addString("avatar", avatar().toStdString());
	root.addString("header", header().toStdString());
	root.addString("locale", locale().toStdString());
	root.addString("theme", theme().toStdString());
	root.addString("themeSelector", themeSelector().toStdString());

	qbit::json::Value lProperties = root.addObject("properties");

	for(std::map<std::string, std::string>::iterator lProperty = properties_.begin(); lProperty != properties_.end(); lProperty++) {
        lProperties.addString(lProperty->first, lProperty->second);
    }

	back_.loadFromString("{}");
	root.clone(back_);
}

void Client::setProperty(QString key, QString value) {
	if (properties_.find(key.toStdString()) == properties_.end()) {
		properties_.insert(std::map<std::string, std::string>::value_type(key.toStdString(), value.toStdString()));
	} else {
		properties_[key.toStdString()] = value.toStdString();
    }

    save();
}

QString Client::save() {
	try {
		if (!settings_) NULL_REF_EXCEPTION();
		settings_->update();
	} catch(buzzer::Exception const& ex) {
        return QString(ex.message().c_str());
    }

    return QString();
}

QString Client::getProperty(QString key) {
	if (properties_.find(key.toStdString()) == properties_.end()) return QString();
	return QString::fromStdString(properties_[key.toStdString()].c_str());
}

bool Client::hasPropertyBeginWith(QString sub) {
	std::string lPropSample = sub.toStdString();
	for(std::map<std::string, std::string>::iterator lProp = properties_.begin(); lProp != properties_.end(); lProp++) {
        if (lProp->first.find(lPropSample) == 0) return true;
    }

    return false;
}

bool Client::hasPropertyBeginWithAndValue(QString sub, QString val) {
	std::string lPropSample = sub.toStdString();
	for(std::map<std::string, std::string>::iterator lProp = properties_.begin(); lProp != properties_.end(); lProp++) {
		if (lProp->first.find(lPropSample) == 0 && lProp->second == val.toStdString()) return true;
    }

    return false;
}
