#include "transaction.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"

qbit::TransactionTypes qbit::gTxTypes;

uint256 qbit::Transaction::hash() {
	CHashWriter lStream(SER_GETHASH, PROTOCOL_VERSION);
	TransactionSerializer::serialize<CHashWriter>(lStream, this);
    return (id_ = lStream.GetHash());	
}

std::string qbit::Transaction::Link::toString() const
{
    return strprintf("link(asset=%s..., hash=%s..., index=%u)", asset_.toString().substr(0,10), hash_.toString().substr(0,10), index_);
}

std::string qbit::Transaction::In::toString() const
{
	std::string str;
	str += "in(";
	str += out_.toString();
	str += strprintf(", owner=%s...)", HexStr(ownership_).substr(0, 24));
	return str;
}

std::string qbit::Transaction::Out::toString() const
{
    return strprintf("out(asset=%s..., amount=%u, destination=%s...)", asset_.toString().substr(0,10), 
    	amount_, HexStr(destination_).substr(0, 30));
}

std::string qbit::Transaction::toString() 
{
	std::string str;
	str += strprintf("transaction(hash=%s..., ins=%u, outs=%u)\n",
		hash().toString().substr(0,10),
		in_.size(),
		out_.size());
	
	for (const auto& tx_in : in_)
		str += "    " + tx_in.toString() + "\n";
	for (const auto& tx_out : out_)
		str += "    " + tx_out.toString() + "\n";
	return str;
}
