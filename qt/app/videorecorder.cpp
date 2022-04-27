#include "videorecorder.h"
#include "../../random.h"

#include <QUrl>
#include <QCameraInfo>

using namespace buzzer;

VideoRecorder::VideoRecorder(QObject *parent) : QObject(parent) {
}

void VideoRecorder::setCamera(QVariant camera) {
	//
	QObject* lCamera = qvariant_cast<QObject*>(camera);
	qInfo() << "VideoRecorder::setCamera" << camera << lCamera;

	camera_ = qvariant_cast<QCamera*>(lCamera->property("mediaObject"));

	if (videoRecorder_) delete videoRecorder_;

	videoRecorder_ = new QMediaRecorder(camera_, this);
	probe_ = new QVideoProbe(this);
	connect(probe_, &QVideoProbe::videoFrameProbed, this, &VideoRecorder::videoFrameProbed);
	probe_->setSource(videoRecorder_);

	connect(videoRecorder_, &QMediaRecorder::durationChanged, this, &VideoRecorder::updateProgress);
	connect(videoRecorder_, &QMediaRecorder::statusChanged, this, &VideoRecorder::updateStatus);
	connect(videoRecorder_, &QMediaRecorder::stateChanged, this, &VideoRecorder::onStateChanged);
	connect(videoRecorder_, &QMediaRecorder::actualLocationChanged, this, &VideoRecorder::actualLocationChanged);
	connect(videoRecorder_, QOverload<QMediaRecorder::Error>::of(&QMediaRecorder::error), this, &VideoRecorder::errorMessage);

	emit cameraChanged();
}

void VideoRecorder::record() {
	toggleRecord();
}

void VideoRecorder::stop() {
	toggleRecord();
}

void VideoRecorder::pause() {
	togglePause();
}

void VideoRecorder::togglePause() {
	if (videoRecorder_->state() != QMediaRecorder::PausedState) {
		videoRecorder_->pause();
		emit paused();
	} else {
		videoRecorder_->record();
		emit recording();
	}
}

void VideoRecorder::setResolution(const QString& resolution) {
	//
	resolution_ = resolution;
	emit resolutionChanged();

	//
	if (resolution_ == "1080p" || !resolution_.size()) {
		maxDuration_ = 3 * 60 * 1000; // 3 minute
		emit maxDurationChanged();
	} else if (resolution_ == "720p") {
		maxDuration_ = 5 * 60 * 1000; // 5 minutes
		emit maxDurationChanged();
	} else if (resolution_ == "480p") {
		maxDuration_ = 7 * 60 * 1000; // 7 minutes
		emit maxDurationChanged();
	} else if (resolution_ == "360p") {
		maxDuration_ = 10 * 60 * 1000; // 10 minutes
		emit maxDurationChanged();
	}
}

void VideoRecorder::toggleRecord() {
	//
	qInfo() << "VideoRecorder::toggleRecord" << videoRecorder_->state();
	//
	if (videoRecorder_->state() == QMediaRecorder::StoppedState) {
		/*
		//audio codecs
		for (auto &codecName: videoRecorder_->supportedAudioCodecs()) {
			qInfo() << "Audio" << codecName;
		}

		//video codecs
		for (auto &codecName: videoRecorder_->supportedVideoCodecs()) {
			qInfo() << "Video" << codecName;
		}

		//containers
		for (auto &containerName: videoRecorder_->supportedContainers()) {
			qInfo() << "Container" << containerName;
		}

		//sample rate
		for (int sampleRate: videoRecorder_->supportedAudioSampleRates()) {
			qInfo() << "Rate" << sampleRate;
		}
		*/

		//
		frame_ = 0;

		//
		QAudioEncoderSettings lSettings;
		lSettings.setCodec("audio/amr");
		lSettings.setQuality(QMultimedia::HighQuality);

		QVideoEncoderSettings lVideoSettings;
		lVideoSettings.setCodec("h264");
		//lVideoSettings.setFrameRate(24.0);

		if (resolution_ == "1080p" || !resolution_.size()) {
			lVideoSettings.setBitRate(2*1024*1024); // 1 Mbit/s
			maxDuration_ = 3 * 60 * 1000; // 3 minute
			emit maxDurationChanged();
		} else if (resolution_ == "720p") {
			lVideoSettings.setBitRate(1024*1024); // 0.7 Mbit/s
			maxDuration_ = 5 * 60 * 1000; // 5 minutes
			emit maxDurationChanged();
		} else if (resolution_ == "480p") {
			lVideoSettings.setBitRate(500*1024); // 0.5 Mbit/s
			maxDuration_ = 7 * 60 * 1000; // 7 minutes
			emit maxDurationChanged();
		} else if (resolution_ == "360p") {
			lVideoSettings.setBitRate(200*1024); // 0.2 Mbit/s
			maxDuration_ = 10 * 60 * 1000; // 10 minutes
			emit maxDurationChanged();
		}

		if (resolution_ == "1080p" || !resolution_.size()) lVideoSettings.setResolution(1920, 1080);
		else if (resolution_ == "720p") lVideoSettings.setResolution(1280, 720);
		else if (resolution_ == "480p") lVideoSettings.setResolution(640, 480);
		else if (resolution_ == "360p") lVideoSettings.setResolution(640, 360);

		lVideoSettings.setQuality(QMultimedia::HighQuality);

		localFile_ = QString::fromStdString(qbit::Random::generate().toHex());
		emit localFileChanged();

		videoRecorder_->setEncodingSettings(lSettings, lVideoSettings);
		videoRecorder_->setContainerFormat("mp4");
		videoRecorder_->setOutputLocation(QUrl::fromLocalFile(localPath_ + "/" + localFile_));
		videoRecorder_->record();

		qInfo() << "START RECORDING";
	} else {
		videoRecorder_->stop();
	}
}

