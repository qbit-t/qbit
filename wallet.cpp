#include "wallet.h"
#include "mkpath.h"
#include "txdapp.h"
#include "txshard.h"
#include "txbase.h"
#include "txblockbase.h"

using namespace qbit;

SKeyPtr Wallet::createKey(const std::list<std::string>& seed) {
	//
	SKey lSKey(seed);
	lSKey.create();

	PKey lPKey = lSKey.createPKey();

	keys_.open(settings_->keysCache());
	if (!keys_.read(lPKey.id(), lSKey))
		if (!keys_.write(lPKey.id(), lSKey)) { 
			throw qbit::exception("WRITE_FAILED", "Write failed."); 
		}

	SKeyPtr lSKeyPtr = SKey::instance(lSKey);
	{
		boost::unique_lock<boost::recursive_mutex> lLock(keyMutex_);
		keysCache_[lPKey.id()] = lSKeyPtr;
	}

	return lSKeyPtr;
}

SKeyPtr Wallet::findKey(const PKey& pkey) {
	if (pkey.valid()) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(keyMutex_);
		std::map<uint160 /*id*/, SKeyPtr>::iterator lSKeyIter = keysCache_.find(pkey.id());
		if (lSKeyIter != keysCache_.end())
			return lSKeyIter->second;

		SKey lSKey;
		if (keys_.read(pkey.id(), lSKey)) {
			SKeyPtr lSKeyPtr = SKey::instance(lSKey);
			keysCache_[pkey.id()] = lSKeyPtr;

			return lSKeyPtr;
		}
	}

	return nullptr;
}

SKeyPtr Wallet::firstKey() {
	//
	{
		boost::unique_lock<boost::recursive_mutex> lLock(keyMutex_);
		std::map<uint160 /*id*/, SKeyPtr>::iterator lSKeyIter = keysCache_.begin();
		if (lSKeyIter != keysCache_.end()) return lSKeyIter->second;
	}

	db::DbContainer<uint160 /*id*/, SKey>::Iterator lKey = keys_.begin();
	if (lKey.valid()) {
		SKeyPtr lSKeyPtr = SKey::instance(*lKey);
		uint160 lId;
		if (lKey.first(lId)) {
			boost::unique_lock<boost::recursive_mutex> lLock(keyMutex_);
			keysCache_[lId] = lSKeyPtr;
			return lSKeyPtr;
		}
	}	

	// try create from scratch
	return createKey(std::list<std::string>());
}

SKeyPtr Wallet::changeKey() {
	//
	PKey lChangeKey = settings_->changeKey();
	if (lChangeKey.valid()) {
		return findKey(lChangeKey);
	}

	// if use first key specified
	if (settings_->useFirstKeyForChange()) return firstKey();

	// try create from scratch
	return createKey(std::list<std::string>());
}

