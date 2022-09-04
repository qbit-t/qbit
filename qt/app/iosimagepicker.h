#ifndef QIIMAGEPICKER_H
#define QIIMAGEPICKER_H

#include <QQuickItem>
#include <QImage>

/// QIImagePicker provides a simple interface to access camera and camera roll via the UIImagePickerController
class QIImagePicker : public QQuickItem
{
    Q_OBJECT
    Q_ENUMS(SourceType)
    Q_ENUMS(Status)
    Q_ENUMS(MediaTypes)
    Q_PROPERTY(SourceType sourceType READ sourceType WRITE setSourceType NOTIFY sourceTypeChanged)
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged)
    Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(bool busy READ busy WRITE setBusy NOTIFY busyChanged)
    Q_PROPERTY(QString mediaType READ mediaType WRITE setMediaType NOTIFY mediaTypeChanged)
    Q_PROPERTY(MediaTypes mediaTypes READ mediaTypes WRITE setMediaTypes NOTIFY mediaTypesChanged)
    Q_PROPERTY(QString mediaUrl READ mediaUrl WRITE setMediaUrl NOTIFY mediaUrlChanged)
    Q_PROPERTY(QString absoluteUrl READ absoluteUrl NOTIFY absoluteUrlChanged)
    Q_PROPERTY(QString referenceUrl READ referenceUrl WRITE setReferenceUrl NOTIFY referenceUrlChanged)

public:
    enum SourceType {
        PhotoLibrary,
        Camera,
        SavedPhotosAlbum
    };

    enum MediaTypes {
        NoMedia = 0,
        Images = 1,
        ImagesAndVideo = 2
    };

    enum Status {
        Null, // Nothing loaded
        Running, // The view controller is running
        Ready, // The image is ready
        Saving // The image is saving
    };

    QIImagePicker(QQuickItem* parent = 0);
    ~QIImagePicker();

    Q_INVOKABLE void show(bool animated = true);

    Q_INVOKABLE void close(bool animated = true);

    Q_INVOKABLE void save(QString fileName);

    /// Save the stored image to tmp file.
    Q_INVOKABLE void saveAsTemp();

    Q_INVOKABLE void clear();

    SourceType sourceType() const;
    void setSourceType(const SourceType &sourceType);

    QImage image() const;
    void setImage(const QImage &image);

    Status status() const;
    void setStatus(const Status &status);

    bool busy() const;
    void setBusy(bool busy);

    QString mediaType() const;
    void setMediaType(const QString &mediaType);

    MediaTypes mediaTypes() const;
    void setMediaTypes(const MediaTypes &mediaTypes);

    QString mediaUrl() const;
    void setMediaUrl(const QString &mediaUrl);

    QString referenceUrl() const;
    void setReferenceUrl(const QString &referenceUrl);

    QString absoluteUrl() const;

signals:
    void sourceTypeChanged();
    void imageChanged();
    void statusChanged();
    void busyChanged();
    void referenceUrlChanged();
    void mediaTypeChanged();
    void mediaTypesChanged();
    void mediaUrlChanged();
    void absoluteUrlChanged(QString url);

    void ready();
    void saved(QString url, QString urlPreview, int orientation);

private:
    Q_INVOKABLE void onReceived(QString name, QVariantMap data);
    Q_INVOKABLE void endSave(QString fileName, QString preview, int orientation);

    SourceType m_sourceType;
    QImage m_image;
    Status m_status;

    QString m_mediaType;
    MediaTypes m_mediaTypes;
    QString m_mediaUrl;
    QString m_referenceUrl;
    QString m_absoluteUrl;
    int m_orientation;

    bool m_busy;
};

#endif // QIIMAGEPICKER_H
