#include <QFileInfo>
#include <QOperatingSystemVersion>

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

#include "emojimodel.h"
#include "audiorecorder.h"
#include "videorecorder.h"

#include <QQuickImageProvider>

#if defined (BUZZER_MOD)
	#include "dapps/buzzer/txbuzzer.h"
	#include "dapps/buzzer/txbuzz.h"
	#include "dapps/buzzer/txbuzzlike.h"
	#include "dapps/buzzer/txbuzzhide.h"
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

#if defined(DESKTOP_PLATFORM)
#include "pushdesktopnotification.h"
#endif

#include "iosimagepicker.h"

#ifdef Q_OS_IOS
#include <iosaudioplayer.h>
#endif

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

std::string Client::settingsDataPath() {
	return settings_->dataPath();
}

std::string Client::getQttAsset() {
	return gApplication->getQttAsset();
}

int Client::getQttAssetLockTime() {
	return gApplication->getQttAssetLockTime();
}

uint64_t Client::getQttAssetVoteAmount() {
	return gApplication->getQttAssetVoteAmount();
}

int Client::open(QString secret) {
	// setup log
#if defined(DESKTOP_PLATFORM)
	qbit::gLog(settings_->dataPath() + "/debug.log");
#else
	// TODO: make action to copy debug log to downloads
	// NOTICE: logs location points to the cache location and can be feed with the other temporary data
	qbit::gLog(application_->getLogsLocation() + "/debug.log");
#endif

	// setup categories
	std::vector<std::string> lCategories;
	boost::split(lCategories, std::string(application_->getLogCategories()), boost::is_any_of(","));
	for (std::vector<std::string>::iterator lCategory = lCategories.begin(); lCategory != lCategories.end(); lCategory++) {
		qbit::gLog().enable(qbit::getLogCategory(*lCategory));
	}

	// rebind qapplication message handler to intercept qInfo(), qError() output messages
	buzzer::gLogger.reset(new buzzer::Logger(application_->getInterceptOutput()));

	// empty emoji data
	emojiData_ = new EmojiData();

	//
#if defined(DESKTOP_PLATFORM)
	if (application_->isDesktop()) {
#if defined(Q_OS_WINDOWS)
		/*
		auto lCurrent = QOperatingSystemVersion::current();
		if (lCurrent >= QOperatingSystemVersion::Windows10) {
			if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoColorEmoji_WindowsCompatible.ttf") == -1) {
				if (gLog().isEnabled(Log::GENERAL_ERROR))
					gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoColorEmoji_WindowsCompatible.ttf was not loaded"));
			}
		}
		*/

		//QString lEmojiFontFile = ":/fonts-desktop/SeguiEmj.ttf";
 		//if (lCurrent >= QOperatingSystemVersion::Windows10)
 		//	lEmojiFontFile = ":/fonts-desktop/NotoColorEmoji_WindowsCompatible.ttf";
		//if (QFontDatabase::addApplicationFont(lEmojiFontFile) == -1) {
		//}
#else
#if defined(Q_OS_MACOS)
		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoColorEmoji.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoColorEmoji.ttf was not loaded"));
		}

		/*
		if (QFontDatabase::addApplicationFont(":/fonts/fa-light-300.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font fa-light-300.ttf was not loaded"));
		}
		*/
#else
		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoColorEmojiN.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoColorEmojiN.ttf was not loaded"));
		}
#endif

		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoSans-Regular.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoSans-Regular.ttf was not loaded"));
		}

		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoSans-Bold.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoSans-Bold.ttf was not loaded"));
		}

		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoSans-ExtraBold.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoSans-ExtraBold.ttf was not loaded"));
		}

		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoSans-Black.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoSans-Black.ttf was not loaded"));
		}

		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoSans-SemiBold.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoSans-SemiBold.ttf was not loaded"));
		}

		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoSans-Light.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoSans-Light.ttf was not loaded"));
		}

		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoSans-Italic.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoSans-Italic.ttf was not loaded"));
		}

		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoSans-Medium.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoSans-Medium.ttf was not loaded"));
		}

		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoSans-Thin.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoSans-Thin.ttf was not loaded"));
		}

		if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoSansMono-Regular.ttf") == -1) {
			if (gLog().isEnabled(Log::GENERAL_ERROR))
				gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoSansMono-Regular.ttf was not loaded"));
		}

#if defined(Q_OS_MACOS)
		QFont lAppFont("Noto Sans, Noto Color Emoji, Noto Sans Mono" /*, -1, QFont::Light*/);
#else
		QFont lAppFont("Noto Sans, Noto Color Emoji N, Noto Sans Mono" /*, -1, QFont::Light*/);
#endif
		lAppFont.setStyleStrategy(QFont::NoSubpixelAntialias);
		QApplication::setFont(lAppFont);
#endif

		emojiData_->open();
	}
#else
	if (QFontDatabase::addApplicationFont(":/fonts-desktop/NotoSansMono-Regular.ttf") == -1) {
		if (gLog().isEnabled(Log::GENERAL_ERROR))
			gLog().write(Log::GENERAL_ERROR, std::string("[Client::open]: font NotoSansMono-Regular.ttf was not loaded"));
	}
#endif

	// setup testnet
	qbit::gTestNet = application_->getTestNet();

	// intercept qbit log - only in debug mode
	if (application_->getDebug() && !application_->getInterceptOutput())
		qbit::gLog().setEcho(boost::bind(&Client::echoLog, this, _1, _2));

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
	walletLog_ = new WalletTransactionsListModel("log", TxAssetType::qbitAsset(), wallet_);
	walletReceivedLog_ = new WalletTransactionsListModelReceived(TxAssetType::qbitAsset(), wallet_);
	walletSentLog_ = new WalletTransactionsListModelSent(TxAssetType::qbitAsset(), wallet_);

	//
	// TODO: we need to make more robust mechanism for assets onboarding - i.e. scan address-bounded assets and extract asset info
	//

	// qtt
	uint256 lQttAsset; lQttAsset.setHex(application_->getQttAsset());
	wallets_[lQttAsset] = _Wallet(lQttAsset, wallet_);
	//

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
	Transaction::registerTransactionType(TX_BUZZ_HIDE, TxBuzzHideCreator::instance());
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
	connect(eventsfeedList_, SIGNAL(eventsfeedUpdated(const qbit::EventsfeedItemProxy&)),
								this, SLOT(eventsfeedUpdated(const qbit::EventsfeedItemProxy&)));

	// conversations feed
	buzzer_->setConversations(conversationsList_->conversations());

	// get conversation updates
	connect(conversationsList_, SIGNAL(conversationsUpdated(const qbit::ConversationItemProxy&)),
								this, SLOT(conversationsUpdated(const qbit::ConversationItemProxy&)));

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
#ifdef Q_OS_ANDROID
	qmlRegisterType<buzzer::ImageListing>("app.buzzer.components", 1, 0, "ImageListing");
	qmlRegisterType<buzzer::AudioRecorder>("app.buzzer.components", 1, 0, "AudioRecorder");
	qmlRegisterType<buzzer::VideoRecorder>("app.buzzer.components", 1, 0, "VideoRecorder");
