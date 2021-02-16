// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CLIENT_SETTINGS_H
#define QBIT_CLIENT_SETTINGS_H

#include "../isettings.h"
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <stdio.h>

namespace qbit {

class ClientSettings: public ISettings {
public:
	ClientSettings(): ClientSettings(".qbit-cli") {}
	ClientSettings(const std::string& dir) {
#if defined(__linux__)
#ifndef MOBILE_PLATFORM
		if (dir.find("/") == std::string::npos) {
			uid_t lUid = geteuid();
			struct passwd *lPw = getpwuid(lUid);
			if (lPw) {
				userName_ = std::string(lPw->pw_name);
			} else {
				char lName[0x100] = {0};
				getlogin_r(lName, sizeof(lName));
				userName_ = std::string(lName);
			}

			path_ = "/home/" + userName_ + "/" + dir;
		} else {
			path_ = dir;
		}
#endif
#endif
	}

	std::string dataPath() { return path_; }
	uint32_t roles() { return State::PeerRoles::CLIENT; }
	qunit_t maxFeeRate() { return QUNIT * 5; }

	static ISettingsPtr instance() { return std::make_shared<ClientSettings>(); }
	static ISettingsPtr instance(const std::string& dir) { return std::make_shared<ClientSettings>(dir); }

private:
	std::string path_;
	std::string userName_;
};

} // qbit

#endif
