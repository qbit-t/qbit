// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZ_REBUZZ_NOTIFY_H
#define QBIT_TXBUZZ_REBUZZ_NOTIFY_H

#include "txbuzz.h"

namespace qbit {

#define TX_REBUZZ_MY_IN 0

//
class TxReBuzzNotify: public TxReBuzz {
public:
	TxReBuzzNotify() { type_ = TX_BUZZ_REBUZZ_NOTIFY; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << score_;
		s << buzzId_;
		s << buzzChainId_;
		s << buzzerId_;
		s << rebuzzChainId_;
		s << rebuzzId_;
	}
	
	virtual void deserialize(DataStream& s) {
		s >> timestamp_;
		s >> score_;
		s >> buzzId_;
		s >> buzzChainId_;
		s >> buzzerId_;
		s >> rebuzzChainId_;
		s >> rebuzzId_;
	}

	std::string name() { return "rebuzz_notify"; }

	virtual bool isFeeFree() { return true; }

	inline uint256 rebuzzChainId() { return rebuzzChainId_; }
	inline void setRebuzzChainId(const uint256& rebuzzChainId) { rebuzzChainId_ = rebuzzChainId; }

	inline uint256 rebuzzId() { return rebuzzId_; }
	inline void setRebuzzId(const uint256& rebuzzId) { rebuzzId_ = rebuzzId; }

	virtual void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
		props["score"] = strprintf("%d", score_);
		props["buzzChainId"] = strprintf("%s", buzzChainId_.toHex());
		props["buzzId"] = strprintf("%s", buzzId_.toHex());
		props["buzzerId"] = strprintf("%s", buzzerId_.toHex());
		props["rebuzzChainId"] = strprintf("%s", rebuzzChainId_.toHex());
		props["rebuzzId"] = strprintf("%s", rebuzzId_.toHex());
	}

private:
	uint256 rebuzzChainId_;
	uint256 rebuzzId_;
};

typedef std::shared_ptr<TxReBuzzNotify> TxReBuzzNotifyPtr;

class TxReBuzzNotifyCreator: public TransactionCreator {
public:
	TxReBuzzNotifyCreator() {}
	TransactionPtr create() { return std::make_shared<TxReBuzzNotify>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxReBuzzNotifyCreator>(); }
};

}

#endif
