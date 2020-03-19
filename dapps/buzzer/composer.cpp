#include "composer.h"
#include "txbuzzer.h"
#include "txbuzz.h"
#include "txbuzzersubscribe.h"
#include "txbuzzerunsubscribe.h"
#include "transactionstoreextension.h"
#include "../../txdapp.h"
#include "../../txshard.h"
#include "../../mkpath.h"
#include "../../timestamp.h"

using namespace qbit;

bool BuzzerComposer::open() {
	if (!opened_) {
		try {
			if (mkpath(std::string(settings_->dataPath() + "/wallet/buzzer").c_str(), 0777)) return false;

			gLog().write(Log::INFO, std::string("[buzzer/wallet/open]: opening buzzer wallet data..."));
			workingSettings_.open();

			std::string lBuzzerTx;
			if (workingSettings_.read("buzzerTx", lBuzzerTx)) {
				buzzerTx_.setHex(lBuzzerTx);
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

bool BuzzerComposer::close() {
	gLog().write(Log::INFO, std::string("[buzzer/wallet/close]: closing buzzer wallet data..."));

	settings_.reset();
	wallet_.reset();

	workingSettings_.close();

	opened_ = false;

	return true;
}

TransactionContextPtr BuzzerComposer::createTxBuzzer(const std::string& buzzerName, const std::string& alias, const std::string& description) {
	//
	SKey lSKey = wallet_->firstKey();
	PKey lPKey = lSKey.createPKey();
	return createTxBuzzer(lPKey, buzzerName, alias, description);
}

TransactionContextPtr BuzzerComposer::createTxBuzzer(const PKey& self, const std::string& buzzerName, const std::string& alias, const std::string& description) {
	// create empty tx
	TxBuzzerPtr lBuzzerTx = TransactionHelper::to<TxBuzzer>(TransactionFactory::create(TX_BUZZER));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lBuzzerTx);
	// dapp entity
	EntityPtr lDApp = wallet_->entityStore()->locateEntity(dAppName()); 
	if (!lDApp) throw qbit::exception("E_ENTITY", "Invalid entity.");

	// check buzzerName
	std::string lBuzzerName;
	if (*buzzerName.begin() != '@') lBuzzerName += '@';
	lBuzzerName += buzzerName; 
	if (wallet_->entityStore()->locateEntity(lBuzzerName)) {
		throw qbit::exception("E_BUZZER_EXISTS", "Buzzer name already taken.");		
	}

	//
	lBuzzerTx->setMyName(lBuzzerName);
	lBuzzerTx->setAlias(alias);
	lBuzzerTx->setDescription(description);

	SKey lSChangeKey = wallet_->changeKey();
	SKey lSKey = wallet_->firstKey();
	if (!lSKey.valid() || !lSChangeKey.valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	// make buzzer out (for buzz creation)
	lBuzzerTx->addBuzzerOut(lSKey, self);
	// for subscriptions
	lBuzzerTx->addBuzzerSubscriptionOut(lSKey, self);
	// endorse
	lBuzzerTx->addBuzzerEndorseOut(lSKey, self);
	// mistrust
	lBuzzerTx->addBuzzerMistrustOut(lSKey, self);

	// make inputs
	std::vector<Transaction::UnlinkedOutPtr> lUtxos;
	if (wallet_->entityStore()->collectUtxoByEntityName(dAppName(), lUtxos)) {
		if (lUtxos.size() >= 2)
			lBuzzerTx->addDAppIn(lSKey, *(++lUtxos.begin())); // second out
		else throw qbit::exception("E_ENTITY_INCORRECT", "DApp outs is incorrect.");
	} else {
		throw qbit::exception("E_ENTITY_OUT", "DApp utxo was not found.");
	}

	std::map<uint32_t, uint256> lShardInfo;
	if (wallet_->entityStore()->entityCountByShards(dAppName(), lShardInfo)) {
		// load tx -> use lesser count
		TransactionPtr lShardTx = wallet_->persistentStore()->locateTransaction(lShardInfo.begin()->second);
		if (lShardTx) {
			TxShardPtr lShard = TransactionHelper::to<TxShard>(lShardTx);
			//
			std::vector<Transaction::UnlinkedOutPtr> lShardUtxos;
			if (wallet_->entityStore()->collectUtxoByEntityName(lShard->entityName(), lShardUtxos)) {
				if (lShardUtxos.size() >= 1)
					lBuzzerTx->addShardIn(lSKey, *(lShardUtxos.begin())); // first out
				else throw qbit::exception("E_SHARD_INCORRECT", "Shard outs is incorrect.");
			} else {
				throw qbit::exception("E_SHARD_OUT", "Shard utxo was not found.");
			}
		} else {
			throw qbit::exception("E_SHARD_ABSENT", "Specified shard was not found for DApp.");	
		}
	} else  {
		throw qbit::exception("E_SHARDS_ABSENT", "Shards was not found for DApp.");
	}

	// try to estimate fee
	qunit_t lRate = wallet_->mempool()->estimateFeeRateByLimit(lCtx, settings_->maxFeeRate());

	amount_t lFee = lRate * lCtx->size();
	amount_t lFeeAmount = wallet_->fillInputs(lBuzzerTx, TxAssetType::qbitAsset(), lFee);
	lBuzzerTx->addFeeOut(lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		lBuzzerTx->addOut(lSChangeKey, lSChangeKey.createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
	}

	if (!lBuzzerTx->finalize(lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	// write to pending transactions
	wallet_->writePendingTransaction(lBuzzerTx->id(), lBuzzerTx);

	// load buzzer tx
	if (!open()) throw qbit::exception("E_BUZZER_NOT_OPEN", "Buzzer was not open.");

	// save our buzzer
	std::string lBuzzerTxHex = lBuzzerTx->id().toHex();
	workingSettings_.write("buzzerTx", lBuzzerTxHex);
	buzzerTx_ = lBuzzerTx->id();
	
	return lCtx;
}

uint256 BuzzerComposer::attachBuzzer(const std::string& buzzer) {
	// dapp
	EntityPtr lDApp = wallet_->entityStore()->locateEntity(dAppName()); 
	if (!lDApp) throw qbit::exception("E_ENTITY", "Invalid entity.");

	// buzzer
	EntityPtr lBuzzer = wallet_->entityStore()->locateEntity(buzzer); 
	if (!lBuzzer) throw qbit::exception("E_BUZZER_ENTITY_NOT_FOUND", "Buzzer entity was not found.");

	// extract and match
	TxBuzzerPtr lMyBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzer);
	PKey lMyPKey;
	if (lMyBuzzer->extractAddress(lMyPKey) && wallet_->findKey(lMyPKey).valid()) {
		// load buzzer tx
		if (!open()) throw qbit::exception("E_BUZZER_NOT_OPEN", "Buzzer was not open.");

		// save our buzzer
		std::string lBuzzerTxHex = lMyBuzzer->id().toHex();
		workingSettings_.write("buzzerTx", lBuzzerTxHex);
		buzzerTx_ = lMyBuzzer->id();

		return buzzerTx_;
	}

	return uint256();
}

TransactionContextPtr BuzzerComposer::createTxBuzz(const std::string& body) {
	//
	SKey lSKey = wallet_->firstKey();
	PKey lPKey = lSKey.createPKey();
	return createTxBuzz(lPKey, body);
}

TransactionContextPtr BuzzerComposer::createTxBuzz(const std::string& publisher, const std::string& body) {
	// dapp
	EntityPtr lDApp = wallet_->entityStore()->locateEntity(dAppName()); 
	if (!lDApp) throw qbit::exception("E_ENTITY", "Invalid entity.");

	// buzzer
	EntityPtr lBuzzer = wallet_->entityStore()->locateEntity(publisher); 
	if (!lBuzzer) throw qbit::exception("E_BUZZER_ENTITY_NOT_FOUND", "Buzzer entity was not found.");

	// extract and match
	TxBuzzerPtr lMyBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzer);
	PKey lMyPKey;
	if (lMyBuzzer->extractAddress(lMyPKey) && wallet_->findKey(lMyPKey).valid()) {
		// load buzzer tx
		if (!open()) throw qbit::exception("E_BUZZER_NOT_OPEN", "Buzzer was not open.");

		// save our buzzer
		std::string lBuzzerTxHex = lMyBuzzer->id().toHex();
		workingSettings_.write("buzzerTx", lBuzzerTxHex);
		buzzerTx_ = lMyBuzzer->id();

		return createTxBuzz(lMyPKey, body);
	} else throw qbit::exception("E_INVALID_BUZZER_OWNER", "Invalid buzzer owner. Check your keys ans try again.");

	return nullptr;
}

TransactionContextPtr BuzzerComposer::createTxBuzz(const PKey& self, const std::string& body) {
	// create empty tx
	TxBuzzPtr lBuzzTx = TransactionHelper::to<TxBuzz>(TransactionFactory::create(TX_BUZZ));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lBuzzTx);
	// dapp entity - check
	EntityPtr lDApp = wallet_->entityStore()->locateEntity(dAppName()); 
	if (!lDApp) throw qbit::exception("E_ENTITY", "Invalid entity.");

	// load buzzer tx
	if (!open()) throw qbit::exception("E_BUZZER_NOT_OPEN", "Buzzer was not open.");

	TransactionPtr lTx = wallet_->persistentStore()->locateTransaction(buzzerTx_);

	if (lTx) {
		//
		TxBuzzerPtr lMyBuzzer = TransactionHelper::to<TxBuzzer>(lTx);
		// extract bound shard
		if (lMyBuzzer->in().size() > 1) {
			//
			Transaction::In& lShardIn = *(++(lMyBuzzer->in().begin())); // second in
			uint256 lShardTx = lShardIn.out().tx();
			// set shard/chain
			lBuzzTx->setChain(lShardTx);
			// set body
			lBuzzTx->setBody(body);
			// set timestamp
			lBuzzTx->setTimestamp(qbit::getMedianMicroseconds());

			//
			std::vector<Transaction::UnlinkedOutPtr> lMyBuzzerUtxos;
			if (wallet_->entityStore()->collectUtxoByEntityName(lMyBuzzer->myName(), lMyBuzzerUtxos)) {
				//
				SKey lSChangeKey = wallet_->changeKey();
				SKey lSKey = wallet_->firstKey();
				if (!lSKey.valid() || !lSChangeKey.valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

				// out[0] - buzzer utxo for new buzz
				lBuzzTx->addBuzzerIn(lSKey, *(lMyBuzzerUtxos.begin())); // first out
				// reply out
				lBuzzTx->addBuzzReplyOut(lSKey, self); // out[0]
				// re-buzz out
				lBuzzTx->addReBuzzOut(lSKey, self); // out[1]
				// like out
				lBuzzTx->addBuzzLikeOut(lSKey, self); // out[2]
				// pin out
				lBuzzTx->addBuzzPinOut(lSKey, self); // out[3]

				// prepare fee tx
				amount_t lFeeAmount = lCtx->size();
				TransactionContextPtr lFee = wallet_->createTxFee(self, lFeeAmount); // size-only, without ratings
				std::list<Transaction::UnlinkedOutPtr> lFeeUtxos = lFee->tx()->utxos(TxAssetType::qbitAsset()); // we need only qbits
				if (lFeeUtxos.size()) {
					// utxo[0]
					lBuzzTx->addIn(lSKey, *(lFeeUtxos.begin())); // qbit fee that was exact for fee - in
					lBuzzTx->addFeeOut(lSKey, TxAssetType::qbitAsset(), lFeeAmount); // to the miner
				} else {
					throw qbit::exception("E_FEE_UTXO_ABSENT", "Fee utxo for buzz was not found.");
				}

				// push linked tx, which need to be pushed and broadcasted before this
				lCtx->addLinkedTx(lFee);

				if (!lBuzzTx->finalize(lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

				// write to pending transactions
				wallet_->writePendingTransaction(lBuzzTx->id(), lBuzzTx);

				// we good
				return lCtx;

			} else {
				throw qbit::exception("E_BUZZER_UTXO_ABSENT", "Buzzer utxo was not found.");
			}

		} else {
			throw qbit::exception("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent.");	
		}
	} else {
		throw qbit::exception("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found.");
	}
	
	return nullptr;
}

TransactionContextPtr BuzzerComposer::createTxBuzzerSubscribe(const std::string& publisher) {
	//
	SKey lSKey = wallet_->firstKey();
	PKey lPKey = lSKey.createPKey();
	return createTxBuzzerSubscribe(lPKey, publisher);
}

TransactionContextPtr BuzzerComposer::createTxBuzzerSubscribe(const std::string& owner, const std::string& publisher) {
	// dapp
	EntityPtr lDApp = wallet_->entityStore()->locateEntity(dAppName()); 
	if (!lDApp) throw qbit::exception("E_ENTITY", "Invalid entity.");

	// buzzer
	EntityPtr lBuzzer = wallet_->entityStore()->locateEntity(owner); 
	if (!lBuzzer) throw qbit::exception("E_BUZZER_ENTITY_NOT_FOUND", "Buzzer entity was not found.");

	// extract and match
	TxBuzzerPtr lMyBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzer);
	PKey lMyPKey;
	if (lMyBuzzer->extractAddress(lMyPKey) && wallet_->findKey(lMyPKey).valid()) {
		// load buzzer tx
		if (!open()) throw qbit::exception("E_BUZZER_NOT_OPEN", "Buzzer was not open.");

		// save our buzzer
		std::string lBuzzerTxHex = lMyBuzzer->id().toHex();
		workingSettings_.write("buzzerTx", lBuzzerTxHex);
		buzzerTx_ = lMyBuzzer->id();

		return createTxBuzzerSubscribe(lMyPKey, publisher);
	} else throw qbit::exception("E_INVALID_BUZZER_OWNER", "Invalid buzzer owner. Check your keys ans try again.");

	return nullptr;
}

TransactionContextPtr BuzzerComposer::createTxBuzzerSubscribe(const PKey& self, const std::string& publisher) {
	//
	// create empty tx
	TxBuzzerSubscribePtr lBuzzerSubscribe = TransactionHelper::to<TxBuzzerSubscribe>(TransactionFactory::create(TX_BUZZER_SUBSCRIBE));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lBuzzerSubscribe);
	// dapp entity - check
	EntityPtr lPublisher = wallet_->entityStore()->locateEntity(publisher); 
	if (!lPublisher) throw qbit::exception("E_BUZZER_NOT_FOUND", "Buzzer not found.");

	// load buzzer tx
	if (!open()) throw qbit::exception("E_BUZZER_NOT_OPEN", "Buzzer was not open.");
	TransactionPtr lTx = wallet_->persistentStore()->locateTransaction(buzzerTx_);

	if (lTx) {
		//
		TxBuzzerPtr lMyBuzzer = TransactionHelper::to<TxBuzzer>(lTx);
		// extract bound shard
		if (lMyBuzzer->in().size() > 1) {
			//
			Transaction::In& lShardIn = *(++(lMyBuzzer->in().begin())); // second in
			uint256 lShardTx = lShardIn.out().tx();
			// set shard/chain
			lBuzzerSubscribe->setChain(lShardTx);
			// set timestamp
			lBuzzerSubscribe->setTimestamp(qbit::getMedianMicroseconds());			

			//
			std::vector<Transaction::UnlinkedOutPtr> lPublisherUtxos;
			if (wallet_->entityStore()->collectUtxoByEntityName(publisher, lPublisherUtxos)) {
				//
				SKey lSChangeKey = wallet_->changeKey();
				SKey lSKey = wallet_->firstKey();
				if (!lSKey.valid() || !lSChangeKey.valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

				if (lPublisherUtxos.size() > 1) {
					lBuzzerSubscribe->addPublisherBuzzerIn(lSKey, *(++(lPublisherUtxos.begin()))); // second out
				} else throw qbit::exception("E_PUBLISHER_BUZZER_INCORRECT", "Publisher buzzer outs is incorrect.");

				std::vector<Transaction::UnlinkedOutPtr> lMyBuzzerUtxos;
				if (!wallet_->entityStore()->collectUtxoByEntityName(lMyBuzzer->myName(), lMyBuzzerUtxos)) {
					throw qbit::exception("E_BUZZER_UTXO_ABSENT", "Buzzer utxo was not found.");
				}

				if (lMyBuzzerUtxos.size() > 1) {
					lBuzzerSubscribe->addSubscriberBuzzerIn(lSKey, *(lMyBuzzerUtxos.begin())); // first out
				} else throw qbit::exception("E_SUBSCRIBER_BUZZER_INCORRECT", "Subscriber buzzer outs is incorrect.");

				// make buzzer subscription out (for canceling reasons)
				lBuzzerSubscribe->addSubscriptionOut(lSKey, self); // out[0]

				if (!lBuzzerSubscribe->finalize(lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

				// write to pending transactions
				wallet_->writePendingTransaction(lBuzzerSubscribe->id(), lBuzzerSubscribe);

				// we good
				return lCtx;

			} else {
				throw qbit::exception("E_BUZZER_UTXO_ABSENT", "Buzzer utxo was not found.");
			}
		} else {
			throw qbit::exception("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent.");	
		}
	} else {
		throw qbit::exception("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found.");
	}
	
	return nullptr;
}

TransactionContextPtr BuzzerComposer::createTxBuzzerUnsubscribe(const std::string& publisher) {
	//
	SKey lSKey = wallet_->firstKey();
	PKey lPKey = lSKey.createPKey();
	return createTxBuzzerUnsubscribe(lPKey, publisher);
}

TransactionContextPtr BuzzerComposer::createTxBuzzerUnsubscribe(const std::string& owner, const std::string& publisher) {
	// dapp
	EntityPtr lDApp = wallet_->entityStore()->locateEntity(dAppName()); 
	if (!lDApp) throw qbit::exception("E_ENTITY", "Invalid entity.");

	// buzzer
	EntityPtr lBuzzer = wallet_->entityStore()->locateEntity(owner); 
	if (!lBuzzer) throw qbit::exception("E_BUZZER_ENTITY_NOT_FOUND", "Buzzer entity was not found.");

	// extract and match
	TxBuzzerPtr lMyBuzzer = TransactionHelper::to<TxBuzzer>(lBuzzer);
	PKey lMyPKey;
	if (lMyBuzzer->extractAddress(lMyPKey) && wallet_->findKey(lMyPKey).valid()) {
		// load buzzer tx
		if (!open()) throw qbit::exception("E_BUZZER_NOT_OPEN", "Buzzer was not open.");

		// save our buzzer
		std::string lBuzzerTxHex = lMyBuzzer->id().toHex();
		workingSettings_.write("buzzerTx", lBuzzerTxHex);
		buzzerTx_ = lMyBuzzer->id();

		return createTxBuzzerUnsubscribe(lMyPKey, publisher);
	} else throw qbit::exception("E_INVALID_BUZZER_OWNER", "Invalid buzzer owner. Check your keys ans try again.");

	return nullptr;
}

TransactionContextPtr BuzzerComposer::createTxBuzzerUnsubscribe(const PKey& self, const std::string& publisher) {
	//
	// create empty tx
	TxBuzzerUnsubscribePtr lBuzzerUnsubscribe = TransactionHelper::to<TxBuzzerUnsubscribe>(TransactionFactory::create(TX_BUZZER_UNSUBSCRIBE));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lBuzzerUnsubscribe);
	// dapp entity - check
	EntityPtr lPublisher = wallet_->entityStore()->locateEntity(publisher); 
	if (!lPublisher) throw qbit::exception("E_BUZZER_NOT_FOUND", "Buzzer not found.");

	// load buzzer tx
	if (!open()) throw qbit::exception("E_BUZZER_NOT_OPEN", "Buzzer was not open.");
	TransactionPtr lTx = wallet_->persistentStore()->locateTransaction(buzzerTx_);

	if (lTx) {
		//
		TxBuzzerPtr lSubscriber = TransactionHelper::to<TxBuzzer>(lTx);
		// extract bound shard
		if (lSubscriber->in().size() > 1) {
			//
			Transaction::In& lShardIn = *(++(lSubscriber->in().begin())); // second in
			uint256 lShardTx = lShardIn.out().tx();

			// locate subscription
			ITransactionStorePtr lStore = wallet_->storeManager()->locate(lShardTx);
			if (lStore && lStore->extension()) {
				//
				BuzzerTransactionStoreExtensionPtr lExtension = std::static_pointer_cast<BuzzerTransactionStoreExtension>(lStore->extension());
				TransactionPtr lSubscription = lExtension->locateSubscription(lSubscriber->id(), lPublisher->id());
				if (lSubscription) {
					//
					SKey lSChangeKey = wallet_->changeKey();
					SKey lSKey = wallet_->firstKey();
					if (!lSKey.valid() || !lSChangeKey.valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

					// set shard/chain
					lBuzzerUnsubscribe->setChain(lShardTx);
					// set timestamp
					lBuzzerUnsubscribe->setTimestamp(qbit::getMedianMicroseconds());			

					std::vector<Transaction::UnlinkedOutPtr> lOuts;
					if (lStore->enumUnlinkedOuts(lSubscription->id(), lOuts)) {
						// add subscription in
						lBuzzerUnsubscribe->addSubscriptionIn(lSKey, *lOuts.begin());

						// finalize
						if (!lBuzzerUnsubscribe->finalize(lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

						// write to pending transactions
						wallet_->writePendingTransaction(lBuzzerUnsubscribe->id(), lBuzzerUnsubscribe);

						// we good
						return lCtx;

					} else {
						throw qbit::exception("E_BUZZER_SUBSCRIPTION_UTXO_ABSENT", "Buzzer subscription utxo was not found.");
					}

				} else {
					// subscription is absent
					throw qbit::exception("E_BUZZER_SUBSCRIPTION_ABSENT", "Buzzer subscription was not found.");
				}

			} else {
				throw qbit::exception("E_STORE_NOT_FOUND", "Storage not found.");
			}

		} else {
			throw qbit::exception("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent.");	
		}
	} else {
		throw qbit::exception("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found.");
	}
	
	return nullptr;
}
