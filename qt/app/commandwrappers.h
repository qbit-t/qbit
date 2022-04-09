#ifndef BUZZER_COMMAND_WRAPPERS_H
#define BUZZER_COMMAND_WRAPPERS_H

#include <QString>
#include <QList>
#include <QAbstractListModel>
#include <QQuickItem>
#include <QMap>
#include <QQmlApplicationEngine>
#include <QRandomGenerator64>

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
#include "../../client/dapps/buzzer/buzzercommands.h"
#include "../../client/dapps/buzzer/buzzercomposer.h"
#include "../../client/dapps/cubix/cubixcommands.h"
#include "../../client/dapps/cubix/cubixcomposer.h"

#include "amount.h"

#include "txbase.h"
#include "txblockbase.h"
#include "txshard.h"

#include "dapps/buzzer/buzzfeed.h"
#include "dapps/buzzer/eventsfeed.h"
#include "dapps/cubix/txmediaheader.h"

#include "iapplication.h"
#include "buzzfeedlistmodel.h"
#include "eventsfeedlistmodel.h"

#define RETRY_MAX_COUNT 4

namespace buzzer {

class AskForQbitsCommand: public QObject
{
	Q_OBJECT

public:
	explicit AskForQbitsCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		command_->process(std::vector<std::string>());
	}

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

signals:
	void processed();
	void error(QString code, QString message);

private:
	qbit::ICommandPtr command_;
};

class BalanceCommand: public QObject
{
	Q_OBJECT

public:
	explicit BalanceCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		command_->process(std::vector<std::string>());
	}

	Q_INVOKABLE void process(QString asset) {
		std::vector<std::string> lArgs;
		lArgs.push_back(asset.toStdString());
		command_->process(lArgs);
	}

	void done(double amount, double pending, uint64_t scale, const qbit::ProcessingError& result) {
		if (result.success()) emit processed(amount, pending, scale);
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

signals:
	void processed(double amount, double pending, qlonglong scale);
	void error(QString code, QString message);

private:
	qbit::ICommandPtr command_;
};

class SearchEntityNamesCommand: public QObject
{
	Q_OBJECT

public:
	explicit SearchEntityNamesCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString part) {
		//
		std::vector<std::string> lArgs;
		lArgs.push_back(part.toStdString());
		command_->process(lArgs);
	}

	void done(const std::string& name, const std::vector<qbit::IEntityStore::EntityName>& names, const qbit::ProcessingError& result) {
		if (result.success()) {
			QStringList lList;
			for (std::vector<qbit::IEntityStore::EntityName>::const_iterator lName = names.begin(); lName != names.end(); lName++) {
				lList.push_back(QString::fromStdString(lName->data()));
			}
			emit processed(QString::fromStdString(name), lList);
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

signals:
	void processed(QString pattern, QStringList entities);
	void error(QString code, QString message);

private:
	qbit::ICommandPtr command_;
};

class LoadHashTagsCommand: public QObject
{
	Q_OBJECT

public:
	explicit LoadHashTagsCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString part) {
		//
		std::vector<std::string> lArgs;
		lArgs.push_back(part.toStdString());
		command_->process(lArgs);
	}

	void done(const std::string& tag, const std::vector<qbit::Buzzer::HashTag>& tags, const qbit::ProcessingError& result) {
		if (result.success()) {
			QStringList lList;
			for (std::vector<qbit::Buzzer::HashTag>::const_iterator lTag = tags.begin(); lTag != tags.end(); lTag++) {
				lList.push_back(QString::fromStdString(lTag->tag()));
			}
			emit processed(QString::fromStdString(tag), lList);
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

signals:
	void processed(QString pattern, QStringList tags);
	void error(QString code, QString message);

private:
	qbit::ICommandPtr command_;
};

class CreateBuzzerCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
	Q_PROPERTY(QString alias READ alias WRITE setAlias NOTIFY aliasChanged)
	Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
	Q_PROPERTY(QString avatar READ avatar WRITE setAvatar NOTIFY avatarChanged)
	Q_PROPERTY(QString header READ header WRITE setHeader NOTIFY headerChanged)

public:
	explicit CreateBuzzerCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		std::vector<std::string> lArgs;
		lArgs.push_back(name_.toStdString());
		lArgs.push_back(alias_.toStdString());
		lArgs.push_back(description_.toStdString());

		lArgs.push_back(avatar_.toStdString()); // full path
		lArgs.push_back("-s");
		lArgs.push_back("160x100");
		lArgs.push_back(header_.toStdString()); // full path

		qInfo() << "create buzzer" << name_ << alias_ << avatar_ << header_;

		command_->process(lArgs);
	}

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

	void done(qbit::TransactionPtr buzzerTx, qbit::TransactionPtr buzzerInfoTx, const qbit::ProcessingError& result) {
		if (result.success())
			emit processed(
					QString::fromStdString(buzzerTx->id().toHex()), QString::fromStdString(buzzerTx->chain().toHex()),
					QString::fromStdString(buzzerInfoTx->id().toHex()), QString::fromStdString(buzzerInfoTx->chain().toHex())
			);
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

	void avatarUploadProgress(const std::string& /*file*/, uint64_t pos, uint64_t size) {
		emit avatarProgress(pos, size);
	}

	void headerUploadProgress(const std::string& /*file*/, uint64_t pos, uint64_t size) {
		emit headerProgress(pos, size);
	}

	void avatarUploaded(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) emit avatarProcessed(QString::fromStdString(tx->id().toHex()), QString::fromStdString(tx->chain().toHex()));
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

	void headerUploaded(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) emit headerProcessed(QString::fromStdString(tx->id().toHex()), QString::fromStdString(tx->chain().toHex()));
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

signals:
	void processed(QString buzzer, QString buzzerChain, QString buzzerInfo, QString buzzerInfoChain);
	void avatarProcessed(QString tx, QString chain);
	void headerProcessed(QString tx, QString chain);
	void avatarProgress(ulong pos, ulong size);
	void headerProgress(ulong pos, ulong size);
	void error(QString code, QString message);
	void nameChanged();
	void aliasChanged();
	void descriptionChanged();
	void avatarChanged();
	void headerChanged();

private:
	QString name_;
	QString alias_;
	QString description_;
	QString avatar_;
	QString header_;

	qbit::CreateBuzzerCommandPtr command_;
};

class CreateBuzzerInfoCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString alias READ alias WRITE setAlias NOTIFY aliasChanged)
	Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
	Q_PROPERTY(QString avatar READ avatar WRITE setAvatar NOTIFY avatarChanged)
	Q_PROPERTY(QString header READ header WRITE setHeader NOTIFY headerChanged)

public:
	explicit CreateBuzzerInfoCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		std::vector<std::string> lArgs;
		lArgs.push_back(alias_.toStdString());
		lArgs.push_back(description_.toStdString());

		lArgs.push_back(avatar_.toStdString()); // full path
		lArgs.push_back("-s");
		lArgs.push_back("160x100");
		lArgs.push_back(header_.toStdString()); // full path

		command_->process(lArgs);
	}

	void setAlias(const QString& alias) { alias_ = alias; emit aliasChanged(); }
	QString alias() const { return alias_; }

	void setDescription(const QString& description) { description_ = description; emit descriptionChanged(); }
	QString description() const { return description_; }

	void setAvatar(const QString& avatar) { avatar_ = avatar; emit avatarChanged(); }
	QString avatar() const { return avatar_; }

	void setHeader(const QString& header) { header_ = header; emit headerChanged(); }
	QString header() const { return header_; }

	void done(qbit::TransactionPtr buzzerTx, qbit::TransactionPtr buzzerInfoTx, const qbit::ProcessingError& result) {
		if (result.success())
			emit processed(
					QString::fromStdString(buzzerInfoTx->id().toHex()), QString::fromStdString(buzzerInfoTx->chain().toHex())
			);
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

	void avatarUploadProgress(const std::string& /*file*/, uint64_t pos, uint64_t size) {
		emit avatarProgress(pos, size);
	}

	void headerUploadProgress(const std::string& /*file*/, uint64_t pos, uint64_t size) {
		emit headerProgress(pos, size);
	}

	void avatarUploaded(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) emit avatarProcessed(QString::fromStdString(tx->id().toHex()), QString::fromStdString(tx->chain().toHex()));
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

	void headerUploaded(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) emit headerProcessed(QString::fromStdString(tx->id().toHex()), QString::fromStdString(tx->chain().toHex()));
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

signals:
	void processed(QString buzzerInfo, QString buzzerInfoChain);
	void avatarProcessed(QString tx, QString chain);
	void headerProcessed(QString tx, QString chain);
	void avatarProgress(ulong pos, ulong size);
	void headerProgress(ulong pos, ulong size);
	void error(QString code, QString message);
	void aliasChanged();
	void descriptionChanged();
	void avatarChanged();
	void headerChanged();

private:
	QString alias_;
	QString description_;
	QString avatar_;
	QString header_;

	qbit::CreateBuzzerInfoCommandPtr command_;
};

