#include "buzzfeedlistmodel.h"
#include "dapps/buzzer/buzzfeedview.h"

#include "client.h"
#include "iapplication.h"

#include "transaction.h"
#include "dapps/buzzer/txbuzzer.h"

using namespace buzzer;

BuzzfeedListModel::BuzzfeedListModel() {
	//
	connect(this, SIGNAL(buzzfeedItemNewSignal(const qbit::BuzzfeedItemProxy&)), this, SLOT(buzzfeedItemNewSlot(const qbit::BuzzfeedItemProxy&)));
	connect(this, SIGNAL(buzzfeedItemUpdatedSignal(const qbit::BuzzfeedItemProxy&)), this, SLOT(buzzfeedItemUpdatedSlot(const qbit::BuzzfeedItemProxy&)));
	connect(this, SIGNAL(buzzfeedItemsUpdatedSignal(const qbit::BuzzfeedItemUpdatesProxy&)), this, SLOT(buzzfeedItemsUpdatedSlot(const qbit::BuzzfeedItemUpdatesProxy&)));
}

int BuzzfeedListModel::rowCount(const QModelIndex& parent) const {
	Q_UNUSED(parent);
	return list_.size();
}

int BuzzfeedListModel::childrenCount(int index) const {
	if (index < 0 || index >= (int)list_.size()) {
		return 0;
	}

	qbit::BuzzfeedItemPtr lItem = list_[index];
	return lItem->count();
}

bool BuzzfeedListModel::hasPrevLink(int index) const {
	if (index < 0 || index >= (int)list_.size()) {
		return false;
	}

	qbit::BuzzfeedItemPtr lItem = list_[index];
	return lItem->hasPrevLink();
}

bool BuzzfeedListModel::hasNextLink(int index) const {
	if (index < 0 || index >= (int)list_.size()) {
		return false;
	}

	qbit::BuzzfeedItemPtr lItem = list_[index];
	return lItem->hasNextLink();
}

QString BuzzfeedListModel::buzzId(int index) const {
	if (index < 0 || index >= (int)list_.size()) {
		return "";
	}

	qbit::BuzzfeedItemPtr lItem = list_[index];
	return QString::fromStdString(lItem->buzzId().toHex());
}

QVariant BuzzfeedListModel::self(int index) const {
	if (index < 0 || index >= (int)list_.size()) {
		return QVariant();
	}

	qbit::BuzzfeedItemPtr lItem = list_[index];
	return QVariant::fromValue(new qbit::QBuzzfeedItem(lItem));
}

void BuzzfeedListModel::setOnChain(int index) {
	if (index < 0 || index >= (int)list_.size()) {
		return;
	}

	qbit::BuzzfeedItemPtr lItem = list_[index];
	lItem->setOnChain();

	QModelIndex lModelIndex = createIndex(index, index);
	emit dataChanged(lModelIndex, lModelIndex, QVector<int>() << OnChainRole);
}

