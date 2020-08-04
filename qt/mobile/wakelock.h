#ifndef WAKELOCK_H
#define WAKELOCK_H

#include <QDebug>

#ifdef Q_OS_ANDROID
#include <QAndroidService>
#include <QAndroidJniObject>
#include <QAndroidIntent>
#include <QAndroidBinder>
#include <QtAndroid>
#endif

namespace buzzer {

class WakeLock
{
public:
    WakeLock()
    {
        setWakeLock(1);
    }
    ~WakeLock()
    {
        setWakeLock(0);
    }

    void setWakeLock(int lock)
    {
    #ifdef Q_OS_ANDROID
        qDebug() << "[WakeLock::setWakeLock]: beggining";
        if (!wakeLock_.isValid())
        {
            QAndroidJniObject lContext = QtAndroid::androidContext();
            if (lContext.isValid())
            {
                QAndroidJniObject lServiceName = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Context","POWER_SERVICE");
                if (lServiceName.isValid())
                {
                    QAndroidJniObject lPowerMgr = lContext.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;",lServiceName.object<jobject>());
                    if (lPowerMgr.isValid())
                    {
                        jint lLevelAndFlags = QAndroidJniObject::getStaticField<jint>("android/os/PowerManager","PARTIAL_WAKE_LOCK");

                        QAndroidJniObject lTag = QAndroidJniObject::fromString("BUZZER-SERVICE");

                        wakeLock_ = lPowerMgr.callObjectMethod("newWakeLock", "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;", lLevelAndFlags,lTag.object<jstring>());
                    }
                }
            }
        }

        if (wakeLock_.isValid())
        {
            if (lock)
            {
                qDebug() << "[WakeLock::setWakeLock]: acquire lock";
                wakeLock_.callMethod<void>("acquire", "()V");
            }
            else if (!lock)
            {
                qDebug() << "[WakeLock::setWakeLock]: release lock";
                wakeLock_.callMethod<void>("release", "()V");
            }
        }
        else
            qDebug() << "[WakeLock::setWakeLock]: lock is not valid.";

    #endif
    }

private:
#ifdef Q_OS_ANDROID
    QAndroidJniObject wakeLock_;
#endif
};

} // buzzer

#endif // WAKELOCK_H
