#include "peerslistmodel.h"

#include "client.h"
#include "iapplication.h"

#include "../requestprocessor.h"

using namespace buzzer;

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif
int __idPeersListModelSharedPtr = qRegisterMetaType<PeersListModelSharedPtr>("PeersListModelSharedPtr");
int __idPeersListModelWeakPtr = qRegisterMetaType<PeersListModelWeakPtr>("PeersListModelWeakPtr");
#ifdef __clang__
#  pragma clang diagnostic pop
#endif

PeerlistLoader::PeerlistLoader(): QObject (nullptr) {
	moveToThread(&thread_);
	connect (this, SIGNAL(loadTo(PeersListModelSharedPtr)), SLOT(get(QString, ImageSharedPtr, bool)), Qt::QueuedConnection);
	thread_.start();
}

PeerlistLoader& PeerlistLoader::instance() {
	static PeerlistLoader instance;
	return instance;
}

void PeerlistLoader::stop() {
	PeerlistLoader::instance().thread_.quit();
}

void PeerlistLoader::load(PeersListModelSharedPtr model) {
	//
}

PeersListModel::PeersListModel() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	//
	connect(lClient, SIGNAL(peerPushedSignal(buzzer::PeerProxy, bool, int)),
			this,  SLOT(peerPushedSlot(buzzer::PeerProxy, bool, int)));
	connect(lClient, SIGNAL(peerPoppedSignal(buzzer::PeerProxy, int)),
			this,  SLOT(peerPoppedSlot(buzzer::PeerProxy, int)));

	QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

int PeersListModel::rowCount(const QModelIndex& parent) const {
	Q_UNUSED(parent);
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	return list_.size();
}

QVariant PeersListModel::data(const QModelIndex& index, int role) const {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);

	//
	if (index.row() < 0 || index.row() >= (int)list_.size()) {
		return QVariant();
	}

	PeerItemPtr lItem = list_[index.row()];
	if (role == IdRole) {
		return QString::fromStdString(lItem->id().toHex());
	} else if (role == EndpointRole) {
		return QString::fromStdString(lItem->endpoint());
	} else if (role == StatusRole) {
		return lItem->status();
	} else if (role == TimeRole) {
		return QVariant::fromValue(lItem->time());
	} else if (role == OutboundRole) {
		return lItem->outbound();
	} else if (role == LocalDateTimeRole) {
		return QString::fromStdString(qbit::formatLocalDateTimeLong(lItem->time() / 1000000));
	} else if (role == LatencyRole) {
		return QVariant::fromValue((int)(lItem->latency() / 1000));
	} else if (role == RolesRole) {
		return QString::fromStdString(lItem->roles());
	} else if (role == AddressRole) {
		return QString::fromStdString(lItem->address());
	} else if (role == InQueueRole) {
		return QVariant::fromValue(lItem->inQueue());
	} else if (role == OutQueueRole) {
		return QVariant::fromValue(lItem->outQueue());
	} else if (role == PendingQueueRole) {
		return QVariant::fromValue(lItem->pendingQueue());
	} else if (role == ReceivedCountRole) {
		return QVariant::fromValue(lItem->receivedCount());
	} else if (role == ReceivedBytesRole) {
		return QVariant::fromValue(lItem->receivedBytes());
	} else if (role == SentCountRole) {
		return QVariant::fromValue(lItem->sentCount());
	} else if (role == SentBytesRole) {
		return QVariant::fromValue(lItem->sentBytes());
	} else if (role == DAppsRole) {
		//
		std::string lDApps;
		for(std::vector<qbit::State::DAppInstance>::const_iterator lInstance = lItem->dapps().begin();
												lInstance != lItem->dapps().end(); lInstance++) {
			lDApps += strprintf("<li>%s# / %s</li>", lInstance->instance().toHex().substr(0, 10), lInstance->name());
		}

		return QString::fromStdString(lDApps);
	} else if (role == ChainsRole) {
		//
		std::string lChains;
		for (std::vector<qbit::State::BlockInfo>::const_iterator lInfo = lItem->chains().begin(); lInfo != lItem->chains().end(); lInfo++) {
			std::string lName = (lInfo->dApp().size() ? lInfo->dApp() : "none");
			std::string lHash = lInfo->hash().toHex();
			std::string lBlock = lHash.substr(0, 6) + "..." + lHash.substr(lHash.size()-6, lHash.size()-1);
			lChains += strprintf("<li>%s - (%s, %s)</li>", lBlock, lInfo->height(), lName);
		}

		return QString::fromStdString(lChains);
	}

	return QVariant();
}

