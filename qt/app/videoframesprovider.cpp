#include "videoframesprovider.h"

using namespace buzzer;

VideoFramesProvider::VideoFramesProvider() {
	qDebug() << "FrameProvider construct";
}

VideoFramesProvider::~VideoFramesProvider() {
	qDebug() << "FrameProvider destruct";
}

QAbstractVideoSurface* VideoFramesProvider::videoSurface() {
	qDebug() << "FrameProvider return videoSurface";
	return m_surface;
}

void VideoFramesProvider::setVideoSurface(QAbstractVideoSurface* surface) {
	qDebug() << "FrameProvider setVideoSurface:" << surface;
	if (m_surface && m_surface != surface && m_surface->isActive()) {
		m_surface->stop();
	}

	m_surface = surface;

	if (m_surface && m_format.isValid()) {
		m_format = m_surface->nearestFormat(m_format);
		m_surface->start(m_format);
		qDebug() << "FrameProvider setVideoSurface start m_surface, m_format:" << m_format.pixelFormat();
	}
}

void VideoFramesProvider::setFormat(int width, int heigth, QVideoFrame::PixelFormat format) {
	//
	qDebug() << "FrameProvider setFormat width:" << width << "height:" << heigth << "format:" << format;
	//
	QSize size(width, heigth);
	QVideoSurfaceFormat formats(size, format);

	m_format = formats;

	if (m_surface) {
		if (m_surface->isActive()) {
			m_surface->stop();
		}

		m_format = m_surface->nearestFormat(m_format);
		m_surface->start(m_format);

		qDebug() << "FrameProvider setFormat start m_surface, m_format:" << m_format.pixelFormat();
	}
}

void VideoFramesProvider::onNewVideoContentReceived(const QVideoFrame& frame) {
	//
	// qDebug() << "FrameProvider onNewVideoContentReceived";
	//
	if (m_surface) {
		//qDebug() << "FrameProvider onNewVideoContentReceived" << "surface exists";
		if (frame.isValid())
		{
			qDebug() << "FrameProvider onNewVideoContentReceived" << "frame valid" << frame.pixelFormat();
			m_surface->present(frame);
		} else {
			qDebug() << "FrameProvider onNewVideoContentReceived" << "frame INVALID" << frame.pixelFormat();
		}
	}
}
