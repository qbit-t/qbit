
#include <QFile>
#include <QScreen>

#include "application.h"
#include "error.h"
#include "line.h"
#include "roundframe.h"
#include "statusbar/statusbar.h"
#include "httprequest.h"
#include "imageqx.h"
#include "videosurface.h"
#include "videoframesprovider.h"

#ifdef Q_OS_IOS
#include "iossystemdispatcher.h"
#endif

using namespace buzzer;
IApplication* buzzer::gApplication = nullptr;

ClipboardAdapter::ClipboardAdapter(QObject *parent) : QObject(parent)
{
    clipboard_ = QGuiApplication::clipboard();
	//connect(clipboard_, SIGNAL(dataChanged()), this, SLOT(dataChanged()));
}

void ClipboardAdapter::dataChanged()
{
	/*
	QString lText = clipboard_->text();

	 if (clipboard_->mimeData()->hasHtml()) {
		QMimeData* lMimeData = new QMimeData();
		lMimeData->setText(lText);
		clipboard_->setMimeData(lMimeData);
	}
	*/
}

int Application::load()
{
#if defined(DESKTOP_PLATFORM)
	try
	{
		qInfo() << "Loading app-config:" << "qrc:/buzzer-app.config";

		QFile lRawFile(":/buzzer-app.config");
		lRawFile.open(QIODevice::ReadOnly | QIODevice::Text);

		QByteArray lRawData = lRawFile.readAll();
		std::string lRawStringData = lRawData.toStdString();

		appConfig_.loadFromString(lRawStringData);

		style_ = QString::fromStdString(appConfig_.operator []("style").getString());
	}
	catch(buzzer::Exception const& ex)
	{
		qCritical() << ex.message().c_str();
		return -1;
	}
	return appConfig_.hasErrors() ? -1 : 1;
#else
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
#endif
}

void Application::appQuit()
{
	ImageQxLoader::stop();
    qInfo() << "Application quit. Bye!";
    emit appSuspending();
}

void Application::appStateChanged(Qt::ApplicationState state) {
	if (state == Qt::ApplicationState::ApplicationSuspended /* || state == Qt::ApplicationState::ApplicationHidden ||
			state == Qt::ApplicationState::ApplicationInactive*/)
    {
		qInfo() << "[appStateChanged]: suspended";
		client_.suspend();
        emit appSuspending();
	} else if (state == Qt::ApplicationState::ApplicationActive) {
		qInfo() << "[appStateChanged]: resumed";
		client_.resume();
		emit appRunning();
    }
}

void Application::deviceTokenChanged()
{
#ifdef Q_OS_IOS
#endif
}

#if defined(DESKTOP_PLATFORM)
//
#endif

