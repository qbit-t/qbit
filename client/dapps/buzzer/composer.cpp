#include "composer.h"
#include "../../../dapps/buzzer/txbuzzer.h"
#include "../../../dapps/buzzer/txbuzz.h"
#include "../../../dapps/buzzer/txbuzzersubscribe.h"
#include "../../../dapps/buzzer/txbuzzerunsubscribe.h"

#include "../../../txdapp.h"
#include "../../../txshard.h"
#include "../../../mkpath.h"
#include "../../../timestamp.h"

using namespace qbit;

bool BuzzerLightComposer::open() {
	if (!opened_) {
		try {
			if (mkpath(std::string(settings_->dataPath() + "/wallet/buzzer").c_str(), 0777)) return false;

			gLog().write(Log::INFO, std::string("[buzzer/wallet/open]: opening buzzer wallet data..."));
			workingSettings_.open();

			std::string lBuzzerTx;
			if (workingSettings_.read("buzzerTx", lBuzzerTx)) {
				std::vector<unsigned char> lBuzzerTxHex = ParseHex(lBuzzerTx);

				DataStream lStream(lBuzzerTxHex, SER_NETWORK, CLIENT_VERSION);
				TransactionPtr lBuzzer = Transaction::Deserializer::deserialize<DataStream>(lStream);
				buzzerTx_ = TransactionHelper::to<TxBuzzer>(lBuzzer);
			}

			std::string lBuzzerUtxo;
			if (workingSettings_.read("buzzerUtxo", lBuzzerUtxo)) {
				std::vector<unsigned char> lBuzzerUtxoHex = ParseHex(lBuzzerUtxo);

				DataStream lStream(lBuzzerUtxoHex, SER_NETWORK, CLIENT_VERSION);
				lStream >> buzzerUtxo_;
			}

			opened_ = true;
		}
		catch(const std::exception& ex) {
			gLog().write(Log::ERROR, std::string("[buzzer/wallet/open/error]: ") + ex.what());
			return false;
		}
	}

	return true;
}

bool BuzzerLightComposer::close() {
	gLog().write(Log::INFO, std::string("[buzzer/wallet/close]: closing buzzer wallet data..."));

	settings_.reset();
	wallet_.reset();

	workingSettings_.close();

	opened_ = false;

	return true;
}

//
// CreateTxBuzzer
//
void BuzzerLightComposer::CreateTxBuzzer::process(errorFunction error) {
	//
	error_ = error;

	// check buzzerName
	std::string lBuzzerName;
	if (*buzzer_.begin() != '@') lBuzzerName += '@';
	lBuzzerName += buzzer_; 

	composer_->requestProcessor()->loadEntity(lBuzzerName, 
		LoadEntity::instance(
			boost::bind(&BuzzerLightComposer::CreateTxBuzzer::buzzerEntityLoaded, shared_from_this(), _1),
			boost::bind(&BuzzerLightComposer::CreateTxBuzzer::timeout, shared_from_this()))
	);
}

