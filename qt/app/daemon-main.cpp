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
#include "../../client/commandshandler.h"
#include "../../client/commands.h"

#include "../txbase.h"
#include "../txblockbase.h"

#if defined (BUZZER_MOD)
	#include "../dapps/buzzer/txbuzzer.h"
	#include "../dapps/buzzer/txbuzz.h"
	#include "../dapps/buzzer/txbuzzlike.h"
	#include "../dapps/buzzer/txbuzzreward.h"
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
	#include "../dapps/buzzer/conversationsfeed.h"
	#include "../../client/dapps/buzzer/buzzerrequestprocessor.h"
	#include "../../client/dapps/buzzer/buzzercomposer.h"
	#include "../../client/dapps/buzzer/buzzercommands.h"
#endif

#if defined (CUBIX_MOD)
	#include "../dapps/cubix/cubix.h"
	#include "../dapps/cubix/txmediaheader.h"
	#include "../dapps/cubix/txmediadata.h"
	#include "../dapps/cubix/txmediasummary.h"
	#include "../dapps/cubix/peerextension.h"
	#include "../../client/dapps/cubix/cubixcomposer.h"
	#include "../../client/dapps/cubix/cubixcommands.h"
#endif

#ifdef Q_OS_ANDROID
#include <QAndroidService>
#include <QAndroidJniObject>
#include <QAndroidIntent>
#include <QAndroidBinder>
#include <QtAndroid>
#endif

#include "settings.h"
#include "applicationpath.h"
#include "error.h"
#include "client.h"

#include "applicationpath.h"

using namespace qbit;

qbit::json::Document gAppConfig;
const char APP_NAME[] = { "buzzer-app" };

cubix::CubixLightComposerPtr gCubixComposer;
BuzzerLightComposerPtr gBuzzerComposer;
BuzzerPtr gBuzzer;

std::string getColor(const std::string& theme, const std::string& selector, const std::string& key) {
	qbit::json::Value lThemes = gAppConfig["themes"];
	qbit::json::Value lTheme = lThemes[theme];
	qbit::json::Value lSelector = lTheme[selector];
	qbit::json::Value lValue = lSelector[key];

	return lValue.getString();
}

QString getLocalization(const QString& locale, const std::string& key) {
	json::Value lLocalization = gAppConfig["localization"];
	json::Value lLocale = lLocalization[locale.toStdString()];
	json::Value lValue = lLocale[key];
	return QString::fromStdString(lValue.getString());
}

//
// client
//
class DaemonClient: public buzzer::IClient {
public:
	DaemonClient() {}

	void setLocale(const QString& locale) { locale_ = locale; }
	QString locale() const { return locale_; }

	void setTheme(const QString& theme) { theme_ = theme; }
	QString theme() const { return theme_; }

	void setThemeSelector(const QString& themeSelector) { themeSelector_ = themeSelector; }
	QString themeSelector() const { return themeSelector_; }

	void settingsFromJSON(qbit::json::Value& root) {
		setLocale(QString::fromStdString(root["locale"].getString()));
		setTheme(QString::fromStdString(root["theme"].getString()));
		setThemeSelector(QString::fromStdString(root["themeSelector"].getString()));
	}

	void settingsToJSON(qbit::json::Value&) {
	}

private:
	QString locale_;
	QString theme_;
	QString themeSelector_;
};

DaemonClient* gClient;

//
// simple hash-id
//

#define A 1 /* a prime */
#define B 3 /* another prime */
#define C 5 /* yet another prime */
#define FIRSTH 7 /* also prime */

int hash_str(const char* s) {
   int h = FIRSTH;
   while (*s) {
	 h = (h * A) ^ (s[0] * B);
	 s++;
   }

   return h; // or return h % C;
}

//
// notification queue
//

class PushNotification;
typedef std::shared_ptr<PushNotification> PushNotificationPtr;
std::map<EventsfeedItem::Key, PushNotificationPtr> gQueue;

//
// notification item
//

class PushNotification {
public:
	PushNotification(EventsfeedItemPtr buzz) : buzz_(buzz) {}

	static PushNotificationPtr instance(EventsfeedItemPtr buzz) {
		return std::make_shared<PushNotification>(buzz);
	}

	void downloadProgress(uint64_t, uint64_t) {
		//
	}

	QString getId() {
		// unsigned
		return QString::number(hash_str(buzz_->buzzId().toHex().c_str()));
	}

