// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXDAPP_H
#define QBIT_TXDAPP_H

#include "entity.h"

namespace qbit {

typedef LimitedString<64>  dapp_name_t;
typedef LimitedString<256> dapp_description_t;

//
class TxDApp: public Entity {
public:
	enum Sharding {
		STATIC 	= 0x01,
		DYNAMIC = 0x02
	};

public:
	TxDApp() { type_ = Transaction::DAPP; sharding_ = Sharding::STATIC; }

	std::string shortName_;	
	std::string longName_;
	Sharding sharding_;
	
	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		dapp_name_t lName(shortName_);
		lName.serialize(s);
		
		dapp_description_t lDescription(longName_);
		lDescription.serialize(s);

		s << (unsigned char) sharding_;
	}
	
	inline void deserialize(DataStream& s) {
		dapp_name_t lName(shortName_);
		lName.deserialize(s);

		dapp_description_t lDescription(longName_);
		lDescription.deserialize(s);

		unsigned char lSharding;
		s >> lSharding; sharding_ = (Sharding)lSharding;
	}

	inline std::string& shortName() { return shortName_; }
	inline void setShortName(const std::string& name) { shortName_ = name; }
	virtual std::string entityName() { return shortName_; }

	inline std::string& longName() { return longName_; }
	inline void setLongName(const std::string& name) { longName_ = name; }

	inline Sharding sharding() { return sharding_; }
	inline void setSharding(Sharding sharding) { sharding_ = sharding; }

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["entity"] = entityName();
		//
		props["description"] = longName_;
		props["sharding"] = (sharding_ == STATIC ? "static" : "dymanic");
	}

	//
	Transaction::UnlinkedOutPtr addDAppOut(const SKey& skey, const PKey& pkey) {
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
	Transaction::UnlinkedOutPtr addDAppPublicOut(const SKey& skey, const PKey& pkey, Transaction::Type instances) {
		//
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QPTXO)		<<
			OP(QMOV)		<< REG(QR1) << CU16(instances) <<
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

	inline std::string name() { return "dapp"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	
};

typedef std::shared_ptr<TxDApp> TxDAppPtr;

class TxDAppCreator: public TransactionCreator {
public:
	TxDAppCreator() {}
	TransactionPtr create() { return std::make_shared<TxDApp>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxDAppCreator>(); }
};

}

#endif