void BuzzerLightComposer::CreateTxBuzzer::buzzerEntityLoaded(EntityPtr buzzer) {
	//
	if (buzzer) {
		error_("E_BUZZER_EXISTS", "Buzzer name already taken.");
		return;
	}

	// check buzzerName
	std::string lBuzzerName;
	if (*buzzer_.begin() != '@') lBuzzerName += '@';
	lBuzzerName += buzzer_;	

	// create empty tx
	buzzerTx_ = TransactionHelper::to<TxBuzzer>(TransactionFactory::create(TX_BUZZER));
	// create context
	ctx_ = TransactionContext::instance(buzzerTx_);
	//
	buzzerTx_->setMyName(lBuzzerName);
	buzzerTx_->setAlias(alias_);
	buzzerTx_->setDescription(description_);

	SKey lSChangeKey = composer_->wallet()->changeKey();
	SKey lSKey = composer_->wallet()->firstKey();
	if (!lSKey.valid() || !lSChangeKey.valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	SKey lFirstKey = composer_->wallet()->firstKey();
	PKey lSelf = lSKey.createPKey();
	// make buzzer out (for buzz creation)
	buzzerTx_->addBuzzerOut(lSKey, lSelf);
	// for subscriptions
	buzzerTx_->addBuzzerSubscriptionOut(lSKey, lSelf);
	// endorse
	buzzerTx_->addBuzzerEndorseOut(lSKey, lSelf);
	// mistrust
	buzzerTx_->addBuzzerMistrustOut(lSKey, lSelf);

	composer_->requestProcessor()->selectUtxoByEntity(composer_->dAppName(), 
		SelectUtxoByEntityName::instance(
			boost::bind(&BuzzerLightComposer::CreateTxBuzzer::utxoByDAppLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::CreateTxBuzzer::timeout, shared_from_this()))
	);
}

void BuzzerLightComposer::CreateTxBuzzer::utxoByDAppLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& dapp) {
	//
	SKey lSChangeKey = composer_->wallet()->changeKey();
	SKey lSKey = composer_->wallet()->firstKey();
	if (!lSKey.valid() || !lSChangeKey.valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	//
	if (utxo.size() >= 2)
		buzzerTx_->addDAppIn(lSKey, Transaction::UnlinkedOut::instance(*(++utxo.begin()))); // second out
	else { error_("E_ENTITY_INCORRECT", "DApp outs is incorrect.");	return; }

	//
	composer_->requestProcessor()->selectEntityCountByShards(composer_->dAppName(), 
		SelectEntityCountByShards::instance(
			boost::bind(&BuzzerLightComposer::CreateTxBuzzer::dAppInstancesCountByShardsLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::CreateTxBuzzer::timeout, shared_from_this()))
	);
}

void BuzzerLightComposer::CreateTxBuzzer::dAppInstancesCountByShardsLoaded(const std::map<uint32_t, uint256>& info, const std::string& dapp) {
	//
	if (info.size()) {
		composer_->requestProcessor()->loadTransaction(MainChain::id(), info.begin()->second, 
			LoadTransaction::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzer::shardLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzer::timeout, shared_from_this()))
		);
	} else {
		error_("E_SHARDS_ABSENT", "Shards was not found for DApp."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzer::shardLoaded(TransactionPtr shard) {
	//
	if (!shard) {
		error_("E_SHARD_ABSENT", "Specified shard was not found for DApp.");	
	}

	shardTx_ = TransactionHelper::to<TxShard>(shard);

	composer_->requestProcessor()->selectUtxoByEntity(shardTx_->entityName(), 
		SelectUtxoByEntityName::instance(
			boost::bind(&BuzzerLightComposer::CreateTxBuzzer::utxoByShardLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::CreateTxBuzzer::timeout, shared_from_this()))
	);
}

void BuzzerLightComposer::CreateTxBuzzer::utxoByShardLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& shardName) {
	//
	SKey lSChangeKey = composer_->wallet()->changeKey();
	SKey lSKey = composer_->wallet()->firstKey();
	if (!lSKey.valid() || !lSChangeKey.valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	//
	if (utxo.size() >= 1) {
		buzzerTx_->addShardIn(lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	} else { 
		error_("E_SHARD_INCORRECT", "Shard outs is incorrect."); return; 
	}

	// try to estimate fee
	qunit_t lRate = composer_->settings()->maxFeeRate();
	amount_t lFee = lRate * ctx_->size(); 

	Transaction::UnlinkedOutPtr lChangeUtxo = nullptr;
	std::list<Transaction::UnlinkedOutPtr> lFeeUtxos;
	//
	amount_t lFeeAmount = composer_->wallet()->fillInputs(buzzerTx_, TxAssetType::qbitAsset(), lFee, lFeeUtxos);
	buzzerTx_->addFeeOut(lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		lChangeUtxo = buzzerTx_->addOut(lSChangeKey, lSChangeKey.createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
	}

	if (!buzzerTx_->finalize(lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }
	if (!composer_->writeBuzzerTx(buzzerTx_)) { error_("E_BUZZER_NOT_OPEN", "Buzzer was not open."); return; }

	// we good
	composer_->wallet()->removeUnlinkedOut(lFeeUtxos);

	if (lChangeUtxo) { 
		composer_->wallet()->cacheUnlinkedOut(lChangeUtxo);
	}	

	created_(ctx_);
}

//
// CreateTxBuzz
//
void BuzzerLightComposer::CreateTxBuzz::process(errorFunction error) {
	//
	error_ = error;

	// create empty tx
	buzzTx_ = TransactionHelper::to<TxBuzz>(TransactionFactory::create(TX_BUZZ));
	// create context
	ctx_ = TransactionContext::instance(buzzTx_);
	// get buzzer tx (saved/cached)
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();

	if (lMyBuzzer) {
		// extract bound shard
		if (lMyBuzzer->in().size() > 1) {
			//
			Transaction::In& lShardIn = *(++(lMyBuzzer->in().begin())); // second in
			uint256 lShardTx = lShardIn.out().tx();
			// set shard/chain
			buzzTx_->setChain(lShardTx);
			// set body
			buzzTx_->setBody(body_);
			// set timestamp
			buzzTx_->setTimestamp(qbit::getMedianMicroseconds());

			std::vector<Transaction::UnlinkedOut> lMyBuzzerUtxos = composer_->buzzerUtxo();
			if (!lMyBuzzerUtxos.size()) {
				composer_->requestProcessor()->selectUtxoByEntity(lMyBuzzer->myName(), 
					SelectUtxoByEntityName::instance(
						boost::bind(&BuzzerLightComposer::CreateTxBuzz::utxoByBuzzerLoaded, shared_from_this(), _1, _2),
						boost::bind(&BuzzerLightComposer::CreateTxBuzz::timeout, shared_from_this()))
				);
			} else {
				utxoByBuzzerLoaded(lMyBuzzerUtxos, lMyBuzzer->myName());
			}

		} else {
			error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;	
		}
	} else {
		error_("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzz::utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKey lSChangeKey = composer_->wallet()->changeKey();
	SKey lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey.createPKey();	

	if (!lSKey.valid() || !lSChangeKey.valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size()) {
		error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;	
	}

	// out[0] - buzzer utxo for new buzz
	buzzTx_->addBuzzerIn(lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	// reply out
	buzzTx_->addBuzzReplyOut(lSKey, lPKey); // out[0]
	// re-buzz out
	buzzTx_->addReBuzzOut(lSKey, lPKey); // out[1]
	// like out
	buzzTx_->addBuzzLikeOut(lSKey, lPKey); // out[2]
	// pin out
	buzzTx_->addBuzzPinOut(lSKey, lPKey); // out[3]

	// prepare fee tx
	amount_t lFeeAmount = ctx_->size();
	TransactionContextPtr lFee;

	try {
		lFee = composer_->wallet()->createTxFee(lPKey, lFeeAmount); // size-only, without ratings
		if (!lFee) { error_("E_TX_CREATE", "Transaction creation error."); return; }
	}
	catch(qbit::exception& ex) {
		error_(ex.code(), ex.what()); return;
	}
	catch(std::exception& ex) {
		error_("E_TX_CREATE", ex.what()); return;
	}

	std::list<Transaction::UnlinkedOutPtr> lFeeUtxos = lFee->tx()->utxos(TxAssetType::qbitAsset()); // we need only qbits
	if (lFeeUtxos.size()) {
		// utxo[0]
		buzzTx_->addIn(lSKey, *(lFeeUtxos.begin())); // qbit fee that was exact for fee - in
		buzzTx_->addFeeOut(lSKey, TxAssetType::qbitAsset(), lFeeAmount); // to the miner
	} else {
		error_("E_FEE_UTXO_ABSENT", "Fee utxo for buzz was not found."); return;
	}

	// push linked tx, which need to be pushed and broadcasted before this
	ctx_->addLinkedTx(lFee);

	if (!buzzTx_->finalize(lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

	created_(ctx_);
}

//
// CreateTxBuzzerSubscribe
//
void BuzzerLightComposer::CreateTxBuzzerSubscribe::process(errorFunction error) {
	//
	error_ = error;

	// create empty tx
	buzzerSubscribeTx_ = TransactionHelper::to<TxBuzzerSubscribe>(TransactionFactory::create(TX_BUZZER_SUBSCRIBE));
	// create context
	ctx_ = TransactionContext::instance(buzzerSubscribeTx_);
	// get buzzer tx (saved/cached)
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();

	if (lMyBuzzer) {
		// extract bound shard
		if (lMyBuzzer->in().size() > 1) {
			//
			Transaction::In& lShardIn = *(++(lMyBuzzer->in().begin())); // second in
			uint256 lShardTx = lShardIn.out().tx();
			// set shard/chain
			buzzerSubscribeTx_->setChain(lShardTx);
			// set timestamp
			buzzerSubscribeTx_->setTimestamp(qbit::getMedianMicroseconds());

			composer_->requestProcessor()->selectUtxoByEntity(publisher_, 
				SelectUtxoByEntityName::instance(
					boost::bind(&BuzzerLightComposer::CreateTxBuzzerSubscribe::utxoByPublisherLoaded, shared_from_this(), _1, _2),
					boost::bind(&BuzzerLightComposer::CreateTxBuzzerSubscribe::timeout, shared_from_this()))
			);

		} else {
			error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;
		}
	} else {
		error_("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzerSubscribe::utxoByPublisherLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKey lSChangeKey = composer_->wallet()->changeKey();
	SKey lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey.createPKey();	

	if (!lSKey.valid() || !lSChangeKey.valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size()) {
		error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;	
	}

	if (utxo.size() > 1) {
		buzzerSubscribeTx_->addPublisherBuzzerIn(lSKey, Transaction::UnlinkedOut::instance(*(++(utxo.begin())))); // second out
	} else { error_("E_PUBLISHER_BUZZER_INCORRECT", "Publisher buzzer outs is incorrect."); return; }

	std::vector<Transaction::UnlinkedOut> lMyBuzzerUtxos = composer_->buzzerUtxo();
	if (!lMyBuzzerUtxos.size()) {
		composer_->requestProcessor()->selectUtxoByEntity(composer_->buzzerTx()->myName(), 
			SelectUtxoByEntityName::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerSubscribe::utxoByBuzzerLoaded, shared_from_this(), _1, _2),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerSubscribe::timeout, shared_from_this()))
		);
	} else {
		utxoByBuzzerLoaded(lMyBuzzerUtxos, composer_->buzzerTx()->myName());
	}
}

void BuzzerLightComposer::CreateTxBuzzerSubscribe::utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKey lSChangeKey = composer_->wallet()->changeKey();
	SKey lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey.createPKey();	

	if (!lSKey.valid() || !lSChangeKey.valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size()) {
		error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;	
	}

	if (utxo.size() > 1) {
		buzzerSubscribeTx_->addSubscriberBuzzerIn(lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	} else { error_("E_SUBSCRIBER_BUZZER_INCORRECT", "Subscriber buzzer outs is incorrect."); return; }

	// make buzzer subscription out (for canceling reasons)
	buzzerSubscribeTx_->addSubscriptionOut(lSKey, lPKey); // out[0]

	if (!buzzerSubscribeTx_->finalize(lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

	created_(ctx_);
}

//
// CreateTxBuzzerUnsubscribe
//
void BuzzerLightComposer::CreateTxBuzzerUnsubscribe::process(errorFunction error) {
	//
	error_ = error;

	// create empty tx
	buzzerUnsubscribeTx_ = TransactionHelper::to<TxBuzzerUnsubscribe>(TransactionFactory::create(TX_BUZZER_UNSUBSCRIBE));
	// create context
	ctx_ = TransactionContext::instance(buzzerUnsubscribeTx_);
	// get buzzer tx (saved/cached)
	TransactionPtr lTx = composer_->buzzerTx();

	if (lTx) {
		//
		TxBuzzerPtr lMyBuzzer = TransactionHelper::to<TxBuzzer>(lTx);
		// extract bound shard
		if (lMyBuzzer->in().size() > 1) {
			//
			Transaction::In& lShardIn = *(++(lMyBuzzer->in().begin())); // second in
			shardTx_ = lShardIn.out().tx();
			// set shard/chain
			buzzerUnsubscribeTx_->setChain(shardTx_);
			// set timestamp
			buzzerUnsubscribeTx_->setTimestamp(qbit::getMedianMicroseconds());

			composer_->requestProcessor()->loadEntity(publisher_, 
				LoadEntity::instance(
					boost::bind(&BuzzerLightComposer::CreateTxBuzzerUnsubscribe::publisherLoaded, shared_from_this(), _1),
					boost::bind(&BuzzerLightComposer::CreateTxBuzzerUnsubscribe::timeout, shared_from_this()))
			);

		} else {
			error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;
		}
	} else {
		error_("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzerUnsubscribe::publisherLoaded(TransactionPtr publisher) {
	//
	if (!publisher) { error_("E_BUZZER_NOT_FOUND", "Buzzer not found."); return; }

	// composer_->buzzerTx() already checked
	composer_->buzzerRequestProcessor()->loadSubscription(shardTx_, composer_->buzzerTx()->id(), publisher->id(), 
		LoadTransaction::instance(
			boost::bind(&BuzzerLightComposer::CreateTxBuzzerUnsubscribe::subscriptionLoaded, shared_from_this(), _1),
			boost::bind(&BuzzerLightComposer::CreateTxBuzzerUnsubscribe::timeout, shared_from_this()))
	);
}

void BuzzerLightComposer::CreateTxBuzzerUnsubscribe::subscriptionLoaded(TransactionPtr subscription) {
	//
	if (!subscription) { error_("E_BUZZER_NOT_FOUND", "Buzzer not found."); return; }

	// composer_->buzzerTx() already checked
	composer_->requestProcessor()->selectUtxoByTransaction(shardTx_, subscription->id(), 
		SelectUtxoByTransaction::instance(
			boost::bind(&BuzzerLightComposer::CreateTxBuzzerUnsubscribe::utxoBySubscriptionLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::CreateTxBuzzerUnsubscribe::timeout, shared_from_this()))
	);
}

void BuzzerLightComposer::CreateTxBuzzerUnsubscribe::utxoBySubscriptionLoaded(const std::vector<Transaction::NetworkUnlinkedOut>& utxo, const uint256& tx) {
	//
	SKey lSChangeKey = composer_->wallet()->changeKey();
	SKey lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey.createPKey();	
	//
	if (utxo.size()) {
		// add subscription in
		buzzerUnsubscribeTx_->addSubscriptionIn(lSKey, Transaction::UnlinkedOut::instance(const_cast<Transaction::NetworkUnlinkedOut&>(*utxo.begin()).utxo()));

		// finalize
		if (!buzzerUnsubscribeTx_->finalize(lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

		created_(ctx_);
	} else {
		error_("E_BUZZER_SUBSCRIPTION_UTXO_ABSENT", "Buzzer subscription utxo was not found."); return;
	}
}
