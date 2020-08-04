// Copyright (c) 2020 Andrew Demuskov (demuskov@gmail.com)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"

#include <iostream>
#include <boost/algorithm/string.hpp>

#include "key.h"
#include "transaction.h"
#include "lightwallet.h"
#include "requestprocessor.h"
#include "iconsensusmanager.h"
#include "ipeermanager.h"
#include "ipeer.h"
#include "peer.h"
#include "peermanager.h"

#include "libsecp256k1-config.h"
#include "scalar_impl.h"
#include "utilstrencodings.h"

#include "itransactionstore.h"
#include "iwallet.h"
#include "ientitystore.h"
#include "transactionvalidator.h"
#include "transactionactions.h"
#include "block.h"
#include "iconsensus.h"
#include "isettings.h"
#include "wallet.h"
#include "transactionstore.h"

#include "memorypoolmanager.h"
#include "transactionstoremanager.h"
#include "peermanager.h"
#include "consensusmanager.h"
#include "validatormanager.h"
#include "shardingmanager.h"

#include "httpserver.h"
#include "httpendpoints.h"

#include "server.h"

#include "txbase.h"
#include "txblockbase.h"
#include "txassettype.h"
#include "txassetemission.h"
#include "txdapp.h"
#include "txshard.h"
#include "txbase.h"
#include "txblockbase.h"

#if defined (BUZZER_MOD)
	#include "dapps/buzzer/validator.h"
	#include "dapps/buzzer/consensus.h"
	#include "dapps/buzzer/txbuzzer.h"
	#include "dapps/buzzer/txbuzz.h"
	#include "dapps/buzzer/txbuzzreply.h"
	#include "dapps/buzzer/txrebuzz.h"
	#include "dapps/buzzer/txrebuzznotify.h"
	#include "dapps/buzzer/txbuzzlike.h"
	#include "dapps/buzzer/txbuzzreward.h"
	#include "dapps/buzzer/txbuzzerendorse.h"
	#include "dapps/buzzer/txbuzzermistrust.h"
	#include "dapps/buzzer/txbuzzersubscribe.h"
	#include "dapps/buzzer/txbuzzerunsubscribe.h"
	#include "dapps/buzzer/transactionstoreextension.h"
	#include "dapps/buzzer/transactionactions.h"
	#include "dapps/buzzer/peerextension.h"
	#include "dapps/buzzer/httpendpoints.h"
	#include "dapps/buzzer/composer.h"
#endif

#if defined (CUBIX_MOD)
	#include "dapps/cubix/cubix.h"
	#include "dapps/cubix/txmediaheader.h"
	#include "dapps/cubix/txmediadata.h"
	#include "dapps/cubix/txmediasummary.h"
	#include "dapps/cubix/validator.h"
	#include "dapps/cubix/consensus.h"
	#include "dapps/cubix/peerextension.h"
#endif

#include <boost/lexical_cast.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>

namespace qbit {

class NodeSettings: public ISettings {
public:
	NodeSettings(): NodeSettings(".qbit", nullptr) {}
	NodeSettings(const std::string& dir, ISettingsPtr other) {
#if defined(__linux__)
		char lName[0x100] = {0};
		getlogin_r(lName, sizeof(lName));
		path_ = "/home/" + std::string(lName) + "/" + dir;
#endif
		if (other) {
			serverPort_ = other->serverPort();
			threadPoolSize_ = other->threadPoolSize();
			roles_ = other->roles();
			httpServerPort_ = other->httpServerPort();
			supportAirdrop_ = other->supportAirdrop();
		}
	}

	std::string dataPath() { return path_; }

	int serverPort() { return serverPort_; }
	void setServerPort(int port) { serverPort_ = port; }

	size_t threadPoolSize() { return threadPoolSize_; }
	void setThreadPoolSize(size_t pool) { threadPoolSize_ = pool; }

	uint32_t roles() { return roles_; }
	void addMinerRole() { roles_ |= State::PeerRoles::MINER; }
	void addNodeRole() { roles_ |= State::PeerRoles::NODE; }
	void addFullNodeRole() { roles_ |= State::PeerRoles::FULLNODE; }

	int httpServerPort() { return httpServerPort_; }
	void setHttpServerPort(int port) { httpServerPort_ = port; }

	bool supportAirdrop() { return supportAirdrop_; }
	void setSupportAirdrop() { supportAirdrop_ = true; }

