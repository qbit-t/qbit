// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZ_FEED_VIEW_H
#define QBIT_BUZZ_FEED_VIEW_H

#include "buzzfeed.h"

#if defined (CLIENT_PLATFORM)
	#include <QObject>
	#include <QString>
	#include <QQmlListProperty>
#endif

namespace qbit {

#if defined (CLIENT_PLATFORM)
//
class QBuzzerMediaPointer;
typedef std::shared_ptr<QBuzzerMediaPointer> QBuzzerMediaPointerPtr;

class QBuzzerMediaPointer: public QObject, public BuzzerMediaPointer {
	Q_OBJECT

	Q_PROPERTY(QString chain READ q_chain NOTIFY q_chainChanged)
	Q_PROPERTY(QString tx READ q_tx NOTIFY q_txChanged)
	Q_PROPERTY(QString url READ q_url NOTIFY q_urlChanged)

public:
	QBuzzerMediaPointer() { chain_.setNull(); tx_.setNull(); }
	QBuzzerMediaPointer(const uint256& chain, const uint256& tx) : BuzzerMediaPointer(chain, tx) {}
	virtual ~QBuzzerMediaPointer() {}

	const uint256& chain() const { return chain_; }
	const uint256& tx() const { return tx_; }

	static QBuzzerMediaPointerPtr instance(const BuzzerMediaPointer& other) {
		return std::make_shared<QBuzzerMediaPointer>(other.chain(), other.tx());
	}

public:
	QString q_chain() {
		return QString::fromStdString(chain_.toHex());
	}

	QString q_tx() {
		return QString::fromStdString(tx_.toHex());
	}

	QString q_url() {
		return QString::fromStdString(tx_.toHex() + "/" + chain_.toHex());
	}

signals:
	void q_chainChanged();
	void q_txChanged();
	void q_urlChanged();
};

class QItemInfo;
typedef std::shared_ptr<QItemInfo> QItemInfoPtr;

class QItemInfo: public QObject, public BuzzfeedItem::ItemInfo {
	Q_OBJECT

	Q_PROPERTY(unsigned long long timestamp READ q_timestamp NOTIFY q_timestampChanged)
	Q_PROPERTY(unsigned long long score READ q_score NOTIFY q_scoreChanged)
	Q_PROPERTY(QString eventId READ q_eventId NOTIFY q_eventIdChanged)
	Q_PROPERTY(QString eventChainId READ q_eventChainId NOTIFY q_eventChainIdChanged)
	Q_PROPERTY(QString buzzerId READ q_buzzerId NOTIFY q_buzzerIdChanged)
	Q_PROPERTY(QString buzzerInfoId READ q_buzzerInfoId NOTIFY q_buzzerInfoIdChanged)
	Q_PROPERTY(QString buzzerInfoChainId READ q_buzzerInfoChainId NOTIFY q_buzzerInfoChainIdChanged)

public:
	QItemInfo() {}
	QItemInfo(const ItemInfo& other) {
		assign(other);
	}

	void assign(const ItemInfo& info) {
		timestamp_ = info.timestamp();
		score_ = info.score();
		eventId_ = info.eventId();
		eventChainId_ = info.eventChainId();
		buzzerId_ = info.buzzerId();
		buzzerInfoId_ = info.buzzerInfoId();
		buzzerInfoChainId_ = info.buzzerInfoChainId();
		signature_ = info.signature();
	}

	static QItemInfoPtr instance() {
		return std::make_shared<QItemInfo>();
	}

	static QItemInfoPtr instance(const ItemInfo& info) {
		return std::make_shared<QItemInfo>(info);
	}

public:
	unsigned long long q_timestamp() { return timestamp_; }
	unsigned long long q_score() { return score_; }
	QString q_eventId() { return QString::fromStdString(eventId_.toHex()); }
	QString q_eventChainId() { return QString::fromStdString(eventChainId_.toHex()); }
	QString q_buzzerId() { return QString::fromStdString(buzzerId_.toHex()); }
	QString q_buzzerInfoId() { return QString::fromStdString(buzzerInfoId_.toHex()); }
	QString q_buzzerInfoChainId() { return QString::fromStdString(buzzerInfoChainId_.toHex()); }

signals:
	void q_timestampChanged();
	void q_scoreChanged();
	void q_eventIdChanged();
	void q_eventChainIdChanged();
	void q_buzzerIdChanged();
	void q_buzzerInfoIdChanged();
	void q_buzzerInfoChainIdChanged();
};

class QBuzzfeedItem: public QObject {
	Q_OBJECT