class LoadBuzzerTrustScoreCommand: public QObject
{
	Q_OBJECT

public:
	explicit LoadBuzzerTrustScoreCommand(QObject* parent = nullptr);
	virtual ~LoadBuzzerTrustScoreCommand() {
		if (command_) command_->terminate();
	}

	Q_INVOKABLE void process() {
		command_->process(std::vector<std::string>());
	}

	Q_INVOKABLE void process(QString buzzer) {
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer.toStdString());
		command_->process(lArgs);
	}

	void done(uint64_t endorsements, uint64_t mistrusts, uint32_t following, uint32_t followers, const qbit::ProcessingError& result) {
		if (result.success()) emit processed(endorsements, mistrusts, following, followers);
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

signals:
	void processed(ulong endorsements, ulong mistrusts, ulong following, ulong followers);
	void error(QString code, QString message);

private:
	qbit::ICommandPtr command_;
};

class LoadBuzzesGlobalCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadBuzzesGlobalCommand(QObject* parent = nullptr);
	virtual ~LoadBuzzesGlobalCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!buzzfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed
		// set buzzes updates to this buzzfeed
		//Client* lClient = static_cast<Client*>(gApplication->getClient());
		//lClient->getBuzzer()->setBuzzfeed(buzzfeedModel_->buzzfeed());

		//
		if (!more_) buzzfeedModel_->buzzfeed()->clear();
		//
		std::vector<std::string> lArgs;
		if (more) lArgs.push_back("more");
		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; }

	void ready(qbit::BuzzfeedPtr base, qbit::BuzzfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::BuzzfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);

private:
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::LoadBuzzesGlobalCommandPtr command_ = nullptr;
	bool more_ = false;
};

class LoadBuzzfeedByBuzzCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzId READ buzzId WRITE setBuzzId NOTIFY buzzIdChanged)
	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadBuzzfeedByBuzzCommand(QObject* parent = nullptr);
	virtual ~LoadBuzzfeedByBuzzCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!buzzfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = false;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) buzzfeedModel_->buzzfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzId_.toStdString());
		if (more) lArgs.push_back("more");

		command_->process(lArgs);
	}

	Q_INVOKABLE void processAndMerge() {
		processAndMerge(false);
	}

	Q_INVOKABLE void processAndMerge(bool more) {
		//
		if (!buzzfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = true;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) buzzfeedModel_->buzzfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzId_.toStdString());
		if (more) lArgs.push_back("more");

		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	void setBuzzId(const QString& buzzId) { buzzId_ = buzzId; emit buzzIdChanged(); }
	QString buzzId() const { return buzzId_; }

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; }

	void ready(qbit::BuzzfeedPtr base, qbit::BuzzfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::BuzzfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);
	void buzzIdChanged();

private:
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::LoadBuzzfeedByBuzzCommandPtr command_ = nullptr;
	QString buzzId_;
	bool more_ = false;
	bool merge_ = false;
};

class LoadBuzzfeedByBuzzerCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzer READ buzzer WRITE setBuzzer NOTIFY buzzerChanged)
	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadBuzzfeedByBuzzerCommand(QObject* parent = nullptr);
	virtual ~LoadBuzzfeedByBuzzerCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!buzzfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = false;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) buzzfeedModel_->buzzfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
		if (more) lArgs.push_back("more");

		command_->process(lArgs);
	}

	Q_INVOKABLE void processAndMerge() {
		//
		if (!buzzfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = false;
		merge_ = true;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) buzzfeedModel_->buzzfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());

		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	void setBuzzer(const QString& buzzer) { buzzer_ = buzzer; emit buzzerChanged(); }
	QString buzzer() const { return buzzer_; }

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; }

	void ready(qbit::BuzzfeedPtr base, qbit::BuzzfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::BuzzfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);
	void buzzerChanged();

private:
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::LoadBuzzfeedByBuzzerCommandPtr command_ = nullptr;
	QString buzzer_;
	bool more_ = false;
	bool merge_ = false;
};

class LoadBuzzfeedByTagCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString tag READ tag WRITE setTag NOTIFY tagChanged)
	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadBuzzfeedByTagCommand(QObject* parent = nullptr);
	virtual ~LoadBuzzfeedByTagCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!buzzfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) buzzfeedModel_->buzzfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(tag_.toStdString());
		if (more) lArgs.push_back("more");

		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	void setTag(const QString& tag) { tag_ = tag; emit tagChanged(); }
	QString tag() const { return tag_; }

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; }

	void ready(qbit::BuzzfeedPtr base, qbit::BuzzfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::BuzzfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);
	void tagChanged();

private:
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::LoadBuzzfeedByTagCommandPtr command_ = nullptr;
	QString tag_;
	bool more_ = false;
};

class LoadBuzzfeedCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadBuzzfeedCommand(QObject* parent = nullptr);
	virtual ~LoadBuzzfeedCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!buzzfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = false;

		// set buzzes updates to this buzzfeed
		Client* lClient = static_cast<Client*>(gApplication->getClient());
		lClient->getBuzzer()->setBuzzfeed(buzzfeedModel_->buzzfeed());

		//
		if (!more_) buzzfeedModel_->buzzfeed()->clear();
		//
		std::vector<std::string> lArgs;
		if (more) lArgs.push_back("more");
		command_->process(lArgs);
	}

	Q_INVOKABLE void processAndMerge() {
		//
		if (!buzzfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = false;
		merge_ = true;

		//
		if (!more_) buzzfeedModel_->buzzfeed()->clear();
		//
		std::vector<std::string> lArgs;
		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; }

	void ready(qbit::BuzzfeedPtr base, qbit::BuzzfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::BuzzfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);

private:
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::LoadBuzzfeedCommandPtr command_ = nullptr;
	bool more_ = false;
	bool merge_ = false;
};

class BuzzerSubscribeCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
	explicit BuzzerSubscribeCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		std::vector<std::string> lArgs;
		lArgs.push_back(name_.toStdString());
		command_->process(lArgs);
	}

	Q_INVOKABLE void process(QString buzzer) {
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer.toStdString());
		command_->process(lArgs);
	}

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

	void setName(const QString& name) { name_ = name; emit nameChanged(); }
	QString name() const { return name_; }

signals:
	void nameChanged();
	void processed();
	void error(QString code, QString message);

private:
	qbit::ICommandPtr command_;
	QString name_;
};

class BuzzerUnsubscribeCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
	explicit BuzzerUnsubscribeCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		std::vector<std::string> lArgs;
		lArgs.push_back(name_.toStdString());
		command_->process(lArgs);
	}

	Q_INVOKABLE void process(QString buzzer) {
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer.toStdString());
		command_->process(lArgs);
	}

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

	void setName(const QString& name) { name_ = name; emit nameChanged(); }
	QString name() const { return name_; }

signals:
	void nameChanged();
	void processed();
	void error(QString code, QString message);

private:
	qbit::ICommandPtr command_;
	QString name_;
};

class BuzzerEndorseCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(int retryCount READ retryCount NOTIFY retryCountChanged)

public:
	explicit BuzzerEndorseCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString buzzer) {
		//
		allowRetry_ = false;
		retryCount_ = 0;
		//
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer.toStdString());
		command_->process(lArgs);
	}

	Q_INVOKABLE bool reprocess() {
		//
		if (command_ != nullptr && allowRetry_) {
			// we suppose that command_ was failed
			retryCount_++; emit retryCountChanged();
			return command_->retry();
		}

		return false;
	}

	Q_INVOKABLE int retryCount() { return retryCount_; }

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else {
			//
			if (result.error() == "E_SENT_TX" && result.message().find("UNKNOWN_REFTX") != std::string::npos) {
				//
				if (retryCount_ >= RETRY_MAX_COUNT) {
					emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
				} else {
					// can retry
					allowRetry_ = true;
					emit retry();
				}
			} else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

signals:
	void processed();
	void error(QString code, QString message);
	void retryCountChanged();
	void retry();

private:
	qbit::BuzzerEndorseCommandPtr command_;
	bool allowRetry_ = false;
	int retryCount_ = 0;
};

class BuzzerMistrustCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(int retryCount READ retryCount NOTIFY retryCountChanged)

public:
	explicit BuzzerMistrustCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString buzzer) {
		//
		allowRetry_ = false;
		retryCount_ = 0;
		//
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer.toStdString());
		command_->process(lArgs);
	}

	Q_INVOKABLE bool reprocess() {
		//
		if (command_ != nullptr && allowRetry_) {
			// we suppose that command_ was failed
			retryCount_++; emit retryCountChanged();
			return command_->retry();
		}

		return false;
	}

	Q_INVOKABLE int retryCount() { return retryCount_; }

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else {
			//
			if (result.error() == "E_SENT_TX" && result.message().find("UNKNOWN_REFTX") != std::string::npos) {
				//
				if (retryCount_ >= RETRY_MAX_COUNT) {
					emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
				} else {
					// can retry
					allowRetry_ = true;
					emit retry();
				}
			} else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

signals:
	void processed();
	void error(QString code, QString message);
	void retryCountChanged();
	void retry();

private:
	qbit::BuzzerMistrustCommandPtr command_;
	bool allowRetry_ = false;
	int retryCount_ = 0;
};

class DownloadMediaCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
	Q_PROPERTY(QString header READ header WRITE setHeader NOTIFY headerChanged)
	Q_PROPERTY(QString chain READ chain WRITE setChain NOTIFY chainChanged)
	Q_PROPERTY(QString localFile READ localFile WRITE setLocalFile NOTIFY localFileChanged)
	Q_PROPERTY(QString pkey READ pkey WRITE setPKey NOTIFY pKeyChanged)
	Q_PROPERTY(bool preview READ preview WRITE setPreview NOTIFY previewChanged)
	Q_PROPERTY(bool skipIfExists READ skipIfExists WRITE setSkipIfExists NOTIFY skipIfExistsChanged)

public:
	explicit DownloadMediaCommand(QObject* parent = nullptr);
	virtual ~DownloadMediaCommand();

	Q_INVOKABLE void process() {
		process(false);
	}

	Q_INVOKABLE void process(bool force) {
		// TODO: potential leak, need "check list" to track such objects
		QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

		std::vector<std::string> lArgs;
		lArgs.push_back(header_.toStdString() + "/" + chain_.toStdString());
		lArgs.push_back(localFile_.toStdString());
		if (pkey_.size()) { lArgs.push_back("-p"); lArgs.push_back(pkey_.toStdString()); }
		if (preview_) lArgs.push_back("-preview");
		if (skipIfExists_ && !force) lArgs.push_back("-skip");
		command_->process(lArgs);
	}

	Q_INVOKABLE void terminate() {
		// TODO: on terminate and on stop - make proper processing in cubix-download
		// command_->terminate();
	}

	void setUrl(const QString& url) {
		url_ = url;

		QStringList lList = url_.split('/');
		if (lList.size() == 2) {
			header_ = lList[0];
			chain_ = lList[1];
		}

		emit urlChanged();
	}
	QString url() const { return url_; }

	void setHeader(const QString& header) { header_ = header; emit headerChanged(); }
	QString header() const { return header_; }

	void setChain(const QString& chain) { chain_ = chain; emit chainChanged(); }
	QString chain() const { return chain_; }

	void setLocalFile(const QString& file) { localFile_ = file; emit localFileChanged(); }
	QString localFile() const { return localFile_; }

	void setPKey(const QString& pkey) { pkey_ = pkey; emit pKeyChanged(); }
	QString pkey() const { return pkey_; }

	void setPreview(bool preview) { preview_ = preview; emit previewChanged(); }
	bool preview() const { return preview_; }

	void setSkipIfExists(bool skip) { skipIfExists_ = skip; emit skipIfExistsChanged(); }
	bool skipIfExists() const { return skipIfExists_; }

	void done(qbit::TransactionPtr tx, const std::string& previewFile, const std::string& originalFile, unsigned short orientation, unsigned int duration, uint64_t size, unsigned short type, const qbit::ProcessingError& result) {
		if (result.success()) {
			QString lTx;
			QString lType = "unknown";
			unsigned int lDuration = duration;
			long long lSize = size;
			switch((qbit::cubix::TxMediaHeader::Type)type) {
				case qbit::cubix::TxMediaHeader::Type::UNKNOWN: lType = "unknown"; break;
				case qbit::cubix::TxMediaHeader::Type::IMAGE_PNG: lType = "image"; break;
				case qbit::cubix::TxMediaHeader::Type::IMAGE_JPEG: lType = "image"; break;
				case qbit::cubix::TxMediaHeader::Type::VIDEO_MJPEG: lType = "video"; break;
				case qbit::cubix::TxMediaHeader::Type::VIDEO_MP4: lType = "video"; break;
				case qbit::cubix::TxMediaHeader::Type::AUDIO_PCM: lType = "audio"; break;
				case qbit::cubix::TxMediaHeader::Type::AUDIO_AMR: lType = "audio"; break;
			}

			if (tx) {
				lTx = QString::fromStdString(tx->id().toHex());
				qbit::cubix::TxMediaHeaderPtr lHeader = qbit::TransactionHelper::to<qbit::cubix::TxMediaHeader>(tx);
				switch(lHeader->mediaType()) {
					case qbit::cubix::TxMediaHeader::Type::UNKNOWN: lType = "unknown"; break;
					case qbit::cubix::TxMediaHeader::Type::IMAGE_PNG: lType = "image"; break;
					case qbit::cubix::TxMediaHeader::Type::IMAGE_JPEG: lType = "image"; break;
					case qbit::cubix::TxMediaHeader::Type::VIDEO_MJPEG: lType = "video"; break;
					case qbit::cubix::TxMediaHeader::Type::VIDEO_MP4: lType = "video"; break;
					case qbit::cubix::TxMediaHeader::Type::AUDIO_PCM: lType = "audio"; break;
					case qbit::cubix::TxMediaHeader::Type::AUDIO_AMR: lType = "audio"; break;
				}

				lDuration = lHeader->duration();
				lSize = lHeader->size();
			}

#ifdef Q_OS_WINDOWS
			emit processed(lTx, QString("/") + QString::fromStdString(previewFile), QString("/") + QString::fromStdString(originalFile), orientation, lDuration, lSize, lType);
#else
			emit processed(lTx, QString::fromStdString(previewFile), QString::fromStdString(originalFile), orientation, lDuration, lSize, lType);
#endif
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

	void downloadProgress(uint64_t pos, uint64_t size) {
		emit progress(pos, size);
	}

signals:
	void processed(QString tx, QString previewFile, QString originalFile, unsigned short orientation, unsigned int duration, long long size, QString type);
	void progress(ulong pos, ulong size);
	void error(QString code, QString message);
	void urlChanged();
	void headerChanged();
	void chainChanged();
	void localFileChanged();
	void pKeyChanged();
	void previewChanged();
	void skipIfExistsChanged();

private:
	QString url_;
	QString header_;
	QString chain_;
	QString localFile_;
	QString pkey_;
	bool preview_;
	bool skipIfExists_;

	qbit::ICommandPtr command_;
};

class UploadMediaCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
	Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
	Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
	Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
	Q_PROPERTY(QString pkey READ pkey WRITE setPKey NOTIFY pKeyChanged)

public:
	explicit UploadMediaCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		std::vector<std::string> lArgs;
		lArgs.push_back(file_.toStdString());

		if (width_ && height_) {
			QString lDim = QString::number(width_) + "x" + QString::number(height_);
			lArgs.push_back("-s");
			lArgs.push_back(lDim.toStdString());
		}

		if (pkey_.size()) {
			lArgs.push_back("-p");
			lArgs.push_back(pkey_.toStdString());
		}

		if (description_.length())
			lArgs.push_back(description_.toStdString());

		command_->process(lArgs);
	}

	void setFile(const QString& file) { file_ = file; emit fileChanged(); }
	QString file() const { return file_; }

	void setWidth(int width) { width_ = width; emit widthChanged(); }
	int width() const { return width_; }

	void setHeight(int height) { height_ = height; emit heightChanged(); }
	int height() const { return height_; }

	void setDescription(const QString& desc) { description_ = desc; emit descriptionChanged(); }
	QString description() const { return description_; }

	void setPKey(const QString& pkey) { pkey_ = pkey; emit pKeyChanged(); }
	QString pkey() const { return pkey_; }

	void done(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) {
			QString lTx;
			if (tx) lTx = QString::fromStdString(tx->id().toHex());
			emit processed(lTx);
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

	void uploadProgress(const std::string& file, uint64_t pos, uint64_t size) {
		emit progress(QString::fromStdString(file), pos, size);
	}

	qbit::ICommandPtr command() { return command_; }
	void setDone(qbit::doneTransactionWithErrorFunction done) {
		std::static_pointer_cast<qbit::cubix::UploadMediaCommand>(command_)->setDone(done);
	}

signals:
	void processed(QString tx);
	void progress(QString file, ulong pos, ulong size);
	void error(QString code, QString message);
	void fileChanged();
	void widthChanged();
	void heightChanged();
	void descriptionChanged();
	void pKeyChanged();

private:
	QString file_;
	int width_ = 0;
	int height_ = 0;
	QString description_;
	QString pkey_;

	qbit::ICommandPtr command_;
};

class BuzzLikeCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit BuzzLikeCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString buzz) {
		//
		if (!buzzfeedModel_) return;

		//
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(buzz.toStdString());
		command_->process(lArgs);
	}

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; emit modelChanged(); }

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

signals:
	void modelChanged();
	void processed();
	void error(QString code, QString message);

private:
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::ICommandPtr command_;
};

class BuzzRewardCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)
	Q_PROPERTY(int retryCount READ retryCount NOTIFY retryCountChanged)

public:
	explicit BuzzRewardCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString buzz, QString reward) {
		//
		allowRetry_ = false;
		retryCount_ = 0;
		//
		if (!buzzfeedModel_) return;

		//
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(buzz.toStdString());
		lArgs.push_back(reward.toStdString());
		command_->process(lArgs);
	}

	Q_INVOKABLE bool reprocess() {
		//
		if (command_ != nullptr && allowRetry_) {
			// we suppose that command_ was failed
			retryCount_++; emit retryCountChanged();
			return command_->retry();
		}

		return false;
	}

	Q_INVOKABLE int retryCount() { return retryCount_; }

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; emit modelChanged(); }

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else {
			//
			if (result.error() == "E_SENT_TX" && result.message().find("UNKNOWN_REFTX") != std::string::npos) {
				//
				if (retryCount_ >= RETRY_MAX_COUNT) {
					emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
				} else {
					// can retry
					allowRetry_ = true;
					emit retry();
				}
			} else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

signals:
	void modelChanged();
	void processed();
	void error(QString code, QString message);
	void retryCountChanged();
	void retry();

private:
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::BuzzRewardCommandPtr command_;
	bool allowRetry_ = false;
	int retryCount_ = 0;
};

class BuzzCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzBody READ buzzBody WRITE setBuzzBody NOTIFY buzzBodyChanged)
	Q_PROPERTY(UploadMediaCommand* uploadCommand READ uploadCommand WRITE setUploadCommand NOTIFY uploadCommandChanged)
	Q_PROPERTY(int retryCount READ retryCount NOTIFY retryCountChanged)

public:
	explicit BuzzCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		//
		allowRetry_ = false;
		retryCount_ = 0;
		//
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzBody_.toStdString());

		for (QStringList::iterator lBuzzer = buzzers_.begin(); lBuzzer != buzzers_.end(); lBuzzer++) {
			lArgs.push_back((*lBuzzer).toStdString());
			qInfo() << *lBuzzer;
		}

		for (QStringList::iterator lMedia = buzzMedia_.begin(); lMedia != buzzMedia_.end(); lMedia++) {
			lArgs.push_back((*lMedia).toStdString());
			qInfo() << *lMedia;
		}

		command_->process(lArgs);
	}

	Q_INVOKABLE bool reprocess() {
		//
		if (command_ != nullptr && allowRetry_) {
			// we suppose that command_ was failed
			retryCount_++; emit retryCountChanged();
			return command_->retry();
		}

		return false;
	}

	Q_INVOKABLE int retryCount() { return retryCount_; }

	void setBuzzBody(const QString& buzzBody) { buzzBody_ = buzzBody; emit buzzBodyChanged(); }
	QString buzzBody() const { return buzzBody_; }

	UploadMediaCommand* uploadCommand() { return uploadCommand_; }
	void setUploadCommand(UploadMediaCommand* uploadCommand) { uploadCommand_ = uploadCommand; }

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else {
			//
			if (result.error() == "E_SENT_TX" && result.message().find("UNKNOWN_REFTX") != std::string::npos) {
				//
				if (retryCount_ >= RETRY_MAX_COUNT) {
					emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
				} else {
					// can retry
					allowRetry_ = true;
					emit retry();
				}
			} else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

	void mediaUploaded(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) {
			QString lTx;
			if (tx) lTx = QString::fromStdString(tx->id().toHex());
			emit mediaUploaded(lTx);
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

	Q_INVOKABLE void addBuzzer(QString buzzer) {
		buzzers_.push_back(buzzer);
	}

	Q_INVOKABLE void addMedia(QString media) {
		buzzMedia_.push_back(media);
	}

signals:
	void buzzBodyChanged();
	void uploadCommandChanged();
	void processed();
	void error(QString code, QString message);
	void mediaUploaded(QString tx);
	void retryChanged();
	void retryCountChanged();
	void retry();

private:
	QString buzzBody_;
	QStringList buzzers_;
	QStringList buzzMedia_;
	UploadMediaCommand* uploadCommand_ = nullptr;
	qbit::CreateBuzzCommandPtr command_;
	bool allowRetry_ = false;
	int retryCount_ = 0;
};

class ReBuzzCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzId READ buzzId WRITE setBuzzId NOTIFY buzzIdChanged)
	Q_PROPERTY(QString buzzBody READ buzzBody WRITE setBuzzBody NOTIFY buzzBodyChanged)
	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)
	Q_PROPERTY(UploadMediaCommand* uploadCommand READ uploadCommand WRITE setUploadCommand NOTIFY uploadCommandChanged)
	Q_PROPERTY(int retryCount READ retryCount NOTIFY retryCountChanged)

public:
	explicit ReBuzzCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		//
		allowRetry_ = false;
		retryCount_ = 0;
		//
		if (!buzzfeedModel_) return;

		//
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzId_.toStdString());
		lArgs.push_back(buzzBody_.toStdString());

		for (QStringList::iterator lBuzzer = buzzers_.begin(); lBuzzer != buzzers_.end(); lBuzzer++) {
			lArgs.push_back((*lBuzzer).toStdString());
		}

		for (QStringList::iterator lMedia = buzzMedia_.begin(); lMedia != buzzMedia_.end(); lMedia++) {
			lArgs.push_back((*lMedia).toStdString());
		}

		command_->process(lArgs);
	}

	Q_INVOKABLE bool reprocess() {
		//
		if (command_ != nullptr && allowRetry_) {
			// we suppose that command_ was failed
			retryCount_++; emit retryCountChanged();
			return command_->retry();
		}

		return false;
	}

	Q_INVOKABLE int retryCount() { return retryCount_; }

	void setBuzzId(const QString& buzzId) { buzzId_ = buzzId; emit buzzIdChanged(); }
	QString buzzId() const { return buzzId_; }

	void setBuzzBody(const QString& buzzBody) { buzzBody_ = buzzBody; emit buzzBodyChanged(); }
	QString buzzBody() const { return buzzBody_; }

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; emit modelChanged(); }

	UploadMediaCommand* uploadCommand() { return uploadCommand_; }
	void setUploadCommand(UploadMediaCommand* uploadCommand) { uploadCommand_ = uploadCommand; }

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else {
			//
			if (result.error() == "E_SENT_TX" && result.message().find("UNKNOWN_REFTX") != std::string::npos) {
				//
				if (retryCount_ >= RETRY_MAX_COUNT) {
					emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
				} else {
					// can retry
					allowRetry_ = true;
					emit retry();
				}
			} else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

	void mediaUploaded(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) {
			QString lTx;
			if (tx) lTx = QString::fromStdString(tx->id().toHex());
			emit mediaUploaded(lTx);
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

	Q_INVOKABLE void addBuzzer(QString buzzer) {
		buzzers_.push_back(buzzer);
	}

	Q_INVOKABLE void addMedia(QString media) {
		buzzMedia_.push_back(media);
	}

signals:
	void buzzIdChanged();
	void buzzBodyChanged();
	void modelChanged();
	void uploadCommandChanged();
	void processed();
	void error(QString code, QString message);
	void mediaUploaded(QString tx);
	void retryCountChanged();
	void retry();

private:
	QString buzzId_;
	QString buzzBody_;
	QStringList buzzers_;
	QStringList buzzMedia_;
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	UploadMediaCommand* uploadCommand_ = nullptr;
	qbit::CreateReBuzzCommandPtr command_;
	bool allowRetry_ = false;
	int retryCount_ = 0;
};

class ReplyCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzId READ buzzId WRITE setBuzzId NOTIFY buzzIdChanged)
	Q_PROPERTY(QString buzzBody READ buzzBody WRITE setBuzzBody NOTIFY buzzBodyChanged)
	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)
	Q_PROPERTY(UploadMediaCommand* uploadCommand READ uploadCommand WRITE setUploadCommand NOTIFY uploadCommandChanged)
	Q_PROPERTY(int retryCount READ retryCount NOTIFY retryCountChanged)

