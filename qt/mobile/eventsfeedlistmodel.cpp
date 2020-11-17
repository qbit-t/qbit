#include "eventsfeedlistmodel.h"
#include "dapps/buzzer/eventsfeedview.h"

#include "client.h"
#include "iapplication.h"

#include "transaction.h"
#include "dapps/buzzer/txbuzzer.h"

using namespace buzzer;

EventsfeedListModel::EventsfeedListModel() {
	//
	connect(this, SIGNAL(eventsfeedItemNewSignal(const qbit::EventsfeedItemProxy&)), this, SLOT(eventsfeedItemNewSlot(const qbit::EventsfeedItemProxy&)));
}

int EventsfeedListModel::rowCount(const QModelIndex& parent) const {
	Q_UNUSED(parent);
	return list_.size();
}

QVariant EventsfeedListModel::self(int index) const {
	if (index < 0 || index >= (int)list_.size()) {
		return QVariant();
	}

	qbit::EventsfeedItemPtr lItem = list_[index];
	return QVariant::fromValue(new qbit::QEventsfeedItem(lItem));
}

QVariant EventsfeedListModel::data(const QModelIndex& index, int role) const {
	//
	if (index.row() < 0 || index.row() >= (int)list_.size()) {
		return QVariant();
	}

	qbit::EventsfeedItemPtr lItem = list_[index.row()];
	if (role == TypeRole) {
		return lItem->type();
	} else if (role == BuzzIdRole) {
		return QString::fromStdString(lItem->buzzId().toHex());
	} else if (role == BuzzChainIdRole) {
		return QString::fromStdString(lItem->buzzChainId().toHex());
	} else if (role == TimestampRole) {
		return QVariant::fromValue(lItem->timestamp());
	} else if (role == AgoRole) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());
		return lClient->timestampAgo(lItem->timestamp());
	} else if (role == LocalDateTimeRole) {
		return QString::fromStdString(qbit::formatLocalDateTime(lItem->timestamp() / 1000000));
	} else if (role == ScoreRole) {
		return QVariant::fromValue(lItem->score());
	} else if (role == PublisherIdRole) {
		return QString::fromStdString(lItem->publisher().toHex());
	} else if (role == PublisherInfoIdRole) {
		return QString::fromStdString(lItem->publisherInfo().toHex());
	} else if (role == PublisherInfoChainIdRole) {
		return QString::fromStdString(lItem->publisherInfoChain().toHex());
	} else if (role == BuzzBodyRole) {
		// decorate @buzzers, #tags and urls
		Client* lClient = static_cast<Client*>(gApplication->getClient());
		return lClient->decorateBuzzBody(QString::fromStdString(lItem->buzzBodyString()));
	} else if (role == ValueRole) {
		return QVariant::fromValue(lItem->value());
	} else if (role == BuzzMediaRole) {
		QList<qbit::QBuzzerMediaPointer*> lMediaPointers;

		for (std::vector<qbit::BuzzerMediaPointer>::const_iterator lPointer = lItem->buzzMedia().begin(); lPointer != lItem->buzzMedia().end(); lPointer++) {
			lMediaPointers.append(new qbit::QBuzzerMediaPointer(lPointer->chain(), lPointer->tx()));
		}

		return QVariant::fromValue(lMediaPointers);
	} else if (role == EventInfosRoles) {
		QList<qbit::QEventInfo*> lInfos;

		for (std::vector<qbit::EventsfeedItem::EventInfo>::const_iterator lItemInfo = lItem->buzzers().begin(); lItemInfo != lItem->buzzers().end(); lItemInfo++) {
			lInfos.append(new qbit::QEventInfo(*lItemInfo));
		}

		return QVariant::fromValue(lInfos);
	} else if (role == SelfRole) {
		return QVariant::fromValue(new qbit::QEventsfeedItem(lItem));
	} else if (role == MentionRole) {
		return lItem->isMentioned();
	} else if (role == LastUrlRole) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());
		return lClient->extractLastUrl(QString::fromStdString(lItem->buzzBodyString()));
	}

	return QVariant();
}

QHash<int, QByteArray> EventsfeedListModel::roleNames() const {
	//
	QHash<int, QByteArray> lRoles;

	lRoles[TypeRole] = "type";
	lRoles[TimestampRole] = "timestamp";
	lRoles[AgoRole] = "ago";
	lRoles[LocalDateTimeRole] = "localDateTime";
	lRoles[ScoreRole] = "score";
	lRoles[BuzzIdRole] = "buzzId";
	lRoles[BuzzChainIdRole] = "buzzChainId";
	//lRoles[PublisherNameRole] = "publisherName";
	//lRoles[PublisherAliasRole] = "publisherAlias";
	lRoles[PublisherIdRole] = "publisherId";
	lRoles[PublisherInfoIdRole] = "publisherInfoId";
	lRoles[PublisherInfoChainIdRole] = "publisherInfoChainId";
	lRoles[BuzzBodyRole] = "buzzBody";
	lRoles[BuzzMediaRole] = "buzzMedia";
	lRoles[LastUrlRole] = "lastUrl";
	lRoles[EventInfosRoles] = "eventInfos";
	lRoles[MentionRole] = "mention";
	lRoles[ValueRole] = "value";
	lRoles[SelfRole] = "self";

	return lRoles;
}

