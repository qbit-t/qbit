// Copyright (c) 2019-2022 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_RULE_H
#define QBIT_TXBUZZER_RULE_H

#include "txbuzzer.h"

namespace qbit {

//
#define TX_BUZZER_RULE_MY_IN 0
//
#define TX_BUZZER_RULE_OUT 0

//
class TxBuzzerRule: public TxEvent {
public:
	TxBuzzerRule() { type_ = TX_BUZZER_RULE; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
	}
	
	inline void deserialize(DataStream& s) {
		s >> timestamp_;
	}

	virtual std::string entityName() { return Entity::emptyName(); }
	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior 

	inline uint64_t timestamp() { return timestamp_; }
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

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

	virtual Transaction::UnlinkedOutPtr addRuleNOut(const SKey& skey, const PKey& pkey, unsigned short specialOut, const std::list<uint256>& list) {
		//
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		ByteCode lByteCode = ByteCode() << OP(QPTXO);

		for (std::list<uint256>::const_iterator lItem = list.begin(); lItem != list.end(); lItem++) {
			//
			lByteCode 	<<
			OP(QMOV)	<< REG(QL0) << CU256(*lItem) <<
			OP(QCMPE)	<< REG(QTH0) << REG(QL0) <<
			OP(QJMPT) 	<< TO(1000);
		}

		lByteCode <<	OP(QJMP) 	<< TO(1001) <<	// false
		LAB(1000) <<	OP(QMOV) 	<< REG(QR3) << CU8(0x01) <<	// we have successfully checked all the conditions - R3 = 0x01
						OP(QMOV)	<< REG(QR1) << CU16(specialOut) <<
						OP(QCMPE)	<< REG(QTH1) << REG(QR1) <<
						OP(QMOV) 	<< REG(QR0) << REG(QC0) <<
		LAB(1001) <<	OP(QRET);

		lOut.setDestination(lByteCode);

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(MainChain::id(), TxAssetType::nullAsset(), out_.size()), // link
			pkey
		);

		out_.push_back(lOut);
		return lUTXO;
	}

	virtual bool extractList(std::list<uint256>& list) {
		//
		Transaction::Out& lOut = (*out().begin());
		VirtualMachine lVM(lOut.destination());
		lVM.execute();

		if (lVM.getLog().size()) {		
			list.insert(list.end(), lVM.getLog().begin(), lVM.getLog().end());
			return true;
		}

		return false;		
	}

	virtual std::string name() { return "buzzer_rule"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	

	virtual void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
	}

protected:
	uint64_t timestamp_;
};

typedef std::shared_ptr<TxBuzzerRule> TxBuzzerRulePtr;

class TxBuzzerRuleCreator: public TransactionCreator {
public:
	TxBuzzerRuleCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerRule>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerRuleCreator>(); }
};

}

#endif