	QString getAlias() {
		//
		uint256 lInfoId;
		if (buzz_->buzzers().size()) {
			lInfoId = buzz_->buzzers()[0].buzzerInfoId();
		} else {
			lInfoId = buzz_->publisherInfo();
		}

		//
		TxBuzzerInfoPtr lInfo = gBuzzer->locateBuzzerInfo(lInfoId);
		if (lInfo) {
			std::string lAlias(lInfo->alias().begin(), lInfo->alias().end());
			return QString::fromStdString(lAlias);
		}

		return QString();
	}

	QString getBuzzer() {
		//
		uint256 lInfoId;
		if (buzz_->buzzers().size()) {
			lInfoId = buzz_->buzzers()[0].buzzerInfoId();
		} else {
			lInfoId = buzz_->publisherInfo();
		}

		//
		TxBuzzerInfoPtr lInfo = gBuzzer->locateBuzzerInfo(lInfoId);
		if (lInfo) {
			return QString::fromStdString(lInfo->myName());
		}

		return QString();
	}

	QString getComment() {
		//
		if (buzz_->type() == TX_BUZZ_LIKE) {
			if (buzz_->buzzers().size() > 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.like.many");
				return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
			} else if (buzz_->buzzers().size() == 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.like.one");
				return lString;
			}
		} else if (buzz_->type() == TX_REBUZZ) {
			if (buzz_->buzzers().size() > 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.rebuzz.many");
				return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
			} else if (buzz_->buzzers().size() == 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.rebuzz.one");
				return lString;
			}
		} else if (buzz_->type() == TX_BUZZ_REWARD) {
			if (buzz_->buzzers().size() > 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.reward.many");
				return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
			} else if (buzz_->buzzers().size() == 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.reward.one");
				return lString;
			}
		} else if (buzz_->type() == TX_BUZZER_MISTRUST) {
			if (buzz_->buzzers().size() > 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.mistrust.many");
				return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
			} else if (buzz_->buzzers().size() == 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.mistrust.one");
				return lString;
			}
		} else if (buzz_->type() == TX_BUZZER_ENDORSE) {
			if (buzz_->buzzers().size() > 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.endorse.many");
				return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
			} else if (buzz_->buzzers().size() == 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.endorse.one");
				return lString;
			}
		} else if (buzz_->type() == TX_BUZZ_REPLY) {
			if (buzz_->buzzers().size() > 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.reply.many");
				return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
			} else if (buzz_->buzzers().size() == 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.reply.one");
				return lString;
			}
		} else if (buzz_->type() == TX_REBUZZ_REPLY) {
			if (buzz_->buzzers().size() > 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.rebuzz.many");
				return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
			} else if (buzz_->buzzers().size() == 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.rebuzz.one");
				return lString;
			}
		} else if (buzz_->type() == TX_BUZZER_SUBSCRIBE) {
			if (buzz_->buzzers().size() > 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.subscribe.many");
				return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
			} else if (buzz_->buzzers().size() == 1) {
				QString lString = getLocalization(gClient->locale(), "Buzzer.event.subscribe.one");
				return lString;
			}
		} else if (buzz_->type() == TX_BUZZER_CONVERSATION) {
			QString lString = getLocalization(gClient->locale(), "Buzzer.event.conversation.one");
			return lString;
		} else if (buzz_->type() == TX_BUZZER_ACCEPT_CONVERSATION) {
			QString lString = getLocalization(gClient->locale(), "Buzzer.event.conversation.accepted.one");
			return lString;
		} else if (buzz_->type() == TX_BUZZER_DECLINE_CONVERSATION) {
			QString lString = getLocalization(gClient->locale(), "Buzzer.event.conversation.declined.one");
			return lString;
		} else if (buzz_->type() == TX_BUZZER_MESSAGE || buzz_->type() == TX_BUZZER_MESSAGE_REPLY) {
			QString lString = getLocalization(gClient->locale(), "Buzzer.event.conversation.message.one");
			return lString;
		}
	}

	QString getText() {
		if (buzz_->type() == TX_BUZZER_MESSAGE || buzz_->type() == TX_BUZZER_MESSAGE_REPLY) {
			QString lString = getLocalization(gClient->locale(), "Buzzer.event.conversation.message.encrypted");
			return lString;
		}

		return QString::fromStdString(buzz_->buzzBodyString());
	}

