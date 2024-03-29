// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IPEER_EXTENSION_H
#define QBIT_IPEER_EXTENSION_H

#include "state.h"
#include "key.h"

#include "transaction.h"
#include "message.h"
#include "transactioncontext.h"

namespace qbit {

// forward
class IPeer;
typedef std::shared_ptr<IPeer> IPeerPtr;

//
// extension for p2p protocol processor
class IPeerExtension {
public:
	IPeerExtension() {}

	virtual void processTransaction(TransactionContextPtr) { throw qbit::exception("NOT_IMPL", "IPeerExtension::processTransaction - not implemented."); }
	virtual bool processMessage(Message& /*message*/, std::list<DataStream>::iterator /*data*/, const boost::system::error_code& /*error*/) { throw qbit::exception("NOT_IMPL", "IPeerExtension::processMessage - not implemented."); }
	virtual void release() { throw qbit::exception("NOT_IMPL", "IPeerExtension::release - not implemented."); }

	void setDApp(const State::DAppInstance& app) { dapp_ = app; }

protected:
	State::DAppInstance dapp_;
};

typedef std::shared_ptr<IPeerExtension> IPeerExtensionPtr;

} // qbit

#endif
