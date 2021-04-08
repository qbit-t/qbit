// Copyright (c) 2019-2020 Andrew Demuskov
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
#include "mainchain.h"

#include <boost/logic/tribool.hpp>

namespace qbit {

using namespace qasm;

// forward
class Transaction;
class TxCoinBase;
class TxSpend;
class TxSpendPrivate;
class TxFee;

typedef std::shared_ptr<Transaction> TransactionPtr;
typedef std::shared_ptr<TxCoinBase> TxCoinBasePtr;
typedef std::shared_ptr<TxSpend> TxSpendPtr;
typedef std::shared_ptr<TxSpendPrivate> TxSpendPrivatePtr;
typedef std::shared_ptr<TxFee> TxFeePtr;

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

typedef std::shared_ptr<TransactionCreator> TransactionCreatorPtr;

//
// tx types
typedef std::map<unsigned short, TransactionCreatorPtr> TransactionTypes;
extern TransactionTypes gTxTypes; // TODO: init on startup

// tx version type
typedef unsigned char version_t;

//
// common serialization methods (inheritance)
#define ADD_INHERITABLE_SERIALIZE_METHODS		\
	inline void serialize(DataStream& s) {		\
		serialize<DataStream>(s);				\
	}											\
	inline void serialize(HashWriter& s) {		\
		serialize<HashWriter>(s);				\
	}											\
	inline void serialize(SizeComputer& s) {	\
		serialize<SizeComputer>(s);				\
	}

//
// generic transaction
class Transaction {
public:
	enum Type {
		UNDEFINED				= 0x0000,
		// value transfer
		COINBASE 				= 0x0001,	// QBIT coinbase transaction
		SPEND					= 0x0002,	// Spend transaction: any asset
		SPEND_PRIVATE			= 0x0003,	// Spend private transaction: any asset (default for all spend operations)
		FEE 					= 0x0004,	// Special type of transaction: fee for sharding puposes
		BASE 					= 0x0005,	// QBIT rewardless transaction: only fee collected without emission (shrding), unspendible
		BLOCKBASE 				= 0x0006,	// QBIT transaction: special tx type for rewarding and spending 

		// entity / action
		CONTRACT 				= 0x0010,	// Smart-contract publishing
		EVENT 					= 0x0011,	// Publish non-persistable event as transaction (ignition for smart-contracts processing)
		MESSAGE					= 0x0012,	// Create and send encrypted message (up to 256 bytes)
		SHARD					= 0x0013,	// Create chain (shard) transaction 
		DAPP					= 0x0014,	// DApp root

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

		// custom tx types for dapps, CUSTOM + 1 - first custom tx type
		CUSTOM_00 				= 0x1000, 	//
		CUSTOM_01 				= 0x1001,	//
		CUSTOM_02 				= 0x1002,	//
		CUSTOM_03 				= 0x1003,	//
		CUSTOM_04 				= 0x1004,	//
		CUSTOM_05 				= 0x1005,	//
		CUSTOM_06 				= 0x1006,	//
		CUSTOM_07 				= 0x1007,	//
		CUSTOM_08 				= 0x1008,	//
		CUSTOM_09 				= 0x1009,	//
		CUSTOM_10 				= 0x100a,	//
		CUSTOM_11 				= 0x100b,	//
		CUSTOM_12 				= 0x100c,	//
		CUSTOM_13 				= 0x100d,	//
		CUSTOM_14 				= 0x100e,	//
		CUSTOM_15 				= 0x100f,	//

		CUSTOM_16 				= 0x1010,	//
		CUSTOM_17 				= 0x1011,	//
		CUSTOM_18 				= 0x1012,	//
		CUSTOM_19 				= 0x1013,	//
		CUSTOM_20 				= 0x1014,	//
		CUSTOM_21 				= 0x1015,	//
		CUSTOM_22 				= 0x1016,	//
		CUSTOM_23 				= 0x1017,	//
		CUSTOM_24 				= 0x1018,	//
		CUSTOM_25 				= 0x1019,	//
		CUSTOM_26 				= 0x101a,	//
		CUSTOM_27 				= 0x101b,	//
		CUSTOM_28 				= 0x101c,	//
		CUSTOM_29 				= 0x101d,	//
		CUSTOM_30 				= 0x101e,	//
		CUSTOM_31 				= 0x101f,	//

