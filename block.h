// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BLOCK_H
#define QBIT_BLOCK_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../allocator.h"

#include "transaction.h"

namespace qbit {

class BlockHeader {
public:
	// header
	int32_t version_;
	uint256 prev_;
	uint256 root_;
	uint32_t time_;
	uint32_t bits_;
	uint32_t nonce_;

	BlockHeader() {
		setNull();
	}

	template <typename Stream>
	void serialize(Stream& s) {
		s << version_;		
		s << prev_;		
		s << root_;
		s << time_;		
		s << bits_;		
		s << nonce_;
	}

	template <typename Stream>
	void deserialize(Stream& s) {
		s >> version_;		
		s >> prev_;		
		s >> root_;
		s >> time_;		
		s >> bits_;		
		s >> nonce_;
	}

	void setNull() {
		version_ = 0;
		prev_.setNull();
		root_.setNull();
		time_ = 0;
		bits_ = 0;
		nonce_ = 0;
	}

	bool isNull() const {
		return (bits_ == 0);
	}

	uint256 hash();

	int64_t time() const {
		return (int64_t)time_;
	}
};

// forward
class Block;
typedef std::shared_ptr<Block> BlockPtr;
typedef std::vector<TransactionPtr> TransactionsContainer;

class Block : public BlockHeader {
public:
	// network and disk
	TransactionsContainer transactions_;

	// memory only
	mutable bool checked_;

	class Serializer {
	public:
		template<typename Stream>
		static inline void serialize(Stream& s, Block* block) {
			block->serialize<Stream>(s);

			WriteCompactSize(s, block->transactions_.size());
			for (TransactionsContainer::const_iterator lTx = block->transactions_.begin(); 
				lTx != block->transactions_.end(); ++lTx) {
				Transaction::Serializer::serialize<Stream>(s, *lTx);
			}
		}

		template<typename Stream>
		static inline void serialize(Stream& s, BlockPtr block) {
			block->serialize<Stream>(s);

			WriteCompactSize(s, block->transactions_.size());
			for (TransactionsContainer::const_iterator lTx = block->transactions_.begin(); 
				lTx != block->transactions_.end(); ++lTx) {
				Transaction::Serializer::serialize<Stream>(s, *lTx);
			}
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
		checked_ = false;
	}

	BlockHeader blockHeader() const {
		BlockHeader lBlock;
		lBlock.version_ = version_;
		lBlock.prev_	= prev_;
		lBlock.root_	= prev_;
		lBlock.time_	= time_;
		lBlock.bits_ 	= bits_;
		lBlock.nonce_	= nonce_;
		return lBlock;
	}

	std::string toString();

	static BlockPtr create() { return std::make_shared<Block>(); }

	void append(TransactionPtr tx) { transactions_.push_back(tx); }
	TransactionsContainer& transactions() { return transactions_; }

	bool equals(BlockPtr other);
};

} // qbit

#endif
