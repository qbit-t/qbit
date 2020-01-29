// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IWALLET_H
#define QBIT_IWALLET_H

#include "key.h"
#include "transaction.h"
#include "itransactionstore.h"
#include "transactioncontext.h"
#include "txassettype.h"
#include "txassetemission.h"
#include "itransactionstoremanager.h"
#include "imemorypoolmanager.h"

namespace qbit {

class IWallet {
public:
	IWallet() {}

	virtual bool open() { throw qbit::exception("NOT_IMPL", "IWallet::open - not implemented."); }
	virtual bool close() { throw qbit::exception("NOT_IMPL", "IWallet::close - not implemented."); }
	virtual bool isOpened() { throw qbit::exception("NOT_IMPL", "IWallet::isOpened - not implemented."); }

	virtual void cleanUpData() { throw qbit::exception("NOT_IMPL", "IWallet::cleanUpData - not implemented."); }

	// key menagement
	virtual SKey createKey(const std::list<std::string>&) { throw qbit::exception("NOT_IMPL", "IWallet::createKey - not implemented."); }
	virtual SKey findKey(const PKey&) { throw qbit::exception("NOT_IMPL", "IWallet::findKey - not implemented."); }
	virtual SKey firstKey() { throw qbit::exception("NOT_IMPL", "IWallet::firstKey - not implemented."); }

	// new utxo and tx context to process deeply
	virtual bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "IWallet::pushUnlinkedOut - not implemented."); }
	virtual bool popUnlinkedOut(const uint256&, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "IWallet::popUnlinkedOut - not implemented."); }
	virtual bool isUnlinkedOutExists(const uint256&) { throw qbit::exception("NOT_IMPL", "IWallet::isUnlinkedOutExists - not implemented."); }
	virtual bool isUnlinkedOutUsed(const uint256&) { throw qbit::exception("NOT_IMPL", "IWallet::isUnlinkedOutExists - not implemented."); }

	// try to locate utxo
	virtual Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&) { throw qbit::exception("NOT_IMPL", "IWallet::findUnlinkedOut - not implemented."); }

	// try to find utxo by asset with amount >=
	virtual Transaction::UnlinkedOutPtr findUnlinkedOutByAsset(const uint256&, amount_t) { throw qbit::exception("NOT_IMPL", "IWallet::findUnlinkedOutByAsset - not implemented."); }
	virtual std::list<Transaction::UnlinkedOutPtr> collectUnlinkedOutsByAsset(const uint256&, amount_t) { throw qbit::exception("NOT_IMPL", "IWallet::collectUnlinkedOutsByAsset - not implemented."); }

	// clean-up assets utxo
	virtual void cleanUp() { throw qbit::exception("NOT_IMPL", "IWallet::cleanUp - not implemented."); }

	// dump utxo by asset
	virtual void dumpUnlinkedOutByAsset(const uint256&, std::stringstream&) { throw qbit::exception("NOT_IMPL", "IWallet::dumpUnlinkedOutByAsset - not implemented."); }

	// rollback tx
	virtual bool rollback(TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "IWallet::rollback - not implemented."); }

	// create coinbase tx
	virtual TransactionContextPtr createTxCoinBase(amount_t /*amount*/) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	// create spend tx
	virtual TransactionContextPtr createTxSpend(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/, qunit_t /*fee limit*/, int32_t targetBlock = -1) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	virtual TransactionContextPtr createTxSpend(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	// create spend private tx
	virtual TransactionContextPtr createTxSpendPrivate(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/, qunit_t /*fee limit*/, int32_t targetBlock = -1) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	virtual TransactionContextPtr createTxSpendPrivate(const uint256& /*asset*/, const PKey& /*dest*/, amount_t /*amount*/) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	// create asset type tx
	virtual TransactionContextPtr createTxAssetType(const PKey& /*dest*/, const std::string& /*short name*/, const std::string& /*long name*/, amount_t /*supply*/, amount_t /*scale*/, TxAssetType::Emission /*emission*/, qunit_t /*fee limit*/, int32_t targetBlock = -1) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	virtual TransactionContextPtr createTxAssetType(const PKey& /*dest*/, const std::string& /*short name*/, const std::string& /*long name*/, amount_t /*supply*/, amount_t /*scale*/, TxAssetType::Emission /*emission*/) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	// total supply = supply * chunks * scale (chunks = number of unlinked outputs)
	virtual TransactionContextPtr createTxAssetType(const PKey& /*dest*/, const std::string& /*short name*/, const std::string& /*long name*/, amount_t /*supply*/, amount_t /*scale*/, int /*chunks*/, TxAssetType::Emission /*emission*/, qunit_t /*fee limit*/, int32_t targetBlock = -1) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	virtual TransactionContextPtr createTxAssetType(const PKey& /*dest*/, const std::string& /*short name*/, const std::string& /*long name*/, amount_t /*supply*/, amount_t /*scale*/, int /*chunks*/, TxAssetType::Emission /*emission*/) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	// create asset emission tx
	virtual TransactionContextPtr createTxLimitedAssetEmission(const PKey& /*dest*/, const uint256& /*asset*/, qunit_t /*fee limit*/, int32_t targetBlock = -1) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	virtual TransactionContextPtr createTxLimitedAssetEmission(const PKey& /*dest*/, const uint256& /*asset*/) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	// wallet balance
	virtual amount_t balance() { return 0; } // qbit balance
	virtual amount_t balance(const uint256& asset) { return 0; }
	virtual void resetCache() { throw qbit::exception("NOT_IMPL", "IWallet::resetCache - not implemented."); }
	virtual bool prepareCache() { throw qbit::exception("NOT_IMPL", "IWallet::prepareCache - not implemented."); }

	virtual ITransactionStorePtr persistentStore() { throw qbit::exception("NOT_IMPL", "IWallet::persistentStore - Not implemented."); }
	virtual IMemoryPoolManagerPtr mempoolManager() { throw qbit::exception("NOT_IMPL", "IWallet::mempoolManager - Not implemented."); }
	virtual ITransactionStoreManagerPtr storeManager() { throw qbit::exception("NOT_IMPL", "IWallet::storeManager - Not implemented."); }

	//
	virtual void removePendingTransaction(const uint256& /*tx*/) { throw qbit::exception("NOT_IMPL", "IWallet::removePendingTransaction - Not implemented."); }
	virtual void collectPendingTransactions(std::list<TransactionPtr>& /*list*/) { throw qbit::exception("NOT_IMPL", "IWallet::collectPendingTransactions - Not implemented."); }
};

typedef std::shared_ptr<IWallet> IWalletPtr;

} // qbit

#endif