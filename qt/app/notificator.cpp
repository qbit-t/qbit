#include "notificator.h"
#include "notificator_p.h"
#include "client.h"

#include <QtGui>
#include <QGraphicsOpacityEffect>

namespace {
	const float WINDOW_TRANSPARENT_OPACITY = 0.9;
	const float WINDOW_NONTRANSPARENT_OPACITY = 1.0;
	const int NOTIFICATION_MARGIN = 10;
	const int DEFAULT_MESSAGE_SHOW_TIME = 10000;
}

void buzzer::Notificator::showMessage(buzzer::PushNotificationPtr item, bool autohide) {
	//
	if (!closeAllButton_) {
		buzzer::Client* lClient = static_cast<buzzer::Client*>(buzzer::gApplication->getClient());

		closeAllButton_ = new QPushButton(buzzer::gApplication->getLocalization(lClient->locale(), "Buzzer.notifications.hide"));
		closeAllButton_->setObjectName("closeAllButton");
		closeAllButton_->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
		closeAllButton_->setFlat(true);

		closeAllButton_->setStyleSheet(
			QString::fromStdString(strprintf(
				"QPushButton#closeAllButton { color: %s; background-color: %s; }",
				buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Material.foreground").toStdString(),
				buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Page.background").toStdString()
			))
		);

		connect(closeAllButton_, &QPushButton::clicked, hideAll);
	}

	// check
	if (index_.find(item->getKey()) == index_.end()) {
		//
		index_.insert(item->getKey());
		//
		Notificator* lInstance = new Notificator(item, autohide);
		bool lDisplay = configureInstance(lInstance);
		lInstance->notify(item, lDisplay);
		if (autohide && QThread::currentThread() != nullptr)
			QTimer::singleShot(DEFAULT_MESSAGE_SHOW_TIME, lInstance, SLOT(fadeOut()));
	}
}

buzzer::Notificator::Notificator(buzzer::PushNotificationPtr item, bool autohide) :
	QFrame(0),
	d(new NotificatorPrivate(autohide)),
	item_(item) {
	//
	hide();
	initializeLayout();
	initializeUI();
}

buzzer::Notificator::~Notificator() {
	delete d;
}

void buzzer::Notificator::adjustGeometry() {
	// display & correct
	QRect lCurrentGeometry = QApplication::desktop()->availableGeometry();
	Notificator* lPrevInstance = nullptr;
	for (QList<Notificator*>::reverse_iterator lIter = current_.rbegin(); lIter != current_.rend(); lIter++) {
		//
		if (!lPrevInstance) {
			QRect lGeometry = (*lIter)->geometry();
			lGeometry.setTop(lCurrentGeometry.bottom() - (*lIter)->size().height());
			(*lIter)->setGeometry(lGeometry);
			lPrevInstance = *lIter;

			continue;
		}

		QRect lGeometry = (*lIter)->geometry();
		lGeometry.setTop(lPrevInstance->geometry().top() - (NOTIFICATION_MARGIN + (*lIter)->size().height()));
		(*lIter)->setGeometry(lGeometry);
		lPrevInstance = *lIter;
	}

	if (lPrevInstance && pending_.size()) {
		closeAllButton_->setGeometry(lPrevInstance->geometry().x(), lPrevInstance->geometry().y() -
									 (30 + NOTIFICATION_MARGIN), 430, 30);
		closeAllButton_->show();
	} else {
		closeAllButton_->hide();
	}
}

void buzzer::Notificator::fadeIn() {
	//
	QPropertyAnimation* lAnimation = new QPropertyAnimation(this, "windowOpacity");
	lAnimation->setDuration(250);
	lAnimation->setStartValue(0);
	lAnimation->setEndValue(1);
	lAnimation->setEasingCurve(QEasingCurve::InBack);
	lAnimation->start(QPropertyAnimation::DeleteWhenStopped);
}

void buzzer::Notificator::fadeOut() {
	//
	QPropertyAnimation* lAnimation = new QPropertyAnimation(this, "windowOpacity");
	lAnimation->setDuration(250);
	lAnimation->setStartValue(1);
	lAnimation->setEndValue(0);
	lAnimation->setEasingCurve(QEasingCurve::OutBack);
	lAnimation->start(QPropertyAnimation::DeleteWhenStopped);
	connect(lAnimation, SIGNAL(finished()), this, SLOT(done()));
	index_.erase(item_->getKey());
}