bool Wallet::open(const std::string& /*secret*/) {
	if (!opened_) {
		try {
			if (mkpath(std::string(settings_->dataPath() + "/wallet").c_str(), 0777)) return false;

			keys_.open();
			utxo_.open();
			ltxo_.open();
			assets_.open();
			assetEntities_.open();
			pendingtxs_.open();
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
	ltxo_.close();
	assets_.close();
	assetEntities_.close();
	pendingtxs_.close();

	return true;
}

bool Wallet::prepareCache() {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);

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
				if (isUnlinkedOutExistsGlobal(lUtxoId, lUtxo)) {
					// utxo exists
					Transaction::UnlinkedOutPtr lUtxoPtr = Transaction::UnlinkedOut::instance(lUtxo);
					
					// cache it
					cacheUtxo(lUtxoPtr);

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
		db::DbMultiContainer<uint256 /*asset*/, uint256 /*utxo*/>::Transaction lEntityTransaction = assetEntities_.transaction();

		// scan entities
		for (db::DbMultiContainer<uint256 /*entity*/, uint256 /*utxo*/>::Iterator lEntity = assetEntities_.begin(); lEntity.valid(); ++lEntity) {
			uint256 lUtxoId = *lEntity; // utxo hash
			uint256 lEntityId;
			lEntity.first(lEntityId); // entity hash

			Transaction::UnlinkedOutPtr lUtxoPtr = findUnlinkedOut(lUtxoId);
			if (lUtxoPtr) assetEntitiesCache_[lEntityId][lUtxoId] = lUtxoPtr;
			else {
				Transaction::UnlinkedOut lUtxo;
				if (utxo_.read(lEntityId, lUtxo)) {
					// if utxo exists
					// TODO: all chains?
					if (isUnlinkedOutExistsGlobal(lUtxoId, lUtxo)) {

						// utxo exists
						lUtxoPtr = Transaction::UnlinkedOut::instance(lUtxo);
						
						// cache it
						cacheUtxo(lUtxoPtr);
						assetEntitiesCache_[lEntityId][lUtxoId] = lUtxoPtr;
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
	//
	uint256 lUtxoId = utxo->hash();

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: ") + 
		strprintf("try to push utxo = %s, tx = %s, ctx = %s/%s#", 
			utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));

	Transaction::UnlinkedOut lLtxo;
	if (ltxo_.read(lUtxoId, lLtxo)) {
		// already exists - just skip
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: ") + 
			strprintf("ALREDY USED, skip action for utxo = %s, tx = %s, ctx = %s/%s#", 
				utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));
		return true;
	}

	Transaction::UnlinkedOut lUtxo;
	if (utxo_.read(lUtxoId, lUtxo)) {
		// already exists - accept
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: ") + 
			strprintf("ALREDY pushed, skip action for utxo = %s, tx = %s, ctx = %s/%s#", 
				utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));
		return true;
	}

	// cache it
	cacheUtxo(utxo);

	// update utxo db
	utxo_.write(lUtxoId, *utxo);

	// preserve in tx context for rollback
	ctx->addNewUnlinkedOut(utxo);

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: pushed ") + 
		strprintf("utxo = %s, tx = %s, ctx = %s/%s#", 
			utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));

	uint256 lAssetId = utxo->out().asset();
	if (ctx->tx()->isValue(utxo)) {
		//
		{
			boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
			assetsCache_[lAssetId].insert(std::multimap<amount_t, uint256>::value_type(utxo->amount(), lUtxoId));
		}

		// update assets db
		assets_.write(lAssetId, lUtxoId);
	} else if (ctx->tx()->isEntity(utxo) && lAssetId != TxAssetType::nullAsset()) {
		//
		{
			boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
			// update cache
			assetEntitiesCache_[lAssetId][lUtxoId] = utxo;
		}

		// update db
		assetEntities_.write(lAssetId, lUtxoId);
	}

	return true;
}

bool Wallet::popUnlinkedOut(const uint256& hash, TransactionContextPtr ctx) {
	//
	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[popUnlinkedOut]: ") + 
		strprintf("try to pop utxo = %s, tx = ?, ctx = %s/%s#", 
			hash.toHex(), ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));

	Transaction::UnlinkedOut lLtxo;
	if (ltxo_.read(hash, lLtxo)) {
		// already exists - accept
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[popUnlinkedOut]: ") + 
			strprintf("ALREDY popped, skip action for utxo = %s, tx = %s, ctx = %s/%s#", 
				lLtxo.hash().toHex(), lLtxo.out().tx().toHex(), ctx->tx()->id().toHex(), lLtxo.out().chain().toHex().substr(0, 10)));
		return true;
	}

	Transaction::UnlinkedOutPtr lUtxoPtr = nullptr;
	Transaction::UnlinkedOut lUtxo;
	if (utxo_.read(hash, lUtxo)) {
		lUtxoPtr = Transaction::UnlinkedOut::instance(lUtxo);
	}

	//Transaction::UnlinkedOutPtr lUtxoPtr = findUnlinkedOut(hash);
	if (lUtxoPtr) {

		// correct balance
		uint256 lAssetId = lUtxoPtr->out().asset();

		// preserve in tx context for rollback
		ctx->addUsedUnlinkedOut(lUtxoPtr);

		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[popUnlinkedOut]: ") + 
			strprintf("popped utxo = %s, tx = %s, ctx = %s/%s#", 
				hash.toHex(), lUtxoPtr->out().tx().toHex(), ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));

		{
			boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
			// try assets cache
			std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(lAssetId);
			if (lAsset != assetsCache_.end()) {
				std::pair<std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator,
							std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator> lRange = lAsset->second.equal_range(lUtxoPtr->amount());
				for (std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator lItem = lRange.first; lItem != lRange.second; lItem++) {
					if (lItem->second == hash) {
						lAsset->second.erase(lItem);
						break;
					}
				}
			}
		}

		{
			boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
			// try entities
			std::map<uint256, std::map<uint256, Transaction::UnlinkedOutPtr>>::iterator lEntity = assetEntitiesCache_.find(lAssetId);
			if (lEntity != assetEntitiesCache_.end()) {
				lEntity->second.erase(hash);
			}
		}

		// remove entry
		removeUtxo(hash);
		utxo_.remove(hash);
		ltxo_.write(hash, *lUtxoPtr);

		// assets_ entries is not cleaned at this point
		// this index maintened on startup only
		// TODO: make more robast index maintenance
		return true;
	}

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[popUnlinkedOut]: ") + 
		strprintf("link NOT FOUND for utxo = %s, tx = ?, ctx = %s/%s#",
			hash.toHex(), ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));

	return false;
}

