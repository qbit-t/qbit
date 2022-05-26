TARGET = buzzer

#QT += qml quick quickcontrols2 multimedia multimediawidgets widgets quickwidgets
QT += multimedia multimediawidgets qml quick quickcontrols2 widgets quickwidgets

CONFIG += c++11
CONFIG += static

VERSION = 0.1.5.62
DEFINES += VERSION_STRING=\\\"$${VERSION}\\\"
DEFINES += QT_ENVIRONMENT
DEFINES += HAVE_CONFIG_H
DEFINES += MOBILE_PLATFORM
DEFINES += CLIENT_PLATFORM
DEFINES += DESKTOP_PLATFORM

# Version
DEFINES += QBIT_VERSION_MAJOR=0
DEFINES += QBIT_VERSION_MINOR=1
DEFINES += QBIT_VERSION_REVISION=5
DEFINES += QBIT_VERSION_BUILD=62

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
    buzzfeedlistmodel.cpp \
    buzztexthighlighter.cpp \
    client.cpp \
    commandwrappers.cpp \
	conversationsfeedlistmodel.cpp \
    emojidata.cpp \
    emojimodel.cpp \
    eventsfeedlistmodel.cpp \
	imagelisting.cpp \
    main.cpp \
    logger.cpp \
    application.cpp \
    cameracontroler.cpp \
    applicationpath.cpp \
    notificator.cpp \
    peerslistmodel.cpp \
    pushdesktopnotification.cpp \
    settings.cpp \
	videoframesprovider.cpp \
	videorecorder.cpp \
	videosurface.cpp \
	wallettransactionslistmodel.cpp \
	websourceinfo.cpp \
	imageqx.cpp \
	imageqxloader.cpp \
	imageqxnode.cpp \
	shareutils.cpp

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
!isEmpty(target.path): INSTALLS += target

include(../qr/QZXing.pri)
include(statusbar/statusbar.pri)

TARGET.CAPABILITY += SwEvent

INCLUDEPATH += $$PWD/../../leveldb/include
INCLUDEPATH += $$PWD/../../secp256k1/include
INCLUDEPATH += $$PWD/../../secp256k1/src
INCLUDEPATH += $$PWD/../../secp256k1
INCLUDEPATH += $$PWD/../..
INCLUDEPATH += $$PWD/../

unix {
    QMAKE_LFLAGS += -no-pie

    INCLUDEPATH += $$PWD/../../boost

    LIBS += "../../leveldb/libleveldb.a"
    LIBS += "../../boost/stage/lib/libboost_random.a"
    LIBS += "../../boost/stage/lib/libboost_system.a"
    LIBS += "../../boost/stage/lib/libboost_thread.a"
    LIBS += "../../boost/stage/lib/libboost_chrono.a"
    LIBS += "../../boost/stage/lib/libboost_filesystem.a"

    LIBS += -ljpeg
    LIBS += -lpng
}

win32 {
    LIBS += "../../leveldb/libleveldb.a"

    LIBS += -lboost_random-mt-x64
    LIBS += -lboost_system-mt-x64
    LIBS += -lboost_thread-mt-x64
    LIBS += -lboost_chrono-mt-x64
    LIBS += -lboost_filesystem-mt-x64

    LIBS += -ljpeg
    LIBS += -lpng

    LIBS += -lopengl32
    LIBS += -lws2_32
    LIBS += -lwsock32
    LIBS += -lz
    LIBS += -lgdi32

    QMAKE_CXXFLAGS += -std=c++11

    DEFINES += WIN32_LEAN_AND_MEAN

    RC_ICONS = $$PWD/../images/buzzer.ico
}

DISTFILES += \
    buzzer-app.config \
    components/QuarkEmojiPopup.qml \
    components/QuarkEmojiTable.qml \
    components/QuarkLabelRegular.qml \
    emoji/emoji-google.json \
    qml/buzzeditor-desktop.qml \
    qml/buzzer-desktop-app.qml \
    qml/buzzer-main-desktop.qml \
    qml/buzzer-stackview-desktop.qml \
    qml/buzzercreateupdate-desktop.qml \
    qml/buzzerinfo-desktop.qml \
    qml/buzzertoolbar-desktop.qml \
    qml/buzzfeedthread-desktop.qml \
    qml/buzzitemmedia-editor-image-desktop.qml \
    qml/buzzitemmedia-editor-video-desktop.qml \
    qml/buzzitemurl-desktop.qml \
    qml/conversationthread-desktop.qml \
    qml/setupaskqbit-desktop.qml \
    qml/setupbuzzer-desktop.qml \
    qml/setupinfo-desktop.qml \
    qml/setupqbit-desktop.qml \
    qml/setupqbitaddress-desktop.qml \
    qml/setuptoolbar-desktop.qml \
    qml/walletreceivereceipt-desktop.qml \
    qml/buzzitemmedia-audio.qml \
    qml/buzzitemmedia-image.qml \
    qml/buzzitemmedia-video.qml \
    qml/buzzitemmediaview-image.qml \
    qml/buzzitemmediaview-video.qml

HEADERS += \
    asiodispatcher.h \
    buzzfeedlistmodel.h \
    buzztexthighlighter.h \
    client.h \
	conversationsfeedlistmodel.h \
    emojidata.h \
    emojimodel.h \
    error.h \
    eventsfeedlistmodel.h \
    iclient.h \
    logger.h \
    application.h \
    iapplication.h \
    applicationpath.h \
	line.h \
    notificator.h \
    notificator_p.h \
    pushdesktopnotification.h \
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
	shareutils.h

RESOURCES += \
    $$files(../fonts/*) \
#	$$files(../fonts-desktop/*) \
    $$files(../images/*) \
    $$files(backend/*) \
    $$files(qml/*) \
    $$files(lib/*) \
    $$files(components/*) \
	$$files(models/*) \
	$$files(emoji/*) \
	./buzzer-app.config \
	./buzzer.desktop \
	../fonts-desktop/NotoColorEmojiN.ttf

FORMS +=

DESTDIR = release

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.u
