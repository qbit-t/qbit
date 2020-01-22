#include "wallet.h"
#include "mkpath.h"

using namespace qbit;

SKey Wallet::createKey(const std::list<std::string>& seed) {
	SKey lSKey(seed);
	lSKey.create();

	PKey lPKey = lSKey.createPKey();

	keys_.open(settings_->keysCache());
	if (!keys_.read(lPKey.id(), lSKey))
		if (!keys_.write(lPKey.id(), lSKey)) { 
			throw qbit::exception("WRITE_FAILED", "Write failed."); 
		}

	return lSKey;
}

SKey Wallet::findKey(const PKey& pkey) {
	if (pkey.valid()) {
		SKey lSKey;
		if (keys_.read(pkey.id(), lSKey)) {
			return lSKey;
		}
	}

	return SKey();
}

SKey Wallet::firstKey() {
	db::DbContainer<uint160 /*id*/, SKey>::Iterator lKey = keys_.begin();
	if (lKey.valid()) {
		return *lKey;
	}

	// TODO: create key silently from seed words
	return SKey();
}

SKey Wallet::changeKey() {
	PKey lChangeKey = settings_->changeKey();
	if (lChangeKey.valid()) {
		return findKey(lChangeKey);
	}

	return firstKey();
}

bool Wallet::open() {
	if (!opened_) {
		try {
			if (mkpath(std::string(settings_->dataPath() + "/wallet").c_str(), 0777)) return false;

			keys_.open();
			utxo_.open();
			assets_.open();
			entities_.open();
		}
		catch(const std::exception& ex) {
			gLog().write(Log::ERROR, std::string("[wallet/open]: ") + ex.what());
			return false;
		}

		return prepareCache();
	}

	return true;
}

bool Wallet::close() {
	gLog().write(Log::INFO, std::string("[wallet/close]: closing wallet data..."));

	settings_.reset();
	mempoolManager_.reset();
	mempool_.reset();
	entityStore_.reset();
	persistentStoreManager_.reset();
	persistentStore_.reset();

	keys_.close();
	utxo_.close();
	assets_.close();
	entities_.close();

	return true;
}

bool Wallet::prepareCache() {
	try {
		// scan assets, check utxo and fill balance
		db::DbMultiContainer<uint256 /*asset*/, uint256 /*utxo*/>::Transaction lAssetTransaction = assets_.transaction();
		// 
		db::DbContainer<uint256 /*utxo*/, Transaction::UnlinkedOut /*data*/>::Transaction lUtxoTransaction = utxo_.transaction();

		for (db::DbMultiContainer<uint256 /*asset*/, uint256 /*utxo*/>::Iterator lAsset = assets_.begin(); lAsset.valid(); ++lAsset) {
			Transaction::UnlinkedOut lUtxo;
			uint256 lUtxoId = *lAsset; // utxo hash
			uint256 lAssetId;
			lAsset.first(lAssetId); // asset hash

			if (utxo_.read(lUtxoId, lUtxo)) {
				// if utxo exists
				// TODO: all chains?
				if (isUnlinkedOutExists(lUtxoId)) {
					// utxo exists
					Transaction::UnlinkedOutPtr lUtxoPtr = Transaction::UnlinkedOut::instance(lUtxo);
					
					// cache it
					cacheUtxo(lUtxoPtr);

					// calc balance
					if (balanceCache_.find(lAssetId) == balanceCache_.end()) {
						balanceCache_[lAssetId] = lUtxo.amount();
					} else {
						balanceCache_[lAssetId] += lUtxo.amount();
					}

					// make map
					assetsCache_[lAssetId].insert(std::multimap<amount_t, uint256>::value_type(lUtxo.amount(), lUtxoId));
				} else {
					lUtxoTransaction.remove(lUtxoId);
				}
			} else {
				// mark to remove: absent entry
				lAssetTransaction.remove(lAsset);
			}
		}

		// make changes to assets
		lAssetTransaction.commit();

		// entity tx
		db::DbMultiContainer<uint256 /*asset*/, uint256 /*utxo*/>::Transaction lEntityTransaction = entities_.transaction();

		// scan entities
		for (db::DbMultiContainer<uint256 /*entity*/, uint256 /*utxo*/>::Iterator lEntity = entities_.begin(); lEntity.valid(); ++lEntity) {
			uint256 lUtxoId = *lEntity; // utxo hash
			uint256 lEntityId;
			lEntity.first(lEntityId); // entity hash

			Transaction::UnlinkedOutPtr lUtxoPtr = findUnlinkedOut(lUtxoId);
			if (lUtxoPtr) entitiesCache_[lEntityId][lUtxoId] = lUtxoPtr;
			else {
				Transaction::UnlinkedOut lUtxo;
				if (utxo_.read(lEntityId, lUtxo)) {
					// if utxo exists
					// TODO: all chains?
					if (isUnlinkedOutExists(lUtxoId)) {

						// utxo exists
						lUtxoPtr = Transaction::UnlinkedOut::instance(lUtxo);
						
						// cache it
						cacheUtxo(lUtxoPtr);
						entitiesCache_[lEntityId][lUtxoId] = lUtxoPtr;
					} else {
						lUtxoTransaction.remove(lEntityId);
					}
				} else {
					// mark to remove: absent entry
					lEntityTransaction.remove(lEntity);				
				}
			}
		}

		// make changes to utxo
		lUtxoTransaction.commit();

		// make changes to entities
		lEntityTransaction.commit();		

		opened_ = true;
	}
	catch(const std::exception& ex) {
		gLog().write(Log::ERROR, std::string("[wallet/prepareCache]: ") + ex.what());
		opened_ = true; // half-opened
	}

	return opened_;
}

