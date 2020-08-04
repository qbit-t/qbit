
#include <QFile>
#include <QScreen>

#include "application.h"
#include "error.h"
#include "line.h"
#include "statusbar/statusbar.h"
#include "httprequest.h"

using namespace buzzer;
IApplication* buzzer::gApplication = nullptr;

ClipboardAdapter::ClipboardAdapter(QObject *parent) : QObject(parent)
{
    clipboard_ = QGuiApplication::clipboard();
}

int Application::load()
{
    try
    {
        QString lAppConfig = ApplicationPath::applicationDirPath() + "/" + APP_NAME + ".config";
        qInfo() << "Loading app-config:" << lAppConfig;

        if (QFile(lAppConfig).exists()) appConfig_.loadFromFile(lAppConfig.toStdString());
        else
        {
            qCritical() << "File is missing" << lAppConfig << "Aborting...";
            return -2;
        }

		style_ = QString::fromStdString(appConfig_.operator []("style").getString());
    }
	catch(buzzer::Exception const& ex)
    {
        qCritical() << ex.message().c_str();
        return -1;
    }

    return appConfig_.hasErrors() ? -1 : 1;
}

void Application::appQuit()
{
    qInfo() << "Application quit. Bye!";
    emit appSuspending();
}

void Application::appStateChanged(Qt::ApplicationState state)
{
    if (state == Qt::ApplicationState::ApplicationSuspended || state == Qt::ApplicationState::ApplicationHidden)
    {
        emit appSuspending();
    }
    else if (state == Qt::ApplicationState::ApplicationActive)
    {
        emit appRunning();
    }
}

void Application::deviceTokenChanged()
{
#ifdef Q_OS_IOS
	if (account_.getProperty("Client.deviceId") == "")
    {
        emit deviceTokenUpdated(localNotificator_->getDeviceToken());
    }
#endif
}

int Application::execute()
{
    qInfo() << "========================BUZZER========================";
    qInfo() << "Configuring app:" << APP_NAME << getVersion();

    QQuickStyle::setStyle(style_); // default style

	qmlRegisterType<buzzer::Client>("app.buzzer.client", 1, 0, "Client");
    qmlRegisterType<buzzer::ClipboardAdapter>("app.buzzer.helpers", 1, 0, "Clipboard");
    qmlRegisterType<StatusBar>("StatusBar", 0, 1, "StatusBar");
    buzzer::QuarkLine::declare();

    networkManager_ = new QNetworkAccessManager(this);
    clipboard_ = new ClipboardAdapter(this);

    engine_.rootContext()->setContextProperty("buzzerApp", this);
	engine_.rootContext()->setContextProperty("buzzerClient", &client_);
    engine_.rootContext()->setContextProperty("appHelper", &helper_);
    engine_.rootContext()->setContextProperty("cameraController", &cameraController_);
    engine_.rootContext()->setContextProperty("clipboard", clipboard_);
    engine_.rootContext()->setContextProperty("localNotificator", nullptr);

#ifdef Q_OS_IOS
    localNotificator_ = LocalNotificator::instance();
    connect(localNotificator_, SIGNAL(deviceTokenChanged()), this, SLOT(deviceTokenChanged()));
    engine_.rootContext()->setContextProperty("localNotificator", localNotificator_);
#endif

    connect(&app_, SIGNAL(aboutToQuit()), this, SLOT(appQuit()));

	// set application
	client_.setApplication(this);

	// open settings
	client_.openSettings();

	// TODO: this _must_ be postponed until: pin entered or just UI transited into first screen
	// open client - fow now
	client_.open("");

    // check background mode
	if (client_.getProperty("Client.runInBackground") == "true")
    {
        qInfo() << "Application run in background mode";
    }

    // wake lock
	if (client_.getProperty("Client.wakeLock") == "true")
    {
        wakeLock();
    }

	if (client_.getProperty("Client.runService") == "true")
    {
        startNotificator();
    }

    qInfo() << "Loading main qml:" <<  QString("qrc:/qml/") + APP_NAME + ".qml";
    engine_.load(QString("qrc:/qml/") + APP_NAME + ".qml");
    if (engine_.rootObjects().isEmpty())
    {
        qCritical() << "Root object is empty. Exiting...";
        return -1;
    }

    qInfo() << "Executing app:" << APP_NAME;
    return app_.exec();
}

void Application::suspendClient() {

}

void Application::resumeClient() {

}

