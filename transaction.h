// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TRANSACTION_H
#define QBIT_TRANSACTION_H

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
#include "vm/qasm.h"
#include "streams.h"

namespace qbit {

using namespace qasm;

class Transaction {
public:
	enum Type {
		// value transfer
		COINBASE 				= 0x0001,	// QBIT coinbase transaction, validator selected (cookaroo maybe)
		SPEND					= 0x0002,	// Spend transaction: any asset

		// entity / action
		CONTRACT 				= 0x0010,	// Smart-contract publishing
		EVENT 					= 0x0011,	// Publish non-persistable event as transaction (ignition for smart-contracts processing)
		MESSAGE					= 0x0012,	// Create and send encrypted message (up to 256 bytes)

		// entity / action
		ASSET_TYPE 				= 0x0020, 	// Create crypto-asset type and embed crypto-asset specification as meta-data
		ASSET_EMISSION 			= 0x0021, 	// Crypto-asset (does not related to the QBIT - any token, described by asset_type metainfo)
		IMPORT_ASSET 			= 0x0022, 	// TODO: Create tiered transaction with external DLT input and lock external assets (should be external multi-sig address)
											// (external key signing system should be used for signing and verification - ecdsa, for example), atomic exchange
		// action
		EXPORT_ASSET 			= 0x0023, 	// TODO: Burn internal assets and release external assets, atomic exchange
		EXCHANGE_ASSETS			= 0x0024, 	// TODO: Atomic exchange transaction between QBIT an external DLT (BTC, for example)

		// org entity
		ORGANIZATION			= 0x0030, 	// Create abstract organization entity

		// roles
		VALIDATOR               = 0x0040, 	// Validator - block emitter, tx checker and QBIT reward receiver. Validators can be created only (first releases): genesis block issuer
		GATEKEEPER				= 0x0041, 	// Gatekeeper - controls exchange\atomic exchange between DLTs, control and provide partial keys and sigs for multi-sig processing

		// specialized org
		ORGANIZATION_EXCHANGE	= 0x0100	// Create exchange entity (pre-defined and integrated into core)
	};

	enum Status {
		CREATED 	= 0x01,
		ACCEPTED 	= 0x02,
		DECLINED 	= 0x03,
		PENDING		= 0x04
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

		inline void serialize(qbit::vector<unsigned char>& result) {
			result.insert(result.end(), asset_.begin(), asset_.end());
			result.insert(result.end(), hash_.begin(), hash_.end());
			result.insert(result.end(), (unsigned char*)(&index_), (unsigned char*)(&index_) + sizeof(index_));
		}

		uint256& asset() { return asset_; }
		uint256& hash() { return hash_; }
		uint32_t index() { return index_; }

		void setAsset(const uint256& asset) { asset_ = asset; }
		void setHash(const uint256& hash) { hash_ = hash; }
		void setIndex(uint32_t index) { index_ = index; }

		std::string toString() const;
	};

	class In {
	public:
		Link out_; // one of previous out
		qasm::ByteCode ownership_;

		In() {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(out_);
			READWRITE(ownership_);
		}

		Link& out() { return  out_; }
		qasm::ByteCode& ownership() { return ownership_; }

		void setOwnership(const qasm::ByteCode& ownership) { ownership_ = ownership; }

		std::string toString() const;
	};

	class Out {
	public:
		uint256 asset_;
		uint64_t amount_;
		qasm::ByteCode destination_;

		Out() { asset_.setNull(); }

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(asset_);
			READWRITE(amount_);
			READWRITE(destination_);
		}

		uint256& asset() { return asset_; }
		uint64_t amount() { return amount_; }
		qasm::ByteCode& destination() { return destination_; }

		void setDestination(const qasm::ByteCode& destination) { destination_ = destination; }
		void setAsset(const uint256& asset) { asset_ = asset; }
		void setAmount(uint64_t amount) { amount_ = amount; }

		std::string toString() const;
	};

	Transaction() { status_ = Status::CREATED; id_.setNull(); }

	virtual void serialize(DataStream& s) {}
	virtual void serialize(CHashWriter& s) {}
	virtual void deserialize(DataStream& s) {}

	uint256 hash();
	inline uint256 id() { return id_; }

	inline std::vector<In>& in() { return in_; }
	inline std::vector<Out>& out() { return out_; }
	inline std::vector<In>& feeIn() { return feeIn_; }
	inline std::vector<Out>& feeOut() { return feeOut_; }

	inline Type type() { return type_; }

	inline Status status() { return status_; }
	inline std::string error() { return error_; }

	inline void setStatus(Status status) { status_ = status; }	
	inline void setError(const std::string& error) { error_ = error; }

	virtual std::string toString();

protected:
	// tx type
	Type type_; // 2 bytes

	// inputs
	std::vector<In> in_;
	// outputs
	std::vector<Out> out_;

	// fee input
	std::vector<In> feeIn_;
	// fee output, with not complete script
	std::vector<Out> feeOut_;

	//
	// in-memory only

	// tx current status
	Status status_;
	// processing error
	std::string error_;
	// id
	uint256 id_;
};

class TxCoinBase;
class TxSpend;

