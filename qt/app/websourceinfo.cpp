#include "websourceinfo.h"

#include <QNetworkAccessManager>
#include <QUrl>

#include "iapplication.h"

using namespace buzzer;

WebSourceInfo::WebSourceInfo(QObject* /*parent*/) : QObject() {
}

WebSourceInfo::~WebSourceInfo() {
	//
	qInfo() << "WebSourceInfo::~WebSourceInfo()";

	if (reply_) {
		disconnect(reply_, &QIODevice::readyRead, 0, 0);
		disconnect(reply_, &QNetworkReply::errorOccurred, 0, 0);
	}

	reply_ = nullptr;
}

void WebSourceInfo::process() {
	//
	// TODO: potential leak, need "check list" to track such objects
	// QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

	//
	QNetworkAccessManager* lManager = gApplication->getNetworkManager();

	QNetworkRequest lRequest(url_);
	lRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true); // follow redirects

	if (url_.indexOf("https:")) {
		lRequest.setSslConfiguration(QSslConfiguration::defaultConfiguration());
	}

	// extract host
	QUrl lUrl(url_);
	host_ = lUrl.host();
	host_ = host_.replace("www.", "");
	totalBytes_ = 0;

	reply_ = lManager->get(lRequest);
	connect(reply_, &QIODevice::readyRead, this, &WebSourceInfo::readyToRead);
	connect(reply_, &QNetworkReply::errorOccurred, this, &WebSourceInfo::errorOccurred);
}

bool WebSourceInfo::extractInfo(const QRegularExpression& expression, const QString& text, QString& result) {
	QRegularExpressionMatch lMatch = expression.match(text);
	if (lMatch.hasMatch()) {
		QString lCaptured = lMatch.captured(3);
		if (lCaptured.size() > 3) {
			result = lCaptured.mid(1, lCaptured.length()-2);
			return true;
		}
	}

	return false;
}

bool WebSourceInfo::extractInfoAtPos(const QRegularExpression& expression, const QString& text, int pos, QString& result) {
	QRegularExpressionMatch lMatch = expression.match(text);
	if (lMatch.hasMatch()) {
		qInfo() << lMatch.captured(0);
		qInfo() << lMatch.captured(1);
		qInfo() << lMatch.captured(pos);
		QString lCaptured = lMatch.captured(pos);
		if (lCaptured.size() > 3) {
			result = lCaptured;
			return true;
		}
	}

	return false;
}

bool WebSourceInfo::extractFullInfo(const QRegularExpression& expression, const QString& text) {
	//
	bool lTitleFound = false;
	bool lDescriptionFound = false;
	bool lImageFound = false;

	QRegularExpressionMatchIterator lMatch = expression.globalMatch(text);
	while (lMatch.hasNext() && (!lTitleFound || !lDescriptionFound || !lImageFound)) {
		QRegularExpressionMatch lCapture = lMatch.next();
		if (lCapture.captured(3) == "property") {
			QString lCaptured = lCapture.captured(4);
			if (lCaptured == "twitter:title" || lCaptured == "og:title") {
				if (!title_.length()) title_ = lCapture.captured(2); lTitleFound = true;
			} else if (lCaptured == "twitter:image" || lCaptured == "og:image") {
				if (!image_.length()) image_ = lCapture.captured(2); lImageFound = true;
			} else if (lCaptured == "twitter:description" || lCaptured == "og:description") {
				if (!description_.length()) description_ = lCapture.captured(2); lDescriptionFound = true;
			}
		}
	}

	return lTitleFound && lDescriptionFound && lImageFound;
}

void WebSourceInfo::errorOccurred(QNetworkReply::NetworkError /*code*/) {
	//
	if (reply_)
		emit error(reply_->errorString());
}

void WebSourceInfo::readyToRead() {
	if (reply_) finished(reply_);
}