void Application::stopNotificator()
{
    qInfo() << "Stopping notificator service...";

#ifdef Q_OS_ANDROID
        QAndroidJniObject lActivity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
        QAndroidJniObject::callStaticMethod<void>("app/buzzer/mobile/NotificatorService",
                                                      "stopNotificatorService",
                                                      "(Landroid/content/Context;)V",
                                                      lActivity.object());
#endif
}

void Application::startNotificator()
{
    qInfo() << "Starting notificator service...";

#ifdef Q_OS_ANDROID
        QAndroidJniObject lActivity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
        QAndroidJniObject::callStaticMethod<void>("app/buzzer/mobile/NotificatorService",
                                                      "startNotificatorService",
                                                      "(Landroid/content/Context;)V",
                                                      lActivity.object());
#endif
}

void Application::startFingerprintAuth()
{
#ifdef Q_OS_ANDROID
    if (fingertipAuthState_ == 1)
    {
        qInfo() << "Begin fingertip authentication...";
        QAndroidJniObject::callStaticMethod<void>("app/buzzer/mobile/MainActivity",
                                                  "externalStartFingertipAuth",
                                                  "()V");
    }
#endif
}

void Application::stopFingerprintAuth()
{
#ifdef Q_OS_ANDROID
    if (fingertipAuthState_ == 1)
    {
        qInfo() << "End fingertip authentication...";
        QAndroidJniObject::callStaticMethod<void>("app/buzzer/mobile/MainActivity",
                                                  "externalStopFingertipAuth",
                                                  "()V");
    }
#endif
}

void Application::enablePinStore(QString pin)
{
#ifdef Q_OS_ANDROID
    if (fingertipAuthState_ == 1)
    {
        qInfo() << "Enabling pin storage...";
        QAndroidJniObject lPin = QAndroidJniObject::fromString(pin);
        QAndroidJniObject::callStaticMethod<void>("app/buzzer/mobile/MainActivity",
                                                  "externalEnablePinStore",
                                                  "(Ljava/lang/String;)V",
                                                  lPin.object<jstring>());
    }
#endif
}

bool Application::isFingerprintAuthAvailable()
{
#ifdef Q_OS_ANDROID
    fingertipAuthState_ = QAndroidJniObject::callStaticMethod<int>("app/buzzer/mobile/MainActivity",
                                                  "externalGetFingertipAuthState",
                                                  "()I");

    qInfo() << "Fingertip auth state =" << fingertipAuthState_;

    if (fingertipAuthState_ == 1) return true;
    return false;
#endif
    return false;
}

void Application::setBackgroundColor(QString color)
{
#ifdef Q_OS_ANDROID
		qInfo() << "Setting background color" << color;
		QAndroidJniObject lPin = QAndroidJniObject::fromString(color);
		QAndroidJniObject::callStaticMethod<void>("app/buzzer/mobile/MainActivity",
												  "externalSetBackgroundColor",
												  "(Ljava/lang/String;)V",
												  lPin.object<jstring>());
#endif
}

bool Application::isReadStoragePermissionGranted()
{
#ifdef Q_OS_ANDROID
	int lResult = QAndroidJniObject::callStaticMethod<int>("app/buzzer/mobile/MainActivity",
												  "isReadStoragePermissionGranted",
												  "()I");

	qInfo() << "Read storage =" << lResult;

	if (lResult == 1) return true;
	return false;
#endif
	return false;
}

void Application::pickImageFromGallery() {
	//
#ifdef Q_OS_ANDROID
	QAndroidJniObject::callStaticMethod<void>("app/buzzer/mobile/MainActivity",
												  "externalPickImage",
												  "()V");
#endif
}

bool Application::isWriteStoragePermissionGranted()
{
#ifdef Q_OS_ANDROID
	int lResult = QAndroidJniObject::callStaticMethod<int>("app/buzzer/mobile/MainActivity",
												  "isWriteStoragePermissionGranted",
												  "()I");

	qInfo() << "Write storage =" << lResult;

	if (lResult == 1) return true;
	return false;
#endif
	return false;
}

bool Application::isFingerprintAccessConfigured()
{
	return client_.isFingerprintAccessConfigured();
}

bool Application::getDebug()
{
	qbit::json::Value lValue = appConfig_["debug"];
    return lValue.getBool();
}

bool Application::getNetworkDebug()
{
	qbit::json::Value lValue = appConfig_["networkDebug"];
    return lValue.getBool();
}

std::string Application::getLogCategories()
{
	qbit::json::Value lValue;
	if (appConfig_.find("logCategories", lValue)) {
		return lValue.getString();
	}

#ifdef QT_DEBUG
	return "all";
#else
	return "error,warn";
#endif
}

