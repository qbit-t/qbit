#include "wallettransactionslistmodel.h"

#include "client.h"
#include "iapplication.h"

using namespace buzzer;

WalletTransactionsListModel::WalletTransactionsListModel() {
}

WalletTransactionsListModel::WalletTransactionsListModel(const std::string& key, qbit::IWalletPtr wallet) : wallet_(wallet) {
	//
	wallet_->setWalletReceiveTransactionFunction(key, boost::bind(&WalletTransactionsListModel::walletReceiveTransaction, this, _1, _2));
	wallet_->setWalletOutUpdatedFunction(key, boost::bind(&WalletTransactionsListModel::outUpdated, this, _1));
	wallet_->setWalletInUpdatedFunction(key, boost::bind(&WalletTransactionsListModel::inUpdated, this, _1));

	//
	connect(this, SIGNAL(walletReceiveTransactionSignal(const buzzer::UnlinkedOutProxy&, const buzzer::TransactionProxy&)),
			this,  SLOT(walletReceiveTransactionSlot(const buzzer::UnlinkedOutProxy&, const buzzer::TransactionProxy&)));
	connect(this, SIGNAL(outUpdatedSignal(const buzzer::NetworkUnlinkedOutProxy&)),
			this,  SLOT(outUpdatedSlot(const buzzer::NetworkUnlinkedOutProxy&)));
	connect(this, SIGNAL(inUpdatedSignal(const buzzer::NetworkUnlinkedOutProxy&)),
			this,  SLOT(inUpdatedSlot(const buzzer::NetworkUnlinkedOutProxy&)));
}

void WalletTransactionsListModel::updateAgo(int index) {
	if (index < 0 || index >= (int)list_.size()) {
		return;
	}

	QModelIndex lModelIndex = createIndex(index, index);
	emit dataChanged(lModelIndex, lModelIndex, QVector<int>() << AgoRole);
}

int WalletTransactionsListModel::rowCount(const QModelIndex& parent) const {
	Q_UNUSED(parent);
	return list_.size();
}

QVariant WalletTransactionsListModel::data(const QModelIndex& index, int role) const {
	//
	if (index.row() < 0 || index.row() >= (int)list_.size()) {
		return QVariant();
	}

	qbit::Transaction::NetworkUnlinkedOutPtr lItem = list_[index_[index.row()]];
	if (role == TypeRole) {
		return lItem->type();
	} else if (role == IdRole) {
		return QString::fromStdString(lItem->utxo().out().tx().toHex());
	} else if (role == ChainRole) {
		return QString::fromStdString(lItem->utxo().out().chain().toHex());
	} else if (role == TimestampRole) {
		return QVariant::fromValue(lItem->timestamp());
	} else if (role == AgoRole) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());
		return lClient->timestampAgo(lItem->timestamp());
	} else if (role == LocalDateTimeRole) {
		return QString::fromStdString(qbit::formatLocalDateTimeLong(lItem->timestamp() / 1000000));
	} else if (role == DirectionRole) {
		return QVariant::fromValue((int)lItem->direction());
	} else if (role == ParentIdRole) {
		if (lItem->parent().isNull()) return QString();
		return QString::fromStdString(lItem->parent().toHex());
	} else if (role == ParentTypeRole) {
		return lItem->parentType();
	} else if (role == AddressRole) {
		return QString::fromStdString(lItem->utxo().address().toString());
	} else if (role == AmountRole) {
		return QVariant::fromValue(lItem->utxo().amount());
	} else if (role == FeeRole) {
		return QVariant::fromValue(lItem->fee());
	} else if (role == ConfirmsRole) {
		return QVariant::fromValue(lItem->confirms());
	} else if (role == ConfirmedRole) {
		return lItem->confirms() > 0;
	} else if (role == CoinbaseRole) {
		return lItem->coinbase();
	} else if (role == TimelockRole) {
		return QVariant::fromValue(lItem->utxo().lock());
	} else if (role == TimelockedRole) {
		return lItem->utxo().lock() > 0;
	} else if (role == UtxoRole) {
		return QString::fromStdString(lItem->utxo().out().hash().toHex());
	} else if (role == BlindedRole) {
		return !lItem->utxo().nonce().isNull();
	}

	return QVariant();
}

