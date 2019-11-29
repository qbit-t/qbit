// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ISETTINGS_H
#define QBIT_ISETTINGS_H

#include "key.h"
#include "amount.h"

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
};

typedef std::shared_ptr<ISettings> ISettingsPtr;

} // qbit

#endif