void buzzer::Notificator::done() {
	//
	hide();
}

void buzzer::Notificator::hideAll() {
	//
	QMutableListIterator<Notificator*> lIterPending(pending_);
	while (lIterPending.hasNext()) {
		Notificator* lInstance = lIterPending.next();
		lIterPending.remove();
		lInstance = 0;
	}

	//
	QMutableListIterator<Notificator*> lIter(current_);
	while (lIter.hasNext()) {
		Notificator* lInstance = lIter.next();
		lInstance->fadeOut();
		lIter.remove();
		lInstance = 0;
	}
}

bool buzzer::Notificator::event(QEvent* event) {
	//
	buzzer::Client* lClient = static_cast<buzzer::Client*>(buzzer::gApplication->getClient());
	//
	if (event->type() == QEvent::HoverEnter) {
		setWindowOpacity(WINDOW_NONTRANSPARENT_OPACITY);
	} else if (event->type() == QEvent::HoverLeave) {
		setWindowOpacity(WINDOW_TRANSPARENT_OPACITY);
	} else if (event->type() == QEvent::MouseButtonPress /*&& d->autoHide()*/) {
		//
		QMouseEvent* lMouseEvent = static_cast<QMouseEvent*>(event);
		if (d->close()->x() <= lMouseEvent->pos().x() && d->close()->x() + d->close()->width() > lMouseEvent->pos().x() &&
			d->close()->y() <= lMouseEvent->pos().y() && d->close()->y() + d->close()->height() > lMouseEvent->pos().y()) {
			// fade out & close
			fadeOut();
		} else {
			// TODO: switch to message/thread/buzzer
			if (item_->getType() == lClient->tx_BUZZER_SUBSCRIBE_TYPE() ||
				item_->getType() == lClient->tx_BUZZER_ENDORSE_TYPE() ||
				item_->getType() == lClient->tx_BUZZER_MISTRUST_TYPE()) {
				//
				lClient->openBuzzfeedByBuzzer(item_->getBuzzer());
			} else if (item_->getType() == lClient->tx_BUZZER_CONVERSATION() ||
						item_->getType() == lClient->tx_BUZZER_ACCEPT_CONVERSATION() ||
						item_->getType() == lClient->tx_BUZZER_DECLINE_CONVERSATION() ||
						item_->getType() == lClient->tx_BUZZER_MESSAGE() ||
						item_->getType() == lClient->tx_BUZZER_MESSAGE_REPLY()) {
				//
				QString lConversationId = item_->getConversationId();
				QVariant lConversation = lClient->locateConversation(lConversationId);
				lClient->openConversation(item_->getId(), lConversationId, lConversation, lClient->getConversationsList());
			} else {
				//
				lClient->openThread(item_->getChain(), item_->getId(), item_->getAlias(), item_->getText());
			}

			// fade out & close
			fadeOut();
		}
	} else if (event->type() == QEvent::Hide) {
		//
		QMutableListIterator<Notificator*> lIter(current_);
		while (lIter.hasNext()) {
			Notificator* lInstance = lIter.next();
			if (lInstance->isHidden()) {
				lIter.remove();
				lInstance = 0;
			}
		}
		//
		if (pending_.size() && current_.size() < 3) {
			Notificator* lInstance = (*pending_.begin());
			pending_.pop_front();
			current_.append(lInstance);
			lInstance->show(); // force
			lInstance->fadeIn();
		}

		adjustGeometry();
	} else if (event->type() == QEvent::Resize) {
	} else if (event->type() == QEvent::Paint) {
	} else if (event->type() == QEvent::Show) {
	}

	return QFrame::event(event);
}

void buzzer::Notificator::notify(buzzer::PushNotificationPtr item, bool display) {
	//
	hide();
	d->initialize(item);
	correctPosition();
	if (display) show();
}

