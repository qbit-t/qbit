// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ISETTINGS_H
#define QBIT_ISETTINGS_H

#include "key.h"
#include "amount.h"
#include "state.h"

namespace qbit {

extern bool gLightDaemon;

class ISettings {
public:
	ISettings() {}

	virtual std::string dataPath() { return ".qbit"; }
	virtual size_t keysCache() { return 0; }

	virtual qunit_t maxFeeRate() { return QUNIT * 10; } // 10 qunits per byte
	virtual PKey changeKey() { return PKey(); } // for change output

	virtual int serverPort() { return 31415; } // main net
	virtual size_t maxMessageSize() { return 12 * 1024 * 1024; } // max incoming message size

	virtual size_t threadPoolSize() { if (!isClient()) return 4; return 1; } // tread pool size

	virtual uint64_t consensusSynchronizationLatency() { return 30; } // latency in seconds

	virtual uint32_t roles() { return State::PeerRoles::FULLNODE|State::PeerRoles::MINER; } // default role	

	virtual bool isMiner() { uint32_t lRoles(roles()); return (lRoles & State::PeerRoles::MINER) != 0; }
	virtual bool isNode() { uint32_t lRoles(roles());  return (lRoles & State::PeerRoles::NODE) != 0; }
	virtual bool isFullNode() { uint32_t lRoles(roles()); return (lRoles & State::PeerRoles::FULLNODE) != 0; }
	virtual bool isClient() { uint32_t lRoles(roles()); return (lRoles & State::PeerRoles::CLIENT) != 0; }
	virtual bool isDaemon() { uint32_t lRoles(roles()); return (lRoles & State::PeerRoles::DAEMON) != 0; }

	virtual void addMinerRole() {}
	virtual void addNodeRole() {}
	virtual void addFullNodeRole() {}

	virtual int httpServerPort() { return 8080; }
	virtual size_t httpThreadPoolSize() { return 2; } // thread pool size

	virtual size_t incomingBlockQueueLength() { return 50; }

	virtual size_t minDAppNodes() { return 5; }	
	virtual size_t maxShardsByNode() { return 25; }	

	virtual bool useFirstKeyForChange() { return true; }

	virtual size_t mainChainMaturity() { return 1; } // for reqular tx that will be enough
	virtual size_t mainChainCoinbaseMaturity() { return 5; } // for coinbase - we need _much_ more, 50 at least

	virtual size_t clientSessionsLimit() { return 50; } // default client sessions for node\full node
	virtual void setClientSessionsLimit(size_t) {}

	virtual size_t mainChainBlockTime() { return 2000; } // ms

	virtual bool supportAirdrop() { return false; }

	virtual bool qbitOnly() { return false; }
	virtual void setQbitOnly() {}

	virtual void setServerPort(int) {}
	virtual void setThreadPoolSize(size_t) {}
	virtual void setHttpServerPort(int) {}
	virtual void setSupportAirdrop() {}

	virtual void notifyTransaction(const uint256&) {}
	virtual void setNotifyTransactionCommand(const std::string&) {}

	virtual int clientActivePeers() { return 3; }

	virtual bool reindex() { return false; }
	virtual void setReindex() {}
	virtual void resetReindex() {}

	virtual uint256 reindexShard() { return uint256(); }
	virtual void setReindexShard(const uint256&) {}

	virtual bool resync() { return false; }
	virtual void setResync() {}
	virtual void resetResync() {}

	virtual void setProofAsset(const uint256&) {}
	virtual uint256 proofAsset() { return uint256(); }

	virtual void setProofAmount(amount_t) {}
	virtual amount_t proofAmount() { return 0; }

	virtual void setProofFrom(uint64_t) {}
	virtual uint64_t proofFrom() { return 0; }

	virtual std::string userName() { return ""; }
};

typedef std::shared_ptr<ISettings> ISettingsPtr;

} // qbit

#endif