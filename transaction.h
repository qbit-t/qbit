// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TRANSACTION_H
#define QBIT_TRANSACTION_H

#include "exception.h"
#include "uint256.h"
#include "crypto/sha256.h"
#include <list>
#include "hash.h"
#include "context.h"
#include "key.h"
#include "serialize.h"
#include "vm/qasm.h"
#include "streams.h"
#include "amount.h"
#include "random.h"

namespace qbit {

using namespace qasm;

// forward
class Transaction;
class TxCoinBase;
class TxSpend;
class TxSpendPrivate;

typedef std::shared_ptr<Transaction> TransactionPtr;
typedef std::shared_ptr<TxCoinBase> TxCoinBasePtr;
typedef std::shared_ptr<TxSpend> TxSpendPtr;
typedef std::shared_ptr<TxSpendPrivate> TxSpendPrivatePtr;

//
// tx helper
class TransactionHelper {
public:
	TransactionHelper() {}

	template<typename type>
	static std::shared_ptr<type> to(TransactionPtr tx) { return std::static_pointer_cast<type>(tx); }
};

//
// tx creation functor
class TransactionCreator {
public:
	TransactionCreator() {}
	virtual TransactionPtr create() { return nullptr; }
};

//
// tx types
typedef std::map<unsigned short, TransactionCreator> TransactionTypes;
extern TransactionTypes gTxTypes; // TODO: init on startup

// tx version type
typedef unsigned char version_t;

//
// generic transaction
class Transaction {
public:
	enum Type {
		// value transfer
		COINBASE 				= 0x0001,	// QBIT coinbase transaction, validator selected (cookaroo maybe)
		SPEND					= 0x0002,	// Spend transaction: any asset
		SPEND_PRIVATE			= 0x0003,	// Spend private transaction: any asset (default for all spend operations)

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
		VALIDATOR 				= 0x0040, 	// Validator - block emitter, tx checker and QBIT reward receiver. Validators can be created only (first releases): genesis block issuer
		GATEKEEPER				= 0x0041, 	// Gatekeeper - controls exchange\atomic exchange between DLTs, control and provide partial keys and sigs for multi-sig processing

		// specialized org
		ORGANIZATION_EXCHANGE	= 0x0100	// Create exchange entity (pre-defined and integrated into core)
	};

	// use to register tx type functor
	static void RegisterTransactionType(Type, TransactionCreator&);

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
		uint256 tx_; // previous transaction hash
		uint32_t index_; // output index

		Link() : index_(NULL_INDEX) { asset_.setNull(); tx_.setNull(); }
		Link(const uint256& asset) : asset_(asset), index_(NULL_INDEX) { tx_.setNull(); }
		Link(const uint256& asset, uint32_t index) : asset_(asset), index_(index) { tx_.setNull(); }
		Link(const uint256& asset, const uint256& tx, uint32_t index) : asset_(asset), tx_(tx), index_(index) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(asset_);
			READWRITE(tx_);
			READWRITE(index_);
		}

		void setNull() { asset_.setNull(); tx_.setNull(); index_ = NULL_INDEX; }
		bool isNull() const { return (asset_.isNull() && tx_.isNull() && index_ == NULL_INDEX); }

		inline void serialize(qbit::vector<unsigned char>& result) {
			result.insert(result.end(), asset_.begin(), asset_.end());
			result.insert(result.end(), tx_.begin(), tx_.end());
			result.insert(result.end(), (unsigned char*)(&index_), (unsigned char*)(&index_) + sizeof(index_));
		}

		uint256& asset() { return asset_; }
		uint256& tx() { return tx_; }
		uint32_t index() { return index_; }

		void setAsset(const uint256& asset) { asset_ = asset; }
		void setTx(const uint256& tx) { tx_ = tx; }
		void setIndex(uint32_t index) { index_ = index; }

		uint256 hash() {
			qbit::vector<unsigned char> lSource; serialize(lSource);
			return Hash(lSource.begin(), lSource.end());			
		}

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
		qasm::ByteCode destination_;

