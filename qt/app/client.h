#ifndef BUZZER_MOBILE_CLIENT_H
#define BUZZER_MOBILE_CLIENT_H

#include <QString>
#include <QList>
#include <QAbstractListModel>
#include <QQuickItem>
#include <QMap>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QQuickTextDocument>
#include <QRegularExpression>

#include "json.h"
#include "iclient.h"
#include "isettings.h"
#include "irequestprocessor.h"

#include "log/log.h"
#include "key.h"
#include "transaction.h"
#include "requestprocessor.h"
#include "iconsensusmanager.h"
#include "ipeermanager.h"
#include "ipeer.h"

#include "iapplication.h"
#include "settings.h"
#include "../../client/commands.h"
#include "../../client/commandshandler.h"
#include "../../client/dapps/buzzer/buzzerrequestprocessor.h"
#include "../../client/dapps/buzzer/buzzercomposer.h"
#include "../../client/dapps/cubix/cubixcomposer.h"

#include "amount.h"

#include "txbase.h"
#include "txblockbase.h"
#include "txshard.h"

#include "dapps/buzzer/buzzfeed.h"
#include "dapps/buzzer/buzzfeedview.h"
#include "dapps/buzzer/eventsfeed.h"
#include "dapps/buzzer/conversationsfeed.h"

#include "buzzfeedlistmodel.h"
#include "eventsfeedlistmodel.h"
#include "conversationsfeedlistmodel.h"
#include "websourceinfo.h"
#include "wallettransactionslistmodel.h"
#include "peerslistmodel.h"

#include "emojidata.h"

#if defined(DESKTOP_PLATFORM)
#include "notificator.h"
#endif

Q_DECLARE_METATYPE(qbit::Buzzfeed*)
Q_DECLARE_METATYPE(qbit::BuzzfeedItem*)
Q_DECLARE_METATYPE(qbit::Eventsfeed*)
Q_DECLARE_METATYPE(qbit::EventsfeedItem*)
Q_DECLARE_METATYPE(qbit::Conversationsfeed*)
Q_DECLARE_METATYPE(qbit::ConversationItem*)

#define REGEX_UNIVERSAL_URL_PATTERN "((((https?|ftp|buzz|msg|qbit):(?://)?)(?:[-;:&=\\+\\$,\\w]+@)?[A-Za-z0-9.-\u0401\u0451\u0410-\u044f]+)((?:\\/[\\+~%\\/.\\w\\(\\)-:]*)?\\??(?:[-\\+=&;%@.\\w_]*)#?(?:[\\w]*))?)"
#define REGEX_EMBEDDED_LINK_START "(\\[(.*?)\\|"
#define REGEX_EMBEDDED_LINK_STOP "\\])"
#define REGEX_EMBEDDED_LINK REGEX_EMBEDDED_LINK_START REGEX_UNIVERSAL_URL_PATTERN REGEX_EMBEDDED_LINK_STOP

#define REGEX_EMBEDDED_URL_START "(\\w*(?<!')"
#define REGEX_EMBEDDED_URL_STOP "(?!'))"
#define REGEX_EMBEDDED_URL REGEX_EMBEDDED_URL_START REGEX_UNIVERSAL_URL_PATTERN REGEX_EMBEDDED_URL_STOP

namespace buzzer {

class Contact: public QObject
{
	Q_OBJECT

public:
	Contact() {}
	Contact(QString buzzer, QString pkey) : buzzer_(buzzer), pkey_(pkey) {}
	Contact(const Contact& other) {
		buzzer_ = other.buzzer();
		pkey_ = other.pkey();
	}