QHash<int, QByteArray> PeersListModel::roleNames() const {
	//
	QHash<int, QByteArray> lRoles;

	lRoles[IdRole] = "peerId";
	lRoles[EndpointRole] = "endpoint";
	lRoles[StatusRole] = "status";
	lRoles[TimeRole] = "time";
	lRoles[LocalDateTimeRole] = "localDateTime";
	lRoles[OutboundRole] = "outbound";
	lRoles[LatencyRole] = "latency";
	lRoles[RolesRole] = "roles";
	lRoles[AddressRole] = "address";
	lRoles[InQueueRole] = "inQueue";
	lRoles[OutQueueRole] = "outQueue";
	lRoles[PendingQueueRole] = "pendingQueue";
	lRoles[ReceivedCountRole] = "receivedCount";
	lRoles[ReceivedBytesRole] = "receivedBytes";
	lRoles[SentCountRole] = "sentCount";
	lRoles[SentBytesRole] = "sentBytes";
	lRoles[DAppsRole] = "dapps";
	lRoles[ChainsRole] = "chains";

	return lRoles;
}

void PeersListModel::update() {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	std::list<qbit::IPeerPtr> lItems;
	feedRawInternal(lItems);

	for (std::list<qbit::IPeerPtr>::iterator lItem = lItems.begin(); lItem != lItems.end(); lItem++) {
		peerPushedInternal(buzzer::PeerProxy(*lItem), true, 1);
	}
}

void PeersListModel::remove(int idx) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (idx < 0 || idx >= (int)list_.size()) {
		return;
	}

	index_.erase(list_[idx]->id());
	list_.erase(list_.begin() + idx);

	beginRemoveRows(QModelIndex(), idx, idx);
	endRemoveRows();
}

void PeersListModel::feedRawInternal(std::list<qbit::IPeerPtr>& items) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	// get peers
	qbit::IPeerManagerPtr lPeerManager = lClient->getPeerManager();
	lPeerManager->allPeers(items);
}

void PeersListModel::feedInternal(std::vector<PeerItemPtr>& items) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	// get peers
	qbit::IPeerManagerPtr lPeerManager = lClient->getPeerManager();
	std::list<qbit::IPeerPtr> lPeers;
	lPeerManager->allPeers(lPeers);

	for (std::list<qbit::IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
		//
		if ((*lPeer)->status() == qbit::IPeer::UNDEFINED) continue;
		if ((*lPeer)->status() != qbit::IPeer::ACTIVE) {
			QString lStatus;
			if ((*lPeer)->status() == qbit::IPeer::QUARANTINE) lStatus = "QUARANTINE";
			else if ((*lPeer)->status() == qbit::IPeer::PENDING_STATE) lStatus = "PENDING";
			else if ((*lPeer)->status() == qbit::IPeer::POSTPONED) lStatus = "POSTPONED";
			qInfo() << "[PeersListModel::feedInternal/warning]:" << QString::fromStdString((*lPeer)->key()) << lStatus;
		}

		items.push_back(PeerItem::instance(*lPeer));
	}
}

void PeersListModel::feed(bool /*more*/) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	beginResetModel();

	list_.clear();
	index_.clear();

	feedInternal(list_);

	// make order
	for (int lItem = 0; lItem < (int)list_.size(); lItem++) {
		index_[list_[lItem]->id()] = lItem;
	}

	//
	endResetModel();
	//
	emit countChanged();
}

void PeersListModel::peerPushedInternal(const buzzer::PeerProxy& peer, bool /*update*/, int /*count*/) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	std::map<uint160 /*peer*/, int>::iterator lIndex = index_.find(peer.get()->addressId());
	if (lIndex != index_.end() && lIndex->second < (int)list_.size()) {
		PeerItemPtr lItem = list_[lIndex->second];
		if (lItem) {
			//
			// qInfo() << "[peerPushedInternal]" << QString::fromStdString(peer.get()->key()) << lIndex->second;
			//
			if (lItem->status() != qbit::IPeer::ACTIVE && peer.get()->status() == qbit::IPeer::ACTIVE) {
				// remove
				beginRemoveRows(QModelIndex(), lIndex->second, lIndex->second);
				endRemoveRows();

				// add
				lItem->assign(peer.get());
				beginInsertRows(QModelIndex(), lIndex->second, lIndex->second);
				endInsertRows();

				emit countChanged();
			} else {
				lItem->assign(peer.get());
				QModelIndex lModelIndex = createIndex(lIndex->second, lIndex->second);
				emit dataChanged(lModelIndex, lModelIndex,
					QVector<int>() <<
						StatusRole <<
						TimeRole <<
						LocalDateTimeRole <<
						OutboundRole <<
						LatencyRole <<
						RolesRole <<
						AddressRole <<
						InQueueRole <<
						OutQueueRole <<
						PendingQueueRole <<
						ReceivedCountRole <<
						ReceivedBytesRole <<
						SentCountRole <<
						SentBytesRole <<
						DAppsRole <<
						ChainsRole
				);
			}
		}
	} else {		
		PeerItemPtr lItem = PeerItem::instance(peer.get());
		list_.push_back(lItem);
		index_[lItem->id()] = list_.size()-1;

		// qInfo() << "[peerPushedInternal]" << QString::fromStdString(peer.get()->key()) << list_.size()-1;

		beginInsertRows(QModelIndex(), list_.size()-1, list_.size()-1);
		endInsertRows();

		emit countChanged();
	}
}

