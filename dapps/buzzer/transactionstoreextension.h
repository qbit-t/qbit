// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_TRANSACTION_STORE_EXTENSION_H
#define QBIT_BUZZER_TRANSACTION_STORE_EXTENSION_H

#include "../../transaction.h"
#include "../../block.h"
#include "../../blockcontext.h"
#include "../../transactioncontext.h"
#include "../../itransactionstoreextension.h"
#include "../../db/containers.h"

#include <memory>

namespace qbit {

class BuzzerTransactionStoreExtension: public ITransactionStoreExtension, public std::enable_shared_from_this<BuzzerTransactionStoreExtension> {
public:
	BuzzerTransactionStoreExtension(ISettingsPtr settings, ITransactionStorePtr store) : 
		settings_(settings),
		store_(store),
		timeline_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/timeline"), 
		subscriptionsIdx_(settings_->dataPath() + "/" + store->chain().toHex() + "/buzzer/indexes/subscriptions") {}

	bool open();
	bool close();

	bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) {}
	bool popUnlinkedOut(const uint256&, TransactionContextPtr) {}
	bool pushEntity(const uint256&, TransactionContextPtr);

	TransactionPtr locateSubscription(const uint256& /*subscriber*/, const uint256& /*publisher*/);

	static ITransactionStoreExtensionPtr instance(ISettingsPtr settings, ITransactionStorePtr store) {
		return std::make_shared<BuzzerTransactionStoreExtension>(settings, store);
	}

	void removeTransaction(TransactionPtr);

private:
	ISettingsPtr settings_;
	ITransactionStorePtr store_;
	bool opened_ = false;

	// timeline
	db::DbMultiContainer<uint64_t /*timestamp*/, uint256 /*buzz*/> timeline_;
	// subscriber|publisher -> tx
	db::DbTwoKeyContainer<uint256 /*subscriber*/, uint256 /*publisher*/, uint256 /*tx*/> subscriptionsIdx_;
};

typedef std::shared_ptr<BuzzerTransactionStoreExtension> BuzzerTransactionStoreExtensionPtr;

//
class BuzzerTransactionStoreExtensionCreator: public TransactionStoreExtensionCreator {
public:
	BuzzerTransactionStoreExtensionCreator() {}
	ITransactionStoreExtensionPtr create(ISettingsPtr settings, ITransactionStorePtr store) {
		return BuzzerTransactionStoreExtension::instance(settings, store);
	}

	static TransactionStoreExtensionCreatorPtr instance() {
		return std::make_shared<BuzzerTransactionStoreExtensionCreator>();
	}
};

} // qbit

#endif
