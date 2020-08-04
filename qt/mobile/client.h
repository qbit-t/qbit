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

#include "iapplication.h"
#include "buzzfeedlistmodel.h"
#include "eventsfeedlistmodel.h"
#include "websourceinfo.h"

Q_DECLARE_METATYPE(qbit::Buzzfeed*)
Q_DECLARE_METATYPE(qbit::BuzzfeedItem*)
Q_DECLARE_METATYPE(qbit::Eventsfeed*)
Q_DECLARE_METATYPE(qbit::EventsfeedItem*)

namespace buzzer {

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
	Q_PROPERTY(QString themeSelector READ themeSelector WRITE setThemeSelector NOTIFY themeSelectorChanged)
	Q_PROPERTY(bool cacheReady READ getCacheReady NOTIFY cacheReadyChanged)
	Q_PROPERTY(bool networkReady READ getNetworkReady NOTIFY networkReadyChanged)
	Q_PROPERTY(bool buzzerDAppReady READ getBuzzerDAppReady NOTIFY buzzerDAppReadyChanged)
	Q_PROPERTY(ulong trustScore READ getTrustScore NOTIFY trustScoreChanged)
	Q_PROPERTY(ulong endorsements READ getEndorsements NOTIFY endorsementsChanged)
	Q_PROPERTY(ulong mistrusts READ getMistrusts NOTIFY mistrustsChanged)

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

    bool isFingerprintAccessConfigured();

    void setName(const QString& name) { name_ = name; emit nameChanged(); }
    QString name() const { return name_; }

	void setAlias(const QString& alias) { alias_ = alias; emit aliasChanged(); }
	QString alias() const { return alias_; }

	void setDescription(const QString& description) { description_ = description; emit descriptionChanged(); }
	QString description() const { return description_; }

	void setAvatar(const QString& avatar) { avatar_ = avatar; emit avatarChanged(); }
	QString avatar() const { return avatar_; }

	void setHeader(const QString& header) { header_ = header; emit headerChanged(); }
	QString header() const { return header_; }

	void setLocale(const QString& locale) { locale_ = locale; emit localeChanged(); }
	QString locale() const { return locale_; }

    void setTheme(const QString& theme) { theme_ = theme; emit themeChanged(); }
    QString theme() const { return theme_; }

    void setThemeSelector(const QString& themeSelector) { themeSelector_ = themeSelector; emit themeSelectorChanged(); }
    QString themeSelector() const { return themeSelector_; }

    Q_INVOKABLE int open(QString);
    Q_INVOKABLE int openSettings();
    Q_INVOKABLE void revertChanges();
    Q_INVOKABLE bool configured();
	Q_INVOKABLE bool pinAccessConfigured();

    Q_INVOKABLE void setTheme(QString theme, QString themeSelector)
    {
        theme_ = theme;
        themeSelector_ = themeSelector;

        emit themeChanged();
        emit themeSelectorChanged();
    }

    Q_INVOKABLE void setProperty(QString, QString);
    Q_INVOKABLE QString getProperty(QString);
    Q_INVOKABLE bool hasPropertyBeginWith(QString);
    Q_INVOKABLE bool hasPropertyBeginWithAndValue(QString, QString);

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

	qbit::BuzzerPtr getBuzzer() { return buzzer_; }
	qbit::LightComposerPtr getComposer() { return composer_; }
	qbit::BuzzerLightComposerPtr getBuzzerComposer() { return buzzerComposer_; }
	qbit::cubix::CubixLightComposerPtr getCubixComposer() { return cubixComposer_; }

	Q_INVOKABLE QVariant getGlobalBuzzfeedList() { return QVariant::fromValue(globalBuzzfeedList_); }
	Q_INVOKABLE QVariant getBuzzfeedList() { return QVariant::fromValue(buzzfeedList_); }
	//Q_INVOKABLE QVariant getBuzzerBuzzfeedList() { return QVariant::fromValue(buzzerBuzzfeedList_); }
	//Q_INVOKABLE QVariant getBuzzesBuzzfeedList() {return QVariant::fromValue(buzzesBuzzfeedList_); }
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
	Q_INVOKABLE ulong getTrustScoreBase();
	Q_INVOKABLE ulong getTrustScore() { return buzzer_->score(); }
	Q_INVOKABLE ulong getEndorsements() { return buzzer_->endorsements(); }
	Q_INVOKABLE ulong getMistrusts() { return buzzer_->mistrusts(); }
	Q_INVOKABLE void setTrustScore(ulong endorsements, ulong mistrusts) {
		buzzer_->setTrustScore(endorsements, mistrusts);
	}
	Q_INVOKABLE void resync() {
		// start re-sync
		syncTimer_->start(500);
	}