#endif

#ifdef Q_OS_IOS
	qmlRegisterType<buzzer::AudioRecorder>("app.buzzer.components", 1, 0, "AudioRecorder");
	qmlRegisterType<buzzer::VideoRecorder>("app.buzzer.components", 1, 0, "VideoRecorder");
	qmlRegisterType<buzzer::AudioPlayer>("app.buzzer.components", 1, 0, "AudioPlayer");
#endif

	qmlRegisterType<QIImagePicker>("app.buzzer.components", 1, 0, "ImagePicker");

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
	qmlRegisterType<buzzer::BuzzHideCommand>("app.buzzer.commands", 1, 0, "BuzzHideCommand");
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

	qmlRegisterType<buzzer::EmojiModel>("app.buzzer.commands", 1, 0, "EmojiModel");

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

	// check bounds
	QString lLastTimestamp = getProperty("Client.lastTimestamp");
	if (!lLastTimestamp.length()) {
		setProperty("Client.lastTimestamp", QString::number(qbit::getMicroseconds()));
	}

	// prepare cache timer
	syncTimer_ = new QTimer(this);
	connect(syncTimer_, &QTimer::timeout, this, &Client::prepareCache);
	connect(this, &Client::fireResyncWalletCache, this, &Client::resyncWalletCache);

	// explicit peers only
	QString lExplicitPeersOnly = getProperty("Client.explicitPeersOnly");
	if (lExplicitPeersOnly == "true") {
		peerManager_->useExplicitPeersOnly();
	}

	// fill up peers
	peerManager_->run();

	// peers
	std::vector<std::string> lPeers;
	boost::split(lPeers, std::string(application_->getPeers()), boost::is_any_of(","));
	for (std::vector<std::string>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
		peerManager_->addPeer(*lPeer);
	}

	return 1;
}

void Client::resyncWalletCache() {
	syncTimer_->start(500);
}

// events feed updated
void Client::eventsfeedUpdated(const qbit::EventsfeedItemProxy& event) {
	//
	qbit::EventsfeedItemPtr lEventsfeedItem(event.get());
	QString lLastTimestamp = getProperty("Client.lastTimestamp");
	//qInfo() << "[eventsfeedUpdated]" << lEventsfeedItem->actualTimestamp() << !suspended_ << lLastTimestamp;
	if (lLastTimestamp.length()) {
		if (lLastTimestamp.toULongLong() < lEventsfeedItem->actualTimestamp()) {
			emit newEvents();
			setProperty("Client.lastTimestamp", QString::number(lEventsfeedItem->actualTimestamp()));
#if defined(DESKTOP_PLATFORM)
			// notify
			if (suspended_ || (QString::fromStdString(lEventsfeedItem->buzzId().toHex()) != topId_ &&
					(!lEventsfeedItem->buzzers().size() ||
					 QString::fromStdString(lEventsfeedItem->beginInfo()->eventId().toHex()) != topId_))) {
				qInfo() << "[eventsfeedUpdated/0]" << lEventsfeedItem->actualTimestamp() << !suspended_;
				PushNotification::instance(
							lEventsfeedItem,
							unMarkdownBuzzBodyLimited(QString::fromStdString(lEventsfeedItem->buzzBodyString()), -1),
							!suspended_)->process();
			}
#endif
		}
	} else {
		emit newEvents();
		setProperty("Client.lastTimestamp", QString::number(lEventsfeedItem->actualTimestamp()));
#if defined(DESKTOP_PLATFORM)
		// notify
		if (suspended_ || (QString::fromStdString(lEventsfeedItem->buzzId().toHex()) != topId_ &&
				(!lEventsfeedItem->buzzers().size() ||
				 QString::fromStdString(lEventsfeedItem->beginInfo()->eventId().toHex()) != topId_))) {
			qInfo() << "[eventsfeedUpdated/1]" << lEventsfeedItem->actualTimestamp() << !suspended_;
			PushNotification::instance(
						lEventsfeedItem,
						unMarkdownBuzzBodyLimited(QString::fromStdString(lEventsfeedItem->buzzBodyString()), -1),
						!suspended_)->process();
		}
#endif
	}
}

// conversations updated
void Client::conversationsUpdated(const qbit::ConversationItemProxy& event) {
	//
	qbit::ConversationItemPtr lEventsfeedItem(event.get());
	QString lLastTimestamp = getProperty("Client.conversationsLastTimestamp");
	if (lLastTimestamp.length()) {
		if (lLastTimestamp.toULongLong() < lEventsfeedItem->timestamp()) {
			qInfo() << "[conversationsUpdated]" << lLastTimestamp.toULongLong() << lEventsfeedItem->timestamp();
			if (lEventsfeedItem->events().size() &&
					lEventsfeedItem->events()[0].buzzerId() != getBuzzerComposer()->buzzerId())
				emit newMessages();
			setProperty("Client.conversationsLastTimestamp", QString::number(lEventsfeedItem->timestamp()));
		}
	} else {
		qInfo() << "[conversationsUpdated]" << lLastTimestamp.toULongLong() << lEventsfeedItem->timestamp();
		if (lEventsfeedItem->events().size() &&
				lEventsfeedItem->events()[0].buzzerId() != getBuzzerComposer()->buzzerId())
			emit newMessages();
		setProperty("Client.conversationsLastTimestamp", QString::number(lEventsfeedItem->timestamp()));
	}
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
	QRegularExpression lRule = QRegularExpression(REGEX_UNIVERSAL_URL_PATTERN);
	QRegularExpressionMatchIterator lMatchIterator = lRule.globalMatch(lResult);
	while (lMatchIterator.hasNext()) {
		QRegularExpressionMatch lMatch = lMatchIterator.next();
		lUrl = lResult.mid(lMatch.capturedStart(), lMatch.capturedLength());
	}

	if (lUrl.size() > 3) {
		QString lLegal = lUrl.mid(0, 3);
		if (lLegal == "htt" || lLegal == "ftp") return lUrl;
	}

	return QString();
}

