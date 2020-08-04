#include "asiodispatcher.h"

using namespace buzzer;

AsioDispatcher::AsioDispatcher(boost::asio::io_service &io_service, QObject *parent) : QAbstractEventDispatcher(parent), io_service_(io_service) {
	//
}

AsioDispatcher::~AsioDispatcher() {

}

bool AsioDispatcher::processEvents(QEventLoop::ProcessEventsFlags /*flags*/) {
	//
	ulong lEvents = 0;
	do {
		io_service_.reset();
		//run all ready handlers
		lEvents = io_service_.poll();

	} while(lEvents > 0);

	return true;
}

void AsioDispatcher::registerSocketNotifier(QSocketNotifier*) {}
void AsioDispatcher::unregisterSocketNotifier(QSocketNotifier*) {}

void AsioDispatcher::registerTimer(int, int, Qt::TimerType, QObject*) {}
bool AsioDispatcher::unregisterTimer(int) { return true; }
bool AsioDispatcher::unregisterTimers(QObject*) { return true; }
QList<QAbstractEventDispatcher::TimerInfo> AsioDispatcher::registeredTimers(QObject*) const {
	return QList<QAbstractEventDispatcher::TimerInfo>();
}

int AsioDispatcher::remainingTime(int) { return 0; }

void AsioDispatcher::wakeUp() {}
void AsioDispatcher::interrupt() {}