bool Wallet::pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr ctx) {
	uint256 lUtxoId = utxo->hash();
	// TODO: all chains?
	if (!findUnlinkedOut(lUtxoId) && !isUnlinkedOutUsed(lUtxoId)) {
		// cache it
		cacheUtxo(utxo);

		// update utxo db
		utxo_.write(lUtxoId, *utxo);

		// preserve in tx context for rollback
		ctx->addNewUnlinkedOut(utxo);

		uint256 lAssetId = utxo->out().asset();
		if (ctx->tx()->isValue(utxo)) {
			assetsCache_[lAssetId].insert(std::multimap<amount_t, uint256>::value_type(utxo->amount(), lUtxoId));

			// update assets db
			assets_.write(lAssetId, lUtxoId);

			// balance
			std::map<uint256 /*asset*/, amount_t /*balance*/>::iterator lBalance = balanceCache_.find(lAssetId);
			if (lBalance != balanceCache_.end())
				balanceCache_[lAssetId] = lBalance->second + utxo->amount();
			else
				balanceCache_[lAssetId] = utxo->amount();
		} else if (ctx->tx()->isEntity(utxo)) {
			// update cache
			entitiesCache_[lAssetId][lUtxoId] = utxo;

			// update db
			entities_.write(lAssetId, lUtxoId);
		}

		return true;
	}

	return false;
}

bool Wallet::popUnlinkedOut(const uint256& hash, TransactionContextPtr ctx) {
	Transaction::UnlinkedOutPtr lUtxoPtr = findUnlinkedOut(hash);
	if (lUtxoPtr) {

		// correct balance
		uint256 lAssetId = lUtxoPtr->out().asset();
		std::map<uint256 /*asset*/, amount_t /*balance*/>::iterator lBalance = balanceCache_.find(lAssetId);
		if (lBalance != balanceCache_.end()) {
			if (lBalance->second > lUtxoPtr->amount()) {
				balanceCache_[lAssetId] = lBalance->second - lUtxoPtr->amount();
			} else {
				balanceCache_.erase(lAssetId);
			}
		}

		// preserve in tx context for rollback
		ctx->addUsedUnlinkedOut(lUtxoPtr);

		// try entities
		std::map<uint256, std::map<uint256, Transaction::UnlinkedOutPtr>>::iterator lEntity = entitiesCache_.find(lAssetId);
		if (lEntity != entitiesCache_.end()) {
			lEntity->second.erase(hash);
		}

		// remove entry
		removeUtxo(hash);
		utxo_.remove(hash);

		// assets_ entries is not cleaned at this point
		// this index maintened on startup only
		// TODO: make more robast index maintenance
		return true;
	}
	return false;
}

Transaction::UnlinkedOutPtr Wallet::findUnlinkedOut(const uint256& hash) {
	
	if (useUtxoCache_) {
		std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lUtxo = utxoCache_.find(hash);
		if (lUtxo != utxoCache_.end()) {
			// TODO: all chains?
			if (isUnlinkedOutExists(hash))
				return lUtxo->second;
		}
	} else {
		Transaction::UnlinkedOut lUtxo;
		if (utxo_.read(hash, lUtxo) && isUnlinkedOutExists(hash)) {
			// utxo exists
			return Transaction::UnlinkedOut::instance(lUtxo);
		}
	}

	return nullptr;
}

