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

//
// main
//
int main(int argc, char *argv[]) {
	//
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	// generic application
	QApplication lApp(argc, argv);

#if defined (DESKTOP_PLATFORM)
	lApp.setWindowIcon(QIcon("qrc:/images/icon-high.png"));
#endif

	// buzzer global application
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
