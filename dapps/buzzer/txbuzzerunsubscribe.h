// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_UNSUBSCRIBE_H
#define QBIT_TXBUZZER_UNSUBSCRIBE_H

#include "txbuzzer.h"

namespace qbit {

//
class TxBuzzerUnsubscribe: public Entity {
public:
	TxBuzzerUnsubscribe() { type_ = TX_BUZZER_UNSUBSCRIBE; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
	}
	
	inline void deserialize(DataStream& s) {
		s >> timestamp_;
	}

	virtual std::string entityName() { return Entity::emptyName(); }
	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior 
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	virtual bool isFeeFee() { return true; }

	virtual In& addSubscriptionIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	inline std::string name() { return "buzzer_unsubscribe"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
	}

private:
	uint64_t timestamp_;
};

typedef std::shared_ptr<TxBuzzerUnsubscribe> TxBuzzerUnsubscribePtr;

class TxBuzzerUnsubscribeCreator: public TransactionCreator {
public:
	TxBuzzerUnsubscribeCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerUnsubscribe>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerUnsubscribeCreator>(); }
};

}

#endif
