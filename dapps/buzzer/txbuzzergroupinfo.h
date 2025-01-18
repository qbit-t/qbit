// Copyright (c) 2024 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_GROUP_INFO_H
#define QBIT_TXBUZZER_GROUP_INFO_H

#include "txbuzzerinfo.h"

namespace qbit {

//
class TxBuzzerGroupInfo: public TxBuzzerInfo {
public:
	TxBuzzerGroupInfo() { type_ = TX_BUZZER_GROUP_INFO; }

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

	inline std::string name() { return "buzzer_group_info"; }
};

typedef std::shared_ptr<TxBuzzerGroupInfo> TxBuzzerGroupInfoPtr;

class TxBuzzerGroupInfoCreator: public TransactionCreator {
public:
	TxBuzzerGroupInfoCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerGroupInfo>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerGroupInfoCreator>(); }
};

}

#endif
