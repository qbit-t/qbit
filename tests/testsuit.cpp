
#include "unittest.h"
#include "keys.h"
#include "vm.h"
#include "transactions.h"
#include "blocks.h"
#include "hash.h"
#include "dbs.h"
#include "assets.h"
#include "wallet.h"
#include "memorypool.h"
#include "transactionstore.h"
#include "server.h"
#include "httpserver.h"
#include "pow.h"

#include "../txassettype.h"
#include "../txassetemission.h"
#include "../txdapp.h"
#include "../txshard.h"
#include "../txbase.h"
#include "../txblockbase.h"

#include "../dapps/buzzer/validator.h"
#include "../dapps/buzzer/txbuzzer.h"
#include "../dapps/buzzer/txbuzz.h"
#include "../dapps/buzzer/txbuzzlike.h"
#include "../dapps/buzzer/txbuzzersubscribe.h"
#include "../dapps/buzzer/txbuzzerunsubscribe.h"
#include "../dapps/buzzer/transactionstoreextension.h"
#include "../dapps/buzzer/peerextension.h"

#include "../log/log.h"

#include <iostream>

using namespace qbit;
using namespace qbit::tests;

int main(int argv, char** argc)
{
	std::cout << std::endl << "q-bit.technology | unit tests v0.1" << std::endl << std::endl;
	TestSuit lSuit;

	/*
	//
	std::string text = "猫伸ばしチャレンジ😃 #tag1 @buzzer1";
	std::vector<unsigned char> data; data.insert(data.end(), text.begin(), text.end());
	std::string text1; text1.insert(text1.end(), data.begin(), data.end()); 
	std::cout << text << ' ' << text.size() << "\n";
	std::cout << text1 << ' ' << text1.length() << "\n";

	size_t lSize = 0;
	for (std::string::iterator lS = text1.begin(); lS != text1.end(); lS++, lSize++) {
		if (*lS == '#') std::cout << "tag start found\n";
		else if (*lS == '@') std::cout << "buzzer start found\n";
	}

	for (std::vector<unsigned char>::iterator lS = data.begin(); lS != data.end(); lS++) {
		if (*lS == '#') std::cout << "tag start found\n";
		else if (*lS == '@') std::cout << "buzzer start found\n";
	}

	std::cout << HexStr(data.begin(), data.end()) << ' ' << data.size() << ' ' << lSize << "\n";
	//
	return 0;
	*/

	// pre-launch
	// tx types
	Transaction::registerTransactionType(Transaction::ASSET_TYPE, TxAssetTypeCreator::instance());
	Transaction::registerTransactionType(Transaction::ASSET_EMISSION, TxAssetEmissionCreator::instance());
	Transaction::registerTransactionType(Transaction::DAPP, TxDAppCreator::instance());
	Transaction::registerTransactionType(Transaction::SHARD, TxShardCreator::instance());
	Transaction::registerTransactionType(Transaction::BASE, TxBaseCreator::instance());
	Transaction::registerTransactionType(Transaction::BLOCKBASE, TxBlockBaseCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER, TxBuzzerCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_SUBSCRIBE, TxBuzzerSubscribeCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_UNSUBSCRIBE, TxBuzzerUnsubscribeCreator::instance());
	Transaction::registerTransactionType(TX_BUZZ, TxBuzzCreator::instance());
	Transaction::registerTransactionType(TX_BUZZ_LIKE, TxBuzzLikeCreator::instance());
	// validators
	ValidatorManager::registerValidator("buzzer", BuzzerValidatorCreator::instance());
	// store extensions
	ShardingManager::registerStoreExtension("buzzer", BuzzerTransactionStoreExtensionCreator::instance());	
	// buzzer message types
	Message::registerMessageType(GET_BUZZER_SUBSCRIPTION, "GET_BUZZER_SUBSCRIPTION");
	Message::registerMessageType(BUZZER_SUBSCRIPTION, "BUZZER_SUBSCRIPTION");
	Message::registerMessageType(BUZZER_SUBSCRIPTION_IS_ABSENT, "BUZZER_SUBSCRIPTION_IS_ABSENT");
	Message::registerMessageType(GET_BUZZ_FEED, "GET_BUZZ_FEED");
	Message::registerMessageType(BUZZ_FEED, "BUZZ_FEED");
	Message::registerMessageType(NEW_BUZZ_NOTIFY, "NEW_BUZZ_NOTIFY");
	Message::registerMessageType(BUZZ_UPDATE_NOTIFY, "BUZZ_UPDATE_NOTIFY");
	// buzzer peer extention
	PeerManager::registerPeerExtension("buzzer", BuzzerPeerExtensionCreator::instance());

	// logs
	if (argv > 1 && std::string(argc[1]) == "-S0") gLog("/tmp/.qbitS0/debug.log"); // setup
	else if (argv > 1 && std::string(argc[1]) == "-S1") gLog("/tmp/.qbitS1/debug.log"); // setup
	else if (argv > 1 && std::string(argc[1]) == "-S2") gLog("/tmp/.qbitS2/debug.log"); // setup
	else if (argv > 1 && std::string(argc[1]) == "-S3") gLog("/tmp/.qbitS3/debug.log"); // setup

	gLog().enable(Log::INFO);
	gLog().enable(Log::POOL);
	gLog().enable(Log::CONSENSUS);
	gLog().enable(Log::STORE);
	gLog().enable(Log::WALLET);
	gLog().enable(Log::HTTP);
	gLog().enable(Log::ERROR);
	gLog().enable(Log::VALIDATOR);
	gLog().enable(Log::SHARDING);
	gLog().enable(Log::NET);
	//gLog().enable(Log::BALANCE);
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
		lSuit << new HashTest();
		lSuit << new WalletQbitCreateSpend();
		lSuit << new WalletQbitCreateSpendRollback();
		lSuit << new WalletAssetCreateAndSpend();
		lSuit << new MemoryPoolQbitCreateSpend();
		lSuit << new StoreQbitCreateSpend();
		lSuit << new PowTest();
		//lSuit << new CuckooHash();
	}

	lSuit.execute();

	// memstat
	//char stats[0x1000] = {0};
	//_jm_threads_print_stats(stats);
	//std::cout << std::endl << stats << std::endl;

	return 0;	
}
