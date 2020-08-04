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

	/*
	qreal factor = 1.0;
	QScreen* screen = lApp.screens()[0];
	qreal pixelDensity = screen->physicalDotsPerInch() / 25.4;
	qreal wFactor = qreal(screen->geometry().width()) / qRound(screen->geometry().width() / pixelDensity);
	qreal hFactor = qreal(screen->geometry().height()) / qRound(screen->geometry().height() / pixelDensity);
	qreal averageDensity = (wFactor + hFactor) / 2;
	if (!qFuzzyCompare(pixelDensity, averageDensity))
		pixelDensity = averageDensity;
	factor *= pixelDensity;

	qInfo() << "--------------->" << factor << pixelDensity << averageDensity << wFactor << hFactor;

	//qputenv("QT_SCREEN_SCALE_FACTORS", "2.623"); // The original value is 2.625 ,8
	//qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");

	*/

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
