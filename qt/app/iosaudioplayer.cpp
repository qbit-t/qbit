#include "iosaudioplayer.h"
#include "../../random.h"
#include "iossystemdispatcher.h"

#include <QUrl>
#include <QDebug>

using namespace buzzer;

AudioPlayer::AudioPlayer(QQuickItem *parent) : QQuickItem(parent) {
}

void AudioPlayer::play() {
	//
	QISystemDispatcher* system = QISystemDispatcher::instance();
	
	//
	QString lFile = source_;
	if (lFile.startsWith("qrc:/")) lFile = source_.mid(3);
	else if (lFile.startsWith("file://")) lFile = source_.mid(7);

	//
	QVariantMap data;
	data["source"] = lFile;

	connect(system, SIGNAL(dispatched(QString, QVariantMap)), this, SLOT(onReceived(QString, QVariantMap)));
	system->dispatch("beginPlayAudio", data);

	state_ = AudioPlayer::Playing;

	if (checkTimer_) {
		disconnect(checkTimer_, &QTimer::timeout, 0, 0);
		delete checkTimer_;
	}

	checkTimer_ = new QTimer(this);
	connect(checkTimer_, &QTimer::timeout, this, &AudioPlayer::queryQurrentTime);
	checkTimer_->start(500); // 500 ms
}

void AudioPlayer::stop() {
	//
	QISystemDispatcher* system = QISystemDispatcher::instance();
	QVariantMap data;
	system->dispatch("endPlayAudio", data);
	state_ = AudioPlayer::Stopped;
}

void AudioPlayer::pause() {
}


void AudioPlayer::onReceived(QString name, QVariantMap data) {
	//
	if (name == "currentAudioPlayTimeCallback") {
		int lSeconds = data["currentTime"].value<int>();
		qInfo() << "AudioPlayer::onReceived" << lSeconds;
		position_ = lSeconds * 1000;
		emit positionChanged();
	}
}

void AudioPlayer::queryQurrentTime() {
	//
	if (state_ == AudioPlayer::Playing) {
		QISystemDispatcher* system = QISystemDispatcher::instance();
		QVariantMap data;
		system->dispatch("currentAudioPlayTime",data);

		checkTimer_->start(500);
	}
}