Transaction::UnlinkedOutPtr Wallet::findUnlinkedOutByAsset(const uint256& asset, amount_t amount) {
	std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(asset);

	if (lAsset != assetsCache_.end()) {
		for (std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator lAmount = lAsset->second.begin(); 
			lAmount != lAsset->second.end(); lAmount++) {
			
			if (lAmount->first >= amount) {
				Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(lAmount->second);

				if (lUtxo == nullptr) {
					// delete from store
					utxo_.remove(lAmount->second);
					lAsset->second.erase(lAmount);
				} else {
					return lUtxo;
				}
			}
		}
	}

	return nullptr;
}

std::list<Transaction::UnlinkedOutPtr> Wallet::collectUnlinkedOutsByAsset(const uint256& asset, amount_t amount) {
	std::list<Transaction::UnlinkedOutPtr> lList;
	std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(asset);

	amount_t lTotal = 0;
	if (lAsset != assetsCache_.end()) {
		for (std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::reverse_iterator lAmount = lAsset->second.rbegin(); 
			lAmount != lAsset->second.rend();) {
			
			Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(lAmount->second);
			if (lUtxo == nullptr) {
				// delete from store
				utxo_.remove(lAmount->second);
				lAsset->second.erase(std::next(lAmount).base());
				continue;
			}

			lTotal += lAmount->first;
			lList.push_back(lUtxo);

			if (lTotal >= amount) {
				break;
			}

			lAmount++;
		}
	}

	if (lTotal < amount) lList.clear(); // amount is unreachable
	return lList;
}

// clean-up assets utxo
void Wallet::cleanUp() {
	utxoCache_.clear();
	assetsCache_.clear();
	balanceCache_.clear();

	prepareCache();
}

void Wallet::cleanUpData() {
	utxo_.close();
	assets_.close();
	entities_.close();

	// clean up
	std::string lPath = settings_->dataPath() + "/wallet/utxo";
	rmpath(lPath.c_str());
	lPath = settings_->dataPath() + "/wallet/utxo";
	rmpath(lPath.c_str());
	lPath = settings_->dataPath() + "/wallet/utxo";
	rmpath(lPath.c_str());

	// re-open
	try {
		utxo_.open();
		assets_.open();
		entities_.open();
	}
	catch(const std::exception& ex) {
		gLog().write(Log::ERROR, std::string("[wallet/cleanUpData]: ") + ex.what());
	}
}

// dump utxo by asset
void Wallet::dumpUnlinkedOutByAsset(const uint256& asset, std::stringstream& s) {
	std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(asset);

	if (lAsset != assetsCache_.end()) {
		for (std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::reverse_iterator lAmount = lAsset->second.rbegin(); 
			lAmount != lAsset->second.rend(); lAmount++) {
			
			Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(lAmount->second);
			if (lUtxo != nullptr) {
				s << lUtxo->out().toString() << ", amount = " << lUtxo->amount() << "\n";
			}
		}
	}
}