QVariant BuzzfeedListModel::data(const QModelIndex& index, int role) const {
	//
	if (index.row() < 0 || index.row() >= (int)list_.size()) {
		return QVariant();
	}

	qbit::BuzzfeedItemPtr lItem = list_[index.row()];	
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
	} else if (role == BuzzerIdRole) {
		return QString::fromStdString(lItem->buzzerId().toHex());
	} else if (role == BuzzerInfoIdRole) {
		return QString::fromStdString(lItem->buzzerInfoId().toHex());
	} else if (role == BuzzerInfoChainIdRole) {
		return QString::fromStdString(lItem->buzzerInfoChainId().toHex());
	} else if (role == BuzzBodyRole) {
		// decorate @buzzers, #tags and urls
		Client* lClient = static_cast<Client*>(gApplication->getClient());
		return lClient->decorateBuzzBody(QString::fromStdString(lItem->buzzBodyString()));
	} else if (role == LastUrlRole) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());
		return lClient->extractLastUrl(QString::fromStdString(lItem->buzzBodyString()));
	} else if (role == RepliesRole) {
		return lItem->replies();
	} else if (role == RebuzzesRole) {
		return lItem->rebuzzes();
	} else if (role == LikesRole) {
		return lItem->likes();
	} else if (role == RewardsRole) {
		return QVariant::fromValue(lItem->rewards());
	} else if (role == OriginalBuzzIdRole) {
		return QString::fromStdString(lItem->originalBuzzId().toHex());
	} else if (role == OriginalBuzzChainIdRole) {
		return QString::fromStdString(lItem->originalBuzzChainId().toHex());
	} else if (role == HasParentRole) {
		return (!lItem->originalBuzzId().isEmpty() && !lItem->wrapped());
	} else if (role == ValueRole) {
		return QVariant::fromValue(lItem->value());
	} else if (role == BuzzerNameRole) {
		return QString::fromStdString(lItem->buzzerName());
	} else if (role == BuzzerAliasRole) {
		return QString::fromStdString(lItem->buzzerAlias());
	} else if (role == BuzzMediaRole) {
		QList<qbit::QBuzzerMediaPointer*> lMediaPointers;

		for (std::vector<qbit::BuzzerMediaPointer>::const_iterator lPointer = lItem->buzzMedia().begin(); lPointer != lItem->buzzMedia().end(); lPointer++) {
			lMediaPointers.append(new qbit::QBuzzerMediaPointer(lPointer->chain(), lPointer->tx()));
		}

		return QVariant::fromValue(lMediaPointers);
	} else if (role == BuzzInfosRole) {
		QList<qbit::QItemInfo*> lInfos;

		for (std::vector<qbit::BuzzfeedItem::ItemInfo>::const_iterator lItemInfo = lItem->infos().begin(); lItemInfo != lItem->infos().end(); lItemInfo++) {
			lInfos.append(new qbit::QItemInfo(*lItemInfo));
		}

		return QVariant::fromValue(lInfos);
	} else if (role == ChildrenCountRole) {
		return QVariant::fromValue((uint32_t)lItem->count());
	} else if (role == FirstChildIdRole) {
		if (lItem->count()) {
			qbit::BuzzfeedItemPtr lChild = lItem->firstChild();
			if (lChild) return QString::fromStdString(lChild->buzzId().toHex());
		}

		return "";
	} else if (role == HasPrevSiblingRole) {
		//
		int lCurrentIndex = index.row();
		if (lCurrentIndex - 1 >= 0) {
			if (!list_[lCurrentIndex - 1]->originalBuzzId().isEmpty() ||
					lItem->originalBuzzId() == list_[lCurrentIndex - 1]->buzzId()) return true;
		}
		return false;
	} else if (role == HasNextSiblingRole) {
		//
		int lCurrentIndex = index.row();
		if (lCurrentIndex + 1 < list_.size()) {
			if (!list_[lCurrentIndex + 1]->originalBuzzId().isEmpty()) return true;
		}
		return false;
	} else if (role == PrevSiblingIdRole) {
		//
		int lCurrentIndex = index.row();
		if (lCurrentIndex - 1 >= 0) {
			return QString::fromStdString(list_[lCurrentIndex - 1]->buzzId().toHex());
		}
		return "";
	} else if (role == NextSiblingIdRole) {
		//
		int lCurrentIndex = index.row();
		if (lCurrentIndex + 1 < list_.size()) {
			return QString::fromStdString(list_[lCurrentIndex + 1]->buzzId().toHex());
		}
		return "";
	} else if (role == WrappedRole) {
		if (lItem->wrapped()) {
			return QVariant::fromValue(new qbit::QBuzzfeedItem(lItem->wrapped()));
		}

		return QVariant();
	} else if (role == SelfRole) {
		return QVariant::fromValue(new qbit::QBuzzfeedItem(lItem));
	} else if (role == ThreadedRole) {
		return lItem->threaded();
	} else if (role == HasPrevLinkRole) {
		return lItem->hasPrevLink();
	} else if (role == HasNextLinkRole) {
		return lItem->hasNextLink();
	} else if (role == OnChainRole) {
		return lItem->isOnChain();
	} else if (role == DynamicRole) {
		return lItem->isDynamic();
	}

	return QVariant();
}