QString Application::getColor(QString theme, QString selector, QString key)
{
	qbit::json::Value lThemes = appConfig_["themes"];
	qbit::json::Value lTheme = lThemes[theme.toStdString()];
	qbit::json::Value lSelector = lTheme[selector.toStdString()];
	qbit::json::Value lValue = lSelector[key.toStdString()];

	return QString::fromStdString(lValue.getString());
}

bool Application::hasLightOnly(QString theme)
{
	qbit::json::Value lThemes = appConfig_["themes"];
	qbit::json::Value lTheme = lThemes[theme.toStdString()];

	qbit::json::Value lValue;
	if (!lTheme.find("dark", lValue) && lTheme.find("light", lValue))
        return true;
    return false;
}

bool Application::hasDarkOnly(QString theme)
{
	qbit::json::Value lThemes = appConfig_["themes"];
	qbit::json::Value lTheme = lThemes[theme.toStdString()];

	qbit::json::Value lValue;
	if (lTheme.find("dark", lValue) && !lTheme.find("light", lValue))
        return true;
    return false;
}

QString Application::getLocalization(QString locale, QString key)
{
	qbit::json::Value lLocalization = appConfig_["localization"];
	qbit::json::Value lLocale = lLocalization[locale.toStdString()];
	qbit::json::Value lValue = lLocale[key.toStdString()];
	return QString::fromStdString(lValue.getString());
}

QString Application::getRealtimeHost()
{
	qbit::json::Value lRealtime = appConfig_["realtimeData"];
	qbit::json::Value lValue = lRealtime["wsHost"];
	return QString::fromStdString(lValue.getString());
}

int Application::getRealtimePort()
{
	qbit::json::Value lRealtime = appConfig_["realtimeData"];
	qbit::json::Value lValue = lRealtime["wsPort"];
    return lValue.getInt();
}

bool Application::getRealtimeEncrypted()
{
	qbit::json::Value lRealtime = appConfig_["realtimeData"];
	qbit::json::Value lValue = lRealtime["encrypted"];
    return lValue.getBool();
}

bool Application::getRealtimeDebug()
{
	qbit::json::Value lRealtime = appConfig_["realtimeData"];
	qbit::json::Value lValue = lRealtime["debug"];
    return lValue.getBool();
}

QString Application::getEndPoint()
{
	qbit::json::Value lValue = appConfig_["endPoint"];
	return QString::fromStdString(lValue.getString());
}

void Application::setAndroidOrientation(int orientation)
{
#ifdef Q_OS_ANDROID
    QAndroidJniObject lActivity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
    if (lActivity.isValid())
    {
        lActivity.callMethod<void>
                ("setRequestedOrientation" // method name
                 , "(I)V" // signature
                 , orientation);
    }
#endif
}

#define ACQUIRE_WAKE_LOCK 0
#define RELEASE_WAKE_LOCK 1

void Application::setWakeLock(int lock)
{
#ifdef Q_OS_ANDROID
    if (!wakeLock_.isValid())
    {
        QAndroidJniObject lActivity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
        if (lActivity.isValid())
        {
            QAndroidJniObject lServiceName = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Context","POWER_SERVICE");
            if (lServiceName.isValid())
            {
                QAndroidJniObject lPowerMgr = lActivity.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;",lServiceName.object<jobject>());
                if (lPowerMgr.isValid())
                {
                    jint lLevelAndFlags = QAndroidJniObject::getStaticField<jint>("android/os/PowerManager","SCREEN_DIM_WAKE_LOCK");

                    QAndroidJniObject lTag = QAndroidJniObject::fromString("GRAVIEX-TAG");

                    wakeLock_ = lPowerMgr.callObjectMethod("newWakeLock", "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;", lLevelAndFlags, lTag.object<jstring>());
                }
            }
        }
    }

    if (wakeLock_.isValid())
    {
        //jboolean lHeld = wakeLock_.callMethod<jboolean>("isHeld", "()V");
        if (lock == ACQUIRE_WAKE_LOCK && !isWakeLocked_)
        {
            qDebug() << "[Application::setWakeLock]: acquire lock";
            wakeLock_.callMethod<void>("acquire", "()V");
            isWakeLocked_ = true;
        }
        else if (lock == RELEASE_WAKE_LOCK && isWakeLocked_)
        {
            qDebug() << "[Application::setWakeLock]: release lock";
            wakeLock_.callMethod<void>("release", "()V");
            isWakeLocked_ = false;
        }
    }
    else
        qDebug() << "[Application::setWakeLock]: lock is not valid.";

#endif
}

void Application::wakeLock()
{
    setWakeLock(ACQUIRE_WAKE_LOCK);
}

