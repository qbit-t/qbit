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

#include "iapplication.h"
#include "buzzfeedlistmodel.h"
#include "eventsfeedlistmodel.h"

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

class LoadBuzzerTrustScoreCommand: public QObject
{
	Q_OBJECT

public:
	explicit LoadBuzzerTrustScoreCommand(QObject* parent = nullptr);

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
	void dataReady(const qbit::BuzzfeedProxy& /*local*/, bool /*more*/);

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
	void dataReady(const qbit::BuzzfeedProxy& /*local*/, bool /*more*/);
	void buzzIdChanged();

private:
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::LoadBuzzfeedByBuzzCommandPtr command_ = nullptr;
	QString buzzId_;
	bool more_ = false;
};

class LoadBuzzfeedByBuzzerCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzer READ buzzer WRITE setBuzzer NOTIFY buzzerChanged)
	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadBuzzfeedByBuzzerCommand(QObject* parent = nullptr);

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
		lArgs.push_back(buzzer_.toStdString());
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

	void setBuzzer(const QString& buzzer) { buzzer_ = buzzer; emit buzzerChanged(); }
	QString buzzer() const { return buzzer_; }

	BuzzfeedListModel* model() { return buzzfeedModel_; }
	void setModel(BuzzfeedListModel* model) { buzzfeedModel_ = model; }

	void ready(qbit::BuzzfeedPtr base, qbit::BuzzfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::BuzzfeedProxy& /*local*/, bool /*more*/);
	void buzzerChanged();

private:
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::LoadBuzzfeedByBuzzerCommandPtr command_ = nullptr;
	QString buzzer_;
	bool more_ = false;
};

class LoadBuzzfeedByTagCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString tag READ tag WRITE setTag NOTIFY tagChanged)
	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadBuzzfeedByTagCommand(QObject* parent = nullptr);

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
	void dataReady(const qbit::BuzzfeedProxy& /*local*/, bool /*more*/);
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

	Q_INVOKABLE void process(bool more) {
		//
		if (!buzzfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;

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
	void dataReady(const qbit::BuzzfeedProxy& /*local*/, bool /*more*/);

private:
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	qbit::LoadBuzzfeedCommandPtr command_ = nullptr;
	bool more_ = false;
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

public:
	explicit BuzzerEndorseCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString buzzer) {
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer.toStdString());
		command_->process(lArgs);
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

class BuzzerMistrustCommand: public QObject
{
	Q_OBJECT

public:
	explicit BuzzerMistrustCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString buzzer) {
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer.toStdString());
		command_->process(lArgs);
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

class DownloadMediaCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
	Q_PROPERTY(QString header READ header WRITE setHeader NOTIFY headerChanged)
	Q_PROPERTY(QString chain READ chain WRITE setChain NOTIFY chainChanged)
	Q_PROPERTY(QString localFile READ localFile WRITE setLocalFile NOTIFY localFileChanged)
	Q_PROPERTY(bool preview READ preview WRITE setPreview NOTIFY previewChanged)
	Q_PROPERTY(bool skipIfExists READ skipIfExists WRITE setSkipIfExists NOTIFY skipIfExistsChanged)

public:
	explicit DownloadMediaCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		std::vector<std::string> lArgs;
		lArgs.push_back(header_.toStdString() + "/" + chain_.toStdString());
		lArgs.push_back(localFile_.toStdString());
		if (preview_) lArgs.push_back("preview");
		if (skipIfExists_) lArgs.push_back("skip");
		command_->process(lArgs);
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

	void setPreview(bool preview) { preview_ = preview; emit previewChanged(); }
	bool preview() const { return preview_; }

	void setSkipIfExists(bool skip) { skipIfExists_ = skip; emit skipIfExistsChanged(); }
	bool skipIfExists() const { return skipIfExists_; }

	void done(qbit::TransactionPtr tx, const std::string& previewFile, const std::string& originalFile, unsigned short orientation, const qbit::ProcessingError& result) {
		if (result.success()) {
			QString lTx;
			if (tx) lTx = QString::fromStdString(tx->id().toHex());
			emit processed(lTx, QString::fromStdString(previewFile), QString::fromStdString(originalFile), orientation);
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

	void downloadProgress(uint64_t pos, uint64_t size) {
		emit progress(pos, size);
	}

signals:
	void processed(QString tx, QString previewFile, QString originalFile, unsigned short orientation);
	void progress(ulong pos, ulong size);
	void error(QString code, QString message);
	void urlChanged();
	void headerChanged();
	void chainChanged();
	void localFileChanged();
	void previewChanged();
	void skipIfExistsChanged();

private:
	QString url_;
	QString header_;
	QString chain_;
	QString localFile_;
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

private:
	QString file_;
	int width_ = 0;
	int height_ = 0;
	QString description_;

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

public:
	explicit BuzzRewardCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString buzz, QString reward) {
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

class BuzzCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzBody READ buzzBody WRITE setBuzzBody NOTIFY buzzBodyChanged)
	Q_PROPERTY(UploadMediaCommand* uploadCommand READ uploadCommand WRITE setUploadCommand NOTIFY uploadCommandChanged)

public:
	explicit BuzzCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
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

	void setBuzzBody(const QString& buzzBody) { buzzBody_ = buzzBody; emit buzzBodyChanged(); }
	QString buzzBody() const { return buzzBody_; }

	UploadMediaCommand* uploadCommand() { return uploadCommand_; }
	void setUploadCommand(UploadMediaCommand* uploadCommand) { uploadCommand_ = uploadCommand; }

	void prepare();

	void done(const qbit::ProcessingError& result) {
		if (result.success()) emit processed();
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
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

private:
	QString buzzBody_;
	QStringList buzzers_;
	QStringList buzzMedia_;
	UploadMediaCommand* uploadCommand_ = nullptr;
	qbit::CreateBuzzCommandPtr command_;
};

class ReBuzzCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzId READ buzzId WRITE setBuzzId NOTIFY buzzIdChanged)
	Q_PROPERTY(QString buzzBody READ buzzBody WRITE setBuzzBody NOTIFY buzzBodyChanged)
	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)
	Q_PROPERTY(UploadMediaCommand* uploadCommand READ uploadCommand WRITE setUploadCommand NOTIFY uploadCommandChanged)

public:
	explicit ReBuzzCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		//
		if (!buzzfeedModel_) return;

		//
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzId_.toStdString());
		if (buzzBody_.length()) lArgs.push_back(buzzBody_.toStdString());

		for (QStringList::iterator lBuzzer = buzzers_.begin(); lBuzzer != buzzers_.end(); lBuzzer++) {
			lArgs.push_back((*lBuzzer).toStdString());
		}

		for (QStringList::iterator lMedia = buzzMedia_.begin(); lMedia != buzzMedia_.end(); lMedia++) {
			lArgs.push_back((*lMedia).toStdString());
		}

		command_->process(lArgs);
	}

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
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
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

private:
	QString buzzId_;
	QString buzzBody_;
	QStringList buzzers_;
	QStringList buzzMedia_;
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	UploadMediaCommand* uploadCommand_ = nullptr;
	qbit::CreateReBuzzCommandPtr command_;
};

class ReplyCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzId READ buzzId WRITE setBuzzId NOTIFY buzzIdChanged)
	Q_PROPERTY(QString buzzBody READ buzzBody WRITE setBuzzBody NOTIFY buzzBodyChanged)
	Q_PROPERTY(BuzzfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)
	Q_PROPERTY(UploadMediaCommand* uploadCommand READ uploadCommand WRITE setUploadCommand NOTIFY uploadCommandChanged)

