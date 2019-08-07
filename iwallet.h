// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IWALLET_H
#define QBIT_IWALLET_H

#include "key.h"
#include "transaction.h"
#include "transactioncontext.h"

namespace qbit {

class IWallet {
public:
	IWallet() {}

	// key menegement
	virtual SKey createKey(const std::list<std::string>&) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	virtual SKey findKey(const PKey&) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	// new utxo and tx context to process deeply
	virtual uint256 pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	virtual bool popUnlinkedOut(const uint256&) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	// try to locate utxo
	virtual Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256&) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	// try to find utxo by asset with amount >=
	virtual Transaction::UnlinkedOutPtr findUnlinkedOutByAsset(const uint256&, amount_t) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	// wallet balance
	virtual amount_t balance() { return 0; } // qbit balance
	virtual amount_t balance(const uint256& asset) { return 0; }
};

typedef std::shared_ptr<IWallet> IWalletPtr;

} // qbit

#endif