void WebSourceInfo::processCommon(QNetworkReply* reply) {
	//
	// TODO: extract content with \r (\"(?:([^,-]*)(?:[^,-]|$))\")
	QRegularExpression lTitle = QRegularExpression("(\"twitter:title\"[ ])(content[= ])(\"(.*?)\")");
	QRegularExpression lImage = QRegularExpression("(\"twitter:image\"[ ])(content[= ])(\"(?:https?|ftp)://\\S+\")");
	QRegularExpression lDescription = QRegularExpression("(\"twitter:description\"[ ])(content[= ])(\"(.*?)\")");
	QRegularExpression lOgTitle = QRegularExpression("(\"og:title\"[ ])(content[= ])(\"(.*?)\")");
	QRegularExpression lOgImage = QRegularExpression("(\"og:image\"[ ])(content[= ])(\"(?:https?|ftp)://\\S+\")");
	QRegularExpression lOgDescription = QRegularExpression("(\"og:description\"[ ])(content[= ])(\"(.*?)\")");

	// all reverse
	QRegularExpression lReverse =
			QRegularExpression("(content[= ]\"(.*?)\")[^>](\\w+)[= ]\"(.*?)\"");

	bool lTitleFound = false;
	bool lDescriptionFound = false;
	bool lImageFound = false;

	//
	qint64 lBytesAvailable = !reply->bytesAvailable() ? 1024 : reply->bytesAvailable();
	std::vector<char> lLine; lLine.resize(lBytesAvailable);
	std::vector<char> lSource;
	//
	size_t lLen = 0;
	bool lFound = false;
	//
	QString lRawSource;
	if ((lLen = reply->read(&lLine[0], lBytesAvailable)) > 0) {
		lLine.resize(lLen); totalBytes_ += lLen;

		// TODO: do we need to concat?
		lRawSource = QString::fromStdString(std::string(lLine.begin(), lLine.end()));

		if (!lTitleFound) lTitleFound = extractInfo(lTitle, lRawSource, title_);
		if (!lDescriptionFound) lDescriptionFound = extractInfo(lDescription, lRawSource, description_);
		if (!lImageFound) lImageFound = extractInfo(lImage, lRawSource, image_);

		if (!lTitleFound) lTitleFound = extractInfo(lOgTitle, lRawSource, title_);
		if (!lDescriptionFound) lDescriptionFound = extractInfo(lOgDescription, lRawSource, description_);
		if (!lImageFound) lImageFound = extractInfo(lOgImage, lRawSource, image_);

		if (lTitleFound && /*lDescriptionFound &&*/ lImageFound) {
			lFound = true;
		}

		/*
		qInfo() << "Title:" << title_;
		qInfo() << "Image:" << image_;
		qInfo() << "totalBytes_:" << totalBytes_;
		qInfo() << "lBytesAvailable:" << lBytesAvailable;
		*/
	}

	if (!lFound) {
		lFound = extractFullInfo(lReverse, lRawSource);
	}

	if (lFound) {
		title_ = title_.replace("&quot;", "\"").replace("&amp;", "&").replace("&#x27;", "'").replace("&#x39;", "'").replace("&#39;", "'").replace("&#039;", "'");
		description_ = description_.replace("&quot;", "\"").replace("&amp;", "&").replace("&#x27;", "'").replace("&#x39;", "'").replace("&#39;", "'").replace("&#039;", "'");
		image_ = image_.replace("&amp;", "&");
		type_ = INFO_RICH;

		emit typeChanged();
		emit titleChanged();
		emit descriptionChanged();
		emit imageChanged();
		emit hostChanged();
		emit processed();
		// remove
		reply->deleteLater();
	} else if (totalBytes_ > 300 * 1024) {
		// just finish it
		emit processed();
		reply->deleteLater();
	}

	// wait next chunk...
}

void WebSourceInfo::finished(QNetworkReply* reply) {
	// extract information
	if (reply->error() == QNetworkReply::NoError) {
		//
		processCommon(reply);
	} else {
		emit error(reply->errorString());
		reply->deleteLater();
	}
}