		CUSTOM_32 				= 0x1020,	//
		CUSTOM_33 				= 0x1021,	//
		CUSTOM_34 				= 0x1022,	//
		CUSTOM_35 				= 0x1023,	//
		CUSTOM_36 				= 0x1024,	//
		CUSTOM_37 				= 0x1025,	//
		CUSTOM_38 				= 0x1026,	//
		CUSTOM_39 				= 0x1027,	//
		CUSTOM_40 				= 0x1028,	//
		CUSTOM_41 				= 0x1029,	//
		CUSTOM_42 				= 0x102a,	//
		CUSTOM_43 				= 0x102b,	//
		CUSTOM_44 				= 0x102c,	//
		CUSTOM_45 				= 0x102d,	//
		CUSTOM_46 				= 0x102e,	//
		CUSTOM_47 				= 0x102f,	//

		CUSTOM_48 				= 0x1030,	//
		CUSTOM_49 				= 0x1031,	//
		CUSTOM_50 				= 0x1032,	//
		CUSTOM_51 				= 0x1033,	//
		CUSTOM_52 				= 0x1034,	//
		CUSTOM_53 				= 0x1035,	//
		CUSTOM_54 				= 0x1036,	//
		CUSTOM_55 				= 0x1037,	//
		CUSTOM_56 				= 0x1038,	//
		CUSTOM_57 				= 0x1039,	//
		CUSTOM_58 				= 0x103a,	//
		CUSTOM_59 				= 0x103b,	//
		CUSTOM_60 				= 0x103c,	//
		CUSTOM_61 				= 0x103d,	//
		CUSTOM_62 				= 0x103e,	//
		CUSTOM_63 				= 0x103f	//
	};

	// use to register tx type functor
	static void registerTransactionType(Type, TransactionCreatorPtr);

	enum Status {
		CREATED 	= 0x01,
		ACCEPTED 	= 0x02,
		DECLINED 	= 0x03,
		PENDING		= 0x04
	};

	class Link {
	public:
		static constexpr uint32_t NULL_INDEX = std::numeric_limits<uint32_t>::max();

		uint256 chain_; // chain
		uint256 asset_; // asset type transaction
		uint256 tx_; // previous transaction hash
		uint32_t index_; // output index

		Link() : index_(NULL_INDEX) { asset_.setNull(); tx_.setNull(); chain_ = MainChain::id(); }
		//Link(const uint256& asset) : asset_(asset), index_(NULL_INDEX) { tx_.setNull(); chain_ = MainChain::id(); }
		//Link(const uint256& asset, uint32_t index) : asset_(asset), index_(index) { tx_.setNull(); chain_ = MainChain::id(); }
		//Link(const uint256& asset, const uint256& tx, uint32_t index) : asset_(asset), tx_(tx), index_(index) { chain_ = MainChain::id(); }

		Link(const uint256& chain, const uint256& asset) : chain_(chain), asset_(asset), index_(NULL_INDEX) { tx_.setNull(); }
		Link(const uint256& chain, const uint256& asset, uint32_t index) : chain_(chain), asset_(asset), index_(index) { tx_.setNull(); }
		Link(const uint256& chain, const uint256& asset, const uint256& tx, uint32_t index) : chain_(chain), asset_(asset), tx_(tx), index_(index) {}

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(chain_);
			READWRITE(asset_);
			READWRITE(tx_);
			READWRITE(index_);
		}

		void setNull() { asset_.setNull(); tx_.setNull(); index_ = NULL_INDEX; }
		bool isNull() const { return (asset_.isNull() && tx_.isNull() && index_ == NULL_INDEX); }

		inline void serialize(qbit::vector<unsigned char>& result) {
			result.insert(result.end(), chain_.begin(), chain_.end());
			result.insert(result.end(), asset_.begin(), asset_.end());
			result.insert(result.end(), tx_.begin(), tx_.end());
			result.insert(result.end(), (unsigned char*)(&index_), (unsigned char*)(&index_) + sizeof(index_));
		}

		const uint256& chain() const { return chain_; }
		const uint256& asset() const { return asset_; }
		const uint256& tx() const { return tx_; }
		uint32_t index() const { return index_; }

		void setChain(const uint256& chain) { chain_ = chain; }
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

		Link& out() { return out_; }
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
	class UnlinkedOut;
	typedef std::shared_ptr<Transaction::UnlinkedOut> UnlinkedOutPtr;

	class UnlinkedOut {
	public:
		class Updater {
		public:
			Updater() {}
			virtual void update(const UnlinkedOut&) {}
		};

		typedef std::shared_ptr<Transaction::UnlinkedOut> UpdaterPtr;

