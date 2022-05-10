#ifndef WALLETTRANSACTIONS_LIST_MODEL_H
#define WALLETTRANSACTIONS_LIST_MODEL_H

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
class UnlinkedOutProxy: public QObject {
	Q_OBJECT

public:
	UnlinkedOutProxy() {}
	UnlinkedOutProxy(qbit::Transaction::UnlinkedOutPtr item): item_(item) {
	}
	UnlinkedOutProxy(const UnlinkedOutProxy& other) {
		set(other.get());
	}

	void set(qbit::Transaction::UnlinkedOutPtr item) { item_ = item; }
	qbit::Transaction::UnlinkedOutPtr get() const { return item_; }

private:
	qbit::Transaction::UnlinkedOutPtr item_;
};

//
class NetworkUnlinkedOutProxy: public QObject {
	Q_OBJECT

public:
	NetworkUnlinkedOutProxy() {}
	NetworkUnlinkedOutProxy(qbit::Transaction::NetworkUnlinkedOutPtr item): item_(item) {
	}
	NetworkUnlinkedOutProxy(const NetworkUnlinkedOutProxy& other) {
		set(other.get());
	}

	void set(qbit::Transaction::NetworkUnlinkedOutPtr item) { item_ = item; }
	qbit::Transaction::NetworkUnlinkedOutPtr get() const { return item_; }

private:
	qbit::Transaction::NetworkUnlinkedOutPtr item_;
};

//
class TransactionProxy: public QObject {
	Q_OBJECT

public:
	TransactionProxy() {}
	TransactionProxy(qbit::TransactionPtr item): item_(item) {
	}
	TransactionProxy(const TransactionProxy& other) {
		set(other.get());
	}

	void set(qbit::TransactionPtr item) { item_ = item; }
	qbit::TransactionPtr get() const { return item_; }

private:
	qbit::TransactionPtr item_;
};

class WalletTransactionsListModel: public QAbstractListModel {
	Q_OBJECT

	Q_PROPERTY(ulong count READ count NOTIFY countChanged)
	Q_PROPERTY(bool noMoreData READ noMoreData NOTIFY noMoreDataChanged)
	Q_PROPERTY(QString asset READ asset WRITE setAsset NOTIFY assetChanged)

public:
	enum WalletTransactionRoles {
		TypeRole = Qt::UserRole + 1,
		TimestampRole,
		DirectionRole,
		AgoRole,
		LocalDateTimeRole,
		IdRole,
		ChainRole,
		AddressRole,
		ParentIdRole,
		ParentTypeRole,
		AmountRole,
		FeeRole,
		ConfirmsRole,
		ConfirmedRole,
		CoinbaseRole,
		TimelockRole,
		TimelockedRole,
		UtxoRole,
		BlindedRole
	};

public:
	WalletTransactionsListModel();
	WalletTransactionsListModel(const std::string&, const uint256&, qbit::IWalletPtr);
	virtual ~WalletTransactionsListModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

	ulong count() { return list_.size(); }
	bool noMoreData() { return noMoreData_; }

	QString asset() { return QString::fromStdString(asset_.toHex()); }
	void setAsset(QString asset) {
		asset_.setHex(asset.toStdString());
	}
	void setAsset(const uint256& asset) {
		asset_ = asset;
	}

	void walletReceiveTransaction(qbit::Transaction::UnlinkedOutPtr, qbit::TransactionPtr);
	void outUpdated(qbit::Transaction::NetworkUnlinkedOutPtr);
	void inUpdated(qbit::Transaction::NetworkUnlinkedOutPtr);

	Q_INVOKABLE void feed(bool);
	Q_INVOKABLE void updateAgo(int);

	Q_INVOKABLE void resetModel() {
		beginResetModel();
		endResetModel();
	}

public slots:
	void walletReceiveTransactionSlot(const buzzer::UnlinkedOutProxy&, const buzzer::TransactionProxy&);
	void outUpdatedSlot(const buzzer::NetworkUnlinkedOutProxy&);
	void inUpdatedSlot(const buzzer::NetworkUnlinkedOutProxy&);

signals:
	void countChanged();
	void noMoreDataChanged();
	void assetChanged();
	void walletReceiveTransactionSignal(const buzzer::UnlinkedOutProxy&, const buzzer::TransactionProxy&);
	void outUpdatedSignal(const buzzer::NetworkUnlinkedOutProxy&);
	void inUpdatedSignal(const buzzer::NetworkUnlinkedOutProxy&);

protected:
	QHash<int, QByteArray> roleNames() const;
	virtual void feedInternal(uint64_t, std::vector<qbit::Transaction::NetworkUnlinkedOutPtr>&);
	virtual void walletReceiveTransactionInternal(const UnlinkedOutProxy&, const TransactionProxy&);
	virtual void outUpdatedInternal(const NetworkUnlinkedOutProxy&);
	virtual void inUpdatedInternal(const NetworkUnlinkedOutProxy&);
	virtual void updatedInternal(const NetworkUnlinkedOutProxy&);
	virtual bool processUpdate(const NetworkUnlinkedOutProxy&);

protected:
	qbit::IWalletPtr wallet_;
	uint64_t from_;
	std::vector<qbit::Transaction::NetworkUnlinkedOutPtr> list_;
	std::map<uint256, int> tx_;
	std::map<uint256, int> txIndex_;
	std::multimap<uint64_t, uint256> order_;
	std::vector<int> index_;
	uint256 asset_ = qbit::TxAssetType::qbitAsset();
	bool noMoreData_ = false;
};

class WalletTransactionsListModelReceived: public WalletTransactionsListModel {
public:
	WalletTransactionsListModelReceived() {}
	WalletTransactionsListModelReceived(const uint256& asset, qbit::IWalletPtr wallet) :
		WalletTransactionsListModel(std::string("received-") + asset.toHex(), asset, wallet) {
		QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
	}

protected:
	void outUpdatedInternal(const NetworkUnlinkedOutProxy&) {}
	void feedInternal(uint64_t from, std::vector<qbit::Transaction::NetworkUnlinkedOutPtr>& items) {
		//
		wallet_->selectIns(asset_, from, items);
	}
	bool processUpdate(const NetworkUnlinkedOutProxy& out) {
		//
		qbit::Transaction::NetworkUnlinkedOutPtr lOut = out.get();
		return !lOut->utxo().change();
	}
};

class WalletTransactionsListModelSent: public WalletTransactionsListModel {
public:
	WalletTransactionsListModelSent() {}
	WalletTransactionsListModelSent(const uint256& asset, qbit::IWalletPtr wallet) :
		WalletTransactionsListModel(std::string("sent-") + asset.toHex(), asset, wallet) {
		QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
	}

protected:
	void inUpdatedInternal(const NetworkUnlinkedOutProxy&) {}
	void feedInternal(uint64_t from, std::vector<qbit::Transaction::NetworkUnlinkedOutPtr>& items) {
		//
		wallet_->selectOuts(asset_, from, items);
	}
	bool processUpdate(const NetworkUnlinkedOutProxy& out) {
		//
		qbit::Transaction::NetworkUnlinkedOutPtr lOut = out.get();
		return (lOut->parentType() == qbit::Transaction::SPEND ||
				lOut->parentType() == qbit::Transaction::SPEND_PRIVATE);
	}
};

}

#endif // WALLETTRANSACTIONS_LIST_MODEL_H
