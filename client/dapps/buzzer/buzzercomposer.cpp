#include "buzzercomposer.h"

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
			subscriptions_.open();

			std::string lBuzzerTx;
			if (workingSettings_.read("buzzerTx", lBuzzerTx)) {
				std::vector<unsigned char> lBuzzerTxHex = ParseHex(lBuzzerTx);

				DataStream lStream(lBuzzerTxHex, SER_NETWORK, PROTOCOL_VERSION);
				TransactionPtr lBuzzer = Transaction::Deserializer::deserialize<DataStream>(lStream);
				buzzerTx_ = TransactionHelper::to<TxBuzzer>(lBuzzer);

				requestProcessor_->clearDApps();
				requestProcessor_->addDAppInstance(State::DAppInstance("buzzer", buzzerTx_->id()));
				instanceChanged_(buzzerTx_->id());
				gLog().write(Log::INFO, std::string("[buzzer/wallet/open]: buzzer = ") + buzzerTx_->id().toHex());
			}

			std::string lBuzzerInfoTx;
			if (workingSettings_.read("buzzerInfoTx", lBuzzerInfoTx)) {
				std::vector<unsigned char> lBuzzerTxHex = ParseHex(lBuzzerInfoTx);

				DataStream lStream(lBuzzerTxHex, SER_NETWORK, PROTOCOL_VERSION);
				TransactionPtr lBuzzerInfo = Transaction::Deserializer::deserialize<DataStream>(lStream);
				buzzerInfoTx_ = TransactionHelper::to<TxBuzzerInfo>(lBuzzerInfo);

				buzzer_->pushBuzzerInfo(buzzerInfoTx_);

				gLog().write(Log::INFO, std::string("[buzzer/wallet/open]: buzzer info = ") + buzzerInfoTx_->id().toHex());
			}

			std::string lBuzzerUtxo;
			if (workingSettings_.read("buzzerUtxo", lBuzzerUtxo)) {
				std::vector<unsigned char> lBuzzerUtxoHex = ParseHex(lBuzzerUtxo);

				DataStream lStream(lBuzzerUtxoHex, SER_NETWORK, PROTOCOL_VERSION);
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
	subscriptions_.close();

	opened_ = false;

	return true;
}

void BuzzerLightComposer::setSecret(const std::string& secret) {
	//
	SKey lSKey;
	uint256 lKey = TxAssetType::qbitAsset();
	lSKey.set<unsigned char*>(lKey.begin(), lKey.end());
	lSKey.encrypt(secret);

	std::string lText = HexStr(lSKey.begin(), lSKey.end());
	workingSettings_.write("buzzerCypher", lText);
}

bool BuzzerLightComposer::verifySecret(const std::string& secret) {
	//		
	std::string lText;
	if (workingSettings_.read("buzzerCypher", lText)) {
		std::vector<unsigned char> lKey = ParseHex(lText);

		SKey lSKey;
		lSKey.setEncryptedKey(lKey);
		lSKey.decrypt(secret);

		return lSKey.equal(TxAssetType::qbitAsset());
	}

	return false;
}

Buzzer::VerificationResult BuzzerLightComposer::verifyPublisherLazy(BuzzfeedItemPtr item) {
	//
	PKey lPKey;
	//
	if (item->type() == TX_BUZZ_LIKE) {
		bool lResult = false;
		for (std::vector<BuzzfeedItem::ItemInfo>::const_iterator lInfo = item->infos().begin(); lInfo != item->infos().end(); lInfo++) {
			//
			bool lFound = subscriptions_.read(lInfo->buzzerId(), lPKey);
			if (!lFound) {
				//
				TxBuzzerInfoPtr lBuzzerInfo = buzzer_->locateBuzzerInfo(lInfo->buzzerInfoId());
				if (lBuzzerInfo && lBuzzerInfo->extractAddress(lPKey)) lFound = true;
			}

			if (lFound) {
				lResult = TxBuzzLike::verifySignature(lPKey, item->type(), lInfo->timestamp(), lInfo->score(),
					lInfo->buzzerInfoId(), item->buzzId(), lInfo->signature());
			} else {
				return Buzzer::VerificationResult::POSTPONED;
			}
		}

		return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);

	} else if (item->type() == TX_BUZZ_REWARD) {
		bool lResult = false;
		for (std::vector<BuzzfeedItem::ItemInfo>::const_iterator lInfo = item->infos().begin(); lInfo != item->infos().end(); lInfo++) {
			//
			bool lFound = subscriptions_.read(lInfo->buzzerId(), lPKey);
			if (!lFound) {
				//
				TxBuzzerInfoPtr lBuzzerInfo = buzzer_->locateBuzzerInfo(lInfo->buzzerInfoId());
				if (lBuzzerInfo && lBuzzerInfo->extractAddress(lPKey)) lFound = true;
			}

			if (lFound) {
				lResult = TxBuzzReward::verifySignature(lPKey, item->type(), lInfo->timestamp(), lInfo->score(), 
					lInfo->amount(), lInfo->buzzerInfoId(), item->buzzId(), lInfo->signature());
			} else {
				return Buzzer::VerificationResult::POSTPONED;
			}
		}

		return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);

	} else if (item->type() == TX_BUZZER_ENDORSE || item->type() == TX_BUZZER_MISTRUST) {
		//
		bool lResult = false;
		BuzzfeedItem::ItemInfo lInfo = *(item->infos().begin());
		//
		bool lFound = subscriptions_.read(item->buzzerId(), lPKey);
		if (!lFound) {
			//
			TxBuzzerInfoPtr lBuzzerInfo = buzzer_->locateBuzzerInfo(item->buzzerInfoId());
			if (lBuzzerInfo && lBuzzerInfo->extractAddress(lPKey)) lFound = true;
		}

		if (lFound) {
			lResult = TxBuzzerEndorse::verifySignature(lPKey, item->type(), item->timestamp(), item->score(),
				item->buzzerInfoId(), item->value(), lInfo.buzzerId(),	item->signature());
		} else {
			return Buzzer::VerificationResult::POSTPONED;
		}

		return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);

	} else {
		//
		bool lFound = subscriptions_.read(item->buzzerId(), lPKey);
		if (!lFound) {
			//
			TxBuzzerInfoPtr lBuzzerInfo = buzzer_->locateBuzzerInfo(item->buzzerInfoId());
			if (lBuzzerInfo && lBuzzerInfo->extractAddress(lPKey)) lFound = true;
		}

		if (lFound) {
			//
			if (item->type() == TX_REBUZZ) {
				if (item->originalBuzzId().isNull()) {
					bool lResult = false;
					for (std::vector<BuzzfeedItem::ItemInfo>::const_iterator lInfo = item->infos().begin(); lInfo != item->infos().end(); lInfo++) {
						//
						bool lFound = subscriptions_.read(lInfo->buzzerId(), lPKey);
						if (!lFound) {
							//
							TxBuzzerInfoPtr lBuzzerInfo = buzzer_->locateBuzzerInfo(lInfo->buzzerInfoId());
							if (lBuzzerInfo && lBuzzerInfo->extractAddress(lPKey)) lFound = true;
						}

						if (lFound) {
							lResult = TxReBuzz::verifySignature(lPKey, item->type(), lInfo->timestamp(), lInfo->score(),
											lInfo->buzzerInfoId(), std::vector<unsigned char>(), std::vector<BuzzerMediaPointer>(), 
											item->buzzId(), item->buzzChainId(), lInfo->signature());
						} else {
							return Buzzer::VerificationResult::POSTPONED;
						}
					}

					return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);

				} else {
					if (TxReBuzz::verifySignature(lPKey, item->type(), item->timestamp(), item->score(),
						item->buzzerInfoId(), item->buzzBody(), item->buzzMedia(), 
						item->originalBuzzId(), item->originalBuzzChainId(), item->signature()))
						return Buzzer::VerificationResult::SUCCESS;
					else
						return Buzzer::VerificationResult::INVALID;
				}
			}

			if (TxBuzz::verifySignature(lPKey, item->type(), item->timestamp(), item->score(),
					item->buzzerInfoId(), item->buzzBody(), item->buzzMedia(), item->signature()))
				return Buzzer::VerificationResult::SUCCESS;
			else
				return Buzzer::VerificationResult::INVALID;
		}
	}

	return Buzzer::VerificationResult::POSTPONED;
}

Buzzer::VerificationResult BuzzerLightComposer::verifyPublisherStrict(BuzzfeedItemPtr item) {
	//
	PKey lPKey;
	//
	if (item->type() == TX_BUZZ_LIKE) {
		bool lResult = false;
		for (std::vector<BuzzfeedItem::ItemInfo>::const_iterator lInfo = item->infos().begin(); lInfo != item->infos().end(); lInfo++) {
			if (subscriptions_.read(lInfo->buzzerId(), lPKey)) {
				lResult = TxBuzzLike::verifySignature(lPKey, item->type(), lInfo->timestamp(), lInfo->score(),
					lInfo->buzzerInfoId(), item->buzzId(), lInfo->signature());
				if (!lResult) break;
			}
		}

		return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);

	} else if (item->type() == TX_BUZZ_REWARD) {
		//
		bool lResult = false;
		for (std::vector<BuzzfeedItem::ItemInfo>::const_iterator lInfo = item->infos().begin(); lInfo != item->infos().end(); lInfo++) {
			if (subscriptions_.read(lInfo->buzzerId(), lPKey)) {
				lResult = TxBuzzReward::verifySignature(lPKey, item->type(), lInfo->timestamp(), lInfo->score(), 
					lInfo->amount(), lInfo->buzzerInfoId(), item->buzzId(), lInfo->signature());
				if (!lResult) break;
			}
		}

		return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);

	} else {
		//
		if (item->type() == TX_REBUZZ) {
			if (item->infos().size() /*item->originalBuzzId().isNull()*/) {
				bool lResult = false;
				for (std::vector<BuzzfeedItem::ItemInfo>::const_iterator lInfo = item->infos().begin(); lInfo != item->infos().end(); lInfo++) {
					if (subscriptions_.read(lInfo->buzzerId(), lPKey)) {
						lResult = TxReBuzz::verifySignature(lPKey, item->type(), lInfo->timestamp(), lInfo->score(),
										lInfo->buzzerInfoId(), std::vector<unsigned char>(), std::vector<BuzzerMediaPointer>(), 
										item->buzzId(), item->buzzChainId(), lInfo->signature());
						if (!lResult) break;
					}
				}

				return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);				
			} else {
				if (subscriptions_.read(item->buzzerId(), lPKey))
					if (TxReBuzz::verifySignature(lPKey, item->type(), item->timestamp(), item->score(),
						item->buzzerInfoId(), item->buzzBody(), item->buzzMedia(), item->originalBuzzId(),
						item->originalBuzzChainId(), item->signature())) {
						return Buzzer::VerificationResult::SUCCESS;
					}
			}
		} else if (subscriptions_.read(item->buzzerId(), lPKey)) {
			if (item->type() == TX_BUZZER_ENDORSE || item->type() == TX_BUZZER_MISTRUST) {
				//
				bool lResult = false;
				BuzzfeedItem::ItemInfo lInfo = *(item->infos().begin());
				lResult = TxBuzzerEndorse::verifySignature(lPKey, item->type(), item->timestamp(), item->score(),
					item->buzzerInfoId(), item->value(), lInfo.buzzerId(), item->signature());

				return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);
			}

			if (TxBuzz::verifySignature(lPKey, item->type(), item->timestamp(), item->score(), item->buzzerInfoId(),
				item->buzzBody(), item->buzzMedia(), item->signature())) return Buzzer::VerificationResult::SUCCESS;
		}
	}

	return Buzzer::VerificationResult::INVALID;
}

