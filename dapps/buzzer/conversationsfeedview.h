// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CONVERSATIONS_FEED_VIEW_H
#define QBIT_CONVERSATIONS_FEED_VIEW_H

#include "conversationsfeed.h"
#include "buzzfeedview.h"

#if defined (CLIENT_PLATFORM)
	#include <QObject>
	#include <QString>
	#include <QQmlListProperty>
#endif

namespace qbit {

#if defined (CLIENT_PLATFORM)

class QConversationEventInfo;
typedef std::shared_ptr<QConversationEventInfo> QConversationEventInfoPtr;

class QConversationEventInfo: public QObject, public ConversationItem::EventInfo {
	Q_OBJECT

	Q_PROPERTY(unsigned short type READ q_type NOTIFY q_typeChanged)
	Q_PROPERTY(unsigned long long timestamp READ q_timestamp NOTIFY q_timestampChanged)
	Q_PROPERTY(unsigned long long score READ q_score NOTIFY q_scoreChanged)
	Q_PROPERTY(QString conversationId READ q_conversationId NOTIFY q_conversationIdChanged)
	Q_PROPERTY(QString eventId READ q_eventId NOTIFY q_eventIdChanged)
	Q_PROPERTY(QString eventChainId READ q_eventChainId NOTIFY q_eventChainIdChanged)
	Q_PROPERTY(QString buzzerId READ q_buzzerId NOTIFY q_buzzerIdChanged)
	Q_PROPERTY(QString buzzerInfoId READ q_buzzerInfoId NOTIFY q_buzzerInfoIdChanged)
	Q_PROPERTY(QString buzzerInfoChainId READ q_buzzerInfoChainId NOTIFY q_buzzerInfoChainIdChanged)
	Q_PROPERTY(QString body READ q_body NOTIFY q_bodyChanged)
	Q_PROPERTY(QString decryptedBody READ q_decryptedBody NOTIFY q_decryptedBodyChanged)

public:
	QConversationEventInfo() {}
	QConversationEventInfo(const ConversationItem::EventInfo& other) {
		assign(other);
	}

	void assign(const ConversationItem::EventInfo& info) {
		type_ = info.type();
		timestamp_ = info.timestamp();
		score_ = info.score();
		conversationId_ = info.conversationId();
		eventId_ = info.eventId();
		eventChainId_ = info.eventChainId();
		buzzerId_ = info.buzzerId();
		buzzerInfoId_ = info.buzzerInfoId();
		buzzerInfoChainId_ = info.buzzerInfoChainId();
		body_ = info.body();
		decryptedBody_ = const_cast<ConversationItem::EventInfo&>(info).decryptedBody();
		signature_ = info.signature();
	}

	static QConversationEventInfoPtr instance() {
		return std::make_shared<QConversationEventInfo>();
	}

	static QConversationEventInfoPtr instance(const ConversationItem::EventInfo& info) {
		return std::make_shared<QConversationEventInfo>(info);
	}

public:
	unsigned short q_type() { return type_; }
	unsigned long long q_timestamp() { return timestamp_; }
	unsigned long long q_score() { return score_; }
	QString q_conversationId() { return QString::fromStdString(conversationId_.toHex()); }
	QString q_eventId() { return QString::fromStdString(eventId_.toHex()); }
	QString q_eventChainId() { return QString::fromStdString(eventChainId_.toHex()); }
	QString q_buzzerId() { return QString::fromStdString(buzzerId_.toHex()); }
	QString q_buzzerInfoId() { return QString::fromStdString(buzzerInfoId_.toHex()); }
	QString q_buzzerInfoChainId() { return QString::fromStdString(buzzerInfoChainId_.toHex()); }
	QString q_body() {
		std::string lBody; lBody.insert(lBody.end(), body_.begin(), body_.end());
		return QString::fromStdString(lBody); 
	}
	QString q_decryptedBody() {
		std::string lBody; lBody.insert(lBody.end(), decryptedBody_.begin(), decryptedBody_.end());
		return QString::fromStdString(lBody); 
	}

signals:
	void q_typeChanged();
	void q_timestampChanged();
	void q_scoreChanged();
	void q_conversationIdChanged();
	void q_eventIdChanged();
	void q_eventChainIdChanged();
	void q_buzzerIdChanged();
	void q_buzzerInfoIdChanged();
	void q_buzzerInfoChainIdChanged();
	void q_bodyChanged();
	void q_decryptedBodyChanged();
};

class QConversationItem: public QObject {
	Q_OBJECT

