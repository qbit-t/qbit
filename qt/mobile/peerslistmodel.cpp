#include "peerslistmodel.h"

#include "client.h"
#include "iapplication.h"

#include "../requestprocessor.h"

using namespace buzzer;

PeersListModel::PeersListModel() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	//
	connect(lClient, SIGNAL(peerPushedSignal(const buzzer::PeerProxy&, bool, int)),
			this,  SLOT(peerPushedSlot(const buzzer::PeerProxy&, bool, int)));
	connect(lClient, SIGNAL(peerPoppedSignal(const buzzer::PeerProxy&, int)),
			this,  SLOT(peerPoppedSlot(const buzzer::PeerProxy&, int)));

	QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

int PeersListModel::rowCount(const QModelIndex& parent) const {
	Q_UNUSED(parent);
	return list_.size();
}

QVariant PeersListModel::data(const QModelIndex& index, int role) const {
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
			std::string lBlock = lHash.substr(0, 8) + "..." + lHash.substr(lHash.size()-9, lHash.size()-1);
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
	std::list<qbit::IPeerPtr> lItems;
	feedRawInternal(lItems);

	for (std::list<qbit::IPeerPtr>::iterator lItem = lItems.begin(); lItem != lItems.end(); lItem++) {
		peerPushedInternal(buzzer::PeerProxy(*lItem), true, 1);
	}
}

void PeersListModel::remove(int idx) {
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
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	// get peers
	qbit::IPeerManagerPtr lPeerManager = lClient->getPeerManager();
	lPeerManager->allPeers(items);
}

void PeersListModel::feedInternal(std::vector<PeerItemPtr>& items) {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	// get peers
	qbit::IPeerManagerPtr lPeerManager = lClient->getPeerManager();
	std::list<qbit::IPeerPtr> lPeers;
	lPeerManager->allPeers(lPeers);

	for (std::list<qbit::IPeerPtr>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
		//
		if ((*lPeer)->status() == qbit::IPeer::UNDEFINED) continue;
		items.push_back(PeerItem::instance(*lPeer));
	}
}

void PeersListModel::feed(bool /*more*/) {
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
	std::map<uint160 /*peer*/, int>::iterator lIndex = index_.find(peer.get()->addressId());
	if (lIndex != index_.end()) {
		PeerItemPtr lItem = list_[lIndex->second];
		if (lItem) {
			//
			if (lItem->status() != qbit::IPeer::ACTIVE && peer.get()->status() == qbit::IPeer::ACTIVE) {
				// remove
				beginRemoveRows(QModelIndex(), lIndex->second, lIndex->second);
				endRemoveRows();

				// add
				lItem->assign(peer.get());
				beginInsertRows(QModelIndex(), lIndex->second, lIndex->second);
				endInsertRows();
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

		//qInfo() << "[peerPushedInternal]" << QString::fromStdString(peer.get()->key()) << list_.size()-1;

		beginInsertRows(QModelIndex(), list_.size()-1, list_.size()-1);
		endInsertRows();
	}
}

void PeersListModel::peerPoppedInternal(const buzzer::PeerProxy& peer, int /*count*/) {
	//
	std::map<uint160 /*peer*/, int>::iterator lIndex = index_.find(peer.get()->addressId());
	if (lIndex != index_.end()) {
		int lIdx = lIndex->second;
		list_.erase(list_.begin() + lIndex->second);
		index_.clear();

		// rebuild index
		for (int lIdx = 0; lIdx < (int)list_.size(); lIdx++) {
			index_[list_[lIdx]->id()] = lIdx;
		}

		qInfo() << "[peerPoppedInternal]" << QString::fromStdString(peer.get()->key()) << lIdx;

		beginRemoveRows(QModelIndex(), lIdx, lIdx);
		endRemoveRows();
	}
}

void PeersListModel::peerPushedSlot(const buzzer::PeerProxy& peer, bool update, int count) {
	peerPushedInternal(peer, update, count);
}

void PeersListModel::peerPoppedSlot(const buzzer::PeerProxy& peer, int count) {
	qInfo() << "[peerPoppedSlot]" << QString::fromStdString(peer.get()->key());
	peerPoppedInternal(peer, count);
}

//
// PeersActiveListModel
//

void PeersActiveListModel::feedInternal(std::vector<PeerItemPtr>& items) {
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
			this,  SLOT(peerPushedSlot(const buzzer::PeerProxy&, bool, int)));
	disconnect(lClient, SIGNAL(peerPoppedSignal(const buzzer::PeerProxy&, int)),
			this,  SLOT(peerPoppedSlot(const buzzer::PeerProxy&, int)));

	QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

void PeersAddedListModel::feedInternal(std::vector<PeerItemPtr>& items) {
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