Buzzer::VerificationResult BuzzerLightComposer::verifyEventPublisher(EventsfeedItemPtr item) {
	//
	//
	PKey lPKey;
	//
	if (item->type() == TX_BUZZ_LIKE) {
		//
		bool lResult = false;

		// check events
		for (std::vector<EventsfeedItem::EventInfo>::const_iterator lInfo = item->buzzers().begin(); lInfo != item->buzzers().end(); lInfo++) {
			//
			bool lFound = subscriptions_.read(lInfo->buzzerId(), lPKey);
			if (!lFound) {
				//
				TxBuzzerInfoPtr lBuzzerInfo = buzzer_->locateBuzzerInfo(lInfo->buzzerInfoId());
				if (lBuzzerInfo && lBuzzerInfo->extractAddress(lPKey)) lFound = true;
			}

			if (lFound) {
				lResult = TxBuzzLike::verifySignature(lPKey, item->type(), lInfo->timestamp(), lInfo->score(),
					lInfo->buzzerInfoId(), item->buzzId(), lInfo->signature());
			} else {
				return Buzzer::VerificationResult::POSTPONED;
			}
		}

		return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);

	} else if (item->type() == TX_BUZZ_REWARD) {
		//
		bool lResult = false;

		// check events
		for (std::vector<EventsfeedItem::EventInfo>::const_iterator lInfo = item->buzzers().begin(); lInfo != item->buzzers().end(); lInfo++) {
			//
			bool lFound = subscriptions_.read(lInfo->buzzerId(), lPKey);
			if (!lFound) {
				//
				TxBuzzerInfoPtr lBuzzerInfo = buzzer_->locateBuzzerInfo(lInfo->buzzerInfoId());
				if (lBuzzerInfo && lBuzzerInfo->extractAddress(lPKey)) lFound = true;
			}

			if (lFound) {
				lResult = TxBuzzReward::verifySignature(lPKey, item->type(), lInfo->timestamp(), lInfo->score(),
					item->value(), lInfo->buzzerInfoId(), item->buzzId(), lInfo->signature());
			} else {
				return Buzzer::VerificationResult::POSTPONED;
			}
		}

		return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);

	} else if (item->type() == TX_BUZZER_ENDORSE || item->type() == TX_BUZZER_MISTRUST) {
		//
		bool lResult = false;
		EventsfeedItem::EventInfo lInfo = *(item->buzzers().begin());
		//
		bool lFound = subscriptions_.read(lInfo.buzzerId(), lPKey);
		if (!lFound) {
			//
			TxBuzzerInfoPtr lBuzzerInfo = buzzer_->locateBuzzerInfo(lInfo.buzzerInfoId());
			if (lBuzzerInfo && lBuzzerInfo->extractAddress(lPKey)) lFound = true;
		}

		if (lFound) {
			//std::cout << strprintf("%d, %d, %s, %d, %s", lInfo.timestamp(), lInfo.score(),
			//	lInfo.buzzerInfoId().toHex(), item->value(), lInfo.buzzerId().toHex()) << "\n";
			lResult = TxBuzzerEndorse::verifySignature(lPKey, item->type(), lInfo.timestamp(), lInfo.score(),
				lInfo.buzzerInfoId(), item->value(), item->publisher(), lInfo.signature());
		} else {
			return Buzzer::VerificationResult::POSTPONED;
		}

		return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);

	} else if (item->type() == TX_BUZZER_SUBSCRIBE) {
		//
		bool lResult = false;
		EventsfeedItem::EventInfo lInfo = *(item->buzzers().begin());
		//
		bool lFound = subscriptions_.read(lInfo.buzzerId(), lPKey);
		if (!lFound) {
			//
			TxBuzzerInfoPtr lBuzzerInfo = buzzer_->locateBuzzerInfo(lInfo.buzzerInfoId());
			if (lBuzzerInfo && lBuzzerInfo->extractAddress(lPKey)) lFound = true;
		}

		if (lFound) {
			lResult = TxBuzzerSubscribe::verifySignature(lPKey, item->type(), lInfo.timestamp(), lInfo.score(),
				lInfo.buzzerInfoId(), item->publisher(), lInfo.signature());
		} else {
			return Buzzer::VerificationResult::POSTPONED;
		}

		return (lResult == true ? Buzzer::VerificationResult::SUCCESS : Buzzer::VerificationResult::INVALID);
	} else {
		//
		bool lResult = false;
		EventsfeedItem::EventInfo lInfo = *(item->buzzers().begin());
		//
		bool lFound = subscriptions_.read(lInfo.buzzerId(), lPKey);
		if (!lFound) {
			//
			TxBuzzerInfoPtr lBuzzerInfo = buzzer_->locateBuzzerInfo(lInfo.buzzerInfoId());
			if (lBuzzerInfo && lBuzzerInfo->extractAddress(lPKey)) lFound = true;
		}

		if (lFound) {
			//
			if (item->type() == TX_REBUZZ) {
				if (TxReBuzz::verifySignature(lPKey, item->type(), lInfo.timestamp(), lInfo.score(),
					lInfo.buzzerInfoId(), lInfo.buzzBody(), lInfo.buzzMedia(), 
					item->buzzId(), item->buzzChainId(), lInfo.signature()))
					return Buzzer::VerificationResult::SUCCESS;
				else
					return Buzzer::VerificationResult::INVALID;
			}

			if (TxBuzz::verifySignature(lPKey, item->type(), lInfo.timestamp(), lInfo.score(),
					lInfo.buzzerInfoId(), lInfo.buzzBody(), lInfo.buzzMedia(), lInfo.signature()))
				return Buzzer::VerificationResult::SUCCESS;
			else
				return Buzzer::VerificationResult::INVALID;
		}
	}

	return Buzzer::VerificationResult::POSTPONED;
}

