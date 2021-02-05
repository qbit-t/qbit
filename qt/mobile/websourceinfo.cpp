#include "websourceinfo.h"

#include <QNetworkAccessManager>
#include <QUrl>

#include "iapplication.h"

using namespace buzzer;

WebSourceInfo::WebSourceInfo(QObject* /*parent*/) : QObject() {
	//
}

void WebSourceInfo::process() {
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
				title_ = lCapture.captured(2); lTitleFound = true;
			} else if (lCaptured == "twitter:image" || lCaptured == "og:image") {
				image_ = lCapture.captured(2); lDescriptionFound = true;
			} else if (lCaptured == "twitter:description" || lCaptured == "og:description") {
				description_ = lCapture.captured(2); lImageFound = true;
			}
		}
	}

	return lTitleFound && lDescriptionFound && lImageFound;
}

void WebSourceInfo::errorOccurred(QNetworkReply::NetworkError /*code*/) {
	//
	emit error(reply_->errorString());
}

void WebSourceInfo::readyToRead() {
	finished(reply_);
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
	std::vector<char> lLine; lLine.resize(1024);
	std::vector<char> lSource;
	//
	size_t lLen = 0;
	bool lFound = false;
	//
	QString lRawSource;
	while (!lFound && (lLen = reply->readLine(&lLine[0], 1023)) > 0 && lSource.size() < 7 * 1024) {
		lLine.resize(lLen); lSource.insert(lSource.end(), lLine.begin(), lLine.end());

		lRawSource = QString::fromStdString(std::string(lSource.begin(), lSource.end()));

		if (!lTitleFound) lTitleFound = extractInfo(lTitle, lRawSource, title_);
		if (!lDescriptionFound) lDescriptionFound = extractInfo(lDescription, lRawSource, description_);
		if (!lImageFound) lImageFound = extractInfo(lImage, lRawSource, image_);

		if (!lTitleFound) lTitleFound = extractInfo(lOgTitle, lRawSource, title_);
		if (!lDescriptionFound) lDescriptionFound = extractInfo(lOgDescription, lRawSource, description_);
		if (!lImageFound) lImageFound = extractInfo(lOgImage, lRawSource, image_);

		if (lTitleFound && lDescriptionFound && lImageFound) {
			lFound = true;
		} else {
			lLine.resize(1024);
		}
	}

	if (!lFound) {
		lFound = extractFullInfo(lReverse, lRawSource);
	}

	if (lFound) {
		title_ = title_.replace("&quot;", "\"").replace("&amp;", "&").replace("&#x27;", "'").replace("&#x39;", "'");
		description_ = description_.replace("&quot;", "\"").replace("&amp;", "&").replace("&#x27;", "'").replace("&#x39;", "'");
		image_ = image_.replace("&amp;", "&");
		type_ = INFO_RICH;

		emit typeChanged();
		emit titleChanged();
		emit descriptionChanged();
		emit imageChanged();
		emit hostChanged();
	}

	emit processed();
}

void WebSourceInfo::finished(QNetworkReply* reply) {
	// extract information
	if (reply->error() == QNetworkReply::NoError) {
		//
		processCommon(reply);
	} else {
		emit error(reply->errorString());
	}

	// finally
	reply->deleteLater();
}