	public:
		UnlinkedOut() {}
		UnlinkedOut(const UnlinkedOut& utxo) {
			out_ = const_cast<UnlinkedOut&>(utxo).out();
			address_ = const_cast<UnlinkedOut&>(utxo).address();
			amount_ = utxo.amount();
			blind_ = utxo.blind();
			nonce_ = utxo.nonce();
			commit_ = utxo.commit();
			lock_ = utxo.lock();
			change_ = utxo.change();
		} 
		UnlinkedOut(const Link& o) : 
			out_(o) {}
		UnlinkedOut(const Link& o, uint64_t lock) : 
			out_(o), lock_(lock) {}
		UnlinkedOut(const Link& o, uint64_t lock, unsigned char change) : 
			out_(o), lock_(lock), change_(change) {}
		UnlinkedOut(const Link& o, const PKey& address) : 
			out_(o), address_(address) {}
		UnlinkedOut(const Link& o, const PKey& address, uint64_t lock) : 
			out_(o), address_(address), lock_(lock) {}
		UnlinkedOut(const Link& o, const PKey& address, uint64_t lock, unsigned char change) : 
			out_(o), address_(address), lock_(lock), change_(change) {}
		UnlinkedOut(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c) : 
			out_(o), address_(address), amount_(a), blind_(b), commit_(c) {}
		UnlinkedOut(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, unsigned char change) : 
			out_(o), address_(address), amount_(a), blind_(b), commit_(c), change_(change) {}
		UnlinkedOut(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, uint64_t lock) : 
			out_(o), address_(address), amount_(a), blind_(b), commit_(c), lock_(lock) {}
		UnlinkedOut(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, uint64_t lock, unsigned char change) : 
			out_(o), address_(address), amount_(a), blind_(b), commit_(c), lock_(lock), change_(change) {}
		UnlinkedOut(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, const uint256& n) : 
			UnlinkedOut(o, address, a, b, c) { nonce_ = n; }
		UnlinkedOut(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, const uint256& n, unsigned char change) : 
			UnlinkedOut(o, address, a, b, c) { nonce_ = n; change_ = change; }
		UnlinkedOut(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, const uint256& n, uint64_t lock) : 
			UnlinkedOut(o, address, a, b, c) { nonce_ = n; lock_ = lock; }
		UnlinkedOut(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, const uint256& n, uint64_t lock, unsigned char change) : 
			UnlinkedOut(o, address, a, b, c) { nonce_ = n; lock_ = lock; change_ = change; }

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(out_);
			READWRITE(address_);
			READWRITE(amount_);
			READWRITE(blind_);
			READWRITE(nonce_);
			READWRITE(commit_);
			READWRITE(lock_);
			READWRITE(change_);
		}

		Link& out() { return out_; }
		amount_t amount() const { return amount_; }
		const uint256& blind() const { return blind_; }
		const uint256& nonce() const { return nonce_; }
		PKey& address() { return address_; }
		const std::vector<unsigned char>& commit() const { return commit_; }
		uint64_t lock() const { return lock_; }

		uint256 hash() { return out_.hash(); }
		unsigned char change() const { return change_; }
		bool blinded() const { return !nonce_.isNull(); }

		static UnlinkedOutPtr instance() { return std::make_shared<UnlinkedOut>(); }
		static UnlinkedOutPtr instance(const UnlinkedOut& utxo) { return std::make_shared<UnlinkedOut>(utxo); }
		static UnlinkedOutPtr instance(const Link& o) { return std::make_shared<UnlinkedOut>(o); }
		static UnlinkedOutPtr instance(const Link& o, uint64_t lock) { return std::make_shared<UnlinkedOut>(o, lock); }
		static UnlinkedOutPtr instance(const Link& o, uint64_t lock, unsigned char change) { return std::make_shared<UnlinkedOut>(o, lock, change); }
		static UnlinkedOutPtr instance(const Link& o, const PKey& address) { return std::make_shared<UnlinkedOut>(o, address); }
		static UnlinkedOutPtr instance(const Link& o, const PKey& address, uint64_t lock) { return std::make_shared<UnlinkedOut>(o, address, lock); }
		static UnlinkedOutPtr instance(const Link& o, const PKey& address, uint64_t lock, unsigned char change) { return std::make_shared<UnlinkedOut>(o, address, lock, change); }
		static UnlinkedOutPtr instance(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c) 
		{ return std::make_shared<UnlinkedOut>(o, address, a, b, c); }
		static UnlinkedOutPtr instance(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, unsigned char change) 
		{ return std::make_shared<UnlinkedOut>(o, address, a, b, c, change); }
		static UnlinkedOutPtr instance(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, uint64_t lock) 
		{ return std::make_shared<UnlinkedOut>(o, address, a, b, c, lock); }
		static UnlinkedOutPtr instance(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, uint64_t lock, unsigned char change) 
		{ return std::make_shared<UnlinkedOut>(o, address, a, b, c, lock, change); }
		static UnlinkedOutPtr instance(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, const uint256& n) 
		{ return std::make_shared<UnlinkedOut>(o, address, a, b, c, n); }
		static UnlinkedOutPtr instance(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, const uint256& n, unsigned char change) 
		{ return std::make_shared<UnlinkedOut>(o, address, a, b, c, n, change); }
		static UnlinkedOutPtr instance(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, const uint256& n, uint64_t lock) 
		{ return std::make_shared<UnlinkedOut>(o, address, a, b, c, n, lock); }
		static UnlinkedOutPtr instance(const Link& o, const PKey& address, amount_t a, const uint256& b, const std::vector<unsigned char>& c, const uint256& n, uint64_t lock, unsigned char change) 
		{ return std::make_shared<UnlinkedOut>(o, address, a, b, c, n, lock, change); }