Buzzer::VerificationResult BuzzerLightComposer::verifyMyThreadUpdates(const uint256& buzzId, uint64_t nonce, const uint512& signature) {
	// make data
	DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
	lSource << buzzId;
	lSource << nonce;

	SKeyPtr lSKey = wallet()->firstKey();
	if (lSKey) {
		uint256 lHash = Hash(lSource.begin(), lSource.end());
		return (lSKey->createPKey().verify(lHash, signature) ?	Buzzer::VerificationResult::SUCCESS :  
																Buzzer::VerificationResult::INVALID);
	}

	return Buzzer::VerificationResult::INVALID;
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

	//
	if (lBuzzerName.length() < 5) {
		error_("E_BUZZER_NAME_TOO_SHORT", "Buzzer name should be at least 5 symbols long.");
		return;
	}

	//
	bool lFail = false;
	for (std::string::iterator lChar = ++lBuzzerName.begin(); lChar != lBuzzerName.end(); lChar++) {
		//
		if((*lChar >= 'a' && *lChar <= 'z') || 
			(*lChar >= 'A' && *lChar <= 'Z') ||
			(*lChar >= '0' && *lChar <= '9')) continue;

		lFail = true;
		break;
	}

	//
	if (lFail) {
		error_("E_BUZZER_NAME_INCORRECT", "Buzzer name is incorrect.");
		return;
	}
	
	//
	buzzerName_ = lBuzzerName;

	composer_->requestProcessor()->loadEntity(buzzerName_, 
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

	// create empty tx
	buzzerTx_ = TransactionHelper::to<TxBuzzer>(TransactionFactory::create(TX_BUZZER));
	// create context
	ctx_ = TransactionContext::instance(buzzerTx_);
	//
	buzzerTx_->setMyName(buzzerName_);

	SKeyPtr lSChangeKey = composer_->wallet()->changeKey();
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	SKeyPtr lFirstKey = composer_->wallet()->firstKey();
	PKey lSelf = lSKey->createPKey();
	// make buzzer out (for buzz creation)
	buzzerOut_ = buzzerTx_->addBuzzerOut(*lSKey, lSelf); // out[0]
	// for subscriptions
	buzzerTx_->addBuzzerSubscriptionOut(*lSKey, lSelf); // out[1]
	// endorse
	buzzerTx_->addBuzzerEndorseOut(*lSKey, lSelf);
	// mistrust
	buzzerTx_->addBuzzerMistrustOut(*lSKey, lSelf);
	// buzzin
	buzzerTx_->addBuzzerBuzzOut(*lSKey, lSelf);

	composer_->requestProcessor()->selectUtxoByEntity(composer_->dAppName(), 
		SelectUtxoByEntityName::instance(
			boost::bind(&BuzzerLightComposer::CreateTxBuzzer::utxoByDAppLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::CreateTxBuzzer::timeout, shared_from_this()))
	);
}

void BuzzerLightComposer::CreateTxBuzzer::utxoByDAppLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& dapp) {
	//
	SKeyPtr lSChangeKey = composer_->wallet()->changeKey();
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	//
	if (utxo.size() >= 2)
		buzzerTx_->addDAppIn(*lSKey, Transaction::UnlinkedOut::instance(*(++utxo.begin()))); // second out
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
	SKeyPtr lSChangeKey = composer_->wallet()->changeKey();
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	//
	if (utxo.size() >= 1) {
		buzzerTx_->addShardIn(*lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	} else { 
		error_("E_SHARD_INCORRECT", "Shard outs is incorrect."); return; 
	}

	// try to estimate fee
	qunit_t lRate = composer_->settings()->maxFeeRate();
	amount_t lFee = lRate * ctx_->size(); 

	Transaction::UnlinkedOutPtr lChangeUtxo = nullptr;
	std::list<Transaction::UnlinkedOutPtr> lFeeUtxos;
	//
	try {
		amount_t lFeeAmount = composer_->wallet()->fillInputs(buzzerTx_, TxAssetType::qbitAsset(), lFee, lFeeUtxos);
		buzzerTx_->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
		if (lFeeAmount > lFee) { // make change
			lChangeUtxo = buzzerTx_->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
		}
	}
	catch(qbit::exception& ex) {
		error_(ex.code(), ex.what()); return;
	}
	catch(std::exception& ex) {
		error_("E_TX_CREATE", ex.what()); return;
	}
	
	if (!buzzerTx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }
	if (!composer_->writeBuzzerTx(buzzerTx_)) { error_("E_BUZZER_NOT_OPEN", "Buzzer was not open."); return; }

	composer_->addSubscription(buzzerTx_->id(), lSKey->createPKey());

	// we good
	composer_->wallet()->removeUnlinkedOut(lFeeUtxos);

	if (lChangeUtxo) { 
		composer_->wallet()->cacheUnlinkedOut(lChangeUtxo);
	}	

	created_(ctx_, buzzerOut_);
}

//
// CreateTxBuzzerInfo
//
void BuzzerLightComposer::CreateTxBuzzerInfo::process(errorFunction error) {
	//
	error_ = error;

	// create empty tx
	tx_ = TransactionHelper::to<TxBuzzerInfo>(TransactionFactory::create(TX_BUZZER_INFO));
	// create context
	ctx_ = TransactionContext::instance(tx_);
	// get buzzer tx (saved/cached)
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();

	if (lMyBuzzer) {
		// extract bound shard
		if (lMyBuzzer->in().size() > 1) {
			//
			SKeyPtr lSKey = composer_->wallet()->firstKey();
			if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

			//
			Transaction::In& lShardIn = *(++(lMyBuzzer->in().begin())); // second in
			uint256 lShardTx = lShardIn.out().tx();
			// set name
			tx_->setMyName(lMyBuzzer->myName());
			// set shard/chain
			tx_->setChain(lShardTx);
			// set avatar
			tx_->setAvatar(avatar_);
			// set header
			tx_->setHeader(header_);
			// set alias
			tx_->setAlias(alias_);
			// set desc
			tx_->setDescription(description_);
			// sign
			tx_->makeSignature(*lSKey);

			std::vector<Transaction::UnlinkedOut> lMyBuzzerUtxos = composer_->buzzerUtxo();
			if (!lMyBuzzerUtxos.size() && !buzzer_) {
				composer_->requestProcessor()->selectUtxoByEntity(lMyBuzzer->myName(), 
					SelectUtxoByEntityName::instance(
						boost::bind(&BuzzerLightComposer::CreateTxBuzzerInfo::saveBuzzerUtxo, shared_from_this(), _1, _2),
						boost::bind(&BuzzerLightComposer::CreateTxBuzzerInfo::timeout, shared_from_this()))
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

void BuzzerLightComposer::CreateTxBuzzerInfo::saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	composer_->writeBuzzerUtxo(utxo);
	utxoByBuzzerLoaded(utxo, lMyBuzzer->myName());
}

void BuzzerLightComposer::CreateTxBuzzerInfo::utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	

	if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size() && !buzzer_) {
		error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;	
	}

	// out[0] - buzzer utxo for new buzz
	if (utxo.size()) tx_->addMyBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	else tx_->addMyBuzzerIn(*lSKey, buzzer_);

	if (!tx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }
	if (!composer_->writeBuzzerInfoTx(tx_)) { error_("E_BUZZER_NOT_OPEN", "Buzzer was not open."); return; }	

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
	// check
	if (!composer_->buzzerInfoTx()) {
		error_("E_BUZZER_INFO_ABSENT", "Buzzer info is absent."); return;
	}

	if (lMyBuzzer) {
		// extract bound shard
		if (lMyBuzzer->in().size() > 1) {
			//
			SKeyPtr lSKey = composer_->wallet()->firstKey();
			if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

			//
			Transaction::In& lShardIn = *(++(lMyBuzzer->in().begin())); // second in
			uint256 lShardTx = lShardIn.out().tx();
			// set shard/chain
			buzzTx_->setChain(lShardTx);
			buzzTx_->setScore(composer_->buzzer()->score());
			buzzTx_->setBuzzerInfo(composer_->buzzerInfoTx()->id());
			buzzTx_->setBuzzerInfoChain(composer_->buzzerInfoTx()->chain());
			buzzTx_->setMediaPointers(mediaPointers_);
			// set timestamp
			buzzTx_->setTimestamp(qbit::getMedianMicroseconds());
			// set body
			buzzTx_->setBody(body_, *lSKey);

			std::vector<Transaction::UnlinkedOut> lMyBuzzerUtxos = composer_->buzzerUtxo();
			if (!lMyBuzzerUtxos.size()) {
				composer_->requestProcessor()->selectUtxoByEntity(lMyBuzzer->myName(), 
					SelectUtxoByEntityName::instance(
						boost::bind(&BuzzerLightComposer::CreateTxBuzz::saveBuzzerUtxo, shared_from_this(), _1, _2),
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

void BuzzerLightComposer::CreateTxBuzz::saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	composer_->writeBuzzerUtxo(utxo);
	utxoByBuzzerLoaded(utxo, lMyBuzzer->myName());
}

void BuzzerLightComposer::CreateTxBuzz::utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	

	if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size()) {
		error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;	
	}

	// out[0] - buzzer utxo for new buzz
	buzzTx_->addMyBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	// reply out
	buzzTx_->addBuzzReplyOut(*lSKey, lPKey); // out[0]
	// re-buzz out
	buzzTx_->addReBuzzOut(*lSKey, lPKey); // out[1]
	// like out
	buzzTx_->addBuzzLikeOut(*lSKey, lPKey); // out[2]
	// reward out
	buzzTx_->addBuzzRewardOut(*lSKey, lPKey); // out[3]
	// pin out
	buzzTx_->addBuzzPinOut(*lSKey, lPKey); // out[4]

	// check buzzers list
	if (!buzzers_.size()) {
		composer_->requestProcessor()->selectUtxoByEntityNames(buzzers_, 
			SelectUtxoByEntityNames::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzz::utxoByBuzzersListLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxBuzz::timeout, shared_from_this()))
		);
	} else {
		utxoByBuzzersListLoaded(std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>());
	}
}

void BuzzerLightComposer::CreateTxBuzz::utxoByBuzzersListLoaded(const std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>& entityUtxos) {
	//
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();
	if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	//
	std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>& lEntityUtxos = const_cast<std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>&>(entityUtxos);
	for (std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>::iterator lEntity = lEntityUtxos.begin(); lEntity != lEntityUtxos.end(); lEntity++) {
		//
		// in[1] - @buzzer.out[TX_BUZZER_BUZZ_OUT] or @buzzer.out[TX_BUZZER_REPLY_OUT]
		// in[2] - @buzzer.out[TX_BUZZER_BUZZ_OUT] or @buzzer.out[TX_BUZZER_REPLY_OUT]	
		// ...
		if (lEntity->utxo().size() > TX_BUZZER_BUZZ_OUT) {
			buzzTx_->addBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(lEntity->utxo()[TX_BUZZER_BUZZ_OUT]));
		}
	}

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
		buzzTx_->addIn(*lSKey, *(lFeeUtxos.begin())); // qbit fee that was exact for fee - in
		buzzTx_->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFeeAmount); // to the miner
	} else {
		error_("E_FEE_UTXO_ABSENT", "Fee utxo for buzz was not found."); return;
	}

	// push linked tx, which need to be pushed and broadcasted before this
	ctx_->addLinkedTx(lFee);

	if (!buzzTx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

	created_(ctx_);
}

//
// CreateTxBuzzerSubscribe
//
void BuzzerLightComposer::CreateTxBuzzerSubscribe::process(errorFunction error) {
	//
	error_ = error;
	publisherTx_ = nullptr;

	// create empty tx
	buzzerSubscribeTx_ = TransactionHelper::to<TxBuzzerSubscribe>(TransactionFactory::create(TX_BUZZER_SUBSCRIBE));
	// create context
	ctx_ = TransactionContext::instance(buzzerSubscribeTx_);
	// get buzzer tx (saved/cached)
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();

	if (lMyBuzzer) {
		// set timestamp
		buzzerSubscribeTx_->setTimestamp(qbit::getMedianMicroseconds());

		composer_->requestProcessor()->loadEntity(publisher_, 
			LoadEntity::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerSubscribe::publisherLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerSubscribe::timeout, shared_from_this()))
		);
	} else {
		error_("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzerSubscribe::publisherLoaded(EntityPtr publisher) {
	//
	if (!publisher) { error_("E_BUZZER_NOT_FOUND", "Buzzer not found."); return; }

	//
	publisherTx_ = publisher;

	//
	// extract bound shard
	if (publisher->in().size() > 1) {
		//
		Transaction::In& lShardIn = *(++(publisher->in().begin())); // second in
		// set shard/chain
		buzzerSubscribeTx_->setChain(lShardIn.out().tx());
		buzzerSubscribeTx_->setScore(composer_->buzzer()->score());
		buzzerSubscribeTx_->setBuzzerInfo(composer_->buzzerInfoTx()->id());
		buzzerSubscribeTx_->setBuzzerInfoChain(composer_->buzzerInfoTx()->chain());

		// add subscription
		TxBuzzerPtr lBuzzer = TransactionHelper::to<TxBuzzer>(publisher);
		PKey lPKey;
		if (lBuzzer->extractAddress(lPKey)) {
			composer_->addSubscription(publisher->id(), lPKey);
		}

		composer_->requestProcessor()->selectUtxoByEntity(publisher_, 
			SelectUtxoByEntityName::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerSubscribe::utxoByPublisherLoaded, shared_from_this(), _1, _2),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerSubscribe::timeout, shared_from_this()))
		);
	} else {
		error_("E_BUZZER_PUBLISHER_INCONSISTENT", "Publisher is inconsistent."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzerSubscribe::utxoByPublisherLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKeyPtr lSChangeKey = composer_->wallet()->changeKey();
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	

	if (!lSKey->valid() || !lSChangeKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size()) {
		error_("E_BUZZER_UTXO_INCONSISTENT", "Publisher utxo is inconsistent."); return;	
	}

	if (utxo.size() > 1) {
		buzzerSubscribeTx_->addPublisherBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(*(++(utxo.begin())))); // second out
	} else { error_("E_PUBLISHER_BUZZER_INCORRECT", "Publisher buzzer outs is incorrect."); return; }

	std::vector<Transaction::UnlinkedOut> lMyBuzzerUtxos = composer_->buzzerUtxo();
	if (!lMyBuzzerUtxos.size()) {
		composer_->requestProcessor()->selectUtxoByEntity(composer_->buzzerTx()->myName(), 
			SelectUtxoByEntityName::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerSubscribe::saveBuzzerUtxo, shared_from_this(), _1, _2),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerSubscribe::timeout, shared_from_this()))
		);
	} else {
		utxoByBuzzerLoaded(lMyBuzzerUtxos, composer_->buzzerTx()->myName());
	}
}

