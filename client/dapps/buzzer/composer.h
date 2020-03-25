// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_LIGHT_COMPOSER_H
#define QBIT_BUZZER_LIGHT_COMPOSER_H

#include "../../handlers.h"
#include "../../../dapps/buzzer/txbuzzer.h"
#include "../../../dapps/buzzer/txbuzz.h"
#include "../../../dapps/buzzer/txbuzzersubscribe.h"
#include "../../../dapps/buzzer/txbuzzerunsubscribe.h"
#include "../../../txshard.h"
#include "../../../db/containers.h"
#include "requestprocessor.h"

namespace qbit {

// forward
class BuzzerLightComposer;
typedef std::shared_ptr<BuzzerLightComposer> BuzzerLightComposerPtr;

//
class BuzzerLightComposer: public std::enable_shared_from_this<BuzzerLightComposer> {
public:
	class CreateTxBuzzer: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzer> {
	public:
		CreateTxBuzzer(BuzzerLightComposerPtr composer, const std::string& buzzer, const std::string& alias, const std::string& description, transactionCreatedFunction created): 
			composer_(composer), buzzer_(buzzer), alias_(alias), description_(description), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& buzzer, const std::string& alias, const std::string& description, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzer>(composer, buzzer, alias, description, created); 
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during creating buzzer.");
		}

		//
		void buzzerEntityLoaded(EntityPtr);
		void utxoByDAppLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void dAppInstancesCountByShardsLoaded(const std::map<uint32_t, uint256>&, const std::string&);
		void shardLoaded(TransactionPtr);
		void utxoByShardLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

	private:
		BuzzerLightComposerPtr composer_;
		std::string buzzer_;
		std::string alias_;
		std::string description_;		
		transactionCreatedFunction created_;
		errorFunction error_;

		TxBuzzerPtr buzzerTx_;
		TransactionContextPtr ctx_;
		TxShardPtr shardTx_;
	};

	class CreateTxBuzz: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzz> {
	public:
		CreateTxBuzz(BuzzerLightComposerPtr composer, const std::string& body, transactionCreatedFunction created): composer_(composer), body_(body), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& body, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzz>(composer, body, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzz creation.");
		}

		//
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

	private:
		BuzzerLightComposerPtr composer_;
		std::string body_;
		transactionCreatedFunction created_;
		errorFunction error_;		

		TxBuzzPtr buzzTx_;
		TransactionContextPtr ctx_;
	};

	class CreateTxBuzzerSubscribe: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzerSubscribe> {
	public:
		CreateTxBuzzerSubscribe(BuzzerLightComposerPtr composer, const std::string& publisher, transactionCreatedFunction created): composer_(composer), publisher_(publisher), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& publisher, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzerSubscribe>(composer, publisher, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzer subscription.");
		}

		//
		void utxoByPublisherLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

	private:
		BuzzerLightComposerPtr composer_;
		std::string publisher_;
		transactionCreatedFunction created_;
		errorFunction error_;		

		TxBuzzerSubscribePtr buzzerSubscribeTx_;
		TransactionContextPtr ctx_;
	};	

	class CreateTxBuzzerUnsubscribe: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzerUnsubscribe> {
	public:
		CreateTxBuzzerUnsubscribe(BuzzerLightComposerPtr composer, const std::string& publisher, transactionCreatedFunction created): composer_(composer), publisher_(publisher), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& publisher, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzerUnsubscribe>(composer, publisher, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzer subscription.");
		}

		//
		void publisherLoaded(TransactionPtr);
		void subscriptionLoaded(TransactionPtr);
		void utxoBySubscriptionLoaded(const std::vector<Transaction::NetworkUnlinkedOut>&, const uint256&);

	private:
		BuzzerLightComposerPtr composer_;
		std::string publisher_;
		transactionCreatedFunction created_;
		errorFunction error_;		

		TxBuzzerUnsubscribePtr buzzerUnsubscribeTx_;
		TransactionContextPtr ctx_;
		uint256 shardTx_;
	};	

public:
	BuzzerLightComposer(ISettingsPtr settings, IWalletPtr wallet, IRequestProcessorPtr requestProcessor, BuzzerRequestProcessorPtr buzzerRequestProcessor): 
		settings_(settings), wallet_(wallet), requestProcessor_(requestProcessor), buzzerRequestProcessor_(buzzerRequestProcessor),
		workingSettings_(settings->dataPath() + "/wallet/buzzer/settings") {
		opened_ = false;
	}

	static BuzzerLightComposerPtr instance(ISettingsPtr settings, IWalletPtr wallet, IRequestProcessorPtr requestProcessor, BuzzerRequestProcessorPtr buzzerRequestProcessor) {
		return std::make_shared<BuzzerLightComposer>(settings, wallet, requestProcessor, buzzerRequestProcessor); 
	}

	// composer management
	bool open();
	bool close();
	bool isOpened() { return opened_; }

	std::string dAppName() { return "buzzer"; }
	IWalletPtr wallet() { return wallet_; }
	ISettingsPtr settings() { return settings_; }
	IRequestProcessorPtr requestProcessor() { return requestProcessor_; }
	BuzzerRequestProcessorPtr buzzerRequestProcessor() { return buzzerRequestProcessor_; }

	bool writeBuzzerTx(TxBuzzerPtr buzzerTx) {
		//
		buzzerTx_ = buzzerTx;
		// load buzzer tx
		if (!open()) return false;

		// save our buzzer
		DataStream lStream(SER_NETWORK, CLIENT_VERSION);
		Transaction::Serializer::serialize<DataStream>(lStream, buzzerTx_);
		std::string lBuzzerTxHex = HexStr(lStream.begin(), lStream.end());
		workingSettings_.write("buzzerTx", lBuzzerTxHex);
		return true;
	}

	bool writeBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>& utxo) {
		//
		buzzerUtxo_ = utxo;
		// load buzzer tx
		if (!open()) return false;

		// save our buzzer
		DataStream lStream(SER_NETWORK, CLIENT_VERSION);
		lStream << buzzerUtxo_;
		std::string lBuzzerUtxoHex = HexStr(lStream.begin(), lStream.end());
		workingSettings_.write("buzzerUtxo", lBuzzerUtxoHex);
		return true;
	}

	TxBuzzerPtr buzzerTx() { open(); return buzzerTx_; }
	std::vector<Transaction::UnlinkedOut>& buzzerUtxo() { open(); return buzzerUtxo_; }

private:
	// various settings, command line args & config file
	ISettingsPtr settings_;
	// wallet
	IWalletPtr wallet_;
	// buzzer tx
	TxBuzzerPtr buzzerTx_;
	// utxos for buzzer tx
	std::vector<Transaction::UnlinkedOut> buzzerUtxo_;
	// request processor
	IRequestProcessorPtr requestProcessor_;
	// special processor
	BuzzerRequestProcessorPtr buzzerRequestProcessor_;

	// persistent settings
	db::DbContainer<std::string /*name*/, std::string /*data*/> workingSettings_;	

	// flag
	bool opened_;
};

} // qbit

#endif