int Application::execute()
{
    qInfo() << "========================BUZZER========================";
    qInfo() << "Configuring app:" << APP_NAME << getVersion();

    QQuickStyle::setStyle(style_); // default style

#if defined(DESKTOP_PLATFORM)
	qmlRegisterType<buzzer::BuzzerWindow>("app.buzzer", 1, 0, "BuzzerWindow");
#endif
	qmlRegisterType<buzzer::Client>("app.buzzer.client", 1, 0, "Client");
    qmlRegisterType<buzzer::ClipboardAdapter>("app.buzzer.helpers", 1, 0, "Clipboard");
    qmlRegisterType<StatusBar>("StatusBar", 0, 1, "StatusBar");
	//
	qmlRegisterType<buzzer::ImageQx>("app.buzzer.components", 1, 0, "ImageQx");
	qmlRegisterType<buzzer::VideoSurfaces>("app.buzzer.components", 1, 0, "VideoSurfaces");
	qmlRegisterType<buzzer::VideoFramesProvider>("app.buzzer.components", 1, 0, "VideoFramesProvider");

	buzzer::QuarkLine::declare();
	buzzer::QuarkRoundFrame::declare();

    networkManager_ = new QNetworkAccessManager(this);
    clipboard_ = new ClipboardAdapter(this);

    engine_.rootContext()->setContextProperty("buzzerApp", this);
	engine_.rootContext()->setContextProperty("buzzerClient", &client_);
	engine_.rootContext()->setContextProperty("shareUtils", shareUtils_);
	engine_.rootContext()->setContextProperty("appHelper", &helper_);
    engine_.rootContext()->setContextProperty("cameraController", &cameraController_);
    engine_.rootContext()->setContextProperty("clipboard", clipboard_);
	engine_.rootContext()->setContextProperty("localNotificator", nullptr);
	engine_.rootContext()->setContextProperty("keyEmitter", &keyEmitter_);


#ifdef Q_OS_IOS
//    localNotificator_ = LocalNotificator::instance();
//    connect(localNotificator_, SIGNAL(deviceTokenChanged()), this, SLOT(deviceTokenChanged()));
//    engine_.rootContext()->setContextProperty("localNotificator", localNotificator_);
#endif

    connect(&app_, SIGNAL(aboutToQuit()), this, SLOT(appQuit()));
	connect(&app_, SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(appStateChanged(Qt::ApplicationState)));

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

#if defined(DESKTOP_PLATFORM)
	QQuickWindow::setDefaultAlphaBuffer(true);
	// NOTICE: software rendeder lacks some functionality
	// QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGL);
#endif

	QString lAppName = APP_NAME;
	if (isTablet()) lAppName = APP_TABLET_NAME;
	//
	qInfo() << "Loading main qml:" <<  QString("qrc:/qml/") + lAppName + ".qml";
	view_ = nullptr;
	engine_.load(QString("qrc:/qml/") + lAppName + ".qml");

	if (engine_.rootObjects().isEmpty()) {
		qCritical() << "Root object is empty. Exiting...";
		return -1;
	}

	QObject* lAppWindow = *(engine_.rootObjects().begin());
	view_ = qobject_cast<QQuickWindow*>(lAppWindow);

#ifdef Q_OS_IOS
	QISystemDispatcher* lSystem = QISystemDispatcher::instance();
	connect(lSystem, SIGNAL(dispatched(QString,QVariantMap)), this, SLOT(externalKeyboardHeightChanged(QString, QVariantMap)));
#endif

#ifdef Q_OS_MACX
	// TODO: make external settings
	setStatusBarColor(getColor(client_.theme(), client_.themeSelector(), "Material.statusBar"));
#endif

	qInfo() << "Executing app:" << lAppName;
	return app_.exec();
}

void Application::externalKeyboardHeightChanged(QString name, QVariantMap data) {
	//
	if (name != "keyboardHeightChanged") return;

	int lHeight = data["keyboardHeight"].value<int>();
	if (!buzzer::gApplication) return;
	qDebug() << "[Objective-C::keyboardHeightChanged]: height =" << lHeight;
	((Application*)buzzer::gApplication)->emit_keyboardHeightChanged(lHeight);	
}

bool Application::isTablet() {
	//
	QRect lRect = QGuiApplication::primaryScreen()->geometry();
	qreal lDensity = QGuiApplication::primaryScreen()->physicalDotsPerInch();

	qreal lWidth = lRect.width() / lDensity;
	qreal lHeight = lRect.height() / lDensity;
	return sqrt((lWidth * lWidth) + (lHeight * lHeight)) > 7.0; // If diagonal > 7" it's a tablet
}

bool Application::isPortrait() {
	//
	QRect lRect = QGuiApplication::primaryScreen()->geometry();
	return lRect.height() > lRect.width();
}

qreal Application::deviceWidth() {
	//
	QRect lRect = QGuiApplication::primaryScreen()->geometry();
	return lRect.width();
}

qreal Application::deviceHeight() {
	//
	QRect lRect = QGuiApplication::primaryScreen()->geometry();
	return lRect.height();
}