bool Wallet::isUnlinkedOutUsed(const uint256& utxo) {
	//
	Transaction::UnlinkedOut lLtxo;
	if (ltxo_.read(utxo, lLtxo)) {
		return true;
	}

	return false;
}

bool Wallet::isUnlinkedOutExists(const uint256& utxo) {
	//
	Transaction::UnlinkedOut lUtxo;
	if (utxo_.read(utxo, lUtxo)) {
		return true;
	}

	return false;
}

Transaction::UnlinkedOutPtr Wallet::findUnlinkedOut(const uint256& hash) {
	//
	if (useUtxoCache_) {
		std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lUtxo = utxoCache_.find(hash);
		if (lUtxo != utxoCache_.end()) {
			if (isUnlinkedOutExistsGlobal(hash, (*lUtxo->second))) {
				return lUtxo->second;
			}
		}
	} else {
		Transaction::UnlinkedOut lUtxo;
		if (utxo_.read(hash, lUtxo) && isUnlinkedOutExistsGlobal(hash, lUtxo)) {
			return Transaction::UnlinkedOut::instance(lUtxo);
		}
	}

	return nullptr;
}

Transaction::UnlinkedOutPtr Wallet::findLinkedOut(const uint256& hash) {
	//	
	Transaction::UnlinkedOut lUtxo;
	if (ltxo_.read(hash, lUtxo) && isLinkedOutExistsGlobal(hash, lUtxo)) {
		return Transaction::UnlinkedOut::instance(lUtxo);
	}

	return nullptr;
}

Transaction::UnlinkedOutPtr Wallet::findUnlinkedOutByAsset(const uint256& asset, amount_t amount) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
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

amount_t Wallet::pendingBalance(const uint256& asset) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
	//
	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + strprintf("computing available balance for %s", asset.toHex()));
	//
	amount_t lBalance = 0;
	std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(asset);
	if (lAsset != assetsCache_.end()) {
		for (std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator lAmount = lAsset->second.begin(); 
			lAmount != lAsset->second.end(); lAmount++) {

			Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(lAmount->second);

			if (lUtxo == nullptr) {
				//
				if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + 
							strprintf("utxo NOT FOUND %s/%s", lAmount->second.toHex(), asset.toHex()));
				// delete from store
				utxo_.remove(lAmount->second);
				lAsset->second.erase(lAmount);
			} else {
				//
				if (gLog().isEnabled(Log::BALANCE)) gLog().write(Log::WALLET, std::string("[balance]: ") + 
							strprintf("utxo FOUND %d/%s/%s", lAmount->first, lAmount->second.toHex(), asset.toHex()));

				lBalance += lAmount->first;
			}
		}
	}

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + strprintf("wallet balance for %s = %d", asset.toHex(), lBalance));
	return lBalance;
}

