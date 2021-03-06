#ifndef BUZZER_IMAGE_LISTING_H
#define BUZZER_IMAGE_LISTING_H

#include <QObject>

#ifdef Q_OS_ANDROID

#include <QtAndroidExtras>

#include <QDebug>

namespace buzzer {

class ImageListing: public QObject, public QAndroidActivityResultReceiver
{
	Q_OBJECT

public:
	ImageListing();

	Q_INVOKABLE void listImages();

	virtual void handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject& data);

signals:
	void imageFound(QString file);
};

} // buzzer

#endif

#endif // BUZZER_IMAGE_LISTING_H