typedef std::shared_ptr<Transaction> TransactionPtr;
typedef std::shared_ptr<TxCoinBase> TxCoinBasePtr;
typedef std::shared_ptr<TxSpend> TxSpendPtr;

//
// Coinbase transaction
class TxCoinBase : public Transaction {
public:
	TxCoinBase() {
		type_ = Transaction::COINBASE;
	}

	static TxCoinBasePtr as(TransactionPtr tx) { return std::static_pointer_cast<TxCoinBase>(tx); }

	void initialize(PKey& pkey, uint64_t amount = 0) {
		in_.resize(1);
		in_[0].out_.setNull();
		in_[0].ownership_ = ByteCode() <<
			OP(QMOV) << REG(QR0) << CU8(0x01) <<
			OP(QRET);

		out_.resize(1);
		out_[0].asset_.setNull();
		out_[0].amount_ = amount;
		out_[0].destination_ = ByteCode() <<
			OP(QMOV) << REG(QR0) << CVAR(pkey.get()) <<
			// TODO: OP(QPUSHD) <<
			OP(QCMPE) << REG(QR0) << REG(QS0) <<
			OP(QMOV) << REG(QR0) << REG(QC0) <<
			OP(QRET);
	}

	inline void serialize(DataStream& s) {}
	inline void deserialize(DataStream& s) {}
};

//
// Spend transaction
class TxSpend : public Transaction {
public:
	TxSpend() { type_ = Transaction::SPEND; }

	static TxSpendPtr as(TransactionPtr tx) { return std::static_pointer_cast<TxSpend>(tx); }

	bool initIn(Transaction::In& in, PKey& pkey, SKey& skey) {
		qbit::vector<unsigned char> lSource;
		in.out().serialize(lSource);
		
		uint256 lHash = Hash(lSource.begin(), lSource.end());
		uint512 lSig;

		if (!skey.sign(lHash, lSig)) return false;

		in.setOwnership(ByteCode() <<
			OP(QMOV) 		<< REG(QS0) << CVAR(pkey.get()) << 
			OP(QMOV) 		<< REG(QS1) << CU512(lSig) <<
			OP(QLHASH256) 	<< REG(QS2) <<
			OP(QCHECKSIG));
			// TODO: OP(QPULLD)

		return true;
	}

	bool initOut(Transaction::Out& out, PKey& pkey) {
		out.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QR0) << CVAR(pkey.get()) << 
			// TODO: OP(QPUSHD) <<
			OP(QCMPE) 		<< REG(QR0) << REG(QS0) <<
			OP(QMOV) 		<< REG(QR0) << REG(QC0) <<
			OP(QRET));

		return true;
	}

	inline void serialize(DataStream& s) {}
	inline void deserialize(DataStream& s) {}
};

//
// tx helper
class TransactionHelper {
public:
	TransactionHelper() {}

	template<typename type>
	static std::shared_ptr<type> from(TransactionPtr tx) { return std::static_pointer_cast<type>(tx); }
};

//
// tx creation functro
class TransactionCreator {
public:
	TransactionCreator() {}
	virtual TransactionPtr create() { return nullptr; }
};

//
// tx types
typedef std::map<Transaction::Type, TransactionCreator> TransactionTypes;
extern TransactionTypes gTxTypes; // TODO: init on startup

//
// Tx Factory
//
class TransactionFactory {
public:
	static TransactionPtr create(Transaction::Type txType) {
		switch(txType) {
			case Transaction::COINBASE: return std::make_shared<TxCoinBase>();
			case Transaction::SPEND: return std::make_shared<TxSpend>();

			default: {
				TransactionTypes::iterator lType = gTxTypes.find(txType);
				if (lType != gTxTypes.end()) {
					return lType->second.create();
				}
			}
			break;			
		}

		return TransactionPtr();
	}
};

class TransactionSerializer {
public:

	template<typename Stream, typename Tx>
	static inline void serialize(Stream& s, Tx* tx) {
		s << (unsigned short)tx->type();
		s << tx->in();
		s << tx->out();
		s << tx->feeIn();
		s << tx->feeOut();

		tx->serialize(s);
	}

	template<typename Stream>
	static inline void serialize(Stream& s, TransactionPtr tx) {
		s << (unsigned short)tx->type();
		s << tx->in();
		s << tx->out();
		s << tx->feeIn();
		s << tx->feeOut();

		tx->serialize(s);
	}
};

class TransactionDeserializer {
public:
	template<typename Stream>
	static inline TransactionPtr deserialize(Stream& s) {
		unsigned short lTxType = 0x0000;
		s >> lTxType;
		Transaction::Type lType = (Transaction::Type)lTxType;

		TransactionPtr lTx;
		switch(lType) {
			case Transaction::COINBASE: lTx = std::make_shared<TxCoinBase>(); break;
			case Transaction::SPEND: lTx = std::make_shared<TxSpend>(); break;
			default: {
				TransactionTypes::iterator lTypeIterator = gTxTypes.find(lType);
				if (lTypeIterator != gTxTypes.end()) {
					lTx = lTypeIterator->second.create();
				}
			}
			break;			
		}

		s >> lTx->in();
		s >> lTx->out();
		s >> lTx->feeIn();
		s >> lTx->feeOut();

		lTx->deserialize(s);

		return lTx;
	}
};


} // qbit

#endif