amount_t Wallet::balance(const uint256& asset) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
	//
	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + strprintf("computing pending balance for %s", asset.toHex()));
	//
	amount_t lBalance = 0;
	std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(asset);
	if (lAsset != assetsCache_.end()) {
		for (std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator lAmount = lAsset->second.begin(); 
			lAmount != lAsset->second.end();) {

			Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(lAmount->second);

			if (lUtxo == nullptr) {
				//
				if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + 
							strprintf("utxo NOT FOUND %s/%s", lAmount->second.toHex(), asset.toHex()));
				// delete from store
				utxo_.remove(lAmount->second);
				lAsset->second.erase(lAmount);
				lAmount++;
			} else {
				//
				if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + 
							strprintf("utxo FOUND %d/%s/%s", lAmount->first, lAmount->second.toHex(), asset.toHex()));

				// extra check
				{
					uint64_t lHeight;
					uint64_t lConfirms;
					ITransactionStorePtr lStore;
					if (storeManager()) lStore = storeManager()->locate(lUtxo->out().chain());
					else lStore = persistentStore_;

					if (lStore) {
						bool lCoinbase = false;
						if (lStore->transactionHeight(lUtxo->out().tx(), lHeight, lConfirms, lCoinbase)) {
							//
							IMemoryPoolPtr lMempool;
							if(mempoolManager()) lMempool = mempoolManager()->locate(lUtxo->out().chain()); // corresponding mempool
							else lMempool = mempool_;

							//
							if (!lCoinbase && lConfirms < lMempool->consensus()->maturity()) {
								if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: transaction is NOT MATURE ") + 
									strprintf("%d/%d/%s/%s#", lConfirms, lMempool->consensus()->maturity(), lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
								lAmount++;
								continue; // skip UTXO
							} else if (lCoinbase && lConfirms < lMempool->consensus()->coinbaseMaturity()) {
								if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: COINBASE transaction is NOT MATURE ") + 
									strprintf("%d/%d/%s/%s#", lConfirms, lMempool->consensus()->coinbaseMaturity(), lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
								lAmount++;
								continue; // skip UTXO
							} else if (lUtxo->lock() && lUtxo->lock() > lHeight + lConfirms) {
								if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: timelock is NOT REACHED ") + 
									strprintf("%d/%d/%s/%s#", lUtxo->lock(), lHeight + lConfirms, lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
								lAmount++;
								continue; // skip UTXO
							}
						} else {
							gLog().write(Log::WALLET, std::string("[balance]: transaction not found ") + 
								strprintf("%s/%s#", lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
						}
					} else {
						gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: storage not found for ") + 
							strprintf("%s#", lUtxo->out().chain().toHex().substr(0, 10)));
					}
				}

				lBalance += lAmount->first;
				lAmount++;
			}
		}
	}

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + strprintf("wallet balance for %s = %d", asset.toHex(), lBalance));
	return lBalance;
}

void Wallet::collectUnlinkedOutsByAsset(const uint256& asset, amount_t amount, std::list<Transaction::UnlinkedOutPtr>& list) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
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

			// extra check
			{
				uint64_t lHeight;
				uint64_t lConfirms;
				ITransactionStorePtr lStore;
				if (storeManager()) lStore = storeManager()->locate(lUtxo->out().chain());
				else lStore = persistentStore_;

				if (lStore) {
					bool lCoinbase = false;
					if (lStore->transactionHeight(lUtxo->out().tx(), lHeight, lConfirms, lCoinbase)) {
						//
						IMemoryPoolPtr lMempool;
						if(mempoolManager()) lMempool = mempoolManager()->locate(lUtxo->out().chain()); // corresponding mempool
						else lMempool = mempool_;

						//
						if (!lCoinbase && lConfirms < lMempool->consensus()->maturity()) {
							if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: transaction is NOT MATURE ") + 
								strprintf("%d/%d/%s/%s#", lConfirms, lMempool->consensus()->maturity(), lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
							lAmount++;
							continue; // skip UTXO
						} else if (lCoinbase && lConfirms < lMempool->consensus()->coinbaseMaturity()) {
							if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: COINBASE transaction is NOT MATURE ") + 
								strprintf("%d/%d/%s/%s#", lConfirms, lMempool->consensus()->coinbaseMaturity(), lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
							lAmount++;
							continue; // skip UTXO
						} else if (lUtxo->lock() && lUtxo->lock() > lHeight + lConfirms) {
							if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: timelock is NOT REACHED ") + 
								strprintf("%d/%d/%s/%s#", lUtxo->lock(), lHeight + lConfirms, lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
							lAmount++;
							continue; // skip UTXO
						}
					} else {
						gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: transaction not found ") + 
							strprintf("%s/%s#", lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
					}
				} else {
					gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: storage not found for ") + 
						strprintf("%s#", lUtxo->out().chain().toHex().substr(0, 10)));
				}
			}

			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: tx found for amount ") + 
				strprintf("%d/%s/%s/%s#", lAmount->first, lUtxo->out().hash().toHex(), lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));

			lTotal += lAmount->first;
			list.push_back(lUtxo);

			if (lTotal >= amount) {
				break;
			}

			lAmount++;
		}
	}

	if (lTotal < amount) list.clear(); // amount is unreachable

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: total amount collected ") + 
		strprintf("%d", lTotal));
}