void BuzzerLightComposer::CreateTxBuzzerSubscribe::saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	composer_->writeBuzzerUtxo(utxo);
	utxoByBuzzerLoaded(utxo, lMyBuzzer->myName());
}

void BuzzerLightComposer::CreateTxBuzzerSubscribe::utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKeyPtr lSChangeKey = composer_->wallet()->changeKey();
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	

	if (!lSKey->valid() || !lSChangeKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size()) {
		error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;	
	}

	if (utxo.size() > 1) {
		buzzerSubscribeTx_->addSubscriberBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	} else { error_("E_SUBSCRIBER_BUZZER_INCORRECT", "Subscriber buzzer outs is incorrect."); return; }

	// make extra signature
	buzzerSubscribeTx_->makeSignature(*lSKey, publisherTx_->id());

	// make buzzer subscription out (for canceling reasons)
	buzzerSubscribeTx_->addSubscriptionOut(*lSKey, lPKey); // out[0]

	if (!buzzerSubscribeTx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

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
		buzzerUnsubscribeTx_->setTimestamp(qbit::getMedianMicroseconds());

		composer_->requestProcessor()->loadEntity(publisher_, 
			LoadEntity::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerUnsubscribe::publisherLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerUnsubscribe::timeout, shared_from_this()))
		);
	} else {
		error_("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzerUnsubscribe::publisherLoaded(EntityPtr publisher) {
	//
	if (!publisher) { error_("E_BUZZER_NOT_FOUND", "Buzzer not found."); return; }

	//
	// extract bound shard
	if (publisher->in().size() > 1) {
		//
		Transaction::In& lShardIn = *(++(publisher->in().begin())); // second in
		shardTx_ = lShardIn.out().tx();
		// set shard/chain
		buzzerUnsubscribeTx_->setChain(shardTx_);
		// remove subscription
		composer_->removeSubscription(publisher->id());

		// composer_->buzzerTx() already checked
		if (!composer_->buzzerRequestProcessor()->loadSubscription(shardTx_, composer_->buzzerTx()->id(), publisher->id(), 
			LoadTransaction::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerUnsubscribe::subscriptionLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerUnsubscribe::timeout, shared_from_this()))
		)) error_("E_LOAD_SUBSCRIPTION", "Load subscription failed.");
	} else {
		error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;
	}
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
	SKeyPtr lSChangeKey = composer_->wallet()->changeKey();
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	
	//
	if (utxo.size()) {
		// add subscription in
		buzzerUnsubscribeTx_->addSubscriptionIn(*lSKey, Transaction::UnlinkedOut::instance(const_cast<Transaction::NetworkUnlinkedOut&>(*utxo.begin()).utxo()));

		// finalize
		if (!buzzerUnsubscribeTx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

		created_(ctx_);
	} else {
		error_("E_BUZZER_SUBSCRIPTION_UTXO_ABSENT", "Buzzer subscription utxo was not found."); return;
	}
}

//
// LoadBuzzfeed
//
void BuzzerLightComposer::LoadBuzzfeed::process(errorFunction error) {
	//
	error_ = error;

	// 
	if (!(count_ = composer_->buzzerRequestProcessor()->selectBuzzfeed(chain_, from_, composer_->buzzerTx()->id(), requests_,
		SelectBuzzFeed::instance(
			boost::bind(&BuzzerLightComposer::LoadBuzzfeed::buzzfeedLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::LoadBuzzfeed::timeout, shared_from_this()))
	))) error_("E_LOAD_BUZZFEED", "Buzzfeed loading failed.");
}

//
// LoadBuzzesByBuzz
//
void BuzzerLightComposer::LoadBuzzesByBuzz::process(errorFunction error) {
	//
	error_ = error;

	// 
	if (!(count_ = composer_->buzzerRequestProcessor()->selectBuzzfeedByBuzz(chain_, from_, buzz_, requests_,
		SelectBuzzFeedByEntity::instance(
			boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzz::buzzfeedLoaded, shared_from_this(), _1, _2, _3),
			boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzz::timeout, shared_from_this()))
	))) error_("E_LOAD_BUZZFEED", "Buzzfeed for buzz loading failed.");
}

//
// LoadBuzzesByBuzzer
//
void BuzzerLightComposer::LoadBuzzesByBuzzer::process(errorFunction error) {
	//
	error_ = error;

	if (buzzerId_.isNull()) {
		if (!composer_->requestProcessor()->loadEntity(buzzer_, 
			LoadEntity::instance(
				boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzzer::publisherLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzzer::timeout, shared_from_this()))
		)) error_("E_LOAD_BUZZER", "Buzzer loading failed.");
	} else {
		if (from_.size()) {
			bool lSent = false;
			for (std::vector<BuzzfeedPublisherFrom>::iterator lPublisher = from_.begin(); lPublisher != from_.end(); lPublisher++) {
				//
				if (lPublisher->publisher() == buzzerId_) {
					if (!(count_ = composer_->buzzerRequestProcessor()->selectBuzzfeedByBuzzer(chain_, lPublisher->from(), buzzerId_, requests_,
						SelectBuzzFeedByEntity::instance(
							boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzzer::buzzfeedLoaded, shared_from_this(), _1, _2, _3),
							boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzzer::timeout, shared_from_this()))
					))) error_("E_LOAD_BUZZFEED", "Buzzfeed for buzzer loading failed.");
					lSent = true;
					break;
				}
			}

			if (!lSent)
				error_("E_LOAD_BUZZFEED_BUZZER_TIMESTAMP", "Buzzer timestamp was not found.");
		} else {
			if (!(count_ = composer_->buzzerRequestProcessor()->selectBuzzfeedByBuzzer(chain_, 0, buzzerId_, requests_,
				SelectBuzzFeedByEntity::instance(
					boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzzer::buzzfeedLoaded, shared_from_this(), _1, _2, _3),
					boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzzer::timeout, shared_from_this()))
			))) error_("E_LOAD_BUZZFEED", "Buzzfeed for buzzer loading failed.");			
		}
	}
}

void BuzzerLightComposer::LoadBuzzesByBuzzer::publisherLoaded(EntityPtr publisher) {
	//
	if (!publisher) {
		error_("E_PUBLISHER_NOT_FOUND", "Buzzer was not found.");
		return;
	}

	if (from_.size()) {
		bool lSent = false;
		for (std::vector<BuzzfeedPublisherFrom>::iterator lPublisher = from_.begin(); lPublisher != from_.end(); lPublisher++) {
			//
			if (lPublisher->publisher() == publisher->id()) {
				if (!(count_ = composer_->buzzerRequestProcessor()->selectBuzzfeedByBuzzer(chain_, lPublisher->from(), publisher->id(), requests_,
					SelectBuzzFeedByEntity::instance(
						boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzzer::buzzfeedLoaded, shared_from_this(), _1, _2, _3),
						boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzzer::timeout, shared_from_this()))
				))) error_("E_LOAD_BUZZFEED", "Buzzfeed for buzzer loading failed.");
				lSent = true;
				break;
			}
		}

		if (!lSent)
			error_("E_LOAD_BUZZFEED_BUZZER_TIMESTAMP", "Buzzer timestamp was not found.");
	} else {
		if (!(count_ = composer_->buzzerRequestProcessor()->selectBuzzfeedByBuzzer(chain_, 0, publisher->id(), requests_,
			SelectBuzzFeedByEntity::instance(
				boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzzer::buzzfeedLoaded, shared_from_this(), _1, _2, _3),
				boost::bind(&BuzzerLightComposer::LoadBuzzesByBuzzer::timeout, shared_from_this()))
		))) error_("E_LOAD_BUZZFEED", "Buzzfeed for buzzer loading failed.");		
	}
}

//
// LoadBuzzesGlobal
//
void BuzzerLightComposer::LoadBuzzesGlobal::process(errorFunction error) {
	//
	error_ = error;

	// 
	if (!(count_ = composer_->buzzerRequestProcessor()->selectBuzzfeedGlobal(
		chain_, 
		timeframeFrom_,
		scoreFrom_,
		timestampFrom_,
		publisher_, 
		requests_,
		SelectBuzzFeed::instance(
			boost::bind(&BuzzerLightComposer::LoadBuzzesGlobal::buzzfeedLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::LoadBuzzesGlobal::timeout, shared_from_this()))
	))) error_("E_LOAD_BUZZFEED", "Buzzfeed for buzzer loading failed.");
}

//
// LoadBuzzesByTag
//
void BuzzerLightComposer::LoadBuzzesByTag::process(errorFunction error) {
	//
	error_ = error;

	// 
	if (!(count_ = composer_->buzzerRequestProcessor()->selectBuzzfeedByTag(
		chain_,
		tag_,
		timeframeFrom_,
		scoreFrom_,
		timestampFrom_,
		publisher_, 
		requests_,
		SelectBuzzFeed::instance(
			boost::bind(&BuzzerLightComposer::LoadBuzzesByTag::buzzfeedLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::LoadBuzzesByTag::timeout, shared_from_this()))
	))) error_("E_LOAD_BUZZFEED", "Buzzfeed for buzzer loading failed.");
}