// rollback tx
bool Wallet::rollback(TransactionContextPtr ctx) {
	//
	// recover new utxos
	for (std::list<Transaction::UnlinkedOutPtr>::iterator lUtxo = ctx->newUtxo().begin(); lUtxo != ctx->newUtxo().end(); lUtxo++) {
		// locate utxo
		uint256 lUtxoId = (*lUtxo)->hash();
		Transaction::UnlinkedOutPtr lUtxoPtr = findUnlinkedOut(lUtxoId);
		if (lUtxoPtr) {

			// correct balance
			uint256 lAssetId = lUtxoPtr->out().asset();
			std::map<uint256 /*asset*/, amount_t /*balance*/>::iterator lBalance = balanceCache_.find(lAssetId);
			if (lBalance != balanceCache_.end()) {
				if (lBalance->second > lUtxoPtr->amount()) {
					balanceCache_[lAssetId] = lBalance->second - lUtxoPtr->amount();
				} else {
					balanceCache_.erase(lAssetId);
				}
			}

			// try entities
			std::map<uint256, std::map<uint256, Transaction::UnlinkedOutPtr>>::iterator lEntity = entitiesCache_.find(lAssetId);
			if (lEntity != entitiesCache_.end()) {
				lEntity->second.erase(lUtxoId);
			}

			// remove entry
			removeUtxo(lUtxoId);
			utxo_.remove(lUtxoId);
		}
	}
	ctx->newUtxo().clear();

	// recover used utxos
	for (std::list<Transaction::UnlinkedOutPtr>::iterator lUtxo = ctx->usedUtxo().begin(); lUtxo != ctx->usedUtxo().end(); lUtxo++) {
		// locate utxo
		uint256 lUtxoId = (*lUtxo)->hash();
		uint256 lAssetId = (*lUtxo)->out().asset();

		if (!findUnlinkedOut(lUtxoId)) {
			// recover
			utxo_.write(lUtxoId, *(*lUtxo));
			cacheUtxo((*lUtxo));

			if (ctx->tx()->isValue(*lUtxo)) {
				// extract asset
				std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(lAssetId);

				// check assets cache
				std::pair<std::multimap<amount_t, uint256>::iterator,
							std::multimap<amount_t, uint256>::iterator> lRange = lAsset->second.equal_range((*lUtxo)->amount());

				bool lFound = false;
				for (std::multimap<amount_t, uint256>::iterator lAmount = lRange.first; lAmount != lRange.second; lAmount++) {
					if (lAmount->second == lUtxoId) { lFound = true; break; }
				}

				if (!lFound) {
					assetsCache_[lAssetId].insert(std::multimap<amount_t, uint256>::value_type((*lUtxo)->amount(), lUtxoId));
				}

				// reconstruct balance
				// balance
				std::map<uint256 /*asset*/, amount_t /*balance*/>::iterator lBalance = balanceCache_.find(lAssetId);
				if (lBalance != balanceCache_.end()) {
					balanceCache_[lAssetId] = lBalance->second + (*lUtxo)->amount();
				} else {
					balanceCache_[lAssetId] = (*lUtxo)->amount();
				}

			} else if (ctx->tx()->isEntity(*lUtxo)) {
				// reconstruct entity
				std::map<uint256, std::map<uint256, Transaction::UnlinkedOutPtr>>::iterator lEntity = entitiesCache_.find(lAssetId);
				if (lEntity != entitiesCache_.end()) {
					lEntity->second.insert(std::map<uint256, Transaction::UnlinkedOutPtr>::value_type(lUtxoId, *lUtxo));
				}
			}
		}
	}
	ctx->usedUtxo().clear();
}

// fill inputs
amount_t Wallet::fillInputs(TxSpendPtr tx, const uint256& asset, amount_t amount) {
	// collect utxo's
	std::list<Transaction::UnlinkedOutPtr> lUtxos = collectUnlinkedOutsByAsset(asset, amount);
	if (!lUtxos.size()) throw qbit::exception("E_AMOUNT", "Insufficient amount.");

	// amount
	amount_t lAmount = 0;

	// fill ins 
	for (std::list<Transaction::UnlinkedOutPtr>::iterator lUtxo = lUtxos.begin(); lUtxo != lUtxos.end(); lUtxo++) {
		// locate skey
		SKey lSKey = findKey((*lUtxo)->address());
		if (lSKey.valid()) { 
			tx->addIn(lSKey, *lUtxo);
			lAmount += (*lUtxo)->amount();
		}
	}	

	return lAmount;
}

