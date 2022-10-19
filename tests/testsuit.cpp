
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
#include "../dapps/buzzer/consensus.h"
#include "../dapps/buzzer/txbuzzer.h"
#include "../dapps/buzzer/txbuzz.h"
#include "../dapps/buzzer/txbuzzreply.h"
#include "../dapps/buzzer/txrebuzz.h"
#include "../dapps/buzzer/txrebuzznotify.h"
#include "../dapps/buzzer/txbuzzlike.h"
#include "../dapps/buzzer/txbuzzerendorse.h"
#include "../dapps/buzzer/txbuzzermistrust.h"
#include "../dapps/buzzer/txbuzzersubscribe.h"
#include "../dapps/buzzer/txbuzzerunsubscribe.h"
#include "../dapps/buzzer/transactionstoreextension.h"
#include "../dapps/buzzer/transactionactions.h"
#include "../dapps/buzzer/peerextension.h"

#include "../dapps/cubix/cubix.h"
#include "../dapps/cubix/txmediaheader.h"
#include "../dapps/cubix/txmediadata.h"
#include "../dapps/cubix/txmediasummary.h"
#include "../dapps/cubix/validator.h"
#include "../dapps/cubix/consensus.h"
#include "../dapps/cubix/peerextension.h"

#include "../log/log.h"

#include <iostream>

using namespace qbit;
using namespace qbit::tests;