void buzzer::Notificator::initializeLayout() {
	//
	QGridLayout* lLayout = new QGridLayout(this);
	lLayout->setHorizontalSpacing(7);
	lLayout->setVerticalSpacing(7);

	QBoxLayout* lHeader = new QBoxLayout(QBoxLayout::LeftToRight);
	lHeader->setSpacing(5);
	lHeader->addWidget(d->alias(), Qt::AlignLeft);
	lHeader->addWidget(d->buzzer(), Qt::AlignLeft);
	lHeader->addStretch(110);
	lHeader->addWidget(d->close(), Qt::AlignRight);

	//
	lLayout->addWidget(d->type(), 0, 0, 1, 1, Qt::AlignTop);
	// lLayout->addWidget(d->closeAll(), 5, 0, 1, 1, Qt::AlignTop);
	lLayout->addWidget(d->avatar(), 0, 2, 5, 4, Qt::AlignTop);
	lLayout->addLayout(lHeader, 0, 8, 0, 22, Qt::AlignTop);

	lLayout->addWidget(d->caption(), 1, 8, 1, item_->getMedia().length() ? 18 : 22, Qt::AlignTop);
	if (item_->getText().length())
		lLayout->addWidget(d->text(), 2, 8, 2, item_->getMedia().length() ? 18 : 22, Qt::AlignTop);

	if (item_->getMedia().length())
		lLayout->addWidget(d->media(), 1, 26, 4, 4, Qt::AlignTop);
}