	Q_INVOKABLE QString buzzer() const { return buzzer_; }
	Q_INVOKABLE QString pkey() const { return pkey_; }

private:
	QString buzzer_;
	QString pkey_;
};

struct DecorationRule {
	QRegularExpression expression;
	QString pattern;
	bool truncate;
	int arg0 = 0;
	int arg1 = 0;
};

struct _Arg { int start; int length; QString text; };

class Settings;
class Client: public QObject, public IClient
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
	Q_PROPERTY(QString alias READ alias WRITE setAlias NOTIFY aliasChanged)
	Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
	Q_PROPERTY(QString avatar READ avatar WRITE setAvatar NOTIFY avatarChanged)
	Q_PROPERTY(QString header READ header WRITE setHeader NOTIFY headerChanged)
	Q_PROPERTY(QString locale READ locale WRITE setLocale NOTIFY localeChanged)
	Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
	Q_PROPERTY(QString statusBarTheme READ statusBarTheme NOTIFY statusBarThemeChanged)
	Q_PROPERTY(QString themeSelector READ themeSelector WRITE setThemeSelector NOTIFY themeSelectorChanged)
	Q_PROPERTY(bool cacheReady READ getCacheReady NOTIFY cacheReadyChanged)
	Q_PROPERTY(bool networkReady READ getNetworkReady NOTIFY networkReadyChanged)
	Q_PROPERTY(bool buzzerDAppReady READ getBuzzerDAppReady NOTIFY buzzerDAppReadyChanged)
	Q_PROPERTY(ulong trustScore READ getTrustScore NOTIFY trustScoreChanged)
	Q_PROPERTY(ulong endorsements READ getEndorsements NOTIFY endorsementsChanged)
	Q_PROPERTY(ulong mistrusts READ getMistrusts NOTIFY mistrustsChanged)
	Q_PROPERTY(QString address READ address NOTIFY addressChanged)
	Q_PROPERTY(QList<buzzer::Contact*> contacts READ selectContacts NOTIFY contactsChanged)
	Q_PROPERTY(QString scale READ scale WRITE setScale NOTIFY scaleChanged)
	Q_PROPERTY(double scaleFactor READ scaleFactor NOTIFY scaleFactorChanged)

public:
	Client(QObject *parent = nullptr);
	~Client();

	void setApplication(IApplication* app) {
		//
		application_ = app;
	}

    Q_INVOKABLE QString save();

	void settingsFromJSON(qbit::json::Value&);
	void settingsToJSON(qbit::json::Value&);
	std::string settingsDataPath();

    bool isFingerprintAccessConfigured();

    void setName(const QString& name) { name_ = name; emit nameChanged(); }
    QString name() const { return name_; }

	void setAlias(const QString& alias) { alias_ = alias; emit aliasChanged(); }
	QString alias() const { return alias_; }

	void setDescription(const QString& description) { description_ = description; emit descriptionChanged(); }
	QString description() const { return description_; }

	void setAvatar(const QString& avatar);
	QString avatar() const { return avatar_; }

	void setHeader(const QString& header);
	QString header() const { return header_; }

	void setLocale(const QString& locale) { locale_ = locale; emit localeChanged(); }
	QString locale() const { return locale_; }

    void setTheme(const QString& theme) { theme_ = theme; emit themeChanged(); }
    QString theme() const { return theme_; }

    void setThemeSelector(const QString& themeSelector) { themeSelector_ = themeSelector; emit themeSelectorChanged(); }
    QString themeSelector() const { return themeSelector_; }

	void setScale(const QString& scale) {
		scale_ = scale;

		if (scale_ == "100") scaleFactor_ = 1.00;
		else if (scale_ == "110") scaleFactor_ = 1.10;
		else if (scale_ == "115") scaleFactor_ = 1.15;
		else if (scale_ == "120") scaleFactor_ = 1.20;
		else if (scale_ == "125") scaleFactor_ = 1.25;
		else if (scale_ == "135") scaleFactor_ = 1.35;
		else if (scale_ == "150") scaleFactor_ = 1.50;
		else if (scale_ == "175") scaleFactor_ = 1.75;
		else if (scale_ == "200") scaleFactor_ = 2.00;
		else if (scale_ == "250") scaleFactor_ = 2.50;
		else if (scale_ == "300") scaleFactor_ = 3.00;

		emit scaleChanged();
		emit scaleFactorChanged();
	}

	QString scale() const { return scale_; }

	double scaleFactor() const { return scaleFactor_; }

	std::string getQttAsset();
	int getQttAssetLockTime();
	uint64_t getQttAssetVoteAmount();

	Q_INVOKABLE int open(QString);
    Q_INVOKABLE int openSettings();
    Q_INVOKABLE void revertChanges();
    Q_INVOKABLE bool configured();
	Q_INVOKABLE bool pinAccessConfigured();

	void suspend();
	void resume();

	Q_INVOKABLE void activatePeersUpdates() {
		peersModelUpdates_ = true;
	}

	Q_INVOKABLE void deactivatePeersUpdates() {
		peersModelUpdates_ = false;
	}

