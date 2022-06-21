#include "conversationsfeedlistmodel.h"

#include "client.h"
#include "iapplication.h"

#include "transaction.h"
#include "dapps/buzzer/txbuzzer.h"

using namespace buzzer;

ConversationsfeedListModel::ConversationsfeedListModel() {
	//
	connect(this, SIGNAL(conversationItemNewSignal(const qbit::ConversationItemProxy&)), this, SLOT(conversationItemNewSlot(const qbit::ConversationItemProxy&)));
	connect(this, SIGNAL(conversationItemUpdatedSignal(const qbit::ConversationItemProxy&)), this, SLOT(conversationItemUpdatedSlot(const qbit::ConversationItemProxy&)));
}

ConversationsfeedListModel::~ConversationsfeedListModel() {
	//
	disconnect(this, SIGNAL(conversationItemNewSignal(const qbit::ConversationItemProxy&)), 0, 0);
	disconnect(this, SIGNAL(conversationItemUpdatedSignal(const qbit::ConversationItemProxy&)), 0, 0);
}

int ConversationsfeedListModel::rowCount(const QModelIndex& parent) const {
	Q_UNUSED(parent);
	return !filtered_ ? list_.size() : filteredList_.size();
}

void ConversationsfeedListModel::setOnChain(int index) {
	if (index < 0 || index >= (int)list_.size()) {
		return;
	}

	qbit::ConversationItemPtr lItem = list_[index];
	lItem->setOnChain();

	QModelIndex lModelIndex = createIndex(index, index);
	emit dataChanged(lModelIndex, lModelIndex, QVector<int>() << OnChainRole);
}

QString ConversationsfeedListModel::locateConversation(QString name) {
	//
	uint256 lId;
	if (conversations_->locateBuzzer(name.toStdString(), lId)) {
		return QString::fromStdString(lId.toHex());
	}

	return QString();
}

void ConversationsfeedListModel::setFilter(QString name) {
	//
	std::vector<uint256> lIds;
	conversations_->locateBuzzerByPart(name.toStdString(), lIds);

	filteredList_.clear();
	filteredIndex_.clear();

	qInfo() << "ConversationsfeedListModel" << name << lIds.size();

	if (lIds.size()) {
		//
		for (std::vector<uint256>::iterator lConversation = lIds.begin(); lConversation != lIds.end(); lConversation++) {
			std::map<uint256, int>::iterator lIndex = index_.find(*lConversation);
			if (lIndex != index_.end() && lIndex->second < (int)list_.size()) {
				//
				filteredList_.push_back(list_[lIndex->second]);
			}
		}

		//
		for (int lIdx = 0; lIdx < (int)filteredList_.size(); lIdx++) {
			//
			filteredIndex_[filteredList_[lIdx]->key()] = lIdx;
		}
	}

	filtered_ = true;
	resetModel();
}

void ConversationsfeedListModel::resetFilter() {
	//
	filtered_ = false;
	filteredList_.clear();
	filteredIndex_.clear();
	resetModel();
}

QVariant ConversationsfeedListModel::data(const QModelIndex& index, int role) const {
	//
	if (index.row() < 0 ||
			(!filtered_ && index.row() >= (int)list_.size()) ||
			(filtered_ && index.row() >= (int)filteredList_.size())) {
		return QVariant();
	}

	qbit::ConversationItemPtr lItem;
	if (!filtered_) lItem = list_[index.row()];
	else lItem = filteredList_[index.row()];

	if (role == TimestampRole) {
		return QVariant::fromValue(lItem->timestamp());
	} else if (role == AgoRole) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());
		return lClient->timestampAgo(lItem->timestamp());
	} else if (role == LocalDateTimeRole) {
		return QString::fromStdString(qbit::formatLocalDateTime(lItem->timestamp() / 1000000));
	} else if (role == ScoreRole) {
		return QVariant::fromValue(lItem->score());
	} else if (role == SideRole) {
		return QVariant::fromValue((unsigned short)lItem->side());
	} else if (role == ConversationIdRole) {
		return QString::fromStdString(lItem->conversationId().toHex());
	} else if (role == ConversationChainIdRole) {
		return QString::fromStdString(lItem->conversationChainId().toHex());

	} else if (role == CreatorIdRole) {
		return QString::fromStdString(lItem->creatorId().toHex());
	} else if (role == CreatorInfoIdRole) {
		return QString::fromStdString(lItem->creatorInfoId().toHex());
	} else if (role == CreatorInfoChainIdRole) {
		return QString::fromStdString(lItem->creatorInfoChainId().toHex());

	} else if (role == CounterpartyIdRole) {
		return QString::fromStdString(lItem->counterpartyId().toHex());
	} else if (role == CounterpartyInfoIdRole) {
		return QString::fromStdString(lItem->counterpartyInfoId().toHex());
	} else if (role == CounterpartyInfoChainIdRole) {
		return QString::fromStdString(lItem->counterpartyInfoChainId().toHex());

	} else if (role == EventsRole) {
		QList<qbit::QConversationEventInfo*> lInfos;

		for (std::vector<qbit::ConversationItem::EventInfo>::const_iterator lItemInfo = lItem->events().begin(); lItemInfo != lItem->events().end(); lItemInfo++) {
			lInfos.append(new qbit::QConversationEventInfo(*lItemInfo));
		}

		return QVariant::fromValue(lInfos);
	} else if (role == OnChainRole) {
		return lItem->isOnChain();
	} else if (role == DynamicRole) {
		return lItem->isDynamic();
	} else if (role == SelfRole) {
		return QVariant::fromValue(new qbit::QConversationItem(lItem));
	}

	return QVariant();
}

