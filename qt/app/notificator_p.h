#ifndef NOTIFICATOR_P_H
#define NOTIFICATOR_P_H

#include <QtGui/QIcon>

#include "pushdesktopnotification.h"

class QLabel;
class QIcon;
class QProgressBar;
class QPropertyAnimation;

namespace buzzer {

class ElidedLabel;

class PushNotification;
typedef std::shared_ptr<PushNotification> PushNotificationPtr;

class NotificatorPrivate
{
public:
	NotificatorPrivate(bool autoHide = true);
	~NotificatorPrivate();

	void initialize(buzzer::PushNotificationPtr);

public:
	bool autoHide() const;

	QLabel* type();
	QLabel* avatar();
	QLabel* alias();
	QLabel* buzzer();
	QLabel* caption();
	QLabel* text();
	QLabel* media();
	QLabel* close();
	QLabel* closeAll();

private:
	bool autoHide_;
	QLabel* type_;
	QLabel* avatar_;
	QLabel* alias_;
	QLabel* buzzer_;
	QLabel* caption_;
	QLabel* text_;
	QLabel* media_;
	QLabel* close_;
	QLabel* closeAll_;
};

} // buzzer

#endif // NOTIFICATOR_P_H