QHash<int, QByteArray> BuzzfeedListModel::roleNames() const {
	//
	QHash<int, QByteArray> lRoles;
	lRoles[TypeRole] = "type";
	lRoles[BuzzIdRole] = "buzzId";
	lRoles[BuzzChainIdRole] = "buzzChainId";
	lRoles[TimestampRole] = "timestamp";
	lRoles[AgoRole] = "ago";
	lRoles[LocalDateTimeRole] = "localDateTime";
	lRoles[ScoreRole] = "score";
	lRoles[BuzzerIdRole] = "buzzerId";
	lRoles[BuzzerInfoIdRole] = "buzzerInfoId";
	lRoles[BuzzerInfoChainIdRole] = "buzzerInfoChainId";
	lRoles[BuzzBodyRole] = "buzzBody";
	lRoles[BuzzMediaRole] = "buzzMedia";
	lRoles[RepliesRole] = "replies";
	lRoles[RebuzzesRole] = "rebuzzes";
	lRoles[LikesRole] = "likes";
	lRoles[RewardsRole] = "rewards";
	lRoles[OriginalBuzzIdRole] = "originalBuzzId";
	lRoles[OriginalBuzzChainIdRole] = "originalBuzzChainId";
	lRoles[HasParentRole] = "hasParent";
	lRoles[ValueRole] = "value";
	lRoles[BuzzInfosRole] = "buzzInfos";
	lRoles[BuzzerNameRole] = "buzzerName";
	lRoles[BuzzerAliasRole] = "buzzerAlias";
	lRoles[ChildrenCountRole] = "childrenCount";
	lRoles[HasPrevSiblingRole] = "hasPrevSibling";
	lRoles[HasNextSiblingRole] = "hasNextSibling";
	lRoles[PrevSiblingIdRole] = "prevSiblingId";
	lRoles[NextSiblingIdRole] = "nextSiblingId";
	lRoles[FirstChildIdRole] = "firstChildId";
	lRoles[LastUrlRole] = "lastUrl";
	lRoles[WrappedRole] = "wrapped";
	lRoles[SelfRole] = "self";
	lRoles[ThreadedRole] = "threaded";
	lRoles[HasPrevLinkRole] = "hasPrevLink";
	lRoles[HasNextLinkRole] = "hasNextLink";
	lRoles[OnChainRole] = "onChain";
	lRoles[DynamicRole] = "dynamic";

	return lRoles;
}

void BuzzfeedListModel::buzzfeedLargeUpdated() {
	//
}

void BuzzfeedListModel::buzzfeedItemAbsent(const uint256& /*chain*/, const uint256& /*buzz*/) {
	//
}

void BuzzfeedListModel::buzzfeedItemNew(qbit::BuzzfeedItemPtr buzz) {
	qInfo() << "BuzzfeedListModel::buzzfeedItemNew";
	emit buzzfeedItemNewSignal(qbit::BuzzfeedItemProxy(buzz));
}

void BuzzfeedListModel::buzzfeedItemUpdated(qbit::BuzzfeedItemPtr buzz) {
	qInfo() << "BuzzfeedListModel::buzzfeedItemUpdated";
	emit buzzfeedItemUpdatedSignal(qbit::BuzzfeedItemProxy(buzz));
}

void BuzzfeedListModel::buzzfeedItemsUpdated(const std::vector<qbit::BuzzfeedItemUpdate>& items) {
	qInfo() << "BuzzfeedListModel::buzzfeedItemsUpdated";
	emit buzzfeedItemsUpdatedSignal(qbit::BuzzfeedItemUpdatesProxy(items));
}

