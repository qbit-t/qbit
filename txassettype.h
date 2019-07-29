// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ITRANSACTION_TYPES_H
#define QBIT_ITRANSACTION_TYPES_H

#include "transaction.h"

namespace qbit {

typedef LimitedString<16> asset_name_t;
typedef LimitedString<128> asset_description_t;

//
// Spend transaction
class TxAssetType : public Transaction {
public:
	enum Emission {
		UNLIMITED 	= 0x01,
		LIMITED 	= 0x02,
		PEGGED 		= 0x03
	};

	static constexpr uint64_t MAX_SUPPLY = std::numeric_limits<uint64_t>::max();	

public:
	TxAssetType() { type_ = Transaction::ASSET_TYPE; }

	asset_name_t name_;	
	asset_description_t description_;
	Emission emission_;

	// can be:
	// 0x00000000	- if emission.pegged
	// MAX_SUPPLY	- if emission.unlimited
	// < MAX_SUPPLY	- if emission.limited
	amount_t supply_;

	// 1 0000 0000 = 1 BTC
	// 1 00 = 1 USD
	amount_t scale_; 

	inline void serialize(DataStream& s) {
		name_.Serialize(s);
		description_.Serialize(s);

		s << (unsigned char) emission_;
		s << supply_;
		s << scale_;
	}
	inline void deserialize(DataStream& s) {
		name_.Deserialize(s);
		description_.Deserialize(s);

		unsigned char lEmission;
		s >> lEmission; emission_ = (Emission)lEmission;
		s >> supply_;
		s >> scale_;
	}	

};

}

#endif