#include "logger.h"
#include "log/log.h"

using namespace buzzer;

QScopedPointer<Logger> buzzer::gLogger;

void buzzer::LoggerMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    if (!gLogger.isNull()) gLogger->Log(type, context.category, msg);
}

Logger::Logger(bool enableQtLog) {
#ifdef QT_DEBUG
    qInfo() << QString("[Logger/Path]: Logging to debug console.");
#else
    if (enableQtLog) qInstallMessageHandler(buzzer::LoggerMessageHandler);
#endif
}

void Logger::Log(QtMsgType type, const QString&, const QString& msg) {
	switch (type) {
		case QtInfoMsg:     qbit::gLog().write(qbit::Log::INFO, std::string(": ") + msg.toStdString()); break;
		case QtDebugMsg:    qbit::gLog().write(qbit::Log::DEBUG, std::string(": ") + msg.toStdString()); break;
		case QtWarningMsg:  qbit::gLog().write(qbit::Log::WARNING, std::string(": ") + msg.toStdString()); break;
		case QtCriticalMsg: qbit::gLog().write(qbit::Log::GENERAL_ERROR, std::string(": ") + msg.toStdString()); break;
		case QtFatalMsg:    qbit::gLog().write(qbit::Log::GENERAL_ERROR, std::string(": ") + msg.toStdString()); break;
    }
}