void BuzzfeedListModel::feed(qbit::BuzzfeedPtr local, bool more) {
	//
	if (!more) {
		//
		qInfo() << "==== START =====";

		beginResetModel();

		// get ordered list
		list_.clear();
		index_.clear();
		buzzfeed_->feed(list_);

		noMoreData_ = false;

		// make index
		for (int lItem = 0; lItem < list_.size(); lItem++) {
			index_[list_[lItem]->key()] = lItem;
			//qInfo() << "Initial: " << QString::fromStdString(list_[lItem]->key().toString()) << QString::fromStdString(list_[lItem]->buzzBodyString());
		}

		//
		endResetModel();

		//
		emit countChanged();
	} else if (!noMoreData_) {
		//
		qInfo() << "==== MORE =====";

		// get 'more' feed
		std::vector<qbit::BuzzfeedItemPtr> lAppendFeed;
		local->feed(lAppendFeed);

		qInfo() << "local.count = " << lAppendFeed.size();

		// merge base and 'more'
		bool lItemsAdded = buzzfeed_->mergeAppend(lAppendFeed);
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
			std::map<qbit::BuzzfeedItem::Key, int> lOldIndex = index_;

			// new feed
			list_.clear();
			index_.clear();
			buzzfeed_->feed(list_);

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
					for (std::map<qbit::BuzzfeedItem::Key, int>::iterator lIdx = lOldIndex.begin(); lIdx != lOldIndex.end(); lIdx++) {
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
				emit dataChanged(lStartIndex, lEndIndex, QVector<int>() << HasPrevLinkRole << HasNextLinkRole);

				//
				emit countChanged();
			}
		} else {
			noMoreData_ = true;
		}
	}
}

void BuzzfeedListModel::feedSlot(const qbit::BuzzfeedProxy& local, bool more) {
	//
	qbit::BuzzfeedPtr lBuzzfeed(local.get());
	feed(lBuzzfeed, more);
}

void BuzzfeedListModel::buzzfeedItemNewSlot(const qbit::BuzzfeedItemProxy& buzz) {
	//
	qbit::BuzzfeedItemPtr lBuzzfeedItem(buzz.get());
	int lIndex = buzzfeed_->locateIndex(lBuzzfeedItem);
	if ((lIndex != -1 && list_.begin() != list_.end()) || !list_.size()) {
		//
		if (index_.find(lBuzzfeedItem->key()) == index_.end()) {
			if (!list_.size()) {
				lIndex = 0;
				list_.push_back(lBuzzfeedItem);
			} else {
				list_.insert(list_.begin() + lIndex, lBuzzfeedItem);
			}

			index_[lBuzzfeedItem->key()] = lIndex;

			qInfo() << "BuzzfeedListModel::buzzfeedItemNewSlot -> insert" << lIndex;
			beginInsertRows(QModelIndex(), lIndex, lIndex);
			endInsertRows();

			emit countChanged();
		} else {
			buzzfeedItemUpdatedSlot(buzz);
		}
	}
}

void BuzzfeedListModel::buzzfeedItemUpdatedSlot(const qbit::BuzzfeedItemProxy& buzz) {
	//
	qbit::BuzzfeedItemPtr lBuzzfeedItem(buzz.get());

	buzzfeedItemUpdatedProcess(lBuzzfeedItem, qbit::Transaction::TX_BUZZ);
	buzzfeedItemUpdatedProcess(lBuzzfeedItem, qbit::Transaction::TX_REBUZZ);
	buzzfeedItemUpdatedProcess(lBuzzfeedItem, qbit::Transaction::TX_BUZZ_REPLY);
	buzzfeedItemUpdatedProcess(lBuzzfeedItem, qbit::Transaction::TX_BUZZ_LIKE);
	buzzfeedItemUpdatedProcess(lBuzzfeedItem, qbit::Transaction::TX_BUZZ_REWARD);
}