public:
	explicit ReplyCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		//
		allowRetry_ = false;
		retryCount_ = 0;
		//
		if (!buzzfeedModel_) return;

		//
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzId_.toStdString());
		lArgs.push_back(buzzBody_.toStdString());

		for (QStringList::iterator lBuzzer = buzzers_.begin(); lBuzzer != buzzers_.end(); lBuzzer++) {
			lArgs.push_back((*lBuzzer).toStdString());
		}

		for (QStringList::iterator lMedia = buzzMedia_.begin(); lMedia != buzzMedia_.end(); lMedia++) {
			lArgs.push_back((*lMedia).toStdString());
		}

		command_->process(lArgs);
	}

	Q_INVOKABLE bool reprocess() {
		//
		if (command_ != nullptr && allowRetry_) {
			// we suppose that command_ was failed
			retryCount_++; emit retryCountChanged();
			return command_->retry();
		}

		return false;
	}

	Q_INVOKABLE int retryCount() { return retryCount_; }

	void setBuzzId(const QString& buzzId) { buzzId_ = buzzId; emit buzzIdChanged(); }
	QString buzzId() const { return buzzId_; }

	void setBuzzBody(const QString& buzzBody) { buzzBody_ = buzzBody; emit buzzBodyChanged(); }
	QString buzzBody() const { return buzzBody_; }

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; emit modelChanged(); }

	UploadMediaCommand* uploadCommand() { return uploadCommand_; }
	void setUploadCommand(UploadMediaCommand* uploadCommand) { uploadCommand_ = uploadCommand; }

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else {
			//
			if (result.error() == "E_SENT_TX" && result.message().find("UNKNOWN_REFTX") != std::string::npos) {
				//
				if (retryCount_ >= RETRY_MAX_COUNT) {
					emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
				} else {
					// can retry
					allowRetry_ = true;
					emit retry();
				}
			} else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

	void mediaUploaded(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) {
			QString lTx;
			if (tx) lTx = QString::fromStdString(tx->id().toHex());
			emit mediaUploaded(lTx);
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

	Q_INVOKABLE void addBuzzer(QString buzzer) {
		buzzers_.push_back(buzzer);
	}

	Q_INVOKABLE void addMedia(QString media) {
		buzzMedia_.push_back(media);
	}

signals:
	void buzzIdChanged();
	void buzzBodyChanged();
	void modelChanged();
	void uploadCommandChanged();
	void processed();
	void error(QString code, QString message);
	void mediaUploaded(QString tx);
	void retryCountChanged();
	void retry();

private:
	QString buzzId_;
	QString buzzBody_;
	QStringList buzzers_;
	QStringList buzzMedia_;
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	UploadMediaCommand* uploadCommand_ = nullptr;
	qbit::CreateBuzzReplyCommandPtr command_;
	bool allowRetry_ = false;
	int retryCount_ = 0;
};

class BuzzSubscribeCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString chain READ chain WRITE setChain NOTIFY chainChanged)
	Q_PROPERTY(QString buzzId READ buzzId WRITE setBuzzId NOTIFY buzzIdChanged)
	Q_PROPERTY(long long nonce READ nonce NOTIFY nonceChanged)
	Q_PROPERTY(QString signature READ signature NOTIFY signatureChanged)
	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit BuzzSubscribeCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		//
		if (!buzzfeedModel_) return;

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(chain_.toStdString());
		lArgs.push_back(buzzId_.toStdString());

		nonce_ = QRandomGenerator::global()->generate64();
		lArgs.push_back(strprintf("%d", nonce_));

		//
		uint256 lId; lId.setHex(buzzId_.toStdString());
		buzzfeedModel_->buzzfeed()->setRootBuzzId(lId);
		buzzfeedModel_->buzzfeed()->setNonce(nonce_);

		// process
		command_->process(lArgs);

		emit nonceChanged();
		emit signatureChanged();
	}

	void setChain(const QString& chain) { chain_ = chain; emit chainChanged(); }
	QString chain() const { return chain_; }

	void setBuzzId(const QString& buzzId) { buzzId_ = buzzId; emit buzzIdChanged(); }
	QString buzzId() const { return buzzId_; }

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; emit modelChanged(); }

	long long nonce() { return nonce_; }
	QString signature() { return QString::fromStdString(signature_.toHex()); }

	void done(const qbit::ProcessingError& result) {
		if (result.success()) {
			//
			signature_ = command_->signature();

			// register subscription
			Client* lClient = static_cast<Client*>(gApplication->getClient());
			lClient->getBuzzerComposer()->buzzer()->registerSubscription(signature_, buzzfeedModel_->buzzfeed());

			// extract peers
			for (std::list<uint160>::const_iterator lPeer = command_->peers().begin(); lPeer != command_->peers().end(); lPeer++) {
				//
				peers_.push_back(QString::fromStdString((*lPeer).toHex()));
			}

			emit processed();
		} else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

	Q_INVOKABLE QStringList peers() { return peers_; }

signals:
	void chainChanged();
	void buzzIdChanged();
	void modelChanged();
	void nonceChanged();
	void signatureChanged();
	void processed();
	void error(QString code, QString message);

private:
	QString chain_;
	QString buzzId_;
	long long nonce_;
	uint512 signature_;
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::BuzzSubscribeCommandPtr command_;
	QStringList peers_;
};

class BuzzUnsubscribeCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(BuzzSubscribeCommand* subscribeCommand READ subscribeCommand WRITE setSubscribeCommand NOTIFY subscribeCommandChanged)

public:
	explicit BuzzUnsubscribeCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		//
		if (!subscribeCommand_) return;

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(subscribeCommand_->chain().toStdString());
		lArgs.push_back(subscribeCommand_->buzzId().toStdString());

		QStringList lPeers = subscribeCommand_->peers();
		for (QStringList::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
			lArgs.push_back((*lPeer).toStdString());
		}

		command_->process(lArgs);
	}

	BuzzSubscribeCommand* subscribeCommand() { return subscribeCommand_; }
	void setSubscribeCommand(BuzzSubscribeCommand* subscribeCommand) { subscribeCommand_ = subscribeCommand; emit subscribeCommandChanged(); }

	void done(const qbit::ProcessingError& result) {
		if (result.success()) {
			// remove subscription
			Client* lClient = static_cast<Client*>(gApplication->getClient());
			uint512 lSubscription; lSubscription.setHex(subscribeCommand_->signature().toStdString());
			lClient->getBuzzerComposer()->buzzer()->removeSubscription(lSubscription);

			emit processed();
		} else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
	}

signals:
	void subscribeCommandChanged();
	void processed();
	void error(QString code, QString message);

private:
	BuzzSubscribeCommand* subscribeCommand_;
	qbit::BuzzUnsubscribeCommandPtr command_;
};

class LoadBuzzerInfoCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzerId READ buzzerId NOTIFY buzzerIdChanged)
	Q_PROPERTY(QString infoId READ infoId NOTIFY infoIdChanged)
	Q_PROPERTY(QString buzzerChainId READ buzzerChainId NOTIFY buzzerChainIdChanged)
	Q_PROPERTY(QString pkey READ pkey NOTIFY pkeyChanged)
	Q_PROPERTY(QString alias READ alias NOTIFY aliasChanged)
	Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
	Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY avatarUrlChanged)
	Q_PROPERTY(QString avatarId READ avatarId NOTIFY avatarIdChanged)
	Q_PROPERTY(QString headerUrl READ headerUrl NOTIFY headerUrlChanged)
	Q_PROPERTY(QString headerId READ headerId NOTIFY headerIdChanged)
	Q_PROPERTY(bool loadUtxo READ loadUtxo WRITE setLoadUtxo NOTIFY headerIdChanged)