QHash<int, QByteArray> WalletTransactionsListModel::roleNames() const {
	//
	QHash<int, QByteArray> lRoles;

	lRoles[TypeRole] = "type";
	lRoles[DirectionRole] = "direction";
	lRoles[TimestampRole] = "timestamp";
	lRoles[AgoRole] = "ago";
	lRoles[LocalDateTimeRole] = "localDateTime";
	lRoles[IdRole] = "txid";
	lRoles[ChainRole] = "chain";
	lRoles[AddressRole] = "address";
	lRoles[ParentIdRole] = "parentId";
	lRoles[ParentTypeRole] = "parentType";
	lRoles[AmountRole] = "amount";
	lRoles[FeeRole] = "fee";
	lRoles[ConfirmsRole] = "confirms";
	lRoles[ConfirmedRole] = "confirmed";
	lRoles[TimelockRole] = "timelock";
	lRoles[TimelockedRole] = "timelocked";
	lRoles[UtxoRole] = "utxo";
	lRoles[BlindedRole] = "blinded";

	return lRoles;
}

void WalletTransactionsListModel::feedInternal(uint64_t from, std::vector<qbit::Transaction::NetworkUnlinkedOutPtr>& items) {
	//
	wallet_->selectLog(from, items);
}

void WalletTransactionsListModel::feed(bool more) {
	//
	if (!more) {
		//
		beginResetModel();

		list_.clear();
		index_.clear();
		order_.clear();
		tx_.clear();
		txIndex_.clear();

		feedInternal(0, list_);

		noMoreData_ = false;

		// make order
		for (int lItem = 0; lItem < (int)list_.size(); lItem++) {
			order_.insert(std::multimap<uint64_t, uint256>::
						  value_type(list_[lItem]->timestamp(), list_[lItem]->utxo().out().hash()));
			tx_[list_[lItem]->utxo().out().hash()] = lItem;
		}

		// make index
		for (std::multimap<uint64_t, uint256>::reverse_iterator lIdx = order_.rbegin(); lIdx != order_.rend(); lIdx++) {
			//
			index_.push_back(tx_[lIdx->second]);
			txIndex_[lIdx->second] = index_.size()-1;
		}

		//
		endResetModel();
		//
		emit countChanged();
	} else if (!noMoreData_) {
		//
		std::vector<qbit::Transaction::NetworkUnlinkedOutPtr> lList;

		uint64_t lFrom = 0;
		if (list_.size()) {
			qbit::Transaction::NetworkUnlinkedOutPtr lItem = list_[index_[list_.size()-1]];
			lFrom = lItem->timestamp();
		}

		feedInternal(lFrom, lList);

		if (lList.size()) {
			//
			int lLast = list_.size();
			//

			// make order
			int lAdded = 0;
			for (int lItem = 0; lItem < (int)lList.size(); lItem++) {
				if (tx_.insert(std::map<uint256, int>::value_type(lList[lItem]->utxo().out().hash(), lLast + lAdded)).second) {
					order_.insert(std::multimap<uint64_t, uint256>::
								  value_type(lList[lItem]->timestamp(), lList[lItem]->utxo().out().hash()));
					list_.push_back(lList[lItem]); lAdded++;
				}
			}

			// make index
			index_.clear();
			txIndex_.clear();
			for (std::multimap<uint64_t, uint256>::reverse_iterator lIdx = order_.rbegin(); lIdx != order_.rend(); lIdx++) {
				//
				index_.push_back(tx_[lIdx->second]);
				txIndex_[lIdx->second] = index_.size()-1;
			}

			//
			beginInsertRows(QModelIndex(), lLast, list_.size() - 1);
			endInsertRows();

			//
			QModelIndex lStartIndex = createIndex(0, 0);
			QModelIndex lEndIndex = createIndex(list_.size()-1, list_.size()-1);
			emit dataChanged(lStartIndex, lEndIndex, QVector<int>() << TimestampRole);

			//
			if (!lAdded) { noMoreData_ = true; emit noMoreDataChanged(); }

			//
			emit countChanged();
		} else {
			noMoreData_ = true;
			emit noMoreDataChanged();
		}
	}
}