//
// LoadHashTags
//
void BuzzerLightComposer::LoadHashTags::process(errorFunction error) {
	//
	error_ = error;

	// 
	if (!(count_ = composer_->buzzerRequestProcessor()->selectHashTags(
		chain_,
		tag_,
		requests_,
		SelectHashTags::instance(
			boost::bind(&BuzzerLightComposer::LoadHashTags::loaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::LoadHashTags::timeout, shared_from_this()))
	))) error_("E_LOAD_BUZZFEED", "Hash tags loading failed.");
}

//
// LoadEventsfeed
//
void BuzzerLightComposer::LoadEventsfeed::process(errorFunction error) {
	//
	error_ = error;

	// 
	if (!(count_ = composer_->buzzerRequestProcessor()->selectEventsfeed(chain_, from_, composer_->buzzerTx()->id(), requests_,
		SelectEventsFeed::instance(
			boost::bind(&BuzzerLightComposer::LoadEventsfeed::eventsfeedLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::LoadEventsfeed::timeout, shared_from_this()))
	))) error_("E_LOAD_EVENTSFEED", "Eventsfeed loading failed.");
}

//
// LoadMistrustsByBuzzer
//
void BuzzerLightComposer::LoadMistrustsByBuzzer::process(errorFunction error) {
	//
	error_ = error;

	//
	if (!buzzer_.isNull()) { 
		if (!(count_ = composer_->buzzerRequestProcessor()->selectMistrustsByBuzzer(chain_, from_, buzzer_, requests_,
			SelectEventsFeedByEntity::instance(
				boost::bind(&BuzzerLightComposer::LoadMistrustsByBuzzer::eventsfeedLoaded, shared_from_this(), _1, _2, _3),
				boost::bind(&BuzzerLightComposer::LoadMistrustsByBuzzer::timeout, shared_from_this()))
		))) error_("E_LOAD_EVENTSFEED", "Endorsements by buzzer loading failed.");
	} else {
		if (!composer_->requestProcessor()->loadEntity(buzzerName_, 
			LoadEntity::instance(
				boost::bind(&BuzzerLightComposer::LoadMistrustsByBuzzer::publisherLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::LoadMistrustsByBuzzer::timeout, shared_from_this()))
		)) error_("E_LOAD_BUZZER", "Buzzer loading failed.");
	}
}

void BuzzerLightComposer::LoadMistrustsByBuzzer::publisherLoaded(EntityPtr publisher) {
	//
	if (!publisher) {
		error_("E_PUBLISHER_NOT_FOUND", "Buzzer was not found.");
		return;
	}

	if (!(count_ = composer_->buzzerRequestProcessor()->selectMistrustsByBuzzer(chain_, from_, publisher->id(), requests_,
		SelectEventsFeedByEntity::instance(
			boost::bind(&BuzzerLightComposer::LoadMistrustsByBuzzer::eventsfeedLoaded, shared_from_this(), _1, _2, _3),
			boost::bind(&BuzzerLightComposer::LoadMistrustsByBuzzer::timeout, shared_from_this()))
	))) error_("E_LOAD_EVENTSFEED", "Endorsements by buzzer loading failed.");
}

//
// LoadSubscriptionsByBuzzer
//
void BuzzerLightComposer::LoadSubscriptionsByBuzzer::process(errorFunction error) {
	//
	error_ = error;

	//
	if (!buzzer_.isNull()) { 
		if (!(count_ = composer_->buzzerRequestProcessor()->selectSubscriptionsByBuzzer(chain_, from_, buzzer_, requests_,
			SelectEventsFeedByEntity::instance(
				boost::bind(&BuzzerLightComposer::LoadSubscriptionsByBuzzer::eventsfeedLoaded, shared_from_this(), _1, _2, _3),
				boost::bind(&BuzzerLightComposer::LoadSubscriptionsByBuzzer::timeout, shared_from_this()))
		))) error_("E_LOAD_EVENTSFEED", "Subscriptions by buzzer loading failed.");
	} else {
		if (!composer_->requestProcessor()->loadEntity(buzzerName_, 
			LoadEntity::instance(
				boost::bind(&BuzzerLightComposer::LoadSubscriptionsByBuzzer::publisherLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::LoadSubscriptionsByBuzzer::timeout, shared_from_this()))
		)) error_("E_LOAD_BUZZER", "Buzzer loading failed.");
	}
}

void BuzzerLightComposer::LoadSubscriptionsByBuzzer::publisherLoaded(EntityPtr publisher) {
	//
	if (!publisher) {
		error_("E_PUBLISHER_NOT_FOUND", "Buzzer was not found.");
		return;
	}

	if (!(count_ = composer_->buzzerRequestProcessor()->selectSubscriptionsByBuzzer(chain_, from_, publisher->id(), requests_,
		SelectEventsFeedByEntity::instance(
			boost::bind(&BuzzerLightComposer::LoadSubscriptionsByBuzzer::eventsfeedLoaded, shared_from_this(), _1, _2, _3),
			boost::bind(&BuzzerLightComposer::LoadSubscriptionsByBuzzer::timeout, shared_from_this()))
	))) error_("E_LOAD_EVENTSFEED", "Endorsements by buzzer loading failed.");
}

//
// LoadFollowersByBuzzer
//
void BuzzerLightComposer::LoadFollowersByBuzzer::process(errorFunction error) {
	//
	error_ = error;

	// 
	if (!buzzer_.isNull()) { 
		if (!(count_ = composer_->buzzerRequestProcessor()->selectFollowersByBuzzer(chain_, from_, buzzer_, requests_,
			SelectEventsFeedByEntity::instance(
				boost::bind(&BuzzerLightComposer::LoadFollowersByBuzzer::eventsfeedLoaded, shared_from_this(), _1, _2, _3),
				boost::bind(&BuzzerLightComposer::LoadFollowersByBuzzer::timeout, shared_from_this()))
		))) error_("E_LOAD_EVENTSFEED", "Followers by buzzer loading failed.");
	} else {
		if (!composer_->requestProcessor()->loadEntity(buzzerName_, 
			LoadEntity::instance(
				boost::bind(&BuzzerLightComposer::LoadFollowersByBuzzer::publisherLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::LoadFollowersByBuzzer::timeout, shared_from_this()))
		)) error_("E_LOAD_BUZZER", "Buzzer loading failed.");
	}
}

void BuzzerLightComposer::LoadFollowersByBuzzer::publisherLoaded(EntityPtr publisher) {
	//
	if (!publisher) {
		error_("E_PUBLISHER_NOT_FOUND", "Buzzer was not found.");
		return;
	}

	if (!(count_ = composer_->buzzerRequestProcessor()->selectFollowersByBuzzer(chain_, from_, publisher->id(), requests_,
		SelectEventsFeedByEntity::instance(
			boost::bind(&BuzzerLightComposer::LoadFollowersByBuzzer::eventsfeedLoaded, shared_from_this(), _1, _2, _3),
			boost::bind(&BuzzerLightComposer::LoadFollowersByBuzzer::timeout, shared_from_this()))
	))) error_("E_LOAD_EVENTSFEED", "Endorsements by buzzer loading failed.");
}

//
// LoadEndorsementsByBuzzer
//
void BuzzerLightComposer::LoadEndorsementsByBuzzer::process(errorFunction error) {
	//
	error_ = error;

	//
	if (!buzzer_.isNull()) { 
		if (!(count_ = composer_->buzzerRequestProcessor()->selectEndorsementsByBuzzer(chain_, from_, buzzer_, requests_,
			SelectEventsFeedByEntity::instance(
				boost::bind(&BuzzerLightComposer::LoadEndorsementsByBuzzer::eventsfeedLoaded, shared_from_this(), _1, _2, _3),
				boost::bind(&BuzzerLightComposer::LoadEndorsementsByBuzzer::timeout, shared_from_this()))
		))) error_("E_LOAD_EVENTSFEED", "Endorsements by buzzer loading failed.");
	} else {
		if (!composer_->requestProcessor()->loadEntity(buzzerName_, 
			LoadEntity::instance(
				boost::bind(&BuzzerLightComposer::LoadEndorsementsByBuzzer::publisherLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::LoadEndorsementsByBuzzer::timeout, shared_from_this()))
		)) error_("E_LOAD_BUZZER", "Buzzer loading failed.");
	}
}

void BuzzerLightComposer::LoadEndorsementsByBuzzer::publisherLoaded(EntityPtr publisher) {
	//
	if (!publisher) {
		error_("E_PUBLISHER_NOT_FOUND", "Buzzer was not found.");
		return;
	}

	if (!(count_ = composer_->buzzerRequestProcessor()->selectEndorsementsByBuzzer(chain_, from_, publisher->id(), requests_,
		SelectEventsFeedByEntity::instance(
			boost::bind(&BuzzerLightComposer::LoadEndorsementsByBuzzer::eventsfeedLoaded, shared_from_this(), _1, _2, _3),
			boost::bind(&BuzzerLightComposer::LoadEndorsementsByBuzzer::timeout, shared_from_this()))
	))) error_("E_LOAD_EVENTSFEED", "Endorsements by buzzer loading failed.");
}

//
// CreateTxBuzzLike
//
void BuzzerLightComposer::CreateTxBuzzLike::process(errorFunction error) {
	//
	error_ = error;
	buzzUtxo_.clear();

	// 
	if (!composer_->requestProcessor()->selectUtxoByTransaction(chain_, buzz_, 
		SelectUtxoByTransaction::instance(
			boost::bind(&BuzzerLightComposer::CreateTxBuzzLike::utxoByBuzzLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::CreateTxBuzzLike::timeout, shared_from_this()))
	)) { error_("E_LOAD_UTXO_BY_BUZZ", "Buzz loading failed."); return; }
}

void BuzzerLightComposer::CreateTxBuzzLike::utxoByBuzzLoaded(const std::vector<Transaction::NetworkUnlinkedOut>& utxo, const uint256& tx) {
	//
	buzzUtxo_ = utxo;

	//
	std::vector<Transaction::UnlinkedOut> lMyBuzzerUtxos = composer_->buzzerUtxo();
	if (!lMyBuzzerUtxos.size()) {
		composer_->requestProcessor()->selectUtxoByEntity(composer_->buzzerTx()->myName(), 
			SelectUtxoByEntityName::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzLike::saveBuzzerUtxo, shared_from_this(), _1, _2),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzLike::timeout, shared_from_this()))
		);
	} else {
		utxoByBuzzerLoaded(lMyBuzzerUtxos, composer_->buzzerTx()->myName());
	}
}