void BuzzfeedListModel::buzzfeedItemsUpdatedSlot(const qbit::BuzzfeedItemUpdatesProxy& items) {
	//
	for (std::vector<qbit::BuzzfeedItemUpdate>::const_iterator lItem = items.get().begin(); lItem != items.get().end(); lItem++) {
		//
		buzzfeedItemsUpdatedProcess(*lItem, qbit::Transaction::TX_BUZZ);
		buzzfeedItemsUpdatedProcess(*lItem, qbit::Transaction::TX_REBUZZ);
		buzzfeedItemsUpdatedProcess(*lItem, qbit::Transaction::TX_BUZZ_REPLY);
		buzzfeedItemsUpdatedProcess(*lItem, qbit::Transaction::TX_BUZZ_LIKE);
		buzzfeedItemsUpdatedProcess(*lItem, qbit::Transaction::TX_BUZZ_REWARD);
	}
}

bool BuzzfeedListModel::buzzfeedItemUpdatedProcess(qbit::BuzzfeedItemPtr item, unsigned short type) {
	//
	std::map<qbit::BuzzfeedItem::Key, int>::iterator lItem = index_.find(qbit::BuzzfeedItem::Key(item->buzzId(), type));
	if (lItem != index_.end()) {
		//
		int lIndex = lItem->second;
		if (lIndex >= 0 && lIndex < list_.size()) {
			qInfo() << "BuzzfeedListModel::buzzfeedItemUpdatedProcess -> update" << lIndex;
			QModelIndex lModelIndex = createIndex(lIndex, lIndex);
			emit dataChanged(lModelIndex, lModelIndex, QVector<int>()
					<< HasParentRole
					<< HasNextSiblingRole
					<< HasPrevSiblingRole
					<< ChildrenCountRole
					<< FirstChildIdRole
					<< NextSiblingIdRole
					<< PrevSiblingIdRole
					<< HasPrevLinkRole
					<< HasNextLinkRole);

			return true;
		}
	}

	return false;
}

bool BuzzfeedListModel::buzzfeedItemsUpdatedProcess(const qbit::BuzzfeedItemUpdate& update, unsigned short type) {
	//
	qbit::BuzzfeedItem::Key lKey(update.buzzId(), type);
	std::map<qbit::BuzzfeedItem::Key, int>::iterator lEntry = index_.find(lKey);
	//
	if (lEntry != index_.end()) {
		//
		int lIndex = lEntry->second;
		QModelIndex lModelIndex = createIndex(lIndex, lIndex);

		qInfo() << "BuzzfeedListModel::buzzfeedItemsUpdatedProcess -> update" <<
				   lIndex << QString::fromStdString(update.fieldString()) << update.count();

		qbit::BuzzfeedItemUpdate::Field lField = update.field();
		if (lField == qbit::BuzzfeedItemUpdate::Field::LIKES)
			emit dataChanged(lModelIndex, lModelIndex, QVector<int>() << LikesRole << HasParentRole << HasNextSiblingRole << HasPrevSiblingRole << ChildrenCountRole << FirstChildIdRole << NextSiblingIdRole << PrevSiblingIdRole << HasPrevLinkRole << HasNextLinkRole);
		else if (lField == qbit::BuzzfeedItemUpdate::Field::REBUZZES)
			emit dataChanged(lModelIndex, lModelIndex, QVector<int>() << RebuzzesRole << HasParentRole << HasNextSiblingRole << HasPrevSiblingRole << ChildrenCountRole << FirstChildIdRole << NextSiblingIdRole << PrevSiblingIdRole << HasPrevLinkRole << HasNextLinkRole);
		else if (lField == qbit::BuzzfeedItemUpdate::Field::REPLIES)
			emit dataChanged(lModelIndex, lModelIndex, QVector<int>() << RepliesRole << HasParentRole << HasNextSiblingRole << HasPrevSiblingRole << ChildrenCountRole << FirstChildIdRole << NextSiblingIdRole << PrevSiblingIdRole << HasPrevLinkRole << HasNextLinkRole);
		else if (lField == qbit::BuzzfeedItemUpdate::Field::REWARDS)
			emit dataChanged(lModelIndex, lModelIndex, QVector<int>() << RewardsRole << HasParentRole << HasNextSiblingRole << HasPrevSiblingRole << ChildrenCountRole << FirstChildIdRole << NextSiblingIdRole << PrevSiblingIdRole << HasPrevLinkRole << HasNextLinkRole);

		return true;
	}

	return false;
}

