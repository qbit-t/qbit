// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_STATE_H
#define QBIT_STATE_H

#include "block.h"
#include "tinyformat.h"

namespace qbit {

class State;
typedef std::shared_ptr<State> StatePtr;

class State {
public:
	class BlockInfo {
	public:
		BlockInfo() {}
		BlockInfo(uint32_t height, uint256 hash) : height_(height), hash_(hash) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(height_);
			READWRITE(hash_);
		}

		inline uint32_t height() { return height_; }
		inline uint256 hash() { return hash_; }

	private:
		uint32_t height_;
		uint256 hash_;
	};

public:
	State() {}
	State(uint64_t time) {
		time_ = time;
	}
	State(uint64_t time, PKey pkey) {
		pkey_ = pkey;
		time_ = time;
	}	

	template <typename Stream>
	void serialize(Stream& s, const SKey& key) {
		// data
		s << time_;
		s << infos_;

		// pub key
		pkey_ = const_cast<SKey&>(key).createPKey();
		s << pkey_;

		// prepare blob
		DataStream lStream(SER_NETWORK, CLIENT_VERSION);
		lStream << time_;
		lStream << infos_;

		// calc hash
		uint256 lData = Hash(lStream.begin(), lStream.end());
		// make signature
		const_cast<SKey&>(key).sign(lData, signature_);

		// signature
		s << signature_;
	}

	template <typename Stream>
	void deserialize(Stream& s) {
		s >> time_;
		s >> infos_;
		s >> pkey_;
		s >> signature_;
	}

	void addHeader(BlockHeader& header, uint32_t height) {
		BlockInfo lBlock(height, header.hash());
		infos_.push_back(lBlock);
	}

	PKey address() { return pkey_; }

	inline bool valid() {
		DataStream lStream(SER_NETWORK, CLIENT_VERSION);

		lStream << time_;
		lStream << infos_;

		uint256 lData = Hash(lStream.begin(), lStream.end());
		return pkey_.verify(lData, signature_);
	}

	inline uint32_t height() {
		if (infos_.size()) return infos_[0].height();
		return 0;
	}

	static StatePtr instance(uint64_t time, PKey pkey) { return std::make_shared<State>(time, pkey); }

	std::string toString()
	{
		std::string str;
		str += strprintf("state(time=%u, pkey=%s)\n",
			time_,
			pkey_.toString().c_str());
		
		//for (auto& lInfo : infos_)
		//	str += "    " + strprintf("%d", lInfo.height()) + ":" + lInfo.hash().toString() + "\n";
		return str;
	}

private:
	uint64_t time_;
	std::vector<BlockInfo> infos_;
	PKey pkey_;
	uint512 signature_;
};

}

#endif