void BuzzerLightComposer::CreateTxBuzzLike::saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	composer_->writeBuzzerUtxo(utxo);
	utxoByBuzzerLoaded(utxo, lMyBuzzer->myName());
}

void BuzzerLightComposer::CreateTxBuzzLike::utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	TxBuzzLikePtr lTx = TransactionHelper::to<TxBuzzLike>(TransactionFactory::create(TX_BUZZ_LIKE));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lTx);
	//
	lTx->setTimestamp(qbit::getMedianMicroseconds());
	lTx->setChain(chain_);
	lTx->setScore(composer_->buzzer()->score());
	lTx->setBuzzerInfo(composer_->buzzerInfoTx()->id());
	lTx->setBuzzerInfoChain(composer_->buzzerInfoTx()->chain());

	// get buzzer tx (saved/cached)
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	if (lMyBuzzer) {
		SKeyPtr lSKey = composer_->wallet()->firstKey();
		//
		if (buzzUtxo_.size() > TX_BUZZ_LIKE_OUT && utxo.size() > TX_BUZZER_MY_OUT) {
			// add my byzzer in
			lTx->addMyBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(const_cast<Transaction::UnlinkedOut&>(utxo[TX_BUZZER_MY_OUT])));
			// add buzz/event in
			lTx->addBuzzLikeIn(*lSKey, Transaction::UnlinkedOut::instance(buzzUtxo_[TX_BUZZ_LIKE_OUT].utxo()));
			// sign
			lTx->makeSignature(*lSKey, buzzUtxo_[TX_BUZZ_LIKE_OUT].utxo().out().tx());

			// finalize
			if (!lTx->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

			created_(lCtx);
		} else {
			error_("E_BUZZ_UTXO_ABSENT", "Buzz utxo was not found."); return;
		}
	} else {
		error_("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found."); return;
	}
}

//
// CreateTxBuzzReward
//
void BuzzerLightComposer::CreateTxBuzzReward::process(errorFunction error) {
	//
	error_ = error;
	buzzUtxo_.clear();

	// 
	if (!composer_->requestProcessor()->selectUtxoByTransaction(chain_, buzz_, 
		SelectUtxoByTransaction::instance(
			boost::bind(&BuzzerLightComposer::CreateTxBuzzReward::utxoByBuzzLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::CreateTxBuzzReward::timeout, shared_from_this()))
	)) { error_("E_LOAD_UTXO_BY_BUZZ", "Buzz loading failed."); return; }
}

void BuzzerLightComposer::CreateTxBuzzReward::utxoByBuzzLoaded(const std::vector<Transaction::NetworkUnlinkedOut>& utxo, const uint256& tx) {
	//
	buzzUtxo_ = utxo;

	//
	std::vector<Transaction::UnlinkedOut> lMyBuzzerUtxos = composer_->buzzerUtxo();
	if (!lMyBuzzerUtxos.size()) {
		composer_->requestProcessor()->selectUtxoByEntity(composer_->buzzerTx()->myName(), 
			SelectUtxoByEntityName::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzReward::saveBuzzerUtxo, shared_from_this(), _1, _2),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzReward::timeout, shared_from_this()))
		);
	} else {
		utxoByBuzzerLoaded(lMyBuzzerUtxos, composer_->buzzerTx()->myName());
	}
}

void BuzzerLightComposer::CreateTxBuzzReward::saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	composer_->writeBuzzerUtxo(utxo);
	utxoByBuzzerLoaded(utxo, lMyBuzzer->myName());
}

void BuzzerLightComposer::CreateTxBuzzReward::utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	TxBuzzRewardPtr lTx = TransactionHelper::to<TxBuzzReward>(TransactionFactory::create(TX_BUZZ_REWARD));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lTx);
	//
	lTx->setTimestamp(qbit::getMedianMicroseconds());
	lTx->setChain(chain_);
	lTx->setScore(composer_->buzzer()->score());
	lTx->setBuzzerInfo(composer_->buzzerInfoTx()->id());
	lTx->setBuzzerInfoChain(composer_->buzzerInfoTx()->chain());

	// get buzzer tx (saved/cached)
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	if (lMyBuzzer) {
		SKeyPtr lSKey = composer_->wallet()->firstKey();
		//
		if (buzzUtxo_.size() > TX_BUZZ_REWARD_OUT && utxo.size() > TX_BUZZER_MY_OUT) {
			// add my byzzer in
			lTx->addMyBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(const_cast<Transaction::UnlinkedOut&>(utxo[TX_BUZZER_MY_OUT])));
			// add buzz/event in
			lTx->addBuzzRewardIn(*lSKey, Transaction::UnlinkedOut::instance(buzzUtxo_[TX_BUZZ_REWARD_OUT].utxo()));

			// create spen tx
			TransactionContextPtr lReward;
			try {
				lReward = composer_->wallet()->createTxSpend(TxAssetType::qbitAsset(), dest_, reward_); // size-only, without ratings
				if (!lReward) { error_("E_TX_CREATE", "Transaction creation error."); return; }
			}
			catch(qbit::exception& ex) {
				error_(ex.code(), ex.what()); return;
			}
			catch(std::exception& ex) {
				error_("E_TX_CREATE", ex.what()); return;
			}

			// set reward tx
			lTx->setRewardTx(lReward->tx()->id());
			// set reward amount
			lTx->setAmount(reward_);
			// sign
			lTx->makeSignature(*lSKey, buzzUtxo_[TX_BUZZ_REWARD_OUT].utxo().out().tx());

			// push linked tx, which need to be pushed and broadcasted before this
			lCtx->addLinkedTx(lReward);

			// finalize
			if (!lTx->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

			created_(lCtx);
		} else {
			error_("E_BUZZ_UTXO_ABSENT", "Buzz utxo was not found."); return;
		}
	} else {
		error_("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found."); return;
	}
}

//
// CreateTxBuzzReply
//
void BuzzerLightComposer::CreateTxBuzzReply::process(errorFunction error) {
	//
	error_ = error;
	buzzUtxo_.clear();

	// 
	if (!composer_->requestProcessor()->selectUtxoByTransaction(chain_, buzz_, 
		SelectUtxoByTransaction::instance(
			boost::bind(&BuzzerLightComposer::CreateTxBuzzReply::utxoByBuzzLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::CreateTxBuzzReply::timeout, shared_from_this()))
	)) { error_("E_LOAD_UTXO_BY_BUZZ", "Buzz loading failed."); return; }
}

void BuzzerLightComposer::CreateTxBuzzReply::utxoByBuzzLoaded(const std::vector<Transaction::NetworkUnlinkedOut>& utxo, const uint256& tx) {
	//
	buzzUtxo_ = utxo;

	// create empty tx
	buzzTx_ = TransactionHelper::to<TxBuzzReply>(TransactionFactory::create(TX_BUZZ_REPLY));
	// create context
	ctx_ = TransactionContext::instance(buzzTx_);
	// get buzzer tx (saved/cached)
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	// check
	if (!composer_->buzzerInfoTx()) {
		error_("E_BUZZER_INFO_ABSENT", "Buzzer info is absent."); return;
	}

	if (lMyBuzzer) {
		// extract bound shard
		if (lMyBuzzer->in().size() > 1) {
			//
			SKeyPtr lSKey = composer_->wallet()->firstKey();
			if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }
			// set info
			buzzTx_->setBuzzerInfo(composer_->buzzerInfoTx()->id());
			// set info chain
			buzzTx_->setBuzzerInfoChain(composer_->buzzerInfoTx()->chain());
			// score
			buzzTx_->setScore(composer_->buzzer()->score());
			// pointers
			buzzTx_->setMediaPointers(mediaPointers_);
			// set timestamp
			buzzTx_->setTimestamp(qbit::getMedianMicroseconds());
			// set body
			buzzTx_->setBody(body_, *lSKey);

			std::vector<Transaction::UnlinkedOut> lMyBuzzerUtxos = composer_->buzzerUtxo();
			if (!lMyBuzzerUtxos.size()) {
				composer_->requestProcessor()->selectUtxoByEntity(lMyBuzzer->myName(), 
					SelectUtxoByEntityName::instance(
						boost::bind(&BuzzerLightComposer::CreateTxBuzzReply::saveBuzzerUtxo, shared_from_this(), _1, _2),
						boost::bind(&BuzzerLightComposer::CreateTxBuzzReply::timeout, shared_from_this()))
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

void BuzzerLightComposer::CreateTxBuzzReply::saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	composer_->writeBuzzerUtxo(utxo);
	utxoByBuzzerLoaded(utxo, lMyBuzzer->myName());
}

void BuzzerLightComposer::CreateTxBuzzReply::utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	

	if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size() || buzzUtxo_.size() <= TX_BUZZ_REPLY_OUT) {
		error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;	
	}

	// in[0] - buzzer utxo for new buzz
	buzzTx_->addMyBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	// in[1] - buzz reply out
	buzzTx_->addBuzzIn(*lSKey, Transaction::UnlinkedOut::instance(buzzUtxo_[TX_BUZZ_REPLY_OUT].utxo()));
	// set shard
	buzzTx_->setChain(buzzUtxo_[TX_BUZZ_REPLY_OUT].utxo().out().chain());
	// reply out
	buzzTx_->addBuzzReplyOut(*lSKey, lPKey); // out[0]
	// re-buzz out
	buzzTx_->addReBuzzOut(*lSKey, lPKey); // out[1]
	// like out
	buzzTx_->addBuzzLikeOut(*lSKey, lPKey); // out[2]
	// reward out
	buzzTx_->addBuzzRewardOut(*lSKey, lPKey); // out[3]
	// NOTE: pin out is not need
	// buzzTx_->addBuzzPinOut(*lSKey, lPKey); // out[4]

	// check buzzers list
	if (!buzzers_.size()) {
		composer_->requestProcessor()->selectUtxoByEntityNames(buzzers_, 
			SelectUtxoByEntityNames::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzReply::utxoByBuzzersListLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzReply::timeout, shared_from_this()))
		);
	} else {
		utxoByBuzzersListLoaded(std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>());
	}
}

