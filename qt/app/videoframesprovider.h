#ifndef VIDEOFRAMESPROVIDER_H
#define VIDEOFRAMESPROVIDER_H

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <QDebug>

namespace buzzer {

class VideoFramesProvider : public QObject {
	Q_OBJECT

	Q_PROPERTY(QAbstractVideoSurface *videoSurface READ videoSurface WRITE setVideoSurface)
public:
	VideoFramesProvider();
	~VideoFramesProvider();
	QAbstractVideoSurface* videoSurface();
	void setVideoSurface(QAbstractVideoSurface* surface);
	Q_INVOKABLE void setFormat(int width, int heigth, QVideoFrame::PixelFormat format);

public slots:
	void onNewVideoContentReceived(const QVideoFrame& frame);

private:
	QAbstractVideoSurface *m_surface = NULL;
	QVideoSurfaceFormat m_format;
};

} // buzzer

#endif // VIDEOFRAMESPROVIDER_H
