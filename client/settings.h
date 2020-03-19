// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CLIENT_SETTINGS_H
#define QBIT_CLIENT_SETTINGS_H

#include "../isettings.h"
#include <unistd.h>

namespace qbit {

class ClientSettings: public ISettings {
public:
	ClientSettings() {
#if defined(__linux__)
		char lName[0x100] = {0};
		getlogin_r(lName, sizeof(lName));
		path_ = "/home/" + std::string(lName) + "/.qbit-cli";
#endif
	}

	std::string dataPath() { return path_; }
	uint32_t roles() { return State::PeerRoles::CLIENT; }

	static ISettingsPtr instance() { return std::make_shared<ClientSettings>(); }

private:
	std::string path_;
};

} // qbit

#endif