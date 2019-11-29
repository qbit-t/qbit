// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IPEER_H
#define QBIT_IPEER_H

#include "state.h"
#include "key.h"
#include "iconsensus.h"

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

namespace qbit {

typedef std::shared_ptr<tcp::socket> SocketPtr;

class IPeer {
public:
	enum Status {
		UNDEFINED = 0,
		ACTIVE = 1,
		QUARANTINE = 2,
		BANNED = 3,
		PENDING_STATE = 4,
		POSTPONED = 5
	};

	enum UpdatePeerResult {
		SUCCESSED = 0,
		EXISTS = 1,
		BAN = 2
	};	

public:
	IPeer() {}

	virtual Status status() { throw qbit::exception("NOT_IMPL", "IPeer::status - not implemented."); }
	virtual uint160 addressId() { throw qbit::exception("NOT_IMPL", "IPeer::addressId - not implemented."); }

	virtual std::string statusString() { throw qbit::exception("NOT_IMPL", "IPeer::statusString - not implemented."); }

	virtual void setState(const State& /*state*/) { throw qbit::exception("NOT_IMPL", "IPeer::setState - not implemented."); }
	virtual void setLatency(uint32_t /*latency*/) { throw qbit::exception("NOT_IMPL", "IPeer::setLatency - not implemented."); }

	virtual PKey address() { throw qbit::exception("NOT_IMPL", "IPeer::address - not implemented."); }
	virtual uint32_t latency() { throw qbit::exception("NOT_IMPL", "IPeer::latency - not implemented."); }
	virtual uint32_t quarantine() { throw qbit::exception("NOT_IMPL", "IPeer::quarantine - not implemented.");}
	virtual int contextId() { throw qbit::exception("NOT_IMPL", "IPeer::contextId - not implemented."); }

	virtual void toQuarantine(uint32_t /*block*/) { throw qbit::exception("NOT_IMPL", "IPeer::toQuarantine - not implemented."); }
	virtual void toBan() { throw qbit::exception("NOT_IMPL", "IPeer::toBan - not implemented."); }
	virtual void toActive() { throw qbit::exception("NOT_IMPL", "IPeer::toActive - not implemented."); }
	virtual void toPendingState() { throw qbit::exception("NOT_IMPL", "IPeer::toPendingState - not implemented."); }
	virtual void toPostponed() { throw qbit::exception("NOT_IMPL", "IPeer::toPostponed - not implemented."); }
	virtual void deactivate() { throw qbit::exception("NOT_IMPL", "IPeer::deactivate - not implemented."); }
	virtual bool isOutbound() { throw qbit::exception("NOT_IMPL", "IPeer::isOutbound - not implemented."); }
	virtual bool postponedTick() { throw qbit::exception("NOT_IMPL", "IPeer::postponedTick - not implemented."); }

	virtual bool onQuarantine() { throw qbit::exception("NOT_IMPL", "IPeer::onQuarantine - not implemented."); }

	virtual SocketPtr socket() { throw qbit::exception("NOT_IMPL", "IPeer::socket - not implemented."); }
	virtual std::string key() { throw qbit::exception("NOT_IMPL", "IPeer::key - not implemented."); }

	virtual void waitForMessage() { throw qbit::exception("NOT_IMPL", "IPeer::waitForMessage - not implemented."); }
	virtual void ping() { throw qbit::exception("NOT_IMPL", "IPeer::ping - not implemented."); }
	virtual void connect() { throw qbit::exception("NOT_IMPL", "IPeer::connect - not implemented."); }
};

typedef std::shared_ptr<IPeer> IPeerPtr;

} // qbit

#endif