		Out() { asset_.setNull(); }

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(asset_);
			READWRITE(destination_);
		}

		uint256& asset() { return asset_; }
		qasm::ByteCode& destination() { return destination_; }

		void setDestination(const qasm::ByteCode& destination) { destination_ = destination; }
		void setAsset(const uint256& asset) { asset_ = asset; }

		std::string toString() const;
	};

	// WARNING:
	// is not a part of regular transaction
	// and it is _never_ persisted to the blockchain
	// used internally by wallet and transaction store for UTXO control
	class UnlinkedOut {
	public:
		UnlinkedOut() {}
		UnlinkedOut(const Link& o) : 
			out_(o) {}
		UnlinkedOut(const Link& o, amount_t a, const uint256& b, const std::vector<unsigned char>& c) : 
			out_(o), amount_(a), blind_(b), commit_(c) {}
		UnlinkedOut(const Link& o, amount_t a, const uint256& b, const std::vector<unsigned char>& c, const uint256& n) : 
			UnlinkedOut(o, a, b, c) { nonce_ = n; }

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(out_);
			READWRITE(amount_);
			READWRITE(blind_);
			READWRITE(nonce_);
			READWRITE(commit_);
		}

		Link& out() { return out_; }
		amount_t amount() { return amount_; }
		uint256& blind() { return blind_; }
		uint256& nonce() { return nonce_; }
		std::vector<unsigned char>& commit() { return commit_; }

		uint256 hash() { return out_.hash(); }

	private:
		Link out_; // one of previous out		
		amount_t amount_;
		uint256 blind_;
		uint256 nonce_;
		std::vector<unsigned char> commit_;
	};

	class Serializer {
	public:

		template<typename Stream, typename Tx>
		static inline void serialize(Stream& s, Tx* tx) {
			s << (unsigned short)tx->type();
			s << tx->version();
			s << tx->timeLock();
			s << tx->in();
			s << tx->out();

			tx->serialize(s);
		}

		template<typename Stream>
		static inline void serialize(Stream& s, TransactionPtr tx) {
			s << (unsigned short)tx->type();
			s << tx->version();
			s << tx->timeLock();
			s << tx->in();
			s << tx->out();

			tx->serialize(s);
		}
	};

	class Deserializer {
	public:
		template<typename Stream>
		static inline TransactionPtr deserialize(Stream& s) {
			unsigned short lTxType = 0x0000;
			s >> lTxType;
			Transaction::Type lType = (Transaction::Type)lTxType;

			TransactionPtr lTx = nullptr;
			switch(lType) {
				case Transaction::COINBASE: lTx = std::make_shared<TxCoinBase>(); break;
				case Transaction::SPEND: lTx = std::make_shared<TxSpend>(); break;
				case Transaction::SPEND_PRIVATE: lTx = std::make_shared<TxSpendPrivate>(); break;
				default: {
					TransactionTypes::iterator lTypeIterator = gTxTypes.find(lType);
					if (lTypeIterator != gTxTypes.end()) {
						lTx = lTypeIterator->second.create();
					}
				}
				break;			
			}

			if (lTx != nullptr) {
				s >> lTx->version_;
				s >> lTx->timeLock_;
				s >> lTx->in();
				s >> lTx->out();

				lTx->deserialize(s);
			}

			return lTx;
		}
	};	

	Transaction() { status_ = Status::CREATED; id_.setNull(); timeLock_ = 0; }

	virtual void serialize(DataStream& s) {}
	virtual void serialize(HashWriter& s) {}
	virtual void deserialize(DataStream& s) {}
	virtual bool finalize(const SKey&) { return false; }

	uint256 hash();
	inline uint256 id() { if (id_.isEmpty()) hash(); return id_; }

	inline std::vector<In>& in() { return in_; }
	inline std::vector<Out>& out() { return out_; }

	inline Out* out(uint32_t o) { if (o < out_.size()) return &out_[o]; return nullptr; }

	inline Type type() { return type_; }
	inline version_t version() { return version_; }
	virtual inline std::string name() { return "basic"; }

	inline Status status() { return status_; }
	inline void setStatus(Status status) { status_ = status; }	

	virtual std::string toString();

	inline uint32_t timeLock() { return timeLock_; }
	inline void setTimelock(uint32_t timelock) { timeLock_ = timelock; }

	virtual In& addFeeIn(const SKey&, const UnlinkedOut&) { throw qbit::exception("NOT_IMPLEMENTED", "Not implemented."); }
	virtual Transaction::UnlinkedOut addFeeOut(const SKey&, const PKey&, const uint256&, amount_t) { throw qbit::exception("NOT_IMPLEMENTED", "Not implemented."); }

protected:
	// tx type
	Type type_; // 2 bytes
	// tx version
	version_t version_; // 1 byte

	// inputs
	std::vector<In> in_;
	// outputs
	std::vector<Out> out_;

	// time-lock
	uint32_t timeLock_;

	//
	// in-memory only

	// tx current status
	Status status_;
	// id
	uint256 id_;
};

