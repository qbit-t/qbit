// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_STATE_H
#define QBIT_STATE_H

#include "block.h"
#include "tinyformat.h"

#include <atomic>

namespace qbit {

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
		BlockInfo(uint256 chain, uint32_t height, uint256 hash) : chain_(chain), height_(height), hash_(hash) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(chain_);
			READWRITE(height_);
			READWRITE(hash_);
		}

		inline uint32_t height() { return height_; }
		inline uint256 hash() { return hash_; }
		inline uint256 chain() { return chain_; }

	private:
		uint256 chain_;
		uint32_t height_;
		uint256 hash_;
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
		s << infos_;
		s << roles_;

		// pub key
		pkey_ = const_cast<SKey&>(key).createPKey();
		s << pkey_;

		// prepare blob
		DataStream lStream(SER_NETWORK, CLIENT_VERSION);
		lStream << time_;
		lStream << infos_;
		lStream << roles_;

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
		s << infos_;
		s << roles_;
		s << pkey_;
		s << signature_;
	}

	/*
	template <typename Stream>
	void deserialize(Stream& s) {
		s >> time_;
		s >> infos_;
		s >> (uint32_t)roles_;
		s >> pkey_;
		s >> signature_;
	}
	*/

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(time_);
		READWRITE(infos_);
		READWRITE(roles_);
		READWRITE(pkey_);
		READWRITE(signature_);
	}

	void addHeader(BlockHeader& header, uint32_t height) {
		BlockInfo lBlock(header.chain(), height, header.hash());
		infos_.push_back(lBlock);
	}

	PKey address() { return pkey_; }
	uint160 addressId() { return pkey_.id(); }

	uint32_t roles() { return roles_; }

	inline bool valid() {
		DataStream lStream(SER_NETWORK, CLIENT_VERSION);

		lStream << time_;
		lStream << infos_;
		lStream << roles_;

		uint256 lData = Hash(lStream.begin(), lStream.end());
		return pkey_.verify(lData, signature_);
	}

	inline uint32_t height() {
		if (infos_.size()) return infos_[0].height();
		return 0;
	}

	static StatePtr instance(uint64_t time, uint32_t roles, PKey pkey) { return std::make_shared<State>(time, roles, pkey); }
	static StatePtr instance(const State& state) { return std::make_shared<State>(state); }

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

	bool isMinerOrValidator() {
		return ((roles_ & MINER) != 0) || ((roles_ & VALIDATOR) != 0);
	}

	bool containsChain(const uint256& chain) {
		if (!chains_.size()) {
			for (std::vector<BlockInfo>::iterator lInfo = infos_.begin(); lInfo != infos_.end(); lInfo++) {
				chains_.insert(std::map<uint256, BlockInfo>::value_type(lInfo->chain(), *lInfo));
			}
		}

		return chains_.find(chain) != chains_.end();
	}

	bool locateChain(const uint256& chain, BlockInfo& info) {
		if (!chains_.size()) {
			for (std::vector<BlockInfo>::iterator lInfo = infos_.begin(); lInfo != infos_.end(); lInfo++) {
				chains_.insert(std::map<uint256, BlockInfo>::value_type(lInfo->chain(), *lInfo));
			}
		}

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

	std::string toString() {
		std::string str;
		str += strprintf("state(time=%u, roles=%s, pkey=%s, chains=%d)\n",
			time_, rolesString(),
			pkey_.toString(), infos_.size());
		
		for (auto& lInfo : infos_)
			str += "  -> chain(" + strprintf("%d/%s#", lInfo.height(), lInfo.chain().toHex().substr(0, 10)) + ")\n";
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
	std::vector<BlockInfo> infos_;
	uint32_t roles_ {0};
	PKey pkey_;
	uint512 signature_;

	// in-memory
	std::map<uint256, BlockInfo> chains_;
};

}

#endif