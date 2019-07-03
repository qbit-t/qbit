// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QUARK_TRANSACTION_H
#define QUARK_TRANSACTION_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../allocator.h"

#include "uint256.h"
#include "crypto/sha256.h"
#include <list>
#include "hash.h"
#include "context.h"
#include "key.h"
#include "serialize.h"
#include "vm/asm.h"
#include "streams.h"

namespace quark {

using namespace quantum;

class Transaction {
public:
	enum Type {
		// value transfer
		COINBASE 		= 0x1,  // QUARK coinbase transaction, validator selected (cookaroo maybe)
		SPEND			= 0x2,	// Spend transaction: any asset

		// entity / action
		CONTRACT 		= 0x10, // Smart-contract publishing
		EVENT 			= 0x11, // Publish non-persistable event as transaction (ignition for smart-contracts processing)
		MESSAGE			= 0x12, // Create and send encrypted message (up to 256 bytes)

		// entity / action
		ASSET_TYPE 		= 0x20, // Create crypto-asset type and embed crypto-asset specification as meta-data
		ASSET_EMISSION 		= 0x21, // Crypto-asset (does not related to the QUARK - any token, described by asset_type metainfo)
		IMPORT_ASSET 		= 0x22, // TODO: Create tiered transaction with external DLT input and lock external assets (should be external multi-sig address)
						// (external key signing system should be used for signing and verification - ecdsa, for example), atomic exchange
		// action
		EXPORT_ASSET 		= 0x23, // TODO: Burn internal assets and release external assets, atomic exchange
		EXCHANGE_ASSETS		= 0x24, // TODO: Atomic exchange transaction between QUARK an external DLT (BTC, for example)

		// org entity
		ORGANIZATION		= 0x30,	// Create abstract organization entity
		ORGANIZATION_EXCHANGE	= 0x31,	// Create exchange entity (pre-defined and integrated into core)

		// roles
		VALIDATOR               = 0x40, // Validator - block emitter, tx checker and QUARK reward receiver. Validators can be created only (first releases): genesis block issuer
		GATEKEEPER		= 0x41	// Gatekeeper - controls exchange\atomic exchange between DLTs, control and provide partial keys and sigs for multi-sig processing
	};

	class Link {
	public:
		static constexpr uint32_t NULL_INDEX = std::numeric_limits<uint32_t>::max();

		uint256 asset_; // asset type transaction
		uint256 hash_; // previous transaction hash
		uint32_t index_; // output index

		Link() : index_(NULL_INDEX) { asset_.setNull(); hash_.setNull(); }
		Link(const uint256& asset, const uint256& hash, uint32_t index) : asset_(asset), hash_(hash), index_(index) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(asset_);
			READWRITE(hash_);
			READWRITE(index_);
		}

		void setNull() { asset_.setNull(); hash_.setNull(); index_ = NULL_INDEX; }
		bool isNull() const { return (asset_.isNull() && hash_.isNull() && index_ == NULL_INDEX); }
	};

	class In {
	public:
		Link out_; // one of previous out
		quantum::Script ownership_;

		In() {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(out_);
			READWRITE(ownership_);
		}
	};

	class Out {
	public:
		uint256 asset_;
		uint64_t amount_;
		quantum::Script destination_;

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(asset_);
			READWRITE(amount_);
			READWRITE(destination_);
		}

		Out() {}
	};

	Transaction() {}

	virtual void serialize(DataStream& s) {}
	virtual void deserialize(DataStream& s) {}
	virtual void initialize(PKey&) {}

public:
	Type type_; // 1 byte

	// inputs
	std::vector<In> in_;
	// outputs
	std::vector<Out> out_;

	// fee input
	std::vector<In> feeIn_;
	// fee output, with not complete script
	std::vector<Out> feeOut_;
};


//
// Coinbase transaction
class TxCoinBase : public Transaction {
public:
	TxCoinBase() {
		type_ = Transaction::COINBASE;
	}

	void initialize(PKey& pkey) {
		in_.resize(1);
		in_[0].out_.setNull();
		in_[0].ownership_ = Script() << OP(QPUSH) << CVAR(pkey.get());

		out_.resize(1);
		out_[0].asset_.setNull();
		out_[0].amount_ = 0;
		out_[0].destination_ = Script() << OP(QDUP) << OP(QPUSH) << CVAR(pkey.id().get()) << OP(QVERIFYEQ) << OP(QCHECKSIG);
	}

	inline void serialize(DataStream& s) {}
	inline void deserialize(DataStream& s) {}
};

//
// Spend transaction
class TxSpend : public Transaction {
public:
	TxSpend() { type_ = Transaction::SPEND; }

	inline void serialize(DataStream& s) {}
	inline void deserialize(DataStream& s) {}
};


//
// Tx Factory
//
typedef std::shared_ptr<Transaction> TransactionPtr;

class TransactionFactory {
public:
	static TransactionPtr create(const std::string& type) {
		if(type == "COINBASE") return std::make_shared<TxCoinBase>();
		else if(type == "SPEND") return std::make_shared<TxSpend>();

		return TransactionPtr();
	}

	static TransactionPtr create(Transaction::Type type) {
		switch(type) {
			case Transaction::COINBASE: return std::make_shared<TxCoinBase>();
			case Transaction::SPEND: return std::make_shared<TxSpend>();
		}

		return TransactionPtr();
	}
};

class TransactionSerializer {
public:

	template<typename Stream, typename Tx>
	static inline void serialize(Stream& s, Tx& tx) {
		s << (unsigned char)tx.type_;
		s << tx.in_;
		s << tx.out_;
		s << tx.feeIn_;
		s << tx.feeOut_;

		tx.serialize(s);
	}

	template<typename Stream>
	static inline void serialize(Stream& s, TransactionPtr tx) {
		s << (unsigned char)tx->type_;
		s << tx->in_;
		s << tx->out_;
		s << tx->feeIn_;
		s << tx->feeOut_;

		tx->serialize(s);
	}
};

template<typename Stream>
class TransactionDeserializer {
public:
	static inline TransactionPtr deserialize(Stream& s) {
		Transaction::Type lType;
		s >> lType;

		TransactionPtr lTx;
		switch(lType) {
			case Transaction::COINBASE: lTx = std::make_shared<TxCoinBase>(); break;
			case Transaction::SPEND: lTx = std::make_shared<TxSpend>(); break;
		}

		s >> lTx->in_;
		s >> lTx->out_;
		s >> lTx->feeIn_;
		s >> lTx->feeOut_;

		lTx->deserialize(s);

		return lTx;
	}
};


} // quark

#endif
