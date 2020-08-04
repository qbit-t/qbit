#ifndef WEBSOURCEINFO_H
#define WEBSOURCEINFO_H

#include <QObject>
#include <QQuickItem>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>

namespace buzzer {

class WebSourceInfo : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
	Q_PROPERTY(QString host READ host NOTIFY hostChanged)
	Q_PROPERTY(QString title READ title NOTIFY titleChanged)
	Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
	Q_PROPERTY(QString image READ image NOTIFY imageChanged)

	Q_PROPERTY(int type READ type NOTIFY typeChanged)

public:
	enum Type {
		INFO_EMPTY = 0,
		INFO_SIMPLE = 1,
		INFO_RICH = 2
	};

public:
	explicit WebSourceInfo(QObject* parent = nullptr);

	QString url() { return url_; }
	void setUrl(QString url) { url_ = url; }

	Q_INVOKABLE void process();
	void finished(QNetworkReply*);

	int type() { return (int)type_; }
	QString host() { return host_; }
	QString title() { return title_; }
	QString description() { return description_; }
	QString image() { return image_; }

signals:
	void urlChanged();
	void typeChanged();
	void hostChanged();
	void titleChanged();
	void descriptionChanged();
	void imageChanged();
	void processed();
	void error(QString message);

public slots:
	void readyToRead();
	void errorOccurred(QNetworkReply::NetworkError);

private:
	bool extractInfo(const QRegularExpression&, const QString&, QString&);
	bool extractInfoAtPos(const QRegularExpression&, const QString&, int pos, QString&);
	bool extractFullInfo(const QRegularExpression&, const QString&);

	void processCommon(QNetworkReply* reply);

private:
	QString url_;
	QNetworkReply* reply_;

	Type type_ = INFO_EMPTY;
	QString host_;
	QString title_;
	QString description_;
	QString image_;
};

} // buzzer

#endif // WEBSOURCEINFO_H
