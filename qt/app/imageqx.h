#pragma once

#include "imageqxloader.h"
#include "imageqxnode.h"

#include <QQuickItem>
#include <QImage>

namespace buzzer {
/**
 * @brief Класс для отрисовки изображения с закруглёнными краями через Scene Graph.
 */
class ImageQx: public QQuickItem
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

	Q_PROPERTY (QString  source         READ source        WRITE setSource        NOTIFY sourceChanged)
	Q_PROPERTY (float    radius         READ radius        WRITE setRadius        NOTIFY radiusChanged)
	Q_PROPERTY (QSize    sourceSize     READ sourceSize                           NOTIFY sourceSizeChanged)
	Q_PROPERTY (FillMode fillMode       READ fillMode      WRITE setFillMode      NOTIFY fillModeChanged)
	Q_PROPERTY (Status   status         READ status                               NOTIFY statusChanged)
	Q_PROPERTY (bool     asynchronous   READ asynchronous  WRITE setAsynchronous  NOTIFY autotransformChanged)
	Q_PROPERTY (bool     autoTransform  READ autotransform WRITE setAutotransform NOTIFY asynchronousChanged)
	Q_PROPERTY (float    coeff          READ coeff                                NOTIFY coeffChanged)
	Q_PROPERTY (int      originalWidth  READ originalWidth                        NOTIFY originalWidthChanged)
	Q_PROPERTY (int      originalHeight READ originalHeight                       NOTIFY originalHeightChanged)
	Q_PROPERTY (QRectF   contentRect    READ contentRect                          NOTIFY contentRectChanged)
	Q_PROPERTY (QString  errorString    READ errorString                          NOTIFY errorStringChanged)

    /** @property mipmap
     *  @brief Флаг mipmap-фильтрации изображения, оно же более качественное масштабирование.
     *  @note  На iOS может глючить, нужно проверить
     */
    Q_PROPERTY (bool mipmap READ mipmap WRITE setMipmap NOTIFY mipmapChanged)

    Q_PROPERTY (HorizontalAlignment horizontalAlignment READ horizontalAlignment WRITE setHorizontalAlignment NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY (VerticalAlignment   verticalAlignment   READ verticalAlignment   WRITE setVerticalAlignment   NOTIFY verticalAlignmentChanged)

    public:
        enum FillMode {
            Stretch = 0
            ,PreserveAspectFit = 1
            ,PreserveAspectCrop = 2
            ,Pad = 6
            ,Parallax = 8
        };

        /** @brief Горизонтальное выравнивание */
        enum HorizontalAlignment {
            AlignLeft = 1
            , AlignRight = 2
            , AlignHCenter = 4
        };

        /** @brief Вертикальное выравнивание */
        enum VerticalAlignment {
            AlignTop = 32
            , AlignBottom = 64
            , AlignVCenter = 128
        };

        enum Status {
            Null = 0
            , Ready = 1
            , Loading = 2
            , Error = 3
        };

        Q_ENUM (FillMode)
        Q_ENUM (HorizontalAlignment)
        Q_ENUM (VerticalAlignment)
        Q_ENUM (Status)

    public:
		ImageQx(QQuickItem *parent = nullptr);
		~ImageQx();

        virtual void classBegin() override;
        virtual void componentComplete() override;
        QSGNode* updatePaintNode(QSGNode *node, UpdatePaintNodeData *) override;

        const QString& source() const { return _source; }
        float    radius() const { return _radius; }
        QSize    sourceSize() const { return _image->size(); }
        FillMode fillMode() const { return _fillMode; }
        Status   status() const { return _status; }
		bool     asynchronous() const { return _asynchronous; }
		bool     autotransform() const { return _autotransform; }
		bool     mipmap() const { return _mipmap; }
        HorizontalAlignment horizontalAlignment() const { return _horizontalAlignment; }
        VerticalAlignment   verticalAlignment() const { return _verticalAlignment; }
		float coeff() { return _coeff; }
		int originalWidth() { return _originalWidth; }
		int originalHeight() { return _originalHeight; }
		QRectF contentRect() { return QRectF(0 , 0, _image->width(), _image->height()); }
		QString errorString() { return _errorString; }

        void setSource(const QString &source);
        void setRadius(float radius);
        void setFillMode(FillMode fillMode);
		void setAsynchronous(bool asynchronous);
		void setAutotransform(bool autotransform);
		void setMipmap(bool mipmap);
        void setHorizontalAlignment (HorizontalAlignment horizontalAlignment);
        void setVerticalAlignment (VerticalAlignment verticalAlignment);

		Q_INVOKABLE void setUpdateDimensions(int, int);

    signals:
        void sourceChanged(const QString&);
        void radiusChanged(float);
        void sourceSizeChanged(const QSize&);
        void statusChanged(Status);
        void fillModeChanged(FillMode);
		void asynchronousChanged(bool);
		void autotransformChanged(bool);
		void mipmapChanged(bool);
        void horizontalAlignmentChanged(HorizontalAlignment);
        void verticalAlignmentChanged(VerticalAlignment);
		void coeffChanged(float);
		void originalHeightChanged(int);
		void originalWidthChanged(int);
		void contentRectChanged();
		void errorStringChanged();

	public slots:
		void rawImageReceived(buzzer::ImageSharedPtr image);

    protected slots:
		void onImageQxLoaded (const QString &source, buzzer::ImageWeakPtr image);
		void onImageQxError  (const QString &source, buzzer::ImageWeakPtr image, const QString &reason);

    protected:
        struct Coefficients {
            float x,y,w,h;
            float tx,ty,tw,th;
        };

        Coefficients coefficientsPad() const;
        Coefficients coefficientsStretch() const;
        Coefficients coefficientsPreserveAspectFit() const;
        Coefficients coefficientsPreserveAspectCrop() const;
        Coefficients coefficientsParallax() const;

    protected:
        bool _completed = false;
        int _vertexAtCorner = 20;
        int _segmentCount = 4*_vertexAtCorner+3;
		float _coeff = 1.0;
		int _originalWidth = 0;
		int _originalHeight = 0;

		ImageQxNode *_node;
		ImageSharedPtr _image;
        bool _imageUpdated = false;

        QString _source;
		float  _radius = 0.0;
        Status _status = Null;
        bool _asynchronous = true;
		bool _autotransform = false;
        bool _mipmap = true;
		QString _errorString;
        FillMode _fillMode = PreserveAspectCrop;
        HorizontalAlignment _horizontalAlignment = AlignHCenter;
        VerticalAlignment _verticalAlignment = AlignVCenter;
};
} // namespace sp {
