
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include "settings.h"
#include "iapplication.h"
#include "applicationpath.h"

#include "crypto/aes.h"
#include "crypto/sha256.h"

#include <stdio.h>

using namespace buzzer;

uint256 Settings::proofAsset() {
	if (!client_) return uint256();
	uint256 lAsset; lAsset.setHex(client_->getQttAsset());
	return lAsset;
}

uint64_t Settings::proofAssetLockTime() {
	if (!client_) return 0;
	return client_->getQttAssetLockTime();
}

uint64_t Settings::oneVoteProofAmount() {
	if (!client_) return 0;
	return client_->getQttAssetVoteAmount();
}

size_t Settings::threadPoolSize() {
	if (daemon_) return 1;
	return 1;
}

Settings* SettingsFactory::get()
{
	return new SettingsJSON();
}

Settings* SettingsFactory::getDaemon()
{
	return new SettingsJSON(true);
}

std::string SettingsJSON::getApplicationDataPath()
{
	QString lGlobalDataPath = ApplicationPath::dataDirPath();
	QString lModuleDataPath = lGlobalDataPath + "/buzzer";

	QDir lCurrentDataDir(lModuleDataPath);
	if (!lCurrentDataDir.exists())
	{
		lCurrentDataDir.setPath(lGlobalDataPath);
		lCurrentDataDir.mkdir("buzzer");
	}

	qInfo() << "Data path: " << lModuleDataPath;
	return lModuleDataPath.toStdString();
}

std::string SettingsJSON::getApplicationLogsPath() {
	qInfo() << "Logs path: " << buzzer::ApplicationPath::logsDirPath();
	return buzzer::ApplicationPath::logsDirPath().toStdString();
}

QString SettingsJSON::getSettingsFile()
{
    QString lGlobalDataPath = ApplicationPath::dataDirPath();
	QString lModuleDataPath = lGlobalDataPath + "/buzzer";

    QDir lCurrentDataDir(lModuleDataPath);
    if (!lCurrentDataDir.exists())
    {
        lCurrentDataDir.setPath(lGlobalDataPath);
		lCurrentDataDir.mkdir("buzzer");
    }

    QString lDbFile = lModuleDataPath + "/settings.json";
    return lDbFile;
}

void SettingsJSON::link(IClient* client)
{
	client_ = client;
}

void SettingsJSON::open()
{
	if (opened_) return;

    //
    // open settings
    //
	qbit::json::Document lSettingsDb;
    QString lSettingsDbFile = getSettingsFile();
    lSettingsDb.loadFromFile(lSettingsDbFile.toStdString());
	qbit::json::Value lSettingsRoot;
	if (lSettingsDb.find("name", lSettingsRoot)) // sanity check
    {
		client_->settingsFromJSON(lSettingsDb);
    }

	opened_ = true;
}

void SettingsJSON::update()
{
	qbit::json::Document lDb;
	lDb.loadFromString("{}");
	client_->settingsToJSON(lDb);
	QString lDbFile = getSettingsFile();
	lDb.saveToFile(lDbFile.toStdString());
}