	void makeNotification() {
		//
#ifdef Q_OS_ANDROID
		jint lType = buzz_->type();
		QAndroidJniObject lId = QAndroidJniObject::fromString(getId());
		QAndroidJniObject lAlias = QAndroidJniObject::fromString(getAlias());
		QAndroidJniObject lName = QAndroidJniObject::fromString(getBuzzer());
		QAndroidJniObject lComment = QAndroidJniObject::fromString(getComment());
		QAndroidJniObject lText = QAndroidJniObject::fromString(getText());
		QAndroidJniObject lAvatarPath = QAndroidJniObject::fromString(QString::fromStdString(avatarFile_));
		QAndroidJniObject lMediaPath = QAndroidJniObject::fromString(QString::fromStdString(mediaFile_));

		qInfo() << "[buzzer-daemon]: Pushing notification";

		QAndroidJniObject::callStaticMethod<void>("app/buzzer/mobile/NotificatorService",
									"notify", "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
									lType,
									lId.object<jstring>(),
									lAlias.object<jstring>(),
									lName.object<jstring>(),
									lComment.object<jstring>(),
									lText.object<jstring>(),
									lAvatarPath.object<jstring>(),
									lMediaPath.object<jstring>());
#endif
	}

	void avatarDownloadDone(qbit::TransactionPtr /*tx*/,
						   const std::string& previewFile,
						   const std::string& /*originalFile*/, unsigned short /*orientation*/, unsigned int /*duration*/,
						   uint64_t /*size*/, unsigned short /*type*/, const ProcessingError& result) {
		//
		if (result.success()) {
			//
			avatarFile_ = previewFile;
			// push notification
			makeNotification();
			// remove from queue (cache)
			gQueue.erase(buzz_->key());
		} else {
			gLog().write(Log::CLIENT, strprintf("[downloadAvater/error]: %s - %s", result.error(), result.message()));
		}
	}

	void mediaDownloadDone(qbit::TransactionPtr /*tx*/,
						   const std::string& previewFile,
						   const std::string& /*originalFile*/, unsigned short /*orientation*/, unsigned int /*duration*/,
						   uint64_t /*size*/, unsigned short type, const ProcessingError& result) {
		//
		if (result.success()) {
			//
			if (type == cubix::TxMediaHeader::Type::IMAGE_JPEG || type == cubix::TxMediaHeader::Type::IMAGE_PNG) {
				mediaFile_ = previewFile;
			}
		} else {
			gLog().write(Log::CLIENT, strprintf("[downloadMedia/error]: %s - %s", result.error(), result.message()));
		}

		// anyway
		loadAvatar();
	}

	void loadAvatar() {
		//
		downloadAvatar_ =
				cubix::DownloadMediaCommand::instance(gCubixComposer,
													  boost::bind(&PushNotification::downloadProgress, this, _1, _2),
													  boost::bind(&PushNotification::avatarDownloadDone, this, _1, _2, _3, _4, _5, _6, _7, _8));
		// locate buzzer info and avater url
		uint256 lInfoId;
		if (buzz_->buzzers().size()) {
			lInfoId = buzz_->buzzers()[0].buzzerInfoId();
		} else {
			lInfoId = buzz_->publisherInfo();
		}

		std::string lUrl;
		std::string lHeader;
		BuzzerMediaPointer lMedia;
		if (gBuzzer->locateAvatarMedia(lInfoId, lMedia)) {
			lUrl = lMedia.url();
			lHeader = lMedia.tx().toHex();
		}

		// temp path
		std::string lPath = buzzer::ApplicationPath::tempFilesDir().toStdString();

		// process
		std::vector<std::string> lArgs;
		lArgs.push_back(lUrl);
		lArgs.push_back(lPath + "/" + lHeader);
		lArgs.push_back("-preview");
		lArgs.push_back("-skip");
		downloadAvatar_->process(lArgs);
	}

	void loadMedia() {
		//
		BuzzerMediaPointer lMedia;
		if (buzz_->buzzMedia().size()) lMedia = buzz_->buzzMedia()[0];
		else {
			lMedia = buzz_->buzzers()[0].buzzMedia()[0];
		}

		downloadMedia_ =
				cubix::DownloadMediaCommand::instance(gCubixComposer,
													  boost::bind(&PushNotification::downloadProgress, this, _1, _2),
													  boost::bind(&PushNotification::mediaDownloadDone, this, _1, _2, _3, _4, _5, _6, _7, _8));
		std::string lUrl = lMedia.url();
		std::string lHeader = lMedia.tx().toHex();

		// temp path
		std::string lPath = buzzer::ApplicationPath::tempFilesDir().toStdString();

		// process
		std::vector<std::string> lArgs;
		lArgs.push_back(lUrl);
		lArgs.push_back(lPath + "/" + lHeader);
		lArgs.push_back("-preview");
		lArgs.push_back("-skip");
		downloadMedia_->process(lArgs);
	}

