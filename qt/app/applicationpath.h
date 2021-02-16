#ifndef MODULE_H
#define MODULE_H

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QAbstractListModel>
#include <QList>
#include <QByteArray>
#include <QModelIndex>
#include <QHash>
#include <QVariant>
#include <QStandardPaths>

#include <set>

#include "json.h"
#include "iapplication.h"

namespace buzzer {

/**
 * @brief The ApplicationPath class
 * AppDir for the various platforms
 */
class ApplicationPath
{
public:
    static QString applicationDirPath();
    static QString applicationUrlPath()
    {
    #ifdef Q_OS_ANDROID
        return QString("assets:");
    #endif

        return qApp->applicationDirPath();
    }
    static  QString assetUrlPath()
    {
    #ifdef Q_OS_ANDROID
        return QString("file:");
    #endif

    #ifdef Q_OS_LINUX
        return QString("file:");
    #else
        return QString("file:///");
    #endif
    }
    static QString dataDirPath()
    {
    #ifdef Q_OS_ANDROID
        return QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    #endif

    #ifdef Q_OS_IOS
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    #endif

        return qApp->applicationDirPath() + "/data";
    }
	static QString tempFilesDir()
	{
	#ifdef Q_OS_ANDROID
		return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
	#endif

	#ifdef Q_OS_IOS
		return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
	#endif

		return qApp->applicationDirPath() + "/data";
	}

	static QString logsDirPath();
};

} // buzzer

#endif // MODULE_H