void Application::commitCurrentInput() {
	app_.inputMethod()->reset();
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

void Application::pauseNotifications(QString name)
{
	qInfo() << "Pausing notifications...";

#ifdef Q_OS_ANDROID
	std::ofstream lDaemonSettings = std::ofstream(client_.settingsDataPath() + "/buzzerd-settings.dat", std::ios::binary);
	std::string lName = name.toStdString();
	unsigned short lNameSize = lName.size();
	lDaemonSettings.write((char*)&lNameSize, sizeof(unsigned short));
	lDaemonSettings.write((char*)lName.c_str(), lNameSize);
	lDaemonSettings.flush();
	lDaemonSettings.close();

	/*
	QAndroidJniObject lActivity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
	QAndroidJniObject lName = QAndroidJniObject::fromString(name);
	QAndroidJniObject::callStaticMethod<void>("app/buzzer/mobile/NotificatorService",
													"pauseNotifications",
													"(Landroid/content/Context;Ljava/lang/String;)V",
													lActivity.object(),
													lName.object<jstring>());
	*/
#endif
}

void Application::resumeNotifications(QString /*name*/)
{
	qInfo() << "Resuming notifications...";

#ifdef Q_OS_ANDROID
	std::ofstream lDaemonSettings = std::ofstream(client_.settingsDataPath() + "/buzzerd-settings.dat", std::ios::binary);
	std::string lName = "none";
	unsigned short lNameSize = lName.size();
	lDaemonSettings.write((char*)&lNameSize, sizeof(unsigned short));
	lDaemonSettings.write((char*)lName.c_str(), lNameSize);
	lDaemonSettings.flush();
	lDaemonSettings.close();

	/*
	QAndroidJniObject lActivity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
	QAndroidJniObject lName = QAndroidJniObject::fromString(name);
	QAndroidJniObject::callStaticMethod<void>("app/buzzer/mobile/NotificatorService",
													"resumeNotifications",
													"(Landroid/content/Context;Ljava/lang/String;)V",
													lActivity.object(),
													lName.object<jstring>());
	*/
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

bool Application::getLimited()
{
	qbit::json::Value lValue = appConfig_["limited"];
	return lValue.getBool();
}

bool Application::getInterceptOutput()
{
	qbit::json::Value lValue = appConfig_["interceptOutput"];
    return lValue.getBool();
}

std::string Application::getQttAsset() {
	qbit::json::Value lValue = appConfig_["qttAsset"];
	return lValue.getString();
}

uint64_t Application::getQttAssetVoteAmount() {
	qbit::json::Value lValue = appConfig_["qttAssetVoteAmount"];
	return lValue.getInt();
}

int Application::getQttAssetLockTime() {
	qbit::json::Value lValue = appConfig_["qttAssetLockTime"];
	return lValue.getInt();
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

std::string Application::getPeers()
{
	qbit::json::Value lValue;
	if (appConfig_.find("peers", lValue)) {
		return lValue.getString();
	}

	return "";
}

bool Application::getTestNet()
{
	qbit::json::Value lValue = appConfig_["testNet"];
	return lValue.getBool();
}

QString Application::getColor(QString theme, QString selector, QString key)
{
	qbit::json::Value lThemes = appConfig_["themes"];
	qbit::json::Value lTheme = lThemes[theme.toStdString()];
	qbit::json::Value lSelector = lTheme[selector.toStdString()];
	qbit::json::Value lValue = lSelector[key.toStdString()];

	return QString::fromStdString(lValue.getString());
}

QString Application::getColorStatusBar(QString theme, QString selector, QString key)
{
	QString lStatusBarTheme = getColor(theme, selector, "StatusBar.theme");
	qbit::json::Value lThemes = appConfig_["themes"];
	qbit::json::Value lTheme = lThemes[theme.toStdString()];
	qbit::json::Value lSelector = lTheme[selector.toStdString()];
	qbit::json::Value lValue = lSelector[(key + "." + lStatusBarTheme).toStdString()];

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

QString Application::getExploreTx() {
	qbit::json::Value lValue = appConfig_["exploreTx"];
	return QString::fromStdString(lValue.getString());
}

QString Application::getExploreTxRaw() {
	qbit::json::Value lValue = appConfig_["exploreTxRaw"];
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
					jint lLevelAndFlags = QAndroidJniObject::getStaticField<jint>("android/os/PowerManager","PARTIAL_WAKE_LOCK"); // SCREEN_DIM_WAKE_LOCK / SCREEN_BRIGHT_WAKE_LOCK

					QAndroidJniObject lTag = QAndroidJniObject::fromString("BUZZER-TAG");

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
#ifdef Q_OS_ANDROID
	QtAndroid::runOnAndroidThread([=]() {
		QAndroidJniObject window = QtAndroid::androidActivity().callObjectMethod("getWindow", "()Landroid/view/Window;");
		window.callMethod<void>("addFlags", "(I)V", 0x00000080);
	});
#endif

    // TODO: maybe we need to setup this mode at startup?
#ifdef Q_OS_IOS
    QISystemDispatcher* system = QISystemDispatcher::instance();
    QVariantMap data;
    system->dispatch("makeBackgroundAudioAvailable", data);
    qDebug() << "[Application::setWakeLock]: acquire lock";
    isWakeLocked_ = true;
#endif
}

void Application::wakeRelease()
{
	setWakeLock(RELEASE_WAKE_LOCK);
#ifdef Q_OS_ANDROID
	QtAndroid::runOnAndroidThread([=]() {
		QAndroidJniObject window = QtAndroid::androidActivity().callObjectMethod("getWindow", "()Landroid/view/Window;");
		window.callMethod<void>("clearFlags", "(I)V", 0x00000080);
	});
#endif

#ifdef Q_OS_IOS
    QISystemDispatcher* system = QISystemDispatcher::instance();
    QVariantMap data;
    system->dispatch("releaseBackgroundAudio", data);
    qDebug() << "[Application::setWakeLock]: release lock";
    isWakeLocked_ = false;
#endif
}

void Application::lockPortraitOrientation()
{
#ifdef Q_OS_ANDROID
	if (isTablet()) return;
    setAndroidOrientation(1);
#endif

#ifdef Q_OS_IOS
    // [[UIDevice currentDevice] performSelector:NSSelectorFromString(@"setOrientation:") withObject:(__bridge id)((void*)UIInterfaceOrientationPortrait)];
#endif
}

void Application::lockLandscapeOrientation()
{
#ifdef Q_OS_ANDROID
	if (isTablet()) return;
    setAndroidOrientation(0);
#endif

#ifdef Q_OS_IOS
    // [[UIDevice currentDevice] performSelector:NSSelectorFromString(@"setOrientation:") withObject:(__bridge id)((void*)UIInterfaceOrientationLandscape)];
#endif
}

void Application::unlockOrientation()
{
#ifdef Q_OS_ANDROID
	if (isTablet()) return;
	setAndroidOrientation(-1);
#endif

#ifdef Q_OS_IOS
#endif
}

void Application::setStatusBarColor(QString color) {
	//
#ifdef Q_OS_MACX
	MacXUtils::setStatusBarColor(view_, color);
#endif
}

void Application::setKeyboardAdjustMode(bool adjustNothing)
{
#if defined (Q_OS_ANDROID)
	QtAndroid::runOnAndroidThread([=]() {
		QtAndroid::androidActivity().callMethod<void>("setKeyboardAdjustMode", "(Z)V", adjustNothing);
	});
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

void Application::emit_fileSelected(QString key, QString preview, QString description)
{
	emit fileSelected(key, preview, description);
}

void Application::emit_externalActivityCalled(int type, QString chain, QString tx, QString buzzer)
{
	emit externalActivityCalled(type, chain, tx, buzzer);
}

void Application::emit_keyboardHeightChanged(int height)
{
	emit keyboardHeightChanged(height);
}

void Application::emit_globalGeometryChanged(int width, int height)
{
	emit globalGeometryChanged(width, height);
}

#if defined(Q_OS_ANDROID)
void Application::onApplicationStateChanged(Qt::ApplicationState applicationState)
{
	if(applicationState == Qt::ApplicationState::ApplicationSuspended) {
		// nothing to do
		return;
	}

	if(applicationState == Qt::ApplicationState::ApplicationActive) {
		// if App was launched from VIEW or SEND Intent
		// there's a race collision: the event will be lost,
		// because App and UI wasn't completely initialized
		// workaround: QShareActivity remembers that an Intent is pending
		if(!pendingIntentsChecked_) {
			pendingIntentsChecked_ = true;
			shareUtils_->checkPendingIntents(ApplicationPath::tempFilesDir());
		}
	}
}

bool Application::checkPermission() {
	//
	QtAndroid::PermissionResult r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
	if(r == QtAndroid::PermissionResult::Denied) {
		QtAndroid::requestPermissionsSync(QStringList() << "android.permission.WRITE_EXTERNAL_STORAGE");
		r = QtAndroid::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");
		if(r == QtAndroid::PermissionResult::Denied) {
			qDebug() << "Permission denied";
			emit noDocumentsWorkLocation();
			return false;
		}
   }
   return true;
}

#endif

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

JNIEXPORT void JNICALL Java_app_buzzer_mobile_MainActivity_fileSelected(JNIEnv* env, jobject, jstring file, jstring preview, jstring description)
{
	QString lFileFound = "none";
	QString lPreviewFound = "none";
	QString lDescriptionFound = "none";

	const char* lFile = env->GetStringUTFChars(file, NULL);
	if (lFile) {
		lFileFound = QString::fromLocal8Bit(lFile);
		env->ReleaseStringUTFChars(file, lFile);  // release resource
	}

	const char* lPreview = env->GetStringUTFChars(preview, NULL);
	if (lPreview) {
		lPreviewFound = QString::fromLocal8Bit(lPreview);
		env->ReleaseStringUTFChars(preview, lPreview);  // release resource
	}

	const char* lDescription = env->GetStringUTFChars(description, NULL);
	if (lDescription) {
		lDescriptionFound = QString::fromLocal8Bit(lDescription);
		env->ReleaseStringUTFChars(description, lDescription);  // release resource
	}

	qDebug() << "[JAVA::fileSelected]: file = " << lFileFound << lPreviewFound << lDescriptionFound;
	((Application*)buzzer::gApplication)->emit_fileSelected(lFileFound, lPreviewFound, lDescriptionFound);
}

JNIEXPORT void JNICALL Java_app_buzzer_mobile_MainActivity_keyboardHeightChanged(JNIEnv* /*env*/, jobject, jint height) {
	if (!buzzer::gApplication) return;
	qDebug() << "[JAVA::keyboardHeightChanged]: height =" << height;
	((Application*)buzzer::gApplication)->emit_keyboardHeightChanged(height);
}

JNIEXPORT void JNICALL Java_app_buzzer_mobile_MainActivity_globalGeometryChanged(JNIEnv* /*env*/, jobject, jint width, jint height) {
	if (!buzzer::gApplication) return;
	qDebug() << "[JAVA::globalGeometryChanged]: width =" << width << ", height =" << height;
	((Application*)buzzer::gApplication)->emit_globalGeometryChanged(width, height);
}

JNIEXPORT void JNICALL Java_app_buzzer_mobile_MainActivity_externalActivityCalled(JNIEnv* env, jobject, jint type, jstring chain, jstring tx, jstring buzzer)
{
	QString lChainFound = "none";
	QString lTxFound = "none";
	QString lBuzzerFound = "none";

	const char* lChain = env->GetStringUTFChars(chain, NULL);
	if (lChain) {
		lChainFound = QString::fromLocal8Bit(lChain);
		env->ReleaseStringUTFChars(chain, lChain);  // release resource
	}

	const char* lTx = env->GetStringUTFChars(tx, NULL);
	if (lTx) {
		lTxFound = QString::fromLocal8Bit(lTx);
		env->ReleaseStringUTFChars(tx, lTx);  // release resource
	}

	const char* lBuzzer = env->GetStringUTFChars(buzzer, NULL);
	if (lBuzzer) {
		lBuzzerFound = QString::fromLocal8Bit(lBuzzer);
		env->ReleaseStringUTFChars(buzzer, lBuzzer);  // release resource
	}

	qDebug() << "[JAVA::externalActivityCalled]: file = " << type << lChainFound << lTxFound << lBuzzerFound;
	((Application*)buzzer::gApplication)->emit_externalActivityCalled(type, lChainFound, lTxFound, lBuzzerFound);
}

#ifdef __cplusplus
}
#endif

#endif
