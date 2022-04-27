//videosurface.h
#ifndef VIDEOSURFACE_H
#define VIDEOSURFACE_H

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include "imageqxloader.h"

namespace buzzer {

class VideoSurfaces : public QAbstractVideoSurface {
	Q_OBJECT
public:
	VideoSurfaces(QObject *parent = nullptr);
	VideoSurfaces(const QVector<QAbstractVideoSurface *> &surfaces, QObject *parent = nullptr);
	~VideoSurfaces();

	QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const override;
	bool start(const QVideoSurfaceFormat &format) override;
	void stop() override;
	bool present(const QVideoFrame &frame) override;

	Q_INVOKABLE void pushSurface(QAbstractVideoSurface* surface);
	Q_INVOKABLE void popSurface();
	Q_INVOKABLE void clearSurfaces();

private:
	QVector<QAbstractVideoSurface *> m_surfaces;
	QVector<QAbstractVideoSurface *> m_stack;
	QVideoSurfaceFormat m_format;
	Q_DISABLE_COPY(VideoSurfaces)
};

} // buzzer

#endif // VIDEOSURFACE_H
