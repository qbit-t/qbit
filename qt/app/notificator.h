#ifndef NOTIFICATOR_H
#define NOTIFICATOR_H

#include <QFrame>
#include "notificator_p.h"

#include <QLabel>
#include <QIcon>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QGridLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QStyle>
#include <QPushButton>

#include <QQuickView>

namespace buzzer {

class ElidedLabel : public QLabel
{
	Q_OBJECT
public:
	ElidedLabel(QWidget* parent=0, Qt::WindowFlags f=0);
	ElidedLabel(const QString& txt, QWidget* parent=0, Qt::WindowFlags f=0);
	ElidedLabel(const QString& txt,
		Qt::TextElideMode elideMode=Qt::ElideRight,
		QWidget* parent=0,
		Qt::WindowFlags f=0
	);
	// Set the elide mode used for displaying text.
	void setElideMode(Qt::TextElideMode elideMode) {
		elideMode_ = elideMode;
		updateGeometry();
	}
	// Get the elide mode currently used to display text.
	Qt::TextElideMode elideMode() const { return elideMode_; }
	// QLabel overrides
	void setText(const QString &);

protected: // QLabel overrides
	void paintEvent(QPaintEvent*);
	void resizeEvent(QResizeEvent*);
	// Cache the elided text so as to not recompute it every paint event
	void cacheElidedText(int w);

private:
	Qt::TextElideMode elideMode_;
	QString cachedElidedText;
};

class Notificator : public QFrame {
	Q_OBJECT

public:
	static void showMessage(buzzer::PushNotificationPtr);
	void fadeIn();
	void fadeOut();
	static void hideAll();

protected:
	bool event(QEvent*);

private:
	Notificator(buzzer::PushNotificationPtr, bool autohide = true);
	~Notificator();

	void notify(buzzer::PushNotificationPtr, bool);

private:
	void initializeLayout();
	void initializeUI();
	void correctPosition();
	void adjustGeometry();

private:
	NotificatorPrivate* d;
	buzzer::PushNotificationPtr item_;

	static bool configureInstance(Notificator* notificator);
	static QList<Notificator*> pending_;
	static QList<Notificator*> current_;
	static QPushButton* closeAllButton_;
};

} // buzzer

#endif //NOTIFICATOR_H