	Q_PROPERTY(unsigned long long timestamp READ q_timestamp NOTIFY q_timestampChanged)
	Q_PROPERTY(unsigned long long score READ q_score NOTIFY q_scoreChanged)
	Q_PROPERTY(QString buzzId READ q_buzzId NOTIFY q_buzzIdChanged)
	Q_PROPERTY(QString buzzChainId READ q_buzzChainId NOTIFY q_buzzChainIdChanged)
	Q_PROPERTY(QString buzzerId READ q_buzzerId NOTIFY q_buzzerIdChanged)
	Q_PROPERTY(QString buzzerInfoId READ q_buzzerInfoId NOTIFY q_buzzerInfoIdChanged)
	Q_PROPERTY(QString buzzerInfoChainId READ q_buzzerInfoChainId NOTIFY q_buzzerInfoChainIdChanged)
	Q_PROPERTY(QString buzzBody READ q_buzzBody NOTIFY q_buzzBodyChanged)
	Q_PROPERTY(QList<qbit::QBuzzerMediaPointer*> buzzMedia READ q_buzzMedia)

public:
	QBuzzfeedItem() {}
	QBuzzfeedItem(BuzzfeedItemPtr item) {
		item_ = item;
	}
	~QBuzzfeedItem() { item_.reset(); }

public:
	unsigned long long q_timestamp() { return item_->timestamp(); }
	unsigned long long q_score() { return item_->score(); }
	QString q_buzzId() { return QString::fromStdString(item_->buzzId().toHex()); }
	QString q_buzzChainId() { return QString::fromStdString(item_->buzzChainId().toHex()); }
	QString q_buzzerId() { return QString::fromStdString(item_->buzzerId().toHex()); }
	QString q_buzzerInfoId() { return QString::fromStdString(item_->buzzerInfoId().toHex()); }
	QString q_buzzerInfoChainId() { return QString::fromStdString(item_->buzzerInfoChainId().toHex()); }
	QString q_buzzBody() { return QString::fromStdString(item_->buzzBodyString()); }
	QList<qbit::QBuzzerMediaPointer*> q_buzzMedia() {
		//
		QList<qbit::QBuzzerMediaPointer*> lMediaPointers;

		for (std::vector<qbit::BuzzerMediaPointer>::const_iterator lPointer = item_->buzzMedia().begin(); lPointer != item_->buzzMedia().end(); lPointer++) {
			lMediaPointers.append(new qbit::QBuzzerMediaPointer(lPointer->chain(), lPointer->tx()));
		}

		return lMediaPointers;
	}

signals:
	void q_timestampChanged();
	void q_scoreChanged();
	void q_buzzIdChanged();
	void q_buzzChainIdChanged();
	void q_buzzerIdChanged();
	void q_buzzerInfoIdChanged();
	void q_buzzerInfoChainIdChanged();
	void q_buzzBodyChanged();

private:
	BuzzfeedItemPtr item_;
};

//
class BuzzfeedItemProxy: public QObject {
	Q_OBJECT

public:
	BuzzfeedItemProxy() {}
	BuzzfeedItemProxy(BuzzfeedItemPtr item): item_(item) {
	}
	BuzzfeedItemProxy(const BuzzfeedItemProxy& other) {
		set(other.get());
	}

	void set(BuzzfeedItemPtr item) { item_ = item; }
	BuzzfeedItemPtr get() const { return item_; }

private:
	BuzzfeedItemPtr item_;
};

//
class BuzzfeedProxy: public QObject {
	Q_OBJECT

public:
	BuzzfeedProxy() {}
	BuzzfeedProxy(BuzzfeedPtr item): item_(item) {
	}
	BuzzfeedProxy(const BuzzfeedProxy& other) {
		set(other.get());
	}

	void set(BuzzfeedPtr item) { item_ = item; }
	BuzzfeedPtr get() const { return item_; }

private:
	BuzzfeedPtr item_;
};

//
class BuzzfeedItemUpdatesProxy: public QObject {
	Q_OBJECT

public:
	BuzzfeedItemUpdatesProxy() {}
	BuzzfeedItemUpdatesProxy(const std::vector<qbit::BuzzfeedItemUpdate>& item): item_(item) {
	}
	BuzzfeedItemUpdatesProxy(const BuzzfeedItemUpdatesProxy& other) {
		set(other.get());
	}

	void set(const std::vector<qbit::BuzzfeedItemUpdate>& item) { item_ = item; }
	const std::vector<qbit::BuzzfeedItemUpdate>& get() const { return item_; }

private:
	std::vector<qbit::BuzzfeedItemUpdate> item_;
};

#endif

} // qbit

#endif
