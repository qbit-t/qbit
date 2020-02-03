
#include "unittest.h"
#include "keys.h"
#include "vm.h"
#include "transactions.h"
#include "blocks.h"
#include "dbs.h"
#include "assets.h"
#include "wallet.h"
#include "memorypool.h"
#include "transactionstore.h"
#include "server.h"
#include "httpserver.h"

#include "../log/log.h"

#include <iostream>

using namespace qbit;
using namespace qbit::tests;

int main(int argv, char** argc)
{
	std::cout << std::endl << "q-bit.technology | unit tests v0.1" << std::endl << std::endl;
	TestSuit lSuit;

	//std::cout << qbit::getTime() << "\n";
	//std::cout << qbit::getMicroseconds() << "\n";

	gLog().enable(Log::INFO);
	gLog().enable(Log::POOL);
	gLog().enable(Log::CONSENSUS);
	gLog().enable(Log::STORE);
	gLog().enable(Log::WALLET);
	gLog().enable(Log::HTTP);
	gLog().enable(Log::ERROR);
	gLog().enable(Log::VALIDATOR);
	//gLog().enable(Log::NET);
	//gLog().enable(Log::ALL);

	if (argv > 1 && std::string(argc[1]) == "-S0") {
		gLog().enableConsole();
		lSuit << new ServerS0();
	}
	else if (argv > 1 && std::string(argc[1]) == "-S1") {
		gLog().enableConsole();
		lSuit << new ServerS1();
	}
	else if (argv > 1 && std::string(argc[1]) == "-S2") {
		gLog().enableConsole();
		lSuit << new ServerS2();
	}
	else if (argv > 1 && std::string(argc[1]) == "-S3") {
		gLog().enableConsole();
		lSuit << new ServerS3();
	}
	else if (argv > 1 && std::string(argc[1]) == "-http") {
		gLog().enableConsole();
		lSuit << new ServerHttp();
	} else {
		lSuit << new RandomTest();
		lSuit << new CreateKeySignAndVerifyMix();
		lSuit << new VMMovCmp();
		lSuit << new VMMovCmpJmpFalse();
		lSuit << new VMMovCmpJmpTrue();
		lSuit << new VMLoop();
		lSuit << new VMCheckLHash256();
		lSuit << new VMCheckSig();
		lSuit << new TxVerify();
		lSuit << new TxVerifyPrivate();
		lSuit << new TxVerifyFee();
		lSuit << new TxVerifyPrivateFee();
		lSuit << new TxAssetVerify();
		lSuit << new TxAssetEmissionVerify();
		lSuit << new TxAssetEmissionSpend();
		lSuit << new BlockCreate();
		lSuit << new DbContainerCreate();
		lSuit << new DbEntityContainerCreate();
		lSuit << new DbContainerIterator();
		lSuit << new DbMultiContainerIterator();
		lSuit << new DbContainerTransaction();
		lSuit << new DbEntityContainerTransaction();
		lSuit << new DbMultiContainerTransaction();
		lSuit << new DbMultiContainerRemove();
		lSuit << new DbEntityContainerRemove();
		lSuit << new DbContainerRemove();
		lSuit << new WalletQbitCreateSpend();
		lSuit << new WalletQbitCreateSpendRollback();
		lSuit << new WalletAssetCreateAndSpend();
		lSuit << new MemoryPoolQbitCreateSpend();
		lSuit << new StoreQbitCreateSpend();
		//lSuit << new CuckooHash();
	}

	lSuit.execute();

	// memstat
	//char stats[0x1000] = {0};
	//_jm_threads_print_stats(stats);
	//std::cout << std::endl << stats << std::endl;

	return 0;	
}
