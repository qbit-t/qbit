// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_ENDORSE_H
#define QBIT_TXBUZZER_ENDORSE_H

#include "txbuzzer.h"

namespace qbit {

//
#define TX_BUZZER_ENDORSE_MY_IN 0
#define TX_BUZZER_ENDORSE_BUZZER_IN 1
#define TX_BUZZER_ENDORSE_FEE_IN 2

#define TX_BUZZER_ENDORSE_FEE_LOCKED_OUT 1

//
class TxBuzzerEndorse: public TxEvent {
public:
	TxBuzzerEndorse() { type_ = TX_BUZZER_ENDORSE; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << score_;
		s << amount_;
		s << buzzerInfo_;
		s << buzzerInfoChain_;
		s << signature_;
	}
	
	inline void deserialize(DataStream& s) {
		s >> timestamp_;
		s >> score_;
		s >> amount_;
		s >> buzzerInfo_;
		s >> buzzerInfoChain_;
		s >> signature_;
	}

	virtual std::string entityName() { return Entity::emptyName(); }
	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior 

	inline uint64_t timestamp() { return timestamp_; }
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	inline uint64_t score() { return score_; }
	inline void setScore(uint64_t score) { score_ = score; }

	inline uint64_t amount() { return amount_; }
	inline void setAmount(uint64_t amount) { amount_ = amount; }

	const uint256& buzzerInfo() const { return buzzerInfo_; }
	void setBuzzerInfo(const uint256& buzzerInfo) { buzzerInfo_ = buzzerInfo; }

	const uint256& buzzerInfoChain() const { return buzzerInfoChain_; }
	void setBuzzerInfoChain(const uint256& buzzerInfoChain) { buzzerInfoChain_ = buzzerInfoChain; }

	const uint512& signature() const { return signature_; }
	void setSignature(const uint512& signature) { signature_ = signature; }

	virtual In& addPublisherBuzzerIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	inline std::string name() { return "buzzer_endorse"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
		props["score"] = strprintf("%d", score_);
		props["amount"] = strprintf("%d", amount_);
		props["buzzer_info"] = buzzerInfo_.toHex();
		props["buzzer_info_chain"] = buzzerInfoChain_.toHex();
		props["signature"] = strprintf("%s", signature_.toHex());
	}

	inline void makeSignature(const SKey& skey, const uint256& publisher) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << timestamp_;
		lSource << score_;
		lSource << amount_;
		lSource << buzzerInfo_;
		lSource << publisher;
		
		uint256 lHash = Hash(lSource.begin(), lSource.end());
		if (!const_cast<SKey&>(skey).sign(lHash, signature_)) {
			throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
		}
	}

	static inline bool verifySignature(const PKey& pkey, unsigned short type, uint64_t timestamp, uint64_t score,
		const uint256& buzzerInfo, amount_t amount, const uint256& publisher, const uint512& signature) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << timestamp;
		lSource << score;
		lSource << amount;
		lSource << buzzerInfo;
		lSource << publisher;

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		return const_cast<PKey&>(pkey).verify(lHash, signature);
	}

	static inline bool extractLockedAmountAndHeight(TxFeePtr fee, amount_t& amount, uint64_t& height) {
		//
		Transaction::Out& lOut = fee->out()[TX_BUZZER_ENDORSE_FEE_LOCKED_OUT];
		VirtualMachine lVM(lOut.destination());
		lVM.execute();

		if (lVM.getR(qasm::QA0).getType() != qasm::QNONE && lVM.getR(qasm::QR1).getType() != qasm::QNONE) {
			amount = lVM.getR(qasm::QA0).to<amount_t>();
			height = lVM.getR(qasm::QR1).to<uint64_t>();

			return true;
		}

		return false;
	}

	static inline bool extractLockedAmountHeightAndAsset(TxFeePtr fee, amount_t& amount, uint64_t& height, uint256& asset) {
		//
		Transaction::Out& lOut = fee->out()[TX_BUZZER_ENDORSE_FEE_LOCKED_OUT];
		VirtualMachine lVM(lOut.destination());
		lVM.execute();

		if (lVM.getR(qasm::QA0).getType() != qasm::QNONE && lVM.getR(qasm::QR1).getType() != qasm::QNONE) {
			asset = lOut.asset();
			amount = lVM.getR(qasm::QA0).to<amount_t>();
			height = lVM.getR(qasm::QR1).to<uint64_t>();

			return true;
		}

		return false;
	}

	static uint64_t calcLockHeight(size_t blockTime) {
		return 30*60 / (blockTime/1000);
	}

private:
	uint64_t timestamp_;
	uint64_t score_;
	uint64_t amount_;
	uint256 buzzerInfo_;
	uint256 buzzerInfoChain_;
	uint512 signature_;
};

typedef std::shared_ptr<TxBuzzerEndorse> TxBuzzerEndorsePtr;

class TxBuzzerEndorseCreator: public TransactionCreator {
public:
	TxBuzzerEndorseCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerEndorse>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerEndorseCreator>(); }
};

}

#endif
