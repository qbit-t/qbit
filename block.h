// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BLOCK_H
#define QBIT_BLOCK_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"

#include "transaction.h"

namespace qbit {

class BlockHeader {
public:
	// header
	int32_t version_;
	uint256 chain_;
	uint256 prev_;
	uint256 root_;
	uint64_t time_;
	uint32_t bits_;
	uint32_t nonce_;
	uint32_t qbits_; // qbit cout txs

	BlockHeader() {
		setNull();
	}

	template <typename Stream>
	void serialize(Stream& s) const {
		s << version_;
		s << chain_;	
		s << prev_;		
		s << root_;
		s << time_;		
		s << bits_;		
		s << nonce_;
		s << qbits_;
	}

	template <typename Stream>
	void deserialize(Stream& s) {
		s >> version_;
		s >> chain_;
		s >> prev_;		
		s >> root_;
		s >> time_;		
		s >> bits_;		
		s >> nonce_;
		s >> qbits_;
	}

	void setNull() {
		version_ = 0;
		chain_ = MainChain::id();
		prev_.setNull();
		root_.setNull();
		time_ = 0;
		bits_ = 0;
		nonce_ = 0;
		qbits_ = 0;
	}

	inline bool isNull() const {
		return (bits_ == 0);
	}

	uint256 hash();

	inline uint64_t time() const {
		return time_;
	}
	inline void setTime(uint64_t time) { time_ = time; } 


	inline void setQbits(uint32_t qbits) { qbits_ = qbits; }
	inline uint32_t qbits() { return qbits_; }

	inline void setChain(const uint256& chain) { chain_ = chain; }
	inline uint256 chain() { return chain_; }
};

// forward
typedef std::vector<TransactionPtr> TransactionsContainer;

class BlockTransactions;
typedef std::shared_ptr<BlockTransactions> BlockTransactionsPtr;

class BlockTransactions {
public:
	// network and disk
	TransactionsContainer transactions_;

	BlockTransactions() {}
	BlockTransactions(const TransactionsContainer& txs) {
		transactions_.insert(transactions_.end(), txs.begin(), txs.end());
	}

	class Serializer {
	public:
		template<typename Stream>
		static inline void serialize(Stream& s, BlockTransactions* txs) {
			WriteCompactSize(s, txs->transactions_.size());
			for (TransactionsContainer::const_iterator lTx = txs->transactions_.begin(); 
				lTx != txs->transactions_.end(); ++lTx) {
				Transaction::Serializer::serialize<Stream>(s, *lTx);
			}
		}

		template<typename Stream>
		static inline void serialize(Stream& s, BlockTransactionsPtr txs) {
			WriteCompactSize(s, txs->transactions_.size());
			for (TransactionsContainer::const_iterator lTx = txs->transactions_.begin(); 
				lTx != txs->transactions_.end(); ++lTx) {
				Transaction::Serializer::serialize<Stream>(s, *lTx);
			}
		}
	};

	class Deserializer {
	public:
		template<typename Stream>
		static inline BlockTransactionsPtr deserialize(Stream& s) {
			BlockTransactionsPtr lTxs = std::make_shared<BlockTransactions>();
			
			unsigned int lSize = ReadCompactSize(s);
			unsigned int lIdx = 0;
			
			lTxs->transactions_.resize(lSize);

			while (lIdx < lSize)
			{
				TransactionPtr lTx = Transaction::Deserializer::deserialize<Stream>(s);
				lTxs->transactions_[lIdx++] = lTx;
			}

			return lTxs;
		}
	};

	static BlockTransactionsPtr instance() { return std::make_shared<BlockTransactions>(); }
	static BlockTransactionsPtr instance(const TransactionsContainer& txs) { return std::make_shared<BlockTransactions>(txs); }

	void append(TransactionPtr tx) { transactions_.push_back(tx); }
	TransactionsContainer& transactions() { return transactions_; }	
};

class Block;
typedef std::shared_ptr<Block> BlockPtr;

class Block: public BlockHeader, public BlockTransactions {
public:
	class Serializer {
	public:
		template<typename Stream>
		static inline void serialize(Stream& s, Block* block) {
			block->serialize<Stream>(s);
			BlockTransactions::Serializer::serialize<Stream>(s, block);
		}

		template<typename Stream>
		static inline void serialize(Stream& s, BlockPtr block) {
			block->serialize<Stream>(s);
			BlockTransactions::Serializer::serialize<Stream>(s, block);
		}
	};

	class Deserializer {
	public:
		template<typename Stream>
		static inline BlockPtr deserialize(Stream& s) {
			BlockPtr lBlock = std::make_shared<Block>();

			lBlock->deserialize<Stream>(s);
			
			unsigned int lSize = ReadCompactSize(s);
			unsigned int lIdx = 0;
			
			lBlock->transactions_.resize(lSize);

			while (lIdx < lSize)
			{
				TransactionPtr lTx = Transaction::Deserializer::deserialize<Stream>(s);
				lBlock->transactions_[lIdx++] = lTx;
			}

			return lBlock;
		}
	};

	Block() {
		setNull();
	}

	Block(const BlockHeader &header) {
		setNull();
		*(static_cast<BlockHeader*>(this)) = header;
	}

	void setNull() {
		BlockHeader::setNull();
		transactions_.clear();
	}

	BlockHeader blockHeader() const {
		BlockHeader lBlock(*(BlockHeader*)this);
		return lBlock;
	}

	BlockTransactionsPtr blockTransactions() { return BlockTransactions::instance(transactions_); }

	std::string toString();

	static BlockPtr instance() { return std::make_shared<Block>(); }
	static BlockPtr instance(const BlockHeader& header) { return std::make_shared<Block>(header); }

	void append(BlockTransactionsPtr txs) {
		transactions_.insert(transactions_.end(), txs->transactions().begin(), txs->transactions().end()); 
	}

	void append(TransactionPtr tx) { transactions_.push_back(tx); }
	TransactionsContainer& transactions() { return transactions_; }

	bool equals(BlockPtr other);
};

} // qbit

#endif
