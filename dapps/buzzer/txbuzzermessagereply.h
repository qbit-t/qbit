// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_MESSAGE_REPLY_H
#define QBIT_TXBUZZER_MESSAGE_REPLY_H

#include "txbuzz.h"

namespace qbit {

#define TX_BUZZER_MESSAGE_REPLY_MY_IN 0
#define TX_BUZZER_MESSAGE_REPLY_MESSAGE_IN 1

//
class TxBuzzerMessageReply: public TxBuzz {
public:
	TxBuzzerMessageReply() { type_ = TX_BUZZER_MESSAGE_REPLY; }

	std::string name() { return "buzzer_message_reply"; }

	virtual In& addMessageIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

typedef std::shared_ptr<TxBuzzerMessageReply> TxBuzzerMessageReplyPtr;

class TxBuzzerMessageReplyCreator: public TransactionCreator {
public:
	TxBuzzerMessageReplyCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerMessageReply>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerMessageReplyCreator>(); }
};

}

#endif
