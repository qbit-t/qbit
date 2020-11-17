// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_DECLINE_CONVERSATION_H
#define QBIT_TXBUZZER_DECLINE_CONVERSATION_H

#include "txbuzzer.h"

#define TX_BUZZER_CONVERSATION_DECLINE_MY_IN 0
#define TX_BUZZER_CONVERSATION_DECLINE_IN 1

namespace qbit {

//
class TxBuzzerDeclineConversation: public TxBuzzerConversation {
public:
	TxBuzzerDeclineConversation() { type_ = TX_BUZZER_DECLINE_CONVERSATION; }

	inline std::string name() { return "buzzer_accept_conversation"; }

	virtual bool isFeeFee() { return true; }

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

		//std::cout << "[TxBuzzerDeclineConversation/verifySignature] " << timestamp << " " << buzzer.toHex() << "\n";		

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		return const_cast<PKey&>(pkey).verify(lHash, signature);
	}

	//
	virtual In& addConversationIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

typedef std::shared_ptr<TxBuzzerDeclineConversation> TxBuzzerDeclineConversationPtr;

class TxBuzzerDeclineConversationCreator: public TransactionCreator {
public:
	TxBuzzerDeclineConversationCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerDeclineConversation>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerDeclineConversationCreator>(); }
};

}

#endif
