//videosurface.cpp
#include "videosurface.h"
#include <QDebug>

using namespace buzzer;

VideoSurfaces::VideoSurfaces(QObject *parent)
	: QAbstractVideoSurface(parent)
{
}

VideoSurfaces::VideoSurfaces(const QVector<QAbstractVideoSurface *> &s, QObject *parent)
	: QAbstractVideoSurface(parent)
	, m_surfaces(s)
{
	for (auto a : s) {
		connect(a, &QAbstractVideoSurface::supportedFormatsChanged, this, [this, a] {
			auto context = property("GLContext").value<QObject *>();
			if (!context)
				setProperty("GLContext", a->property("GLContext"));

			emit supportedFormatsChanged();
		});
	}
}

VideoSurfaces::~VideoSurfaces()
{
	qInfo() << "VideoSurfaces::~VideoSurfaces";
}

QList<QVideoFrame::PixelFormat> VideoSurfaces::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const
{
	QList<QVideoFrame::PixelFormat> result;
	QMap<QVideoFrame::PixelFormat, int> formats;
	for (auto &s : m_surfaces) {
		for (auto &p : s->supportedPixelFormats(type)) {
			if (++formats[p] == m_surfaces.size())
				result << p;
		}
	}

	qInfo() << "VideoSurfaces::supportedPixelFormats" << result;

	return result;
}

bool VideoSurfaces::start(const QVideoSurfaceFormat &format)
{
	bool result = true;
	for (auto &s : m_surfaces)
		result &= s->start(format);

	if (result) m_format = format;

	qInfo() << "VideoSurfaces::start" << result << m_format;

	emit surfaceFormatChanged(m_format);
	emit activeChanged(true);

	return result && QAbstractVideoSurface::start(format);
}

void VideoSurfaces::stop()
{
	for (auto &s : m_surfaces)
		s->stop();

	qInfo() << "VideoSurfaces::stop" << m_format;

	QAbstractVideoSurface::stop();
}

bool VideoSurfaces::present(const QVideoFrame &frame)
{
	bool result = true;
	if (isActive()) {
		for (auto &s : m_surfaces)
			if (s->isActive()) result &= s->present(frame);

		//qInfo() << "VideoSurfaces::present" << result;
	} else {
		//qInfo() << "VideoSurfaces::present" << "NOT active";
	}

	emit nativeResolutionChanged(nativeResolution());

	return result;
}

void VideoSurfaces::pushSurface(QAbstractVideoSurface* surface) {
	//
	qInfo() << "VideoSurfaces::pushSurface" << surface << m_surfaces.size();
	//
	if (surface) {
		//
		m_surfaces.push_back(surface);
		//
		connect(surface, &QAbstractVideoSurface::supportedFormatsChanged, this, [this, surface] {
			auto context = property("GLContext").value<QObject *>();
			if (!context)
				setProperty("GLContext", surface->property("GLContext"));

			emit supportedFormatsChanged();
		});

		start(m_format);

		emit surfaceFormatChanged(m_format);
		emit nativeResolutionChanged(nativeResolution());
	}
}

void VideoSurfaces::popSurface() {
	//
	qInfo() << "VideoSurfaces::popSurface" << m_surfaces.size();
	m_surfaces.pop_back();

	start(m_format);

	emit surfaceFormatChanged(m_format);
	emit nativeResolutionChanged(nativeResolution());
}

void VideoSurfaces::clearSurfaces() {
	//
	stop();
	m_surfaces.clear();
}
