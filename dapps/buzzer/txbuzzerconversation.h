// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_CONVERSATION_H
#define QBIT_TXBUZZER_CONVERSATION_H

#include "txbuzzer.h"

#define TX_BUZZER_CONVERSATION_MY_IN 0
#define TX_BUZZER_CONVERSATION_BUZZER_IN 1

#define TX_BUZZER_CONVERSATION_ACCEPT_OUT 0
#define TX_BUZZER_CONVERSATION_DECLINE_OUT 1
#define TX_BUZZER_CONVERSATION_MESSAGE_OUT 2

namespace qbit {

//
class TxBuzzerConversation: public TxEvent {
public:
	TxBuzzerConversation() { type_ = TX_BUZZER_CONVERSATION; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << counterparty_;
		s << signature_;
	}
	
	inline void deserialize(DataStream& s) {
		s >> timestamp_;
		s >> counterparty_;
		s >> signature_;
	}

	virtual std::string entityName() { return Entity::emptyName(); }
	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior 
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }
	inline uint64_t timestamp() { return timestamp_; }

	void setCountepartyAddress(const PKey& pkey) {
		//
		counterparty_.set<const unsigned char*>(pkey.begin(), pkey.end());
	}

	void initiatorAddress(PKey& pkey) {
		extractAddress(pkey);
	}

	void counterpartyAddress(PKey& pkey) {
		pkey.set<const unsigned char*>(counterparty_.begin(), counterparty_.end());
	}

	/*
	const uint256& buzzerInfo() const { return buzzerInfo_; }
	void setBuzzerInfo(const uint256& buzzerInfo) { buzzerInfo_ = buzzerInfo; }

	const uint256& buzzerInfoChain() const { return buzzerInfoChain_; }
	void setBuzzerInfoChain(const uint256& buzzerInfoChain) { buzzerInfoChain_ = buzzerInfoChain; }
	*/

	const uint512& signature() const { return signature_; }
	void setSignature(const uint512& signature) { signature_ = signature; }

	virtual bool isFeeFee() { return false; }

	inline void makeSignature(const SKey& skey, const uint256& creator, const uint256& counterparty) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << creator;
		lSource << counterparty;

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		if (!const_cast<SKey&>(skey).sign(lHash, signature_)) {
			throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
		}
	}

	inline static bool verifySignature(const PKey& pkey, uint64_t timestamp,
		const uint256& creator, const uint256& counterparty, const uint512& signature) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << creator;
		lSource << counterparty;

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		return const_cast<PKey&>(pkey).verify(lHash, signature);
	}

	//
	Transaction::UnlinkedOutPtr addAcceptOut(const SKey& skey, const PKey& pkey) {
		//
		return addSpecialOut(skey, pkey, TX_BUZZER_ACCEPT_CONVERSATION);
	}

	//
	Transaction::UnlinkedOutPtr addDeclineOut(const SKey& skey, const PKey& pkey) {
		//
		return addSpecialOut(skey, pkey, TX_BUZZER_DECLINE_CONVERSATION);
	}

	//
	Transaction::UnlinkedOutPtr addMessageOut(const SKey& skey, const PKey& pkey) {
		//
		return addSpecialOut(skey, pkey, TX_BUZZER_MESSAGE);
	}

	//
	virtual In& addBuzzerIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	//
	virtual In& addMyBuzzerIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	inline std::string name() { return "buzzer_init_conversation"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
		props["signature"] = strprintf("%s", signature_.toHex());

		PKey lPKey;
		extractAddress(lPKey);

		props["initiator"] = lPKey.toString();
		props["counterparty"] = counterparty_.toString();
	}

private:
	//
	Transaction::UnlinkedOutPtr addSpecialOut(const SKey& skey, const PKey& pkey, unsigned short specialOut) {
		//
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QPTXO)		<< // use in entity-based pushUnlinkedOut's
			OP(QMOV)		<< REG(QR1) << CU16(specialOut) <<
			OP(QCMPE)		<< REG(QTH1) << REG(QR1) <<
			OP(QMOV) 		<< REG(QR0) << REG(QC0) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(MainChain::id(), TxAssetType::nullAsset(), out_.size()), // link
			pkey
		);

		out_.push_back(lOut);
		return lUTXO;
	}

	inline bool extractAddress(PKey& pkey) {
		//
		Transaction::In& lIn = (*in().begin());
		VirtualMachine lVM(lIn.ownership());
		lVM.execute();

		if (lVM.getR(qasm::QS0).getType() != qasm::QNONE) {		
			pkey.set<unsigned char*>(lVM.getR(qasm::QS0).begin(), lVM.getR(qasm::QS0).end());

			return true;
		}

		return false;
	}	

protected:
	uint64_t timestamp_;
	uint512 signature_;
	PKey counterparty_;
};

typedef std::shared_ptr<TxBuzzerConversation> TxBuzzerConversationPtr;

class TxBuzzerConversationCreator: public TransactionCreator {
public:
	TxBuzzerConversationCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerConversation>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerConversationCreator>(); }
};

}

#endif
