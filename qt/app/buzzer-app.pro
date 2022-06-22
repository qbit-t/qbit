TARGET = buzzer

android: QT += qml quick quickcontrols2 androidextras multimedia sensors
else: QT += qml quick quickcontrols2 multimedia

CONFIG += c++11

VERSION = 0.1.6.71
DEFINES += VERSION_STRING=\\\"$${VERSION}\\\"
DEFINES += QT_ENVIRONMENT
DEFINES += BUZZER_MOD
DEFINES += CUBIX_MOD
DEFINES += HAVE_CONFIG_H
DEFINES += MOBILE_PLATFORM
DEFINES += CLIENT_PLATFORM

# Version
DEFINES += QBIT_VERSION_MAJOR=0
DEFINES += QBIT_VERSION_MINOR=1
DEFINES += QBIT_VERSION_REVISION=6
DEFINES += QBIT_VERSION_BUILD=71

DEFINES += BUZZER_MOD
DEFINES += CUBIX_MOD
# remove for production
DEFINES += PRODUCTION_MOD

SOURCES += \
    ../../secp256k1/src/secp256k1.c \
    ../../key.cpp \
    ../../uint256.cpp \
    ../../arith_uint256.cpp \
    ../../utilstrencodings.cpp \
    ../../context.cpp \
    ../../crypto/ctaes/ctaes.c \
    ../../crypto/aes.cpp \
    ../../crypto/ripemd160.cpp \
    ../../crypto/sha256.cpp \
    ../../crypto/sha512.cpp \
    ../../crypto/hmac_sha256.cpp \
    ../../crypto/hmac_sha512.cpp \
    ../../hash/blake2/blake2b-ref.c \
    ../../base58.cpp \
    ../../helpers/cleanse.cpp \
    ../../allocator.cpp \
    ../../lockedpool.cpp \
    ../../vm/qasm.cpp \
    ../../vm/vm.cpp \
    ../../transaction.cpp \
    ../../hash.cpp \
    ../../block.cpp \
    ../../log/log.cpp \
    ../../db/leveldb.cpp \
    ../../fs.cpp \
    ../../timestamp.cpp \
    ../../mkpath.cpp \
    ../../peer.cpp \
    ../../seedwords.cpp \
    ../../message.cpp \
    ../../lightwallet.cpp \
    ../../json.cpp \
    ../../pow.cpp \
    ../../dapps/buzzer/buzzer.cpp \
    ../../dapps/buzzer/buzzfeed.cpp \
	../../dapps/buzzer/eventsfeed.cpp \
	../../dapps/buzzer/conversationsfeed.cpp \
	../../dapps/buzzer/peerextension.cpp \
    ../../dapps/buzzer/transactionstoreextension.cpp \
    ../../dapps/buzzer/transactionactions.cpp \
    ../../client/dapps/buzzer/buzzercommands.cpp \
    ../../client/dapps/buzzer/buzzercomposer.cpp \
    ../../client/dapps/cubix/cubixcommands.cpp \
    ../../client/dapps/cubix/cubixcomposer.cpp \
	../../client/dapps/cubix/exif.cpp \
	../../client/commands.cpp \
    ../../client/commandshandler.cpp \
    asiodispatcher.cpp \
    audiorecorder.cpp \
    buzzfeedlistmodel.cpp \
    buzztexthighlighter.cpp \
    client.cpp \
    commandwrappers.cpp \
	conversationsfeedlistmodel.cpp \
    eventsfeedlistmodel.cpp \
	imagelisting.cpp \
    main.cpp \
    logger.cpp \
    application.cpp \
    cameracontroler.cpp \
    applicationpath.cpp \
    peerslistmodel.cpp \
    settings.cpp \
    videoframesprovider.cpp \
    videorecorder.cpp \
    videosurface.cpp \
    wallettransactionslistmodel.cpp \
	websourceinfo.cpp \
	imageqx.cpp \
	imageqxloader.cpp \
	imageqxnode.cpp \
	androidshareutils.cpp \
	shareutils.cpp \
	emojimodel.cpp \
	emojidata.cpp