	void process() {
		//
		if ((buzz_->buzzMedia().size() || (buzz_->buzzers().size() && buzz_->buzzers()[0].buzzMedia().size())) &&
				(buzz_->type() != TX_BUZZER_MESSAGE && buzz_->type() != TX_BUZZER_MESSAGE_REPLY)) {
			loadMedia();
		} else {
			loadAvatar();
		}
	}

private:
	EventsfeedItemPtr buzz_;
	ICommandPtr downloadAvatar_;
	ICommandPtr downloadMedia_;

	std::string mediaFile_;
	std::string avatarFile_;
};

#if defined (BUZZER_MOD)

//
// eventsfeed
//

void eventsfeedLargeUpdated() {
	//
}

void eventsfeedItemNew(EventsfeedItemPtr buzz) {
	//
	PushNotificationPtr lItem = PushNotification::instance(buzz);
	gQueue[buzz->key()] = lItem;
	lItem->process();
}

void eventsfeedItemUpdated(EventsfeedItemPtr /*buzz*/) {
	//
}

void eventsfeedItemsUpdated(const std::vector<EventsfeedItem>& /*items*/) {
	//
}

void trustScoreUpdated(amount_t /*edorsements*/, amount_t /*mistrusts*/) {
	//
}

#endif

int loadConfig() {
	try {
		QString lAppConfig = buzzer::ApplicationPath::applicationDirPath() + "/" + APP_NAME + ".config";
		qInfo() << "Loading app-config:" << lAppConfig;

		if (QFile(lAppConfig).exists()) gAppConfig.loadFromFile(lAppConfig.toStdString());
		else {
			gLog().write(Log::CLIENT, strprintf("[error]: File is missing %s. Aborting...", lAppConfig.toStdString()));
			return -2;
		}
	} catch(buzzer::Exception const& ex) {
		gLog().write(Log::CLIENT, strprintf("[error]: %s", ex.message()));
		return -1;
	}

	return gAppConfig.hasErrors() ? -1 : 1;
}

std::string getLogCategories() {
	qbit::json::Value lValue;
	if (gAppConfig.find("logCategories", lValue)) {
		return lValue.getString();
	}

#ifdef QT_DEBUG
	return "all";
#else
	return "error,warn";
#endif
}

std::string getPeers() {
	qbit::json::Value lValue;
	if (gAppConfig.find("peers", lValue)) {
		return lValue.getString();
	}

	return "";
}

bool getTestNet() {
	qbit::json::Value lValue = gAppConfig["testNet"];
	return lValue.getBool();
}

