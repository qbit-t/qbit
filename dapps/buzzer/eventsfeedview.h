// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_EVENTS_FEED_VIEW_H
#define QBIT_EVENTS_FEED_VIEW_H

#include "eventsfeed.h"
#include "buzzfeedview.h"

#if defined (CLIENT_PLATFORM)
	#include <QObject>
	#include <QString>
	#include <QQmlListProperty>
#endif

namespace qbit {

#if defined (CLIENT_PLATFORM)

class QEventInfo;
typedef std::shared_ptr<QEventInfo> QEventInfoPtr;

class QEventInfo: public QObject, public EventsfeedItem::EventInfo {
	Q_OBJECT

	Q_PROPERTY(unsigned long long timestamp READ q_timestamp NOTIFY q_timestampChanged)
	Q_PROPERTY(unsigned long long score READ q_score NOTIFY q_scoreChanged)
	Q_PROPERTY(QString eventId READ q_eventId NOTIFY q_eventIdChanged)
	Q_PROPERTY(QString eventChainId READ q_eventChainId NOTIFY q_eventChainIdChanged)
	Q_PROPERTY(QString buzzerId READ q_buzzerId NOTIFY q_buzzerIdChanged)
	Q_PROPERTY(QString buzzerInfoId READ q_buzzerInfoId NOTIFY q_buzzerInfoIdChanged)
	Q_PROPERTY(QString buzzerInfoChainId READ q_buzzerInfoChainId NOTIFY q_buzzerInfoChainIdChanged)
	Q_PROPERTY(QString buzzBody READ q_buzzBody NOTIFY q_buzzBodyChanged)
	Q_PROPERTY(QString buzzBodyHex READ q_buzzBodyHex NOTIFY q_buzzBodyHexChanged)
	Q_PROPERTY(QList<qbit::QBuzzerMediaPointer*> buzzMedia READ q_buzzMedia)

public:
	QEventInfo() {}
	QEventInfo(const EventInfo& other) {
		assign(other);
	}

	void assign(const EventInfo& info) {
		timestamp_ = info.timestamp();
		score_ = info.score();
		eventId_ = info.eventId();
		eventChainId_ = info.eventChainId();
		buzzerId_ = info.buzzerId();
		buzzerInfoId_ = info.buzzerInfoId();
		buzzerInfoChainId_ = info.buzzerInfoChainId();
		body_ = info.buzzBody();
		media_ = info.buzzMedia();
		signature_ = info.signature();
	}

	static QEventInfoPtr instance() {
		return std::make_shared<QEventInfo>();
	}

	static QEventInfoPtr instance(const EventInfo& info) {
		return std::make_shared<QEventInfo>(info);
	}

public:
	unsigned long long q_timestamp() { return timestamp_; }
	unsigned long long q_score() { return score_; }
	QString q_eventId() { return QString::fromStdString(eventId_.toHex()); }
	QString q_eventChainId() { return QString::fromStdString(eventChainId_.toHex()); }
	QString q_buzzerId() { return QString::fromStdString(buzzerId_.toHex()); }
	QString q_buzzerInfoId() { return QString::fromStdString(buzzerInfoId_.toHex()); }
	QString q_buzzerInfoChainId() { return QString::fromStdString(buzzerInfoChainId_.toHex()); }
	QString q_buzzBody() { return QString::fromStdString(buzzBodyString()); }
	QString q_buzzBodyHex() { return QString::fromStdString(buzzBodyHex());	}
	QList<qbit::QBuzzerMediaPointer*> q_buzzMedia() {
		//
		QList<qbit::QBuzzerMediaPointer*> lMediaPointers;

		for (std::vector<qbit::BuzzerMediaPointer>::const_iterator lPointer = media_.begin(); lPointer != media_.end(); lPointer++) {
			lMediaPointers.append(new qbit::QBuzzerMediaPointer(lPointer->chain(), lPointer->tx()));
		}

		return lMediaPointers;
	}

signals:
	void q_timestampChanged();
	void q_scoreChanged();
	void q_eventIdChanged();
	void q_eventChainIdChanged();
	void q_buzzerIdChanged();
	void q_buzzerInfoIdChanged();
	void q_buzzerInfoChainIdChanged();
	void q_buzzBodyChanged();
	void q_buzzBodyHexChanged();
};

class QEventsfeedItem: public QObject {
	Q_OBJECT