void buzzer::Notificator::initializeUI() {
	QPalette lPalette = this->palette();
	buzzer::Client* lClient = static_cast<buzzer::Client*>(buzzer::gApplication->getClient());
	lPalette.setColor(QPalette::Background, QColor(buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Page.background")));
	setAutoFillBackground(true);
	setPalette(lPalette);

	setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
	//setAttribute(Qt::WA_Hover, true);

	// type
	QString lTypeColor;
	if (item_->getType() == lClient->tx_BUZZ_REPLY_TYPE()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.reply");
	} else if (item_->getType() == lClient->tx_REBUZZ_TYPE()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.rebuzz");
	} else if (item_->getType() == lClient->tx_BUZZ_LIKE_TYPE()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.like");
	} else if (item_->getType() == lClient->tx_BUZZER_SUBSCRIBE_TYPE()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.subscribe");
	} else if (item_->getType() == lClient->tx_BUZZ_REWARD_TYPE()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.reward");
	} else if (item_->getType() == lClient->tx_BUZZER_ENDORSE_TYPE()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.endorse");
	} else if (item_->getType() == lClient->tx_BUZZER_MISTRUST_TYPE()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.mistrust");
	} else if (item_->getType() == lClient->tx_REBUZZ_REPLY_TYPE()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.rebuzz");
	} else if (item_->getType() == lClient->tx_BUZZER_CONVERSATION()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.conversation");
	} else if (item_->getType() == lClient->tx_BUZZER_ACCEPT_CONVERSATION()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.conversation.accept");
	} else if (item_->getType() == lClient->tx_BUZZER_DECLINE_CONVERSATION()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.conversation.decline");
	} else if (item_->getType() == lClient->tx_BUZZER_MESSAGE()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.message");
	} else if (item_->getType() == lClient->tx_BUZZER_MESSAGE_REPLY()) {
		lTypeColor = buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Buzzer.event.message");
	}

	setStyleSheet(
		QString::fromStdString(strprintf(
			"QLabel#closeAll { color: %s; } QLabel#close { color: %s; } QLabel#type { color: %s; } QLabel#alias { color: %s; } QLabel#buzzer { color: %s; } QLabel#caption { color: %s; } QLabel#text { color: %s; } QLabel#avatar { } QLabel#media { }",
			buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Material.foreground").toStdString(),
			buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Material.foreground").toStdString(),
			lTypeColor.toStdString(),
			buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Material.foreground").toStdString(),
			buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Material.disabled").toStdString(),
			buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Material.disabled").toStdString(),
			buzzer::gApplication->getColor(lClient->theme(), lClient->themeSelector(), "Material.foreground").toStdString()
		))
	);

	setAutoFillBackground(true);
	setWindowOpacity(WINDOW_TRANSPARENT_OPACITY);
}

void buzzer::Notificator::correctPosition() {
	//
	setMinimumWidth(430);
	setMaximumWidth(430);
	setMaximumHeight(120);
	setMinimumHeight(120);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	QRect notificationGeometry = QApplication::desktop()->availableGeometry();
	QSize notificationSize = sizeHint();
	notificationSize.setWidth(430);

	notificationGeometry.setTop(0);
	notificationGeometry.setLeft(notificationGeometry.right() - (notificationSize.width() + NOTIFICATION_MARGIN));

	notificationGeometry.setSize(notificationSize);
	setGeometry(notificationGeometry);

	adjustGeometry();
}

QList<buzzer::Notificator*> buzzer::Notificator::pending_;
QList<buzzer::Notificator*> buzzer::Notificator::current_;
QPushButton* buzzer::Notificator::closeAllButton_;
std::set<qbit::EventsfeedItem::Key> buzzer::Notificator::index_;

bool buzzer::Notificator::configureInstance(buzzer::Notificator* notificator) {
	//
	QMutableListIterator<Notificator*> lIter(current_);
	while (lIter.hasNext()) {
		Notificator* lInstance = lIter.next();
		if (lInstance->isHidden()) {
			lIter.remove();
			lInstance = 0;
		}
	}

	if (notificator != 0) {
		if (current_.size() < 3) {
			current_.append(notificator);
			notificator->fadeIn();
			return true;
		} else {
			pending_.append(notificator);
		}
	}

	return false;
}

buzzer::NotificatorPrivate::NotificatorPrivate(bool autohide) :
	autoHide_(autohide),
	type_(0),
	avatar_(0),
	alias_(0),
	buzzer_(0),
	caption_(0),
	text_(0),
	media_(0),
	close_(0),
	closeAll_(0){
}

buzzer::NotificatorPrivate::~NotificatorPrivate() {
	//
	if (close()->parent() == 0) {
		delete close();
	}

	if (closeAll()->parent() == 0) {
		delete closeAll();
	}

	if (type()->parent() == 0) {
		delete type();
	}

	if (avatar()->parent() == 0) {
		delete avatar();
	}

	if (alias()->parent() == 0) {
		delete alias();
	}

	if (buzzer()->parent() == 0) {
		delete buzzer();
	}

	if (caption()->parent() == 0) {
		delete caption();
	}

	if (text()->parent() == 0) {
		delete text();
	}

	if (media()->parent() == 0) {
		delete media();
	}
}

void buzzer::NotificatorPrivate::initialize(buzzer::PushNotificationPtr item) {
	//
	// avatar
	//
	QImage lAvatar(item->getAvatar());
	QPixmap lAvatarTarget = QPixmap(this->avatar()->size());
	lAvatarTarget.fill(Qt::transparent);
	QPixmap lAvatarTemp = QPixmap::fromImage(lAvatar).scaled(this->avatar()->width(), this->avatar()->width(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

	QPainter lAvatarPainter(&lAvatarTarget);
	lAvatarPainter.setRenderHint(QPainter::Antialiasing, true);
	lAvatarPainter.setRenderHint(QPainter::HighQualityAntialiasing, true);
	lAvatarPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);

	QPainterPath lAvatarPath = QPainterPath();
	lAvatarPath.addRoundedRect(0, 0, this->avatar()->width(), this->avatar()->height(), this->avatar()->height() / 2, this->avatar()->height() / 2);
	lAvatarPainter.setClipPath(lAvatarPath);
	lAvatarPainter.drawPixmap(0, 0, lAvatarTemp);

	this->avatar()->setPixmap(lAvatarTarget);

	//
	this->alias()->setText(item->getAlias());
	this->buzzer()->setText(item->getBuzzer());

	//
	this->caption()->setText(item->getComment());

	//
	if (item->getText().size())	{
		if (item->getText().size() > 50) this->text()->setText(QString(item->getText()).mid(0, 50).replace( "\n", " ") + "...");
		else this->text()->setText(QString(item->getText()).replace( "\n", " "));
	}

	// type
	buzzer::Client* lClient = static_cast<buzzer::Client*>(buzzer::gApplication->getClient());
	if (item->getType() == lClient->tx_BUZZ_REPLY_TYPE()) {
		this->type()->setText("\uf075");
	} else if (item->getType() == lClient->tx_REBUZZ_TYPE()) {
		this->type()->setText("\uf364");
	} else if (item->getType() == lClient->tx_BUZZ_LIKE_TYPE()) {
		this->type()->setText("\uf004");
	} else if (item->getType() == lClient->tx_BUZZER_SUBSCRIBE_TYPE()) {
		this->type()->setText("\uf504");
	} else if (item->getType() == lClient->tx_BUZZ_REWARD_TYPE()) {
		this->type()->setText("\uf51e");
	} else if (item->getType() == lClient->tx_BUZZER_ENDORSE_TYPE()) {
		this->type()->setText("\uf164");
	} else if (item->getType() == lClient->tx_BUZZER_MISTRUST_TYPE()) {
		this->type()->setText("\uf165");
	} else if (item->getType() == lClient->tx_REBUZZ_REPLY_TYPE()) {
		this->type()->setText("\uf364");
	} else if (item->getType() == lClient->tx_BUZZER_CONVERSATION()) {
		this->type()->setText("\uf234");
	} else if (item->getType() == lClient->tx_BUZZER_ACCEPT_CONVERSATION()) {
		this->type()->setText("\uf4fc");
	} else if (item->getType() == lClient->tx_BUZZER_DECLINE_CONVERSATION()) {
		this->type()->setText("\uf235");
	} else if (item->getType() == lClient->tx_BUZZER_MESSAGE()) {
		this->type()->setText("\uf4a6");
	} else if (item->getType() == lClient->tx_BUZZER_MESSAGE_REPLY()) {
		this->type()->setText("\uf4a6");
	}

	//
	// media
	//
	if (item->getMedia().size()) {
		//
		QImage lMedia(item->getMedia());
		QPixmap lMediaTarget = QPixmap(this->media()->size());
		lMediaTarget.fill(Qt::transparent);
		QPixmap lMediaTemp = QPixmap::fromImage(lMedia).scaled(this->media()->width(), this->media()->width(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

		QPainter lMediaPainter(&lMediaTarget);
		lMediaPainter.setRenderHint(QPainter::Antialiasing, true);
		lMediaPainter.setRenderHint(QPainter::HighQualityAntialiasing, true);
		lMediaPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);

		QPainterPath lMediaPath = QPainterPath();
		lMediaPath.addRoundedRect(0, 0, this->media()->width(), this->media()->height(), 3, 3);
		lMediaPainter.setClipPath(lMediaPath);
		lMediaPainter.drawPixmap(0, 0, lMediaTemp);

		this->media()->setPixmap(lMediaTarget);
	}
}

bool buzzer::NotificatorPrivate::autoHide() const {
	return autoHide_;
}

QLabel* buzzer::NotificatorPrivate::type() {
	if (type_ == 0) {
		type_ = new QLabel;
		type_->setObjectName( "type" );
		type_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		type_->setMaximumSize(20, 20);
		type_->setMinimumSize(20, 20);

		buzzer::Client* lClient = static_cast<buzzer::Client*>(buzzer::gApplication->getClient());
		QFont lFont("Font Awesome 5 Pro Light", lClient->scaleFactor() * 12, QFont::Normal);
		type_->setFont(lFont);
	}

	return type_;
}

QLabel* buzzer::NotificatorPrivate::avatar() {
	//
	if (avatar_ == 0) {
		avatar_ = new QLabel;
		avatar_->setObjectName( "avatar" );
		avatar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		avatar_->setMaximumSize(70, 70);
		avatar_->setMinimumSize(70, 70);
	}

	return avatar_;
}

QLabel* buzzer::NotificatorPrivate::alias() {
	if (alias_ == 0) {
		alias_ = new QLabel;
		alias_->setObjectName( "alias" );
		alias_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

		buzzer::Client* lClient = static_cast<buzzer::Client*>(buzzer::gApplication->getClient());
#if defined(Q_OS_WINDOWS)
		QFont lFont("Segoe UI Emoji", lClient->scaleFactor() * 11, QFont::Bold);
#else
		QFont lFont("Noto Color Emoji N", lClient->scaleFactor() * 11, QFont::Bold);
#endif
		alias_->setFont(lFont);
	}

	return alias_;
}

QLabel* buzzer::NotificatorPrivate::buzzer() {
	if (buzzer_ == 0) {
		buzzer_ = new QLabel;
		buzzer_->setObjectName( "buzzer" );
		buzzer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

		buzzer::Client* lClient = static_cast<buzzer::Client*>(buzzer::gApplication->getClient());
		QFont lFont = buzzer_->font();
		lFont.setPointSize(lClient->scaleFactor() * 11);
		buzzer_->setFont(lFont);
	}

	return buzzer_;
}

QLabel* buzzer::NotificatorPrivate::text() {
	if (text_ == 0) {
		text_ = new QLabel;
		text_->setObjectName( "text" );
		text_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		text_->setTextFormat(Qt::RichText);
		text_->setOpenExternalLinks(true);
		text_->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
		//text_->setElideMode(Qt::ElideRight);
		text_->setWordWrap(true);

		buzzer::Client* lClient = static_cast<buzzer::Client*>(buzzer::gApplication->getClient());
#if defined(Q_OS_WINDOWS)
		QFont lFont("Segoe UI Emoji", lClient->scaleFactor() * 11, QFont::Normal);
#else
		QFont lFont("Noto Color Emoji N", lClient->scaleFactor() * 11, QFont::Normal);
#endif
		text_->setFont(lFont);
	}

	return text_;
}

QLabel* buzzer::NotificatorPrivate::caption() {
	if (caption_ == 0) {
		caption_ = new QLabel;
		caption_->setObjectName( "caption" );
		caption_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		caption_->setTextFormat(Qt::RichText);
		caption_->setOpenExternalLinks(true);
		caption_->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
		caption_->setWordWrap(true);

		buzzer::Client* lClient = static_cast<buzzer::Client*>(buzzer::gApplication->getClient());
#if defined(Q_OS_WINDOWS)		
		QFont lFont("Segoe UI Emoji", lClient->scaleFactor() * 11, QFont::Normal);
#else
		QFont lFont("Noto Color Emoji N", lClient->scaleFactor() * 11, QFont::Normal);
#endif
		caption_->setFont(lFont);
	}

	return caption_;
}

QLabel* buzzer::NotificatorPrivate::media() {
	//
	if (media_ == 0) {
		media_ = new QLabel;
		media_->setObjectName( "media" );
		media_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		media_->setMaximumSize(60, 60);
		media_->setMinimumSize(60, 60);
	}

	return media_;
}

QLabel* buzzer::NotificatorPrivate::close() {
	if (close_ == 0) {
		close_ = new QLabel;
		close_->setObjectName( "close" );
		close_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		close_->setMaximumSize(20, 20);
		close_->setMinimumSize(20, 20);
		close_->setAlignment(Qt::AlignHCenter);
		close_->setText("\uf00d");

		buzzer::Client* lClient = static_cast<buzzer::Client*>(buzzer::gApplication->getClient());
		QFont lFont("Font Awesome 5 Pro Light", lClient->scaleFactor() * 12, QFont::Normal);
		close_->setFont(lFont);
	}

	return close_;
}

QLabel* buzzer::NotificatorPrivate::closeAll() {
	if (closeAll_ == 0) {
		closeAll_ = new QLabel;
		closeAll_->setObjectName( "closeAll" );
		closeAll_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		closeAll_->setMaximumSize(20, 20);
		closeAll_->setMinimumSize(20, 20);
		closeAll_->setAlignment(Qt::AlignHCenter);
		closeAll_->setText("\uf100");

		buzzer::Client* lClient = static_cast<buzzer::Client*>(buzzer::gApplication->getClient());
		QFont lFont("Font Awesome 5 Pro Light", lClient->scaleFactor() * 12, QFont::Normal);
		closeAll_->setFont(lFont);
	}

	return closeAll_;
}

//
//
//

buzzer::ElidedLabel::ElidedLabel(QWidget* parent, Qt::WindowFlags f)
	: QLabel(parent, f)
	, elideMode_(Qt::ElideRight)
{
	// setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

buzzer::ElidedLabel::ElidedLabel(const QString& txt, QWidget* parent, Qt::WindowFlags f)
	: QLabel(txt, parent, f)
	, elideMode_(Qt::ElideRight)
{
}

buzzer::ElidedLabel::ElidedLabel(const QString& txt, Qt::TextElideMode elideMode,
	QWidget *parent, Qt::WindowFlags f)
	: QLabel(txt, parent, f)
	, elideMode_(elideMode)
{
}

void buzzer::ElidedLabel::setText(const QString& txt) {
	QLabel::setText(txt);
	cacheElidedText(geometry().width());
}

void buzzer::ElidedLabel::cacheElidedText(int w) {
	cachedElidedText = fontMetrics().elidedText(text(), elideMode_, w, Qt::TextShowMnemonic);
}

void buzzer::ElidedLabel::resizeEvent(QResizeEvent* e) {
	QLabel::resizeEvent(e);
	cacheElidedText(e->size().width());
}

void buzzer::ElidedLabel::paintEvent(QPaintEvent* e) {
	if (elideMode_ == Qt::ElideNone) {
		QLabel::paintEvent(e);
	} else {
		QPainter p(this);
		p.drawText(0, 0,
			geometry().width(),
			geometry().height(),
			alignment(),
			cachedElidedText
		);
	}
}