void EventsfeedListModel::eventsfeedLargeUpdated() {
	//
}

void EventsfeedListModel::eventsfeedItemNew(qbit::EventsfeedItemPtr event) {
	qInfo() << "EventsfeedListModel::eventsfeedItemNew";
	emit eventsfeedItemNewSignal(qbit::EventsfeedItemProxy(event));
}

void EventsfeedListModel::eventsfeedItemUpdated(qbit::EventsfeedItemPtr) {
}

void EventsfeedListModel::eventsfeedItemsUpdated(const std::vector<qbit::EventsfeedItem>&) {
}

void EventsfeedListModel::feed(qbit::EventsfeedPtr local, bool more) {
	//
	if (!more) {
		//
		qInfo() << "==== START =====";

		beginResetModel();

		// get ordered list
		list_.clear();
		index_.clear();
		eventsfeed_->feed(list_);

		noMoreData_ = false;

		// make index
		for (int lItem = 0; lItem < (int)list_.size(); lItem++) {
			index_[list_[lItem]->key()] = lItem;
			//qInfo() << "Initial: " << QString::fromStdString(list_[lItem]->key().toString()) << QString::fromStdString(list_[lItem]->toString());
		}

		//
		endResetModel();

		//
		emit countChanged();
		if (list_.size()) {
			emit eventsfeedUpdated(list_[0]->timestamp());
		}
	} else if (!noMoreData_) {
		//
		qInfo() << "==== MORE =====";

		// get 'more' feed
		std::vector<qbit::EventsfeedItemPtr> lAppendFeed;
		local->feed(lAppendFeed);

		qInfo() << "local.count = " << lAppendFeed.size();

		// merge base and 'more'
		bool lItemsAdded = eventsfeed_->mergeAppend(lAppendFeed);
		qInfo() << "existing.count = " << list_.size();

		/*
		if (lRemoveTop) {
			beginRemoveRows(QModelIndex(), 0, lRemoveTop);
			endRemoveRows();
		}
		*/

		//
		if (lItemsAdded) {
			//
			int lFrom = list_.size();

			// index for check
			std::map<qbit::EventsfeedItem::Key, int> lOldIndex = index_;

			// new feed
			list_.clear();
			index_.clear();
			eventsfeed_->feed(list_);

			// make index
			for (int lItem = 0; lItem < (int)list_.size(); lItem++) {
				index_[list_[lItem]->key()] = lItem;
				//qInfo() << "More: " << QString::fromStdString(list_[lItem]->key().toString()) << QString::fromStdString(list_[lItem]->toString());
			}

			qInfo() << "new.count = " << list_.size();

			if ((int)list_.size() == lFrom) {
				noMoreData_ = true;
			} else {
				if (lFrom >= (int)list_.size()) {
					for (std::map<qbit::EventsfeedItem::Key, int>::iterator lIdx = lOldIndex.begin(); lIdx != lOldIndex.end(); lIdx++) {
						//
						if (index_.find(lIdx->first) == index_.end()) {
							beginRemoveRows(QModelIndex(), lIdx->second, lIdx->second);
							qInfo() << "Removed: " << QString::fromStdString(lIdx->first.toString());
							endRemoveRows();
						}
					}

				} else if (list_.size() && lFrom <= (int)list_.size() - 1) {
					beginInsertRows(QModelIndex(), lFrom, list_.size() - 1);
					endInsertRows();
				} else {
					beginResetModel();
					endResetModel();
				}

				//
				QModelIndex lStartIndex = createIndex(0, 0);
				QModelIndex lEndIndex = createIndex(list_.size()-1, list_.size()-1);
				emit dataChanged(lStartIndex, lEndIndex, QVector<int>() << TimestampRole);

				//
				emit countChanged();
			}
		} else {
			noMoreData_ = true;
		}
	}
}

void EventsfeedListModel::feedSlot(const qbit::EventsfeedProxy& local, bool more) {
	//
	qbit::EventsfeedPtr lEventsfeed(local.get());
	feed(lEventsfeed, more);
}