int main(int argc, char** argv) {
	//
	qputenv("QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED", "0");

#ifdef Q_OS_ANDROID
	QAndroidService lApp(argc, argv);

	// light daemon mode
	qbit::gLightDaemon = true;

	// home
	buzzer::Settings* lSettings = buzzer::SettingsFactory::getDaemon();
	gClient = new DaemonClient();
	lSettings->link(gClient);
	lSettings->open();

	// setup log
	qbit::gLog(lSettings->dataPath() + "/debug-daemon.log"); // buzzer::ApplicationPath::logsDirPath().toStdString()

	// load shared configuration
	loadConfig();

	// setup categories
	std::vector<std::string> lCategories;
	boost::split(lCategories, std::string(getLogCategories()), boost::is_any_of(","));
	for (std::vector<std::string>::iterator lCategory = lCategories.begin(); lCategory != lCategories.end(); lCategory++) {
		qbit::gLog().enable(qbit::getLogCategory(*lCategory));
	}

	//
	std::stringstream lVer;
	lVer << "qbit-daemon-cli (" <<
		QBIT_VERSION_MAJOR << "." <<
		QBIT_VERSION_MINOR << "." <<
		QBIT_VERSION_REVISION << "." <<
		QBIT_VERSION_BUILD << ") | (c) 2020 q-bit.technology | MIT license";

	// setup testnet
	qbit::gTestNet = getTestNet();

	// request processor
	IRequestProcessorPtr lRequestProcessor = nullptr;
	lRequestProcessor = RequestProcessor::instance(lSettings->shared());

	// wallet
	IWalletPtr lWallet = LightWallet::instance(lSettings->shared(), lRequestProcessor);
	std::static_pointer_cast<RequestProcessor>(lRequestProcessor)->setWallet(lWallet);
	lWallet->open(""); // secret missing - notification service should not have access to the encrypted keys

	// composer
	LightComposerPtr lComposer = LightComposer::instance(lSettings->shared(), lWallet, lRequestProcessor);

	// peer manager
	IPeerManagerPtr lPeerManager = PeerManager::instance(lSettings->shared(),
										std::static_pointer_cast<RequestProcessor>(lRequestProcessor)->consensusManager());

	// tx types
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
	Message::registerMessageType(BUZZER_AND_INFO_ABSENT, "BUZZER_AND_INFO_ABSENT");
	Message::registerMessageType(GET_CONVERSATIONS_FEED_BY_BUZZER, "GET_CONVERSATIONS_FEED_BY_BUZZER");
	Message::registerMessageType(CONVERSATIONS_FEED_BY_BUZZER, "CONVERSATIONS_FEED_BY_BUZZER");
	Message::registerMessageType(GET_MESSAGES_FEED_BY_CONVERSATION, "GET_MESSAGES_FEED_BY_CONVERSATION");
	Message::registerMessageType(MESSAGES_FEED_BY_CONVERSATION, "MESSAGES_FEED_BY_CONVERSATION");
	Message::registerMessageType(NEW_BUZZER_CONVERSATION_NOTIFY, "NEW_BUZZER_CONVERSATION_NOTIFY");
	Message::registerMessageType(NEW_BUZZER_MESSAGE_NOTIFY, "NEW_BUZZER_MESSAGE_NOTIFY");
	Message::registerMessageType(UPDATE_BUZZER_CONVERSATION_NOTIFY, "UPDATE_BUZZER_CONVERSATION_NOTIFY");

	// buzzer request processor
	BuzzerRequestProcessorPtr lBuzzerRequestProcessor = BuzzerRequestProcessor::instance(lRequestProcessor);

	// buzzer
	gBuzzer = Buzzer::instance(lRequestProcessor, lBuzzerRequestProcessor, lWallet, boost::bind(&trustScoreUpdated, _1, _2));

	// buzzer composer
	gBuzzerComposer = BuzzerLightComposer::instance(lSettings->shared(),
																gBuzzer, lWallet, lRequestProcessor, lBuzzerRequestProcessor);

	// eventsfeed
	EventsfeedPtr lEventsfeed = Eventsfeed::instance(gBuzzer,
		boost::bind(&BuzzerLightComposer::verifyEventPublisher, gBuzzerComposer, _1),
		boost::bind(&BuzzerLightComposer::verifyPublisherStrict, gBuzzerComposer, _1),
		boost::bind(&eventsfeedLargeUpdated), 
		boost::bind(&eventsfeedItemNew, _1), 
		boost::bind(&eventsfeedItemUpdated, _1),
		boost::bind(&eventsfeedItemsUpdated, _1),
		EventsfeedItem::Merge::UNION
	);

	lEventsfeed->prepare();
	gBuzzer->setEventsfeed(lEventsfeed); // main events feed

	// buzzer peer extention
	PeerManager::registerPeerExtension("buzzer", BuzzerPeerExtensionCreator::instance(gBuzzer, nullptr, lEventsfeed));
#endif

#if defined (CUBIX_MOD)
	std::cout << "enabling 'cubix' module" << std::endl;

	// buzzer transactions
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_HEADER, cubix::TxMediaHeaderCreator::instance());
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_DATA, cubix::TxMediaDataCreator::instance());
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_SUMMARY, cubix::TxMediaSummaryCreator::instance());

	// cubix composer
	gCubixComposer = cubix::CubixLightComposer::instance(lSettings->shared(), lWallet, lRequestProcessor);

	// peer extention
	PeerManager::registerPeerExtension("cubix", cubix::DefaultPeerExtensionCreator::instance());	

	// dapp instance - shared
	gBuzzerComposer->setInstanceChangedCallback(boost::bind(&cubix::CubixLightComposer::setDAppSharedInstance, gCubixComposer, _1));
#endif

#if defined (BUZZER_MOD)
	// prepare composers
	gCubixComposer->open();
	gBuzzerComposer->open();
#endif

	// peers
	std::vector<std::string> lPeers;
	boost::split(lPeers, std::string(getPeers()), boost::is_any_of(","));
	for (std::vector<std::string>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
		lPeerManager->addPeer(*lPeer);
	}

	// control thread
	lPeerManager->run();

	// main thread and event loop
	return lApp.exec();
#else
	return -1;
#endif
}
