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
	PKey origin_; // block emitent
	uint64_t time_;
	std::vector<uint160> cycle_; // chrono-time proof
	uint256 nextBlockChallenge_; // proof-of-content challenge for the next validator: block
	int32_t nextTxChallenge_; // proof-of-content challenge for the next validator: tx index
	uint256 prevChallenge_; // proof-of-content resolved challenge (for quick check)
	uint256 proofTx_; // pos proof
	uint512 signature_; // block signature (by origin)

	BlockHeader() {
		setNull();
	}

	template <typename Stream>
	void serialize(Stream& s) const {
		s << version_;
		s << chain_;
		s << prev_;
		s << root_;
		s << origin_;
		s << time_;
		s << cycle_;
		s << nextBlockChallenge_;
		s << nextTxChallenge_;
		s << prevChallenge_;
		s << proofTx_;
		s << signature_;
	}

	template <typename Stream>
	void serializeHash(Stream& s) const {
		s << version_;
		s << chain_;
		s << prev_;
		s << root_;
		s << origin_;
		s << cycle_;
		s << nextBlockChallenge_;
		s << nextTxChallenge_;
		s << prevChallenge_;
		s << proofTx_;
	}

	template <typename Stream>
	void deserialize(Stream& s) {
		s >> version_;
		s >> chain_;
		s >> prev_;
		s >> root_;
		s >> origin_;
		s >> time_;
		s >> cycle_;
		s >> nextBlockChallenge_;
		s >> nextTxChallenge_;
		s >> prevChallenge_;
		s >> proofTx_;
		s >> signature_;
	}

	void setNull() {
		version_ = 0;
		chain_ = MainChain::id();
		prev_.setNull();
		root_.setNull();
		time_ = 0;
		nextTxChallenge_ = -1;
		nextBlockChallenge_.setNull();
		prevChallenge_.setNull();
		signature_.setNull();
		proofTx_.setNull();
	}

	inline bool isNull() const {
		return (time_ == 0);
	}

	uint256 hash();

	inline int32_t version() const { return version_; }

	inline uint64_t time() const { return time_; }
	inline void setTime(uint64_t time) { time_ = time; } 

	inline void setChain(const uint256& chain) { chain_ = chain; }
	inline uint256 chain() { return chain_; }

	inline void setPrev(const uint256& prev) { prev_ = prev; }
	inline uint256 prev() { return prev_; }

	inline void setRoot(const uint256& root) { root_ = root; }
	inline uint256 root() { return root_; }

	inline void setOrigin(const PKey& origin) { origin_ = origin; }
	inline PKey origin() { return origin_; }

	inline void setChallenge(const uint256& nextBlockChallenge, uint32_t nextTxChallenge) {
		nextBlockChallenge_ = nextBlockChallenge;
		nextTxChallenge_ = nextTxChallenge;
	}

	inline uint256 nextBlockChallenge() { return nextBlockChallenge_; }
	inline int32_t nextTxChallenge() { return nextTxChallenge_; }

	inline void setPrevChallenge(const uint256& prevChallenge) { prevChallenge_ = prevChallenge; }
	inline uint256 prevChallenge() { return prevChallenge_; }

	inline void setProofTx(const uint256& proof) { proofTx_ = proof; }
	inline uint256 proofTx() { return proofTx_; }

	inline void setSignature(const uint512& signature) { signature_ = signature; }
	inline uint512 signature() { return signature_; }
};

// broadcast found block
class NetworkBlockHeader {
public:
	NetworkBlockHeader() {}
	NetworkBlockHeader(const BlockHeader &header, uint64_t height) : 
		header_(header), height_(height), confirms_(0) {}

	NetworkBlockHeader(const BlockHeader &header, uint64_t height, uint64_t confirms) : 
		header_(header), height_(height), confirms_(confirms) {}

	BlockHeader& blockHeader() { return header_; }
	uint64_t height() { return height_; }
	uint64_t confirms() { return confirms_; }

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		if (ser_action.ForRead()) {
			header_.deserialize(s); // header info
			s >> height_;
			confirms_ = ReadCompactSize(s);
		} else {
			header_.serialize(s);
			s << height_;
			WriteCompactSize(s, confirms_);
		}
	}

private:
	BlockHeader header_;
	uint64_t height_;
	uint64_t confirms_;
};

// forward
typedef std::vector<TransactionPtr> TransactionsContainer;
typedef std::vector<uint256> HeadersContainer;

class BlockTransactions;
typedef std::shared_ptr<BlockTransactions> BlockTransactionsPtr;

class BlockTransactions {
public:
	// network and disk
	TransactionsContainer transactions_;
	HeadersContainer headers_;

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
	void prepend(TransactionPtr tx) { transactions_.insert(transactions_.begin(), tx); }
	TransactionsContainer& transactions() { return transactions_; }	
	HeadersContainer& headers() { return headers_; }	

	void transactionsHashes(std::list<uint256>&);
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
	void compact();

	void append(BlockTransactionsPtr txs) {
		transactions_.insert(transactions_.end(), txs->transactions().begin(), txs->transactions().end()); 
	}

	void append(TransactionPtr tx) { transactions_.push_back(tx); }
	TransactionsContainer& transactions() { return transactions_; }

	bool equals(BlockPtr other);
};

} // qbit

#endif