int main(int argv, char** argc) {
	//
	std::cout << std::endl << "q-bit.technology | unit tests v0.1" << std::endl << std::endl;
	TestSuit lSuit;

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
	Transaction::registerTransactionType(TX_BUZZ_HIDE, TxBuzzHideCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_HIDE, TxBuzzerHideCreator::instance());
	Transaction::registerTransactionType(TX_BUZZ_REPLY, TxBuzzReplyCreator::instance());
	Transaction::registerTransactionType(TX_REBUZZ, TxReBuzzCreator::instance());
	Transaction::registerTransactionType(TX_BUZZ_REBUZZ_NOTIFY, TxReBuzzNotifyCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_ENDORSE, TxBuzzerEndorseCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_MISTRUST, TxBuzzerMistrustCreator::instance());
	Transaction::registerTransactionType(TX_BUZZER_INFO, TxBuzzerInfoCreator::instance());
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_HEADER, cubix::TxMediaHeaderCreator::instance());
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_DATA, cubix::TxMediaDataCreator::instance());
	Transaction::registerTransactionType(TX_CUBIX_MEDIA_SUMMARY, cubix::TxMediaSummaryCreator::instance());
	// tx verification actions
	TransactionProcessor::registerTransactionAction(TxBuzzerTimelockOutsVerify::instance());
	// validators
	ValidatorManager::registerValidator("buzzer", BuzzerValidatorCreator::instance());
	ValidatorManager::registerValidator("cubix", cubix::MiningValidatorCreator::instance());
	// consensuses
	ConsensusManager::registerConsensus("buzzer", BuzzerConsensusCreator::instance());
	ConsensusManager::registerConsensus("cubix", cubix::DefaultConsensusCreator::instance());
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
	Message::registerMessageType(GET_BUZZES, "GET_BUZZES");
	Message::registerMessageType(GET_EVENTS_FEED, "GET_EVENTS_FEED");
	Message::registerMessageType(EVENTS_FEED, "EVENTS_FEED");
	Message::registerMessageType(NEW_EVENT_NOTIFY, "NEW_EVENT_NOTIFY");
	Message::registerMessageType(EVENT_UPDATE_NOTIFY, "EVENT_UPDATE_NOTIFY");	
	Message::registerMessageType(GET_BUZZER_TRUST_SCORE, "GET_BUZZER_TRUST_SCORE");
	Message::registerMessageType(BUZZER_TRUST_SCORE, "BUZZER_TRUST_SCORE"); 
	Message::registerMessageType(BUZZER_TRUST_SCORE_UPDATE, "BUZZER_TRUST_SCORE_UPDATE");
	Message::registerMessageType(GET_BUZZER_MISTRUST_TX, "GET_BUZZER_MISTRUST_TX");
	Message::registerMessageType(BUZZER_MISTRUST_TX, "BUZZER_MISTRUST_TX");
	Message::registerMessageType(GET_BUZZER_ENDORSE_TX, "GET_BUZZER_ENDORSE_TX");
	Message::registerMessageType(BUZZER_ENDORSE_TX, "BUZZER_ENDORSE_TX");
	Message::registerMessageType(GET_BUZZ_FEED_BY_BUZZ, "GET_BUZZ_FEED_BY_BUZZ");
	Message::registerMessageType(BUZZ_FEED_BY_BUZZ, "BUZZ_FEED_BY_BUZZ");
	Message::registerMessageType(GET_BUZZ_FEED_BY_BUZZER, "GET_BUZZ_FEED_BY_BUZZER");
	Message::registerMessageType(BUZZ_FEED_BY_BUZZER, "BUZZ_FEED_BY_BUZZER");
	Message::registerMessageType(GET_MISTRUSTS_FEED_BY_BUZZER, "GET_MISTRUSTS_FEED_BY_BUZZER");
	Message::registerMessageType(MISTRUSTS_FEED_BY_BUZZER, "MISTRUSTS_FEED_BY_BUZZER");
	Message::registerMessageType(GET_ENDORSEMENTS_FEED_BY_BUZZER, "GET_ENDORSEMENTS_FEED_BY_BUZZER");
	Message::registerMessageType(ENDORSEMENTS_FEED_BY_BUZZER, "ENDORSEMENTS_FEED_BY_BUZZER");
	Message::registerMessageType(GET_BUZZER_SUBSCRIPTIONS, "GET_BUZZER_SUBSCRIPTIONS");
	Message::registerMessageType(BUZZER_SUBSCRIPTIONS, "BUZZER_SUBSCRIPTIONS");
	Message::registerMessageType(GET_BUZZER_FOLLOWERS, "GET_BUZZER_FOLLOWERS");
	Message::registerMessageType(BUZZER_FOLLOWERS, "BUZZER_FOLLOWERS");
	Message::registerMessageType(GET_BUZZ_FEED_GLOBAL, "GET_BUZZ_FEED_GLOBAL");
	Message::registerMessageType(BUZZ_FEED_GLOBAL, "BUZZ_FEED_GLOBAL");
	Message::registerMessageType(GET_BUZZ_FEED_BY_TAG, "GET_BUZZ_FEED_BY_TAG");
	Message::registerMessageType(BUZZ_FEED_BY_TAG, "BUZZ_FEED_BY_TAG");
	Message::registerMessageType(GET_HASH_TAGS, "GET_HASH_TAGS");
	Message::registerMessageType(HASH_TAGS, "HASH_TAGS");

	// buzzer peer extention
	PeerManager::registerPeerExtension("buzzer", BuzzerPeerExtensionCreator::instance());
	// cubix
	PeerManager::registerPeerExtension("cubix", cubix::DefaultPeerExtensionCreator::instance());

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
	gLog().enable(Log::WARNING);
	gLog().enable(Log::GENERAL_ERROR);
	gLog().enable(Log::VALIDATOR);
	gLog().enable(Log::SHARDING);
	gLog().enable(Log::NET);
	gLog().enable(Log::BALANCE);
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
		/*
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
		*/
		// lSuit << new DbContainerHash();
		/*
		lSuit << new HashTest();
		lSuit << new WalletQbitCreateSpend();
		lSuit << new WalletQbitCreateSpendRollback();
		lSuit << new WalletAssetCreateAndSpend();
		lSuit << new MemoryPoolQbitCreateSpend();
		*/
		lSuit << new StoreQbitCreateSpend();
		lSuit << new PowTest();
		lSuit << new PowTestOne();
		lSuit << new PowTestTwo();
		//lSuit << new CuckooHash();
	}

	lSuit.execute();

	// memstat
	//char stats[0x1000] = {0};
	//_jm_threads_print_stats(stats);
	//std::cout << std::endl << stats << std::endl;

	return 0;	
}
