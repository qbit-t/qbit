TEMPLATE = subdirs

android {
    SUBDIRS += buzzer-app.pro
}

ios {
    SUBDIRS += buzzer-app.pro
}


android: include(/home/demuskov/Android/Sdk/android_openssl/openssl.pri)

