TEMPLATE = subdirs

android {
    SUBDIRS += buzzer-app.pro buzzer-daemon.pro
}

ios {
    SUBDIRS += buzzer-app.pro
}

linux {
    SUBDIRS += buzzer-desktop-app.pro
}

android: include(/home/demuskov/Android/Sdk/android_openssl/openssl.pri)