public:
	explicit ReplyCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process() {
		//
		if (!buzzfeedModel_) return;

		//
		prepare();

		//
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzId_.toStdString());
		if (buzzBody_.length()) lArgs.push_back(buzzBody_.toStdString());

		for (QStringList::iterator lBuzzer = buzzers_.begin(); lBuzzer != buzzers_.end(); lBuzzer++) {
			lArgs.push_back((*lBuzzer).toStdString());
		}

		for (QStringList::iterator lMedia = buzzMedia_.begin(); lMedia != buzzMedia_.end(); lMedia++) {
			lArgs.push_back((*lMedia).toStdString());
		}

		command_->process(lArgs);
	}

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
		else emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
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

private:
	QString buzzId_;
	QString buzzBody_;
	QStringList buzzers_;
	QStringList buzzMedia_;
	BuzzfeedListModel* buzzfeedModel_ = nullptr;
	UploadMediaCommand* uploadCommand_ = nullptr;
	qbit::CreateBuzzReplyCommandPtr command_;
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
	Q_PROPERTY(QString buzzerChainId READ buzzerChainId NOTIFY buzzerChainIdChanged)
	Q_PROPERTY(QString alias READ alias NOTIFY aliasChanged)
	Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
	Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY avatarUrlChanged)
	Q_PROPERTY(QString avatarId READ avatarId NOTIFY avatarIdChanged)
	Q_PROPERTY(QString headerUrl READ headerUrl NOTIFY headerUrlChanged)
	Q_PROPERTY(QString headerId READ headerId NOTIFY headerIdChanged)

