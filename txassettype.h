// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXASSETTYPE_H
#define QBIT_TXASSETTYPE_H

#include "entity.h"
#include "tinyformat.h"

namespace qbit {

typedef LimitedString<64>  asset_name_t;
typedef LimitedString<256> asset_description_t;

//
// Asset type transaction
// ----------------------
// in[0]  - qbit fee
// out[0] - unlinked out to use as "in" for asset emission
// out[1] - qbit miner fee 
// out[2] - qbit fee change
class TxAssetType: public Entity {
public:
	static inline uint256 qbitAsset() { return uint256S("0f690892c0c5ebc826a3c51321178ef816171bb81570d9c3b7573e102c6de601"); }
	static inline uint256 nullAsset() { return uint256S("f000000000000000000000000000000000000000000000000000000000000000"); }

public:
	enum Emission {		
		LIMITED 	= 0x01,
		UNLIMITED 	= 0x02, // qbit, mining
		PEGGED 		= 0x03
	};

	static constexpr uint64_t MAX_SUPPLY = std::numeric_limits<uint64_t>::max();	

public:
	TxAssetType() { type_ = Transaction::ASSET_TYPE; supply_ = 0x00; emission_ = Emission::LIMITED; }

	std::string shortName_;	
	std::string longName_;
	Emission emission_;

	amount_t supply_;

	// 1 0000 0000 = 1 QBIT
	// 1 0000 0000 = 1 BTC
	// 1 00 = 1 USD
	amount_t scale_; 

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		asset_name_t lName(shortName_);
		lName.Serialize(s);
		
		asset_description_t lDescription(longName_);
		lDescription.Serialize(s);

		s << (unsigned char) emission_;
		s << supply_;
		s << scale_;
	}
	
	inline void deserialize(DataStream& s) {
		asset_name_t lName(shortName_);
		lName.Deserialize(s);

		asset_description_t lDescription(longName_);
		lDescription.Deserialize(s);

		unsigned char lEmission;
		s >> lEmission; emission_ = (Emission)lEmission;
		s >> supply_;
		s >> scale_;
	}

	inline std::string& shortName() { return shortName_; }
	inline void setShortName(const std::string& name) { shortName_ = name; }
	virtual std::string entityName() { return shortName_; }

	inline std::string& longName() { return longName_; }
	inline void setLongName(const std::string& name) { longName_ = name; }

	inline amount_t supply() { return supply_; }
	inline void setSupply(amount_t supply) { supply_ = supply; }

	inline amount_t scale() { return scale_; }
	inline void setScale(amount_t scale) { scale_ = scale; }

	inline Emission emission() { return emission_; }
	inline void setEmission(Emission emission) { emission_ = emission; }

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["entity"] = entityName();
		//
		props["description"] = longName_;
		props["supply"] = strprintf("%d", supply_);
		props["scale"] = strprintf("%d", scale_);
		props["emission"] = (emission_ == LIMITED ? "limited" : (emission_ == UNLIMITED ? "unlimited" : "pegged"));
	}

	static std::string scaleFormat(amount_t scale) {
		if (scale == QBIT) return "%.8f";
		else if (scale == 10000000) return "%.7f";
		else if (scale == 1000000) return "%.6f";
		else if (scale == 100000) return "%.5f";
		else if (scale == 10000) return "%.4f";
		else if (scale == 1000) return "%.3f";
		else if (scale == 100) return "%.2f";
		else if (scale == 10) return "%.1f";
		return "%.0f";
	}

	// Ability to make "mining-like" assets:
	// - 1000 total supply
	// - 10 outputs by 100 tokens
	Transaction::UnlinkedOutPtr addLimitedOut(const SKey& skey, const PKey& pkey, amount_t emission) {
		qbit::vector<unsigned char> lCommitment;

		if (emission_ != Emission::LIMITED) throw qbit::exception("INVALID_EMISSION_TYPE", "Invalid emission type.");

		uint256 lBlind = const_cast<SKey&>(skey).shared(pkey);
		Math::mul(lBlind, lBlind, Random::generate());

		if (!const_cast<SKey&>(skey).context()->createCommitment(lCommitment, lBlind, emission)) {
			throw qbit::exception("INVALID_COMMITMENT", "Commitment creation failed.");
		}

		Transaction::Out lOut;
		lOut.setAsset(nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
			OP(QMOV) 		<< REG(QA0) << CU64(emission) <<
			OP(QMOV) 		<< REG(QA1) << CVAR(lCommitment) <<			
			OP(QMOV) 		<< REG(QA2) << CU256(lBlind) <<
			OP(QCHECKA) 	<<
			OP(QATXOA) 		<<
			OP(QEQADDR) 	<<
			OP(QPAT) 		<<
			OP(QMOV)		<< REG(QR1) << CU16(Transaction::ASSET_EMISSION) <<
			OP(QCMPE)		<< REG(QTH1) << REG(QR1) <<
			OP(QMOV) 		<< REG(QR0)  << REG(QC0) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(MainChain::id(), nullAsset(), out_.size()), // link
			pkey,
			emission, // amount
			lBlind, // blinding key
			lCommitment // commit
		);

		assetOut_[nullAsset()].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}

	// For pegged asset emissions
	// Total supply is unknown
	// This unlinked out can be used many times (there is no value transfer)
	Transaction::UnlinkedOutPtr addPeggedOut(const SKey& skey, const PKey& pkey) {
		qbit::vector<unsigned char> lCommitment;

		if (emission_ != Emission::PEGGED) throw qbit::exception("INVALID_EMISSION_TYPE", "Invalid emission type.");

		Transaction::Out lOut;
		lOut.setAsset(nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
			OP(QATXO)		<<
			OP(QEQADDR) 	<<
			OP(QPAT) 		<<
			OP(QMOV)		<< REG(QR1) << CU16(Transaction::ASSET_EMISSION) <<
			OP(QCMPE)		<< REG(QTH1) << REG(QR1) <<
			OP(QMOV) 		<< REG(QR0)  << REG(QC0) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(MainChain::id(), nullAsset(), out_.size()), // link
			pkey
		);

		assetOut_[nullAsset()].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}	

	inline std::string name() { return "asset_type"; }

	virtual bool finalize(const SKey& skey) {
		bool lResult = TxSpend::finalize(skey);

		// only qbit fee inputs

		amount_t lSupply = 0;
		_assetMap::iterator lGroup = assetOut_.find(nullAsset());
		if (lGroup != assetOut_.end()) {
			std::list<Transaction::UnlinkedOutPtr> lOutList = lGroup->second;
			for (std::list<Transaction::UnlinkedOutPtr>::iterator lOUtxo = lOutList.begin(); lOUtxo != lOutList.end(); lOUtxo++) {
				lSupply += (*lOUtxo)->amount(); 
			}
		}

		if (emission_ == Emission::LIMITED && lSupply != supply_) return false;
		if (emission_ != Emission::LIMITED && supply_ != 0x00) return false;

		return lResult;
	}

	bool isValue(UnlinkedOutPtr utxo) {
		return !(utxo->out().asset() == id()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return utxo->out().asset() == id(); 
	}
};

typedef std::shared_ptr<TxAssetType> TxAssetTypePtr;

class TxAssetTypeCreator: public TransactionCreator {
public:
	TxAssetTypeCreator() {}
	TransactionPtr create() { return std::make_shared<TxAssetType>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxAssetTypeCreator>(); }
};

}

#endif