// clean-up assets utxo
void Wallet::cleanUp() {
	resetCache();
	prepareCache();
}

void Wallet::cleanUpData() {
	utxo_.close();
	ltxo_.close();
	assets_.close();
	assetEntities_.close();

	// clean up
	std::string lPath = settings_->dataPath() + "/wallet/utxo";
	rmpath(lPath.c_str());
	lPath = settings_->dataPath() + "/wallet/ltxo";
	rmpath(lPath.c_str());
	lPath = settings_->dataPath() + "/wallet/assets";
	rmpath(lPath.c_str());
	lPath = settings_->dataPath() + "/wallet/asset_entities";
	rmpath(lPath.c_str());

	// re-open
	try {
		utxo_.open();
		ltxo_.open();
		assets_.open();
		assetEntities_.open();
	}
	catch(const std::exception& ex) {
		gLog().write(Log::ERROR, std::string("[wallet/cleanUpData]: ") + ex.what());
	}
}

// dump utxo by asset
void Wallet::dumpUnlinkedOutByAsset(const uint256& asset, std::stringstream& s) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
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
	gLog().write(Log::WALLET, std::string("[rollback]: rollback tx ") + 
		strprintf("%s/%s#", ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));
	// recover new utxos
	for (std::list<Transaction::UnlinkedOutPtr>::iterator lUtxo = ctx->newUtxo().begin(); lUtxo != ctx->newUtxo().end(); lUtxo++) {
		// locate utxo
		uint256 lUtxoId = (*lUtxo)->hash();
		Transaction::UnlinkedOutPtr lUtxoPtr = findUnlinkedOut(lUtxoId);
		if (lUtxoPtr) {
			//
			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[rollback]: remove utxo ") + 
				strprintf("%s/%s/%s#", 
					lUtxoId.toHex(), (*lUtxo)->out().tx().toHex(), (*lUtxo)->out().chain().toHex().substr(0, 10)));

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

		ltxo_.remove(lUtxoId);

		if (!findUnlinkedOut(lUtxoId)) {
			//
			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[rollback]: reconstruct utxo ") + 
				strprintf("%s/%s/%s#", 
					lUtxoId.toHex(), (*lUtxo)->out().tx().toHex(), (*lUtxo)->out().chain().toHex().substr(0, 10)));
			// recover
			utxo_.write(lUtxoId, *(*lUtxo));
			cacheUtxo((*lUtxo));

			if (ctx->tx()->isValue(*lUtxo)) {
				//
				boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);

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

			} else if (ctx->tx()->isEntity(*lUtxo)) {
				//
				boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
				// reconstruct entity
				std::map<uint256, std::map<uint256, Transaction::UnlinkedOutPtr>>::iterator lEntity = assetEntitiesCache_.find(lAssetId);
				if (lEntity != assetEntitiesCache_.end()) {
					lEntity->second.insert(std::map<uint256, Transaction::UnlinkedOutPtr>::value_type(lUtxoId, *lUtxo));
				}
			}
		}
	}

	ctx->usedUtxo().clear();

	return true;
}