SUBDIRS += \
    ../../client \
    ../../client/dapps/cubix \
    ../../client/dapps/buzzer

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include(../qr/QZXing.pri)

TARGET.CAPABILITY += SwEvent

INCLUDEPATH += $$PWD/leveldb/android/include
INCLUDEPATH += $$PWD/libjpeg/android/jni
INCLUDEPATH += $$PWD/libpng/android/jni
INCLUDEPATH += $$PWD/../../secp256k1/include
INCLUDEPATH += $$PWD/../../secp256k1/src
INCLUDEPATH += $$PWD/../../secp256k1
INCLUDEPATH += $$PWD/../..
INCLUDEPATH += $$PWD/../

DISTFILES += \
    buzzer-app.config \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
	android/res/values/libs.xml \
	android/res/xml/filepaths.xml \
	android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat \
    android/src/app/buzzer/mobile/NotificatorService.java \
    android/src/app/buzzer/mobile/MainActivity.java \
    android/src/app/buzzer/mobile/Notification.java \
    android/src/app/buzzer/mobile/NotificatorBroadcastReceiver.java \
    android/src/app/buzzer/mobile/FingerprintHandler.java \
	android/src/app/buzzer/mobile/FileUtils.java \
	android/src/app/buzzer/mobile/ShareUtils.java \
	android/src/app/buzzer/mobile/KeyboardProvider.java \
	components/QuarkRoundRectangle.qml \
    components/QuarkRoundSymbolButton.qml \
    ios/Info.plist \
    ios/Launch.xib \
    qml/buzzercreateupdate.qml \
    qml/buzzerheader.qml \
    qml/buzzerqbitkey.qml \
    qml/buzzerquickhelp.qml \
    qml/buzzitemmedia-audio.qml \
    qml/buzzitemmedia-editor-audio.qml \
    qml/buzzitemmedia-editor-image.qml \
    qml/buzzitemmedia-editor-video.qml \
    qml/buzzitemmedia-image.qml \
    qml/buzzitemmedia-mediaplayer.qml \
    qml/buzzitemmedia-player-controller.qml \
    qml/buzzitemmedia-video.qml \
    qml/buzzitemmedia-videooutput.qml \
    qml/buzzitemmediaview-image.qml \
    qml/buzzitemmediaview-video.qml \
    qml/camera-video.qml \
    qml/conversationitem.qml \
    qml/conversationmessageitem.qml \
    qml/conversationsfeed.qml \
    qml/conversationthread.qml \
    qml/eventconversationitem.qml \
    qml/peerinactiveitem.qml \
    qml/peeritem.qml \
    qml/peers.qml \
    qml/peerslist.qml \
    qml/peersmanuallist.qml \
    qml/walletlist.qml

HEADERS += \
    asiodispatcher.h \
    audiorecorder.h \
    buzzfeedlistmodel.h \
    buzztexthighlighter.h \
    client.h \
	conversationsfeedlistmodel.h \
    error.h \
    eventsfeedlistmodel.h \
    iclient.h \
    logger.h \
    application.h \
    iapplication.h \
    applicationpath.h \
	line.h \
	roundframe.h \
	cameracontroler.h \
    peerslistmodel.h \
	settings.h \
	commandwrappers.h \
	imagelisting.h \
	../../dapps/buzzer/buzzer.h \
	../../dapps/buzzer/buzzfeed.h \
	../../dapps/buzzer/buzzfeedview.h \
	../../dapps/buzzer/eventsfeed.h \
	../../dapps/buzzer/eventsfeedview.h \
	../../dapps/buzzer/conversationsfeed.h \
	../../dapps/buzzer/conversationsfeedview.h \
	../../client/dapps/buzzer/buzzercommands.h \
	../../client/dapps/buzzer/buzzercomposer.h \
	../../client/dapps/cubix/cubixcommands.h \
	../../client/dapps/cubix/cubixcomposer.h \
	../../client/commands.h \
    videoframesprovider.h \
    videorecorder.h \
    videosurface.h \
    wallettransactionslistmodel.h \
	websourceinfo.h \
	imageqx.h \
	imageqxloader.h \
	imageqxnode.h \
	androidshareutils.h \
	shareutils.h \
	emojimodel.h \
	emojidata.h