void PeersListModel::peerPoppedInternal(const buzzer::PeerProxy& peer, int /*count*/) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	std::map<uint160 /*peer*/, int>::iterator lIndex = index_.find(peer.get()->addressId());
	if (lIndex != index_.end() && lIndex->second < (int)list_.size()) {
		int lIdx = lIndex->second;
		list_.erase(list_.begin() + lIdx);
		index_.clear();

		// rebuild index
		for (int lIdxNew = 0; lIdxNew < (int)list_.size(); lIdxNew++) {
			index_[list_[lIdxNew]->id()] = lIdxNew;
		}

		// qInfo() << "[peerPoppedInternal]" << QString::fromStdString(peer.get()->key()) << lIdx;

		beginRemoveRows(QModelIndex(), lIdx, lIdx);
		endRemoveRows();

		emit countChanged();
	}
}

void PeersListModel::peerPushedSlot(buzzer::PeerProxy peer, bool update, int count) {
	peerPushedInternal(peer, update, count);
}

void PeersListModel::peerPoppedSlot(buzzer::PeerProxy peer, int count) {
	peerPoppedInternal(peer, count);
}

//
// PeersActiveListModel
//

void PeersActiveListModel::feedInternal(std::vector<PeerItemPtr>& items) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	// get peers
	qbit::IRequestProcessorPtr lProcessor = lClient->getBuzzerComposer()->requestProcessor();
	qbit::RequestProcessorPtr lRequestProcessor = std::static_pointer_cast<qbit::RequestProcessor>(lProcessor);

	std::map<uint160, qbit::IPeerPtr> lPeers;
	lRequestProcessor->collectPeers(lPeers);

	for (std::map<uint160, qbit::IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
		//
		if (lPeer->second->status() == qbit::IPeer::UNDEFINED) continue;
		items.push_back(PeerItem::instance(lPeer->second));
	}
}

void PeersActiveListModel::feedRawInternal(std::list<qbit::IPeerPtr>& items) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	// get peers
	qbit::IRequestProcessorPtr lProcessor = lClient->getBuzzerComposer()->requestProcessor();
	qbit::RequestProcessorPtr lRequestProcessor = std::static_pointer_cast<qbit::RequestProcessor>(lProcessor);

	std::map<uint160, qbit::IPeerPtr> lPeers;
	lRequestProcessor->collectPeers(lPeers);

	for (std::map<uint160, qbit::IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
		//
		if (lPeer->second->status() == qbit::IPeer::UNDEFINED) continue;
		items.push_back(lPeer->second);
	}
}

//
// PeersAddedListModel
//

PeersAddedListModel::PeersAddedListModel() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	//
	disconnect(lClient, SIGNAL(peerPushedSignal(const buzzer::PeerProxy&, bool, int)),
			0, 0);
	disconnect(lClient, SIGNAL(peerPoppedSignal(const buzzer::PeerProxy&, int)),
			0, 0);

	QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

void PeersAddedListModel::feedInternal(std::vector<PeerItemPtr>& items) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	// get peers
	qbit::IPeerManagerPtr lPeerManager = lClient->getPeerManager();
	std::list<qbit::IPeerPtr> lPeers;
	lPeerManager->explicitPeers(lPeers);

	for (std::list<qbit::IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
		//
		items.push_back(PeerItem::instance(*lPeer));
	}
}

void PeersAddedListModel::feedRawInternal(std::list<qbit::IPeerPtr>& items) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	// get peers
	qbit::IPeerManagerPtr lPeerManager = lClient->getPeerManager();
	std::list<qbit::IPeerPtr> lPeers;
	lPeerManager->explicitPeers(lPeers);

	for (std::list<qbit::IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
		//
		items.push_back(*lPeer);
	}
}