	Q_PROPERTY(unsigned long long timestamp READ q_timestamp NOTIFY q_timestampChanged)
	Q_PROPERTY(unsigned long long score READ q_score NOTIFY q_scoreChanged)
	Q_PROPERTY(QString buzzId READ q_buzzId NOTIFY q_buzzIdChanged)
	Q_PROPERTY(QString buzzChainId READ q_buzzChainId NOTIFY q_buzzChainIdChanged)
	Q_PROPERTY(QString publisherId READ q_publisherId NOTIFY q_publisherIdChanged)
	Q_PROPERTY(QString publisherInfoId READ q_publisherInfoId NOTIFY q_publisherInfoIdChanged)
	Q_PROPERTY(QString publisherInfoChainId READ q_publisherInfoChainId NOTIFY q_publisherInfoChainIdChanged)
	Q_PROPERTY(QString buzzBody READ q_buzzBody NOTIFY q_buzzBodyChanged)
	Q_PROPERTY(QList<qbit::QBuzzerMediaPointer*> buzzMedia READ q_buzzMedia)
	Q_PROPERTY(QList<qbit::QEventInfo*> buzzerInfos READ q_buzzerInfosMedia)

public:
	QEventsfeedItem() {}
	QEventsfeedItem(EventsfeedItemPtr item) {
		item_ = item;
	}
	~QEventsfeedItem() { item_.reset(); }

public:
	unsigned long long q_timestamp() { return item_->timestamp(); }
	unsigned long long q_score() { return item_->score(); }
	QString q_buzzId() { return QString::fromStdString(item_->buzzId().toHex()); }
	QString q_buzzChainId() { return QString::fromStdString(item_->buzzChainId().toHex()); }
	QString q_publisherId() { return QString::fromStdString(item_->publisher().toHex()); }
	QString q_publisherInfoId() { return QString::fromStdString(item_->publisherInfo().toHex()); }
	QString q_publisherInfoChainId() { return QString::fromStdString(item_->publisherInfoChain().toHex()); }
	QString q_buzzBody() { return QString::fromStdString(item_->buzzBodyString()); }
	QList<qbit::QBuzzerMediaPointer*> q_buzzMedia() {
		//
		QList<qbit::QBuzzerMediaPointer*> lMediaPointers;

		for (std::vector<qbit::BuzzerMediaPointer>::const_iterator lPointer = item_->buzzMedia().begin(); lPointer != item_->buzzMedia().end(); lPointer++) {
			lMediaPointers.append(new qbit::QBuzzerMediaPointer(lPointer->chain(), lPointer->tx()));
		}

		return lMediaPointers;
	}
	QList<qbit::QEventInfo*> q_buzzerInfosMedia() {
		//
		QList<qbit::QEventInfo*> lBuzzers;

		for (std::vector<qbit::EventsfeedItem::EventInfo>::const_iterator lPointer = item_->buzzers().begin(); lPointer != item_->buzzers().end(); lPointer++) {
			lBuzzers.append(new qbit::QEventInfo(*lPointer));
		}

		return lBuzzers;
	}

signals:
	void q_timestampChanged();
	void q_scoreChanged();
	void q_buzzIdChanged();
	void q_buzzChainIdChanged();
	void q_publisherIdChanged();
	void q_publisherInfoIdChanged();
	void q_publisherInfoChainIdChanged();
	void q_buzzBodyChanged();

private:
	qbit::EventsfeedItemPtr item_;
};

//
class EventsfeedItemProxy: public QObject {
	Q_OBJECT

public:
	EventsfeedItemProxy() {}
	EventsfeedItemProxy(EventsfeedItemPtr item): item_(item) {
	}
	EventsfeedItemProxy(const EventsfeedItemProxy& other) {
		set(other.get());
	}

	void set(EventsfeedItemPtr item) { item_ = item; }
	EventsfeedItemPtr get() const { return item_; }

private:
	EventsfeedItemPtr item_;
};

//
class EventsfeedProxy: public QObject {
	Q_OBJECT

public:
	EventsfeedProxy() {}
	EventsfeedProxy(EventsfeedPtr item): item_(item) {
	}
	EventsfeedProxy(const EventsfeedProxy& other) {
		set(other.get());
	}

	void set(EventsfeedPtr item) { item_ = item; }
	EventsfeedPtr get() const { return item_; }

private:
	EventsfeedPtr item_;
};

#endif

} // qbit

#endif
