// Copyright (c) 2019-2022 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZERGROUP_H
#define QBIT_TXBUZZERGROUP_H

#include "txbuzzer.h"

namespace qbit {

#define TX_BUZZER_GROUP_IN 0

#define TX_BUZZER_GROUP_OUT 0
#define TX_BUZZER_GROUP_INVITE_OUT 1
#define TX_BUZZER_GROUP_KEYS_OUT 2

//
class TxBuzzerGroup: public Entity {
public:
	TxBuzzer() { type_ = TX_BUZZER_GROUP; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		buzzer_name_t lName(name_);
		lName.serialize(s);
	}
	
	inline void deserialize(DataStream& s) {
		buzzer_name_t lName(name_);
		lName.deserialize(s);
	}

	inline std::string& myName() { return name_; }
	inline void setMyName(const std::string& name) { name_ = name; }
	virtual std::string entityName() { return name_; }

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["entity"] = entityName();
	}

	//
	Transaction::UnlinkedOutPtr addBuzzerGroupOut(const SKey& skey, const PKey& pkey) {
		//
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
			OP(QEQADDR) 	<<
			OP(QPEN) 		<<
			OP(QPTXO)		<< // use in entity-based pushUnlinkedOut's
			OP(QMOV) 		<< REG(QR0) << CU8(0x01) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(MainChain::id(), TxAssetType::nullAsset(), out_.size()), // link
			pkey
		);

		// fill up for finalization
		assetOut_[TxAssetType::nullAsset()].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}

	//
	Transaction::UnlinkedOutPtr addBuzzerGroupInviteOut(const SKey& skey, const PKey& pkey) {
		//
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
			OP(QEQADDR) 	<<
			OP(QPEN) 		<<
			OP(QPTXO)		<< // use in entity-based pushUnlinkedOut's
			OP(QMOV)		<< REG(QR1) << CU16(TX_BUZZER_GROUP_INVITE) <<
			OP(QCMPE)		<< REG(QTH1) << REG(QR1) <<
			OP(QMOV) 		<< REG(QR0) << REG(QC0) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(MainChain::id(), TxAssetType::nullAsset(), out_.size()), // link
			pkey
		);

		// fill up for finalization
		assetOut_[TxAssetType::nullAsset()].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}

	//
	Transaction::UnlinkedOutPtr addBuzzerGroupKeysOut(const SKey& skey, const PKey& pkey) {
		//
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
			OP(QEQADDR) 	<<
			OP(QPEN) 		<<
			OP(QPTXO)		<< // use in entity-based pushUnlinkedOut's
			OP(QMOV)		<< REG(QR1) << CU16(TX_BUZZER_GROUP_KEYS) <<
			OP(QCMPE)		<< REG(QTH1) << REG(QR1) <<
			OP(QMOV) 		<< REG(QR0) << REG(QC0) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(MainChain::id(), TxAssetType::nullAsset(), out_.size()), // link
			pkey
		);

		// fill up for finalization
		assetOut_[TxAssetType::nullAsset()].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}

	virtual In& addDAppIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	virtual In& addShardIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	inline std::string name() { return "buzzer_group"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	

	inline bool extractAddress(PKey& pkey) {
		//
		Transaction::Out& lOut = (*out().begin());
		VirtualMachine lVM(lOut.destination());
		lVM.execute();

		if (lVM.getR(qasm::QD0).getType() != qasm::QNONE) {		
			pkey.set<unsigned char*>(lVM.getR(qasm::QD0).begin(), lVM.getR(qasm::QD0).end());

			return true;
		}

		return false;
	}

private:
	std::string name_;
};

typedef std::shared_ptr<TxBuzzerGroup> TxBuzzerGroupPtr;

class TxBuzzerGroupCreator: public TransactionCreator {
public:
	TxBuzzerGroupCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerGroup>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerGroupCreator>(); }
};

}

#endif