	QString address() {
		//
		if (wallet_) {
			//
			qbit::SKeyPtr lKey = wallet_->firstKey();
			qbit::PKey lPKey = lKey->createPKey();
			return QString::fromStdString(lPKey.toString());
		}

		return QString();
	}

    Q_INVOKABLE void setTheme(QString theme, QString themeSelector)
    {
        theme_ = theme;
        themeSelector_ = themeSelector;

        emit themeChanged();
        emit themeSelectorChanged();
		emit statusBarThemeChanged();
    }

    Q_INVOKABLE void setProperty(QString, QString);
    Q_INVOKABLE QString getProperty(QString);
    Q_INVOKABLE bool hasPropertyBeginWith(QString);
    Q_INVOKABLE bool hasPropertyBeginWithAndValue(QString, QString);

	QString statusBarTheme();

	Q_INVOKABLE void setFavEmoji(QString);

	bool getCacheReady() { return cacheReady_; }
	void setCacheReady() { cacheReady_ = true; emit cacheReadyChanged(); }
	bool getNetworkReady() { return networkReady_; }
	void setNetworkReady(bool state) {
		bool lNotify = state != networkReady_;
		networkReady_ = state;
		if (lNotify) emit networkReadyChanged();
	}
	bool getBuzzerDAppReady() { return buzzerDAppReady_; }
	void setBuzzerDAppReady();

    Q_INVOKABLE void unlink();

	void poll() {
		if (peerManager_) {
			peerManager_->poll();
		}
	}

	void extractFavoriteEmojis(std::vector<std::string>&);

	qbit::BuzzerPtr getBuzzer() { return buzzer_; }
	qbit::LightComposerPtr getComposer() { return composer_; }
	qbit::BuzzerLightComposerPtr getBuzzerComposer() { return buzzerComposer_; }
	qbit::cubix::CubixLightComposerPtr getCubixComposer() { return cubixComposer_; }
	qbit::IPeerManagerPtr getPeerManager() { return peerManager_; }
	buzzer::EmojiData* emojiData() { return emojiData_; }
	ConversationsListModel* getConversationsModel() { return conversationsList_; }

	Q_INVOKABLE void broadcastCurrentState() {
		//
		requestProcessor_->broadcastCurrentState();
	}

	Q_INVOKABLE QVariant getGlobalBuzzfeedList() { return QVariant::fromValue(globalBuzzfeedList_); }
	Q_INVOKABLE QVariant getBuzzfeedList() { return QVariant::fromValue(buzzfeedList_); }
	Q_INVOKABLE QVariant getEventsfeedList() { return QVariant::fromValue(eventsfeedList_); }

	Q_INVOKABLE QVariant getWalletLog(QString asset) {
		// qbit
		if (asset == "*") return QVariant::fromValue(walletLog_);
		// other
		uint256 lAsset; lAsset.setHex(asset.toStdString());
		std::map<uint256, _Wallet>::iterator lWalletLogs = wallets_.find(lAsset);
		if (lWalletLogs != wallets_.end()) return QVariant::fromValue(lWalletLogs->second.log_);
		return QVariant();
	}

	Q_INVOKABLE QVariant getWalletReceivedLog(QString asset) {
		// qbit
		if (asset == "*") return QVariant::fromValue(walletReceivedLog_);
		// other
		uint256 lAsset; lAsset.setHex(asset.toStdString());
		std::map<uint256, _Wallet>::iterator lWalletLogs = wallets_.find(lAsset);
		if (lWalletLogs != wallets_.end()) return QVariant::fromValue(lWalletLogs->second.receivedLog_);
		return QVariant();
	}

	Q_INVOKABLE QVariant getWalletSentLog(QString asset) {
		// qbit
		if (asset == "*") return QVariant::fromValue(walletSentLog_);
		// other
		uint256 lAsset; lAsset.setHex(asset.toStdString());
		std::map<uint256, _Wallet>::iterator lWalletLogs = wallets_.find(lAsset);
		if (lWalletLogs != wallets_.end()) return QVariant::fromValue(lWalletLogs->second.sentLog_);
		return QVariant();
	}

	Q_INVOKABLE QVariant getConversationsList() { return QVariant::fromValue(conversationsList_); }

	Q_INVOKABLE QVariant getPeersActive() { return QVariant::fromValue(peersActive_); }
	Q_INVOKABLE QVariant getPeersAll() { return QVariant::fromValue(peersAll_); }
	Q_INVOKABLE QVariant getPeersAdded() { return QVariant::fromValue(peersAdded_); }

