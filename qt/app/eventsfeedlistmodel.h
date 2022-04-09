#ifndef EVENTSFEED_LIST_MODEL_H
#define EVENTSFEED_LIST_MODEL_H

#include <QString>
#include <QList>
#include <QAbstractListModel>
#include <QQuickItem>
#include <QMap>
#include <QQmlApplicationEngine>

#include "dapps/buzzer/eventsfeed.h"
#include "dapps/buzzer/eventsfeedview.h"

namespace buzzer {

class EventsfeedListModel: public QAbstractListModel {
	Q_OBJECT

	Q_PROPERTY(ulong count READ count NOTIFY countChanged)
	Q_PROPERTY(bool noMoreData READ noMoreData NOTIFY noMoreDataChanged)

public:
	enum EventsfeedItemRoles {
		TypeRole = Qt::UserRole + 1,
		TimestampRole,
		AgoRole,
		LocalDateTimeRole,
		ScoreRole,
		BuzzIdRole,
		BuzzChainIdRole,
		//PublisherNameRole,
		//PublisherAliasRole,
		PublisherIdRole,
		PublisherInfoIdRole,
		PublisherInfoChainIdRole,
		BuzzBodyRole,
		BuzzBodyFlatRole,
		BuzzMediaRole,
		LastUrlRole,
		EventInfosRoles,
		MentionRole,
		ValueRole,
		SelfRole
	};

public:
	EventsfeedListModel();
	virtual ~EventsfeedListModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

	Q_INVOKABLE QVariant self(int index) const;

	void eventsfeedLargeUpdated();
	void eventsfeedItemNew(qbit::EventsfeedItemPtr /*buzz*/);
	void eventsfeedItemUpdated(qbit::EventsfeedItemPtr /*buzz*/);
	void eventsfeedItemsUpdated(const std::vector<qbit::EventsfeedItem>& /*items*/);

	void feed(qbit::EventsfeedPtr /*local*/, bool /*more*/, bool /*merge*/);
	void merge();

	qbit::EventsfeedPtr eventsfeed() { return eventsfeed_; }

	Q_INVOKABLE void clear() {
		if (eventsfeed_) eventsfeed_->clear();
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
	void feedSlot(const qbit::EventsfeedProxy& local, bool more, bool merge);
	void eventsfeedItemNewSlot(const qbit::EventsfeedItemProxy& buzz);

signals:
	void countChanged();
	void noMoreDataChanged();
	void eventsfeedItemNewSignal(const qbit::EventsfeedItemProxy& buzz);
	void eventsfeedUpdated(const qbit::EventsfeedItemProxy& event);

protected:
	QHash<int, QByteArray> roleNames() const;

private:
	bool eventsfeedItemUpdatedProcess(qbit::EventsfeedItemPtr, unsigned short);

protected:
	qbit::EventsfeedPtr eventsfeed_;
	std::vector<qbit::EventsfeedItemPtr> list_;
	std::map<qbit::EventsfeedItem::Key, int> index_;
	bool noMoreData_ = false;
};

class EventsfeedListModelPersonal: public EventsfeedListModel {
public:
	EventsfeedListModelPersonal();
};

class EventsfeedListModelEndorsements: public EventsfeedListModel {
public:
	EventsfeedListModelEndorsements();
};

class EventsfeedListModelMistrusts: public EventsfeedListModel {
public:
	EventsfeedListModelMistrusts();
};

class EventsfeedListModelFollowing: public EventsfeedListModel {
public:
	EventsfeedListModelFollowing();
};

class EventsfeedListModelFollowers: public EventsfeedListModel {
public:
	EventsfeedListModelFollowers();
};

}

#endif // EVENTSFEED_LIST_MODEL_H
