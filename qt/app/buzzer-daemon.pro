TARGET = buzzerd

android: QT += qml quick androidextras
else: QT += qml quick

CONFIG += c++11

VERSION = 0.1.5.59
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
DEFINES += QBIT_VERSION_REVISION=5
DEFINES += QBIT_VERSION_BUILD=59

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
    daemon-main.cpp \
    logger.cpp \
    applicationpath.cpp \
	settings.cpp

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

TARGET.CAPABILITY += SwEvent

INCLUDEPATH += $$PWD/leveldb/android/include
INCLUDEPATH += $$PWD/libjpeg/android/jni
INCLUDEPATH += $$PWD/libpng/android/jni
INCLUDEPATH += $$PWD/../../secp256k1/include
INCLUDEPATH += $$PWD/../../secp256k1/src
INCLUDEPATH += $$PWD/../../secp256k1
INCLUDEPATH += $$PWD/../..
INCLUDEPATH += $$PWD/../

HEADERS += \
    error.h \
    logger.h \
    applicationpath.h \
    settings.h \
    ../../dapps/buzzer/buzzer.h \
    ../../dapps/buzzer/buzzfeed.h \
    ../../dapps/buzzer/buzzfeedview.h \
    ../../dapps/buzzer/eventsfeed.h \
    ../../dapps/buzzer/eventsfeedview.h \
    ../../client/dapps/buzzer/buzzercommands.h \
    ../../client/dapps/buzzer/buzzercomposer.h \
    ../../client/dapps/cubix/cubixcommands.h \
    ../../client/dapps/cubix/cubixcomposer.h \
	../../client/commands.h

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
