#ifndef LOGGER_H
#define LOGGER_H

#include <QFile>
#include <QScopedPointer>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>

namespace buzzer {

extern void LoggerMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

class Logger {
public:
	Logger();
    void Log(QtMsgType type, const QString& category, const QString& msg);
};

extern QScopedPointer<Logger> gLogger;

} // buzzer

#endif // LOGGER_H