RESOURCES += \
    $$files(../fonts/*) \
    $$files(../images/*) \
    $$files(backend/*) \
    $$files(qml/*) \
    $$files(lib/*) \
    $$files(components/*) \
	$$files(models/*) \
	../fonts-desktop/NotoSansMono-Regular.ttf

include(statusbar/statusbar.pri)

dep_root.files += android/*
dep_root.path = /assets
INSTALLS += dep_root

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    INCLUDEPATH += $$PWD/boost/boost_1_73_0

    #QMAKE_CXXFLAGS += -fPIC

    DEFINES += OS_ANDROID
	DEFINES += MOBILE_PLATFORM_32

    DEPENDPATH += $$PWD/leveldb/android/obj/local/armeabi-v7a
    DEPENDPATH += $$PWD/boost/boost_1_73_0/stage/lib
    DEPENDPATH += $$PWD/libjpeg/android/obj/local/armeabi-v7a
    DEPENDPATH += $$PWD/libpng/android/obj/local/armeabi-v7a

    LIBS += -L$$PWD/leveldb/android/obj/local/armeabi-v7a/ -lleveldb
	LIBS += -L$$PWD/boost/boost_1_73_0/stage/lib/ -lboost_random-clang-mt-s-a32-1_73
	LIBS += -L$$PWD/boost/boost_1_73_0/stage/lib/ -lboost_system-clang-mt-s-a32-1_73
	LIBS += -L$$PWD/boost/boost_1_73_0/stage/lib/ -lboost_thread-clang-mt-s-a32-1_73
	LIBS += -L$$PWD/boost/boost_1_73_0/stage/lib/ -lboost_chrono-clang-mt-s-a32-1_73
	LIBS += -L$$PWD/boost/boost_1_73_0/stage/lib/ -lboost_filesystem-clang-mt-s-a32-1_73

    LIBS += -L$$PWD/libjpeg/android/obj/local/armeabi-v7a/ -ljpeg
	LIBS += -L$$PWD/libpng/android/obj/local/armeabi-v7a/ -lpng

    # BUG: https://bugreports.qt.io/browse/QTBUG-81866
	# ANDROID_EXTRA_LIBS = \
	#    $$PWD/leveldb/android/obj/local/armeabi-v7a/libleveldb.so \
	#	$$PWD/bin/android/arm/libcrypto_1_1.so \
	#	$$PWD/bin/android/arm/libssl_1_1.so
}

contains(ANDROID_TARGET_ARCH,arm64-v8a) {
    INCLUDEPATH += $$PWD/boost/boost_64_1_73_0

    DEFINES += OS_ANDROID
	DEFINES += MOBILE_PLATFORM_64

    DEPENDPATH += $$PWD/leveldb/android/obj/local/arm64-v8a
    DEPENDPATH += $$PWD/boost/boost_64_1_73_0/stage/lib
    DEPENDPATH += $$PWD/libjpeg/android/obj/local/arm64-v8a
    DEPENDPATH += $$PWD/libpng/android/obj/local/arm64-v8a

    LIBS += -L$$PWD/leveldb/android/obj/local/arm64-v8a/ -lleveldb
	LIBS += -L$$PWD/boost/boost_64_1_73_0/stage/lib/ -lboost_random-clang-mt-s-a64-1_73
	LIBS += -L$$PWD/boost/boost_64_1_73_0/stage/lib/ -lboost_system-clang-mt-s-a64-1_73
	LIBS += -L$$PWD/boost/boost_64_1_73_0/stage/lib/ -lboost_thread-clang-mt-s-a64-1_73
	LIBS += -L$$PWD/boost/boost_64_1_73_0/stage/lib/ -lboost_chrono-clang-mt-s-a64-1_73
	LIBS += -L$$PWD/boost/boost_64_1_73_0/stage/lib/ -lboost_filesystem-clang-mt-s-a64-1_73

    LIBS += -L$$PWD/libjpeg/android/obj/local/arm64-v8a/ -ljpeg
	LIBS += -L$$PWD/libpng/android/obj/local/arm64-v8a/ -lpng

    # BUG: https://bugreports.qt.io/browse/QTBUG-81866
	# ANDROID_EXTRA_LIBS = \
	#    $$PWD/leveldb/android/obj/local/arm64-v8a/libleveldb.so \
	#	$$PWD/bin/android/arm64/libcrypto_1_1.so \
	#	$$PWD/bin/android/arm64/libssl_1_1.so
}

android {
    EXTRA_FILES += \
	    $$PWD/buzzer-app.config
		for(FILE,EXTRA_FILES){
		    QMAKE_POST_LINK += $$quote(cp $${FILE} $$PWD/android$$escape_expand(\n\t))
		}
}

ios {

    IOS_TARGET_OS = "iphoneos"
    CONFIG(iphonesimulator, iphoneos|iphonesimulator): IOS_TARGET_OS = "iphonesimulator"

    QMAKE_INFO_PLIST = ios/Info.plist

    OBJECTIVE_SOURCES += \
        $$PWD/ios/biometricauthenticator.m \
        $$PWD/ios/localnotificator.mm

    LIBS += -framework UIKit
    LIBS += -framework UserNotifications

    #Q_PRODUCT_BUNDLE_IDENTIFIER.name = PRODUCT_BUNDLE_IDENTIFIER
    #Q_PRODUCT_BUNDLE_IDENTIFIER.value = app.buzzer.ios
    #QMAKE_MAC_XCODE_SETTINGS += Q_PRODUCT_BUNDLE_IDENTIFIER

    QMAKE_TARGET_BUNDLE_PREFIX = app.buzzer
    QMAKE_BUNDLE = ios

    MY_DEVELOPMENT_TEAM.value = 97PSCD5842 # fix for production
    #QMAKE_PROVISINOING_PROFILE = 5d8e82be-4740-435c-848c-f0ab72d23ba7

    MY_DEVELOPMENT_TEAM.name = DEVELOPMENT_TEAM
    QMAKE_MAC_XCODE_SETTINGS += MY_DEVELOPMENT_TEAM

    MY_ENTITLEMENTS.name = CODE_SIGN_ENTITLEMENTS
    MY_ENTITLEMENTS.value = $$PWD/ios/localnotificator_production.entitlements # fix for production
    QMAKE_MAC_XCODE_SETTINGS += MY_ENTITLEMENTS

    #plugins.path = PlugIns
    #plugins.files = $$OUT_PWD/Release-iphoneos/buzzer-notification.appex
    #QMAKE_BUNDLE_DATA += plugins

    # app_launch_images.files = $$PWD/ios/Launch.xib $$files($$PWD/ios/LaunchImage*.png)
    app_launch_images.files = $$PWD/ios/Launch.xib $$PWD/buzzer-icon-128.png $$PWD/buzzer-icon-256.png $$PWD/buzzer-icon-512.png $$PWD/buzzer-app.config
    QMAKE_BUNDLE_DATA += app_launch_images
    EXTRA_FILES += \
        $$PWD/buzzer-icon-128.png \
        $$PWD/buzzer-icon-256.png \
        $$PWD/buzzer-icon-512.png \
        $$PWD/buzzer-app.config
    for(FILE,EXTRA_FILES){
        QMAKE_POST_LINK += $$quote(cp $${FILE} $$OUT_PWD/Release-$$IOS_TARGET_OS/buzzer.app$$escape_expand(\n\t))
    }
}