	private:
		Link out_; // one of previous out
		PKey address_;
		amount_t amount_ = 0;
		uint256 blind_;
		uint256 nonce_;
		std::vector<unsigned char> commit_;
		uint64_t lock_ = 0;
		unsigned char change_ = 0;
	};

	class NetworkUnlinkedOut;
	typedef std::shared_ptr<Transaction::NetworkUnlinkedOut> NetworkUnlinkedOutPtr;

	class NetworkUnlinkedOut {
	public:
		enum Direction {
			O_IN = 1,
			O_OUT = 2
		};
	public:
		NetworkUnlinkedOut() {}
		NetworkUnlinkedOut(const UnlinkedOut& utxo, uint64_t confirms, bool coinbase) : 
			utxo_(utxo), confirms_(confirms), coinbase_(coinbase) {}
		NetworkUnlinkedOut(const UnlinkedOut& utxo, unsigned short type, uint64_t confirms, bool coinbase) : 
			utxo_(utxo), type_(type), confirms_(confirms), coinbase_(coinbase) {}
		NetworkUnlinkedOut(const UnlinkedOut& utxo, unsigned short type, uint64_t confirms, bool coinbase, uint64_t fee) : 
			utxo_(utxo), type_(type), confirms_(confirms), coinbase_(coinbase), fee_(fee) {}
		NetworkUnlinkedOut(const UnlinkedOut& utxo, unsigned short type, uint64_t confirms, bool coinbase, const uint256& parent, unsigned short parentType) : 
			utxo_(utxo), type_(type), confirms_(confirms), coinbase_(coinbase), parent_(parent), parentType_(parentType) {}
		NetworkUnlinkedOut(const NetworkUnlinkedOut& txo) {
			utxo_ = const_cast<NetworkUnlinkedOut&>(txo).utxo();
			confirms_ = const_cast<NetworkUnlinkedOut&>(txo).confirms();
			coinbase_ = const_cast<NetworkUnlinkedOut&>(txo).coinbase();
			type_ = const_cast<NetworkUnlinkedOut&>(txo).type();
			timestamp_ = const_cast<NetworkUnlinkedOut&>(txo).timestamp();
			parent_ = const_cast<NetworkUnlinkedOut&>(txo).parent();
			parentType_ = const_cast<NetworkUnlinkedOut&>(txo).parentType();
			direction_ = const_cast<NetworkUnlinkedOut&>(txo).direction();
			fee_ = const_cast<NetworkUnlinkedOut&>(txo).fee();
		}

		inline UnlinkedOut utxo() { return utxo_; }
		inline void setUtxo(const UnlinkedOut& utxo) { utxo_ = utxo; }
		inline uint64_t confirms() { return confirms_; }
		inline bool coinbase() { return coinbase_; }
		inline unsigned short type() { return type_; }
		inline uint64_t timestamp() { return timestamp_; }
		inline Direction direction() { return (Direction)direction_; }
		inline uint64_t fee() { return fee_; }

		inline unsigned short parentType() { return parentType_; }
		inline uint256& parent() { return parent_; }

		inline void setParentType(unsigned short type) { parentType_ = type; }
		inline void setParent(const uint256& parent) { parent_ = parent; }
		inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }
		inline void setDirection(Direction direction) { direction_ = direction; }
		inline void setConfirms(uint64_t confirms) { confirms_ = confirms; }
		inline void setFee(uint64_t fee) { fee_ = fee; }

