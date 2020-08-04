#include "logger.h"
#include "log/log.h"

using namespace buzzer;

QScopedPointer<Logger> buzzer::gLogger;

void buzzer::LoggerMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    if (!gLogger.isNull()) gLogger->Log(type, context.category, msg);
}

Logger::Logger() {
#ifdef QT_DEBUG
    qInfo() << QString("[Logger/Path]: Logging to debug console.");
#endif
}

void Logger::Log(QtMsgType type, const QString&, const QString& msg) {
	switch (type) {
		case QtInfoMsg:     qbit::gLog().write(qbit::Log::INFO, msg.toStdString()); break;
		case QtDebugMsg:    qbit::gLog().write(qbit::Log::DEBUG, msg.toStdString()); break;
		case QtWarningMsg:  qbit::gLog().write(qbit::Log::WARNING, msg.toStdString()); break;
		case QtCriticalMsg: qbit::gLog().write(qbit::Log::ERROR, msg.toStdString()); break;
		case QtFatalMsg:    qbit::gLog().write(qbit::Log::ERROR, msg.toStdString()); break;
    }
}

