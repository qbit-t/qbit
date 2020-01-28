// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ISETTINGS_H
#define QBIT_ISETTINGS_H

#include "key.h"
#include "amount.h"
#include "state.h"

namespace qbit {

class ISettings {
public:
	ISettings() {}

	virtual std::string dataPath() { return "~/.qbit"; }
	virtual size_t keysCache() { return 0; }

	virtual qunit_t maxFeeRate() { return QUNIT * 10; } // 10 qunits per byte
	virtual PKey changeKey() { return PKey(); } // for change output

	virtual int serverPort() { return 31415; } // main net
	virtual size_t maxMessageSize() { return 1024 * 1024; } // max incoming message size

	virtual size_t threadPoolSize() { return 4; } // tread pool size

	virtual uint64_t consensusSynchronizationLatency() { return 30; } // latency in seconds

	virtual uint32_t roles() { return State::PeerRoles::FULLNODE|State::PeerRoles::MINER; } // default role	

	virtual bool isMiner() { uint32_t lRoles(roles()); return (lRoles & State::PeerRoles::MINER) != 0; }
	virtual bool isNode() { uint32_t lRoles(roles());  return (lRoles & State::PeerRoles::NODE) != 0; }
	virtual bool isFullNode() { uint32_t lRoles(roles()); return (lRoles & State::PeerRoles::FULLNODE) != 0; }
	virtual bool isClient() { uint32_t lRoles(roles()); return (lRoles & State::PeerRoles::CLIENT) != 0; }

	virtual int httpServerPort() { return 8080; }
	virtual size_t httpThreadPoolSize() { return 2; } // tread pool size

	virtual size_t incomingBlockQueueLength() { return 50; }
};

typedef std::shared_ptr<ISettings> ISettingsPtr;

} // qbit

#endif