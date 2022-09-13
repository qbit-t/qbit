#ifndef IAPPLICATION_H
#define IAPPLICATION_H

#include <QQmlApplicationEngine>
#include <QNetworkAccessManager>
#include <QQuickWindow>

#include "iclient.h"

namespace buzzer {

class IApplication {
public:
    virtual QQmlApplicationEngine* getEngine() = 0;
	virtual IClient* getClient() = 0;
	virtual QQuickWindow* view() = 0;
    virtual QNetworkAccessManager* getNetworkManager() = 0;
	virtual QString getLocalization(QString locale, QString key) = 0;
	virtual QString getColor(QString localtheme, QString selector, QString key) = 0;
	virtual std::string getLogsLocation() = 0;
	virtual int load() = 0;
	virtual int execute() = 0;
	virtual bool isDesktop() = 0;

	virtual std::string getLogCategories() = 0;
	virtual std::string getPeers() = 0;
	virtual std::string getQttAsset() = 0;
	virtual uint64_t getQttAssetVoteAmount() = 0;
	virtual int getQttAssetLockTime() = 0;
	virtual qreal defaultFontSize() = 0;
	virtual bool getTestNet() = 0;
	virtual bool getDebug() = 0;
	virtual bool getInterceptOutput() = 0;
	virtual bool checkPermission() { return false; }
	virtual bool isWakeLocked() = 0;
};

extern buzzer::IApplication* gApplication;

} // buzzer

#endif // IAPPLICATION_H
