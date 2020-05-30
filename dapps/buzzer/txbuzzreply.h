// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZ_REPLY_H
#define QBIT_TXBUZZ_REPLY_H

#include "txbuzz.h"

namespace qbit {

#define TX_BUZZ_REPLY_MY_IN 0
#define TX_BUZZ_REPLY_BUZZ_IN 1

//
class TxBuzzReply: public TxBuzz {
public:
	TxBuzzReply() { type_ = TX_BUZZ_REPLY; }

	std::string name() { return "buzz_reply"; }

	virtual In& addBuzzIn(const SKey& skey, UnlinkedOutPtr utxo) {
		Transaction::In lIn;
		lIn.out().setNull();
		lIn.out().setChain(utxo->out().chain());
		lIn.out().setAsset(utxo->out().asset());
		lIn.out().setTx(utxo->out().tx());
		lIn.out().setIndex(utxo->out().index());

		lIn.setOwnership(ByteCode() <<
			OP(QDETXO)); // entity/check/push

		in_.push_back(lIn);
		return in_[in_.size()-1];
	}
};

typedef std::shared_ptr<TxBuzzReply> TxBuzzReplyPtr;

class TxBuzzReplyCreator: public TransactionCreator {
public:
	TxBuzzReplyCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzReply>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzReplyCreator>(); }
};

}

#endif
