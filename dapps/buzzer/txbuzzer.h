// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_H
#define QBIT_TXBUZZER_H

#include "../../amount.h"
#include "../../entity.h"
#include "../../txassettype.h"
#include "../../vm/vm.h"

#if defined (CLIENT_PLATFORM)
    #include <QString>
    #include <QObject>
#endif

namespace qbit {

typedef LimitedString<64> buzzer_name_t;

#define TX_BUZZER 				Transaction::CUSTOM_00 // main chain: entity, fees
#define TX_BUZZ 				Transaction::CUSTOM_01 // shard: fees
#define TX_BUZZER_SUBSCRIBE 	Transaction::CUSTOM_02 // shard: fee-free
#define TX_BUZZER_UNSUBSCRIBE 	Transaction::CUSTOM_03 // shard: fee-free 
#define TX_BUZZER_ENDORSE 		Transaction::CUSTOM_04 // ?
#define TX_BUZZER_MISTRUST		Transaction::CUSTOM_05 // ?
#define TX_REBUZZ				Transaction::CUSTOM_06 // shard: fees
#define TX_BUZZ_LIKE			Transaction::CUSTOM_07 // shard: fee-free
#define TX_BUZZ_REPLY			Transaction::CUSTOM_08 // shard: fees
#define TX_BUZZ_PIN				Transaction::CUSTOM_09 // shard: fee-free
#define TX_BUZZ_REBUZZ_NOTIFY	Transaction::CUSTOM_10 // shard: fee-free, action
#define TX_BUZZER_INFO			Transaction::CUSTOM_11 //
#define TX_BUZZ_REWARD			Transaction::CUSTOM_12 //
#define TX_REBUZZ_REPLY			Transaction::CUSTOM_13 // dummy

#define TX_BUZZER_ALIAS_SIZE 64 
#define TX_BUZZER_DESCRIPTION_SIZE 256 

#define TX_BUZZER_SHARD_IN 1

#define TX_BUZZER_MY_OUT 0
#define TX_BUZZER_SUBSCRIPTION_OUT 1
#define TX_BUZZER_ENDORSE_OUT 2
#define TX_BUZZER_MISTRUST_OUT 3
#define TX_BUZZER_REPLY_OUT 4
#define TX_BUZZER_BUZZ_OUT 5

//
// Endorse/mistrust model parameters
#define BUZZER_TRUST_SCORE_BASE QBIT
#define BUZZER_ENDORSE_LIKE		1000 
#define BUZZER_ENDORSE_REBUZZ	10000
#define BUZZER_MIN_EM_STEP		10000000

//
class TxEvent: public Entity {
public:
	TxEvent() { type_ = Transaction::UNDEFINED; }

	virtual std::string entityName() { return Entity::emptyName(); }
	virtual uint64_t timestamp() { throw qbit::exception("NOT_IMPL", "TxEvent::timestamp - Not implemented."); }
	virtual uint64_t score() { throw qbit::exception("NOT_IMPL", "TxEvent::score - Not implemented."); }

	inline bool extractSpecialType(Transaction::Out& out, unsigned short& type) {
		//
		VirtualMachine lVM(out.destination());
		lVM.execute();

		// special out
		if (lVM.getR(qasm::QR1).getType() != qasm::QNONE) {		
			type = lVM.getR(qasm::QR1).to<unsigned short>();
			return true;
		}

		return false;
	}	
};
typedef std::shared_ptr<TxEvent> TxEventPtr;

class BuzzerMediaPointer {
public:
	BuzzerMediaPointer() { chain_.setNull(); tx_.setNull(); }
	BuzzerMediaPointer(const uint256& chain, const uint256& tx) : chain_(chain), tx_(tx) {}

	void set(const BuzzerMediaPointer& other) {
		chain_ = other.chain();
		tx_ = other.tx();
	}

	const uint256& chain() const { return chain_; }
	const uint256& tx() const { return tx_; }

	std::string url() const {
		return strprintf("%s/%s", tx_.toHex(), chain_.toHex());
	}

	void setChain(const uint256& chain) { chain_ = chain; }
	void setTx(const uint256& tx) { tx_ = tx; }

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(chain_);
		READWRITE(tx_);
	}

protected:
	mutable uint256 chain_;
	mutable uint256 tx_;
};

//
class TxBuzzer: public Entity {
public:
	TxBuzzer() { type_ = TX_BUZZER; }

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
	Transaction::UnlinkedOutPtr addBuzzerOut(const SKey& skey, const PKey& pkey) {
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
	Transaction::UnlinkedOutPtr addBuzzerSubscriptionOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzerSpecialOut(skey, pkey, TX_BUZZER_SUBSCRIBE);
	}

	Transaction::UnlinkedOutPtr addBuzzerEndorseOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzerSpecialOut(skey, pkey, TX_BUZZER_ENDORSE);
	}

	Transaction::UnlinkedOutPtr addBuzzerMistrustOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzerSpecialOut(skey, pkey, TX_BUZZER_MISTRUST);
	}

	Transaction::UnlinkedOutPtr addBuzzerReplyOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzerSpecialOut(skey, pkey, TX_BUZZ_REPLY);
	}

	Transaction::UnlinkedOutPtr addBuzzerBuzzOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzerSpecialOut(skey, pkey, TX_BUZZ);
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

	inline std::string name() { return "buzzer"; }

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
	//
	Transaction::UnlinkedOutPtr addBuzzerSpecialOut(const SKey& skey, const PKey& pkey, unsigned short specialOut) {
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

private:
	std::string name_;
};

typedef std::shared_ptr<TxBuzzer> TxBuzzerPtr;

class TxBuzzerCreator: public TransactionCreator {
public:
	TxBuzzerCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzer>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerCreator>(); }
};

}

#endif
