#pragma once

#include <QObject>
#include <QThread>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QImage>

namespace buzzer {
typedef QSharedPointer<QImage> ImageSharedPtr;
typedef QWeakPointer<QImage> ImageWeakPtr;

/***************************************************************************//**
 * @brief Класс-singleton загрузщика изображений для ImageSp.
 *
 * @note Работает в параллельном потоке.
 ******************************************************************************/
class ImageQxLoader : public QObject {
    Q_OBJECT

	ImageQxLoader ();
    public:
		static ImageQxLoader& instance();
		static void stop();

    public slots:
        /// @brief Загружает изображение синхронно.
		void get (QString source, ImageSharedPtr image, bool autotransform);

    signals:
        /// @brief Загружает изображение ассинхронно.
		void loadTo(QString source, ImageSharedPtr image, bool autotransform);

        /// @brief [signal] Сигнал, уведомляющий о загрузке изображения.
		void loaded(const QString &source, ImageWeakPtr image);

        /// @brief [signal] Сигнал об ошибке при загрузке изображения.
		void error(const QString &source, ImageWeakPtr image, const QString &reason);

    private:
        QThread _thread;
}; // class ImageSpLoader
} // namespace sp {

Q_DECLARE_METATYPE(buzzer::ImageSharedPtr)
Q_DECLARE_METATYPE(buzzer::ImageWeakPtr)
