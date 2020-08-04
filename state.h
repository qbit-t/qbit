// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_STATE_H
#define QBIT_STATE_H

#include "block.h"
#include "tinyformat.h"
#include "log/log.h"

#include <atomic>

namespace qbit {

typedef LimitedString<64> dapp_name_t;
typedef LimitedString<256> device_id_t;

class State;
typedef std::shared_ptr<State> StatePtr;

class State {
public:
	enum PeerRoles: uint32_t {
		UNDEFINED	= 0,
		FULLNODE	= (1 <<  0),
		MINER		= (1 <<  1),
		VALIDATOR	= (1 <<  2),
		CLIENT 		= (1 <<  3),
		NODE 		= (1 <<  4),
		ALL			= ~(uint32_t)0
	};	

public:
	class BlockInfo {
	public:
		BlockInfo() {}
		BlockInfo(const uint256& chain, uint64_t height, const uint256& hash) : chain_(chain), height_(height), hash_(hash) {}
		BlockInfo(const uint256& chain, uint64_t height, const uint256& hash, const std::string& dApp) : chain_(chain), height_(height), hash_(hash), dApp_(dApp) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(chain_);
			READWRITE(height_);
			READWRITE(hash_);

			if (ser_action.ForRead()) {
				dapp_name_t lName(dApp_);
				lName.deserialize(s);
			} else {
				dapp_name_t lName(dApp_);
				lName.serialize(s);
			}
		}

		inline uint64_t height() { return height_; }
		inline uint256 hash() { return hash_; }
		inline uint256 chain() { return chain_; }
		inline std::string dApp() { return dApp_; }

	private:
		uint256 chain_;
		uint64_t height_;
		uint256 hash_;
		std::string dApp_;
	};

public:
	class DAppInstance {
	public:
		DAppInstance() {}
		DAppInstance(const std::string& name, const uint256& instance) : name_(name), instance_(instance) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			//
			if (ser_action.ForRead()) {
				dapp_name_t lName(name_);
				lName.deserialize(s);
			} else {
				dapp_name_t lName(name_);
				lName.serialize(s);
			}

			READWRITE(instance_);
		}

		inline const std::string& name() const { return name_; }
		inline const uint256& instance() const { return instance_; }

	private:
		std::string name_;
		uint256 instance_;
	};