public:
	explicit LoadBuzzerInfoCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString buzzer) {
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer.toStdString());
		if (loadUtxo_) lArgs.push_back("-utxo");
		command_->process(lArgs);
	}

	Q_INVOKABLE bool updateLocalBuzzer() {
		//
		if (buzzer_ != nullptr && info_ != nullptr && outs_.size()) {
			//
			Client* lClient = static_cast<Client*>(gApplication->getClient());
			lClient->getBuzzerComposer()->writeBuzzerTx(qbit::TransactionHelper::to<qbit::TxBuzzer>(buzzer_));
			lClient->getBuzzerComposer()->writeBuzzerInfoTx(info_);

			std::vector<qbit::Transaction::UnlinkedOut> lOuts;
			for (std::vector<qbit::Transaction::NetworkUnlinkedOut>::iterator lOut = outs_.begin(); lOut != outs_.end(); lOut++) {
				//
				lOuts.push_back(lOut->utxo());
			}

			lClient->getBuzzerComposer()->writeBuzzerUtxo(lOuts);

			qbit::PKey lPKey;
			if (info_->extractAddress(lPKey)) {
				lClient->getBuzzerComposer()->addSubscription(buzzer_->id(), lPKey); // recover at least yourself subscription
			}

			// TODO: lazy subcription recovering - if buzz apeared in list and there is no subscription, try
			// to load subscription and if subscription exists - add it

			return true;
		}

		return false;
	}

	QString buzzerId() {
		if (buzzer_) {
			return QString::fromStdString(buzzer_->id().toHex());
		}

		return QString();
	}

	QString infoId() {
		if (info_) {
			return QString::fromStdString(info_->id().toHex());
		}

		return QString();
	}

	QString buzzerChainId() {
		if (info_) {
			return QString::fromStdString(info_->chain().toHex());
		}

		return QString();
	}

	QString avatarUrl() {
		if (info_) {
			return QString::fromStdString(info_->avatar().url());
		}

		return QString();
	}

	QString avatarId() {
		if (info_) {
			return QString::fromStdString(info_->avatar().tx().toHex());
		}

		return QString();
	}

	QString headerUrl() {
		if (info_) {
			return QString::fromStdString(info_->header().url());
		}

		return QString();
	}

	QString headerId() {
		if (info_) {
			return QString::fromStdString(info_->header().tx().toHex());
		}

		return QString();
	}

	QString alias() {
		if (info_) {
			std::string lAlias;
			lAlias.insert(lAlias.end(), info_->alias().begin(), info_->alias().end());
			return QString::fromStdString(lAlias);
		}

		return QString();
	}

	QString description() {
		if (info_) {
			std::string lDescription;
			lDescription.insert(lDescription.end(), info_->description().begin(), info_->description().end());
			return QString::fromStdString(lDescription);
		}

		return QString();
	}

	QString pkey() {
		if (info_) {
			qbit::PKey lPKey;
			if (info_->extractAddress(lPKey)) {
				//
				return QString::fromStdString(lPKey.toString());
			}
		}

		return QString();
	}

	bool loadUtxo() { return loadUtxo_; }
	void setLoadUtxo(bool loadUtxo) { loadUtxo_ = loadUtxo; emit loadUtxoChanged(); }

	void done(qbit::EntityPtr buzzer, qbit::TransactionPtr info, const std::vector<qbit::Transaction::NetworkUnlinkedOut>& outs, const std::string& name, const qbit::ProcessingError& result) {
		if (result.success()) {
			buzzer_ = buzzer;
			info_ = qbit::TransactionHelper::to<qbit::TxBuzzerInfo>(info);
			outs_ = outs;
			emit processed(QString::fromStdString(name));
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

signals:
	void processed(QString name);
	void error(QString code, QString message);
	void aliasChanged();
	void pkeyChanged();
	void descriptionChanged();
	void buzzerIdChanged();
	void avatarUrlChanged();
	void headerUrlChanged();
	void avatarIdChanged();
	void headerIdChanged();
	void buzzerChainIdChanged();
	void loadUtxoChanged();
	void infoIdChanged();

private:
	qbit::LoadBuzzerInfoCommandPtr command_;
	qbit::EntityPtr buzzer_;
	std::vector<qbit::Transaction::NetworkUnlinkedOut> outs_;
	qbit::TxBuzzerInfoPtr info_;
	bool loadUtxo_ = false;
};

class LoadEventsfeedCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(EventsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadEventsfeedCommand(QObject* parent = nullptr);
	virtual ~LoadEventsfeedCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = false;

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();
		//
		std::vector<std::string> lArgs;
		if (more) lArgs.push_back("more");
		command_->process(lArgs);
	}

	Q_INVOKABLE void processAndMerge() {
		processAndMerge(false);
	}

	Q_INVOKABLE void processAndMerge(bool more) {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = true;

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();
		//
		std::vector<std::string> lArgs;
		if (more) lArgs.push_back("more");
		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	EventsfeedListModel* model() { return eventsfeedModel_; }
	void setModel(EventsfeedListModel* model) { eventsfeedModel_ = model; }

	void ready(qbit::EventsfeedPtr base, qbit::EventsfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::EventsfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);

private:
	EventsfeedListModel* eventsfeedModel_ = nullptr;
	qbit::LoadEventsfeedCommandPtr command_ = nullptr;
	bool more_ = false;
	bool merge_ = false;
};

class LoadEndorsementsByBuzzerCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzer READ buzzer WRITE setBuzzer NOTIFY buzzerChanged)
	Q_PROPERTY(EventsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadEndorsementsByBuzzerCommand(QObject* parent = nullptr);
	virtual ~LoadEndorsementsByBuzzerCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = false;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
		if (more) lArgs.push_back("more");

		command_->process(lArgs);
	}

	Q_INVOKABLE void processAndMerge() {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = false;
		merge_ = true;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	void setBuzzer(const QString& buzzer) { buzzer_ = buzzer; emit buzzerChanged(); }
	QString buzzer() const { return buzzer_; }

	EventsfeedListModel* model() { return eventsfeedModel_; }
	void setModel(EventsfeedListModel* model) { eventsfeedModel_ = model; }

	void ready(qbit::EventsfeedPtr base, qbit::EventsfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::EventsfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);
	void buzzerChanged();

private:
	EventsfeedListModel* eventsfeedModel_ = nullptr;
	qbit::LoadEndorsementsByBuzzerCommandPtr command_ = nullptr;
	QString buzzer_;
	bool more_ = false;
	bool merge_ = false;
};

class LoadMistrustsByBuzzerCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzer READ buzzer WRITE setBuzzer NOTIFY buzzerChanged)
	Q_PROPERTY(EventsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadMistrustsByBuzzerCommand(QObject* parent = nullptr);
	virtual ~LoadMistrustsByBuzzerCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = false;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
		if (more) lArgs.push_back("more");

		command_->process(lArgs);
	}

	Q_INVOKABLE void processAndMerge() {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = false;
		merge_ = true;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	void setBuzzer(const QString& buzzer) { buzzer_ = buzzer; emit buzzerChanged(); }
	QString buzzer() const { return buzzer_; }

	EventsfeedListModel* model() { return eventsfeedModel_; }
	void setModel(EventsfeedListModel* model) { eventsfeedModel_ = model; }

	void ready(qbit::EventsfeedPtr base, qbit::EventsfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::EventsfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);
	void buzzerChanged();

private:
	EventsfeedListModel* eventsfeedModel_ = nullptr;
	qbit::LoadMistrustsByBuzzerCommandPtr command_ = nullptr;
	QString buzzer_;
	bool more_ = false;
	bool merge_ = false;
};

class LoadFollowingByBuzzerCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzer READ buzzer WRITE setBuzzer NOTIFY buzzerChanged)
	Q_PROPERTY(EventsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadFollowingByBuzzerCommand(QObject* parent = nullptr);
	virtual ~LoadFollowingByBuzzerCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = false;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
		if (more) lArgs.push_back("more");

		command_->process(lArgs);
	}

	Q_INVOKABLE void processAndMerge() {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = false;
		merge_ = true;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	void setBuzzer(const QString& buzzer) { buzzer_ = buzzer; emit buzzerChanged(); }
	QString buzzer() const { return buzzer_; }

	EventsfeedListModel* model() { return eventsfeedModel_; }
	void setModel(EventsfeedListModel* model) { eventsfeedModel_ = model; }

	void ready(qbit::EventsfeedPtr base, qbit::EventsfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::EventsfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);
	void buzzerChanged();

private:
	EventsfeedListModel* eventsfeedModel_ = nullptr;
	qbit::LoadSubscriptionsByBuzzerCommandPtr command_ = nullptr;
	QString buzzer_;
	bool more_ = false;
	bool merge_ = false;
};

class LoadFollowersByBuzzerCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzer READ buzzer WRITE setBuzzer NOTIFY buzzerChanged)
	Q_PROPERTY(EventsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadFollowersByBuzzerCommand(QObject* parent = nullptr);
	virtual ~LoadFollowersByBuzzerCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = false;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
		if (more) lArgs.push_back("more");

		command_->process(lArgs);
	}

	Q_INVOKABLE void processAndMerge() {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = false;
		merge_ = true;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	void setBuzzer(const QString& buzzer) { buzzer_ = buzzer; emit buzzerChanged(); }
	QString buzzer() const { return buzzer_; }

	EventsfeedListModel* model() { return eventsfeedModel_; }
	void setModel(EventsfeedListModel* model) { eventsfeedModel_ = model; }

	void ready(qbit::EventsfeedPtr base, qbit::EventsfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::EventsfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);
	void buzzerChanged();

private:
	EventsfeedListModel* eventsfeedModel_ = nullptr;
	qbit::LoadFollowersByBuzzerCommandPtr command_ = nullptr;
	QString buzzer_;
	bool more_ = false;
	bool merge_ = false;
};

class LoadTransactionCommand: public QObject
{
	Q_OBJECT

public:
	explicit LoadTransactionCommand(QObject* parent = nullptr);
	virtual ~LoadTransactionCommand() {
		if (command_) command_->terminate();
	}

	Q_INVOKABLE void process(QString tx, QString chain) {
		QString lArg = tx + "/" + chain;
		std::vector<std::string> lArgs;
		lArgs.push_back(lArg.toStdString());

		command_->process(lArgs);
	}

	Q_INVOKABLE void updateWalletInfo() {
		//
		if (tx_) {
			Client* lClient = static_cast<Client*>(gApplication->getClient());
			lClient->getComposer()->wallet()->updateOuts(tx_);
		}
	}

	void done(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) {
			if (tx) {
				// TODO: check integrity (tx->id() == params.tx)
				tx_ = tx;
				emit processed(QString::fromStdString(tx->id().toHex()), QString::fromStdString(tx->chain().toHex()));
			} else emit transactionNotFound();
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

signals:
	void processed(QString tx, QString chain);
	void error(QString code, QString message);
	void transactionNotFound();

private:
	qbit::ICommandPtr command_;
	qbit::TransactionPtr tx_;
};

class LoadEntityCommand: public QObject
{
	Q_OBJECT

public:
	explicit LoadEntityCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString entity) {
		std::vector<std::string> lArgs;
		lArgs.push_back(entity.toStdString());

		command_->process(lArgs);
	}

	Q_INVOKABLE QString extractAddress();

	void done(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) {
			if (tx) {
				// TODO: check integrity (tx->id() == params.tx)
				tx_ = tx;
				emit processed(QString::fromStdString(tx->id().toHex()), QString::fromStdString(tx->chain().toHex()));
			} else emit transactionNotFound();
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

signals:
	void processed(QString tx, QString chain);
	void error(QString code, QString message);
	void transactionNotFound();

private:
	qbit::ICommandPtr command_;
	qbit::TransactionPtr tx_;
};

class SendToAddressCommand: public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool manualProcessing READ manualProcessing WRITE setManualProcessing NOTIFY manualProcessingChanged)
	Q_PROPERTY(bool privateSend READ privateSend WRITE setPrivateSend NOTIFY privateSendChanged)

public:
	explicit SendToAddressCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString asset, QString address, QString amount) {
		//
		process(asset, address, amount, QString());
	}

	Q_INVOKABLE void process(QString asset, QString address, QString amount, QString feeRate) {
		//
		makeCommand();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(asset.toStdString());
		lArgs.push_back(address.toStdString());
		lArgs.push_back(amount.toStdString());
		if (feeRate.size()) lArgs.push_back(feeRate.toStdString());

		command_->process(lArgs);
	}

	bool manualProcessing() {
		return manualProcessing_;
	}

	void setManualProcessing(bool manual) {
		manualProcessing_ = manual;
		emit manualProcessingChanged();
	}

	bool privateSend() {
		return privateSend_;
	}

	void setPrivateSend(bool privateSend) {
		privateSend_ = privateSend;
		emit privateSendChanged();
	}

	Q_INVOKABLE void broadcast() {
		//
		if (command_)
			command_->broadcast();
	}

	Q_INVOKABLE void rollback() {
		//
		if (command_)
			command_->rollback();
	}

	Q_INVOKABLE QString finalAmount() {
		//
		if (command_)
			return QString::fromStdString(command_->finalAmountFormatted());
		return "0.00000000";
	}

	void done(bool broadcasted, const qbit::ProcessingError& result) {
		if (result.success()) {
			emit processed(broadcasted);
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

signals:
	void processed(bool broadcasted);
	void error(QString code, QString message);
	void transactionNotFound();
	void manualProcessingChanged();
	void privateSendChanged();

private:
	void makeCommand() {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		// if previous tx was prepared
		if (command_) {
			command_->rollback();
		}

		if (!privateSend_) {
			command_ = std::static_pointer_cast<qbit::SendToAddressCommand>(qbit::SendToAddressCommand::instance(
				lClient->getComposer(), manualProcessing_,
				boost::bind(&SendToAddressCommand::done, this, boost::placeholders::_1, boost::placeholders::_2))
			);
		} else {
			command_ = std::static_pointer_cast<qbit::SendToAddressCommand>(qbit::SendPrivateToAddressCommand::instance(
				lClient->getComposer(), manualProcessing_,
				boost::bind(&SendToAddressCommand::done, this, boost::placeholders::_1, boost::placeholders::_2))
			);
		}
	}

private:
	qbit::SendToAddressCommandPtr command_;
	bool manualProcessing_ = false;
	bool privateSend_ = false;
};

class LoadConversationsCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(ConversationsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadConversationsCommand(QObject* parent = nullptr);
	virtual ~LoadConversationsCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!conversationsModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = false;

		//
		if (!more_) conversationsModel_->conversations()->clear();
		//
		std::vector<std::string> lArgs;
		if (more) lArgs.push_back("more");
		command_->process(lArgs);
	}

	Q_INVOKABLE void processAndMerge() {
		//
		if (!conversationsModel_) return;

		// prepare command
		prepare();

		// params
		more_ = false;
		merge_ = true;

		//
		if (!more_) conversationsModel_->conversations()->clear();
		//
		std::vector<std::string> lArgs;
		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	ConversationsfeedListModel* model() { return conversationsModel_; }
	void setModel(ConversationsfeedListModel* model) {
		conversationsModel_ = model;
		emit modelChanged();
	}

	void ready(qbit::ConversationsfeedPtr base, qbit::ConversationsfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::ConversationsfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);

private:
	ConversationsfeedListModel* conversationsModel_ = nullptr;
	qbit::LoadConversationsCommandPtr command_ = nullptr;
	bool more_ = false;
	bool merge_ = false;
};

class CreateConversationCommand: public QObject
{
	Q_OBJECT

public:
	explicit CreateConversationCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString buzzer) {
		//
		buzzer_ = buzzer;
		//
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer.toStdString());
		command_->process(lArgs);
	}

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed(buzzer_);
	}

signals:
	void processed(QString buzzer);
	void error(QString code, QString message);

private:
	qbit::CreateBuzzerConversationCommandPtr command_ = nullptr;
	QString buzzer_;
};

class AcceptConversationCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(ConversationsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit AcceptConversationCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString conversation) {
		//
		if (!conversationsfeedModel_) return;

		// prepare command
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(conversation.toStdString());
		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	ConversationsfeedListModel* model() { return conversationsfeedModel_; }
	void setModel(ConversationsfeedListModel* model) { conversationsfeedModel_ = model; }

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();

private:
	ConversationsfeedListModel* conversationsfeedModel_ = nullptr;
	qbit::AcceptConversationCommandPtr command_ = nullptr;
};

class DeclineConversationCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(ConversationsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit DeclineConversationCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString conversation) {
		//
		if (!conversationsfeedModel_) return;

		// prepare command
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(conversation.toStdString());
		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	ConversationsfeedListModel* model() { return conversationsfeedModel_; }
	void setModel(ConversationsfeedListModel* model) { conversationsfeedModel_ = model; }

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();

private:
	ConversationsfeedListModel* conversationsfeedModel_ = nullptr;
	qbit::DeclineConversationCommandPtr command_ = nullptr;
};

class DecryptMessageBodyCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(ConversationsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit DecryptMessageBodyCommand(QObject* parent = nullptr);
	virtual ~DecryptMessageBodyCommand() {
		if (command_) command_->terminate();
	}

	Q_INVOKABLE void process(QString conversation) {
		//
		if (!conversationsfeedModel_) return;

		// prepare command
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(conversation.toStdString());
		command_->process(lArgs);
	}

	Q_INVOKABLE void processBody(QString conversation, QString body) {
		//
		if (!conversationsfeedModel_) return;

		// prepare command
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(conversation.toStdString());
		lArgs.push_back(body.toStdString());
		command_->process(lArgs);
	}

	void prepare();

	void done(const std::string& key, const std::string& body, const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed(QString::fromStdString(key), QString::fromStdString(body));
	}

	ConversationsfeedListModel* model() { return conversationsfeedModel_; }
	void setModel(ConversationsfeedListModel* model) { conversationsfeedModel_ = model; emit modelChanged(); }

signals:
	void processed(QString key, QString body);
	void error(QString code, QString message);
	void modelChanged();

private:
	ConversationsfeedListModel* conversationsfeedModel_ = nullptr;
	qbit::DecryptMessageBodyCommandPtr command_ = nullptr;
};

class DecryptBuzzerMessageCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit DecryptBuzzerMessageCommand(QObject* parent = nullptr);
	virtual ~DecryptBuzzerMessageCommand() {
		if (command_) command_->terminate();
	}

	Q_INVOKABLE void process(QString id) {
		//
		if (!conversationModel_) return;

		// prepare command
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(id.toStdString());
		command_->process(lArgs);
	}

	void prepare();

	void done(const std::string& key, const std::string& body, const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed(QString::fromStdString(key), QString::fromStdString(body));
	}

	BuzzfeedListModel* model() { return conversationModel_; }
	void setModel(BuzzfeedListModel* model) { conversationModel_ = model; emit modelChanged(); }

signals:
	void processed(QString key, QString body);
	void error(QString code, QString message);
	void modelChanged();

private:
	BuzzfeedListModel* conversationModel_ = nullptr;
	qbit::DecryptBuzzerMessageCommandPtr command_ = nullptr;
};

class LoadConversationMessagesCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString conversationId READ conversationId WRITE setConversationId NOTIFY conversationIdChanged)
	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadConversationMessagesCommand(QObject* parent = nullptr);
	virtual ~LoadConversationMessagesCommand();

	Q_INVOKABLE void process(bool more) {
		//
		if (!buzzfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = false;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) buzzfeedModel_->buzzfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(conversationId_.toStdString());
		if (more) lArgs.push_back("more");

		command_->process(lArgs);
	}

	Q_INVOKABLE void processAndMerge() {
		processAndMerge(false);
	}

	Q_INVOKABLE void processAndMerge(bool more) {
		//
		if (!buzzfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;
		merge_ = true;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) buzzfeedModel_->buzzfeed()->clear();

		std::vector<std::string> lArgs;
		if (more) lArgs.push_back("more");
		lArgs.push_back(conversationId_.toStdString());

		command_->process(lArgs);
	}

	void prepare();

	Q_INVOKABLE void reset() {
		Client* lClient = static_cast<Client*>(gApplication->getClient());
		lClient->getBuzzerComposer()->buzzer()->setConversation(nullptr);
	}

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

	void setConversationId(const QString& conversationId) { conversationId_ = conversationId; emit conversationIdChanged(); }
	QString conversationId() const { return conversationId_; }

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; emit modelChanged(); }

	void ready(qbit::BuzzfeedPtr base, qbit::BuzzfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::BuzzfeedProxy& /*local*/, bool /*more*/, bool /*merge*/);
	void conversationIdChanged();

private:
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::LoadMessagesCommandPtr command_ = nullptr;
	QString conversationId_;
	bool more_ = false;
	bool merge_ = false;
};

class ConversationMessageCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString conversation READ conversation WRITE setConversation NOTIFY conversationChanged)
	Q_PROPERTY(QString messageBody READ messageBody WRITE setMessageBody NOTIFY messageBodyChanged)
	Q_PROPERTY(UploadMediaCommand* uploadCommand READ uploadCommand WRITE setUploadCommand NOTIFY uploadCommandChanged)
	Q_PROPERTY(int retryCount READ retryCount NOTIFY retryCountChanged)

public:
	explicit ConversationMessageCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		//
		allowRetry_ = false;
		retryCount_ = 0;
		//
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(conversation_.toStdString());
		lArgs.push_back(messageBody_.toStdString());

		for (QStringList::iterator lBuzzer = buzzers_.begin(); lBuzzer != buzzers_.end(); lBuzzer++) {
			lArgs.push_back((*lBuzzer).toStdString());
			qInfo() << *lBuzzer;
		}

		for (QStringList::iterator lMedia = messageMedia_.begin(); lMedia != messageMedia_.end(); lMedia++) {
			lArgs.push_back((*lMedia).toStdString());
			qInfo() << *lMedia;
		}

		command_->process(lArgs);
	}

	Q_INVOKABLE bool reprocess() {
		//
		if (command_ != nullptr && allowRetry_) {
			// we suppose that command_ was failed
			retryCount_++; emit retryCountChanged();
			return command_->retry();
		}

		return false;
	}

	Q_INVOKABLE int retryCount() { return retryCount_; }

	void setMessageBody(const QString& messageBody) { messageBody_ = messageBody; emit messageBodyChanged(); }
	QString messageBody() const { return messageBody_; }

	void setConversation(const QString& conversation) { conversation_ = conversation; emit conversationChanged(); }
	QString conversation() const { return conversation_; }

	UploadMediaCommand* uploadCommand() { return uploadCommand_; }
	void setUploadCommand(UploadMediaCommand* uploadCommand) { uploadCommand_ = uploadCommand; emit uploadCommandChanged(); }

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else {
			//
			if (result.error() == "E_SENT_TX" && result.message().find("UNKNOWN_REFTX") != std::string::npos) {
				//
				if (retryCount_ >= RETRY_MAX_COUNT) {
					emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
				} else {
					// can retry
					allowRetry_ = true;
					emit retry();
				}
			} else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

	void mediaUploaded(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) {
			QString lTx;
			if (tx) lTx = QString::fromStdString(tx->id().toHex());
			emit mediaUploaded(lTx);
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

	Q_INVOKABLE void addBuzzer(QString buzzer) {
		buzzers_.push_back(buzzer);
	}

	Q_INVOKABLE void addMedia(QString media) {
		messageMedia_.push_back(media);
	}

signals:
	void conversationChanged();
	void messageBodyChanged();
	void uploadCommandChanged();
	void processed();
	void error(QString code, QString message);
	void mediaUploaded(QString tx);
	void retryCountChanged();
	void retry();

private:
	QString conversation_;
	QString messageBody_;
	QStringList buzzers_;
	QStringList messageMedia_;
	UploadMediaCommand* uploadCommand_ = nullptr;
	qbit::CreateBuzzerMessageCommandPtr command_;
	bool allowRetry_ = false;
	int retryCount_ = 0;
};

class LoadCounterpartyKeyCommand: public QObject
{
	Q_OBJECT

public:
	explicit LoadCounterpartyKeyCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString id) {
		// prepare command
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(id.toStdString());
		command_->process(lArgs);
	}

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (!result.success()) {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
			return;
		}

		emit processed();
	}

signals:
	void processed();
	void error(QString code, QString message);

private:
	BuzzfeedListModel* conversationModel_ = nullptr;
	qbit::LoadCounterpartyKeyCommandPtr command_ = nullptr;
};

} // buzzer

#endif // BUZZER_COMMAND_WRAPPERS_H
