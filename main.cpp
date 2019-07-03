//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../allocator.h"

#include <iostream>
#include "key.h"
#include "transaction.h"

#include "libsecp256k1-config.h"
#include "scalar_impl.h"
#include "utilstrencodings.h"

using namespace quark;

int main()
{
	// 0
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));

	Context lContext;
	SKey lKey0(&lContext, lSeed0);

	lKey0.create();
	std::cout << "skey0 = " << lKey0.toString() << std::endl;

	PKey lPKey0 = lKey0.createPKey();
	std::cout << "pkey0 = " << lPKey0.toString() << ", psize = " << lPKey0.getPackedSize() << std::endl;


	// 1.0
	// create transaction
	TransactionPtr lTx = TransactionFactory::create(Transaction::COINBASE);
	lTx->initialize(lPKey0);

	// TODO!
        // 1.1 sign & verify
        std::string lMessage = "this is a first message of mine!";
        uint256 lHash = Hash(lMessage.begin(), lMessage.end());
        std::vector<unsigned char> lSig;
        if (lKey0.sign(lHash, lSig)) {
                std::cout << "Sighned(" << lSig.size() << "): " << HexStr(lSig.begin(), lSig.end()) << std::endl;

                if (lPKey0.verify(lHash, lSig)) {
                        std::cout << "-> Verified" << std::endl;
                }
        }

	// 2.0 serialize
	DataStream lStream(SER_NETWORK, 0);
	TransactionSerializer::serialize<DataStream>(lStream, lTx);

	// stat
	std::cout << "coinbase-tx: " << HexStr(lStream.begin(), lStream.end()) << std::endl;

	char stats[0x1000] = {0};
	_jm_threads_print_stats(stats);
	std::cout << std::endl << stats << std::endl;

	return 0;
}
