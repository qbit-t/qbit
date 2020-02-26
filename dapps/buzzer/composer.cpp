#include "composer.h"
#include "txbuzzer.h"
#include "txbuzz.h"
#include "../../txdapp.h"
#include "../../txshard.h"
#include "../../mkpath.h"

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

TransactionContextPtr BuzzerComposer::createTxBuzz(const std::string& body) {
	//
	SKey lSKey = wallet_->firstKey();
	PKey lPKey = lSKey.createPKey();
	return createTxBuzz(lPKey, body);
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

			//
			std::vector<Transaction::UnlinkedOutPtr> lMyBuzzerUtxos;
			if (wallet_->entityStore()->collectUtxoByEntityName(lMyBuzzer->myName(), lMyBuzzerUtxos)) {
				//
				SKey lSChangeKey = wallet_->changeKey();
				SKey lSKey = wallet_->firstKey();
				if (!lSKey.valid() || !lSChangeKey.valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

				// out[0] - buzzer utxo for new buzz
				lBuzzTx->addBuzzerIn(lSKey, *(lMyBuzzerUtxos.begin())); // first out
				// make buzz out (reply\buzz-twister)
				lBuzzTx->addBuzzOut(lSKey, self); // out[0]
				// public linkage
				lBuzzTx->addBuzzPublicOut(lSKey, self); // out[1]

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
