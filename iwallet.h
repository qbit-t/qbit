// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IWALLET_H
#define QBIT_IWALLET_H

#include "key.h"
#include "transaction.h"

namespace qbit {

class IWallet {
public:
	IWallet() {}

	virtual SKey createKey(const std::list<std::string>&) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	virtual SKey findKey(const PKey&) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	virtual uint256 pushUnlinkedOut(const Transaction::UnlinkedOut&) {}
	virtual bool popUnlinkedOut(const uint256&) {}

	virtual bool findUnlinkedOut(const uint256&, Transaction::UnlinkedOut&) { return false; }

	virtual amount_t balance() { return 0; }
};

typedef std::shared_ptr<IWallet> IWalletPtr;

} // qbit

#endif