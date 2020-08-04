#ifndef ASIODISPATCHER_H
#define ASIODISPATCHER_H

#include <QAbstractEventDispatcher>

#include <boost/asio.hpp>

namespace buzzer {

class AsioDispatcher: public QAbstractEventDispatcher {
	Q_OBJECT
public:
	explicit AsioDispatcher(boost::asio::io_service &io_service, QObject *parent = nullptr);
	~AsioDispatcher();

	bool processEvents(QEventLoop::ProcessEventsFlags flags);
	void registerSocketNotifier(QSocketNotifier *notifier) Q_DECL_FINAL;
	void unregisterSocketNotifier(QSocketNotifier *notifier) Q_DECL_FINAL;

	void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object) Q_DECL_FINAL;
	bool unregisterTimer(int timerId) Q_DECL_FINAL;
	bool unregisterTimers(QObject *object) Q_DECL_FINAL;
	QList<QAbstractEventDispatcher::TimerInfo> registeredTimers(QObject *object) const Q_DECL_FINAL;
	int remainingTime(int timerId) Q_DECL_FINAL;

	void wakeUp() Q_DECL_FINAL;
	void interrupt() Q_DECL_FINAL;

private:
	boost::asio::io_service &io_service_;
};

} // buzzer

#endif // ASIODISPATCHER_H
