#ifndef APPLICATION_H
#define APPLICATION_H

#include <QGuiApplication>
#include <QtWidgets/QApplication>
#include <QQmlApplicationEngine>

#include <QDir>
#include <QPluginLoader>
#include <QQmlExtensionPlugin>
#include <QDebug>

#include <QQuickView>
#include <QQmlContext>
#include <QSettings>
#include <QQuickStyle>

#include <QQuickItem>
#include <QClipboard>

#include <QDateTime>

#include <QNetworkAccessManager>

#ifdef Q_OS_ANDROID
#include <QAndroidService>
#include <QAndroidJniObject>
#include <QAndroidIntent>
#include <QAndroidBinder>
#include <QtAndroid>
#endif

#if defined(DESKTOP_PLATFORM)
#include <QQuickWindow>
#endif

#include "json.h"
#include "client.h"
#include "iapplication.h"
#include "applicationpath.h"
#include "cameracontroler.h"

#ifdef Q_OS_IOS
#include "ios/localnotificator.h"
#endif

//
// Android Icon generator
// https://romannurik.github.io/AndroidAssetStudio/icons-notification.html#source.type=image&source.space.trim=1&source.space.pad=0

namespace buzzer {

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
const char APP_NAME[] = { "buzzer-app" };
#else
const char APP_NAME[] = { "buzzer-desktop-app" };
#endif

#include <QKeyEvent>
class KeyEmitter : public QObject
{
	Q_OBJECT
public:
	KeyEmitter(QObject* parent=nullptr) : QObject(parent) {}
	Q_INVOKABLE void keyPressed(QObject* tf, Qt::Key k) {
		QKeyEvent keyPressEvent = QKeyEvent(QEvent::Type::KeyPress, k, Qt::NoModifier, QKeySequence(k).toString());
		QCoreApplication::sendEvent(tf, &keyPressEvent);
	}
};

class Helper : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE QString fileNameFromPath(const QString& filePath) const
    {
        return QFileInfo(filePath).fileName();
    }

    Q_INVOKABLE QString addReal(qreal left, qreal right)
    {
        return QString::number(left + right);
    }

    Q_INVOKABLE QString minusReal(qreal left, qreal right)
    {
        return QString::number(left - right);
    }

    Q_INVOKABLE QString formatNumber(qreal number, QString format, int digits)
    {
        return QString().setNum((double)number, format.toStdString().c_str()[0], digits);
    }
};

class ClipboardAdapter : public QObject
{
    Q_OBJECT
public:
	explicit ClipboardAdapter(QObject *parent = nullptr);

    Q_INVOKABLE void setText(QString text)
    {
        clipboard_->setText(text, QClipboard::Clipboard);
        clipboard_->setText(text, QClipboard::Selection);
    }

    Q_INVOKABLE QString getText()
    {
        return clipboard_->text();
    }

private:
    QClipboard* clipboard_;
};

#if defined(DESKTOP_PLATFORM)
class BuzzerWindow : public QQuickWindow {
	Q_OBJECT
public:
	BuzzerWindow() {}
	BuzzerWindow(QWindow *parent) : QQuickWindow(parent) {
	}
};
#endif

/**
 * @brief The Application class
 * Entry point to the buzzer.app
 */
class Application : public QQuickItem, public IApplication
{
    Q_OBJECT

    Q_PROPERTY(QString profile READ profile)
    Q_PROPERTY(QString assetsPath READ assetsPath)
	Q_PROPERTY(QStringList picturesLocation READ picturesLocation)
	Q_PROPERTY(QString filesLocation READ filesLocation)
	Q_PROPERTY(bool isDesktop READ isDesktop NOTIFY isDesktopChanged)

public:
    Application(QApplication& app) : app_(app)
    {
		profile_ = "local";
        connectionState_ = "offline"; isWakeLocked_ = false;
        serviceRunning_ = false; fingertipAuthState_ = 0;
#ifdef Q_OS_IOS
        localNotificator_ = nullptr;
#endif
    }

    int load();
    int execute();

    QString profile() const { return profile_; }
    QString assetsPath() const
    {
        QString lAssetsPath = ApplicationPath::assetUrlPath() + ApplicationPath::applicationUrlPath() + "/" + profile_;
        return lAssetsPath;
    }

    QStringList picturesLocation() const
    {
#ifdef Q_OS_ANDROID
        QStringList lList;

		QAndroidJniObject lMediaDir = QAndroidJniObject::callStaticObjectMethod("android/os/Environment", "getExternalStorageDirectory", "()Ljava/io/File;");
		QAndroidJniObject lMediaPath = lMediaDir.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" );
		QString lPicturesPath0 = lMediaPath.toString()+"/DCIM/Camera";
		QString lPicturesPath1 = lMediaPath.toString()+"/DCIM";

		if (QDir(lPicturesPath0).exists()) lList << "file://" + lPicturesPath0;
		if (QDir(lPicturesPath1).exists()) lList << "file://" + lPicturesPath1;

        lList << "file://" + QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
              << "file://" + QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);

