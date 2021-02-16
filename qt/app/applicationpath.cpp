#include <QDir>
#include <QPluginLoader>
#include <QQmlExtensionPlugin>
#include <QDebug>
#include <QException>
#include <QStandardPaths>
#include <QStringList>
#include <QJsonDocument>

#include "applicationpath.h"
#include "error.h"

using namespace buzzer;

#ifdef Q_OS_ANDROID
std::wstring gProfile(L"android");
#else
std::wstring gProfile(L"desktop");
#endif

QString ApplicationPath::applicationDirPath()
{
#ifdef Q_OS_ANDROID
    return QString("assets:");
#endif

    return qApp->applicationDirPath();
}

QString ApplicationPath::logsDirPath()
{
#ifdef Q_OS_ANDROID
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
#endif

#ifdef Q_OS_IOS
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif

    return qApp->applicationDirPath() + "/logs";
}

