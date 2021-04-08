#include "imagelisting.h"

#ifdef Q_OS_ANDROID

using namespace buzzer;

ImageListing::ImageListing() {

}

void ImageListing::listImages() {
	//
	/*
	QAndroidJniObject lActionPick = QAndroidJniObject::getStaticObjectField("android/content/Intent", "ACTION_PICK", "Ljava/lang/String;");
	QAndroidJniObject lExternalContentUri = QAndroidJniObject::getStaticObjectField("android/provider/MediaStore$Images$Media", "EXTERNAL_CONTENT_URI", "Landroid/net/Uri;");

	QAndroidJniObject lIntent = QAndroidJniObject("android/content/Intent", "(Ljava/lang/String;Landroid/net/Uri;)V", lActionPick.object<jstring>(), lExternalContentUri.object<jobject>());

	if (lActionPick.isValid() && lIntent.isValid()) {
		lIntent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString("image/*").object<jstring>());
		QtAndroid::startActivity(lIntent.object<jobject>(), 101, this);
	}
	*/

	QAndroidJniObject lActionPick = QAndroidJniObject::getStaticObjectField("android/content/Intent", "ACTION_GET_CONTENT", "Ljava/lang/String;");
	//QAndroidJniObject lOpenable = QAndroidJniObject::getStaticObjectField("android/content/Intent", "CATEGORY_OPENABLE", "Ljava/lang/String;");
	QAndroidJniObject lExternalContentUri = QAndroidJniObject::getStaticObjectField("android/provider/MediaStore$Images$Media", "EXTERNAL_CONTENT_URI", "Landroid/net/Uri;");

	QAndroidJniObject lIntent = QAndroidJniObject("android/content/Intent", "(Ljava/lang/String;Landroid/net/Uri;)V", lActionPick.object<jstring>(), lExternalContentUri.object<jobject>());

	if (lActionPick.isValid() && lIntent.isValid()) {
		//lIntent.callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;", lOpenable.object<jstring>());
		lIntent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString("image/*").object<jstring>());
		QtAndroid::startActivity(lIntent.object<jobject>(), 101, this);
	}
}

void ImageListing::handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data) {
	//
	jint lResult = QAndroidJniObject::getStaticField<jint>("android/app/Activity", "RESULT_OK");
	qInfo() << "GALLERY" << resultCode << lResult << receiverRequestCode;
	if (/*receiverRequestCode == 101 || */ resultCode == lResult) {
		//
		QAndroidJniObject lUri = data.callObjectMethod("getData", "()Landroid/net/Uri;");
		QAndroidJniObject lDataAndroid = QAndroidJniObject::getStaticObjectField("android/provider/MediaStore$MediaColumns", "DATA", "Ljava/lang/String;");
		QAndroidJniEnvironment lEnv;
		jobjectArray lObjArray = (jobjectArray)lEnv->NewObjectArray(1, lEnv->FindClass("java/lang/String"), NULL);
		jobject lObj = lEnv->NewStringUTF(lDataAndroid.toString().toStdString().c_str());
		lEnv->SetObjectArrayElement(lObjArray, 0, lObj);
		QAndroidJniObject lResolver = QtAndroid::androidActivity().callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
		QAndroidJniObject lCursor = lResolver.callObjectMethod("query", "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;", lUri.object<jobject>(), lObjArray, NULL, NULL, NULL);
		jint lIndex = lCursor.callMethod<jint>("getColumnIndex", "(Ljava/lang/String;)I", lDataAndroid.object<jstring>());
		lCursor.callMethod<jboolean>("moveToFirst", "()Z");
		QAndroidJniObject lRes = lCursor.callObjectMethod("getString", "(I)Ljava/lang/String;", lIndex);
		QString lFile = lRes.toString();
		qInfo() << "GALLERY" << lFile;
		emit imageFound(lFile);

		/*
		QAndroidJniObject lString = data.callObjectMethod("getDataString", "()java/lang/String;");
		QString lFile = lString.toString();
		qInfo() << "GALLERY" << lFile;
		emit imageFound(lFile);
		*/
	}
}

#endif
