#include "blockcontext.h"

using namespace qbit;

uint256 BlockContext::calculateMerkleRoot(std::list<uint256>& hashes) {
	return calcTree(hashes.begin(), hashes.end());
}

uint256 BlockContext::calcTree(std::list<uint256>::iterator current, std::list<uint256>::iterator end) {
	//
	std::list<uint256> lResult;
	while(current != end) {
		HashWriter lNode(SER_GETHASH, PROTOCOL_VERSION);
		lNode << *current; current++;
		if (current != end) { lNode << *current; current++; }

		lResult.push_back(lNode.hash());
	}

	if (lResult.size() == 1) return *lResult.begin();

	return calcTree(lResult.begin(), lResult.end());
}