//
// DataStream specialization
template void Transaction::Serializer::serialize<DataStream>(DataStream&, TransactionPtr);
template TransactionPtr Transaction::Deserializer::deserialize<DataStream>(DataStream&);

//
// Coinbase transaction
class TxCoinBase : public Transaction {
public:
	TxCoinBase() {
		type_ = Transaction::COINBASE;
	}

	static TxCoinBasePtr as(TransactionPtr tx) { return std::static_pointer_cast<TxCoinBase>(tx); }

	In& addIn() {
		in_.resize(1);
		in_[0].out_.setNull();
		in_[0].ownership_ = ByteCode() <<
			OP(QMOV) << REG(QR0) << CU8(0x01);

		return in_[0];
	}

	Transaction::UnlinkedOut addOut(const SKey& skey, const PKey& pkey, const uint256& asset, amount_t amount) {
		qbit::vector<unsigned char> lCommitment;

		uint256 lBlind = const_cast<SKey&>(skey).shared(pkey);
		Math::mul(lBlind, lBlind, Random::generate());

		if (!const_cast<SKey&>(skey).context()->createCommitment(lCommitment, lBlind, amount)) {
			throw qbit::exception("INVALID_COMMITMENT", "Commitment creation failed.");
		}

		out_.resize(1);
		out_[0].asset_ = asset;
		out_[0].destination_ = ByteCode() <<
			OP(QMOV) 	<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) <<
			OP(QMOV) 	<< REG(QA0) << CU64(amount) <<
			OP(QMOV) 	<< REG(QA1) << CVAR(lCommitment) <<
			OP(QMOV) 	<< REG(QA2) << CU256(lBlind) <<
			OP(QCHECKA) <<
			OP(QATXOA) 	<<
			OP(QEQADDR) <<
			OP(QRET);

		return Transaction::UnlinkedOut(
			Transaction::Link(asset, 0), // link
			amount, // amount
			lBlind, // blinding key
			lCommitment // commit
		);
	}

	inline std::string name() { return "coinbase"; }

	inline void serialize(DataStream& s) {}
	inline void deserialize(DataStream& s) {}
};

//
// Spend transaction
class TxSpend : public Transaction {
public:
	TxSpend() { type_ = Transaction::SPEND; }

	static TxSpendPtr as(TransactionPtr tx) { return std::static_pointer_cast<TxSpend>(tx); }

	//
	// skey - our secret key
	// utxo - unlinked tx out
	virtual In& addIn(const SKey& skey, const UnlinkedOut& utxo) {
		Transaction::In lIn;
		lIn.out().setNull();
		lIn.out().setAsset(const_cast<UnlinkedOut&>(utxo).out().asset());
		lIn.out().setTx(const_cast<UnlinkedOut&>(utxo).out().tx());
		lIn.out().setIndex(const_cast<UnlinkedOut&>(utxo).out().index());

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
			OP(QDTXO));

		// fill up for finalization
		assetIn_[const_cast<UnlinkedOut&>(utxo).out().asset()].push_back(utxo);