		ADD_SERIALIZE_METHODS;

		template <typename Stream, typename Operation>
		inline void serializationOp(Stream& s, Operation ser_action) {
			READWRITE(utxo_);
			READWRITE(direction_);
			READWRITE(type_);
			READWRITE(confirms_);
			READWRITE(coinbase_);
			READWRITE(timestamp_);
			READWRITE(fee_);

			if (type_ != Transaction::UNDEFINED) {
				READWRITE(parent_);
				READWRITE(parentType_);
			}
		}		

		static NetworkUnlinkedOutPtr instance(const NetworkUnlinkedOut& txo) { return std::make_shared<NetworkUnlinkedOut>(txo); }
		static NetworkUnlinkedOutPtr instance(const UnlinkedOut& utxo, uint64_t confirms, bool coinbase) { 
			return std::make_shared<NetworkUnlinkedOut>(utxo, confirms, coinbase); 
		}
		static NetworkUnlinkedOutPtr instance(const UnlinkedOut& utxo, unsigned short type, uint64_t confirms, bool coinbase) { 
			return std::make_shared<NetworkUnlinkedOut>(utxo, type, confirms, coinbase); 
		}
		static NetworkUnlinkedOutPtr instance(const UnlinkedOut& utxo, unsigned short type, uint64_t confirms, bool coinbase, uint64_t fee) { 
			return std::make_shared<NetworkUnlinkedOut>(utxo, type, confirms, coinbase, fee); 
		}
		static NetworkUnlinkedOutPtr instance(const UnlinkedOut& utxo, unsigned short type, uint64_t confirms, bool coinbase, const uint256& parent, unsigned short parentType) { 
			return std::make_shared<NetworkUnlinkedOut>(utxo, type, confirms, coinbase, parent, parentType); 
		}

	private:
		UnlinkedOut utxo_;
		unsigned short type_ = Transaction::UNDEFINED;
		uint64_t confirms_ = 0;
		bool coinbase_ = false;
		uint64_t timestamp_ = 0;
		char direction_ = 0;
		uint64_t fee_ = 0;