	static ISettingsPtr instance() { return std::make_shared<NodeSettings>(); }
	static ISettingsPtr instance(const std::string& dir, ISettingsPtr other) { return std::make_shared<NodeSettings>(dir, other); }

private:
	std::string path_;
	uint32_t roles_ = State::PeerRoles::UNDEFINED;
	int serverPort_ = 31415;
	size_t threadPoolSize_ = 2;
	int httpServerPort_ = 8080;
	bool supportAirdrop_ = false;
};

class Node;
typedef std::shared_ptr<Node> NodePtr;

class Node {
public:
	Node(ISettingsPtr settings) {
		settings_ = settings;

		// pre-launch
		// tx types
		Transaction::registerTransactionType(Transaction::ASSET_TYPE, TxAssetTypeCreator::instance());
		Transaction::registerTransactionType(Transaction::ASSET_EMISSION, TxAssetEmissionCreator::instance());
		Transaction::registerTransactionType(Transaction::DAPP, TxDAppCreator::instance());
		Transaction::registerTransactionType(Transaction::SHARD, TxShardCreator::instance());
		Transaction::registerTransactionType(Transaction::BASE, TxBaseCreator::instance());
		Transaction::registerTransactionType(Transaction::BLOCKBASE, TxBlockBaseCreator::instance());

#if defined (BUZZER_MOD)
		Transaction::registerTransactionType(TX_BUZZER, TxBuzzerCreator::instance());
		Transaction::registerTransactionType(TX_BUZZER_SUBSCRIBE, TxBuzzerSubscribeCreator::instance());
		Transaction::registerTransactionType(TX_BUZZER_UNSUBSCRIBE, TxBuzzerUnsubscribeCreator::instance());
		Transaction::registerTransactionType(TX_BUZZ, TxBuzzCreator::instance());
		Transaction::registerTransactionType(TX_BUZZ_LIKE, TxBuzzLikeCreator::instance());
		Transaction::registerTransactionType(TX_BUZZ_REPLY, TxBuzzReplyCreator::instance());
		Transaction::registerTransactionType(TX_REBUZZ, TxReBuzzCreator::instance());
		Transaction::registerTransactionType(TX_BUZZ_REBUZZ_NOTIFY, TxReBuzzNotifyCreator::instance());
		Transaction::registerTransactionType(TX_BUZZER_ENDORSE, TxBuzzerEndorseCreator::instance());
		Transaction::registerTransactionType(TX_BUZZER_MISTRUST, TxBuzzerMistrustCreator::instance());
		Transaction::registerTransactionType(TX_BUZZER_INFO, TxBuzzerInfoCreator::instance());
		Transaction::registerTransactionType(TX_BUZZ_REWARD, TxBuzzRewardCreator::instance());
#endif
#if defined (CUBIX_MOD)		
		Transaction::registerTransactionType(TX_CUBIX_MEDIA_HEADER, cubix::TxMediaHeaderCreator::instance());
		Transaction::registerTransactionType(TX_CUBIX_MEDIA_DATA, cubix::TxMediaDataCreator::instance());
		Transaction::registerTransactionType(TX_CUBIX_MEDIA_SUMMARY, cubix::TxMediaSummaryCreator::instance());
#endif

		// tx verification actions
		TransactionProcessor::registerTransactionAction(TxBuzzerTimelockOutsVerify::instance());

		// validators
#if defined (BUZZER_MOD)		
		ValidatorManager::registerValidator("buzzer", BuzzerValidatorCreator::instance());
#endif

#if defined (CUBIX_MOD)		
		ValidatorManager::registerValidator("cubix", cubix::MiningValidatorCreator::instance());
#endif

		// consensuses
#if defined (BUZZER_MOD)		
		ConsensusManager::registerConsensus("buzzer", BuzzerConsensusCreator::instance());
#endif
#if defined (CUBIX_MOD)		
		ConsensusManager::registerConsensus("cubix", cubix::DefaultConsensusCreator::instance());
#endif

		// store extensions
#if defined (BUZZER_MOD)		
		ShardingManager::registerStoreExtension("buzzer", BuzzerTransactionStoreExtensionCreator::instance());	
#endif

		// buzzer message types
#if defined (BUZZER_MOD)		
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
#endif

		// buzzer peer extention
#if defined (BUZZER_MOD)		
		PeerManager::registerPeerExtension("buzzer", BuzzerPeerExtensionCreator::instance());
#endif
		// cubix
#if defined (CUBIX_MOD)		
		PeerManager::registerPeerExtension("cubix", cubix::DefaultPeerExtensionCreator::instance());
#endif
		
		// main working configuration
		wallet_ = Wallet::instance(settings_);
		consensusManager_ = ConsensusManager::instance(settings_);
		mempoolManager_ = MemoryPoolManager::instance(settings_);
		storeManager_ = TransactionStoreManager::instance(settings_);
		peerManager_ = PeerManager::instance(settings_, consensusManager_, mempoolManager_);		
		validatorManager_ = ValidatorManager::instance(settings_, consensusManager_, mempoolManager_, storeManager_);
		shardingManager_ = ShardingManager::instance(settings_, consensusManager_, 
											storeManager_, validatorManager_,
											mempoolManager_);
		// store
		std::static_pointer_cast<TransactionStoreManager>(storeManager_)->setWallet(wallet_);
		std::static_pointer_cast<TransactionStoreManager>(storeManager_)->setShardingManager(shardingManager_);
		// push main chain to store manager
		storeManager_->push(MainChain::id());

		// consensus
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setStoreManager(storeManager_);
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setValidatorManager(validatorManager_);
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setWallet(wallet_);
		std::static_pointer_cast<ConsensusManager>(consensusManager_)->setPeerManager(peerManager_);
		// mempool
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setStoreManager(storeManager_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setConsensusManager(consensusManager_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setWallet(wallet_);
		std::static_pointer_cast<MemoryPoolManager>(mempoolManager_)->setEntityStore(storeManager_->locate(MainChain::id())->entityStore());
		// wallet
		std::static_pointer_cast<Wallet>(wallet_)->setStoreManager(storeManager_);
		std::static_pointer_cast<Wallet>(wallet_)->setMemoryPoolManager(mempoolManager_);
		std::static_pointer_cast<Wallet>(wallet_)->setEntityStore(storeManager_->locate(MainChain::id())->entityStore());

		// push main chain
		consensusManager_->push(MainChain::id(), nullptr);
		mempoolManager_->push(MainChain::id());
		validatorManager_->push(MainChain::id(), nullptr);

		// buzzer
		buzzerComposer_ = BuzzerComposer::instance(settings_, wallet_);

		//
		server_ = Server::instance(settings_, peerManager_);

		//
		httpRequestHandler_ = HttpRequestHandler::instance(settings_, wallet_, peerManager_);
		httpRequestHandler_->push(HttpMallocStats::instance());
		httpRequestHandler_->push(HttpGetPeerInfo::instance());
		httpRequestHandler_->push(HttpGetKey::instance());
		httpRequestHandler_->push(HttpGetBalance::instance());
		httpRequestHandler_->push(HttpSendToAddress::instance());
		httpRequestHandler_->push(HttpCreateDApp::instance());
		httpRequestHandler_->push(HttpCreateShard::instance());
		httpRequestHandler_->push(HttpGetTransaction::instance());
		httpRequestHandler_->push(HttpCreateBuzzer::instance(buzzerComposer_)); // custom
		httpRequestHandler_->push(HttpAttachBuzzer::instance(buzzerComposer_)); // custom
		httpRequestHandler_->push(HttpBuzz::instance(buzzerComposer_)); // custom
		httpRequestHandler_->push(HttpBuzzerSubscribe::instance(buzzerComposer_)); // custom
		httpRequestHandler_->push(HttpBuzzerUnsubscribe::instance(buzzerComposer_)); // custom
		httpConnectionManager_ = HttpConnectionManager::instance(settings_, httpRequestHandler_);
		httpServer_ = HttpServer::instance(settings_, httpConnectionManager_);		
	}

	static NodePtr instance(ISettingsPtr settings) { return std::make_shared<Node>(settings); }

	ITransactionStoreManagerPtr storeManager() { return storeManager_; }
	IWalletPtr wallet() { return wallet_; }
	IPeerManagerPtr peerManager() { return peerManager_; }
	IValidatorManagerPtr validatorManager() { return validatorManager_; } 
	IShardingManagerPtr shardingManager() { return shardingManager_; }
	HttpServerPtr httpServer() { return httpServer_; }
	ServerPtr server() { return server_; }

private:
	BuzzerComposerPtr buzzerComposer_;
	IShardingManagerPtr shardingManager_;
	ITransactionStoreManagerPtr storeManager_;
	IMemoryPoolManagerPtr mempoolManager_;
	IPeerManagerPtr peerManager_;
	IConsensusManagerPtr consensusManager_;
	IValidatorManagerPtr validatorManager_;
	ServerPtr server_;

	HttpRequestHandlerPtr httpRequestHandler_;
	HttpConnectionManagerPtr httpConnectionManager_;
	HttpServerPtr httpServer_;

	IWalletPtr wallet_;
	ISettingsPtr settings_;
};

}

using namespace qbit;

int main(int argv, char** argc) {
	//
	uint32_t _QBIT_VERSION = 
			((((QBIT_VERSION_MAJOR << 16) + QBIT_VERSION_MINOR) << 8) + QBIT_VERSION_REVISION);

	std::cout << std::endl << "qbitd (" << 
		QBIT_VERSION_MAJOR << "." <<
		QBIT_VERSION_MINOR << "." <<
		QBIT_VERSION_REVISION << "." <<
		QBIT_VERSION_BUILD << ") | (c) 2020 q-bit.technology | MIT license" << std::endl;

	// home
	ISettingsPtr lSettings = NodeSettings::instance();
	bool lIsLogConfigured = false;

	// command line
	bool lDebugFound = false;
	std::vector<std::string> lPeers;
	for (int lIdx = 1; lIdx < argv; lIdx++) {
		//
		if (std::string(argc[lIdx]) == std::string("-debug")) {
			std::vector<std::string> lCategories; 
			boost::split(lCategories, std::string(argc[++lIdx]), boost::is_any_of(","));

			if (!lIsLogConfigured) { 
				gLog(lSettings->dataPath() + "/debug.log"); // setup 
				lIsLogConfigured = true;
			}

			for (std::vector<std::string>::iterator lCategory = lCategories.begin(); lCategory != lCategories.end(); lCategory++) {
				gLog().enable(getLogCategory(*lCategory));				
			}

			lDebugFound = true;
		} else if (std::string(argc[lIdx]) == std::string("-peers")) {
			//
			boost::split(lPeers, std::string(argc[++lIdx]), boost::is_any_of(","));
		} else if (std::string(argc[lIdx]) == std::string("-home")) {
			//
			std::string lHome = std::string(argc[++lIdx]);
			lSettings = NodeSettings::instance(lHome, lSettings); // re-create
			gLog(lSettings->dataPath() + "/debug.log"); // setup
			lIsLogConfigured = true;
		} else if (std::string(argc[lIdx]) == std::string("-port")) {
			//
			int lPort;
			if (boost::conversion::try_lexical_convert<int>(std::string(argc[++lIdx]), lPort)) {
				lSettings->setServerPort(lPort);
			} else {
				std::cout << "port: incorrect value" << std::endl;
				return -1;
			}
		} else if (std::string(argc[lIdx]) == std::string("-threadpool")) {
			//
			size_t lPool;
			if (boost::conversion::try_lexical_convert<size_t>(std::string(argc[++lIdx]), lPool)) {
				lSettings->setThreadPoolSize(lPool);
			} else {
				std::cout << "threadpool: incorrect value" << std::endl;
				return -1;
			}
		} else if (std::string(argc[lIdx]) == std::string("-http")) {
			//
			int lPort;
			if (boost::conversion::try_lexical_convert<int>(std::string(argc[++lIdx]), lPort)) {
				lSettings->setHttpServerPort(lPort);
			} else {
				std::cout << "http: incorrect value" << std::endl;
				return -1;
			}
		} else if (std::string(argc[lIdx]) == std::string("-console")) {
			//
			gLog().enableConsole();
		} else if (std::string(argc[lIdx]) == std::string("-airdrop")) {
			//
			lSettings->setSupportAirdrop();
		} else if (std::string(argc[lIdx]) == std::string("-roles")) {
			//
			std::vector<std::string> lRoles;
			boost::split(lRoles, std::string(argc[++lIdx]), boost::is_any_of(","));

			for (std::vector<std::string>::iterator lRole = lRoles.begin(); lRole != lRoles.end(); lRole++) {
				//
				if (*lRole == "node") {
					lSettings->addNodeRole();
				} else if (*lRole == "fullnode") {
					lSettings->addFullNodeRole();
				} else if (*lRole == "miner") {
					lSettings->addMinerRole();
				}
			}
		}
	}

	if (!lDebugFound) {
		gLog().enable(Log::INFO);
		gLog().enable(Log::WARNING);
		gLog().enable(Log::ERROR);
	}

	if (!lSettings->roles()) lSettings->addNodeRole();

	// create main emtry
	NodePtr lNode = Node::instance(lSettings);

	// prepare main store
	if (!lNode->storeManager()->locate(MainChain::id())->open()) {
		std::cout << "node: storage open failed." << std::endl;
		return -1;
	}

	// prepare wallet
	if (!lNode->wallet()->open("")) {
		std::cout << "node: wallet open failed." << std::endl;
		return -1;
	}

	// initialize first key (if absent - create it)
	lNode->wallet()->firstKey();

	// NOTE: add static nodes here
	// lNode->peerManager()->addPeerExplicit("8.8.8.8:31415");

	// add nodes
	for (std::vector<std::string>::iterator lPeer = lPeers.begin(); lPeer != lPeers.end(); lPeer++) {
		//
		lNode->peerManager()->addPeerExplicit(*lPeer);
	}

	// start sequence
	lNode->validatorManager()->run();
	lNode->shardingManager()->run();
	lNode->httpServer()->run();
	lNode->server()->run();

	return 0;
}
