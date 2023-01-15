// Copyright (c) 2019-2022 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_RULE_ALLOWED_CONVERSATIONS_H
#define QBIT_TXBUZZER_RULE_ALLOWED_CONVERSATIONS_H

#include "txbuzzerrule.h"

namespace qbit {

//
class TxBuzzerRuleAllowedConversations: public TxBuzzerRule {
public:
	TxBuzzerRuleAllowedConversations() { type_ = TX_BUZZER_RULE_ALLOWED_CONVERSATIONS; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
	}
	
	inline void deserialize(DataStream& s) {
		s >> timestamp_;
	}

	virtual std::string name() { return "buzzer_rule_allowed_conversations"; }
};

typedef std::shared_ptr<TxBuzzerRuleAllowedConversations> TxBuzzerRuleAllowedConversationsPtr;

class TxBuzzerRuleAllowedConversationsCreator: public TransactionCreator {
public:
	TxBuzzerRuleAllowedConversationsCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerRuleAllowedConversations>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerRuleAllowedConversationsCreator>(); }
};

}

#endif
