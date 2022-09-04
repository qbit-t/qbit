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

VideoRecorder::~VideoRecorder() {
	//
	if (videoRecorder_) {
		qInfo() << "VideoRecorder::~VideoRecorder()";

		disconnect(probe_, &QVideoProbe::videoFrameProbed, this, &VideoRecorder::videoFrameProbed);

		delete probe_;
		delete videoRecorder_;
		if (preview_) delete preview_;
	}
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
		maxDuration_ = 1 * 60 * 1000; // 1 minute
		emit maxDurationChanged();
	} else if (resolution_ == "720p") {
		maxDuration_ = 3 * 60 * 1000; // 3 minutes
		emit maxDurationChanged();
	} else if (resolution_ == "480p") {
		maxDuration_ = 5 * 60 * 1000; // 5 minutes
		emit maxDurationChanged();
	}
}

void VideoRecorder::toggleRecord() {
	//
	qInfo() << "VideoRecorder::toggleRecord" << videoRecorder_->state();
	//
	if (videoRecorder_->state() == QMediaRecorder::StoppedState) {
		//containers
		for (auto &containerName: videoRecorder_->supportedContainers()) {
			qInfo() << "Container" << containerName;
		}

		//video codecs
		bool lHas264 = false;
		for (auto &codecName: videoRecorder_->supportedVideoCodecs()) {
			qInfo() << "Video" << codecName;
			if (codecName == "264") lHas264 = true;
		}

		//video resolutions
		for (QSize sampleRate: videoRecorder_->supportedResolutions()) {
			qInfo() << "Video resolution" << sampleRate;
		}

		//video rate
		for (qreal sampleRate: videoRecorder_->supportedFrameRates()) {
			qInfo() << "Video frame rate" << sampleRate;
		}

		//audio codecs
		bool lHasAmr = false;
		for (auto &codecName: videoRecorder_->supportedAudioCodecs()) {
			qInfo() << "Audio" << codecName;
			if (codecName == "audio/amr") lHasAmr = true;
		}

		//sample rate
		bool lHas96000 = false;
		bool lHas48000 = false;
		bool lHas44000 = false;
		for (int sampleRate: videoRecorder_->supportedAudioSampleRates()) {
			qInfo() << "Audio rate" << sampleRate;
			if (sampleRate == 96000) lHas96000 = true;
			if (sampleRate == 48000) lHas48000 = true;
			if (sampleRate == 44000) lHas44000 = true;
		}

		//
		frame_ = 0;

		//
		QAudioEncoderSettings lAudioSettings;
		if (lHasAmr) lAudioSettings.setCodec("audio/amr");
		lAudioSettings.setEncodingMode(QMultimedia::AverageBitRateEncoding);

		QVideoEncoderSettings lVideoSettings;
		if (lHas264) lVideoSettings.setCodec("h264");
		lVideoSettings.setEncodingMode(QMultimedia::AverageBitRateEncoding);

		if (resolution_ == "1080p" || !resolution_.size()) {
			lVideoSettings.setBitRate(4*1024*1024); // 4 Mbit/s
			lAudioSettings.setChannelCount(2);
			if (lHas96000) lAudioSettings.setBitRate(96000);
			maxDuration_ = 1 * 60 * 1000; // 1 minute
			emit maxDurationChanged();
		} else if (resolution_ == "720p") {
			lVideoSettings.setBitRate(2*1024*1024); // 2 Mbit/s
			lAudioSettings.setChannelCount(2);
			if (lHas48000) lAudioSettings.setBitRate(48000);
			maxDuration_ = 3 * 60 * 1000; // 3 minutes
			emit maxDurationChanged();
		} else if (resolution_ == "480p") {
			lVideoSettings.setBitRate(1024*1024); // 1 Mbit/s
			lAudioSettings.setChannelCount(2);
			if (lHas44000) lAudioSettings.setBitRate(44000);
			maxDuration_ = 5 * 60 * 1000; // 5 minutes
			emit maxDurationChanged();
		}

		if (resolution_ == "1080p" || !resolution_.size()) lVideoSettings.setResolution(1920, 1080);
		else if (resolution_ == "720p") lVideoSettings.setResolution(1280, 720);
		else if (resolution_ == "480p") lVideoSettings.setResolution(720, 480);

		localFile_ = QString::fromStdString(qbit::Random::generate().toHex());
		emit localFileChanged();

		videoRecorder_->setVideoSettings(lVideoSettings);
		videoRecorder_->setAudioSettings(lAudioSettings);
		videoRecorder_->setContainerFormat("mp4");
		videoRecorder_->setOutputLocation(QUrl::fromLocalFile(localPath_ + "/" + localFile_ + ".mp4"));
		videoRecorder_->record();

		qInfo() << "START RECORDING" << videoRecorder_->videoSettings().resolution() << videoRecorder_->videoSettings().bitRate();
	} else {
		videoRecorder_->stop();
	}
}

void VideoRecorder::actualLocationChanged(const QUrl& location) {
	//
	actualFileLocation_ = location.toLocalFile();

	qInfo() << "[VideoRecorder::actualLocationChanged]:" << actualFileLocation_;

	//
	videoCaptured_ = true;

	//
	if (!previewSaved_ && frame_ >= 10) {
		previewSaved_ = savePreview();

		if (previewSaved_) emit actualFileLocationChanged();
	}
}

bool VideoRecorder::savePreview() {
	//
	bool lSaved = false;
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

#if !defined(DESKTOP_PLATFORM)

#ifdef Q_OS_ANDROID
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
#endif

#ifdef Q_OS_IOS
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
#endif

#endif

			QImage lNewImage = preview_->transformed(lMatrix);
			lSaved = lNewImage.save(lPreview, "JPG", 80);
		} else {
			lSaved = preview_->save(lPreview, "JPG", 80);
		}

#if !defined(DESKTOP_PLATFORM)
		qInfo() << "[VideoRecorder::actualLocationChanged]:" << lPreview << lSaved << lInfo.orientation() << orientation_ << unifiedOrientation_;
#endif

		previewLocation_ = lPreview;
		emit previewLocationChanged();

		return lSaved;
	} else {
		qInfo() << "[VideoRecorder::actualLocationChanged]: preview_ == nullptr";
	}

	return lSaved;
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
	qInfo() << "VideoRecorder::onStateChanged" << state;
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
	if (previewSaved_ || frame_ > 10) return;
	//
	frame_++;
	qInfo() << "VideoRecorder::videoFrameProbed" << frame_;
	if (preview_) delete preview_;
	preview_ = new QImage(buffer.image());

	//
	if (videoCaptured_ && !previewSaved_ && frame_ >= 10) {
		//
		previewSaved_ = savePreview();
		if (previewSaved_) emit actualFileLocationChanged();
	}
}
