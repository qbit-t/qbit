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

QString ApplicationPath::tempFilesDir()
{
#ifdef Q_OS_ANDROID
	return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#endif

#ifdef Q_OS_IOS
	return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#endif

	QString lGlobalDataPath = qApp->applicationDirPath() + "/data";
	QDir lDataDir(lGlobalDataPath);
	if (!lDataDir.exists()) {
		lDataDir.setPath(qApp->applicationDirPath());
		lDataDir.mkdir("data");
	}

	QString lCacheDataPath = lGlobalDataPath + "/cache";
	QDir lCacheDir(lCacheDataPath);
	if (!lCacheDir.exists()) {
		lCacheDir.setPath(lGlobalDataPath);
		lCacheDir.mkdir("cache");
	}

#ifdef Q_OS_WINDOWS
	lCacheDataPath.replace(0, 1, lCacheDataPath[0].toLower());

	return lCacheDataPath;
#endif

	return lCacheDataPath;
}
