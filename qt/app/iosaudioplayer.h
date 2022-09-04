#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QQuickItem>
#include <QTimer>

namespace buzzer {

class AudioPlayer : public QQuickItem {
	//
	Q_OBJECT
	Q_ENUMS(State)
	Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
	Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
	Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)

public:
    enum State {
        Stopped,
        Playing,
        Paused
    };

    explicit AudioPlayer(QQuickItem *parent = nullptr);

	Q_INVOKABLE void play();
	Q_INVOKABLE void stop();
	Q_INVOKABLE void pause();

	qint64 duration() { return duration_; }
	qint64 position() { return position_; }
	void setPosition(qint64 position) { position_ = position; }
	QString source() { return source_; }
	void setSource(const QString& source) { source_ = source; emit sourceChanged(); }

signals:
	void positionChanged();
	void sourceChanged();
	void durationChanged();

private slots:
	void onReceived(QString name, QVariantMap data);
	void queryQurrentTime();

private:
	qint64 duration_ = 0;
	qint64 position_ = 0;
	QString source_;
	State state_ = AudioPlayer::Stopped;
	QTimer* checkTimer_ = nullptr;
};

} // buzzer

#endif // AUDIORECORDER_H
