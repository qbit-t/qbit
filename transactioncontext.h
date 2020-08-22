// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TRANSACTION_CONTEXT_H
#define QBIT_TRANSACTION_CONTEXT_H

#include "transaction.h"
#include "version.h"

namespace qbit {

class TransactionContext;
typedef std::shared_ptr<TransactionContext> TransactionContextPtr;

class TransactionContext {
public:
	enum ProcessingContext {
		COMMON,
		MEMPOOL_COMMIT,
		STORE_COMMIT,
		STORE_PUSH
	};

public:
	typedef std::map<uint256, std::list<std::vector<unsigned char>>> _commitMap;

public:
	class Error {
	public:
		Error() {}
		Error(const std::string& error) {
			data_.insert(data_.end(), error.begin(), error.end());
		}

		std::string data() {
			std::string lResult;
			lResult.insert(lResult.end(), data_.begin(), data_.end());
			return lResult;
		}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(data_);
		}

	private:
		std::vector<unsigned char> data_;
	};

public:
	explicit TransactionContext() { fee_ = 0; size_ = 0; qbitTx_ = false; }
	TransactionContext(TransactionPtr tx) : tx_(tx) { fee_ = 0; size_ = 0; qbitTx_ = false; }
	TransactionContext(TransactionPtr tx, ProcessingContext context) : tx_(tx), context_(context) { 
		fee_ = 0; size_ = 0; qbitTx_ = false; 
	}

	inline TransactionPtr tx() { return tx_; }

	inline std::set<PKey>& addresses() { return addresses_; }
	inline std::set<PKey>& outAddresses() { return outAddresses_; }
	inline std::list<std::string>& errors() { return errors_; }
	inline void packErrors(std::vector<Error>& errors) {
		for (std::list<std::string>::iterator lError = errors_.begin(); lError != errors_.end(); lError++) {
			errors.push_back(Error(*lError));
		}
	}
	inline bool errorsContains(const std::string& err) {
		for (std::list<std::string>::iterator lError = errors_.begin(); lError != errors_.end(); lError++) {
			if ((*lError).find(err) != std::string::npos)
				return true;
		}

		return false;
	}
	inline std::list<Transaction::UnlinkedOutPtr>& usedUtxo() { return usedUtxo_; }
	inline std::list<Transaction::UnlinkedOutPtr>& newUtxo() { return newUtxo_; }
	inline std::list<TransactionContextPtr>& linkedTxs() { return linkedTxs_; }
	inline void addAddress(const PKey& key) { addresses_.insert(key); }
	inline void addOutAddress(const PKey& key) { outAddresses_.insert(key); }
	inline void addError(const std::string& error) { errors_.push_back(error); }
	inline void addLinkedTx(TransactionContextPtr tx) { linkedTxs_.push_back(tx); }
	inline std::set<uint256>& crossLinks() { return crossLinks_; }
	inline void pushCrossLink(const uint256& other) { crossLinks_.insert(other); }
	inline void clearCrossLinks() { crossLinks_.clear(); }
	
	// wallet processed unlinked outs (my)
	inline void addUsedUnlinkedOut(Transaction::UnlinkedOutPtr out) { usedUtxo_.push_back(out); }
	inline void addNewUnlinkedOut(Transaction::UnlinkedOutPtr out) { newUtxo_.push_back(out); }

	inline void addOut(const Transaction::Link& out) { outs_.push_back(out); }
	inline std::vector<Transaction::Link>& out() { return outs_; }
	inline std::vector<Transaction::In>& in() { return tx_->in(); }

	inline static TransactionContextPtr instance(TransactionPtr tx) { return std::make_shared<TransactionContext>(tx); }
	inline static TransactionContextPtr instance(TransactionPtr tx, ProcessingContext context) { 
		return std::make_shared<TransactionContext>(tx, context); 
	}

	inline _commitMap& commitIn() { return commitIn_; }
	inline _commitMap& commitOut() { return commitOut_; }

	inline amount_t fee() { return fee_; }
	inline void addFee(amount_t fee) { fee_ += fee; }

	inline amount_t amount() { return amount_; }
	inline void addAmount(amount_t amount) { amount_ += amount; }

	inline void setQbitTx() { qbitTx_ = true; }
	inline void resetQbitTx() { qbitTx_ = true; }
	inline bool qbitTx() { return qbitTx_; }

	inline ProcessingContext context() { return context_; }
	inline void setContext(ProcessingContext context) { context_ = context; }

	inline int incrementReprocessed() { return ++reprocessed_; }
	inline bool reprocessTimedout() { return reprocessed_ >= 30; /* approx 3 block */}

	// estimated rate (feeIn/out maybe excluded)
	inline qunit_t feeRate() {
		size_t lSize = size();
		qunit_t lRate = fee_ / lSize; 
		if (!lRate) return QUNIT;
		return lRate;
	}

	inline size_t size() {
		if (!size_) {
			SizeComputer lStream(PROTOCOL_VERSION);
			Transaction::Serializer::serialize<SizeComputer>(lStream, tx_);
			size_ = lStream.size();
		}

		return size_;
	}

	// locate tx
	TransactionContextPtr locateByType(unsigned short tx) {
		//
		for (std::list<TransactionContextPtr>::iterator lLinkedCtx = linkedTxs().begin(); lLinkedCtx != linkedTxs().end(); lLinkedCtx++) {
			if ((*lLinkedCtx)->tx()->type() == tx) return *lLinkedCtx;
		}

		return nullptr;
	}

private:
	TransactionPtr tx_;
	// in-addresses
	std::set<PKey> addresses_;
	// out-addresses
	std::set<PKey> outAddresses_;
	// errors
	std::list<std::string> errors_;
	// commit in group
	_commitMap commitIn_;
	// commit out group
	_commitMap commitOut_;
	// miner fee
	amount_t fee_;
	// output amount (if possible)
	amount_t amount_;
	// cached size
	size_t size_;
	// used my utxo's
	std::list<Transaction::UnlinkedOutPtr> usedUtxo_;
	// new my utxo's
	std::list<Transaction::UnlinkedOutPtr> newUtxo_;
	// outs
	std::vector<Transaction::Link> outs_;
	// qbit tx
	bool qbitTx_;
	// processing context
	ProcessingContext context_ = TransactionContext::COMMON;
	// chained/linked txs
	std::list<TransactionContextPtr> linkedTxs_;
	// cross-mempool links
	std::set<uint256 /*from*/> crossLinks_;
	// re-process count
	int reprocessed_ = 0;
};

} // qbit

#endif