		// parent
		uint256 parent_;
		unsigned short parentType_ = Transaction::UNDEFINED;
	};

	class Serializer {
	public:

		template<typename Stream, typename Tx>
		static inline void serialize(Stream& s, Tx* tx) {
			s << (unsigned short)tx->type();
			s << tx->version();
			s << tx->timeLock();
			s << tx->chain();
			s << tx->in();
			s << tx->out();

			if (s.GetType() == SER_NETWORK) {
				s << tx->block();
				s << tx->height();
				s << tx->confirms();
				s << tx->index();
				s << tx->mempool();
			}

			tx->serialize(s);
		}

		template<typename Stream>
		static inline void serialize(Stream& s, TransactionPtr tx) {
			s << (unsigned short)tx->type();
			s << tx->version();
			s << tx->timeLock();
			s << tx->chain();
			s << tx->in();
			s << tx->out();

			if (s.GetType() == SER_NETWORK) {
				s << tx->block();
				s << tx->height();
				s << tx->confirms();
				s << tx->index();
				s << tx->mempool();
			}

			tx->serialize(s);
		}
	};

	class Deserializer {
	public:
		template<typename Stream>
		static inline TransactionPtr deserialize(Stream& s);
	};	

	Transaction() { status_ = Status::CREATED; id_.setNull(); timeLock_ = 0; chain_ = MainChain::id(); version_ = 0; }

	virtual void serialize(DataStream& s) {}
	virtual void serialize(HashWriter& s) {}
	virtual void serialize(SizeComputer& s) {}
	virtual void deserialize(DataStream& s) {}
	virtual bool finalize(const SKey&) { return false; }

	uint256 hash();
	inline uint256 id() { if (id_.isEmpty()) hash(); return id_; }

	inline std::vector<In>& in() { return in_; }
	inline std::vector<Out>& out() { return out_; }

	inline uint64_t confirms() { return confirms_; }
	inline void setConfirms(uint64_t confirms) { confirms_ = confirms; }
	inline uint64_t height() { return height_; }
	inline void setHeight(uint64_t height) { height_ = height; }
	inline uint32_t index() { return height_; }
	inline void setIndex(uint32_t index) { index_ = index; }
	inline bool mempool() { return mempool_; }
	inline void setMempool(bool mempool) { mempool_ = mempool; }
	inline uint256& block() { return block_; }
	inline void setBlock(const uint256& block) { block_ = block; }

	inline Out* out(uint32_t o) { if (o < out_.size()) return &out_[o]; return nullptr; }

	inline Type type() { return type_; }
	inline version_t version() { return version_; }
	virtual inline std::string name() { return "basic"; }

	inline Status status() { return status_; }
	inline void setStatus(Status status) { status_ = status; }	

	virtual std::string toString();

	inline uint64_t timeLock() { return timeLock_; }
	inline void setTimelock(uint64_t timelock) { timeLock_ = timelock; }

	virtual In& addFeeIn(const SKey&, UnlinkedOutPtr) { throw qbit::exception("NOT_IMPL", "Not implemented."); }
	virtual Transaction::UnlinkedOutPtr addFeeOut(const SKey&, const PKey&, const uint256&, amount_t) { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	virtual bool isValue(UnlinkedOutPtr) { return false; }
	virtual bool isEntity(UnlinkedOutPtr) { return false; }
	virtual bool isEntity() { return false; }
	virtual bool isFeeFee() { return false; }
	
	virtual bool isContinuousData() { return false; }
	
	virtual std::string entityName() { throw qbit::exception("NOT_IMPL", "Not implemented."); }

	virtual inline void setChain(const uint256& chain) { chain_ = chain; }
	inline uint256& chain() { return chain_; }

	virtual void properties(std::map<std::string, std::string>&) {
	}

	virtual std::list<Transaction::UnlinkedOutPtr> utxos(const uint256& asset) {
		return std::list<Transaction::UnlinkedOutPtr>();
	}

	inline bool hasOuterIns() {
		//
		if (hasOuterIns_ == -1) {
			for (std::vector<In>::iterator lIn = in_.begin(); lIn != in_.end(); lIn++) {
				if ((*lIn).out().chain() != chain_) { hasOuterIns_ = 1; break; }
			}

			if (hasOuterIns_ == -1) hasOuterIns_ = 0;
		}

		return (hasOuterIns_ == 1 ? true : false);
	}

protected:
	// tx type
	Type type_; // 2 bytes
	// tx version
	version_t version_; // 1 byte
	// branch (shard)
	uint256 chain_; // 32 b

	// inputs
	std::vector<In> in_;
	// outputs
	std::vector<Out> out_;

	// time-lock
	uint64_t timeLock_;

	//
	// in-memory only
	//

	// block
	uint256 block_;
	// height
	uint64_t height_ = 0;
	// confirms
	uint64_t confirms_ = 0;
	// index
	uint32_t index_ = 0;
	// mempool
	bool mempool_ = false;
	// tx current status
	Status status_;
	// id
	uint256 id_;
	// has outer ins
	int hasOuterIns_ = -1;
};

//
// Coinbase transaction
class TxCoinBase : public Transaction {
public:
	TxCoinBase() {
		type_ = Transaction::COINBASE;
	}

	In& addIn() {
		in_.resize(1);
		in_[0].out_.setNull();
		in_[0].ownership_ = ByteCode() <<
			OP(QMOV) << REG(QR0) << CU8(0x00);

		return in_[0];
	}

	Transaction::UnlinkedOutPtr addOut(const SKey& skey, const PKey& pkey, const uint256& asset, amount_t amount) {
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
			OP(QMOV) 	<< REG(QR0) << CU8(0x01) <<	
			OP(QRET);

		return Transaction::UnlinkedOut::instance(
			Transaction::Link(chain(), asset, 0), // link
			pkey,
			amount, // amount
			lBlind, // blinding key
			lCommitment // commit
		);
	}

	inline std::string name() { return "coinbase"; }

	inline void serialize(DataStream& s) {}
	inline void deserialize(DataStream& s) {}

	bool isValue(UnlinkedOutPtr) { return true; }
	bool isEntity(UnlinkedOutPtr) { return false; }
};

//
// Spend transaction
class TxSpend : public Transaction {
public:
	TxSpend() { type_ = Transaction::SPEND; }

	//
	// skey - our secret key
	// utxo - unlinked tx out
	virtual In& addIn(const SKey& skey, UnlinkedOutPtr utxo) {
		Transaction::In lIn;
		lIn.out().setNull();
		lIn.out().setChain(utxo->out().chain());
		lIn.out().setAsset(utxo->out().asset());
		lIn.out().setTx(utxo->out().tx());
		lIn.out().setIndex(utxo->out().index());

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
		assetIn_[utxo->out().asset()].push_back(utxo);

		in_.push_back(lIn);
		return in_[in_.size()-1];
	}