// TODO: just keep it
// static const QString gEmojiPatternString(QStringLiteral("\U0001F3F4\U000E0067\U000E0062(?:\U000E0077\U000E006C\U000E0073|\U000E0073\U000E0063\U000E0074|\U000E0065\U000E006E\U000E0067)\U000E007F|\U0001F469\u200D\U0001F469\u200D(?:\U0001F466\u200D\U0001F466|\U0001F467\u200D[\U0001F466\U0001F467])|\U0001F468(?:\U0001F3FF\u200D(?:\U0001F91D\u200D\U0001F468[\U0001F3FB-\U0001F3FE]|[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD])|\U0001F3FE\u200D(?:\U0001F91D\u200D\U0001F468[\U0001F3FB-\U0001F3FD\U0001F3FF]|[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD])|\U0001F3FD\u200D(?:\U0001F91D\u200D\U0001F468[\U0001F3FB\U0001F3FC\U0001F3FE\U0001F3FF]|[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD])|\U0001F3FC\u200D(?:\U0001F91D\u200D\U0001F468[\U0001F3FB\U0001F3FD-\U0001F3FF]|[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD])|\U0001F3FB\u200D(?:\U0001F91D\u200D\U0001F468[\U0001F3FC-\U0001F3FF]|[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD])|\u200D(?:\u2764\uFE0F\u200D(?:\U0001F48B\u200D)?\U0001F468|[\U0001F468\U0001F469]\u200D(?:\U0001F466\u200D\U0001F466|\U0001F467\u200D[\U0001F466\U0001F467])|\U0001F466\u200D\U0001F466|\U0001F467\u200D[\U0001F466\U0001F467]|[\U0001F468\U0001F469]\u200D[\U0001F466\U0001F467]|[\u2695\u2696\u2708]\uFE0F|[\U0001F466\U0001F467]|[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD])|(?:\U0001F3FF\u200D[\u2695\u2696\u2708]|\U0001F3FE\u200D[\u2695\u2696\u2708]|\U0001F3FD\u200D[\u2695\u2696\u2708]|\U0001F3FC\u200D[\u2695\u2696\u2708]|\U0001F3FB\u200D[\u2695\u2696\u2708])\uFE0F|\U0001F3FF|\U0001F3FE|\U0001F3FD|\U0001F3FC|\U0001F3FB)?|\U0001F9D1(?:[\U0001F3FB-\U0001F3FF]\u200D(?:\U0001F91D\u200D\U0001F9D1[\U0001F3FB-\U0001F3FF]|[\U0001F33E\U0001F373\U0001F37C\U0001F384\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD])|\u200D(?:\U0001F91D\u200D\U0001F9D1|[\U0001F33E\U0001F373\U0001F37C\U0001F384\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD]))|\U0001F469(?:\u200D(?:\u2764\uFE0F\u200D(?:\U0001F48B\u200D[\U0001F468\U0001F469]|[\U0001F468\U0001F469])|[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD])|\U0001F3FF\u200D[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD]|\U0001F3FE\u200D[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD]|\U0001F3FD\u200D[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD]|\U0001F3FC\u200D[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD]|\U0001F3FB\u200D[\U0001F33E\U0001F373\U0001F37C\U0001F393\U0001F3A4\U0001F3A8\U0001F3EB\U0001F3ED\U0001F4BB\U0001F4BC\U0001F527\U0001F52C\U0001F680\U0001F692\U0001F9AF-\U0001F9B3\U0001F9BC\U0001F9BD])|\U0001F469\U0001F3FF\u200D\U0001F91D\u200D[\U0001F468\U0001F469][\U0001F3FB-\U0001F3FE]|\U0001F469\U0001F3FE\u200D\U0001F91D\u200D[\U0001F468\U0001F469][\U0001F3FB-\U0001F3FD\U0001F3FF]|\U0001F469\U0001F3FD\u200D\U0001F91D\u200D[\U0001F468\U0001F469][\U0001F3FB\U0001F3FC\U0001F3FE\U0001F3FF]|\U0001F469\U0001F3FC\u200D\U0001F91D\u200D[\U0001F468\U0001F469][\U0001F3FB\U0001F3FD-\U0001F3FF]|\U0001F469\U0001F3FB\u200D\U0001F91D\u200D[\U0001F468\U0001F469][\U0001F3FC-\U0001F3FF]|\U0001F469\u200D\U0001F466\u200D\U0001F466|\U0001F469\u200D\U0001F469\u200D[\U0001F466\U0001F467]|(?:\U0001F441\uFE0F\u200D\U0001F5E8|\U0001F469(?:\U0001F3FF\u200D[\u2695\u2696\u2708]|\U0001F3FE\u200D[\u2695\u2696\u2708]|\U0001F3FD\u200D[\u2695\u2696\u2708]|\U0001F3FC\u200D[\u2695\u2696\u2708]|\U0001F3FB\u200D[\u2695\u2696\u2708]|\u200D[\u2695\u2696\u2708])|\U0001F3F3\uFE0F\u200D\u26A7|\U0001F9D1(?:[\U0001F3FB-\U0001F3FF]\u200D[\u2695\u2696\u2708]|\u200D[\u2695\u2696\u2708])|\U0001F43B\u200D\u2744|(?:[\U0001F3C3\U0001F3C4\U0001F3CA\U0001F46E\U0001F470\U0001F471\U0001F473\U0001F477\U0001F481\U0001F482\U0001F486\U0001F487\U0001F645-\U0001F647\U0001F64B\U0001F64D\U0001F64E\U0001F6A3\U0001F6B4-\U0001F6B6\U0001F926\U0001F935\U0001F937-\U0001F939\U0001F93D\U0001F93E\U0001F9B8\U0001F9B9\U0001F9CD-\U0001F9CF\U0001F9D6-\U0001F9DD][\U0001F3FB-\U0001F3FF]|[\U0001F46F\U0001F93C\U0001F9DE\U0001F9DF])\u200D[\u2640\u2642]|[\u26F9\U0001F3CB\U0001F3CC\U0001F575][\uFE0F\U0001F3FB-\U0001F3FF]\u200D[\u2640\u2642]|\U0001F3F4\u200D\u2620|[\U0001F3C3\U0001F3C4\U0001F3CA\U0001F46E\U0001F470\U0001F471\U0001F473\U0001F477\U0001F481\U0001F482\U0001F486\U0001F487\U0001F645-\U0001F647\U0001F64B\U0001F64D\U0001F64E\U0001F6A3\U0001F6B4-\U0001F6B6\U0001F926\U0001F935\U0001F937-\U0001F939\U0001F93D\U0001F93E\U0001F9B8\U0001F9B9\U0001F9CD-\U0001F9CF\U0001F9D6-\U0001F9DD]\u200D[\u2640\u2642]|[\\xA9\\xAE\u203C\u2049\u2122\u2139\u2194-\u2199\u21A9\u21AA\u2328\u23CF\u23ED-\u23EF\u23F1\u23F2\u23F8-\u23FA\u24C2\u25AA\u25AB\u25B6\u25C0\u25FB\u25FC\u2600-\u2604\u260E\u2611\u2618\u2620\u2622\u2623\u2626\u262A\u262E\u262F\u2638-\u263A\u2640\u2642\u265F\u2660\u2663\u2665\u2666\u2668\u267B\u267E\u2692\u2694-\u2697\u2699\u269B\u269C\u26A0\u26A7\u26B0\u26B1\u26C8\u26CF\u26D1\u26D3\u26E9\u26F0\u26F1\u26F4\u26F7\u26F8\u2702\u2708\u2709\u270F\u2712\u2714\u2716\u271D\u2721\u2733\u2734\u2744\u2747\u2763\u2764\u27A1\u2934\u2935\u2B05-\u2B07\u3030\u303D\u3297\u3299\U0001F170\U0001F171\U0001F17E\U0001F17F\U0001F202\U0001F237\U0001F321\U0001F324-\U0001F32C\U0001F336\U0001F37D\U0001F396\U0001F397\U0001F399-\U0001F39B\U0001F39E\U0001F39F\U0001F3CD\U0001F3CE\U0001F3D4-\U0001F3DF\U0001F3F5\U0001F3F7\U0001F43F\U0001F4FD\U0001F549\U0001F54A\U0001F56F\U0001F570\U0001F573\U0001F576-\U0001F579\U0001F587\U0001F58A-\U0001F58D\U0001F5A5\U0001F5A8\U0001F5B1\U0001F5B2\U0001F5BC\U0001F5C2-\U0001F5C4\U0001F5D1-\U0001F5D3\U0001F5DC-\U0001F5DE\U0001F5E1\U0001F5E3\U0001F5E8\U0001F5EF\U0001F5F3\U0001F5FA\U0001F6CB\U0001F6CD-\U0001F6CF\U0001F6E0-\U0001F6E5\U0001F6E9\U0001F6F0\U0001F6F3])\uFE0F|\U0001F469\u200D\U0001F467\u200D[\U0001F466\U0001F467]|\U0001F3F3\uFE0F\u200D\U0001F308|\U0001F469\u200D\U0001F467|\U0001F469\u200D\U0001F466|\U0001F415\u200D\U0001F9BA|\U0001F469(?:\U0001F3FF|\U0001F3FE|\U0001F3FD|\U0001F3FC|\U0001F3FB)?|\U0001F1FD\U0001F1F0|\U0001F1F6\U0001F1E6|\U0001F1F4\U0001F1F2|\U0001F408\u200D\u2B1B|\U0001F441\uFE0F|\U0001F3F3\uFE0F|\U0001F9D1[\U0001F3FB-\U0001F3FF]?|\U0001F1FF[\U0001F1E6\U0001F1F2\U0001F1FC]|\U0001F1FE[\U0001F1EA\U0001F1F9]|\U0001F1FC[\U0001F1EB\U0001F1F8]|\U0001F1FB[\U0001F1E6\U0001F1E8\U0001F1EA\U0001F1EC\U0001F1EE\U0001F1F3\U0001F1FA]|\U0001F1FA[\U0001F1E6\U0001F1EC\U0001F1F2\U0001F1F3\U0001F1F8\U0001F1FE\U0001F1FF]|\U0001F1F9[\U0001F1E6\U0001F1E8\U0001F1E9\U0001F1EB-\U0001F1ED\U0001F1EF-\U0001F1F4\U0001F1F7\U0001F1F9\U0001F1FB\U0001F1FC\U0001F1FF]|\U0001F1F8[\U0001F1E6-\U0001F1EA\U0001F1EC-\U0001F1F4\U0001F1F7-\U0001F1F9\U0001F1FB\U0001F1FD-\U0001F1FF]|\U0001F1F7[\U0001F1EA\U0001F1F4\U0001F1F8\U0001F1FA\U0001F1FC]|\U0001F1F5[\U0001F1E6\U0001F1EA-\U0001F1ED\U0001F1F0-\U0001F1F3\U0001F1F7-\U0001F1F9\U0001F1FC\U0001F1FE]|\U0001F1F3[\U0001F1E6\U0001F1E8\U0001F1EA-\U0001F1EC\U0001F1EE\U0001F1F1\U0001F1F4\U0001F1F5\U0001F1F7\U0001F1FA\U0001F1FF]|\U0001F1F2[\U0001F1E6\U0001F1E8-\U0001F1ED\U0001F1F0-\U0001F1FF]|\U0001F1F1[\U0001F1E6-\U0001F1E8\U0001F1EE\U0001F1F0\U0001F1F7-\U0001F1FB\U0001F1FE]|\U0001F1F0[\U0001F1EA\U0001F1EC-\U0001F1EE\U0001F1F2\U0001F1F3\U0001F1F5\U0001F1F7\U0001F1FC\U0001F1FE\U0001F1FF]|\U0001F1EF[\U0001F1EA\U0001F1F2\U0001F1F4\U0001F1F5]|\U0001F1EE[\U0001F1E8-\U0001F1EA\U0001F1F1-\U0001F1F4\U0001F1F6-\U0001F1F9]|\U0001F1ED[\U0001F1F0\U0001F1F2\U0001F1F3\U0001F1F7\U0001F1F9\U0001F1FA]|\U0001F1EC[\U0001F1E6\U0001F1E7\U0001F1E9-\U0001F1EE\U0001F1F1-\U0001F1F3\U0001F1F5-\U0001F1FA\U0001F1FC\U0001F1FE]|\U0001F1EB[\U0001F1EE-\U0001F1F0\U0001F1F2\U0001F1F4\U0001F1F7]|\U0001F1EA[\U0001F1E6\U0001F1E8\U0001F1EA\U0001F1EC\U0001F1ED\U0001F1F7-\U0001F1FA]|\U0001F1E9[\U0001F1EA\U0001F1EC\U0001F1EF\U0001F1F0\U0001F1F2\U0001F1F4\U0001F1FF]|\U0001F1E8[\U0001F1E6\U0001F1E8\U0001F1E9\U0001F1EB-\U0001F1EE\U0001F1F0-\U0001F1F5\U0001F1F7\U0001F1FA-\U0001F1FF]|\U0001F1E7[\U0001F1E6\U0001F1E7\U0001F1E9-\U0001F1EF\U0001F1F1-\U0001F1F4\U0001F1F6-\U0001F1F9\U0001F1FB\U0001F1FC\U0001F1FE\U0001F1FF]|\U0001F1E6[\U0001F1E8-\U0001F1EC\U0001F1EE\U0001F1F1\U0001F1F2\U0001F1F4\U0001F1F6-\U0001F1FA\U0001F1FC\U0001F1FD\U0001F1FF]|[#\\*0-9]\uFE0F\u20E3|[\U0001F3C3\U0001F3C4\U0001F3CA\U0001F46E\U0001F470\U0001F471\U0001F473\U0001F477\U0001F481\U0001F482\U0001F486\U0001F487\U0001F645-\U0001F647\U0001F64B\U0001F64D\U0001F64E\U0001F6A3\U0001F6B4-\U0001F6B6\U0001F926\U0001F935\U0001F937-\U0001F939\U0001F93D\U0001F93E\U0001F9B8\U0001F9B9\U0001F9CD-\U0001F9CF\U0001F9D6-\U0001F9DD][\U0001F3FB-\U0001F3FF]|[\u26F9\U0001F3CB\U0001F3CC\U0001F575][\uFE0F\U0001F3FB-\U0001F3FF]|\U0001F3F4|[\u270A\u270B\U0001F385\U0001F3C2\U0001F3C7\U0001F442\U0001F443\U0001F446-\U0001F450\U0001F466\U0001F467\U0001F46B-\U0001F46D\U0001F472\U0001F474-\U0001F476\U0001F478\U0001F47C\U0001F483\U0001F485\U0001F4AA\U0001F57A\U0001F595\U0001F596\U0001F64C\U0001F64F\U0001F6C0\U0001F6CC\U0001F90C\U0001F90F\U0001F918-\U0001F91C\U0001F91E\U0001F91F\U0001F930-\U0001F934\U0001F936\U0001F977\U0001F9B5\U0001F9B6\U0001F9BB\U0001F9D2-\U0001F9D5][\U0001F3FB-\U0001F3FF]|[\u261D\u270C\u270D\U0001F574\U0001F590][\uFE0F\U0001F3FB-\U0001F3FF]|[\u270A\u270B\U0001F385\U0001F3C2\U0001F3C7\U0001F408\U0001F415\U0001F43B\U0001F442\U0001F443\U0001F446-\U0001F450\U0001F466\U0001F467\U0001F46B-\U0001F46D\U0001F472\U0001F474-\U0001F476\U0001F478\U0001F47C\U0001F483\U0001F485\U0001F4AA\U0001F57A\U0001F595\U0001F596\U0001F64C\U0001F64F\U0001F6C0\U0001F6CC\U0001F90C\U0001F90F\U0001F918-\U0001F91C\U0001F91E\U0001F91F\U0001F930-\U0001F934\U0001F936\U0001F977\U0001F9B5\U0001F9B6\U0001F9BB\U0001F9D2-\U0001F9D5]|[\U0001F3C3\U0001F3C4\U0001F3CA\U0001F46E\U0001F470\U0001F471\U0001F473\U0001F477\U0001F481\U0001F482\U0001F486\U0001F487\U0001F645-\U0001F647\U0001F64B\U0001F64D\U0001F64E\U0001F6A3\U0001F6B4-\U0001F6B6\U0001F926\U0001F935\U0001F937-\U0001F939\U0001F93D\U0001F93E\U0001F9B8\U0001F9B9\U0001F9CD-\U0001F9CF\U0001F9D6-\U0001F9DD]|[\U0001F46F\U0001F93C\U0001F9DE\U0001F9DF]|[\u231A\u231B\u23E9-\u23EC\u23F0\u23F3\u25FD\u25FE\u2614\u2615\u2648-\u2653\u267F\u2693\u26A1\u26AA\u26AB\u26BD\u26BE\u26C4\u26C5\u26CE\u26D4\u26EA\u26F2\u26F3\u26F5\u26FA\u26FD\u2705\u2728\u274C\u274E\u2753-\u2755\u2757\u2795-\u2797\u27B0\u27BF\u2B1B\u2B1C\u2B50\u2B55\U0001F004\U0001F0CF\U0001F18E\U0001F191-\U0001F19A\U0001F201\U0001F21A\U0001F22F\U0001F232-\U0001F236\U0001F238-\U0001F23A\U0001F250\U0001F251\U0001F300-\U0001F320\U0001F32D-\U0001F335\U0001F337-\U0001F37C\U0001F37E-\U0001F384\U0001F386-\U0001F393\U0001F3A0-\U0001F3C1\U0001F3C5\U0001F3C6\U0001F3C8\U0001F3C9\U0001F3CF-\U0001F3D3\U0001F3E0-\U0001F3F0\U0001F3F8-\U0001F407\U0001F409-\U0001F414\U0001F416-\U0001F43A\U0001F43C-\U0001F43E\U0001F440\U0001F444\U0001F445\U0001F451-\U0001F465\U0001F46A\U0001F479-\U0001F47B\U0001F47D-\U0001F480\U0001F484\U0001F488-\U0001F4A9\U0001F4AB-\U0001F4FC\U0001F4FF-\U0001F53D\U0001F54B-\U0001F54E\U0001F550-\U0001F567\U0001F5A4\U0001F5FB-\U0001F644\U0001F648-\U0001F64A\U0001F680-\U0001F6A2\U0001F6A4-\U0001F6B3\U0001F6B7-\U0001F6BF\U0001F6C1-\U0001F6C5\U0001F6D0-\U0001F6D2\U0001F6D5-\U0001F6D7\U0001F6EB\U0001F6EC\U0001F6F4-\U0001F6FC\U0001F7E0-\U0001F7EB\U0001F90D\U0001F90E\U0001F910-\U0001F917\U0001F91D\U0001F920-\U0001F925\U0001F927-\U0001F92F\U0001F93A\U0001F93F-\U0001F945\U0001F947-\U0001F976\U0001F978\U0001F97A-\U0001F9B4\U0001F9B7\U0001F9BA\U0001F9BC-\U0001F9CB\U0001F9D0\U0001F9E0-\U0001F9FF\U0001FA70-\U0001FA74\U0001FA78-\U0001FA7A\U0001FA80-\U0001FA86\U0001FA90-\U0001FAA8\U0001FAB0-\U0001FAB6\U0001FAC0-\U0001FAC2\U0001FAD0-\U0001FAD6]|[\u231A\u231B\u23E9-\u23EC\u23F0\u23F3\u25FD\u25FE\u2614\u2615\u2648-\u2653\u267F\u2693\u26A1\u26AA\u26AB\u26BD\u26BE\u26C4\u26C5\u26CE\u26D4\u26EA\u26F2\u26F3\u26F5\u26FA\u26FD\u2705\u270A\u270B\u2728\u274C\u274E\u2753-\u2755\u2757\u2795-\u2797\u27B0\u27BF\u2B1B\u2B1C\u2B50\u2B55\U0001F004\U0001F0CF\U0001F18E\U0001F191-\U0001F19A\U0001F1E6-\U0001F1FF\U0001F201\U0001F21A\U0001F22F\U0001F232-\U0001F236\U0001F238-\U0001F23A\U0001F250\U0001F251\U0001F300-\U0001F320\U0001F32D-\U0001F335\U0001F337-\U0001F37C\U0001F37E-\U0001F393\U0001F3A0-\U0001F3CA\U0001F3CF-\U0001F3D3\U0001F3E0-\U0001F3F0\U0001F3F4\U0001F3F8-\U0001F43E\U0001F440\U0001F442-\U0001F4FC\U0001F4FF-\U0001F53D\U0001F54B-\U0001F54E\U0001F550-\U0001F567\U0001F57A\U0001F595\U0001F596\U0001F5A4\U0001F5FB-\U0001F64F\U0001F680-\U0001F6C5\U0001F6CC\U0001F6D0-\U0001F6D2\U0001F6D5-\U0001F6D7\U0001F6EB\U0001F6EC\U0001F6F4-\U0001F6FC\U0001F7E0-\U0001F7EB\U0001F90C-\U0001F93A\U0001F93C-\U0001F945\U0001F947-\U0001F978\U0001F97A-\U0001F9CB\U0001F9CD-\U0001F9FF\U0001FA70-\U0001FA74\U0001FA78-\U0001FA7A\U0001FA80-\U0001FA86\U0001FA90-\U0001FAA8\U0001FAB0-\U0001FAB6\U0001FAC0-\U0001FAC2\U0001FAD0-\U0001FAD6]|[#\\*0-9\\xA9\\xAE\u203C\u2049\u2122\u2139\u2194-\u2199\u21A9\u21AA\u231A\u231B\u2328\u23CF\u23E9-\u23F3\u23F8-\u23FA\u24C2\u25AA\u25AB\u25B6\u25C0\u25FB-\u25FE\u2600-\u2604\u260E\u2611\u2614\u2615\u2618\u261D\u2620\u2622\u2623\u2626\u262A\u262E\u262F\u2638-\u263A\u2640\u2642\u2648-\u2653\u265F\u2660\u2663\u2665\u2666\u2668\u267B\u267E\u267F\u2692-\u2697\u2699\u269B\u269C\u26A0\u26A1\u26A7\u26AA\u26AB\u26B0\u26B1\u26BD\u26BE\u26C4\u26C5\u26C8\u26CE\u26CF\u26D1\u26D3\u26D4\u26E9\u26EA\u26F0-\u26F5\u26F7-\u26FA\u26FD\u2702\u2705\u2708-\u270D\u270F\u2712\u2714\u2716\u271D\u2721\u2728\u2733\u2734\u2744\u2747\u274C\u274E\u2753-\u2755\u2757\u2763\u2764\u2795-\u2797\u27A1\u27B0\u27BF\u2934\u2935\u2B05-\u2B07\u2B1B\u2B1C\u2B50\u2B55\u3030\u303D\u3297\u3299\U0001F004\U0001F0CF\U0001F170\U0001F171\U0001F17E\U0001F17F\U0001F18E\U0001F191-\U0001F19A\U0001F1E6-\U0001F1FF\U0001F201\U0001F202\U0001F21A\U0001F22F\U0001F232-\U0001F23A\U0001F250\U0001F251\U0001F300-\U0001F321\U0001F324-\U0001F393\U0001F396\U0001F397\U0001F399-\U0001F39B\U0001F39E-\U0001F3F0\U0001F3F3-\U0001F3F5\U0001F3F7-\U0001F4FD\U0001F4FF-\U0001F53D\U0001F549-\U0001F54E\U0001F550-\U0001F567\U0001F56F\U0001F570\U0001F573-\U0001F57A\U0001F587\U0001F58A-\U0001F58D\U0001F590\U0001F595\U0001F596\U0001F5A4\U0001F5A5\U0001F5A8\U0001F5B1\U0001F5B2\U0001F5BC\U0001F5C2-\U0001F5C4\U0001F5D1-\U0001F5D3\U0001F5DC-\U0001F5DE\U0001F5E1\U0001F5E3\U0001F5E8\U0001F5EF\U0001F5F3\U0001F5FA-\U0001F64F\U0001F680-\U0001F6C5\U0001F6CB-\U0001F6D2\U0001F6D5-\U0001F6D7\U0001F6E0-\U0001F6E5\U0001F6E9\U0001F6EB\U0001F6EC\U0001F6F0\U0001F6F3-\U0001F6FC\U0001F7E0-\U0001F7EB\U0001F90C-\U0001F93A\U0001F93C-\U0001F945\U0001F947-\U0001F978\U0001F97A-\U0001F9CB\U0001F9CD-\U0001F9FF\U0001FA70-\U0001FA74\U0001FA78-\U0001FA7A\U0001FA80-\U0001FA86\U0001FA90-\U0001FAA8\U0001FAB0-\U0001FAB6\U0001FAC0-\U0001FAC2\U0001FAD0-\U0001FAD6]\uFE0F|[\u261D\u26F9\u270A-\u270D\U0001F385\U0001F3C2-\U0001F3C4\U0001F3C7\U0001F3CA-\U0001F3CC\U0001F442\U0001F443\U0001F446-\U0001F450\U0001F466-\U0001F478\U0001F47C\U0001F481-\U0001F483\U0001F485-\U0001F487\U0001F48F\U0001F491\U0001F4AA\U0001F574\U0001F575\U0001F57A\U0001F590\U0001F595\U0001F596\U0001F645-\U0001F647\U0001F64B-\U0001F64F\U0001F6A3\U0001F6B4-\U0001F6B6\U0001F6C0\U0001F6CC\U0001F90C\U0001F90F\U0001F918-\U0001F91F\U0001F926\U0001F930-\U0001F939\U0001F93C-\U0001F93E\U0001F977\U0001F9B5\U0001F9B6\U0001F9B8\U0001F9B9\U0001F9BB\U0001F9CD-\U0001F9CF\U0001F9D1-\U0001F9DD]"));

