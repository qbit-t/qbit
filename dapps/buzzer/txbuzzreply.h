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

		qbit::vector<unsigned char> lSource;
		lIn.out().serialize(lSource);
		
		uint256 lHash = Hash(lSource.begin(), lSource.end());
		uint512 lSig;

		if (!const_cast<SKey&>(skey).sign(lHash, lSig)) {
			throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
		}

		PKey lPKey = const_cast<SKey&>(skey).createPKey(); // pkey is allways the same
		lIn.setOwnership(ByteCode() <<
			OP(QMOV) 		<< REG(QS0) << CVAR(lPKey.get()) << 
			OP(QMOV) 		<< REG(QS1) << CU512(lSig) <<
			OP(QLHASH256) 	<< REG(QS2) <<
			OP(QCHECKSIG)	<<
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