	//
	// skey - our secret key
	// pkey - destination address
	// asset - asset type hash
	virtual Transaction::UnlinkedOutPtr addOut(const SKey& skey, const PKey& pkey, const uint256& asset, amount_t amount, bool change = false) {
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
			OP(QMOV) 		<< REG(QA6) << CU8((unsigned char)change) <<
			OP(QCHECKA) 	<<
			OP(QATXOA) 		<<
			OP(QEQADDR) 	<<
			OP(QMOV) 		<< REG(QR0) << CU8(0x01) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(chain(), asset, out_.size()), // link
			pkey,
			amount, // amount
			lBlind, // blinding key
			lCommitment, // commit
			(unsigned char)change
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
	virtual Transaction::UnlinkedOutPtr addLockedOut(const SKey& skey, const PKey& pkey, const uint256& asset, amount_t amount, uint64_t height) {
		qbit::vector<unsigned char> lCommitment;

		uint256 lBlind = const_cast<SKey&>(skey).shared(pkey);
		Math::mul(lBlind, lBlind, Random::generate());

		if (!const_cast<SKey&>(skey).context()->createCommitment(lCommitment, lBlind, amount)) {
			throw qbit::exception("INVALID_COMMITMENT", "Commitment creation failed.");
		}

		Transaction::Out lOut;
		lOut.setAsset(asset);
		lOut.setDestination(ByteCode() 		<<
							OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
							OP(QMOV) 		<< REG(QA0) << CU64(amount) <<
							OP(QMOV) 		<< REG(QA1) << CVAR(lCommitment) <<	
							OP(QMOV) 		<< REG(QA2) << CU256(lBlind) <<	
							OP(QCHECKA) 	<<
							// NOTE: QR1 and CHECKH _should_ be called prior to any ATXO instructions
							OP(QMOV) 		<< REG(QR1) << CU64(height) <<	
							OP(QCHECKH) 	<< REG(QR1) <<	// check height: height < th4, set c0 & c1
							OP(QATXOA) 		<<				// make utxo (push)
							OP(QJMPT) 		<< TO(1000) <<	// if true - return (goto), false - continue
							OP(QEQADDR) 	<<
							OP(QMOV) 		<< REG(QR0) << CU8(0x01) <<	
			LAB(1000) <<	OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(chain(), asset, out_.size()), // link
			pkey,
			amount, // amount
			lBlind, // blinding key
			lCommitment, // commit
			height
		);

		// fill up for finalization
		assetOut_[asset].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}

	//
	// skey - our secret key
	// asset - asset type hash
	virtual Transaction::UnlinkedOutPtr addFeeOut(const SKey& skey, const uint256& asset, amount_t amount) {
		qbit::vector<unsigned char> lCommitment;

		uint256 lBlind = Random::generate();
		if (!const_cast<SKey&>(skey).context()->createCommitment(lCommitment, lBlind, amount)) {
			throw qbit::exception("INVALID_COMMITMENT", "Commitment creation failed.");
		}

		PKey lPKey = const_cast<SKey&>(skey).createPKey();

		Transaction::Out lOut;
		lOut.setAsset(asset);
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CU16(0xFFFA) << // miner
			OP(QMOV) 		<< REG(QA0) << CU64(amount) << // always open amount
			OP(QMOV) 		<< REG(QA1) << CVAR(lCommitment) <<			
			OP(QMOV) 		<< REG(QA2) << CU256(lBlind) <<			
			OP(QCHECKA) 	<<
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(chain(), asset, out_.size()), // link
			lPKey,
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

	virtual bool isValue(UnlinkedOutPtr) { return true; }
	virtual bool isEntity(UnlinkedOutPtr) { return false; }	

	virtual std::list<Transaction::UnlinkedOutPtr> utxos(const uint256& asset) {
		_assetMap::iterator lItem = assetOut_.find(asset);
		if (lItem != assetOut_.end())
			return lItem->second;
 
		return std::list<Transaction::UnlinkedOutPtr>();
	}

	//
	// balance amounts and make extra out for balance check
	// acts as finalization for create
	virtual bool finalize(const SKey&);

protected:
	typedef std::map<uint256, std::list<Transaction::UnlinkedOutPtr>> _assetMap;
	_assetMap assetIn_; // in asset group
	_assetMap assetOut_; // out asset group
};

//
// Spend private transaction
class TxSpendPrivate : public TxSpend {
public:
	TxSpendPrivate() { type_ = Transaction::SPEND_PRIVATE; }

	//
	// skey - our secret key
	// pkey - destination address
	// asset - asset type hash
	Transaction::UnlinkedOutPtr addOut(const SKey& skey, const PKey& pkey, const uint256& asset, amount_t amount, bool change = false) {
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
			OP(QMOV) 		<< REG(QA6) << CU8((unsigned char)change) <<
			OP(QCHECKP) 	<<
			OP(QATXOP)	 	<<
			OP(QEQADDR) 	<<
			OP(QMOV) 		<< REG(QR0) << CU8(0x01) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(chain(), asset, out_.size()), // link
			pkey,
			amount, // amount
			lBlind, // blinding key
			lCommitment, // commit
			lNonce,
			(unsigned char)change
		);

		// fill up for finalization
		assetOut_[asset].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}

	inline std::string name() { return "spend_private"; }

	inline void serialize(DataStream& s) {}
	inline void deserialize(DataStream& s) {}

	virtual bool isValue(UnlinkedOutPtr) { return true; }
	virtual bool isEntity(UnlinkedOutPtr) { return false; }		

	//
	// balance amounts and make extra out for balance check
	// acts as finalization for create
	bool finalize(const SKey&);	
};

//
// Fee transaction
class TxFee : public TxSpend {
public:
	TxFee() { type_ = Transaction::FEE; }

