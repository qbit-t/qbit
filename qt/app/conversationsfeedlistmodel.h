#ifndef CONVERSATIONSFEED_LIST_MODEL_H
#define CONVERSATIONSFEED_LIST_MODEL_H

#include <QString>
#include <QList>
#include <QAbstractListModel>
#include <QQuickItem>
#include <QMap>
#include <QQmlApplicationEngine>

#include "dapps/buzzer/buzzfeed.h"
#include "dapps/buzzer/conversationsfeed.h"

#include "dapps/buzzer/conversationsfeedview.h"

namespace buzzer {

class ConversationsfeedListModel : public QAbstractListModel {
	Q_OBJECT

	Q_PROPERTY(ulong count READ count NOTIFY countChanged)
	Q_PROPERTY(bool noMoreData READ noMoreData NOTIFY noMoreDataChanged)

public:
	enum ConversationItemRoles {
		TimestampRole = Qt::UserRole + 1,
		AgoRole,
		ScoreRole,
		SideRole,
		LocalDateTimeRole,
		ConversationIdRole,
		ConversationChainIdRole,
		CreatorIdRole,
		CreatorInfoIdRole,
		CreatorInfoChainIdRole,
		CounterpartyIdRole,
		CounterpartyInfoIdRole,
		CounterpartyInfoChainIdRole,
		EventsRole,
		SelfRole,
		OnChainRole,
		DynamicRole
	};

public:
	ConversationsfeedListModel();
	virtual ~ConversationsfeedListModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

	Q_INVOKABLE void setOnChain(int index);
	Q_INVOKABLE QString locateConversation(QString);
	Q_INVOKABLE void setFilter(QString);
	Q_INVOKABLE void resetFilter();

	void conversationsLargeUpdated();
	void conversationItemNew(qbit::ConversationItemPtr /*conversation*/);
	void conversationItemUpdated(qbit::ConversationItemPtr /*conversation*/);
	void conversationsfeedItemsUpdated(const std::vector<qbit::ConversationItem>& /*items*/) {}

	void feed(qbit::ConversationsfeedPtr /*local*/, bool /*more*/, bool /*merge*/);
	void merge();

	qbit::ConversationsfeedPtr conversations() {
		return conversations_;
	}

	Q_INVOKABLE void clear() {
		if (conversations_) conversations_->clear();
		list_.clear();
		index_.clear();
	}

	ulong count() { return list_.size(); }
	bool noMoreData() { return noMoreData_; }

	// TODO: reconsider to use this - model resetted and view as well
	// TODO: need to set width for the visible items in the list
	Q_INVOKABLE void resetModel() {
		beginResetModel();
		endResetModel();
	}

public slots:
	void feedSlot(const qbit::ConversationsfeedProxy& local, bool more, bool merge);
	void conversationItemNewSlot(const qbit::ConversationItemProxy& buzz);
	void conversationItemUpdatedSlot(const qbit::ConversationItemProxy& buzz);

signals:
	void countChanged();
	void noMoreDataChanged();
	void rootIdChanged();
	void conversationItemNewSignal(const qbit::ConversationItemProxy& buzz);
	void conversationItemUpdatedSignal(const qbit::ConversationItemProxy& buzz);
	void conversationsUpdated(const qbit::ConversationItemProxy&);

protected:
	QHash<int, QByteArray> roleNames() const;

protected:
	qbit::ConversationsfeedPtr conversations_;
	std::vector<qbit::ConversationItemPtr> list_;
	std::map<uint256, int> index_;
	bool noMoreData_ = false;

	bool filtered_ = false;
	std::vector<qbit::ConversationItemPtr> filteredList_;
	std::map<uint256, int> filteredIndex_;
};

class ConversationsListModel: public ConversationsfeedListModel {
public:
	ConversationsListModel();
};

}

#endif // CONVERSATIONSFEED_LIST_MODEL_H