// fill inputs
amount_t Wallet::fillInputs(TxSpendPtr tx, const uint256& asset, amount_t amount, std::list<Transaction::UnlinkedOutPtr>& /*utxos*/) {
	// collect utxo's
	std::list<Transaction::UnlinkedOutPtr> lUtxos;
	collectUnlinkedOutsByAsset(asset, amount, lUtxos);

	if (!lUtxos.size()) throw qbit::exception("E_AMOUNT", "Insufficient amount.");

	// amount
	amount_t lAmount = 0;

	// fill ins 
	for (std::list<Transaction::UnlinkedOutPtr>::iterator lUtxo = lUtxos.begin(); lUtxo != lUtxos.end(); lUtxo++) {
		// locate skey
		SKeyPtr lSKey = findKey((*lUtxo)->address());
		if (lSKey && lSKey->valid()) { 
			tx->addIn(*lSKey, *lUtxo);
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
	std::list<Transaction::UnlinkedOutPtr> lUtxos, lFeeUtxos;
	amount_t lAmount = fillInputs(lTx, asset, amount, lUtxos);

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[makeTxSpend]: creating spend tx: ") + 
		strprintf("to = %s, amount = %d, asset = %s#", const_cast<PKey&>(dest).toString(), amount, asset.toHex().substr(0, 10)));

	// fill output
	SKeyPtr lSChangeKey = changeKey();
	SKeyPtr lSKey = firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");
	lTx->addOut(*lSKey, dest, asset, amount);

	// make change
	if (asset != TxAssetType::qbitAsset() && lAmount > amount) {
		lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, asset, lAmount - amount);			
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
			lTx->addFeeOut(*lSKey, asset, lFee); // to miner

			if (lAmount > amount + lFee) { // make change
				lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, asset, lAmount - (amount + lFee));
			}
		} else { // rare cases
			return makeTxSpend(type, asset, dest, amount + lFee, feeRateLimit, targetBlock);
		}
	} else {
		// add fee
		amount_t lFeeAmount = fillInputs(lTx, TxAssetType::qbitAsset(), lFee, lFeeUtxos);
		lTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
		if (lFeeAmount > lFee) { // make change
			lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
		}
	}

	if (!lTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[makeTxSpend]: spend tx created: ") + 
		strprintf("to = %s, amount = %d/%d, fee = %d, asset = %s#", const_cast<PKey&>(dest).toString(), amount, lAmount, lFee, asset.toHex().substr(0, 10)));

	// write to pending transactions
	pendingtxs_.write(lTx->id(), lTx);

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

	SKeyPtr lSChangeKey = changeKey();
	SKeyPtr lSKey = firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	if (emission == TxAssetType::LIMITED) {
		for (int lIdx = 0; lIdx < chunks; lIdx++)
			lAssetTypeTx->addLimitedOut(*lSKey, dest, supply * scale);
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
	std::list<Transaction::UnlinkedOutPtr> lUtxos;
	amount_t lFeeAmount = fillInputs(lAssetTypeTx, TxAssetType::qbitAsset(), lFee, lUtxos);
	lAssetTypeTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		lAssetTypeTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
	}

	if (!lAssetTypeTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	// write to pending transactions
	pendingtxs_.write(lAssetTypeTx->id(), lAssetTypeTx);	
	
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
	std::map<uint256, std::map<uint256, Transaction::UnlinkedOutPtr>>::iterator lEntity = assetEntitiesCache_.find(entity);
	if (lEntity != assetEntitiesCache_.end() && lEntity->second.size()) {
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

	SKeyPtr lSChangeKey = changeKey();
	SKeyPtr lSKey = firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	lAssetEmissionTx->addLimitedIn(*lSKey, lUtxoPtr);
	lAssetEmissionTx->addOut(*lSKey, dest, asset, lUtxoPtr->amount()); // each emission === utxo.amount()

	// try to estimate fee
	qunit_t lRate = 0;
	if (targetBlock == -1) {
		lRate = mempool()->estimateFeeRateByLimit(lCtx, feeRateLimit);
	} else {
		lRate = mempool()->estimateFeeRateByBlock(lCtx, targetBlock);
	}

	amount_t lFee = lRate * lCtx->size(); 
	std::list<Transaction::UnlinkedOutPtr> lUtxos;
	amount_t lFeeAmount = fillInputs(lAssetEmissionTx, TxAssetType::qbitAsset(), lFee, lUtxos);
	lAssetEmissionTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		lAssetEmissionTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
	}

	if (!lAssetEmissionTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	// write to pending transactions
	pendingtxs_.write(lAssetEmissionTx->id(), lAssetEmissionTx);	
	
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

	SKeyPtr lSKey = firstKey(); // TODO: do we need to specify exact skey?
	if (!lSKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	// make output
	Transaction::UnlinkedOutPtr lUtxoPtr = lTx->addOut(*lSKey, lSKey->createPKey(), TxAssetType::qbitAsset(), amount);
	lUtxoPtr->out().setTx(lTx->id()); // finalize

	return lCtx;
}

TransactionContextPtr Wallet::createTxDApp(const PKey& dest, const std::string& shortName, const std::string& longName, TxDApp::Sharding sharding, Transaction::Type instances) {
	// create empty tx
	TxDAppPtr lDAppTx = TransactionHelper::to<TxDApp>(TransactionFactory::create(Transaction::DAPP));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lDAppTx);
	// sanity check
	if (entityStore_->locateEntity(shortName)) throw qbit::exception("E_ENTITY", "Invalid entity.");

	lDAppTx->setShortName(shortName);
	lDAppTx->setLongName(longName);
	lDAppTx->setSharding(sharding);

	SKeyPtr lSChangeKey = changeKey();
	SKeyPtr lSKey = firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	// make dapp main out
	lDAppTx->addDAppOut(*lSKey, dest); // first out - identity link
	// main public out
	lDAppTx->addDAppPublicOut(*lSKey, dest, instances);  // second out - public link

	// try to estimate fee
	qunit_t lRate = mempool()->estimateFeeRateByLimit(lCtx, settings_->maxFeeRate());

	amount_t lFee = lRate * lCtx->size(); 
	std::list<Transaction::UnlinkedOutPtr> lUtxos;
	amount_t lFeeAmount = fillInputs(lDAppTx, TxAssetType::qbitAsset(), lFee, lUtxos);
	lDAppTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		lDAppTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
	}

	if (!lDAppTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	// write to pending transactions
	pendingtxs_.write(lDAppTx->id(), lDAppTx);	
	
	return lCtx;
}

TransactionContextPtr Wallet::createTxShard(const PKey& dest, const std::string& dappName, const std::string& shortName, const std::string& longName) {
	// create empty tx
	TxShardPtr lShardTx = TransactionHelper::to<TxShard>(TransactionFactory::create(Transaction::SHARD));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lShardTx);
	// dapp entity
	EntityPtr lDApp = entityStore_->locateEntity(dappName); 
	if (!lDApp) throw qbit::exception("E_ENTITY", "Invalid entity.");

	//
	lShardTx->setShortName(shortName);
	lShardTx->setLongName(longName);

	SKeyPtr lSChangeKey = changeKey();
	SKeyPtr lSKey = firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	// make shard public out
	lShardTx->addShardOut(*lSKey, dest);

	// make input
	std::vector<Transaction::UnlinkedOutPtr> lUtxos;
	if (entityStore_->collectUtxoByEntityName(dappName, lUtxos)) {
		lShardTx->addDAppIn(*lSKey, *lUtxos.begin()); // first out -> first in
	} else {
		throw qbit::exception("E_ENTITY_OUT", "Entity utxo was not found.");
	}

	// try to estimate fee
	qunit_t lRate = mempool()->estimateFeeRateByLimit(lCtx, settings_->maxFeeRate());

	amount_t lFee = lRate * lCtx->size(); 
	std::list<Transaction::UnlinkedOutPtr> lInputsUtxos;
	amount_t lFeeAmount = fillInputs(lShardTx, TxAssetType::qbitAsset(), lFee, lInputsUtxos);
	lShardTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		lShardTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
	}

	if (!lShardTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	// write to pending transactions
	pendingtxs_.write(lShardTx->id(), lShardTx);
	
	return lCtx;
}