        return lList;
#else
        QStringList lList;
        lList << "file:///" + QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
              << "file:///" + QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        return lList;
#endif
	}

	QString filesLocation() const
	{
#ifdef Q_OS_ANDROID
		auto lContext = QtAndroid::androidContext();
		QAndroidJniObject lDir = QAndroidJniObject::fromString(QString(""));
		QAndroidJniObject lPath = lContext.callObjectMethod("getExternalFilesDir",
																 "(Ljava/lang/String;)Ljava/io/File;", lDir.object());

		QAndroidJniObject lMediaPath = lPath.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" );

		return QString("file://") + lMediaPath.toString();
#endif
	}

	bool isDesktop() {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
		return false;
#else
		return true;
#endif
	}

    QQmlApplicationEngine* getEngine() { return &engine_; }
	IClient* getClient() { return &client_; }
    QNetworkAccessManager* getNetworkManager() { return networkManager_; }

    Q_INVOKABLE QString getDeviceInfo();

    Q_INVOKABLE void makeNetworkAccesible() { networkManager_->setNetworkAccessible(QNetworkAccessManager::Accessible); }

    Q_INVOKABLE void lockPortraitOrientation();
    Q_INVOKABLE void lockLandscapeOrientation();
	Q_INVOKABLE void unlockOrientation();

    Q_INVOKABLE QString getColor(QString theme, QString selector, QString key);
    Q_INVOKABLE QString getLocalization(QString locale, QString key);

    Q_INVOKABLE bool hasLightOnly(QString theme);
    Q_INVOKABLE bool hasDarkOnly(QString theme);

    Q_INVOKABLE QString getEndPoint();
    Q_INVOKABLE QString getRealtimeHost();
    Q_INVOKABLE int getRealtimePort();
    Q_INVOKABLE bool getRealtimeEncrypted();
    Q_INVOKABLE bool getRealtimeDebug();
	Q_INVOKABLE QString getExploreTx();
	Q_INVOKABLE QString getExploreTxRaw();

    Q_INVOKABLE bool getDebug();
    Q_INVOKABLE bool getNetworkDebug();

	Q_INVOKABLE QString qttAsset() {
		return QString::fromStdString(getQttAsset());
	}

    Q_INVOKABLE QString getLanguages();
    Q_INVOKABLE QString getColorSchemes();

    Q_INVOKABLE void wakeLock();
    Q_INVOKABLE void wakeRelease();

    Q_INVOKABLE QString getVersion();

    Q_INVOKABLE void setConnectionState(QString);
    Q_INVOKABLE QString getConnectionState() { return connectionState_; }

    Q_INVOKABLE void stopNotificator();
    Q_INVOKABLE void startNotificator();

	Q_INVOKABLE void pauseNotifications();
	Q_INVOKABLE void resumeNotifications();

	Q_INVOKABLE void startFingerprintAuth();
    Q_INVOKABLE void stopFingerprintAuth();
	Q_INVOKABLE bool isFingerprintAuthAvailable();
    Q_INVOKABLE void enablePinStore(QString);

    Q_INVOKABLE bool isFingerprintAccessConfigured();
	Q_INVOKABLE bool isReadStoragePermissionGranted();
	Q_INVOKABLE bool isWriteStoragePermissionGranted();

	Q_INVOKABLE void setBackgroundColor(QString);

	Q_INVOKABLE void suspendClient();
	Q_INVOKABLE void resumeClient();

	Q_INVOKABLE void pickImageFromGallery();

	Q_INVOKABLE void commitCurrentInput();

	std::string getLogCategories();
	bool getTestNet();
	std::string getPeers();
	std::string getQttAsset();

    void emit_fingertipAuthSuccessed(QString);
    void emit_fingertipAuthFailed();
	void emit_fileSelected(QString);

public slots:
    void appQuit();
    void appStateChanged(Qt::ApplicationState);
    void deviceTokenChanged();

signals:
    void appSuspending();
    void appRunning();
    void appConnectionState(QString state);
    void fingertipAuthSuccessed(QString);
    void fingertipAuthFailed();
    void deviceTokenUpdated(QString token);
	void fileSelected(QString file);
	void isDesktopChanged();

private:
    void setAndroidOrientation(int);
    void setWakeLock(int);

private:
    QApplication& app_;
    QQmlApplicationEngine engine_;
	KeyEmitter keyEmitter_;

    QString style_;
    QString profile_;
	qbit::json::Document appConfig_;
    QString connectionState_;

	Client client_;
    Helper helper_;

    CameraControler cameraController_;

    QNetworkAccessManager* networkManager_;
    ClipboardAdapter* clipboard_;

	QTimer* timer_;

    // wake lock
#ifdef Q_OS_ANDROID
    QAndroidJniObject wakeLock_;
#endif

    bool isWakeLocked_;

#ifdef Q_OS_IOS
    LocalNotificator* localNotificator_;
#endif

    bool serviceRunning_;
    int fingertipAuthState_;
};

} // buzzer

#endif // APPLICATION_H