void Client::processDecoration(QVector<DecorationRule>& rules, const QString& body, int limit, bool simplify, QString& result) {
	//
	QString lResult = body;

	//
	if (limit != -1) {
		//

		for (const DecorationRule &lRule : qAsConst(rules)) {
			QRegularExpressionMatchIterator lMatchIterator = lRule.expression.globalMatch(lResult);
			while (lMatchIterator.hasNext()) {
				QRegularExpressionMatch lMatch = lMatchIterator.next();
				if (limit != -1 && limit >= lMatch.capturedStart() && limit <= lMatch.capturedStart() + lMatch.capturedLength()) {
					limit = lMatch.capturedStart() + lMatch.capturedLength();
				}
			}
		}

		lResult = body.mid(0, limit);
	}

	//
	int lPrevMatch = 0;
	bool lHasMatches = false;

	for (const DecorationRule &lRule : qAsConst(rules)) {
		//
		result.clear();

		//
		QRegularExpressionMatchIterator lMatchIterator = lRule.expression.globalMatch(lResult);
		while (lMatchIterator.hasNext()) {
			//
			//qInfo() << "-----------";
			QRegularExpressionMatch lMatch = lMatchIterator.next();
			QVector<_Arg> lArgs;
			for (int lArg = 0; lArg <= lMatch.lastCapturedIndex(); lArg++) {
				lArgs.push_back(_Arg { lMatch.capturedStart(lArg), lMatch.capturedLength(lArg), lMatch.captured(lArg) });
				//qInfo() << "Captured:" << lMatch.captured(lArg) << lMatch.capturedStart(lArg) << lMatch.capturedLength(lArg);
				//qInfo() << lRule.pattern;
			}

			//
			QString lUrlPatternDest = lRule.pattern;
			//
			QString lUrl = (lArgs.size() > lRule.arg0 ? lArgs[lRule.arg0].text : "");
			lUrlPatternDest.replace("\\1", lUrl);
			//
			lUrl = (lArgs.size() > lRule.arg1 ? lArgs[lRule.arg1].text : "");
			if (lRule.truncate && lUrl.length() > 25) {
				lUrl.truncate(25); lUrl += "...";
			}

			lUrlPatternDest.replace("\\2", lUrl);

			if (lMatch.capturedStart() >= lPrevMatch) {
				lHasMatches = true;
				result += lResult.mid(lPrevMatch, lMatch.capturedStart() - lPrevMatch);
				result += lUrlPatternDest;
				lPrevMatch = lMatch.capturedStart() + lMatch.capturedLength();
			}

			if (limit != -1 && limit >= lMatch.capturedStart() && limit <= lMatch.capturedStart() + lMatch.capturedLength()) {
				limit = lMatch.capturedStart() + lMatch.capturedLength();
			}
		}

		// the rest
		if (!lHasMatches) result = lResult;
		else if (lResult.size()) {
			result += lResult.mid(lPrevMatch, lResult.size()-1);
		}

		lPrevMatch = 0;
		lHasMatches = false;

		lResult = result;
	}

	if (result.size() && result[result.size()-1] == 0x0a) result[result.size()-1] = 0x20;
	if (!simplify) result.replace(QRegExp("\n"), QString("<br>"));

	if (limit > 0 && body.length() > limit && !simplify) {
		//
		result += QString("<a style='text-decoration:none;color:") +
				gApplication->getColor(theme(), themeSelector(), "Material.link.rgb") +
				QString("'>...</a>");
	}
}