		in_.push_back(lIn);
		return in_[in_.size()-1];
	}

	//
	// skey - our secret key
	// pkey - destination address
	// asset - asset type hash
	virtual Transaction::UnlinkedOut addOut(const SKey& skey, const PKey& pkey, const uint256& asset, amount_t amount) {
		qbit::vector<unsigned char> lCommitment;

		uint256 lBlind = const_cast<SKey&>(skey).shared(pkey);
		Math::mul(lBlind, lBlind, Random::generate());

		if (!const_cast<SKey&>(skey).context()->createCommitment(lCommitment, lBlind, amount)) {
			throw qbit::exception("INVALID_COMMITMENT", "Commitment creation failed.");
		}

		Transaction::Out lOut;
		lOut.setAsset(asset);
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
			OP(QMOV) 		<< REG(QA0) << CU64(amount) <<
			OP(QMOV) 		<< REG(QA1) << CVAR(lCommitment) <<			
			OP(QMOV) 		<< REG(QA2) << CU256(lBlind) <<			
			OP(QCHECKA) 	<<
			OP(QATXOA) 		<<
			OP(QEQADDR) 	<<
			OP(QRET));

		Transaction::UnlinkedOut lUTXO(
			Transaction::Link(asset, out_.size()), // link
			amount, // amount
			lBlind, // blinding key
			lCommitment // commit
		);

		// fill up for finalization
		assetOut_[asset].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}

	//
	// skey - our secret key
	// pkey - destination address
	// asset - asset type hash
	virtual Transaction::UnlinkedOut addFeeOut(const SKey& skey, const uint256& asset, amount_t amount) {
		qbit::vector<unsigned char> lCommitment;

		uint256 lBlind = Random::generate();
		if (!const_cast<SKey&>(skey).context()->createCommitment(lCommitment, lBlind, amount)) {
			throw qbit::exception("INVALID_COMMITMENT", "Commitment creation failed.");
		}

		Transaction::Out lOut;
		lOut.setAsset(asset);
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CU16(0xFFFA) << // miner
			OP(QMOV) 		<< REG(QA0) << CU64(amount) << // always open amount
			OP(QMOV) 		<< REG(QA1) << CVAR(lCommitment) <<			
			OP(QMOV) 		<< REG(QA2) << CU256(lBlind) <<			
			OP(QCHECKA) 	<<
			OP(QRET));

		Transaction::UnlinkedOut lUTXO(
			Transaction::Link(asset, out_.size()), // link
			amount, // amount
			lBlind, // blinding key
			lCommitment // commit
		);

		// fill up for finalization
		assetOut_[asset].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}

	virtual inline std::string name() { return "spend"; }

	virtual inline void serialize(DataStream& s) {}
	virtual inline void deserialize(DataStream& s) {}

	//
	// balance amounts and make extra out for balance check
	// acts as finalization for create
	virtual bool finalize(const SKey&);

protected:
	typedef std::map<uint256, std::list<Transaction::UnlinkedOut>> _assetMap;
	_assetMap assetIn_; // in asset group
	_assetMap assetOut_; // out asset group
};

//
// Spend private transaction
class TxSpendPrivate : public TxSpend {
public:
	TxSpendPrivate() { type_ = Transaction::SPEND_PRIVATE; }

	static TxSpendPrivatePtr as(TransactionPtr tx) { return std::static_pointer_cast<TxSpendPrivate>(tx); }

	//
	// skey - our secret key
	// pkey - destination address
	// asset - asset type hash
	Transaction::UnlinkedOut addOut(const SKey& skey, const PKey& pkey, const uint256& asset, amount_t amount) {
		qbit::vector<unsigned char> lCommitment;
		qbit::vector<unsigned char> lRangeProof;

		uint256 lNonce = const_cast<SKey&>(skey).shared(pkey);
		uint256 lBlind(lNonce); 
		Math::mul(lBlind, lBlind, Random::generate());

		if (!const_cast<SKey&>(skey).context()->createCommitment(lCommitment, lBlind, amount)) {
			throw qbit::exception("INVALID_COMMITMENT", "Commitment creation failed.");
		}

		if (!const_cast<SKey&>(skey).context()->createRangeProof(lRangeProof, lCommitment, lBlind, lNonce, amount)) {
			throw qbit::exception("INVALID_RANGEPROOF", "Range proof creation failed.");
		}

		Transaction::Out lOut;
		lOut.setAsset(asset);
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
			OP(QMOV) 		<< REG(QA0) << CU64(0x00) <<
			OP(QMOV) 		<< REG(QA1) << CVAR(lCommitment) <<			
			OP(QMOV) 		<< REG(QA2) << CVAR(lRangeProof) <<	
			OP(QCHECKP) 	<<
			OP(QATXOP)	 	<<
			OP(QEQADDR) 	<<
			OP(QRET));

		Transaction::UnlinkedOut lUTXO(
			Transaction::Link(asset, out_.size()), // link
			amount, // amount
			lBlind, // blinding key
			lCommitment, // commit
			lNonce
		);

		// fill up for finalization
		assetOut_[asset].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}

	inline std::string name() { return "spend_private"; }

	inline void serialize(DataStream& s) {}
	inline void deserialize(DataStream& s) {}

	//
	// balance amounts and make extra out for balance check
	// acts as finalization for create
	bool finalize(const SKey&);	
};

//
// Tx Factory
//
class TransactionFactory {
public:
	static TransactionPtr create(Transaction::Type txType) {
		switch(txType) {
			case Transaction::COINBASE: return std::make_shared<TxCoinBase>();
			case Transaction::SPEND: return std::make_shared<TxSpend>();
			case Transaction::SPEND_PRIVATE: return std::make_shared<TxSpendPrivate>();

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

} // qbit

#endif