void VideoRecorder::actualLocationChanged(const QUrl& location) {
	//
	actualFileLocation_ = location.toLocalFile();
	//
	if (preview_) {
		//
		QString lPreview = localPath_ + "/" + localFile_ + ".jpeg";
		// final orientation
		QCameraInfo lInfo(*camera_);

		if (lInfo.orientation() != 0) {
			// ---- 270 -----
			//
			// +------+
			// |    ^ | - (180) / LeftUp
			// +------+
			//
			// +---+
			// |<  | - (90) / TopUp
			// |   |
			// +---+
			//
			// +------+
			// |    . | - (0) / RightUp
			// +------+
			//
			// +---+
			// |   | - (-90) / TopDown
			// |  >|
			// +---+
			//
			// ---- 90 -----
			//
			// +------+
			// |    . | - (180) / LeftUp
			// +------+
			//
			// +---+
			// |  >| - (-90) / TopUp
			// |   |
			// +---+
			//
			// +------+
			// |    ^ | - (0) / RightUp
			// +------+
			//
			// +---+
			// |   | - (90) / TopDown
			// |<  |
			// +---+

			// transform
			QPoint lCenter = preview_->rect().center();
			QTransform lMatrix;

			lMatrix.translate(lCenter.x(), lCenter.y());
			if (lInfo.orientation() == 270 && (orientation_ == QOrientationReading::TopUp || orientation_ == QOrientationReading::FaceUp)) {
				lMatrix.rotate(90);
				unifiedOrientation_ = 6;
				emit unifiedOrientationChanged();
			} else if (lInfo.orientation() == 270 && orientation_ == QOrientationReading::TopDown) {
				lMatrix.rotate(-90);
				unifiedOrientation_ = 8;
				emit unifiedOrientationChanged();
			} else if (lInfo.orientation() == 270 && orientation_ == QOrientationReading::LeftUp) {
				lMatrix.rotate(180);
				unifiedOrientation_ = 3;
				emit unifiedOrientationChanged();

			} else if (lInfo.orientation() == 90 && (orientation_ == QOrientationReading::TopUp || orientation_ == QOrientationReading::FaceUp)) {
				lMatrix.rotate(-90);
				unifiedOrientation_ = 8;
				emit unifiedOrientationChanged();
			} else if (lInfo.orientation() == 90 && orientation_ == QOrientationReading::TopDown) {
				lMatrix.rotate(90);
				unifiedOrientation_ = 6;
				emit unifiedOrientationChanged();
			} else if (lInfo.orientation() == 90 && orientation_ == QOrientationReading::LeftUp) {
				lMatrix.rotate(180);
				unifiedOrientation_ = 3;
				emit unifiedOrientationChanged();
			}

			QImage lNewImage = preview_->transformed(lMatrix);
			lNewImage.save(lPreview, "JPG", 80);
		} else {
			preview_->save(lPreview, "JPG", 80);
		}

		//qInfo() << "[actualLocationChanged]:" << lPreview << lInfo.orientation() << orientation_ << unifiedOrientation_;

		previewLocation_ = lPreview;
		emit previewLocationChanged();
	}

	emit actualFileLocationChanged();
}

void VideoRecorder::updateStatus(QMediaRecorder::Status status) {
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

void VideoRecorder::onStateChanged(QMediaRecorder::State state) {
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

void VideoRecorder::errorMessage() {
	//
	emit error(videoRecorder_->errorString());
}

void VideoRecorder::updateProgress(qint64 duration) {
	//
	duration_ = duration;
	emit durationChanged();
}

void VideoRecorder::videoFrameProbed(const QVideoFrame& buffer) {
	//
	if (frame_++ > 5) return;
	if (!preview_) delete preview_;
	preview_ = new QImage(buffer.image());
}
