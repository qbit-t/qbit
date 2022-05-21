#include "imageqxloader.h"

#include <QThread>
#include <QMetaType>
#include <QPainter>
#include <QBrush>
#include <QDebug>
#include <QImageReader>
#include <QFileInfo>

using namespace buzzer;

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif
int __idImageSharedPtr = qRegisterMetaType<ImageSharedPtr>("ImageSharedPtr");
int __idImageWeakPtr = qRegisterMetaType<ImageWeakPtr>("ImageWeakPtr");
#ifdef __clang__
#  pragma clang diagnostic pop
#endif

//------------------------------------------------------------------------------
ImageQxLoader::ImageQxLoader()
    : QObject (nullptr)
{
    moveToThread(&_thread);
	connect (this, SIGNAL(loadTo(QString, ImageSharedPtr, bool)), SLOT(get(QString, ImageSharedPtr, bool)), Qt::QueuedConnection);
    _thread.start();
}

//------------------------------------------------------------------------------
ImageQxLoader& ImageQxLoader::instance()
{
	static ImageQxLoader instance;
    return instance;
}

//------------------------------------------------------------------------------
void ImageQxLoader::get(QString source, ImageSharedPtr image, bool autotransform)
{
	//
	QString lPath;
	if (source.startsWith("qrc:/")) lPath = source.mid(3);
	else if (source.startsWith("file://"))
#if defined(Q_OS_WINDOWS)
		lPath = source.mid(8);
#else
		lPath = source.mid(7);
#endif

	QImageReader lReader(lPath);
	lReader.setAutoTransform(autotransform);
	lReader.setAutoDetectImageFormat(true);

	QString lFile = lPath;
	QFileInfo lInfo(lFile);

	QImage lImage = lReader.read();
	//qInfo() << "[ImageQxLoader::get]:" << source << lImage.sizeInBytes() << lInfo.size();

	if (!lImage.isNull()) {
		image->swap(lImage);
		emit loaded(lPath, image);
	} else {
		emit error(lPath, image, lReader.errorString());
	}

	/*
	if (image->load(lPath)) {
		emit loaded(lPath, image);
	} else {
		emit error(lPath, image, QString("Ошибка загрузки изображения: %1").arg(lPath));
	}
	*/
	// return;

	//*image = QImage();
	//emit error(source, image, "Неизвестный способ загрузки изображения.");
}