QHash<int, QByteArray> ConversationsfeedListModel::roleNames() const {
	//
	QHash<int, QByteArray> lRoles;
	lRoles[TimestampRole] = "timestamp";
	lRoles[AgoRole] = "ago";
	lRoles[ScoreRole] = "score";
	lRoles[SideRole] = "side";
	lRoles[LocalDateTimeRole] = "localDateTime";

	lRoles[ConversationIdRole] = "conversationId";
	lRoles[ConversationChainIdRole] = "conversationChainId";
	lRoles[CreatorIdRole] = "creatorId";
	lRoles[CreatorInfoIdRole] = "creatorInfoId";
	lRoles[CreatorInfoChainIdRole] = "creatorInfoChainId";
	lRoles[CounterpartyIdRole] = "counterpartyId";
	lRoles[CounterpartyInfoIdRole] = "counterpartyInfoId";
	lRoles[CounterpartyInfoChainIdRole] = "counterpartyInfoChainId";
	lRoles[EventsRole] = "events";
	lRoles[SelfRole] = "self";

	lRoles[OnChainRole] = "onChain";
	lRoles[DynamicRole] = "dynamic";

	return lRoles;
}

void ConversationsfeedListModel::conversationsLargeUpdated() {
	//
}

void ConversationsfeedListModel::conversationItemNew(qbit::ConversationItemPtr buzz) {
	qInfo() << "ConversationsfeedListModel::conversationItemNew";
	emit conversationItemNewSignal(qbit::ConversationItemProxy(buzz));
}

void ConversationsfeedListModel::conversationItemUpdated(qbit::ConversationItemPtr buzz) {
	qInfo() << "ConversationsfeedListModel::conversationItemUpdated";
	emit conversationItemUpdatedSignal(qbit::ConversationItemProxy(buzz));
}