BuzzfeedListModelPersonal::BuzzfeedListModelPersonal() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	buzzfeed_ = qbit::Buzzfeed::instance(lClient->getBuzzer(),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherStrict, lClient->getBuzzerComposer(), _1),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherLazy, lClient->getBuzzerComposer(), _1),
		boost::bind(&BuzzfeedListModel::buzzfeedLargeUpdated, this),
		boost::bind(&BuzzfeedListModel::buzzfeedItemNew, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemUpdated, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemsUpdated, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemAbsent, this, _1, _2),
		qbit::BuzzfeedItem::Merge::UNION,
		qbit::BuzzfeedItem::Expand::FULL
	);

	buzzfeed_->prepare();
}

BuzzfeedListModelGlobal::BuzzfeedListModelGlobal() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	buzzfeed_ = qbit::Buzzfeed::instance(lClient->getBuzzer(),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherLazy, lClient->getBuzzerComposer(), _1),
		boost::bind(&BuzzfeedListModel::buzzfeedLargeUpdated, this),
		boost::bind(&BuzzfeedListModel::buzzfeedItemNew, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemUpdated, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemsUpdated, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemAbsent, this, _1, _2),
		qbit::BuzzfeedItem::Merge::INTERSECT
	);

	buzzfeed_->prepare();
}

BuzzfeedListModelBuzzer::BuzzfeedListModelBuzzer() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	buzzfeed_ = qbit::Buzzfeed::instance(lClient->getBuzzer(),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherLazy, lClient->getBuzzerComposer(), _1),
		boost::bind(&BuzzfeedListModel::buzzfeedLargeUpdated, this),
		boost::bind(&BuzzfeedListModel::buzzfeedItemNew, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemUpdated, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemsUpdated, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemAbsent, this, _1, _2),
		qbit::BuzzfeedItem::Merge::UNION
	);

	buzzfeed_->prepare();
}

BuzzfeedListModelBuzzes::BuzzfeedListModelBuzzes() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	buzzfeed_ = qbit::Buzzfeed::instance(lClient->getBuzzer(),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherLazy, lClient->getBuzzerComposer(), _1),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherLazy, lClient->getBuzzerComposer(), _1),
		boost::bind(&qbit::BuzzerLightComposer::verifyMyThreadUpdates, lClient->getBuzzerComposer(), _1, _2, _3),
		boost::bind(&BuzzfeedListModel::buzzfeedLargeUpdated, this),
		boost::bind(&BuzzfeedListModel::buzzfeedItemNew, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemUpdated, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemsUpdated, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemAbsent, this, _1, _2),
		qbit::BuzzfeedItem::Merge::INTERSECT,
		qbit::BuzzfeedItem::Order::FORWARD,
		qbit::BuzzfeedItem::Group::TIMESTAMP,
		qbit::BuzzfeedItem::Expand::FULL
	);

	buzzfeed_->prepare();
}

BuzzfeedListModelTag::BuzzfeedListModelTag() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	buzzfeed_ = qbit::Buzzfeed::instance(lClient->getBuzzer(),
		boost::bind(&qbit::BuzzerLightComposer::verifyPublisherLazy, lClient->getBuzzerComposer(), _1),
		boost::bind(&BuzzfeedListModel::buzzfeedLargeUpdated, this),
		boost::bind(&BuzzfeedListModel::buzzfeedItemNew, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemUpdated, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemsUpdated, this, _1),
		boost::bind(&BuzzfeedListModel::buzzfeedItemAbsent, this, _1, _2),
		qbit::BuzzfeedItem::Merge::INTERSECT
	);

	buzzfeed_->prepare();
}