public:
	State() {}
	State(uint64_t time, uint32_t roles, PKey pkey) {
		time_ = time;
		roles_ = roles;
		pkey_ = pkey;
	}

	template <typename Stream>
	void serialize(Stream& s, const SKey& key) {
		// data
		s << time_;
		s << roles_;

		if (client()) {
			s << dAppInstance_;
		} else {
			s << infos_;
		}

		// pub key
		pkey_ = const_cast<SKey&>(key).createPKey();
		s << pkey_;

		// prepare blob
		DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
		lStream << time_;
		lStream << roles_;
		if (client()) {
			lStream << dAppInstance_;
		} else {
			lStream << infos_;
		}

		// calc hash
		uint256 lData = Hash(lStream.begin(), lStream.end());
		// make signature
		const_cast<SKey&>(key).sign(lData, signature_);

		// signature
		s << signature_;
	}

	template <typename Stream>
	void serialize(Stream& s) {
		// data
		s << time_;
		s << roles_;
		if (client()) {
			s << dAppInstance_;
		} else {
			s << infos_;
		}
		s << pkey_;
		s << signature_;
	}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(time_);
		READWRITE(roles_);
		if (client()) {
			READWRITE(dAppInstance_);
		} else {
			READWRITE(infos_);
		}
		READWRITE(pkey_);
		READWRITE(signature_);
	}

	void addHeader(BlockHeader& header, uint64_t height, const std::string& dapp) {
		BlockInfo lBlock(header.chain(), height, header.hash(), dapp);
		infos_.push_back(lBlock);
	}

	PKey address() { return pkey_; }
	uint160 addressId() { return pkey_.id(); }

	uint32_t roles() { return roles_; }

	inline bool valid() {
		DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);

		lStream << time_;
		lStream << roles_;
		if (client()) {
			lStream << dAppInstance_;
		} else {
			lStream << infos_;
		}

		uint256 lData = Hash(lStream.begin(), lStream.end());
		return pkey_.verify(lData, signature_);
	}

	inline uint64_t height() {
		if (infos_.size()) return infos_[0].height();
		return 0;
	}

	//static StatePtr instance(uint64_t time, uint32_t roles, PKey pkey, const std::string& dApp, const uint256& dAppInstance) { return std::make_shared<State>(time, roles, pkey, dApp, dAppInstance); }
	static StatePtr instance(uint64_t time, uint32_t roles, PKey pkey) { return std::make_shared<State>(time, roles, pkey); }
	static StatePtr instance(const State& state) { 
		StatePtr lState = std::make_shared<State>(state);
		lState->prepare();
		return lState;
	}

	std::string rolesString() {
		std::string lRoles = "";
		if (!roles_) lRoles += "UNDEFINED";
		if ((roles_ & FULLNODE) != 0) { if (lRoles.size()) lRoles += "|"; lRoles += "FULLNODE"; }
		if ((roles_ & MINER) != 0) { if (lRoles.size()) lRoles += "|"; lRoles += "MINER"; }
		if ((roles_ & VALIDATOR) != 0) { if (lRoles.size()) lRoles += "|"; lRoles += "VALIDATOR"; }
		if ((roles_ & CLIENT) != 0) { if (lRoles.size()) lRoles += "|"; lRoles += "CLIENT"; }
		if ((roles_ & NODE) != 0) { if (lRoles.size()) lRoles += "|"; lRoles += "NODE"; }

		return lRoles;
	}

	bool client() { return (roles_ & CLIENT) != 0; }
	bool minerOrValidator() {
		return ((roles_ & MINER) != 0) || ((roles_ & VALIDATOR) != 0);
	}
	bool nodeOrFullNode() {
		return ((roles_ & FULLNODE) != 0) || ((roles_ & NODE) != 0);
	}

	void prepare() {
		//
		if (!chains_.size()) {
			for (std::vector<BlockInfo>::iterator lInfo = infos_.begin(); lInfo != infos_.end(); lInfo++) {
				chains_.insert(std::map<uint256, BlockInfo>::value_type(lInfo->chain(), *lInfo));
			}
		}

		if (!dApps_.size() && dAppInstance_.size()) {
			for (std::vector<DAppInstance>::iterator lInstance = dAppInstance_.begin(); lInstance != dAppInstance_.end(); lInstance++) {
				dApps_.insert(lInstance->name());
			}
		}
	}

	bool containsChain(const uint256& chain) {
		prepare();
		return chains_.find(chain) != chains_.end();
	}

	bool locateChain(const uint256& chain, BlockInfo& info) {
		prepare();
		std::map<uint256, BlockInfo>::iterator lChain = chains_.find(chain);
		if (lChain != chains_.end())
		{
			info = lChain->second;
			return true;
		}

		return false;
	}

	bool equals(StatePtr other) {
		return (other->signature() == signature_);
	}

	uint512& signature() { return signature_; }

	std::vector<BlockInfo>& infos() { return infos_; }

	// NOTICE: that is normal, because user conection should handle very little dapps in one time (i.e. buzzer, decos)
	inline bool containsDApp(const std::string& name) {
		for (std::vector<DAppInstance>::iterator lItem = dAppInstance_.begin(); lItem != dAppInstance_.end(); lItem++) {
			if (lItem->name() == name) return true;
		}

		return false;
	}

	void addDAppInstance(const DAppInstance& instance) {
		//
		if (dApps_.find(instance.name()) == dApps_.end()) {
			dApps_.insert(instance.name());
			dAppInstance_.push_back(instance);
		}
	}

	const std::vector<DAppInstance>& dApps() const {
		return dAppInstance_;
	}

	std::string dAppInstancesToString() {
		std::string lResult;
		for (std::vector<DAppInstance>::iterator lItem = dAppInstance_.begin(); lItem != dAppInstance_.end(); lItem++) {
			//
			if (lResult.size()) lResult += ",";
			lResult += (*lItem).instance().toHex();
		}

		return lResult;
	}

	std::string dAppsToString() {
		std::string lResult;
		for (std::vector<DAppInstance>::iterator lItem = dAppInstance_.begin(); lItem != dAppInstance_.end(); lItem++) {
			//
			if (lResult.size()) lResult += ",";
			lResult += (*lItem).name();
		}

		return lResult;
	}

	std::string toString() {
		std::string str;
		if (client()) {
			str += strprintf("state(time=%u, roles=%s, pkey=%s, id=%s, dapp=%s/%s)",
				time_, rolesString(),
				pkey_.toString(), addressId().toHex(), dAppsToString(), dAppInstancesToString());
		} else {
			str += strprintf("state(time=%u, roles=%s, pkey=%s, id=%s, chains=%d)\n",
				time_, rolesString(),
				pkey_.toString(), addressId().toHex(), infos_.size());
			
			for (auto& lInfo : infos_)
				str += "  -> chain(" + strprintf("%s, %d/%s#", lInfo.dApp().size() ? lInfo.dApp() : "none", lInfo.height(), lInfo.chain().toHex().substr(0, 10)) + ")\n";
		}

		return str;
	}

	std::string toStringShort() {
		std::string str;
		str += strprintf("state(time=%u, roles=%s, pkey=%s, chains=%d)\n",
			time_, rolesString(),
			pkey_.toString(), infos_.size());
		return str;
	}

private:
	uint64_t time_;
	uint32_t roles_ {0};
	std::vector<BlockInfo> infos_; // for node & full node
	std::vector<DAppInstance> dAppInstance_; // for client only
	std::string device_; // for client only: client's device id for notificaion purposes
	PKey pkey_;
	uint512 signature_;

	// in-memory
	std::map<uint256, BlockInfo> chains_;
	std::set<std::string> dApps_;
};

}

#endif