	Q_INVOKABLE long getQbitRate();

	Q_INVOKABLE QString generateRandom() {
		return QString::fromStdString(qbit::Random::generate().toHex());
	}

	Q_INVOKABLE void forceResolveBuzzerInfos() {
		buzzer_->resolveBuzzerInfos();
	}

	Q_INVOKABLE QVariant createBuzzesBuzzfeedList() {
		BuzzfeedListModelBuzzes* lBuzzesBuzzfeedList = new BuzzfeedListModelBuzzes();
		buzzer_->registerUpdate(lBuzzesBuzzfeedList->buzzfeed());
		return QVariant::fromValue(lBuzzesBuzzfeedList);
	}

	Q_INVOKABLE QVariant createTagBuzzfeedList() {
		BuzzfeedListModelTag* lTagBuzzfeedList = new BuzzfeedListModelTag();
		buzzer_->registerUpdate(lTagBuzzfeedList->buzzfeed());
		return QVariant::fromValue(lTagBuzzfeedList);
	}

	Q_INVOKABLE QVariant createBuzzerBuzzfeedList() {
		BuzzfeedListModelBuzzer* lBuzzerBuzzfeedList = new BuzzfeedListModelBuzzer();
		buzzer_->registerUpdate(lBuzzerBuzzfeedList->buzzfeed());
		return QVariant::fromValue(lBuzzerBuzzfeedList);
	}

	Q_INVOKABLE QVariant createBuzzerFollowingList() {
		EventsfeedListModelFollowing* lList = new EventsfeedListModelFollowing();
		return QVariant::fromValue(lList);
	}

	Q_INVOKABLE QVariant createBuzzerFollowersList() {
		EventsfeedListModelFollowers* lList = new EventsfeedListModelFollowers();
		return QVariant::fromValue(lList);
	}

	Q_INVOKABLE QVariant createBuzzerEndorsementsList() {
		EventsfeedListModelEndorsements* lList = new EventsfeedListModelEndorsements();
		return QVariant::fromValue(lList);
	}

	Q_INVOKABLE QVariant createBuzzerMistrustsList() {
		EventsfeedListModelMistrusts* lList = new EventsfeedListModelMistrusts();
		return QVariant::fromValue(lList);
	}

	Q_INVOKABLE void unregisterBuzzfeed(BuzzfeedListModel* model) {
		if (model) buzzer_->removeUpdate(model->buzzfeed());
	}

	Q_INVOKABLE QVariant createConversationMessagesList() {
		ConversationMessagesList* lList = new ConversationMessagesList();
		return QVariant::fromValue(lList);
	}

	Q_INVOKABLE QString getCounterpartyKey(QString id) {
		//
		uint256 lId; lId.setHex(id.toStdString());
		qbit::PKey lPKey;
		if (buzzerComposer_->getCounterparty(lId, lPKey)) {
			return QString::fromStdString(lPKey.toString());
		}

		return QString();
	}

	Q_INVOKABLE bool subscriptionExists(QString publisher);
	Q_INVOKABLE bool haveSubscriptions() { return buzzerComposer_->haveSubscriptions(); }
	Q_INVOKABLE QString getBuzzerAvatarUrl(QString id);
	Q_INVOKABLE QString getBuzzerHeaderUrl(QString id);
	Q_INVOKABLE QString getBuzzerAvatarUrlHeader(QString id);
	Q_INVOKABLE QString getBuzzerHeaderUrlHeader(QString id);
	Q_INVOKABLE QString getBuzzerDescription(QString id);
	Q_INVOKABLE QString getBuzzerName(QString id);
	Q_INVOKABLE QString getBuzzerAlias(QString id);
	Q_INVOKABLE QString getTempFilesPath();
	Q_INVOKABLE QString getCurrentBuzzerId() {
		return QString::fromStdString(getBuzzerComposer()->buzzerId().toHex());
	}
	Q_INVOKABLE ulong getQbitBase();
	Q_INVOKABLE ulong getTrustScoreBase();
	Q_INVOKABLE ulong getTrustScore() { return buzzer_->score(); }
	Q_INVOKABLE ulong getEndorsements() { return buzzer_->endorsements(); }
	Q_INVOKABLE ulong getMistrusts() { return buzzer_->mistrusts(); }
	Q_INVOKABLE void setTrustScore(ulong endorsements, ulong mistrusts) {
		buzzer_->setTrustScore(endorsements, mistrusts);
	}
	Q_INVOKABLE void resetWalletCache() {
		//
		wallet_->resetCache();
	}
	Q_INVOKABLE void resync() {
		// start re-sync
		syncTimer_->start(500);
	}
	Q_INVOKABLE void setTopId(QString topId) {
		topId_ = topId;
	}

