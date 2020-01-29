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

	inline void setPrev(const uint256& prev) { prev_ = prev; }
	inline uint256 prev() { return prev_; }

	inline void setRoot(const uint256& root) { root_ = root; }
	inline uint256 root() { return root_; }
};

// broadcast found block
class NetworkBlockHeader {
public:
	NetworkBlockHeader() {}
	NetworkBlockHeader(const BlockHeader &header, size_t height, /*TransactionPtr coinbase,*/ const uint160& address) : 
		header_(header), /*coinbase_(coinbase),*/ address_(address), height_(height) {}

	BlockHeader& blockHeader() { return header_; }
	//TransactionPtr coinbase() { return coinbase_; }
	uint160 addressId() { return address_; }
	size_t height() { return height_; }

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		if (ser_action.ForRead()) {
			header_.deserialize(s); // header info
			//coinbase_ = Transaction::Deserializer::deserialize<Stream>(s); // "mined" transaction
			s >> address_;
			s >> height_;
		} else {
			header_.serialize(s);
			//Transaction::Serializer::serialize<Stream>(s, coinbase_);
			s << address_;
			s << height_;
		}
	}

private:
	BlockHeader header_;
	//TransactionPtr coinbase_;
	uint160 address_;
	size_t height_;
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

	void transactionsHashes(std::vector<uint256>&);
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

	Block(const BlockHeader &header, BlockTransactionsPtr transactions) {
		setNull();
		*(static_cast<BlockHeader*>(this)) = header;
		append(transactions);
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
	static BlockPtr instance(const BlockHeader& header, BlockTransactionsPtr transactions) { 
		return std::make_shared<Block>(header, transactions); 
	}

	BlockPtr clone();

	void append(BlockTransactionsPtr txs) {
		transactions_.insert(transactions_.end(), txs->transactions().begin(), txs->transactions().end()); 
	}

	void append(TransactionPtr tx) { transactions_.push_back(tx); }
	TransactionsContainer& transactions() { return transactions_; }

	bool equals(BlockPtr other);
};

} // qbit

#endif
