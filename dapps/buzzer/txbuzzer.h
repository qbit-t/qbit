// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_H
#define QBIT_TXBUZZER_H

#include "../../entity.h"

namespace qbit {

typedef LimitedString<64> buzzer_name_t;

#define TX_BUZZER Transaction::CUSTOM_00
#define TX_BUZZER_ALIAS_SIZE 64 
#define TX_BUZZER_DESCRIPTION_SIZE 256 

//
class TxBuzzer: public Entity {
public:
	TxBuzzer() { type_ = TX_BUZZER; }

	inline void serialize(DataStream& s) {
		buzzer_name_t lName(name_);
		lName.Serialize(s);
		s << alias_;
		s << description_;
	}
	
	inline void deserialize(DataStream& s) {
		buzzer_name_t lName(name_);
		lName.Deserialize(s);
		s >> alias_;
		s >> description_;

		if (alias_.size() > TX_BUZZER_ALIAS_SIZE) alias_.resize(TX_BUZZER_ALIAS_SIZE);
		if (description_.size() > TX_BUZZER_DESCRIPTION_SIZE) description_.resize(TX_BUZZER_DESCRIPTION_SIZE);
	}

	inline std::string& myName() { return name_; }
	inline void setMyName(const std::string& name) { name_ = name; }
	virtual std::string entityName() { return name_; }

	inline std::vector<unsigned char>& description() { return description_; }
	inline void setDescription(const std::vector<unsigned char>& description) {
		if (description.size() <= TX_BUZZER_DESCRIPTION_SIZE) 
			description_ = description;
		else throw qbit::exception("E_SIZE", "Description size is incorrect.");
	}
	inline void setDescription(const std::string& description) {
		if (description.size() <= TX_BUZZER_DESCRIPTION_SIZE) 
			description_.insert(description_.end(), description.begin(), description.end()); 
		else throw qbit::exception("E_SIZE", "Description size is incorrect.");
	}

	inline std::vector<unsigned char>& alias() { return alias_; }
	inline void setAlias(const std::vector<unsigned char>& alias) {
		if (alias.size() <= TX_BUZZER_ALIAS_SIZE) 
			alias_ = alias;
		else throw qbit::exception("E_SIZE", "Alias size is incorrect.");
	}
	inline void setAlias(const std::string& alias) {
		if (alias.size() <= TX_BUZZER_ALIAS_SIZE) 
			alias_.insert(alias_.end(), alias.begin(), alias.end()); 
		else throw qbit::exception("E_SIZE", "Alias size is incorrect.");
	}

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["entity"] = entityName();
		//
		std::string lAlias; lAlias.insert(lAlias.end(), alias_.begin(), alias_.end());
		props["alias"] = lAlias;

		std::string lDescription; lDescription.insert(lDescription.end(), description_.begin(), description_.end());
		props["description"] = lDescription;
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

		out_.push_back(lOut);
		return lUTXO;
	}

	//
	Transaction::UnlinkedOutPtr addBuzzerSubscriptionOut(const SKey& skey, const PKey& pkey) {
		//
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QPTXO)		<< // use in entity-based pushUnlinkedOut's
			OP(QMOV) 		<< REG(QR0) << CU8(0x01) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(MainChain::id(), TxAssetType::nullAsset(), out_.size()), // link
			pkey
		);

		out_.push_back(lOut);
		return lUTXO;
	}

	virtual In& addDAppIn(const SKey& skey, UnlinkedOutPtr utxo) {
		Transaction::In lIn;
		lIn.out().setNull();
		lIn.out().setChain(MainChain::id());
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
		lIn.out().setChain(MainChain::id());
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

private:
	std::string name_;
	std::vector<unsigned char> alias_;
	std::vector<unsigned char> description_;
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