QString Client::decorateBuzzBodyLimited(const QString& body, int limit) {
	//
	// already decorated
	if (body.indexOf("<a") != -1) return body;

	QString lCommonResult;

	QVector<DecorationRule> lDecorationRules;

	DecorationRule lRule;
	lRule.pattern = QString("<a href='\\2' style='text-decoration:none;color:") +
			gApplication->getColor(theme(), themeSelector(), "Material.link.rgb") +
			QString("'>\\1</a>");
	lRule.expression = QRegularExpression(REGEX_EMBEDDED_LINK);
	lRule.truncate = false;
	lRule.arg0 = 2;
	lRule.arg1 = 3;
	lDecorationRules.append(lRule);

	lRule.pattern = QString("<a href='\\1' style='text-decoration:none;color:") +
			gApplication->getColor(theme(), themeSelector(), "Material.link.rgb") +
			QString("'>\\2</a>");
	lRule.expression = QRegularExpression(REGEX_EMBEDDED_URL);
	lRule.truncate = true;
	lRule.arg0 = 2;
	lRule.arg1 = 2;
	lDecorationRules.append(lRule);

	lRule.pattern = QString("<a href='mailto:\\1' style='text-decoration:none;color:") +
			gApplication->getColor(theme(), themeSelector(), "Material.link.rgb") +
			QString("'>\\2</a>");
	lRule.expression = QRegularExpression(QString("(\\w*(?<!\\:)") + QString("([a-zA-Z0-9._-]+@[a-zA-Z0-9._-]+\\.[a-zA-Z0-9_-]+)") + QString(REGEX_EMBEDDED_URL_STOP)); // user@mail.com
	lRule.truncate = false;
	lRule.arg0 = 0;
	lRule.arg1 = 0;
	lDecorationRules.append(lRule);

	lRule.pattern = QString("<a href='\\1' style='text-decoration:none;color:") +
			gApplication->getColor(theme(), themeSelector(), "Material.link.rgb") +
			QString("'>\\2</a>");
	lRule.expression = QRegularExpression("(?<=\\s|^)(#[\\w\u0400-\u04FF]+)"); // #tags
	lRule.truncate = false;
	lRule.arg0 = 0;
	lRule.arg1 = 0;
	lDecorationRules.append(lRule);

	lRule.pattern = QString("<a href='\\1' style='text-decoration:none;color:") +
			gApplication->getColor(theme(), themeSelector(), "Material.link.rgb") +
			QString("'>\\2</a>");
	lRule.expression = QRegularExpression("(?<=\\s|^)(@[A-Za-z0-9]+)"); // @buzzers
	lRule.truncate = false;
	lRule.arg0 = 0;
	lRule.arg1 = 0;
	lDecorationRules.append(lRule);

	lRule.pattern = QString("<b>\\1</b>");
	lRule.expression = QRegularExpression("(\\*\\*((.|\\n\\r|\\n|\\r)*?)\\*\\*)"); // **bold**
	lRule.truncate = false;
	lRule.arg0 = 2;
	lRule.arg1 = 2;
	lDecorationRules.append(lRule);

	lRule.pattern = QString("<i>\\1</i>");
	lRule.expression = QRegularExpression("(__((.|\\n\\r|\\n|\\r)*?)__)"); // __italic__
	lRule.truncate = false;
	lRule.arg0 = 2;
	lRule.arg1 = 2;
	lDecorationRules.append(lRule);

	lRule.pattern = QString("<code>\\1</code>");
	lRule.expression = QRegularExpression("(`((.|\\n\\r|\\n|\\r)*?)`)"); // code with monotype
	lRule.truncate = false;
	lRule.arg0 = 2;
	lRule.arg1 = 2;
	lDecorationRules.append(lRule);

	processDecoration(lDecorationRules, body, limit, false, lCommonResult);

	return lCommonResult;
}

