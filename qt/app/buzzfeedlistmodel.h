#ifndef BUZZFEED_LIST_MODEL_H
#define BUZZFEED_LIST_MODEL_H

#include <QString>
#include <QList>
#include <QAbstractListModel>
#include <QQuickItem>
#include <QMap>
#include <QQmlApplicationEngine>

#include "dapps/buzzer/buzzfeed.h"
#include "dapps/buzzer/eventsfeed.h"

#include "dapps/buzzer/buzzfeedview.h"

namespace buzzer {

class BuzzfeedListModel : public QAbstractListModel {
	Q_OBJECT

	Q_PROPERTY(ulong count READ count NOTIFY countChanged)
	Q_PROPERTY(bool noMoreData READ noMoreData NOTIFY noMoreDataChanged)
	Q_PROPERTY(QString rootId READ rootId NOTIFY rootIdChanged)

public:
	enum BuzzfeedItemRoles {
		TypeRole = Qt::UserRole + 1,
		BuzzIdRole,
		BuzzChainIdRole,
		TimestampRole,
		AgoRole,
		LocalDateTimeRole,
		ScoreRole,
		BuzzerIdRole,
		BuzzerInfoIdRole,
		BuzzerInfoChainIdRole,
		BuzzBodyRole,
		BuzzBodyFlatRole,
		BuzzBodyLimitedRole,
		BuzzMediaRole,
		RepliesRole,
		RebuzzesRole,
		LikesRole,
		RewardsRole,
		OriginalBuzzIdRole,
		OriginalBuzzChainIdRole,
		HasParentRole,
		ChildrenCountRole,
		HasPrevSiblingRole,
		HasNextSiblingRole,
		PrevSiblingIdRole,
		NextSiblingIdRole,
		FirstChildIdRole,
		ValueRole,
		BuzzInfosRole,
		BuzzerNameRole,
		BuzzerAliasRole,
		LastUrlRole,
		WrappedRole,
		SelfRole,
		ThreadedRole,
		HasPrevLinkRole,
		HasNextLinkRole,
		OnChainRole,
		DynamicRole,
		LocalDynamicRole,
		FeedingRole,
		OwnLikeRole,
		OwnRebuzzRole,
		AdjustDataRole,
		IsEmojiRole,
		BlockedRole
	};

public:
	BuzzfeedListModel();
	virtual ~BuzzfeedListModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

	Q_INVOKABLE bool hasPrevLink(int index) const;
	Q_INVOKABLE bool hasNextLink(int index) const;
	Q_INVOKABLE QString buzzId(int index) const;
	Q_INVOKABLE QString buzzerId(int index) const;
	Q_INVOKABLE QVariant self(int index) const;
	Q_INVOKABLE int childrenCount(int index) const;
	//
	Q_INVOKABLE void setOnChain(int index);
	Q_INVOKABLE void setHasLike(int index);
	Q_INVOKABLE void setHasRebuzz(int index);
	Q_INVOKABLE void resetLocalDynamic(int index);
	//
	Q_INVOKABLE int locateIndex(QString key);
	Q_INVOKABLE bool feeding() { return feeding_; }
	Q_INVOKABLE QString itemToString(int index);
	Q_INVOKABLE void remove(int index);
	Q_INVOKABLE void forceRelayout(int index, int count);
	Q_INVOKABLE void markBlocked(const QString&);
	Q_INVOKABLE void removeBlocked();

	void buzzfeedLargeUpdated();
	void buzzfeedItemNew(qbit::BuzzfeedItemPtr /*buzz*/);
	void buzzfeedItemUpdated(qbit::BuzzfeedItemPtr /*buzz*/);
	void buzzfeedItemsUpdated(const std::vector<qbit::BuzzfeedItemUpdate>& /*items*/);
	void buzzfeedItemAbsent(const uint256& /*chain*/, const uint256& /*buzz*/);

	void feed(qbit::BuzzfeedPtr /*local*/, bool /*more*/, bool /*merge*/);
	Q_INVOKABLE void merge();

	Q_INVOKABLE bool readyToFeed(int /*position*/, int /*threshold*/);

	qbit::BuzzfeedPtr buzzfeed() { return buzzfeed_; }

	Q_INVOKABLE void clear() {
		if (buzzfeed_) buzzfeed_->clear();
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

	QString rootId() {
		if (rootId_.isEmpty()) return "";
		return QString::fromStdString(rootId_.toHex());
	}

	void setRootId(const uint256& rootId) {
		rootId_ = rootId;
		emit rootIdChanged();
	}

public slots:
	void feedSlot(const qbit::BuzzfeedProxy& local, bool more, bool merge);
	void buzzfeedItemNewSlot(const qbit::BuzzfeedItemProxy& buzz);
	void buzzfeedItemUpdatedSlot(const qbit::BuzzfeedItemProxy& buzz);
	void buzzfeedItemsUpdatedSlot(const qbit::BuzzfeedItemUpdatesProxy& items);

signals:
	void countChanged();
	void noMoreDataChanged();
	void rootIdChanged();
	void buzzfeedItemNewSignal(const qbit::BuzzfeedItemProxy& buzz);
	void buzzfeedItemUpdatedSignal(const qbit::BuzzfeedItemProxy& buzz);
	void buzzfeedItemsUpdatedSignal(const qbit::BuzzfeedItemUpdatesProxy& items);

protected:
	QHash<int, QByteArray> roleNames() const;

private:
	bool buzzfeedItemsUpdatedProcess(const qbit::BuzzfeedItemUpdate&, unsigned short);
	bool buzzfeedItemUpdatedProcess(qbit::BuzzfeedItemPtr, unsigned short);
	bool buzzfeedItemRemoveProcess(qbit::BuzzfeedItemPtr, unsigned short);

protected:
	qbit::BuzzfeedPtr buzzfeed_;
	std::vector<qbit::BuzzfeedItemPtr> list_;
	std::vector<bool> adjustData_;
	std::map<qbit::BuzzfeedItem::Key, int> index_;
	bool noMoreData_ = false;
	uint256 rootId_;
	bool feeding_ = false;
};

class BuzzfeedListModelPersonal: public BuzzfeedListModel {
public:
	BuzzfeedListModelPersonal();
};

class BuzzfeedListModelGlobal: public BuzzfeedListModel {
public:
	BuzzfeedListModelGlobal();
};

class BuzzfeedListModelBuzzer: public BuzzfeedListModel {
public:
	BuzzfeedListModelBuzzer();
};

class BuzzfeedListModelBuzzes: public BuzzfeedListModel {
public:
	BuzzfeedListModelBuzzes();
};

class BuzzfeedListModelTag: public BuzzfeedListModel {
public:
	BuzzfeedListModelTag();
};

class ConversationMessagesList: public BuzzfeedListModel {
public:
	ConversationMessagesList();
};

}

#endif // BUZZFEED_LIST_MODEL_H
