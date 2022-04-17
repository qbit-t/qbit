#include "imageqxloader.h"

#include <QThread>
#include <QMetaType>
#include <QPainter>
#include <QBrush>
#include <QDebug>
#include <QImageReader>

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
	connect (this, SIGNAL(loadTo(const QString&, ImageSharedPtr, bool)), SLOT(get(const QString&, ImageSharedPtr, bool)));
    _thread.start();
}

//------------------------------------------------------------------------------
ImageQxLoader& ImageQxLoader::instance()
{
	static ImageQxLoader instance;
    return instance;
}

//------------------------------------------------------------------------------
void ImageQxLoader::get(const QString &source, ImageSharedPtr image, bool autotransform)
{
	//
	QString lPath;
	if (source.startsWith("qrc:/")) lPath = source.mid(3);
	else if (source.startsWith("file://")) lPath = source.mid(7);

	QImageReader lReader(lPath);
	lReader.setAutoTransform(autotransform);
	lReader.setAutoDetectImageFormat(true);

	if (lReader.read(image.get())) {
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