QString Client::unMarkdownBuzzBodyLimited(const QString& body, int limit) {
	//
	QString lResult = body;
	QString lCommonResult;

	QVector<DecorationRule> lDecorationRules;

	DecorationRule lRule;
	lRule.pattern = QString("\\1");
	lRule.expression = QRegularExpression(REGEX_EMBEDDED_LINK); // [word|link]
	lRule.truncate = false;
	lRule.arg0 = 2;
	lRule.arg1 = 3;
	lDecorationRules.append(lRule);

	lRule.pattern = QString("\\1");
	lRule.expression = QRegularExpression("(\\*\\*((.|\\n\\r|\\n|\\r)*?)\\*\\*)"); // **bold**
	lRule.truncate = false;
	lRule.arg0 = 2;
	lRule.arg1 = 2;
	lDecorationRules.append(lRule);

	lRule.pattern = QString("\\1");
	lRule.expression = QRegularExpression("(__((.|\\n\\r|\\n|\\r)*?)__)"); // __italic__
	lRule.truncate = false;
	lRule.arg0 = 2;
	lRule.arg1 = 2;
	lDecorationRules.append(lRule);

	processDecoration(lDecorationRules, body, limit, true, lCommonResult);

	return lCommonResult;
}

QString Client::decorateBuzzBody(const QString& body) {
	return decorateBuzzBodyLimited(body, -1);
}

