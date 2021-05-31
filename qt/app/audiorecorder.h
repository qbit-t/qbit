#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QAudioProbe>
#include <QAudioRecorder>
#include <QMediaRecorder>

namespace buzzer {

class AudioRecorder : public QObject {
	//
	Q_OBJECT

	Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
	Q_PROPERTY(QString localPath READ localPath WRITE setLocalPath NOTIFY localPathChanged)
	Q_PROPERTY(QString localFile READ localFile NOTIFY localFileChanged)
	Q_PROPERTY(QString actualFileLocation READ actualFileLocation NOTIFY actualFileLocationChanged)
	Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
	Q_PROPERTY(bool isStopped READ isStopped NOTIFY isStoppedChanged)

public:
	explicit AudioRecorder(QObject *parent = nullptr);

	Q_INVOKABLE void record();
	Q_INVOKABLE void stop();
	Q_INVOKABLE void pause();

	qint64 duration() { return duration_; }
	QString localPath() { return localPath_; }
	QString localFile() { return localFile_; }
	QString actualFileLocation() { return actualFileLocation_; }
	void setLocalPath(const QString& localPath) { localPath_ = localPath; emit localPathChanged(); }
	bool isRecording() {
		if (audioRecorder_) {
			//
			return audioRecorder_->state() == QMediaRecorder::RecordingState;
		}

		return false;
	}

	bool isStopped() {
		if (audioRecorder_) {
			//
			return audioRecorder_->state() == QMediaRecorder::StoppedState;
		}

		return false;
	}

signals:
	void durationChanged();
	void recording();
	void stopped();
	void paused();
	void localPathChanged();
	void localFileChanged();
	void isRecordingChanged();
	void isStoppedChanged();
	void error(QString);
	void actualFileLocationChanged();

private slots:
	void togglePause();
	void toggleRecord();

	void updateStatus(QMediaRecorder::Status);
	void onStateChanged(QMediaRecorder::State);
	void updateProgress(qint64 duration);
	void processBuffer(const QAudioBuffer&);
	void errorMessage();
	void actualLocationChanged(const QUrl&);

private:
	QAudioRecorder* audioRecorder_ = nullptr;
	QAudioProbe* probe_ = nullptr;

	qint64 duration_ = 0;
	QString localPath_;
	QString localFile_;
	QString actualFileLocation_;
};

} // buzzer

#endif // AUDIORECORDER_H
