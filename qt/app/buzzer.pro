TEMPLATE = subdirs

android {
    SUBDIRS += buzzer-app.pro buzzer-daemon.pro buzzer-desktop-app.pro
}

ios {
    SUBDIRS += buzzer-app.pro
}


android: include(/home/demuskov/Android/Sdk/android_openssl/openssl.pri)