void Application::wakeRelease()
{
    setWakeLock(RELEASE_WAKE_LOCK);
}

void Application::lockPortraitOrientation()
{
#ifdef Q_OS_ANDROID
    setAndroidOrientation(1);
#endif

#ifdef Q_OS_IOS
    // [[UIDevice currentDevice] performSelector:NSSelectorFromString(@"setOrientation:") withObject:(__bridge id)((void*)UIInterfaceOrientationPortrait)];
#endif
}

void Application::lockLandscapeOrientation()
{
#ifdef Q_OS_ANDROID
    setAndroidOrientation(0);
#endif

#ifdef Q_OS_IOS
    // [[UIDevice currentDevice] performSelector:NSSelectorFromString(@"setOrientation:") withObject:(__bridge id)((void*)UIInterfaceOrientationLandscape)];
#endif
}

void Application::unlockOrientation()
{
#ifdef Q_OS_ANDROID
	setAndroidOrientation(-1);
#endif

#ifdef Q_OS_IOS
#endif
}

QString Application::getLanguages()
{
    QString lResult;
	qbit::json::Value lList = appConfig_["localization"];
	for(qbit::json::ValueMemberIterator lIter = lList.begin(); lIter != lList.end(); lIter++)
    {
		qbit::json::ValueMemberIterator lSymbol = lIter.getValue().begin();
		QString lItem = QString::fromStdString(lIter.getName());
		QString lSym = QString::fromStdString(lSymbol.getValue().getString());
		lResult += lSym + "," + lItem + "|";
    }

    return lResult;
}

QString Application::getColorSchemes()
{
    QString lResult;
	qbit::json::Value lList = appConfig_["themes"];
	for(qbit::json::ValueMemberIterator lIter = lList.begin(); lIter != lList.end(); lIter++)
    {
		QString lItem = QString::fromStdString(lIter.getName());
        lResult += lItem + "|";
    }

    return lResult;
}

QString Application::getVersion()
{
    return QString(VERSION_STRING);
}

void Application::setConnectionState(QString state)
{
    connectionState_ = state;
    emit appConnectionState(state);
}

QString Application::getDeviceInfo()
{
    QString manufacturer = "";
    QString model = "";

#ifdef Q_OS_ANDROID
    manufacturer = QAndroidJniObject::getStaticObjectField<jstring>("android/os/Build", "MANUFACTURER").toString();
    model = QAndroidJniObject::getStaticObjectField<jstring>("android/os/Build", "MODEL").toString();
#endif

    return manufacturer + " - " + model;
}

void Application::emit_fingertipAuthSuccessed(QString key)
{
    emit fingertipAuthSuccessed(key);
}

void Application::emit_fingertipAuthFailed()
{
    emit fingertipAuthFailed();
}

void Application::emit_fileSelected(QString key)
{
	emit fileSelected(key);
}

//
//
//
#ifdef Q_OS_ANDROID

#include <jni.h>

#ifdef __cplusplus
extern "C"
{
#endif

JNIEXPORT void JNICALL Java_app_buzzer_mobile_FingerprintHandler_authenticationFailed(JNIEnv*, jobject)
{
    qDebug() << "[JAVA::authenticationFailed]: auth failed";
	((Application*)buzzer::gApplication)->emit_fingertipAuthFailed();
}

JNIEXPORT void JNICALL Java_app_buzzer_mobile_FingerprintHandler_authenticationSucceeded(JNIEnv* env, jobject, jstring key)
{
    QString lKeyFound = "none";
    const char* lKey = env->GetStringUTFChars(key, NULL);
    if (lKey) {
        lKeyFound = QString::fromLocal8Bit(lKey);
        env->ReleaseStringUTFChars(key, lKey);  // release resource
    }

    qDebug() << "[JAVA::authenticationSucceeded]: auth succeeded, key = " << lKeyFound;
	((Application*)buzzer::gApplication)->emit_fingertipAuthSuccessed(lKeyFound);
}

JNIEXPORT void JNICALL Java_app_buzzer_mobile_MainActivity_fileSelected(JNIEnv* env, jobject, jstring file)
{
	QString lFileFound = "none";
	const char* lFile = env->GetStringUTFChars(file, NULL);
	if (lFile) {
		lFileFound = QString::fromLocal8Bit(lFile);
		env->ReleaseStringUTFChars(file, lFile);  // release resource
	}

	qDebug() << "[JAVA::fileSelected]: file = " << lFileFound;
	((Application*)buzzer::gApplication)->emit_fileSelected(lFileFound);
}

#ifdef __cplusplus
}
#endif

#endif
