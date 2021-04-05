#include <QGuiApplication>
#include <QtWidgets/QApplication>
#include <QQmlApplicationEngine>

#include <QDir>
#include <QPluginLoader>
#include <QQmlExtensionPlugin>
#include <QDebug>

#include <QFile>
#include <QScopedPointer>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>

#include "logger.h"
#include "application.h"
#include "applicationpath.h"

#include <QScreen>

#if defined(Q_OS_LINUX)
#include <pwd.h>
#endif

//
// main
//
int main(int argc, char *argv[]) {
	//
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

	// generic application
	QApplication lApp(argc, argv);

#if defined(DESKTOP_PLATFORM)
	// icon
	lApp.setWindowIcon(QIcon("qrc:/images/icon-high.png"));

	// check and deploy
#if defined(Q_OS_LINUX)
	// current user home directory
	uid_t lUid = geteuid();
	std::string lUsername;
	std::string lHome;
	struct passwd *lPw = getpwuid(lUid);
	if (lPw) {
		lUsername = std::string(lPw->pw_name);
	} else {
		char lName[0x100] = {0};
		getlogin_r(lName, sizeof(lName));
		lUsername = std::string(lName);
	}

	lHome = "/home/" + lUsername + "/";

	// extract and save buzzer.png
	if (!QFileInfo::exists(QString::fromStdString(lHome) + ".local/share/icons/buzzer.png")) {
		//
		QFile lRawFile(":/images/icon-high.png");
		lRawFile.open(QIODevice::ReadOnly);

		QByteArray lRawData = lRawFile.readAll();

		QFile lIconFile(QString::fromStdString(lHome) + ".local/share/icons/buzzer.png");
		lIconFile.open(QIODevice::WriteOnly);
		lIconFile.write(lRawData);
		lIconFile.close();
	}

	// extract, correct and save buzzer.desktop
	if (!QFileInfo::exists(QString::fromStdString(lHome) + ".local/share/applications/buzzer.desktop")) {
		//
		QFile lRawFile(":/buzzer.desktop");
		lRawFile.open(QIODevice::ReadOnly | QIODevice::Text);

		QByteArray lRawData = lRawFile.readAll();
		QString lData = QString::fromStdString(lRawData.toStdString());

		lData.replace("{version}", QString(VERSION_STRING));
		lData.replace("{path}", buzzer::ApplicationPath::applicationDirPath());

		QFile lDesktopFile(QString::fromStdString(lHome) + ".local/share/applications/buzzer.desktop");
		lDesktopFile.open(QIODevice::WriteOnly);
		lDesktopFile.write(lData.toUtf8());
		lDesktopFile.close();

		// restart
		QProcess* lProcess = new QProcess();
		lProcess->startDetached(buzzer::ApplicationPath::applicationDirPath() + "/buzzer", QStringList());
		lProcess->waitForStarted();
		return 0;
	}

#endif

#endif
	// buzzer global application
	qInfo() << "====================== STARTING =========================";
	try {
		buzzer::gApplication = new buzzer::Application(lApp);
		if (buzzer::gApplication->load() > 0) {
			return buzzer::gApplication->execute();
		} else {
			qWarning() << "Unexpected exit.";
		}
	} catch(qbit::exception& ex) {
		qWarning() << QString::fromStdString(ex.code()) << QString::fromStdString(ex.what());
	}

    return -1;
}
