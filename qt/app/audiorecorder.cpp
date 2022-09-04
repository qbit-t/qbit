#include "audiorecorder.h"
#include "../../random.h"
#include "iossystemdispatcher.h"

#include <QUrl>

using namespace buzzer;

AudioRecorder::AudioRecorder(QObject *parent) : QObject(parent) {
	audioRecorder_ = new QAudioRecorder(this);
	probe_ = new QAudioProbe(this);
	connect(probe_, &QAudioProbe::audioBufferProbed, this, &AudioRecorder::processBuffer);
	probe_->setSource(audioRecorder_);

	connect(audioRecorder_, &QAudioRecorder::durationChanged, this, &AudioRecorder::updateProgress);
	connect(audioRecorder_, &QAudioRecorder::statusChanged, this, &AudioRecorder::updateStatus);
	connect(audioRecorder_, &QAudioRecorder::stateChanged, this, &AudioRecorder::onStateChanged);
	connect(audioRecorder_, &QAudioRecorder::actualLocationChanged, this, &AudioRecorder::actualLocationChanged);
	connect(audioRecorder_, QOverload<QMediaRecorder::Error>::of(&QAudioRecorder::error), this, &AudioRecorder::errorMessage);
}

void AudioRecorder::record() {
	toggleRecord();
}

void AudioRecorder::stop() {
	toggleRecord();
}

void AudioRecorder::pause() {
	togglePause();
}

void AudioRecorder::togglePause() {
	if (audioRecorder_->state() != QMediaRecorder::PausedState) {
		audioRecorder_->pause();
		emit paused();
	} else {
		audioRecorder_->record();
		emit recording();
	}
}

void AudioRecorder::onReceived(QString name, QVariantMap data) {
	//
	if (name == "currentRecordingTimeCallback") {
		int lSeconds = data["currentTime"].value<int>();
		qInfo() << "AudioRecorder::onReceived" << lSeconds;
		duration_ = lSeconds * 1000;
		emit durationChanged();
	}
}

void AudioRecorder::queryQurrentTime() {
	//
	if (recording_) {
		QISystemDispatcher* system = QISystemDispatcher::instance();
		QVariantMap data;
		system->dispatch("currentRecordingTime",data);

		checkTimer_->start(500);
	}
}

void AudioRecorder::toggleRecord() {
	//
#ifdef Q_OS_IOS
	//
	if (!recording_) {
		QISystemDispatcher* system = QISystemDispatcher::instance();
		//
		localFile_ = QString::fromStdString(qbit::Random::generate().toHex());
		actualFileLocation_ = localPath_ + "/" + localFile_ + ".caf";

		QVariantMap data;
		data["localFile"] = QString(actualFileLocation_);

		connect(system, SIGNAL(dispatched(QString, QVariantMap)), this, SLOT(onReceived(QString,QVariantMap)));
		system->dispatch("beginRecordAudio",data);

		recording_ = true;

		if (checkTimer_) {
			disconnect(checkTimer_, &QTimer::timeout, 0, 0);
			delete checkTimer_;
		}

		checkTimer_ = new QTimer(this);
		connect(checkTimer_, &QTimer::timeout, this, &AudioRecorder::queryQurrentTime);
		checkTimer_->start(500); // 500 ms

        emit isRecordingChanged();
        emit isStoppedChanged();
        emit recording();
    } else {
		QISystemDispatcher* system = QISystemDispatcher::instance();
		QVariantMap data;
		system->dispatch("endRecordAudio",data);
		recording_ = false;

		emit isRecordingChanged();
		emit isStoppedChanged();
		emit stopped();
	}

#else	
	//
	if (audioRecorder_->state() == QMediaRecorder::StoppedState) {

		//audio devices
		for (auto &device: audioRecorder_->audioInputs()) {
			qInfo() << "Input" << device;
		}

		//audio codecs
		bool lHasAmr = false;
		bool lHasPcm = false;
		for (auto &codecName: audioRecorder_->supportedAudioCodecs()) {
			qInfo() << "Codec" << codecName;
			if (codecName == "audio/amr") lHasAmr = true;
			if (codecName == "audio/pcm") lHasPcm = true;
		}

		//containers
		bool lHasWavContainer = false;
		QString lWavContainer;
		bool lHasAmrContainer = false;
		QString lAmrContainer;
		for (auto &containerName: audioRecorder_->supportedContainers()) {
			qInfo() << "Containers" << containerName;
			if (containerName.contains("wav")) {
				lHasWavContainer = true;
				lWavContainer = containerName;
			}

			if (containerName.contains("amr")) {
				lHasAmrContainer = true;
				lAmrContainer = containerName;
			}
		}

		//sample rate
		for (int sampleRate: audioRecorder_->supportedAudioSampleRates()) {
			qInfo() << "Rate" << sampleRate;
		}

		//
		QAudioEncoderSettings lSettings;
		if (lHasPcm) lSettings.setCodec("audio/pcm");
		else if (lHasAmr) lSettings.setCodec("audio/amr");
		lSettings.setQuality(QMultimedia::HighQuality);
		localFile_ = QString::fromStdString(qbit::Random::generate().toHex());
		emit localFileChanged();

		audioRecorder_->setEncodingSettings(lSettings);
		if (lHasWavContainer) audioRecorder_->setContainerFormat(lWavContainer);
		else if (lHasAmrContainer) audioRecorder_->setContainerFormat(lAmrContainer);

		audioRecorder_->setOutputLocation(QUrl::fromLocalFile(localPath_ + "/" + localFile_ + (lHasWavContainer ? ".wav" : ".amr")));
		audioRecorder_->record();

		qInfo() << "RECORDING" << audioRecorder_->audioInput();
	} else {
		audioRecorder_->stop();
	}
#endif
}

void AudioRecorder::actualLocationChanged(const QUrl& location) {
	//
	actualFileLocation_ = location.toLocalFile();
	qInfo() << "[actualLocationChanged]" << actualFileLocation_;
	emit actualFileLocationChanged();
}

void AudioRecorder::updateStatus(QMediaRecorder::Status status) {
	//
    qInfo() << "AudioRecorder::updateStatus" << status;
	switch (status) {
	case QMediaRecorder::RecordingStatus:
		break;
	case QMediaRecorder::PausedStatus:
		break;
	case QMediaRecorder::UnloadedStatus:
		break;
	case QMediaRecorder::LoadedStatus:
	default:
		break;
	}
}

void AudioRecorder::onStateChanged(QMediaRecorder::State state) {
	//
    qInfo() << "AudioRecorder::onStateChanged" << state;
	switch (state) {
	case QMediaRecorder::RecordingState:
			emit isRecordingChanged();
			emit isStoppedChanged();
			emit recording();
		break;
	case QMediaRecorder::PausedState:
			emit isRecordingChanged();
			emit isStoppedChanged();
			emit paused();
		break;
	case QMediaRecorder::StoppedState:
			emit isRecordingChanged();
			emit isStoppedChanged();
			emit stopped();
		break;
	}
}

void AudioRecorder::errorMessage() {
	//
    qWarning() << "audioRecorder/error" << audioRecorder_->errorString();
	emit error(audioRecorder_->errorString());
}

void AudioRecorder::updateProgress(qint64 duration) {
	//
	qInfo() << "AudioRecorder::updateProgress" << duration;
	duration_ = duration;
	emit durationChanged();
}

void AudioRecorder::processBuffer(const QAudioBuffer&) {
	//
	// TODO: collect and show levels
}