void ConversationsfeedListModel::feed(qbit::ConversationsfeedPtr local, bool more, bool merge) {
	//
	if (filtered_) return;

	//
	if (merge) {
		ConversationsfeedListModel::merge();
		return;
	}

	//
	if (!more) {
		//
		qInfo() << "==== START =====";

		beginResetModel();

		// get ordered list
		list_.clear();
		index_.clear();
		conversations_->feed(list_);

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
			emit conversationsUpdated(list_[0]);
		}
	} else if (!noMoreData_) {
		//
		qInfo() << "==== MORE =====";

		// get 'more' feed
		std::vector<qbit::ConversationItemPtr> lAppendFeed;
		local->feed(lAppendFeed);

		qInfo() << "local.count = " << lAppendFeed.size();

		// merge base and 'more'
		bool lItemsAdded = conversations_->mergeAppend(lAppendFeed);
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
			std::map<uint256, int> lOldIndex = index_;

			// new feed
			list_.clear();
			index_.clear();
			conversations_->feed(list_);

			// make index
			for (int lItem = 0; lItem < (int)list_.size(); lItem++) {
				index_[list_[lItem]->key()] = lItem;

				//qInfo() << "More: " << QString::fromStdString(list_[lItem]->key().toString()) << QString::fromStdString(list_[lItem]->buzzBodyString());
			}

			qInfo() << "new.count = " << list_.size();

			if ((int)list_.size() == lFrom) {
				noMoreData_ = true;
			} else {
				if (lFrom >= (int)list_.size()) {
					for (std::map<uint256, int>::iterator lIdx = lOldIndex.begin(); lIdx != lOldIndex.end(); lIdx++) {
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

void ConversationsfeedListModel::merge() {
	// index for check
	std::map<uint256, int> lOldIndex = index_;

	//
	qInfo() << "==== MERGE =====";

	// new feed
	list_.clear();
	index_.clear();
	conversations_->feed(list_);

	// make index
	for (int lItem = 0; lItem < (int)list_.size(); lItem++) {
		index_[list_[lItem]->key()] = lItem;
	}

	// reverse
	for (std::map<uint256, int>::iterator lIdx = lOldIndex.begin(); lIdx != lOldIndex.end(); lIdx++) {
		//
		std::map<uint256, int>::iterator lNewIdx = index_.find(lIdx->first);
		if (lNewIdx == index_.end()) {
			beginRemoveRows(QModelIndex(), lIdx->second, lIdx->second);
			endRemoveRows();
		} else {
			QModelIndex lModelIndex = createIndex(lNewIdx->second, lNewIdx->second);
			emit dataChanged(lModelIndex, lModelIndex, QVector<int>() << AgoRole << EventsRole);
		}
	}

	// forward merge
	for (std::map<uint256, int>::iterator lIdx = index_.begin(); lIdx != index_.end(); lIdx++) {
		//
		std::map<uint256, int>::iterator lOldIdx = lOldIndex.find(lIdx->first);
		if (lOldIdx == lOldIndex.end()) {
			beginInsertRows(QModelIndex(), lIdx->second, lIdx->second);
			endInsertRows();
		}
	}
}

void ConversationsfeedListModel::feedSlot(const qbit::ConversationsfeedProxy& local, bool more, bool merge) {
	//
	qbit::ConversationsfeedPtr lFeed(local.get());
	feed(lFeed, more, merge);
}

void ConversationsfeedListModel::conversationItemNewSlot(const qbit::ConversationItemProxy& buzz) {
	//
	qbit::ConversationItemPtr lItem(buzz.get());
	int lIndex = conversations_->locateIndex(lItem);
	if ((lIndex != -1 && list_.begin() != list_.end()) || !list_.size()) {
		//
		if (index_.find(lItem->key()) == index_.end()) {
			if (lIndex < list_.size() || !list_.size()) {
				if (!list_.size()) {
					lIndex = 0;
					list_.push_back(lItem);
				} else {
					list_.insert(list_.begin() + lIndex, lItem);
				}
			}

			// rebuild index
			index_.clear();
			//
			for (int lItemIdx = 0; lItemIdx < (int)list_.size(); lItemIdx++) {
				index_[list_[lItemIdx]->key()] = lItemIdx;
			}

			if (!filtered_) {
				qInfo() << "ConversationsfeedListModel::conversationItemNewSlot -> insert" << lIndex;
				beginInsertRows(QModelIndex(), lIndex, lIndex);
				endInsertRows();

				emit countChanged();
				emit conversationsUpdated(lItem);
			}
		} else {
			conversationItemUpdatedSlot(buzz);
		}
	}
}

void ConversationsfeedListModel::conversationItemUpdatedSlot(const qbit::ConversationItemProxy& buzz) {
	//
	qbit::ConversationItemPtr lItem(buzz.get());
	//
	int lOriginalIndex = conversations_->locateIndex(lItem);
	std::map<uint256, int>::iterator lIndexPtr = index_.find(lItem->key());
	if (lIndexPtr != index_.end()) {
		//
		int lIndex = lIndexPtr->second;

		// rebuild intems\index
		if (lOriginalIndex != -1 && lIndex != lOriginalIndex && !filtered_ && lIndex > lOriginalIndex) {
			//
			qInfo() << "ConversationsfeedListModel::conversationItemUpdatedSlot -> elevate" << lIndex << lOriginalIndex;

			//
			list_.erase(list_.begin() + lIndex);
			list_.insert(list_.begin() + lOriginalIndex, lItem);

			// rebuild index
			index_.clear();
			//
			for (int lItemIdx = 0; lItemIdx < (int)list_.size(); lItemIdx++) {
				index_[list_[lItemIdx]->key()] = lItemIdx;
			}

			beginMoveRows(QModelIndex(), lIndex, lIndex, QModelIndex(), lOriginalIndex);
			endMoveRows();

			QModelIndex lModelIndex = createIndex(lOriginalIndex, lOriginalIndex);
			emit dataChanged(lModelIndex, lModelIndex, QVector<int>()
					<< AgoRole
					<< EventsRole);
			emit conversationsUpdated(lItem);
		} else if (lIndex >= 0 && lIndex < (int)list_.size() && !filtered_) {
			qInfo() << "ConversationsfeedListModel::conversationItemUpdatedSlot -> update" << lIndex;
			QModelIndex lModelIndex = createIndex(lIndex, lIndex);
			emit dataChanged(lModelIndex, lModelIndex, QVector<int>()
					<< AgoRole
					<< EventsRole);
			emit conversationsUpdated(lItem);
		}
	}
}

ConversationsListModel::ConversationsListModel() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	conversations_ = qbit::Conversationsfeed::instance(lClient->getBuzzer(),
		boost::bind(&qbit::BuzzerLightComposer::verifyConversationPublisher, lClient->getBuzzerComposer(), boost::placeholders::_1),
		boost::bind(&ConversationsfeedListModel::conversationsLargeUpdated, this),
		boost::bind(&ConversationsfeedListModel::conversationItemNew, this, boost::placeholders::_1),
		boost::bind(&ConversationsfeedListModel::conversationItemUpdated, this, boost::placeholders::_1),
		boost::bind(&ConversationsfeedListModel::conversationsfeedItemsUpdated, this, boost::placeholders::_1)
	);

	conversations_->prepare();

	QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}
