#ifndef VIDEORECORDER_H
#define VIDEORECORDER_H

#include <QObject>
#include <QVideoProbe>
#include <QMediaRecorder>
#include <QCamera>

#if !defined(DESKTOP_PLATFORM)
#include <QOrientationReading>
#endif

namespace buzzer {

class VideoRecorder : public QObject {
	//
	Q_OBJECT

	Q_PROPERTY(QVariant camera READ camera WRITE setCamera NOTIFY cameraChanged)
#if !defined(DESKTOP_PLATFORM)
	Q_PROPERTY(QOrientationReading::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
#endif
	Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
	Q_PROPERTY(qint64 maxDuration READ maxDuration NOTIFY maxDurationChanged)
	Q_PROPERTY(QString localPath READ localPath WRITE setLocalPath NOTIFY localPathChanged)
	Q_PROPERTY(QString localFile READ localFile NOTIFY localFileChanged)
	Q_PROPERTY(QString actualFileLocation READ actualFileLocation NOTIFY actualFileLocationChanged)
	Q_PROPERTY(QString previewLocation READ previewLocation NOTIFY previewLocationChanged)
	Q_PROPERTY(QString resolution READ resolution WRITE setResolution NOTIFY resolutionChanged)
	Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
	Q_PROPERTY(bool isStopped READ isStopped NOTIFY isStoppedChanged)
	Q_PROPERTY(unsigned short unifiedOrientation READ unifiedOrientation NOTIFY unifiedOrientationChanged)

public:
	explicit VideoRecorder(QObject *parent = nullptr);
	virtual ~VideoRecorder();

	Q_INVOKABLE void record();
	Q_INVOKABLE void stop();
	Q_INVOKABLE void pause();

	QVariant camera() { return QVariant::fromValue(camera_); }
#if !defined(DESKTOP_PLATFORM)
	QOrientationReading::Orientation orientation() { return orientation_; }
	void setOrientation(QOrientationReading::Orientation orientation) { orientation_ = orientation; emit orientationChanged(); }
#endif
	void setCamera(QVariant camera);
	qint64 duration() { return duration_; }
	qint64 maxDuration() { return maxDuration_; }
	unsigned short unifiedOrientation() { return unifiedOrientation_; }
	QString localPath() { return localPath_; }
	QString localFile() { return localFile_; }
	QString actualFileLocation() { return actualFileLocation_; }
	QString previewLocation() { return previewLocation_; }
	QString resolution() { return resolution_; }
	void setLocalPath(const QString& localPath) { localPath_ = localPath; emit localPathChanged(); }
	void setResolution(const QString&);
	bool isRecording() {
		if (videoRecorder_) {
			//
			return videoRecorder_->state() == QMediaRecorder::RecordingState;
		}

		return false;
	}

	bool isStopped() {
		if (videoRecorder_) {
			//
			return videoRecorder_->state() == QMediaRecorder::StoppedState;
		}

		return false;
	}

private:
	bool savePreview();

signals:
	void cameraChanged();
	void durationChanged();
	void maxDurationChanged();
	void recording();
	void stopped();
	void paused();
	void localPathChanged();
	void localFileChanged();
	void isRecordingChanged();
	void isStoppedChanged();
	void error(QString);
	void actualFileLocationChanged();
	void orientationChanged();
	void previewLocationChanged();
	void unifiedOrientationChanged();
	void resolutionChanged();

private slots:
	void togglePause();
	void toggleRecord();

	void updateStatus(QMediaRecorder::Status);
	void onStateChanged(QMediaRecorder::State);
	void updateProgress(qint64 duration);
	void videoFrameProbed(const QVideoFrame&);
	void errorMessage();
	void actualLocationChanged(const QUrl&);

private:
	QMediaRecorder* videoRecorder_ = nullptr;
	QVideoProbe* probe_ = nullptr;
	QCamera* camera_ = nullptr;
	QImage* preview_ = nullptr;

	qint64 duration_ = 0;
	qint64 maxDuration_ = 0;
	QString localPath_;
	QString localFile_;
	QString actualFileLocation_;
	bool previewSaved_ = false;
	bool videoCaptured_ = false;
#if !defined(DESKTOP_PLATFORM)
	QOrientationReading::Orientation orientation_ = QOrientationReading::Undefined;
#endif
	unsigned short unifiedOrientation_ = 0;
	QString previewLocation_;
	QString resolution_;

	int frame_ = 0;
};

} // buzzer

#endif // AUDIORECORDER_H