	Q_PROPERTY(unsigned long long timestamp READ q_timestamp NOTIFY q_timestampChanged)
	Q_PROPERTY(unsigned long long score READ q_score NOTIFY q_scoreChanged)
	Q_PROPERTY(unsigned short side READ q_side NOTIFY q_sideChanged)
	Q_PROPERTY(QString conversationId READ q_conversationId NOTIFY q_conversationIdChanged)
	Q_PROPERTY(QString creatorId READ q_creatorId NOTIFY q_creatorIdChanged)
	Q_PROPERTY(QString creatorInfoId READ q_creatorInfoId NOTIFY q_creatorInfoIdChanged)
	Q_PROPERTY(QString creatorInfoChainId READ q_creatorInfoChainId NOTIFY q_creatorInfoChainIdChanged)
	Q_PROPERTY(QString counterpartyId READ q_counterpartyId NOTIFY q_counterpartyIdChanged)
	Q_PROPERTY(QString counterpartyInfoId READ q_counterpartyInfoId NOTIFY q_counterpartyInfoIdChanged)
	Q_PROPERTY(QString counterpartyInfoChainId READ q_counterpartyInfoChainId NOTIFY q_counterpartyInfoChainIdChanged)
	Q_PROPERTY(QList<qbit::QConversationEventInfo*> eventInfos READ q_eventInfos NOTIFY q_eventInfosChanged)

public:
	QConversationItem() {}
	QConversationItem(ConversationItemPtr item) {
		item_ = item;
	}
	~QConversationItem() { item_.reset(); }

public:
	unsigned long long q_timestamp() { return item_->timestamp(); }
	unsigned long long q_score() { return item_->score(); }
	unsigned short q_side() { return item_->side(); }

	QString q_conversationId() { return QString::fromStdString(item_->conversationId().toHex()); }
	QString q_creatorId() { return QString::fromStdString(item_->creatorId().toHex()); }
	QString q_creatorInfoId() { return QString::fromStdString(item_->creatorInfoId().toHex()); }
	QString q_creatorInfoChainId() { return QString::fromStdString(item_->creatorInfoChainId().toHex()); }

	QString q_counterpartyId() { return QString::fromStdString(item_->counterpartyId().toHex()); }
	QString q_counterpartyInfoId() { return QString::fromStdString(item_->counterpartyInfoId().toHex()); }
	QString q_counterpartyInfoChainId() { return QString::fromStdString(item_->counterpartyInfoChainId().toHex()); }

	QList<qbit::QConversationEventInfo*> q_eventInfos() {
		//
		QList<qbit::QConversationEventInfo*> lEvents;

		for (std::vector<qbit::ConversationItem::EventInfo>::const_iterator lPointer = item_->events().begin(); lPointer != item_->events().end(); lPointer++) {
			lEvents.append(new qbit::QConversationEventInfo(*lPointer));
		}

		return lEvents;
	}

signals:
	void q_timestampChanged();
	void q_scoreChanged();
	void q_sideChanged();
	void q_stateChanged();
	void q_conversationIdChanged();
	void q_creatorIdChanged();
	void q_creatorInfoIdChanged();
	void q_creatorInfoChainIdChanged();
	void q_counterpartyIdChanged();
	void q_counterpartyInfoIdChanged();
	void q_counterpartyInfoChainIdChanged();
	void q_eventInfosChanged();

private:
	qbit::ConversationItemPtr item_;
};

//
class ConversationItemProxy: public QObject {
	Q_OBJECT

public:
	ConversationItemProxy() {}
	ConversationItemProxy(ConversationItemPtr item): item_(item) {
	}
	ConversationItemProxy(const ConversationItemProxy& other) {
		set(other.get());
	}

	void set(ConversationItemPtr item) { item_ = item; }
	ConversationItemPtr get() const { return item_; }

private:
	ConversationItemPtr item_;
};

//
class ConversationsfeedProxy: public QObject {
	Q_OBJECT

public:
	ConversationsfeedProxy() {}
	ConversationsfeedProxy(ConversationsfeedPtr item): item_(item) {
	}
	ConversationsfeedProxy(const ConversationsfeedProxy& other) {
		set(other.get());
	}

	void set(ConversationsfeedPtr item) { item_ = item; }
	ConversationsfeedPtr get() const { return item_; }

private:
	ConversationsfeedPtr item_;
};

#endif

} // qbit

#endif