	Q_INVOKABLE QString getPlainText(QQuickTextDocument* textDocument) {
		QString lStr = textDocument->textDocument()->toPlainText();
		return lStr; // toRawText
	}

	Q_INVOKABLE QStringList extractBuzzers(QString text) {
		//
		QStringList lList;
		QRegularExpression pattern = QRegularExpression("@[A-Za-z0-9]+");
		QRegularExpressionMatchIterator matchIterator = pattern.globalMatch(text);
		while (matchIterator.hasNext()) {
			QRegularExpressionMatch match = matchIterator.next();
			lList.push_back(text.mid(match.capturedStart(), match.capturedLength()));
		}

		return lList;
	}

	Q_INVOKABLE void dAppStatusChanged() {
		emit buzzerDAppReadyChanged();
	}

	Q_INVOKABLE void addContact(QString, QString);
	Q_INVOKABLE void removeContact(QString);
	QList<buzzer::Contact*> selectContacts();

	Q_INVOKABLE QVariant locateConversation(QString);

	Q_INVOKABLE unsigned short tx_BUZZER_CONVERSATION() { return qbit::Transaction::TX_BUZZER_CONVERSATION; }
	Q_INVOKABLE unsigned short tx_BUZZER_ACCEPT_CONVERSATION() { return qbit::Transaction::TX_BUZZER_ACCEPT_CONVERSATION; }
	Q_INVOKABLE unsigned short tx_BUZZER_DECLINE_CONVERSATION() { return qbit::Transaction::TX_BUZZER_DECLINE_CONVERSATION; }
	Q_INVOKABLE unsigned short tx_BUZZER_MESSAGE() { return qbit::Transaction::TX_BUZZER_MESSAGE; }
	Q_INVOKABLE unsigned short tx_BUZZER_MESSAGE_REPLY() { return qbit::Transaction::TX_BUZZER_MESSAGE_REPLY; }
	Q_INVOKABLE unsigned short tx_BUZZER_TYPE() { return qbit::Transaction::TX_BUZZER; }
	Q_INVOKABLE unsigned short tx_BUZZ_TYPE() { return qbit::Transaction::TX_BUZZ; }
	Q_INVOKABLE unsigned short tx_REBUZZ_TYPE() { return qbit::Transaction::TX_REBUZZ; }
	Q_INVOKABLE unsigned short tx_REBUZZ_REPLY_TYPE() { return qbit::Transaction::TX_REBUZZ_REPLY; }
	Q_INVOKABLE unsigned short tx_BUZZ_REPLY_TYPE() { return qbit::Transaction::TX_BUZZ_REPLY; }
	Q_INVOKABLE unsigned short tx_BUZZ_LIKE_TYPE() { return qbit::Transaction::TX_BUZZ_LIKE; }
	Q_INVOKABLE unsigned short tx_BUZZ_REWARD_TYPE() { return qbit::Transaction::TX_BUZZ_REWARD; }
	Q_INVOKABLE unsigned short tx_BUZZER_MISTRUST_TYPE() { return qbit::Transaction::TX_BUZZER_MISTRUST; }
	Q_INVOKABLE unsigned short tx_BUZZER_ENDORSE_TYPE() { return qbit::Transaction::TX_BUZZER_ENDORSE; }
	Q_INVOKABLE unsigned short tx_BUZZER_SUBSCRIBE_TYPE() { return qbit::Transaction::TX_BUZZER_SUBSCRIBE; }
	Q_INVOKABLE unsigned short tx_CUBIX_MEDIA_SUMMARY() { return qbit::Transaction::TX_CUBIX_MEDIA_SUMMARY; }
	Q_INVOKABLE unsigned short tx_SPEND() { return qbit::Transaction::SPEND; }
	Q_INVOKABLE unsigned short tx_SPEND_PRIVATE() { return qbit::Transaction::SPEND_PRIVATE; }
	Q_INVOKABLE unsigned short tx_FEE() { return qbit::Transaction::FEE; }
	Q_INVOKABLE unsigned short tx_COINBASE() { return qbit::Transaction::COINBASE; }

