// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_INFO_H
#define QBIT_TXBUZZER_INFO_H

#include "txbuzzer.h"

namespace qbit {

#define TX_BUZZER_INFO_MY_IN 0

//
class TxBuzzerInfo: public Entity {
public:
	TxBuzzerInfo() { type_ = TX_BUZZER_INFO; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		buzzer_name_t lName(name_);
		lName.serialize(s);
		s << alias_;
		s << description_;
		s << avatar_;
		s << header_;
		s << signature_;
	}
	
	inline void deserialize(DataStream& s) {
		buzzer_name_t lName(name_);
		lName.deserialize(s);
		s >> alias_;
		s >> description_;
		s >> avatar_;
		s >> header_;
		s >> signature_;

		if (alias_.size() > TX_BUZZER_ALIAS_SIZE) alias_.resize(TX_BUZZER_ALIAS_SIZE);
		if (description_.size() > TX_BUZZER_DESCRIPTION_SIZE) description_.resize(TX_BUZZER_DESCRIPTION_SIZE);
	}

	inline std::string& myName() { return name_; }
	inline void setMyName(const std::string& name) { name_ = name; }
	virtual std::string entityName() { return Entity::emptyName(); }

	inline const BuzzerMediaPointer& avatar() const { return avatar_; }
	inline void setAvatar(const BuzzerMediaPointer& avatar) { avatar_ = avatar; }

	inline const BuzzerMediaPointer& header() const { return header_; }
	inline void setHeader(const BuzzerMediaPointer& header) { header_ = header; }

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

	inline void makeSignature(const SKey& skey) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << alias_;
		lSource << description_;
		lSource << avatar_;
		lSource << header_;
		
		uint256 lHash = Hash(lSource.begin(), lSource.end());
		if (!const_cast<SKey&>(skey).sign(lHash, signature_)) {
			throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
		}		
	}

	inline bool verifySignature() {
		//
		PKey lPKey;
		if (extractAddress(lPKey)) {
			DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
			lSource << alias_;
			lSource << description_;
			lSource << avatar_;
			lSource << header_;

			uint256 lHash = Hash(lSource.begin(), lSource.end());
			return lPKey.verify(lHash, signature_);
		}

		return false;
	}	

	inline void properties(std::map<std::string, std::string>& props) {
		//
		std::string lAlias; lAlias.insert(lAlias.end(), alias_.begin(), alias_.end());
		props["alias"] = lAlias;

		std::string lDescription; lDescription.insert(lDescription.end(), description_.begin(), description_.end());
		props["description"] = lDescription;

		props["avatar"] = strprintf("%s/%s", avatar_.tx().toHex(), avatar_.chain().toHex());
		props["header"] = strprintf("%s/%s", header_.tx().toHex(), header_.chain().toHex());

		props["name"] = name_;
		props["signature"] = strprintf("%s", signature_.toHex());		
	}

	//
	// in[0] - mybuzzer.out[TX_BUZZER_MY_OUT] - my buzzer with signature
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

	inline std::string name() { return "buzzer_info"; }

	virtual bool isFeeFee() { return true; }

	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior	

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	

	inline bool extractAddress(PKey& pkey) {
		//
		if (pkey_.valid()) { pkey = pkey_; return true; }
		
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
	std::string name_;
	std::vector<unsigned char> alias_;
	std::vector<unsigned char> description_;
	BuzzerMediaPointer avatar_;
	BuzzerMediaPointer header_;
	uint512 signature_;

	//
	PKey pkey_;
};

typedef std::shared_ptr<TxBuzzerInfo> TxBuzzerInfoPtr;

class TxBuzzerInfoCreator: public TransactionCreator {
public:
	TxBuzzerInfoCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerInfo>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerInfoCreator>(); }
};

}

#endif