void WalletTransactionsListModel::walletReceiveTransaction(qbit::Transaction::UnlinkedOutPtr out, qbit::TransactionPtr tx) {
	//
	emit walletReceiveTransactionSignal(buzzer::UnlinkedOutProxy(out), buzzer::TransactionProxy(tx));
}

void WalletTransactionsListModel::outUpdated(qbit::Transaction::NetworkUnlinkedOutPtr out) {
	//
	emit outUpdatedSignal(buzzer::NetworkUnlinkedOutProxy(out));
}

void WalletTransactionsListModel::inUpdated(qbit::Transaction::NetworkUnlinkedOutPtr out) {
	//
	emit inUpdatedSignal(buzzer::NetworkUnlinkedOutProxy(out));
}

void WalletTransactionsListModel::walletReceiveTransactionSlot(const buzzer::UnlinkedOutProxy& out, const buzzer::TransactionProxy& tx) {
	walletReceiveTransactionInternal(out, tx);
}

void WalletTransactionsListModel::outUpdatedSlot(const buzzer::NetworkUnlinkedOutProxy& out) {
	outUpdatedInternal(out);
}

void WalletTransactionsListModel::inUpdatedSlot(const buzzer::NetworkUnlinkedOutProxy& out) {
	inUpdatedInternal(out);
}

void WalletTransactionsListModel::walletReceiveTransactionInternal(const buzzer::UnlinkedOutProxy&, const buzzer::TransactionProxy&) {

}

void WalletTransactionsListModel::updatedInternal(const buzzer::NetworkUnlinkedOutProxy& out) {
	//
	qbit::Transaction::NetworkUnlinkedOutPtr lOut = out.get();
	//
	std::map<uint256, int>::iterator lTx = tx_.find(lOut->utxo().out().hash());
	if (lTx != tx_.end()) {
		//
		list_[lTx->second] = lOut;
		//
		std::map<uint256, int>::iterator lTxIdx = txIndex_.find(lOut->utxo().out().hash());
		if (lTxIdx != txIndex_.end()) {
			QModelIndex lModelIndex = createIndex(lTxIdx->second, lTxIdx->second);
			emit dataChanged(lModelIndex, lModelIndex, QVector<int>() << TimestampRole << ConfirmsRole << ConfirmedRole
																								<< AgoRole << LocalDateTimeRole);
		}
	} else {
		//
		list_.push_back(lOut);
		order_.insert(std::multimap<uint64_t, uint256>::
					  value_type(lOut->timestamp(), lOut->utxo().out().hash()));
		// make index
		int lIntIdx = 0;
		for (std::multimap<uint64_t, uint256>::reverse_iterator lIdx = order_.rbegin(); lIdx != order_.rend(); lIdx++, lIntIdx++) {
			//
			if (lOut->utxo().out().hash() == lIdx->second) {
				index_.insert(index_.begin() + lIntIdx, list_.size()-1);
				tx_[lOut->utxo().out().hash()] = list_.size()-1;
				txIndex_[lOut->utxo().out().hash()] = lIntIdx;

				beginInsertRows(QModelIndex(), lIntIdx, lIntIdx);
				endInsertRows();

				break;
			}
		}
	}
}

void WalletTransactionsListModel::outUpdatedInternal(const buzzer::NetworkUnlinkedOutProxy& out) {
	//
	updatedInternal(out);
}

void WalletTransactionsListModel::inUpdatedInternal(const buzzer::NetworkUnlinkedOutProxy& out) {
	//
	updatedInternal(out);
}



