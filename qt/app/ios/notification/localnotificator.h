#ifndef LOCALNOTIFICATOR_H
#define LOCALNOTIFICATOR_H

#include <QObject>
#include "applicationpath.h"

namespace buzzer {

class LocalNotificator : public QObject
{
    Q_OBJECT
public:
    explicit LocalNotificator(QObject *parent = nullptr) : QObject(parent)
    {
        QString lAppConfig = buzzer::ApplicationPath::applicationDirPath() + "/buzzer-app.config";
        if (QFile(lAppConfig).exists()) appConfig_.loadFromFile(lAppConfig.toStdString());
    }

    ~LocalNotificator()
    {
    }

#ifdef Q_OS_IOS
    Q_INVOKABLE void notify(QString id, QString title, QString message);
    Q_INVOKABLE void reset();
#endif

#ifdef Q_OS_ANDROID
    Q_INVOKABLE void notify(QString id, QString title, QString message) {}
    Q_INVOKABLE void reset() {}
#endif

    static LocalNotificator* instance()
    {
        static LocalNotificator* lLocalNotificator = new LocalNotificator();
        return lLocalNotificator;
    }

    void setDeviceToken(QString token) { deviceToken_ = token; emit deviceTokenChanged(); }
    Q_INVOKABLE QString getDeviceToken() { return deviceToken_; }

    QString getLocalization(QString locale, QString key)
    {
        qbit::json::Value lLocalization = appConfig_["localization"];
        qbit::json::Value lLocale = lLocalization[locale.toStdString()];
        qbit::json::Value lValue = lLocale[key.toStdString()];
        return QString::fromStdString(lValue.getString());
    }

signals:
    void deviceTokenChanged();

private:
    QString deviceToken_;
    qbit::json::Document appConfig_;
};

} // qbit

#endif // LOCALNOTIFICATOR_H