bool Client::isEmoji(const QString& body) {
	//
	bool lEmojiString = true;
	int lCount = 0;
	std::string lBody = body.toStdString();
	for (size_t lIdx = 0; lIdx < lBody.size() && lCount < 30; ) {
		//
		if (lIdx + 1 < lBody.size() && (unsigned char)(lBody.c_str()[lIdx]) == 0xF0 && (unsigned char)(lBody.c_str()[lIdx+1]) == 0x9F) {
			lCount++; lIdx += 4;
		} else if (lBody.c_str()[lIdx] == 0x20 || lBody.c_str()[lIdx] == 0x0a) {
			lIdx++;
		} else { lEmojiString = false; break; }
	}

	return lEmojiString;
}

QString Client::statusBarTheme() {
	//
	return gApplication->getColor(theme(), themeSelector(), "StatusBar.theme");
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
	//qInfo() << "peerPushed" << QString::fromStdString(peer->key()) << update << count;

	//
	if (count == 1) {
		setNetworkReady(true);
		emit networkLimited();
	} else {
		setNetworkReady(true);
		emit networkReady();
	}

	if (BUZZFEED_PEERS_CONFIRMATIONS > count) G_BUZZFEED_PEERS_CONFIRMATIONS = count;
	else G_BUZZFEED_PEERS_CONFIRMATIONS = BUZZFEED_PEERS_CONFIRMATIONS;

	if (BUZZ_PEERS_CONFIRMATIONS > count) G_BUZZ_PEERS_CONFIRMATIONS = count;
	else G_BUZZ_PEERS_CONFIRMATIONS = BUZZ_PEERS_CONFIRMATIONS;

	//if ((!update && gApplication->isDesktop()) ||
	//		!gApplication->isDesktop())
	setBuzzerDAppReady();

	if (!suspended_ && peersModelUpdates_)
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

	qInfo() << "peerPopped" << QString::fromStdString(peer->key()) << count;

	setBuzzerDAppReady();

	if (!suspended_ && peersModelUpdates_)
		emit peerPoppedSignal(PeerProxy(peer), count);
}