void BuzzerLightComposer::CreateTxBuzzReply::utxoByBuzzersListLoaded(const std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>& entityUtxos) {
	//
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();
	if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	//
	std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>& lEntityUtxos = const_cast<std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>&>(entityUtxos);
	for (std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>::iterator lEntity = lEntityUtxos.begin(); lEntity != lEntityUtxos.end(); lEntity++) {
		//
		// in[1] - @buzzer.out[TX_BUZZER_BUZZ_OUT] or @buzzer.out[TX_BUZZER_REPLY_OUT]
		// in[2] - @buzzer.out[TX_BUZZER_BUZZ_OUT] or @buzzer.out[TX_BUZZER_REPLY_OUT]	
		// ...
		if (lEntity->utxo().size() > TX_BUZZER_BUZZ_OUT) {
			buzzTx_->addBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(lEntity->utxo()[TX_BUZZER_BUZZ_OUT]));
		}
	}

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
		buzzTx_->addIn(*lSKey, *(lFeeUtxos.begin())); // qbit fee that was exact for fee - in
		buzzTx_->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFeeAmount); // to the miner
	} else {
		error_("E_FEE_UTXO_ABSENT", "Fee utxo for buzz was not found."); return;
	}

	// push linked tx, which need to be pushed and broadcasted before this
	ctx_->addLinkedTx(lFee);

	if (!buzzTx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

	created_(ctx_);
}

//
// CreateTxRebuzz
//
void BuzzerLightComposer::CreateTxRebuzz::process(errorFunction error) {
	//
	error_ = error;

	if (!composer_->requestProcessor()->loadTransaction(chain_, buzz_, 
			LoadTransaction::instance(
				boost::bind(&BuzzerLightComposer::CreateTxRebuzz::buzzLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxRebuzz::timeout, shared_from_this()))
		))	error_("E_BUZZ_NOT_LOADED", "Buzz not loaded.");
}

void BuzzerLightComposer::CreateTxRebuzz::buzzLoaded(TransactionPtr tx) {
	// create empty tx
	buzzTx_ = TransactionHelper::to<TxReBuzz>(TransactionFactory::create(TX_REBUZZ));
	// create context
	ctx_ = TransactionContext::instance(buzzTx_);
	// get buzzer tx (saved/cached)
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	// check
	if (!composer_->buzzerInfoTx()) {
		error_("E_BUZZER_INFO_ABSENT", "Buzzer info is absent."); return;
	}

	if (lMyBuzzer) {
		// extract bound shard
		if (lMyBuzzer->in().size() > 1) {
			//
			SKeyPtr lSKey = composer_->wallet()->firstKey();
			if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }
			//
			Transaction::In& lShardIn = *(++(lMyBuzzer->in().begin())); // second in
			uint256 lShardTx = lShardIn.out().tx();
			// set shard/chain
			buzzTx_->setChain(lShardTx); // our shard
			buzzTx_->setScore(composer_->buzzer()->score());
			buzzTx_->setBuzzerInfo(composer_->buzzerInfoTx()->id());
			buzzTx_->setBuzzerInfoChain(composer_->buzzerInfoTx()->chain());
			buzzTx_->setMediaPointers(mediaPointers_);
			// set timestamp
			buzzTx_->setTimestamp(qbit::getMedianMicroseconds());
			// set buzzId
			buzzTx_->setBuzzId(buzz_);
			// set buzzChainId
			buzzTx_->setBuzzChainId(chain_);
			// set body
			buzzTx_->setBody(body_, *lSKey);
			// original publisher
			if (tx && tx->in().size() > TX_BUZZ_MY_IN) {
				buzzTx_->setBuzzerId(tx->in()[TX_BUZZ_MY_IN].out().tx());
			} else {
				error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;
			}

			std::vector<Transaction::UnlinkedOut> lMyBuzzerUtxos = composer_->buzzerUtxo();
			if (!lMyBuzzerUtxos.size()) {
				composer_->requestProcessor()->selectUtxoByEntity(lMyBuzzer->myName(), 
					SelectUtxoByEntityName::instance(
						boost::bind(&BuzzerLightComposer::CreateTxRebuzz::saveBuzzerUtxo, shared_from_this(), _1, _2),
						boost::bind(&BuzzerLightComposer::CreateTxRebuzz::timeout, shared_from_this()))
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

void BuzzerLightComposer::CreateTxRebuzz::saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	composer_->writeBuzzerUtxo(utxo);
	utxoByBuzzerLoaded(utxo, lMyBuzzer->myName());
}

void BuzzerLightComposer::CreateTxRebuzz::utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	

	if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size()) {
		error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;	
	}

	// in[0] - buzzer utxo for new buzz
	buzzTx_->addMyBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	// reply out
	buzzTx_->addBuzzReplyOut(*lSKey, lPKey); // out[0]
	// re-buzz out
	buzzTx_->addReBuzzOut(*lSKey, lPKey); // out[1]
	// like out
	buzzTx_->addBuzzLikeOut(*lSKey, lPKey); // out[2]
	// reward out
	buzzTx_->addBuzzRewardOut(*lSKey, lPKey); // out[3]
	// pin out
	buzzTx_->addBuzzPinOut(*lSKey, lPKey); // out[4]

	// make "notify" to source chain
	notifyTx_ = TransactionHelper::to<TxReBuzzNotify>(TransactionFactory::create(TX_BUZZ_REBUZZ_NOTIFY));
	notifyTx_->addMyBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	notifyTx_->setChain(chain_); // source chain
	notifyTx_->setScore(composer_->buzzer()->score());
	notifyTx_->setTimestamp(qbit::getMedianMicroseconds());
	notifyTx_->setBuzzId(buzz_);
	notifyTx_->setBuzzChainId(chain_);
	notifyTx_->setBuzzerId(buzzTx_->buzzerId());
	ctx_->addLinkedTx(TransactionContext::instance(notifyTx_));

	// check buzzers list
	if (!buzzers_.size()) {
		composer_->requestProcessor()->selectUtxoByEntityNames(buzzers_, 
			SelectUtxoByEntityNames::instance(
				boost::bind(&BuzzerLightComposer::CreateTxRebuzz::utxoByBuzzersListLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxRebuzz::timeout, shared_from_this()))
		);
	} else {
		utxoByBuzzersListLoaded(std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>());
	}
}

void BuzzerLightComposer::CreateTxRebuzz::utxoByBuzzersListLoaded(const std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>& entityUtxos) {
	//
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();
	if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	//
	std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>& lEntityUtxos = const_cast<std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>&>(entityUtxos);
	for (std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>::iterator lEntity = lEntityUtxos.begin(); lEntity != lEntityUtxos.end(); lEntity++) {
		//
		// in[1] - @buzzer.out[TX_BUZZER_BUZZ_OUT] or @buzzer.out[TX_BUZZER_REPLY_OUT]
		// in[2] - @buzzer.out[TX_BUZZER_BUZZ_OUT] or @buzzer.out[TX_BUZZER_REPLY_OUT]	
		// ...
		if (lEntity->utxo().size() > TX_BUZZER_BUZZ_OUT) {
			buzzTx_->addBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(lEntity->utxo()[TX_BUZZER_BUZZ_OUT]));
		}
	}

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
		buzzTx_->addIn(*lSKey, *(lFeeUtxos.begin())); // qbit fee that was exact for fee - in
		buzzTx_->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFeeAmount); // to the miner
	} else {
		error_("E_FEE_UTXO_ABSENT", "Fee utxo for buzz was not found."); return;
	}

	// push linked tx, which need to be pushed and broadcasted before this
	ctx_->addLinkedTx(lFee);

	// finalize
	if (!buzzTx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

	// cross-link
	notifyTx_->setRebuzzChainId(buzzTx_->chain());
	notifyTx_->setRebuzzId(buzzTx_->id()); // tx is fully composed

	created_(ctx_);
}

//
// CreateTxBuzzerEndorse
//
void BuzzerLightComposer::CreateTxBuzzerEndorse::process(errorFunction error) {
	//
	error_ = error;
	publisherUtxo_.resize(0);
	publisherId_.setNull();

	// create empty tx
	buzzerEndorseTx_ = TransactionHelper::to<TxBuzzerEndorse>(TransactionFactory::create(TX_BUZZER_ENDORSE));
	// create context
	ctx_ = TransactionContext::instance(buzzerEndorseTx_);
	// get buzzer tx (saved/cached)
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();

	if (lMyBuzzer) {
		// set timestamp
		buzzerEndorseTx_->setTimestamp(qbit::getMedianMicroseconds());
		buzzerEndorseTx_->setBuzzerInfo(composer_->buzzerInfoTx()->id());
		buzzerEndorseTx_->setBuzzerInfoChain(composer_->buzzerInfoTx()->chain());

		composer_->requestProcessor()->loadEntity(publisher_, 
			LoadEntity::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerEndorse::publisherLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerEndorse::timeout, shared_from_this()))
		);
	} else {
		error_("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzerEndorse::publisherLoaded(EntityPtr publisher) {
	//
	if (!publisher) { error_("E_BUZZER_NOT_FOUND", "Buzzer not found."); return; }

	//
	publisherId_ = publisher->id();	

	//
	// extract bound shard
	if (publisher->in().size() > 1) {
		//
		Transaction::In& lShardIn = *(++(publisher->in().begin())); // second in
		// set shard/chain
		buzzerEndorseTx_->setChain(lShardIn.out().tx());
		// score
		buzzerEndorseTx_->setScore(composer_->buzzer()->score());

		// check endorsement
		composer_->buzzerRequestProcessor()->selectBuzzerEndorse(lShardIn.out().tx(), composer_->buzzerId(), publisher->id(),
			LoadEndorse::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerEndorse::endorseTxLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerEndorse::timeout, shared_from_this()))
		);
	} else {
		error_("E_BUZZER_PUBLISHER_INCONSISTENT", "Publisher is inconsistent."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzerEndorse::endorseTxLoaded(const uint256& tx) {
	//
	if (tx.isNull()) {
		composer_->requestProcessor()->selectUtxoByEntity(publisher_, 
			SelectUtxoByEntityName::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerEndorse::utxoByPublisherLoaded, shared_from_this(), _1, _2),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerEndorse::timeout, shared_from_this()))
		);
	} else {
		error_("E_ENDORSED_ALREDY", "Buzzer already endorsed."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzerEndorse::utxoByPublisherLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKeyPtr lSChangeKey = composer_->wallet()->changeKey();
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	

	if (!lSKey->valid() || !lSChangeKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size()) {
		error_("E_BUZZER_UTXO_INCONSISTENT", "Publisher utxo is inconsistent."); return;	
	}

	publisherUtxo_.insert(publisherUtxo_.end(), utxo.begin(), utxo.end());

	std::vector<Transaction::UnlinkedOut> lMyBuzzerUtxos = composer_->buzzerUtxo();
	if (!lMyBuzzerUtxos.size()) {
		composer_->requestProcessor()->selectUtxoByEntity(composer_->buzzerTx()->myName(), 
			SelectUtxoByEntityName::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerEndorse::saveBuzzerUtxo, shared_from_this(), _1, _2),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerEndorse::timeout, shared_from_this()))
		);
	} else {
		utxoByBuzzerLoaded(lMyBuzzerUtxos, composer_->buzzerTx()->myName());
	}
}