	Q_INVOKABLE QString getPlainText(QQuickTextDocument* textDocument) {
		return textDocument->textDocument()->toPlainText();
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

	Q_INVOKABLE unsigned short tx_BUZZ_TYPE() { return qbit::Transaction::TX_BUZZ; }
	Q_INVOKABLE unsigned short tx_REBUZZ_TYPE() { return qbit::Transaction::TX_REBUZZ; }
	Q_INVOKABLE unsigned short tx_BUZZ_REPLY_TYPE() { return qbit::Transaction::TX_BUZZ_REPLY; }
	Q_INVOKABLE unsigned short tx_BUZZ_LIKE_TYPE() { return qbit::Transaction::TX_BUZZ_LIKE; }
	Q_INVOKABLE unsigned short tx_BUZZ_REWARD_TYPE() { return qbit::Transaction::TX_BUZZ_REWARD; }
	Q_INVOKABLE unsigned short tx_BUZZER_MISTRUST_TYPE() { return qbit::Transaction::TX_BUZZER_MISTRUST; }
	Q_INVOKABLE unsigned short tx_BUZZER_ENDORSE_TYPE() { return qbit::Transaction::TX_BUZZER_ENDORSE; }

	Q_INVOKABLE int getBuzzBodyMaxSize() { return TX_BUZZ_BODY_SIZE; }

	Q_INVOKABLE QString timestampAgo(long long timestamp);
	Q_INVOKABLE QString decorateBuzzBody(const QString&);
	Q_INVOKABLE QString extractLastUrl(const QString&);

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

public:
	// Wallet
	Q_INVOKABLE QString firstPKey();
	Q_INVOKABLE QString firstSKey();
	Q_INVOKABLE QStringList firstSeedWords();

private:
	//
	void echoLog(unsigned int /*category*/, const std::string& message) {
		// debug only
		qInfo() << QString::fromStdString(message);
	}

	//
	void transactionReceived(qbit::Transaction::UnlinkedOutPtr utxo, qbit::TransactionPtr tx) {
		//
		emit walletTransactionReceived(QString::fromStdString(tx->chain().toHex()), QString::fromStdString(tx->id().toHex()));
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

signals:
	void nameChanged();
	void aliasChanged();
	void descriptionChanged();
	void avatarChanged();
	void headerChanged();
	void localeChanged();
	void themeChanged();
    void themeSelectorChanged();
	void cacheReadyChanged();
	void networkReadyChanged();
	void buzzerDAppReadyChanged();
	void trustScoreChanged(long endorsements, long mistrusts);
	void endorsementsChanged();
	void mistrustsChanged();

	void networkReady();
	void networkLimited();
	void networkUnavailable();

	void walletTransactionReceived(QString chain, QString tx);

private:
    QString name_;
	QString alias_;
	QString description_;
	QString avatar_;
	QString header_;
	QString locale_;
    QString theme_;
    QString themeSelector_;
	std::map<std::string, std::string> properties_;
	qbit::json::Document back_;
	qbit::json::Document appConfig_;
	Settings* settings_;
	IApplication* application_;
	QTimer* syncTimer_;
	bool cacheReady_ = false;
	bool networkReady_ = false;
	bool buzzerDAppReady_ = false;

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
	BuzzfeedListModelPersonal* buzzfeedList_;
	BuzzfeedListModelGlobal* globalBuzzfeedList_;
	EventsfeedListModelPersonal* eventsfeedList_;

	//BuzzfeedListModelBuzzer* buzzerBuzzfeedList_;
	//BuzzfeedListModelBuzzes* buzzesBuzzfeedList_;
};

} // buzzer

#endif // BUZZER_MOBILE_CLIENT_H