	virtual Transaction::UnlinkedOutPtr addExternalOut(const SKey& skey, const PKey& pkey, const uint256& asset, amount_t amount) {
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
			OP(QTIFMC)		<<
			OP(QMOV) 		<< REG(QR0) << CU8(0x01) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(chain(), asset, out_.size()), // link
			pkey,
			amount, // amount
			lBlind, // blinding key
			lCommitment // commit
		);

		// fill up for finalization
		assetOut_[asset].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}

	inline std::string name() { return "fee"; }

	inline void serialize(DataStream& s) {}
	inline void deserialize(DataStream& s) {}

	virtual bool isValue(UnlinkedOutPtr) { return true; }
	virtual bool isEntity(UnlinkedOutPtr) { return false; }		
};

//
template<typename Stream>
inline TransactionPtr Transaction::Deserializer::deserialize(Stream& s) {
	unsigned short lTxType = 0x0000;
	s >> lTxType;
	Transaction::Type lType = (Transaction::Type)lTxType;

	TransactionPtr lTx = nullptr;
	switch(lType) {
		case Transaction::COINBASE: lTx = TransactionPtr(static_cast<Transaction*>(new TxCoinBase())); break;
		case Transaction::SPEND: lTx = TransactionPtr(static_cast<Transaction*>(new TxSpend())); break;
		case Transaction::SPEND_PRIVATE: lTx = TransactionPtr(static_cast<Transaction*>(new TxSpendPrivate())); break;
		case Transaction::FEE: lTx = TransactionPtr(static_cast<Transaction*>(new TxFee())); break;
		default: {
			TransactionTypes::iterator lTypeIterator = gTxTypes.find(lType);
			if (lTypeIterator != gTxTypes.end()) {
				lTx = lTypeIterator->second->create();
			}
		}
		break;			
	}

	if (lTx != nullptr) {
		s >> lTx->version_;
		s >> lTx->timeLock_;
		s >> lTx->chain_;
		s >> lTx->in();
		s >> lTx->out();

		if (s.GetType() == SER_NETWORK) {
			s >> lTx->block_;
			s >> lTx->height_;
			s >> lTx->confirms_;
			s >> lTx->index_;
			s >> lTx->mempool_;
		}

		lTx->deserialize(s);
	}

	return lTx;
}

//
// DataStream specialization
template void Transaction::Serializer::serialize<DataStream>(DataStream&, TransactionPtr);
template TransactionPtr Transaction::Deserializer::deserialize<DataStream>(DataStream&);

//
// Tx Factory
//
class TransactionFactory {
public:
	static TransactionPtr create(Transaction::Type txType) {
		switch(txType) {
		    case Transaction::COINBASE: return TransactionPtr(static_cast<Transaction*>(new TxCoinBase()));
		    case Transaction::SPEND: return TransactionPtr(static_cast<Transaction*>(new TxSpend()));
		    case Transaction::SPEND_PRIVATE: return TransactionPtr(static_cast<Transaction*>(new TxSpendPrivate()));
		    case Transaction::FEE: return TransactionPtr(static_cast<Transaction*>(new TxFee()));

			default: {
				TransactionTypes::iterator lType = gTxTypes.find(txType);
				if (lType != gTxTypes.end()) {
					return lType->second->create();
				}
			}
			break;			
		}

		return TransactionPtr();
	}
};

} // qbit

#endif