// make tx spend...
TransactionContextPtr Wallet::makeTxSpend(Transaction::Type type, const uint256& asset, const PKey& dest, amount_t amount, qunit_t feeRateLimit, int32_t targetBlock) {
	// create empty tx
	TxSpendPtr lTx = TransactionHelper::to<TxSpend>(TransactionFactory::create(type)); // TxSendPrivate -> TxSend
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lTx);
	// fill inputs
	amount_t lAmount = fillInputs(lTx, asset, amount);

	// fill output
	SKey lSKey = changeKey(); // TODO: do we need to specify exact skey?
	if (!lSKey.valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");
	lTx->addOut(lSKey, dest, asset, amount);

	// make change
	if (asset != TxAssetType::qbitAsset() && lAmount > amount) {
		// TODO: do we need ability to create new keys for change?
		lTx->addOut(lSKey, lSKey.createPKey()/*change*/, asset, lAmount - amount);			
	}

	// try to estimate fee
	qunit_t lRate = 0;
	if (targetBlock == -1) {
		lRate = mempool()->estimateFeeRateByLimit(lCtx, feeRateLimit);
	} else {
		lRate = mempool()->estimateFeeRateByBlock(lCtx, targetBlock);
	}

	amount_t lFee = lRate * lCtx->size(); 
	if (asset == TxAssetType::qbitAsset()) { // if qbit sending
		// try to check amount
		if (lAmount >= amount + lFee) {
			lTx->addFeeOut(lSKey, asset, lFee); // to miner

			if (lAmount > amount + lFee) { // make change
				// TODO: do we need ability to create new keys for change?
				lTx->addOut(lSKey, lSKey.createPKey()/*change*/, asset, lAmount - (amount + lFee));
			}
		} else { // rare cases
			return makeTxSpend(type, asset, dest, amount + lFee, feeRateLimit, targetBlock);
		}
	} else {
		// add fee
		amount_t lFeeAmount = fillInputs(lTx, TxAssetType::qbitAsset(), lFee);
		lTx->addFeeOut(lSKey, TxAssetType::qbitAsset(), lFee); // to miner
		if (lFeeAmount > lFee) { // make change
			// TODO: do we need ability to create new keys for change?
			lTx->addOut(lSKey, lSKey.createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
		}
	}

	if (!lTx->finalize(lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finlization failed.");

	return lCtx;
}

// create spend tx
TransactionContextPtr Wallet::createTxSpend(const uint256& asset, const PKey& dest, amount_t amount, qunit_t feeRateLimit, int32_t targetBlock) {
	return makeTxSpend(Transaction::SPEND, asset, dest, amount, feeRateLimit, targetBlock);
}

TransactionContextPtr Wallet::createTxSpend(const uint256& asset, const PKey& dest, amount_t amount) {
	qunit_t lRate = settings_->maxFeeRate();
	return makeTxSpend(Transaction::SPEND, asset, dest, amount, lRate, -1);
}

// create spend private tx
TransactionContextPtr Wallet::createTxSpendPrivate(const uint256& asset, const PKey& dest, amount_t amount, qunit_t feeRateLimit, int32_t targetBlock) {
	return makeTxSpend(Transaction::SPEND_PRIVATE, asset, dest, amount, feeRateLimit, targetBlock);	
}

TransactionContextPtr Wallet::createTxSpendPrivate(const uint256& asset, const PKey& dest, amount_t amount) {
	qunit_t lRate = settings_->maxFeeRate();
	return makeTxSpend(Transaction::SPEND_PRIVATE, asset, dest, amount, lRate, -1);	
}

TransactionContextPtr Wallet::createTxAssetType(const PKey& dest, const std::string& shortName, const std::string& longName, amount_t supply, amount_t scale, int chunks, TxAssetType::Emission emission, qunit_t feeRateLimit, int32_t targetBlock) {
	// create empty asset type
	TxAssetTypePtr lAssetTypeTx = TransactionHelper::to<TxAssetType>(TransactionFactory::create(Transaction::ASSET_TYPE));
	lAssetTypeTx->setShortName(shortName);
	lAssetTypeTx->setLongName(longName);
	lAssetTypeTx->setSupply(supply * chunks * scale);
	lAssetTypeTx->setScale(scale);
	lAssetTypeTx->setEmission(emission);

	SKey lSKey = changeKey(); // TODO: do we need to specify exact skey?
	if (!lSKey.valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	if (emission == TxAssetType::LIMITED) {
		for (int lIdx = 0; lIdx < chunks; lIdx++)
			lAssetTypeTx->addLimitedOut(lSKey, dest, supply * scale);
	}

	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lAssetTypeTx);

	// try to estimate fee
	qunit_t lRate = 0;
	if (targetBlock == -1) {
		lRate = mempool()->estimateFeeRateByLimit(lCtx, feeRateLimit);
	} else {
		lRate = mempool()->estimateFeeRateByBlock(lCtx, targetBlock);
	}

	amount_t lFee = lRate * lCtx->size(); 	
	amount_t lFeeAmount = fillInputs(lAssetTypeTx, TxAssetType::qbitAsset(), lFee);
	lAssetTypeTx->addFeeOut(lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		// TODO: do we need ability to create new keys for change?
		lAssetTypeTx->addOut(lSKey, lSKey.createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
	}

	if (!lAssetTypeTx->finalize(lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finlization failed.");
	
	return lCtx;
}

TransactionContextPtr Wallet::createTxAssetType(const PKey& dest, const std::string& shortName, const std::string& longName, amount_t supply, amount_t scale, int chunks, TxAssetType::Emission emission) {
	qunit_t lRate = settings_->maxFeeRate();
	return createTxAssetType(dest, shortName, longName, supply, scale, emission, lRate, -1);
}

TransactionContextPtr Wallet::createTxAssetType(const PKey& dest, const std::string& shortName, const std::string& longName, amount_t supply, amount_t scale, TxAssetType::Emission emission, qunit_t feeRateLimit, int32_t targetBlock) {
	return createTxAssetType(dest, shortName, longName, supply, scale, 1, emission, feeRateLimit, targetBlock);
}

TransactionContextPtr Wallet::createTxAssetType(const PKey& dest, const std::string& shortName, const std::string& longName, amount_t supply, amount_t scale, TxAssetType::Emission emission) {
	qunit_t lRate = settings_->maxFeeRate();
	return createTxAssetType(dest, shortName, longName, supply, scale, 1, emission, lRate, -1);
}

Transaction::UnlinkedOutPtr Wallet::findUnlinkedOutByEntity(const uint256& entity) {
	std::map<uint256, std::map<uint256, Transaction::UnlinkedOutPtr>>::iterator lEntity = entitiesCache_.find(entity);
	if (lEntity != entitiesCache_.end() && lEntity->second.size()) {
		return lEntity->second.begin()->second;
	}

	return nullptr;
}

TransactionContextPtr Wallet::createTxLimitedAssetEmission(const PKey& dest, const uint256& asset, qunit_t feeRateLimit, int32_t targetBlock) {
	// create empty tx
	TxAssetEmissionPtr lAssetEmissionTx = TransactionHelper::to<TxAssetEmission>(TransactionFactory::create(Transaction::ASSET_EMISSION));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lAssetEmissionTx);
	// sanity check
	if (!entityStore_->locateEntity(asset)) throw qbit::exception("E_ENTITY", "Invalid entity.");

	// locate next utxo
	Transaction::UnlinkedOutPtr lUtxoPtr = findUnlinkedOutByEntity(asset);
	if (!lUtxoPtr) throw qbit::exception("E_ASSET_EMPTY", "Asset is empty.");

	SKey lSKey = changeKey(); // TODO: do we need to specify exact skey?
	if (!lSKey.valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	lAssetEmissionTx->addLimitedIn(lSKey, lUtxoPtr);
	lAssetEmissionTx->addOut(lSKey, lSKey.createPKey(), asset, lUtxoPtr->amount()); // each emission === utxo.amount()

	// try to estimate fee
	qunit_t lRate = 0;
	if (targetBlock == -1) {
		lRate = mempool()->estimateFeeRateByLimit(lCtx, feeRateLimit);
	} else {
		lRate = mempool()->estimateFeeRateByBlock(lCtx, targetBlock);
	}

	amount_t lFee = lRate * lCtx->size(); 	
	amount_t lFeeAmount = fillInputs(lAssetEmissionTx, TxAssetType::qbitAsset(), lFee);
	lAssetEmissionTx->addFeeOut(lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		// TODO: do we need ability to create new keys for change?
		lAssetEmissionTx->addOut(lSKey, lSKey.createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
	}

	if (!lAssetEmissionTx->finalize(lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finlization failed.");
	
	return lCtx;
}

TransactionContextPtr Wallet::createTxLimitedAssetEmission(const PKey& dest, const uint256& asset) {
	qunit_t lRate = settings_->maxFeeRate();
	return createTxLimitedAssetEmission(dest, asset, lRate, -1);
}

TransactionContextPtr Wallet::createTxCoinBase(amount_t amount) {
	// create empty tx
	TxCoinBasePtr lTx = TransactionHelper::to<TxCoinBase>(TransactionFactory::create(Transaction::COINBASE));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lTx);

	// add input
	lTx->addIn();

	SKey lSKey = changeKey(); // TODO: do we need to specify exact skey?
	if (!lSKey.valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	// make output
	Transaction::UnlinkedOutPtr lUtxoPtr = lTx->addOut(lSKey, lSKey.createPKey(), TxAssetType::qbitAsset(), amount);
	lUtxoPtr->out().setTx(lTx->id()); // finalize
	
	return lCtx;
}
