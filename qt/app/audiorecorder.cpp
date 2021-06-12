#include "audiorecorder.h"
#include "../../random.h"

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

void AudioRecorder::toggleRecord() {
	//
	if (audioRecorder_->state() == QMediaRecorder::StoppedState) {
		/*
		//audio devices
		for (auto &device: audioRecorder_->audioInputs()) {
			qInfo() << "Input" << device;
		}

		//audio codecs
		for (auto &codecName: audioRecorder_->supportedAudioCodecs()) {
			qInfo() << "Codec" << codecName;
		}

		//containers
		for (auto &containerName: audioRecorder_->supportedContainers()) {
			qInfo() << "Codec" << containerName;
		}

		//sample rate
		for (int sampleRate: audioRecorder_->supportedAudioSampleRates()) {
			qInfo() << "Rate" << sampleRate;
		}
		*/

		//
		QAudioEncoderSettings lSettings;
		lSettings.setCodec("audio/amr");
		lSettings.setQuality(QMultimedia::HighQuality);
		localFile_ = QString::fromStdString(qbit::Random::generate().toHex());
		emit localFileChanged();

		audioRecorder_->setEncodingSettings(lSettings);
		audioRecorder_->setContainerFormat("amr");
		audioRecorder_->setOutputLocation(QUrl::fromLocalFile(localPath_ + "/" + localFile_));
		audioRecorder_->record();
	} else {
		audioRecorder_->stop();
	}
}

void AudioRecorder::actualLocationChanged(const QUrl& location) {
	//
	actualFileLocation_ = location.toLocalFile();
	// qInfo() << "[actualLocationChanged]" << actualFileLocation_;
	emit actualFileLocationChanged();
}

void AudioRecorder::updateStatus(QMediaRecorder::Status status) {
	//
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
	emit error(audioRecorder_->errorString());
}

void AudioRecorder::updateProgress(qint64 duration) {
	//
	duration_ = duration;
	emit durationChanged();
}

void AudioRecorder::processBuffer(const QAudioBuffer&) {
	//
	// TODO: collect and show levels
}
