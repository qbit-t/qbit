#include "block.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"
#include "log/log.h"

using namespace qbit;

uint256 BlockHeader::hash() {
	HashWriter lStream(SER_GETHASH, PROTOCOL_VERSION);
	serialize_hash<HashWriter>(lStream);
	return lStream.GetHash();
}

BlockPtr Block::clone() { 
	BlockPtr lBlock = std::make_shared<Block>(*(static_cast<BlockHeader*>(this))); 
	lBlock->transactions().insert(lBlock->transactions().end(), transactions_.begin(), transactions_.end());
	return lBlock;
}

bool Block::equals(BlockPtr other) {
	if (hash() == other->hash() && transactions_.size() == other->transactions_.size()) {
		for (int lIdx = 0; lIdx < transactions_.size(); lIdx++) {
			if (transactions_[lIdx]->hash() != other->transactions_[lIdx]->hash())
				return false;
		}

		return true;
	}

	return false;
}

std::string Block::toString() {
	std::stringstream s;
	s << strprintf("block(hash=%s..., version=0x%08x, prev=%s..., root=%s..., time=%u, bits=%08x, nonce=%u, txs=%u)\n",
		hash().toString().substr(0,10),
		version_,
		prev_.toString().substr(0,10),
		root_.toString().substr(0,10),
		time_, bits_, nonce_,
		transactions_.size());

	for (const auto& lTx : transactions_) {
		s << "  " << lTx->toString() << "\n";
	}
	
	return s.str();
}

void BlockTransactions::transactionsHashes(std::list<uint256>& hashes) {
	//
	for (int lIdx = 0; lIdx < transactions_.size(); lIdx++) {
		//if (gLog().isEnabled(Log::CONSENSUS)) gLog().write(Log::CONSENSUS, std::string("[transactionsHashes] tx = ") + 
		//	strprintf("%s", transactions_[lIdx]->id().toHex()));

		hashes.push_back(transactions_[lIdx]->id()); // hash
	}
}