void BuzzerLightComposer::CreateTxBuzzerEndorse::saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	composer_->writeBuzzerUtxo(utxo);
	utxoByBuzzerLoaded(utxo, lMyBuzzer->myName());
}

void BuzzerLightComposer::CreateTxBuzzerEndorse::utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKeyPtr lSChangeKey = composer_->wallet()->changeKey();
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	

	if (!lSKey->valid() || !lSChangeKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size()) {
		error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;	
	}

	if (utxo.size() > 1) {
		buzzerEndorseTx_->addMyBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	} else { error_("E_SUBSCRIBER_BUZZER_INCORRECT", "My buzzer outs is incorrect."); return; }

	if (publisherUtxo_.size() > 1) {
		buzzerEndorseTx_->addPublisherBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(publisherUtxo_[TX_BUZZER_ENDORSE_OUT])); //
	} else { error_("E_PUBLISHER_BUZZER_INCORRECT", "Publisher buzzer outs is incorrect."); return; }

	// prepare fee tx
	amount_t lFeeAmount = ctx_->size();
	amount_t lLockedAmount = points_ /*endorsement points in BITS ot UNITS*/;
	TransactionContextPtr lFee;

	try {
		uint64_t lHeight = composer_->requestProcessor()->locateHeight(MainChain::id()); // qbits - lives in main chain
		lHeight += TxBuzzerEndorse::calcLockHeight(composer_->settings()->mainChainBlockTime());

		lFee = composer_->wallet()->createTxFeeLockedChange(lPKey, lFeeAmount, lLockedAmount, lHeight + 5 /*delta*/);
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
		buzzerEndorseTx_->addIn(*lSKey, *(lFeeUtxos.begin())); // qbit fee that was exact for fee - in
		buzzerEndorseTx_->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFeeAmount); // to the miner

		// just for info
		buzzerEndorseTx_->setAmount(lLockedAmount);
		buzzerEndorseTx_->makeSignature(*lSKey, publisherId_);
	} else {
		error_("E_FEE_UTXO_ABSENT", "Fee utxo for buzz was not found."); return;
	}

	// push linked tx, which need to be pushed and broadcasted before this
	ctx_->addLinkedTx(lFee);

	if (!buzzerEndorseTx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

	created_(ctx_);
}

//
// CreateTxBuzzerMistrust
//
void BuzzerLightComposer::CreateTxBuzzerMistrust::process(errorFunction error) {
	//
	error_ = error;
	publisherUtxo_.resize(0);
	publisherId_.setNull();

	// create empty tx
	buzzerMistrustTx_ = TransactionHelper::to<TxBuzzerMistrust>(TransactionFactory::create(TX_BUZZER_MISTRUST));
	// create context
	ctx_ = TransactionContext::instance(buzzerMistrustTx_);
	// get buzzer tx (saved/cached)
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();

	if (lMyBuzzer) {
		// set timestamp
		buzzerMistrustTx_->setTimestamp(qbit::getMedianMicroseconds());
		buzzerMistrustTx_->setBuzzerInfo(composer_->buzzerInfoTx()->id());
		buzzerMistrustTx_->setBuzzerInfoChain(composer_->buzzerInfoTx()->chain());

		composer_->requestProcessor()->loadEntity(publisher_, 
			LoadEntity::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerMistrust::publisherLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerMistrust::timeout, shared_from_this()))
		);
	} else {
		error_("E_BUZZER_TX_NOT_FOUND", "Local buzzer was not found."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzerMistrust::publisherLoaded(EntityPtr publisher) {
	//
	if (!publisher) { error_("E_BUZZER_NOT_FOUND", "Buzzer not found."); return; }

	//
	publisherId_ = publisher->id();

	//
	// extract bound shard
	if (publisher->in().size() > 1) {
		//
		Transaction::In& lShardIn = *(++(publisher->in().begin())); // second in
		// set shard/chain
		buzzerMistrustTx_->setChain(lShardIn.out().tx());
		// score
		buzzerMistrustTx_->setScore(composer_->buzzer()->score());

		// check endorsement
		composer_->buzzerRequestProcessor()->selectBuzzerMistrust(lShardIn.out().tx(), composer_->buzzerId(), publisher->id(),
			LoadMistrust::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerMistrust::mistrustTxLoaded, shared_from_this(), _1),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerMistrust::timeout, shared_from_this()))
		);
	} else {
		error_("E_BUZZER_PUBLISHER_INCONSISTENT", "Publisher is inconsistent."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzerMistrust::mistrustTxLoaded(const uint256& tx) {
	//
	if (tx.isNull()) {
		composer_->requestProcessor()->selectUtxoByEntity(publisher_, 
			SelectUtxoByEntityName::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerMistrust::utxoByPublisherLoaded, shared_from_this(), _1, _2),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerMistrust::timeout, shared_from_this()))
		);
	} else {
		error_("E_MISTRUSTED_ALREDY", "Buzzer already mistrusted."); return;
	}
}

void BuzzerLightComposer::CreateTxBuzzerMistrust::utxoByPublisherLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKeyPtr lSChangeKey = composer_->wallet()->changeKey();
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	

	if (!lSKey->valid() || !lSChangeKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size()) {
		error_("E_BUZZER_UTXO_INCONSISTENT", "Publisher utxo is inconsistent."); return;	
	}

	publisherUtxo_.insert(publisherUtxo_.end(), utxo.begin(), utxo.end());

	std::vector<Transaction::UnlinkedOut> lMyBuzzerUtxos = composer_->buzzerUtxo();
	if (!lMyBuzzerUtxos.size()) {
		composer_->requestProcessor()->selectUtxoByEntity(composer_->buzzerTx()->myName(), 
			SelectUtxoByEntityName::instance(
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerMistrust::saveBuzzerUtxo, shared_from_this(), _1, _2),
				boost::bind(&BuzzerLightComposer::CreateTxBuzzerMistrust::timeout, shared_from_this()))
		);
	} else {
		utxoByBuzzerLoaded(lMyBuzzerUtxos, composer_->buzzerTx()->myName());
	}
}

void BuzzerLightComposer::CreateTxBuzzerMistrust::saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	TxBuzzerPtr lMyBuzzer = composer_->buzzerTx();
	composer_->writeBuzzerUtxo(utxo);
	utxoByBuzzerLoaded(utxo, lMyBuzzer->myName());
}

void BuzzerLightComposer::CreateTxBuzzerMistrust::utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>& utxo, const std::string& buzzer) {
	//
	SKeyPtr lSChangeKey = composer_->wallet()->changeKey();
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	

	if (!lSKey->valid() || !lSChangeKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	if (!utxo.size()) {
		error_("E_BUZZER_TX_INCONSISTENT", "Buzzer tx is inconsistent."); return;	
	}

	if (utxo.size() > 1) {
		buzzerMistrustTx_->addMyBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(*(utxo.begin()))); // first out
	} else { error_("E_SUBSCRIBER_BUZZER_INCORRECT", "My buzzer outs is incorrect."); return; }

	if (publisherUtxo_.size() > 1) {
		buzzerMistrustTx_->addPublisherBuzzerIn(*lSKey, Transaction::UnlinkedOut::instance(publisherUtxo_[TX_BUZZER_MISTRUST_OUT])); //
	} else { error_("E_PUBLISHER_BUZZER_INCORRECT", "Publisher buzzer outs is incorrect."); return; }

	// prepare fee tx
	amount_t lFeeAmount = ctx_->size();
	amount_t lLockedAmount = points_ /*endorsement points in BITS ot UNITS*/;
	TransactionContextPtr lFee;

	try {
		uint64_t lHeight = composer_->requestProcessor()->locateHeight(MainChain::id()); // qbits - lives in main chain
		lHeight += TxBuzzerMistrust::calcLockHeight(composer_->settings()->mainChainBlockTime());

		lFee = composer_->wallet()->createTxFeeLockedChange(lPKey, lFeeAmount, lLockedAmount, lHeight + 5 /*delta*/);
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
		buzzerMistrustTx_->addIn(*lSKey, *(lFeeUtxos.begin())); // qbit fee that was exact for fee - in
		buzzerMistrustTx_->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFeeAmount); // to the miner

		// just for info
		buzzerMistrustTx_->setAmount(lLockedAmount);
		buzzerMistrustTx_->makeSignature(*lSKey, publisherId_);
	} else {
		error_("E_FEE_UTXO_ABSENT", "Fee utxo for buzz was not found."); return;
	}

	// push linked tx, which need to be pushed and broadcasted before this
	ctx_->addLinkedTx(lFee);

	if (!buzzerMistrustTx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

	created_(ctx_);
}

//
// LoadBuzzerInfo
//
void BuzzerLightComposer::LoadBuzzerInfo::process(errorFunction error) {
	//
	error_ = error;

	if (!composer_->buzzerRequestProcessor()->loadBuzzerAndInfo(buzzer_, 
		LoadBuzzerAndInfo::instance(
			boost::bind(&BuzzerLightComposer::LoadBuzzerInfo::transactionLoaded, shared_from_this(), _1, _2),
			boost::bind(&BuzzerLightComposer::LoadBuzzerInfo::timeout, shared_from_this()))
	)) error_("E_LOAD_BUZZER", "Buzzer load failed.");
}

void BuzzerLightComposer::LoadBuzzerInfo::transactionLoaded(EntityPtr buzzer, TransactionPtr info) {
	//
	if (!buzzer) { error_("E_BUZZER_NOT_FOUND", "Buzzer not found."); return; }

	//
	loaded_(buzzer, info, buzzer_);
}