TransactionContextPtr Wallet::createTxFee(const PKey& dest, amount_t amount) {
	// create empty tx
	TxFeePtr lTx = TransactionHelper::to<TxFee>(TransactionFactory::create(Transaction::FEE)); // TxSendPrivate -> TxSend
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lTx);
	// fill inputs
	std::list<Transaction::UnlinkedOutPtr> lUtxos;
	amount_t lAmount = fillInputs(lTx, TxAssetType::qbitAsset(), amount, lUtxos);

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[createTxFee]: creating fee tx: ") + 
		strprintf("to = %s, amount = %d", const_cast<PKey&>(dest).toString(), amount));

	// fill output
	SKeyPtr lSChangeKey = changeKey();
	SKeyPtr lSKey = firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");
	lTx->addExternalOut(*lSKey, dest, TxAssetType::qbitAsset(), amount); // out[0] - use to pay fee in shards

	amount_t lRate = mempool()->estimateFeeRateByLimit(lCtx, settings_->maxFeeRate());
	amount_t lFee = (lRate * lCtx->size()) / 2; // half fee 

	// try to check amount
	if (lAmount >= amount + lFee) {
		lTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner

		if (lAmount > amount + lFee) { // make change
			lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lAmount - (amount + lFee));
		}
	} else { // rare cases
		return createTxFee(dest, amount + lFee);
	}

	if (!lTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[createTxFee]: fee tx created: ") + 
		strprintf("to = %s, amount = %d/%d", const_cast<PKey&>(dest).toString(), amount, lAmount));

	// write to pending transactions
	pendingtxs_.write(lTx->id(), lTx);

	return lCtx;
}