	Q_INVOKABLE unsigned short peer_UNDEFINED() { return qbit::IPeer::UNDEFINED; }
	Q_INVOKABLE unsigned short peer_ACTIVE() { return qbit::IPeer::ACTIVE; }
	Q_INVOKABLE unsigned short peer_QUARANTINE() { return qbit::IPeer::QUARANTINE; }
	Q_INVOKABLE unsigned short peer_BANNED() { return qbit::IPeer::BANNED; }
	Q_INVOKABLE unsigned short peer_PENDING_STATE() { return qbit::IPeer::PENDING_STATE; }
	Q_INVOKABLE unsigned short peer_POSTPONED() { return qbit::IPeer::POSTPONED; }

	//
	// external open ...
	//
	Q_INVOKABLE void openThread(QString buzzChainId, QString buzzId, QString buzzerAlias, QString buzzBody) {
		//
		emit tryOpenThread(buzzChainId, buzzId, buzzerAlias, buzzBody);
	}
	Q_INVOKABLE void openBuzzfeedByBuzzer(QString buzzer) {
		//
		emit tryOpenBuzzfeedByBuzzer(buzzer);
	}
	Q_INVOKABLE void openConversation(QString id, QString conversationId, QVariant conversation, QVariant list) {
		//
		emit tryOpenConversation(id, conversationId, conversation, list);
	}

	Q_INVOKABLE int getBuzzBodyMaxSize() { return TX_BUZZ_BODY_SIZE; }
	Q_INVOKABLE int getBuzzBodySize(const QString& body) {
		std::string lStr = body.toStdString();
		return lStr.length();
	}

	Q_INVOKABLE bool addPeerExplicit(QString);
	Q_INVOKABLE void removePeer(QString);

	Q_INVOKABLE QString timestampAgo(long long timestamp);
	Q_INVOKABLE QString timeAgo(long long timestamp);
	Q_INVOKABLE QString decorateBuzzBody(const QString&);
	Q_INVOKABLE QString decorateBuzzBodyLimited(const QString&, int);
	Q_INVOKABLE QString unMarkdownBuzzBodyLimited(const QString&, int);
	Q_INVOKABLE bool isEmoji(const QString&);
	Q_INVOKABLE QString extractLastUrl(const QString&);

	Q_INVOKABLE void notifyBuzzerChanged() {
		emit buzzerChanged();
	}

	Q_INVOKABLE void cleanUpBuzzerCache();
	Q_INVOKABLE void preregisterBuzzerInstance();

	Q_INVOKABLE bool isTimelockReached(const QString&);

	Q_INVOKABLE QString resolveBuzzerAlias(QString buzzerInfoId) {
		//
		uint256 lInfoId; lInfoId.setHex(buzzerInfoId.toStdString());
		std::string lName;
		std::string lAlias;
		buzzer_->locateBuzzerInfo(lInfoId, lName, lAlias);

		return QString::fromStdString(lAlias);
	}

	Q_INVOKABLE QString resolveBuzzerName(QString buzzerInfoId) {
		//
		uint256 lInfoId; lInfoId.setHex(buzzerInfoId.toStdString());
		std::string lName;
		std::string lAlias;
		buzzer_->locateBuzzerInfo(lInfoId, lName, lAlias);

		return QString::fromStdString(lName);
	}

	Q_INVOKABLE QString resolveBuzzerDescription(QString buzzerInfoId);

	//qbit::BuzzfeedPtr getBuzzfeed() { return buzzfeed_; }
	//qbit::BuzzfeedPtr getGlobalBuzzfeed() { return globalBuzzfeed_; }
	//qbit::BuzzfeedPtr getBuzzerBuzzfeed() { return buzzerBuzzfeed_; }
	//qbit::BuzzfeedPtr getBuzzesBuzzfeed() { return buzzesBuzzfeed_; }

	//QList<qbit::BuzzfeedItemViewPtr>& getGlobalBuzzfeedList() { return globalBuzzfeedList_; }

	//QVariant q_globalBuzzfeedList() { return QVariant::fromValue(globalBuzzfeedList_); }

public slots:
	// events feed updated
	void eventsfeedUpdated(const qbit::EventsfeedItemProxy& /*event*/);

	// conversations updated
	void conversationsUpdated(const qbit::ConversationItemProxy& /*event*/);