public:
	explicit LoadBuzzerInfoCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString buzzer) {
		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer.toStdString());
		command_->process(lArgs);
	}

	QString buzzerId() {
		if (buzzer_) {
			return QString::fromStdString(buzzer_->id().toHex());
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

	void done(qbit::EntityPtr buzzer, qbit::TransactionPtr info, const std::string& name, const qbit::ProcessingError& result) {
		if (result.success()) {
			buzzer_ = buzzer;
			info_ = qbit::TransactionHelper::to<qbit::TxBuzzerInfo>(info);
			emit processed(QString::fromStdString(name));
		} else {
			emit error(QString::fromStdString(result.error()), QString::fromStdString(result.message()));
		}
	}

signals:
	void processed(QString name);
	void error(QString code, QString message);
	void aliasChanged();
	void descriptionChanged();
	void buzzerIdChanged();
	void avatarUrlChanged();
	void headerUrlChanged();
	void avatarIdChanged();
	void headerIdChanged();
	void buzzerChainIdChanged();

private:
	qbit::LoadBuzzerInfoCommandPtr command_;
	qbit::EntityPtr buzzer_;
	qbit::TxBuzzerInfoPtr info_;
};

class LoadEventsfeedCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(EventsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadEventsfeedCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(bool more) {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;

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
	void dataReady(const qbit::EventsfeedProxy& /*local*/, bool /*more*/);

private:
	EventsfeedListModel* eventsfeedModel_ = nullptr;
	qbit::LoadEventsfeedCommandPtr command_ = nullptr;
	bool more_ = false;
};

class LoadEndorsementsByBuzzerCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzer READ buzzer WRITE setBuzzer NOTIFY buzzerChanged)
	Q_PROPERTY(EventsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadEndorsementsByBuzzerCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(bool more) {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
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

	void setBuzzer(const QString& buzzer) { buzzer_ = buzzer; emit buzzerChanged(); }
	QString buzzer() const { return buzzer_; }

	EventsfeedListModel* model() { return eventsfeedModel_; }
	void setModel(EventsfeedListModel* model) { eventsfeedModel_ = model; }

	void ready(qbit::EventsfeedPtr base, qbit::EventsfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::EventsfeedProxy& /*local*/, bool /*more*/);
	void buzzerChanged();

private:
	EventsfeedListModel* eventsfeedModel_ = nullptr;
	qbit::LoadEndorsementsByBuzzerCommandPtr command_ = nullptr;
	QString buzzer_;
	bool more_ = false;
};

class LoadMistrustsByBuzzerCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzer READ buzzer WRITE setBuzzer NOTIFY buzzerChanged)
	Q_PROPERTY(EventsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadMistrustsByBuzzerCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(bool more) {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
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

	void setBuzzer(const QString& buzzer) { buzzer_ = buzzer; emit buzzerChanged(); }
	QString buzzer() const { return buzzer_; }

	EventsfeedListModel* model() { return eventsfeedModel_; }
	void setModel(EventsfeedListModel* model) { eventsfeedModel_ = model; }

	void ready(qbit::EventsfeedPtr base, qbit::EventsfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::EventsfeedProxy& /*local*/, bool /*more*/);
	void buzzerChanged();

private:
	EventsfeedListModel* eventsfeedModel_ = nullptr;
	qbit::LoadMistrustsByBuzzerCommandPtr command_ = nullptr;
	QString buzzer_;
	bool more_ = false;
};

class LoadFollowingByBuzzerCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzer READ buzzer WRITE setBuzzer NOTIFY buzzerChanged)
	Q_PROPERTY(EventsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadFollowingByBuzzerCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(bool more) {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
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

	void setBuzzer(const QString& buzzer) { buzzer_ = buzzer; emit buzzerChanged(); }
	QString buzzer() const { return buzzer_; }

	EventsfeedListModel* model() { return eventsfeedModel_; }
	void setModel(EventsfeedListModel* model) { eventsfeedModel_ = model; }

	void ready(qbit::EventsfeedPtr base, qbit::EventsfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::EventsfeedProxy& /*local*/, bool /*more*/);
	void buzzerChanged();

private:
	EventsfeedListModel* eventsfeedModel_ = nullptr;
	qbit::LoadSubscriptionsByBuzzerCommandPtr command_ = nullptr;
	QString buzzer_;
	bool more_ = false;
};

class LoadFollowersByBuzzerCommand: public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString buzzer READ buzzer WRITE setBuzzer NOTIFY buzzerChanged)
	Q_PROPERTY(EventsfeedListModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
	explicit LoadFollowersByBuzzerCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(bool more) {
		//
		if (!eventsfeedModel_) return;

		// prepare command
		prepare();

		// params
		more_ = more;

		// NOTICE: we actually able to process only "our" updates not "all", that is why there is no
		// interception of destination have sence for global feed

		//
		if (!more_) eventsfeedModel_->eventsfeed()->clear();

		std::vector<std::string> lArgs;
		lArgs.push_back(buzzer_.toStdString());
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

	void setBuzzer(const QString& buzzer) { buzzer_ = buzzer; emit buzzerChanged(); }
	QString buzzer() const { return buzzer_; }

	EventsfeedListModel* model() { return eventsfeedModel_; }
	void setModel(EventsfeedListModel* model) { eventsfeedModel_ = model; }

	void ready(qbit::EventsfeedPtr base, qbit::EventsfeedPtr local);

signals:
	void processed();
	void error(QString code, QString message);
	void modelChanged();
	void dataReady(const qbit::EventsfeedProxy& /*local*/, bool /*more*/);
	void buzzerChanged();

private:
	EventsfeedListModel* eventsfeedModel_ = nullptr;
	qbit::LoadFollowersByBuzzerCommandPtr command_ = nullptr;
	QString buzzer_;
	bool more_ = false;
};

class LoadTransactionCommand: public QObject
{
	Q_OBJECT

public:
	explicit LoadTransactionCommand(QObject* parent = nullptr);

	Q_INVOKABLE void process(QString tx, QString chain) {
		QString lArg = tx + "/" + chain;
		std::vector<std::string> lArgs;
		lArgs.push_back(lArg.toStdString());

		command_->process(lArgs);
	}

	void done(qbit::TransactionPtr tx, const qbit::ProcessingError& result) {
		if (result.success()) {
			if (tx) {
				// TODO: check integrity (tx->id() == params.tx)
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
};

} // buzzer

#endif // BUZZER_COMMAND_WRAPPERS_H
