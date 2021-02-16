#ifndef PEERS_LIST_MODEL_H
#define PEERS_LIST_MODEL_H

#include <QString>
#include <QList>
#include <QAbstractListModel>
#include <QQuickItem>
#include <QMap>
#include <QQmlApplicationEngine>

#include "transaction.h"
#include "iwallet.h"

namespace buzzer {

//
class PeerProxy: public QObject {
	Q_OBJECT

public:
	PeerProxy() {}
	PeerProxy(qbit::IPeerPtr item): item_(item) {
	}
	PeerProxy(const PeerProxy& other) {
		set(other.get());
	}

	void set(qbit::IPeerPtr item) { item_ = item; }
	qbit::IPeerPtr get() const { return item_; }

private:
	qbit::IPeerPtr item_;
};

class PeerItem;
typedef std::shared_ptr<PeerItem> PeerItemPtr;

class PeerItem: public std::enable_shared_from_this<PeerItem> {
public:
	PeerItem() {}
	PeerItem(qbit::IPeerPtr peer) {
		assign(peer);
	}

	void assign(qbit::IPeerPtr peer) {
		id_ = peer->addressId();
		endpoint_ = peer->key();
		status_ = peer->status();
		time_ = peer->time();
		outbound_ = peer->isOutbound() ? true : false;
		latency_ = peer->latency();
		roles_ = peer->state()->rolesString();
		if (peer->state()->address().valid())
			address_ = peer->state()->address().toString();
		else
			address_ = "undefined";

		inQueue_ = peer->inQueueLength();
		outQueue_ = peer->outQueueLength();
		pendingQueue_ = peer->pendingQueueLength();
		receivedCount_ = peer->receivedMessagesCount();
		receivedBytes_ = peer->bytesReceived();
		sentCount_ = peer->sentMessagesCount();
		sentBytes_ = peer->bytesSent();

		dapps_ = peer->state()->dApps();
		chains_ = peer->state()->infos();
	}

	const uint160& id() const { return id_; }
	const std::string& endpoint() const { return endpoint_; }
	unsigned short status() const { return (unsigned short)status_; }
	uint64_t time() const { return time_; }
	bool outbound() const { return outbound_; }
	int latency() const { return latency_; }
	const std::string& roles() const { return roles_; }
	const std::string& address() const { return address_; }

	unsigned int inQueue() const { return inQueue_; }
	unsigned int outQueue() const { return outQueue_; }
	unsigned int pendingQueue() const { return pendingQueue_; }
	unsigned int receivedCount() const { return receivedCount_; }
	uint64_t receivedBytes() const { return receivedBytes_; }
	unsigned int sentCount() const { return sentCount_; }
	uint64_t sentBytes() const { return sentBytes_; }

	const std::vector<qbit::State::DAppInstance>& dapps() const { return dapps_; }
	const std::vector<qbit::State::BlockInfo>& chains() const { return chains_; }

	static PeerItemPtr instance(qbit::IPeerPtr peer) {
		return std::make_shared<PeerItem>(peer);
	}

private:
	uint160 id_;
	std::string endpoint_;
	qbit::IPeer::Status status_;
	uint64_t time_;
	bool outbound_;
	int latency_;
	std::string roles_;
	std::string address_;

	unsigned int inQueue_;
	unsigned int outQueue_;
	unsigned int pendingQueue_;
	unsigned int receivedCount_;
	uint64_t receivedBytes_;
	unsigned int sentCount_;
	uint64_t sentBytes_;

	std::vector<qbit::State::DAppInstance> dapps_;
	std::vector<qbit::State::BlockInfo> chains_;
};

class PeersListModel: public QAbstractListModel {
	Q_OBJECT

	Q_PROPERTY(ulong count READ count NOTIFY countChanged)

public:
	enum PeerRoles {
		IdRole = Qt::UserRole + 1,
		EndpointRole,
		StatusRole,
		TimeRole,
		LocalDateTimeRole,
		OutboundRole,
		LatencyRole,
		RolesRole,
		AddressRole,
		InQueueRole,
		OutQueueRole,
		PendingQueueRole,
		ReceivedCountRole,
		ReceivedBytesRole,
		SentCountRole,
		SentBytesRole,
		DAppsRole,
		ChainsRole
	};

public:
	PeersListModel();
	virtual ~PeersListModel() {}

	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

	ulong count() { return list_.size(); }

	Q_INVOKABLE void update();
	Q_INVOKABLE void feed(bool);
	Q_INVOKABLE void resetModel() {
		beginResetModel();
		endResetModel();
	}
	Q_INVOKABLE void remove(int);

public slots:
	void peerPushedSlot(const buzzer::PeerProxy& /*peer*/, bool /*update*/, int /*count*/);
	void peerPoppedSlot(const buzzer::PeerProxy& /*peer*/, int /*count*/);

signals:
	void countChanged();
	void noMoreDataChanged();
	void assetChanged();

protected:
	QHash<int, QByteArray> roleNames() const;
	virtual void feedInternal(std::vector<PeerItemPtr>&);
	virtual void feedRawInternal(std::list<qbit::IPeerPtr>&);
	virtual void peerPushedInternal(const buzzer::PeerProxy& /*peer*/, bool /*update*/, int /*count*/);
	virtual void peerPoppedInternal(const buzzer::PeerProxy& /*peer*/, int /*count*/);

protected:
	std::vector<PeerItemPtr> list_;
	std::map<uint160 /*peer*/, int> index_;
};

class PeersActiveListModel: public PeersListModel {
public:
	PeersActiveListModel() {
		QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
	}

protected:
	void feedInternal(std::vector<PeerItemPtr>&);
	void feedRawInternal(std::list<qbit::IPeerPtr>&);
};

class PeersAddedListModel: public PeersListModel {
public:
	PeersAddedListModel();

protected:
	void feedInternal(std::vector<PeerItemPtr>&);
	void feedRawInternal(std::list<qbit::IPeerPtr>&);
};

}

#endif // PEERS_LIST_MODEL_H
