// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_TRANSACTION_ACTIONS_H
#define QBIT_BUZZER_TRANSACTION_ACTIONS_H

#include "../../transactionvalidator.h"

namespace qbit {

class TxBuzzerTimelockOutsVerify: public TransactionAction {
	class TxEntityStore: public IEntityStore {
	public:
		TxEntityStore() {}

		static IEntityStorePtr instance() {
			return std::make_shared<TxEntityStore>();
		}

		EntityPtr locateEntity(const uint256& entity) {
			return nullptr;
		}

		EntityPtr locateEntity(const std::string& name) {
			return nullptr;
		}

		bool pushEntity(const uint256& id, TransactionContextPtr ctx) {
			return true;
		}
	};
	typedef std::shared_ptr<TxEntityStore> TxEntityStorePtr;

	class TxBlockStore: public ITransactionStore {
	public:
		TxBlockStore() {}

		static ITransactionStorePtr instance() {
			return std::make_shared<TxBlockStore>();
		}

		inline bool pushUnlinkedOut(Transaction::UnlinkedOutPtr, TransactionContextPtr) {
			return true;
		}

		inline bool popUnlinkedOut(const uint256& utxo, TransactionContextPtr ctx) {
			return true;
		}
		
		inline void addLink(const uint256& /*from*/, const uint256& /*to*/) {}

		inline bool pushTransaction(TransactionContextPtr ctx) {
			return true;
		}

		inline TransactionPtr locateTransaction(const uint256& id) {
			return nullptr;
		}

		inline ISettingsPtr settings() {
			return nullptr;
		}

		bool synchronizing() { return false; } 

	};
	typedef std::shared_ptr<TxBlockStore> TxBlockStorePtr;

public:
	TxBuzzerTimelockOutsVerify() {}
	static TransactionActionPtr instance() { return std::make_shared<TxBuzzerTimelockOutsVerify>(); }

	TransactionAction::Result execute(TransactionContextPtr, ITransactionStorePtr, IWalletPtr, IEntityStorePtr);
};

} // qbit

#endif