	// resync wallet cache
	void resyncWalletCache();

public:
	// Wallet
	Q_INVOKABLE QString firstPKey();
	Q_INVOKABLE QString firstSKey();
	Q_INVOKABLE QString findSKey(const QString&);
	Q_INVOKABLE QStringList firstSeedWords();
	Q_INVOKABLE void removeAllKeys();
	Q_INVOKABLE QString importKey(QStringList);
	Q_INVOKABLE bool checkKey(QStringList);
	Q_INVOKABLE bool setCurrentKey(const QString&);
	Q_INVOKABLE QStringList ownBuzzers();
	Q_INVOKABLE bool isOwnBuzzer(const QString&);

	Q_INVOKABLE QString newKeyPair();
	Q_INVOKABLE QStringList keySeedWords(const QString&);

	bool isSuspended() { return suspended_; }

private:
	//
	void echoLog(unsigned int /*category*/, const std::string& message) {
		// debug only
		if (application_->getDebug())
			qInfo() << QString::fromStdString(message);
	}

	//
	void transactionReceived(qbit::Transaction::UnlinkedOutPtr /*utxo*/, qbit::TransactionPtr tx) {
		//
		emit walletTransactionReceived(QString::fromStdString(tx->chain().toHex()), QString::fromStdString(tx->id().toHex()));
	}

	//
	void chainStateChanged(const uint256& chain, const uint256 block, uint64_t height, uint64_t timestamp) {
		//
		QString lDateTime = QString::fromStdString(qbit::formatLocalDateTimeLong(timestamp));
		QString lAgo = timeAgo(timestamp);
		//
		emit chainStateUpdated(QString::fromStdString(chain.toHex()),
							   QString::fromStdString(block.toHex()), height, lDateTime, lAgo);

		if (chain == qbit::MainChain::id()) {
			emit mainChainStateUpdated(QString::fromStdString(block.toHex()), height, lDateTime, lAgo);
		}
	}

	//
	void prepareCache();

	//
	void trustScoreUpdated(uint64_t endorsements, uint64_t mistrusts) {
		//
		emit trustScoreChanged(endorsements, mistrusts);
		emit endorsementsChanged();
		emit mistrustsChanged();
	}

	//
	// Eventsfeed
	void eventsfeedLargeUpdated() {
		//
	}

	void eventsfeedItemNew(qbit::EventsfeedItemPtr /*buzz*/) {
		//
	}

	void eventsfeedItemUpdated(qbit::EventsfeedItemPtr /*buzz*/) {
		//
	}

	void eventsfeedItemsUpdated(const std::vector<qbit::EventsfeedItem>& /*items*/) {
		//
	}

	//
	// Context eventsfeed
	void eventsfeedLargeUpdatedContext() {
		//
	}

	void eventsfeedItemNewContext(qbit::EventsfeedItemPtr /*buzz*/) {
		//
	}

	void eventsfeedItemUpdatedContext(qbit::EventsfeedItemPtr /*buzz*/) {
		//
	}

	void eventsfeedItemsUpdatedContext(const std::vector<qbit::EventsfeedItem>& /*items*/) {
		//
	}

	//
	// peers/network information
	void peerPushed(qbit::IPeerPtr /*peer*/, bool /*update*/, int /*count*/);
	void peerPopped(qbit::IPeerPtr /*peer*/, int /*count*/);

	QString internalTimestampAgo(long long, long long);

	void processDecoration(QVector<DecorationRule>& rules, const QString& body, int limit, bool simplify, QString& result);

signals:
	void nameChanged();
	void aliasChanged();
	void descriptionChanged();
	void avatarChanged();
	void headerChanged();
	void localeChanged();
	void themeChanged();
	void scaleChanged();
	void scaleFactorChanged();
	void themeSelectorChanged();
	void cacheReadyChanged();
	void networkReadyChanged();
	void buzzerDAppReadyChanged();
	void buzzerDAppResumed();
	void buzzerDAppSuspended();
	void trustScoreChanged(long endorsements, long mistrusts);
	void endorsementsChanged();
	void mistrustsChanged();
	void chainStateUpdated(QString chain, QString block, long long height, QString time, QString ago);
	void mainChainStateUpdated(QString block, long long height, QString time, QString ago);
	void addressChanged();
	void contactsChanged();
	void statusBarThemeChanged();
	void fireResyncWalletCache();

	void buzzerChanged();

	void networkReady();
	void networkLimited();
	void networkUnavailable();

