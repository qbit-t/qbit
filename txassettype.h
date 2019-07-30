// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXASSETTYPE_H
#define QBIT_TXASSETTYPE_H

#include "transaction.h"

namespace qbit {

typedef LimitedString<16>  asset_name_t;
typedef LimitedString<128> asset_description_t;

//
// Spend transaction
class TxAssetType : public Transaction {
public:
	// TODO: replace by original QBIT asset type before run
	static inline uint256 qbitAsset() { return uint256S("0f690892c0c5ebc826a3c51321178ef816171bb81570d9c3b7573e102c6de601"); }

public:
	enum Emission {
		UNLIMITED 	= 0x01,
		LIMITED 	= 0x02,
		PEGGED 		= 0x03
	};

	static constexpr uint64_t MAX_SUPPLY = std::numeric_limits<uint64_t>::max();	

public:
	TxAssetType() { type_ = Transaction::ASSET_TYPE; }

	std::string shortName_;	
	std::string longName_;
	Emission emission_;

	// can be:
	// 0x00000000	- if emission.pegged
	// MAX_SUPPLY	- if emission.unlimited
	// < MAX_SUPPLY	- if emission.limited
	amount_t supply_;

	// 1 0000 0000 = 1 QBIT
	// 1 0000 0000 = 1 BTC
	// 1 00 = 1 USD
	amount_t scale_; 

	inline void serialize(DataStream& s) {
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
	inline std::string& longName() { return longName_; }
	inline amount_t supply() { return supply_; }
	inline amount_t scale() { return scale_; }
	inline Emission emission() { return emission_; }

};

}

#endif