// Copyright (c) 2019-2024 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_GROUP_INVITE_ACCEPT_H
#define QBIT_TXBUZZER_GROUP_INVITE_ACCEPT_H

#include "txbuzzer.h"

#define TX_BUZZER_GROUP_INVITE_ACCEPT_MY_IN 0
#define TX_BUZZER_GROUP_INVITE_ACCEPT_IN 1

namespace qbit {

//
class TxBuzzerGroupInviteAccept: public TxBuzzerGroupInvite {
public:
	TxBuzzerGroupInviteAccept() { type_ = TX_BUZZER_GROUP_INVITE_ACCEPT; }

	inline std::string name() { return "buzzer_group_invite_accept"; }

	virtual bool isFeeFree() { return true; }

	inline void makeSignature(const SKey& skey, const uint256& buzzer) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << timestamp_;
		lSource << buzzer;

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		if (!const_cast<SKey&>(skey).sign(lHash, signature_)) {
			throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
		}
	}

	inline static bool verifySignature(const PKey& pkey, uint64_t timestamp, const uint256& buzzer, const uint512& signature) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << timestamp;
		lSource << buzzer;

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		return const_cast<PKey&>(pkey).verify(lHash, signature);
	}

	//
	Transaction::UnlinkedOutPtr addConfirmOut(const SKey& skey, const PKey& pkey) {
		//
		return addSpecialOut(skey, pkey, TX_BUZZER_GROUP_INVITE_CONFIRM_OUT);
	}

	//
	virtual In& addGroupIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

typedef std::shared_ptr<TxBuzzerGroupInviteAccept> TxBuzzerGroupInviteAcceptPtr;

class TxBuzzerGroupInviteAcceptCreator: public TransactionCreator {
public:
	TxBuzzerGroupInviteAcceptCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerGroupInviteAccept>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerGroupInviteAcceptCreator>(); }
};

}

#endif