	void walletTransactionReceived(QString chain, QString tx);

	void newEvents();
	void newMessages();

	void peerPushedSignal(buzzer::PeerProxy peer, bool update, int count);
	void peerPoppedSignal(buzzer::PeerProxy peer, int count);

	void tryOpenThread(QString buzzChainId, QString buzzId, QString buzzerAlias, QString buzzBody);
	void tryOpenBuzzfeedByBuzzer(QString buzzer);
	void tryOpenConversation(QString id, QString conversationId, QVariant conversation, QVariant list);

private:
    QString name_;
	QString alias_;
	QString description_;
	QString avatar_;
	QString header_;
	QString locale_;
    QString theme_;
    QString themeSelector_;
	QString scale_ = "100";
	double scaleFactor_ = 1.00;
	std::map<std::string, std::string> properties_;
	qbit::json::Document back_;
	qbit::json::Document appConfig_;
	Settings* settings_;
	IApplication* application_;
	QTimer* syncTimer_;
	bool cacheReady_ = false;
	bool networkReady_ = false;
	bool buzzerDAppReady_ = false;
	bool suspended_ = false;
	bool recallWallet_ = true;
	bool opened_ = false;
	bool peersModelUpdates_ = false;
	QString topId_ = "";

private:
	qbit::IRequestProcessorPtr requestProcessor_ = nullptr;
	qbit::IWalletPtr wallet_ = nullptr;
	qbit::LightComposerPtr composer_ = nullptr;
	qbit::IPeerManagerPtr peerManager_ = nullptr;
	qbit::CommandsHandlerPtr commandsHandler_ = nullptr;

	/*
	qbit::BuzzfeedPtr buzzfeed_ = nullptr;
	qbit::BuzzfeedPtr globalBuzzfeed_ = nullptr;
	qbit::BuzzfeedPtr buzzerBuzzfeed_ = nullptr;
	qbit::BuzzfeedPtr buzzesBuzzfeed_ = nullptr;
	*/
	/*
	qbit::EventsfeedPtr eventsfeed_ = nullptr;
	qbit::EventsfeedPtr contextEventsfeed_ = nullptr;
	*/

	qbit::BuzzerPtr buzzer_ = nullptr;
	qbit::BuzzerRequestProcessorPtr buzzerRequestProcessor_ = nullptr;
	qbit::BuzzerLightComposerPtr buzzerComposer_ = nullptr;
	qbit::cubix::CubixLightComposerPtr cubixComposer_ = nullptr;

private:
	BuzzfeedListModelPersonal* buzzfeedList_ = nullptr;
	BuzzfeedListModelGlobal* globalBuzzfeedList_ = nullptr;
	EventsfeedListModelPersonal* eventsfeedList_ = nullptr;
	ConversationsListModel* conversationsList_ = nullptr;

	struct _Wallet {
		WalletTransactionsListModel* log_ = nullptr;
		WalletTransactionsListModelReceived* receivedLog_ = nullptr;
		WalletTransactionsListModelSent* sentLog_ = nullptr;

		_Wallet() {}
		_Wallet(WalletTransactionsListModel* walletLog, WalletTransactionsListModelReceived* walletReceivedLog, WalletTransactionsListModelSent* walletSentLog) {
			log_ = walletLog;
			receivedLog_ = walletReceivedLog;
			sentLog_ = walletSentLog;
		}
		_Wallet(const uint256& asset, qbit::IWalletPtr wallet) {
			log_ = new WalletTransactionsListModel(std::string("log-") + asset.toHex(), asset, wallet);
			receivedLog_ = new WalletTransactionsListModelReceived(asset, wallet);
			sentLog_ = new WalletTransactionsListModelSent(asset, wallet);
		}
	};

	std::map<uint256 /*asset*/, _Wallet> wallets_;

	WalletTransactionsListModel* walletLog_ = nullptr;
	WalletTransactionsListModelReceived* walletReceivedLog_ = nullptr;
	WalletTransactionsListModelSent* walletSentLog_ = nullptr;

	PeersActiveListModel* peersActive_ = nullptr;
	PeersListModel* peersAll_ = nullptr;
	PeersAddedListModel* peersAdded_ = nullptr;
	int lastPeersCount_ = 0;

	EmojiData* emojiData_ = nullptr;
};

} // buzzer

#endif // BUZZER_MOBILE_CLIENT_H