TransactionContextPtr Wallet::createTxFeeLockedChange(const PKey& dest, amount_t amount, amount_t locked, uint64_t height) {
	// create empty tx
	TxFeePtr lTx = TransactionHelper::to<TxFee>(TransactionFactory::create(Transaction::FEE)); // TxSendPrivate -> TxSend
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lTx);
	// fill inputs
	std::list<Transaction::UnlinkedOutPtr> lUtxos;
	amount_t lAmount = fillInputs(lTx, TxAssetType::qbitAsset(), amount+locked, lUtxos);

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[createTxFeeLockedChange]: creating fee tx: ") + 
		strprintf("to = %s, amount = %d", const_cast<PKey&>(dest).toString(), amount+locked));

	// fill output
	SKeyPtr lSChangeKey = changeKey();
	SKeyPtr lSKey = firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");
	lTx->addExternalOut(*lSKey, dest, TxAssetType::qbitAsset(), amount); // out[0] - use to pay fee in shards
	lTx->addLockedOut(*lSKey, dest, TxAssetType::qbitAsset(), locked, height); // out[1] - locked out

	amount_t lRate = mempool()->estimateFeeRateByLimit(lCtx, settings_->maxFeeRate());
	amount_t lFee = (lRate * lCtx->size()) / 2; // half fee 

	// try to check amount
	if (lAmount >= amount + locked + lFee) {
		lTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner

		if (lAmount > amount + locked + lFee) { // make change
			lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lAmount - (amount + locked + lFee));
		}
	} else { // rare cases
		return createTxFeeLockedChange(dest, amount + lFee, locked, height);
	}

	if (!lTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[createTxFeeLockedChange]: fee tx created: ") + 
		strprintf("to = %s, amount = [%d/%d]%d", const_cast<PKey&>(dest).toString(), amount, locked, lAmount));

	// write to pending transactions
	pendingtxs_.write(lTx->id(), lTx);

	return lCtx;
}

TransactionContextPtr Wallet::createTxBase(const uint256& chain, amount_t amount) {
	// create empty tx
	TxBasePtr lTx = TransactionHelper::to<TxBase>(TransactionFactory::create(Transaction::BASE));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lTx);

	// add input
	lTx->addIn();
	// set chain
	lTx->setChain(chain);

	SKeyPtr lSKey = firstKey(); 
	if (!lSKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	// make output
	Transaction::UnlinkedOutPtr lUtxoPtr = lTx->addOut(*lSKey, lSKey->createPKey(), TxAssetType::qbitAsset(), amount);
	lUtxoPtr->out().setTx(lTx->id()); // finalize

	return lCtx;
}

TransactionContextPtr Wallet::createTxBlockBase(const BlockHeader& blockHeader, amount_t amount) {
	// create empty tx
	TxBlockBasePtr lTx = TransactionHelper::to<TxBlockBase>(TransactionFactory::create(Transaction::BLOCKBASE));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lTx);

	// add input
	lTx->addIn();

	// set header
	lTx->setBlockHeader(blockHeader);

	SKeyPtr lSKey = firstKey(); 
	if (!lSKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	// make output
	Transaction::UnlinkedOutPtr lUtxoPtr = lTx->addOut(*lSKey, lSKey->createPKey(), TxAssetType::qbitAsset(), amount);
	lUtxoPtr->out().setTx(lTx->id()); // finalize

	// write to pending transactions
	pendingtxs_.write(lTx->id(), lTx);

	return lCtx;
}