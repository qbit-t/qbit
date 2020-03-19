// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXSHARD_H
#define QBIT_TXSHARD_H

#include "entity.h"

namespace qbit {

typedef LimitedString<64>  shard_name_t;
typedef LimitedString<256> shard_description_t;

//
class TxShard: public Entity {
public:
	TxShard() { type_ = Transaction::SHARD; }

	std::string shortName_;	
	std::string longName_;

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		shard_name_t lName(shortName_);
		lName.serialize(s);
		
		shard_description_t lDescription(longName_);
		lDescription.serialize(s);
	}
	
	inline void deserialize(DataStream& s) {
		shard_name_t lName(shortName_);
		lName.deserialize(s);

		shard_description_t lDescription(longName_);
		lDescription.deserialize(s);
	}

	inline std::string& shortName() { return shortName_; }
	inline void setShortName(const std::string& name) { shortName_ = name; }
	virtual std::string entityName() { return shortName_; }

	inline std::string& longName() { return longName_; }
	inline void setLongName(const std::string& name) { longName_ = name; }

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["entity"] = entityName();
		//
		props["description"] = longName_;
	}

	//
	Transaction::UnlinkedOutPtr addShardOut(const SKey& skey, const PKey& pkey) {
		//
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QPEN) 		<<
			OP(QPTXO) 		<< // use in entity-based pushUnlinkedOut's
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

	inline std::string name() { return "shard"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	
};

typedef std::shared_ptr<TxShard> TxShardPtr;

class TxShardCreator: public TransactionCreator {
public:
	TxShardCreator() {}
	TransactionPtr create() { return std::make_shared<TxShard>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxShardCreator>(); }
};

}

#endif