void Client::prepareCache() {
	//
	if (wallet_->prepareCache()) {
		if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[Client::prepareCache]: cache is ready"));
		syncTimer_->stop();
		setCacheReady();
		//setBuzzerDAppReady();
	}
}

void Client::suspend() {
	//
	suspended_ = true;
	//
	if (gApplication->isDesktop()) return;

	//
	if (peerManager_ && !gApplication->isWakeLocked()) {
		qInfo() << "Suspending PeerManager...";
		peerManager_->suspend();
		//
		emit buzzerDAppSuspended();
	} else {
		qInfo() << "PeerManager continue to do the work in case of CPU is locked to process...";
	}
}

void Client::resume() {
	//
	if (!opened_) return;
	//
	suspended_ = false;
	//
	if (gApplication->isDesktop()) return;

	buzzerDAppReady_ = false;
	recallWallet_ = true;

	//
	if (peerManager_) {
		qInfo() << "Resuming PeerManager...";
		peerManager_->resume();
	}
}

void Client::setBuzzerDAppReady() {
	//
	if (!gApplication->isDesktop() && suspended_) return;

	//
	std::map<uint256 /*chain*/, std::multimap<uint32_t /*latency*/, IPeerPtr>> lMap;
	requestProcessor_->collectPeersByDApp("buzzer", lMap);
	//
	std::set<uint160> lPeersControl;
	for (std::map<uint256, std::multimap<uint32_t, IPeerPtr>>::iterator lItem = lMap.begin(); lItem != lMap.end(); lItem++) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lPeers;
		requestProcessor_->collectPeersByChain(lItem->first, lPeers);
		//
		for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::reverse_iterator lPeer = lPeers.rbegin(); lPeer != lPeers.rend(); lPeer++) {
			lPeersControl.insert(lPeer->second->addressId());
		}
	}

	//
	std::map<uint256 /*chain*/, std::multimap<uint32_t /*latency*/, IPeerPtr>> lMap2;
	requestProcessor_->collectPeersByDApp("cubix", lMap2);
	//
	std::set<uint160> lPeersControl2;
	for (std::map<uint256, std::multimap<uint32_t, IPeerPtr>>::iterator lItem = lMap2.begin(); lItem != lMap2.end(); lItem++) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lPeers;
		requestProcessor_->collectPeersByChain(lItem->first, lPeers);
		//
		for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::reverse_iterator lPeer = lPeers.rbegin(); lPeer != lPeers.rend(); lPeer++) {
			lPeersControl2.insert(lPeer->second->addressId());
		}
	}

	// if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[Client::setBuzzerDAppReady]: peers %d, %d - confirmations = %d", lPeersControl.size(), lPeersControl2.size(), G_BUZZFEED_PEERS_CONFIRMATIONS));

	// we have enough nodes to start servicing
	if ((double)lPeersControl.size() / (double)G_BUZZ_PEERS_CONFIRMATIONS > 1.0 &&
		(double)lPeersControl2.size() / (double)G_BUZZ_PEERS_CONFIRMATIONS > 1.0) {
		//
		bool lNotify = !buzzerDAppReady_;
		buzzerDAppReady_ = true;
		if (lNotify) {
			if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[Client::setBuzzerDAppReady]: buzzer is ready"));
			if (recallWallet_) {
				if (wallet_->prepareCache()) {
					recallWallet_ = false;
					if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[Client::setBuzzerDAppReady]: cache is ready"));
					emit buzzerDAppResumed();
				} else {
					emit fireResyncWalletCache();
				}
			}

			opened_ = true; // allways
			emit buzzerDAppReadyChanged();
		} /*else if (recallWallet_) {
			if (wallet_->prepareCache()) {
				recallWallet_ = false;
				if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[Client::setBuzzerDAppReady]: cache is ready"));
				emit buzzerDAppResumed();
			}
		}*/
	} else {
		bool lNotify = buzzerDAppReady_;
		buzzerDAppReady_ = false;
		if (lNotify) {
			emit buzzerDAppReadyChanged();
			if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, std::string("[Client::setBuzzerDAppReady]: buzzer is NOT ready"));
		}
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

		buzzerDAppReady_ = false;
		recallWallet_ = true;
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
#if defined(DESKTOP_PLATFORM)
		setScale(QString::fromStdString(back_["scale"].getString()));
#endif
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
#if defined(DESKTOP_PLATFORM)
	setScale(QString::fromStdString(root["scale"].getString()));
#endif

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
#if defined(DESKTOP_PLATFORM)
	root.addString("scale", scale().toStdString());
#endif

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

void Client::extractFavoriteEmojis(std::vector<std::string>& fav) {
	//
	for(std::map<std::string, std::string>::iterator lProp = properties_.begin(); lProp != properties_.end(); lProp++) {
		if (lProp->first.find("emoji.") == 0) {
			std::vector<std::string> lParts;
			boost::split(lParts, lProp->first, boost::is_any_of("."), boost::token_compress_on);

			if (lParts.size() == 2) {
				fav.push_back(lParts[1]);
			}
		}
	}
}

void Client::setFavEmoji(QString emoji) {
	//
	setProperty(QString("emoji.") + emoji, "true");
#if defined(DESKTOP_PLATFORM)
	emojiData_->updateFavEmojis();
#endif
}

void Client::setAvatar(const QString& avatar) {
#ifdef Q_OS_WINDOWS
	QString lAvatar = avatar;
	//lAvatar.replace(0, 1, lAvatar[0].toLower());
	avatar_ = lAvatar[0] != "/" ? ("/" + lAvatar) : lAvatar;
#else
	avatar_ = avatar;
#endif
	emit avatarChanged();
}

void Client::setHeader(const QString& header) {
#ifdef Q_OS_WINDOWS
	QString lHeader = header;
	//lHeader.replace(0, 1, lHeader[0].toLower());
	header_ = lHeader[0] != "/" ? ("/" + lHeader) : lHeader;
#else
	header_ = header;
#endif
	emit headerChanged(); 
}
