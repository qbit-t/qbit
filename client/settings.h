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
	ClientSettings(): ClientSettings(".qbit-cli") {}
	ClientSettings(const std::string& dir) {
#if defined(__linux__)
#ifndef MOBILE_PLATFORM
		char lName[0x100] = {0};
		getlogin_r(lName, sizeof(lName));
		path_ = "/home/" + std::string(lName) + "/" + dir;
#endif
#endif
	}

	std::string dataPath() { return path_; }
	uint32_t roles() { return State::PeerRoles::CLIENT; }

	static ISettingsPtr instance() { return std::make_shared<ClientSettings>(); }
	static ISettingsPtr instance(const std::string& dir) { return std::make_shared<ClientSettings>(dir); }

private:
	std::string path_;
};

} // qbit

#endif