void EventsfeedListModel::eventsfeedItemNewSlot(const qbit::EventsfeedItemProxy& event) {
	//
	qbit::EventsfeedItemPtr lEventsfeedItem(event.get());
	int lIndex = eventsfeed_->locateIndex(lEventsfeedItem);
	if (lIndex != -1 /*&& list_.begin() != list_.end()*/ /*lIndex <= list_.size()*/) {
		//
		if (index_.find(lEventsfeedItem->key()) == index_.end()) {
			qInfo() << "EventsfeedListModel::eventsfeedItemNewSlot -> insert" << lIndex;

			if (list_.begin() != list_.end() && lIndex <= (int)list_.size())
				list_.insert(list_.begin() + lIndex, lEventsfeedItem);
			else
				list_.push_back(lEventsfeedItem);

			index_[lEventsfeedItem->key()] = lIndex;
			beginInsertRows(QModelIndex(), lIndex, lIndex);
			endInsertRows();

			emit countChanged();
			emit eventsfeedUpdated(lEventsfeedItem->timestamp());
		} else {
			qInfo() << "EventsfeedListModel::eventsfeedItemNewSlot -> update" << lIndex;
			QModelIndex lModelIndex = createIndex(lIndex, lIndex);
			emit dataChanged(lModelIndex, lModelIndex, QVector<int>() << TimestampRole << EventInfosRoles);
		}
	}
}

EventsfeedListModelPersonal::EventsfeedListModelPersonal() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	eventsfeed_ = qbit::Eventsfeed::instance(lClient->getBuzzer(),
		boost::bind(&qbit::BuzzerLightComposer::verifyEventPublisher, lClient->getBuzzerComposer(), _1),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherStrict, lClient->getBuzzerComposer(), _1),
		boost::bind(&EventsfeedListModel::eventsfeedLargeUpdated, this),
		boost::bind(&EventsfeedListModel::eventsfeedItemNew, this, _1),
		boost::bind(&EventsfeedListModel::eventsfeedItemUpdated, this, _1),
		boost::bind(&EventsfeedListModel::eventsfeedItemsUpdated, this, _1),
		qbit::EventsfeedItem::Merge::UNION
	);

	eventsfeed_->prepare();

	QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

EventsfeedListModelEndorsements::EventsfeedListModelEndorsements() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	eventsfeed_ = qbit::Eventsfeed::instance(lClient->getBuzzer(),
		boost::bind(&qbit::BuzzerLightComposer::verifyEventPublisher, lClient->getBuzzerComposer(), _1),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherLazy, lClient->getBuzzerComposer(), _1),
		boost::bind(&EventsfeedListModel::eventsfeedLargeUpdated, this),
		boost::bind(&EventsfeedListModel::eventsfeedItemNew, this, _1),
		boost::bind(&EventsfeedListModel::eventsfeedItemUpdated, this, _1),
		boost::bind(&EventsfeedListModel::eventsfeedItemsUpdated, this, _1),
		qbit::EventsfeedItem::Merge::INTERSECT
	);

	eventsfeed_->prepare();
}

EventsfeedListModelMistrusts::EventsfeedListModelMistrusts() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	eventsfeed_ = qbit::Eventsfeed::instance(lClient->getBuzzer(),
		boost::bind(&qbit::BuzzerLightComposer::verifyEventPublisher, lClient->getBuzzerComposer(), _1),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherLazy, lClient->getBuzzerComposer(), _1),
		boost::bind(&EventsfeedListModel::eventsfeedLargeUpdated, this),
		boost::bind(&EventsfeedListModel::eventsfeedItemNew, this, _1),
		boost::bind(&EventsfeedListModel::eventsfeedItemUpdated, this, _1),
		boost::bind(&EventsfeedListModel::eventsfeedItemsUpdated, this, _1),
		qbit::EventsfeedItem::Merge::INTERSECT
	);

	eventsfeed_->prepare();
}

EventsfeedListModelFollowing::EventsfeedListModelFollowing() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	eventsfeed_ = qbit::Eventsfeed::instance(lClient->getBuzzer(),
		boost::bind(&qbit::BuzzerLightComposer::verifyEventPublisher, lClient->getBuzzerComposer(), _1),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherLazy, lClient->getBuzzerComposer(), _1),
		boost::bind(&EventsfeedListModel::eventsfeedLargeUpdated, this),
		boost::bind(&EventsfeedListModel::eventsfeedItemNew, this, _1),
		boost::bind(&EventsfeedListModel::eventsfeedItemUpdated, this, _1),
		boost::bind(&EventsfeedListModel::eventsfeedItemsUpdated, this, _1),
		qbit::EventsfeedItem::Merge::INTERSECT
	);

	eventsfeed_->prepare();
}

EventsfeedListModelFollowers::EventsfeedListModelFollowers() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	eventsfeed_ = qbit::Eventsfeed::instance(lClient->getBuzzer(),
		boost::bind(&qbit::BuzzerLightComposer::verifyEventPublisher, lClient->getBuzzerComposer(), _1),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherLazy, lClient->getBuzzerComposer(), _1),
		boost::bind(&EventsfeedListModel::eventsfeedLargeUpdated, this),
		boost::bind(&EventsfeedListModel::eventsfeedItemNew, this, _1),
		boost::bind(&EventsfeedListModel::eventsfeedItemUpdated, this, _1),
		boost::bind(&EventsfeedListModel::eventsfeedItemsUpdated, this, _1),
		qbit::EventsfeedItem::Merge::INTERSECT
	);

	eventsfeed_->prepare();
}
