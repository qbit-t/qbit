// Copyright (c) 2019-2022 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_UNBLOCK_H
#define QBIT_TXBUZZER_UNBLOCK_H

#include "txbuzzer.h"

namespace qbit {

#define TX_BUZZER_MY_BUZZER_IN 0

//
class TxBuzzerUnBlock: public TxEvent {
public:
	TxBuzzerUnBlock() { type_ = TX_BUZZER_UNBLOCK; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << buzzer_;
	}
	
	inline void deserialize(DataStream& s) {
		s >> timestamp_;
		s >> buzzer_;
	}

	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }
	inline uint64_t timestamp() { return timestamp_; }

	inline void setBuzzer(const uint256& buzzer) { buzzer_ = buzzer; }
	inline const uint256& buzzer() { return buzzer_; }
	
	virtual bool isFeeFree() { return true; }

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

	inline std::string name() { return "buzzer_unblock"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
		props["buzzer"] = buzzer_.toHex();
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

private:
	uint64_t timestamp_;
	uint256 buzzer_;
};

typedef std::shared_ptr<TxBuzzerUnBlock> TxBuzzerUnBlockPtr;

class TxBuzzerUnBlockCreator: public TransactionCreator {
public:
	TxBuzzerUnBlockCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerUnBlock>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerUnBlockCreator>(); }
};

}

#endif
