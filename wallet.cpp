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

SKeyPtr Wallet::removeKey(const PKey& pkey) {
	if (pkey.valid()) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(keyMutex_);
		std::map<uint160 /*id*/, SKeyPtr>::iterator lSKeyIter = keysCache_.find(pkey.id());
		if (lSKeyIter != keysCache_.end())
			keysCache_.erase(lSKeyIter);

		SKey lSKey;
		if (keys_.read(pkey.id(), lSKey)) {
			SKeyPtr lSKeyPtr = SKey::instance(lSKey);
			keys_.remove(pkey.id());

			return lSKeyPtr;
		}
	}

	return nullptr;
}

SKeyPtr Wallet::firstKey() {
	//
	PKey lMainKey = settings_->mainKey();
	if (lMainKey.valid()) {
		return findKey(lMainKey);
	}
	//
	{
		boost::unique_lock<boost::recursive_mutex> lLock(keyMutex_);
		std::map<uint160 /*id*/, SKeyPtr>::iterator lSKeyIter = keysCache_.begin();
		if (lSKeyIter != keysCache_.end() && lSKeyIter->first == settings_->shadowKey().id()) lSKeyIter++;
		if (lSKeyIter != keysCache_.end()) return lSKeyIter->second;
	}
	//
	uint160 lKeyId;
	db::DbContainer<uint160 /*id*/, SKey>::Iterator lKey = keys_.begin();
	if (lKey.valid() && lKey.first(lKeyId) && lKeyId == settings_->shadowKey().id()) lKey++;
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

void Wallet::removeAllKeys() {
	//
	db::DbContainer<uint160 /*id*/, SKey>::Iterator lKey = keys_.begin();
	for (; lKey.valid(); ++lKey) {
		//
		keys_.remove(lKey);
	}

	//
	{
		boost::unique_lock<boost::recursive_mutex> lLock(keyMutex_);
		keysCache_.clear();
	}
}

void Wallet::allKeys(std::list<SKeyPtr>& keys) {
	//
	db::DbContainer<uint160 /*id*/, SKey>::Iterator lKey = keys_.begin();
	for (; lKey.valid(); ++lKey) {
		//
		keys.push_back(SKey::instance(*lKey));
	}
}

bool Wallet::open(const std::string& /*secret*/) {
	if (!opened_) {
		try {
			//
			gLog().write(Log::INFO, std::string("[wallet/open]: opening wallet data..."));
			//
			if (mkpath(std::string(settings_->dataPath() + "/wallet").c_str(), 0777)) return false;

			keys_.open();

			utxo_.open();
			utxo_.attach();

			ltxo_.open();
			ltxo_.attach();

			assets_.open();
			assets_.attach();

			assetEntities_.open();
			assetEntities_.attach();

			pendingtxs_.open();
			pendingtxs_.attach();

			balance_.open();
			balance_.attach();

			pendingUtxo_.open();
			pendingUtxo_.attach();

			// finally - open space
			if (settings_->repairDb()) {
				gLog().write(Log::INFO, std::string("[open]: repairing wallet data..."));
				space_->repair();
			}
			//
			space_->open();	

			//
			gLog().write(Log::INFO, std::string("[wallet/open]: wallet storage opened..."));
		}
		catch(const std::exception& ex) {
			gLog().write(Log::GENERAL_ERROR, std::string("[wallet/open]: ") + ex.what());
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
	balance_.close();

	space_->close();

	return true;
}

bool Wallet::prepareCache() {
	//
	try {
		// scan assets, check utxo and fill balance
		db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Transaction lAssetTransaction = assets_.transaction();
		// 
		db::DbContainerShared<uint256 /*utxo*/, Transaction::UnlinkedOut /*data*/>::Transaction lUtxoTransaction = utxo_.transaction();

		for (db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Iterator lAsset = assets_.begin(); lAsset.valid(); ++lAsset) {
			Transaction::UnlinkedOut lUtxo;
			uint256 lUtxoId;
			uint256 lAssetId;
			lAsset.first(lAssetId, lUtxoId); // asset hash, utxo hash

			if (utxo_.read(lUtxoId, lUtxo)) {
				// if utxo exists
				if (!isUnlinkedOutExistsGlobal(lUtxoId, lUtxo)) {
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
		db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Transaction lEntityTransaction = assetEntities_.transaction();

		// scan entities
		for (db::DbTwoKeyContainerShared<uint256 /*entity*/, uint256 /*utxo*/, uint256 /*tx*/>::Iterator lEntity = assetEntities_.begin(); lEntity.valid(); ++lEntity) {
			uint256 lUtxoId = *lEntity; // utxo hash
			uint256 lEntityId;
			lEntity.first(lEntityId); // entity hash

			Transaction::UnlinkedOutPtr lUtxoPtr = findUnlinkedOut(lUtxoId);
			if (!lUtxoPtr) {
				Transaction::UnlinkedOut lUtxo;
				if (utxo_.read(lEntityId, lUtxo)) {
					// if utxo exists
					// TODO: all chains?
					if (!isUnlinkedOutExistsGlobal(lUtxoId, lUtxo)) {
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

		// try to re-check pending txs
		checkPendingUtxo();

		opened_ = true;

		//
		gLog().write(Log::INFO, std::string("[wallet/prepareCache]: cache prepared, wallet is fully opened"));
	}
	catch(const std::exception& ex) {
		gLog().write(Log::GENERAL_ERROR, std::string("[wallet/prepareCache]: ") + ex.what());
		opened_ = true; // half-opened
	}

	return opened_;
}

bool Wallet::pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr ctx) {
	//
	if (!opened_) {
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: ") + 
			strprintf("wallet is NOT opened yet, REJECTING utxo = %s, tx = %s, ctx = %s/%s#", 
				utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));
		return false;
	}

	//
	uint256 lUtxoId = utxo->hash();

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: ") + 
		strprintf("try to push utxo = %s, tx = %s, ctx = %s/%s#", 
			utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));

	Transaction::UnlinkedOut lLtxo;
	if (ltxo_.read(lUtxoId, lLtxo)) {
		// already exists - just skip
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: ") + 
			strprintf("ALREADY USED, skip action for utxo = %s, tx = %s, ctx = %s/%s#", 
				utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));
		return true;
	}

	Transaction::UnlinkedOut lUtxo;
	if (utxo_.read(lUtxoId, lUtxo)) {
		// already exists - accept
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: ") + 
			strprintf("ALREADY pushed, skip action for utxo = %s, tx = %s, ctx = %s/%s#", 
				utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));
		return true;
	}

	// cache it
	cacheUtxo(utxo);

	// update utxo db
	utxo_.write(lUtxoId, *utxo);

	// preserve in tx context for rollback
	ctx->addNewUnlinkedOut(utxo);

	// notify
	settings_->notifyTransaction(utxo->out().tx());

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: pushed ") + 
		strprintf("utxo = %s, tx = %s, ctx = %s/%s#", 
			utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));

	uint256 lAssetId = utxo->out().asset();
	if (ctx->tx()->isValue(utxo)) {
		// update assets db
		if (utxo->amount() > 0) {
			assets_.write(lAssetId, lUtxoId, utxo->out().tx()); // write assets
			pendingUtxo_.write(lUtxoId, utxo->out().tx()); // write pendings
			{
				//
				boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
				amount_t lAmount = 0;
				balance_.read(lAssetId, lAmount);
				lAmount += utxo->amount(); // return amount
				balance_.write(lAssetId, lAmount);
			}

			// TODO: do we need every-time check hehe?
			// checkPendingUtxo();
		}
	} else if (ctx->tx()->isEntity(utxo) && lAssetId != TxAssetType::nullAsset()) {
		// update db
		assetEntities_.write(lAssetId, lUtxoId, utxo->out().tx());
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

		// remove entry
		removeUtxo(hash);
		utxo_.remove(hash);
		ltxo_.write(hash, *lUtxoPtr);

		//
		if (lUtxoPtr->amount() > 0) {
			//
			assets_.remove(lAssetId, hash);
			pendingUtxo_.remove(hash);
			{
				boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
				amount_t lAmount = 0;
				balance_.read(lAssetId, lAmount);
				lAmount -= lUtxoPtr->amount();
				balance_.write(lAssetId, lAmount);
			}

			// TODO: do we need every-time check here?
			// checkPendingUtxo();
		} else {
			// try entities
			assetEntities_.remove(lAssetId, hash);
		}

		return true;
	}

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[popUnlinkedOut]: ") + 
		strprintf("link NOT FOUND for utxo = %s, tx = ?, ctx = %s/%s#",
			hash.toHex(), ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));

	return false;
}

bool Wallet::tryRemoveUnlinkedOut(const uint256& utxo) {
	//
	if (!opened_) return false;

	//
	Transaction::UnlinkedOut lUtxo;
	if (utxo_.read(utxo, lUtxo)) {
		//
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[tryRemoveUnlinkedOut]: ") + 
			strprintf("try to remove utxo = %s", utxo.toHex()));
		//
		Transaction::UnlinkedOutPtr lUtxoPtr = Transaction::UnlinkedOut::instance(lUtxo);
		uint256 lAssetId = lUtxoPtr->out().asset();
		if (lUtxoPtr->amount() > 0) {
			//
			assets_.remove(lAssetId, utxo);
			{
				boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
				amount_t lAmount = 0;
				balance_.read(lAssetId, lAmount);
				if (lAmount >= lUtxoPtr->amount()) lAmount -= lUtxoPtr->amount(); // remove amount
				balance_.write(lAssetId, lAmount);
			}
		}

		utxo_.remove(utxo);

		return true;
	}

	return false;
}

bool Wallet::tryRevertUnlinkedOut(const uint256& utxo) {
	//
	if (!opened_) return false;

	//
	Transaction::UnlinkedOut lUtxo;
	if (ltxo_.read(utxo, lUtxo)) {
		//
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[tryRevertUnlinkedOut]: ") + 
			strprintf("try to remove utxo = %s", utxo.toHex()));
		//
		Transaction::UnlinkedOutPtr lUtxoPtr = Transaction::UnlinkedOut::instance(lUtxo);
		uint256 lAssetId = lUtxoPtr->out().asset();
		if (lUtxoPtr->amount() > 0) {
			//
			assets_.write(lAssetId, utxo, lUtxoPtr->out().tx());
			{
				boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
				amount_t lAmount = 0;
				balance_.read(lAssetId, lAmount);
				lAmount += lUtxoPtr->amount(); // return amount
				balance_.write(lAssetId, lAmount);
			}
		}

		ltxo_.remove(utxo);
		utxo_.write(utxo, lUtxo);

		return true;
	}

	return false;
}

void Wallet::checkPendingUtxo() {
	//
	for (db::DbContainerShared<uint256 /*utxo*/, uint256 /*transaction*/>::Iterator lItem = pendingUtxo_.begin(); lItem.valid(); ++lItem) {
		//
		uint64_t lHeight;
		uint64_t lConfirms;
		ITransactionStorePtr lStore;
		//
		uint256 lUtxoId;
		Transaction::UnlinkedOutPtr lUtxo;
		if (lItem.first(lUtxoId)) {
			lUtxo = findUnlinkedOut(lUtxoId);
		}

		//
		if (lUtxo == nullptr) {
			pendingUtxo_.remove(lUtxoId);
			continue;
		}

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
					continue; // skip UTXO
				} else if (lCoinbase && lConfirms < lMempool->consensus()->coinbaseMaturity()) {
					continue; // skip UTXO
				} else if (lUtxo->lock() && lUtxo->lock() > lHeight + lConfirms) {
					continue; // skip UTXO
				}

				// remove utxo from pending
				pendingUtxo_.remove(lUtxoId);
			}
		}
	}
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
	Transaction::UnlinkedOut lUtxo;
	if (utxo_.read(hash, lUtxo) && isUnlinkedOutExistsGlobal(hash, lUtxo)) {
		return Transaction::UnlinkedOut::instance(lUtxo);
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
	db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Transaction lAssetTransaction = assets_.transaction();
	db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Iterator lAsset = assets_.find(asset);
	lAsset.setKey2Empty();
	//
	for (; lAsset.valid(); ++lAsset) {
		uint256 lUtxoId = *lAsset; // utxo hash
		Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(lUtxoId);
		//
		if (lUtxo != nullptr) {
			if (lUtxo->amount() >= amount)
				lAssetTransaction.commit();
				return lUtxo;
		} else {
			// delete from store
			utxo_.remove(lUtxoId);
			lAssetTransaction.remove(asset, lUtxoId);
		}
	}

	lAssetTransaction.commit();
	return nullptr;
}

amount_t Wallet::pendingBalance(const uint256& asset) {
	//
	amount_t lPeending = 0, lActual = 0;
	balance(asset, lPeending, lActual, false);
	return lPeending;
}

amount_t Wallet::balance(const uint256& asset, bool rescan) {
	//
	amount_t lPeending = 0, lActual = 0;
	balance(asset, lPeending, lActual, rescan);
	return lActual;
}

void Wallet::balance(const uint256& asset, amount_t& pending, amount_t& actual, bool rescan) {
	//
	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + strprintf("computing balance for %s", asset.toHex()));
	//
	amount_t lAmount = 0, lNotConfirmed = 0;
	bool lProcess = false;
	{
		boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
		lProcess = balance_.read(asset, lAmount);
	}
	if (lProcess && !rescan) {
		//
		pending = lAmount;
		//
		db::DbContainerShared<uint256 /*utxo*/, uint256 /*transaction*/>::Transaction lPendingTx = pendingUtxo_.transaction();
		for (db::DbContainerShared<uint256 /*utxo*/, uint256 /*transaction*/>::Iterator lItem = pendingUtxo_.begin(); lItem.valid(); ++lItem) {
			//
			uint64_t lHeight;
			uint64_t lConfirms;
			ITransactionStorePtr lStore;
			//
			uint256 lUtxoId;
			Transaction::UnlinkedOutPtr lUtxo;
			if (lItem.first(lUtxoId)) {
				lUtxo = findUnlinkedOut(lUtxoId);
			}

			//
			if (lUtxo == nullptr) {
				lPendingTx.remove(lUtxoId);
				continue;
			}

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
					if ((!lCoinbase && lConfirms < lMempool->consensus()->maturity()) ||
							(lCoinbase && lConfirms < lMempool->consensus()->coinbaseMaturity()) || 
								(lUtxo->lock() && lUtxo->lock() > lHeight + lConfirms)) {
						lNotConfirmed += lUtxo->amount();
						continue; // skip UTXO
					}

					// remove utxo from pending
					lPendingTx.remove(lUtxoId);
				} else {
					//
					lNotConfirmed += lUtxo->amount();
				}
			}
		}

		//
		lPendingTx.commit();

		//
		actual = pending - lNotConfirmed;
	} else {
		//
		db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Transaction lAssetTransaction = assets_.transaction();
		db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Iterator lAsset = assets_.find(asset);
		lAsset.setKey2Empty();
		//
		for (; lAsset.valid(); ++lAsset) {
			uint256 lAssetId;
			uint256 lUtxoId;
			lAsset.first(lAssetId, lUtxoId);
			//
			Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(lUtxoId);
			//
			if (lUtxo == nullptr) {
				//
				if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + 
							strprintf("utxo NOT FOUND %s/%s", lUtxoId.toHex(), asset.toHex()));
				// delete from store
				lAssetTransaction.remove(lAssetId, lUtxoId);
				utxo_.remove(lUtxoId);
			} else {
				//
				if (!lUtxo->amount()) continue;
				//
				if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + 
							strprintf("utxo FOUND %d/%s/%s", lUtxo->amount(), lUtxoId.toHex(), asset.toHex()));
				// available + pending
				pending += lUtxo->amount();

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
								continue; // skip UTXO
							} else if (lCoinbase && lConfirms < lMempool->consensus()->coinbaseMaturity()) {
								if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: COINBASE transaction is NOT MATURE ") + 
									strprintf("%d/%d/%s/%s#", lConfirms, lMempool->consensus()->coinbaseMaturity(), lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
								continue; // skip UTXO
							} else if (lUtxo->lock() && lUtxo->lock() > lHeight + lConfirms) {
								if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: timelock is NOT REACHED ") + 
									strprintf("%d/%d/%s/%s#", lUtxo->lock(), lHeight + lConfirms, lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
								continue; // skip UTXO
							}
						} else {
							gLog().write(Log::WALLET, std::string("[balance]: transaction not found ") + 
								strprintf("%s/%s#", lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
							continue; // skip UTXO
						}
					} else {
						gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: storage not found for ") + 
							strprintf("%s#", lUtxo->out().chain().toHex().substr(0, 10)));
						continue; // skip UTXO
					}
				}

				actual += lUtxo->amount();
			}
		}

		{
			// update
			boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
			balance_.write(asset, pending);
		}

		lAssetTransaction.commit();
	}

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + strprintf("wallet balance for %s = %d", asset.toHex(), actual));
}

void Wallet::collectUnlinkedOutsByAsset(const uint256& asset, amount_t amount, bool aggregate, std::list<Transaction::UnlinkedOutPtr>& list) {
	// TODO: aggregate - consider to re-implement collection of dust
	collectUnlinkedOutsByAssetReverse(asset, amount, list);
}

void Wallet::collectUnlinkedOutsByAssetReverse(const uint256& asset, amount_t amount, std::list<Transaction::UnlinkedOutPtr>& list) {
	//
	amount_t lTotal = 0;
	db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Transaction lAssetTransaction = assets_.transaction();
	db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Iterator lAsset = assets_.find(asset);
	lAsset.setKey2Empty();
	//
	for (; lAsset.valid(); ++lAsset) {
		uint256 lAssetId;
		uint256 lUtxoId;
		lAsset.first(lAssetId, lUtxoId);
		//
		Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(lUtxoId);
		//
		if (lUtxo == nullptr) {
			//
			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAssete]: ") + 
						strprintf("utxo NOT FOUND %s/%s", lUtxoId.toHex(), asset.toHex()));
			// delete from store
			utxo_.remove(lUtxoId);
			lAssetTransaction.remove(lAssetId, lUtxoId);
		} else {
			//
			if (!lUtxo->amount()) continue;
			//
			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: ") + 
						strprintf("utxo FOUND %d/%s/%s", lUtxo->amount(), lUtxoId.toHex(), asset.toHex()));

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
							continue; // skip UTXO
						} else if (lCoinbase && lConfirms < lMempool->consensus()->coinbaseMaturity()) {
							if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: COINBASE transaction is NOT MATURE ") + 
								strprintf("%d/%d/%s/%s#", lConfirms, lMempool->consensus()->coinbaseMaturity(), lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
							continue; // skip UTXO
						} else if (lUtxo->lock() && lUtxo->lock() > lHeight + lConfirms) {
							if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: timelock is NOT REACHED ") + 
								strprintf("%d/%d/%s/%s#", lUtxo->lock(), lHeight + lConfirms, lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
							continue; // skip UTXO
						}
					} else {
						gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: transaction not found ") + 
							strprintf("%s/%s#", lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
						continue; // skip UTXO
					}
				} else {
					gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: storage not found for ") + 
						strprintf("%s#", lUtxo->out().chain().toHex().substr(0, 10)));
					continue; // skip UTXO
				}
			}

			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: tx found for amount ") + 
				strprintf("%d/%s/%s/%s#", lUtxo->amount(), lUtxo->out().hash().toHex(), lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));

			lTotal += lUtxo->amount();
			list.push_back(lUtxo);

			if (lTotal >= amount) {
				break;
			}
		}
	}

	lAssetTransaction.commit();

	if (lTotal < amount) list.clear(); // amount is unreachable

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: total amount collected ") + 
		strprintf("%d", lTotal));
}

void Wallet::collectUnlinkedOutsByAssetForward(const uint256& asset, amount_t amount, std::list<Transaction::UnlinkedOutPtr>& list) {
	// TODO: consider to re-implement
}

void Wallet::collectCoinbaseUnlinkedOuts(std::list<Transaction::UnlinkedOutPtr>& list) {
	//
	db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Transaction lAssetTransaction = assets_.transaction();
	db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Iterator lAsset = assets_.find(TxAssetType::qbitAsset());
	lAsset.setKey2Empty();
	//
	for (; lAsset.valid(); ++lAsset) {
		uint256 lAssetId;
		uint256 lUtxoId;
		lAsset.first(lAssetId, lUtxoId);
		//
		Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(lUtxoId);
		//
		if (lUtxo == nullptr) {
			//
			// delete from store
			utxo_.remove(lUtxoId);
			lAssetTransaction.remove(lAssetId, lUtxoId);
		} else {
			//
			if (!lUtxo->amount()) continue;
			// extra check
			{
				uint64_t lHeight;
				uint64_t lConfirms;
				ITransactionStorePtr lStore;
				if (storeManager()) lStore = storeManager()->locate(lUtxo->out().chain());
				else lStore = persistentStore_;

				if (lStore) {
					bool lCoinbase = false;
					if (lStore->transactionHeight(lUtxo->out().tx(), lHeight, lConfirms, lCoinbase) && lCoinbase) {
						//
						IMemoryPoolPtr lMempool;
						if(mempoolManager()) lMempool = mempoolManager()->locate(lUtxo->out().chain()); // corresponding mempool
						else lMempool = mempool_;

						//
						if (lConfirms < lMempool->consensus()->coinbaseMaturity() * 200) {
							if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectCoinbaseUnlinkedOuts]: COINBASE transaction is NOT MATURE ") + 
								strprintf("%d/%d/%s/%s#", lConfirms, lMempool->consensus()->coinbaseMaturity(), lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));
							continue; // skip UTXO
						}
					} else {
						continue; // skip UTXO
					}
				} else {
					gLog().write(Log::WALLET, std::string("[collectCoinbaseUnlinkedOuts]: storage not found for ") + 
						strprintf("%s#", lUtxo->out().chain().toHex().substr(0, 10)));
					continue; // skip UTXO
				}
			}

			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectCoinbaseUnlinkedOuts]: tx found for amount ") + 
				strprintf("%d/%s/%s/%s#", lUtxo->amount(), lUtxo->out().hash().toHex(), lUtxo->out().tx().toHex(), lUtxo->out().chain().toHex().substr(0, 10)));

			list.push_back(lUtxo);

			if (list.size() >= 500) {
				break;
			}
		}
	}

	lAssetTransaction.commit();

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectCoinbaseUnlinkedOuts]: total collected ") + 
		strprintf("%d", list.size()));
}

// fill inputs
amount_t Wallet::fillCoinbaseInputs(TxSpendPtr tx) {
	// collect utxo's
	std::list<Transaction::UnlinkedOutPtr> lUtxos;
	collectCoinbaseUnlinkedOuts(lUtxos);

	// amount
	amount_t lAmount = 0;

	if (lUtxos.size() >= 500) {
		// fill ins 
		for (std::list<Transaction::UnlinkedOutPtr>::iterator lUtxo = lUtxos.begin(); lUtxo != lUtxos.end(); lUtxo++) {
			// locate skey
			SKeyPtr lSKey = findKey((*lUtxo)->address());
			if (lSKey && lSKey->valid()) { 
				tx->addIn(*lSKey, *lUtxo);
				lAmount += (*lUtxo)->amount();
			}
		}
	}

	return lAmount;
}

TransactionContextPtr Wallet::aggregateCoinbaseTxs() {
	// check pending txs (to properly and quickly collect assets balances)
	checkPendingUtxo();

	// create empty tx
	TxSpendPtr lTx = TransactionHelper::to<TxSpend>(TransactionFactory::create(Transaction::SPEND));
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lTx);
	// fill inputs
	std::list<Transaction::UnlinkedOutPtr> lFeeUtxos;
	// collect coinbase confirmed outs
	amount_t lAmount = fillCoinbaseInputs(lTx);
	if (!lAmount) {
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[aggregateCoinbaseTxs]: nothing to aggregate..."));
		return nullptr;
	}

	// fill output
	SKeyPtr lSChangeKey = changeKey();
	SKeyPtr lSKey = firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");
	PKey lDest = lSKey->createPKey(); // main address

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[aggregateCoinbaseTxs]: creating spend tx: ") + 
		strprintf("to = %s, amount = %d, asset = %s#", const_cast<PKey&>(lDest).toString(), lAmount, TxAssetType::qbitAsset().toHex().substr(0, 10)));

	// try to estimate fee
	qunit_t lRate = mempool()->estimateFeeRateByLimit(lCtx, settings_->maxFeeRate());
	amount_t lFee = lRate * lCtx->size();

	// try to check amount
	if (lAmount - lFee > 0) {
		lTx->addOut(*lSKey, lDest, TxAssetType::qbitAsset(), lAmount - lFee); // aggregate
		lTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner

		if (!lTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[aggregateCoinbaseTxs]: spend tx created: ") + 
			strprintf("to = %s, amount = %d/%d, fee = %d, asset = %s#", const_cast<PKey&>(lDest).toString(), lAmount, lAmount-lFee, lFee, TxAssetType::qbitAsset().toHex().substr(0, 10)));

		// write to pending transactions
		pendingtxs_.write(lTx->id(), lTx);

		return lCtx;
	}

	// we do not have enough...
	rollback(lCtx);
	//
	return nullptr;
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
		gLog().write(Log::GENERAL_ERROR, std::string("[wallet/cleanUpData]: ") + ex.what());
	}
}

// dump utxo by asset
void Wallet::dumpUnlinkedOutByAsset(const uint256& asset, std::stringstream& s) {
	//
	for (db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Iterator lAsset = assets_.find(asset); lAsset.valid(); ++lAsset) {
		uint256 lAssetId;
		uint256 lUtxoId;
		lAsset.first(lAssetId, lUtxoId);
		Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(lUtxoId);
		//
		if (lUtxo != nullptr) {
			s << lUtxo->out().toString() << ", amount = " << lUtxo->amount() << "\n";
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
				strprintf("a = %d, %s/%s/%s#", 
					lUtxoPtr->amount(), lUtxoId.toHex(), (*lUtxo)->out().tx().toHex(), (*lUtxo)->out().chain().toHex().substr(0, 10)));

			// remove entry
			removeUtxo(lUtxoId);
			//
			uint256 lTx;
			uint256 lAssetId = lUtxoPtr->out().asset(), lAssetIdA, lUtxoA;
			db::DbTwoKeyContainerShared<uint256 /*asset*/, uint256 /*utxo*/, uint256 /*tx*/>::Iterator lAssetUtxo = assets_.find(lAssetId, lUtxoId);
			if (lUtxoPtr->amount() > 0 && lAssetUtxo.valid() && lAssetUtxo.first(lAssetIdA, lUtxoA) && lUtxoA == lUtxoId) {
				//
				assets_.remove(lAssetId, lUtxoId);
				{
					boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
					amount_t lAmount = 0;
					balance_.read(lAssetId, lAmount);
					if (lAmount >= lUtxoPtr->amount()) lAmount -= lUtxoPtr->amount(); // remove amount
					balance_.write(lAssetId, lAmount);
				}
			}

			//
			utxo_.remove(lUtxoId);
		}
	}

	ctx->newUtxo().clear();

	// recover used utxos
	for (std::list<Transaction::UnlinkedOutPtr>::iterator lUtxo = ctx->usedUtxo().begin(); lUtxo != ctx->usedUtxo().end(); lUtxo++) {
		// locate utxo
		uint256 lUtxoId = (*lUtxo)->hash();
		//
		ltxo_.remove(lUtxoId);
		//
		if (!findUnlinkedOut(lUtxoId)) {
			//
			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[rollback]: reconstruct utxo ") + 
				strprintf("a = %d, %s/%s/%s#", 
					(*lUtxo)->amount(), lUtxoId.toHex(), (*lUtxo)->out().tx().toHex(), (*lUtxo)->out().chain().toHex().substr(0, 10)));
			// recover
			utxo_.write(lUtxoId, *(*lUtxo));
			//
			uint256 lAssetId = (*lUtxo)->out().asset();
			if ((*lUtxo)->amount() > 0) {
				//
				assets_.write(lAssetId, lUtxoId, (*lUtxo)->out().tx());
				{
					boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
					amount_t lAmount = 0;
					balance_.read(lAssetId, lAmount);
					lAmount += (*lUtxo)->amount(); // return amount
					balance_.write(lAssetId, lAmount);
				}
			}
		}
	}

	ctx->usedUtxo().clear();

	return true;
}

// fill inputs
amount_t Wallet::fillInputs(TxSpendPtr tx, const uint256& asset, amount_t amount, bool aggregate, std::list<Transaction::UnlinkedOutPtr>& /*utxos*/) {
	// collect utxo's
	std::list<Transaction::UnlinkedOutPtr> lUtxos;
	collectUnlinkedOutsByAsset(asset, amount, aggregate, lUtxos);

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
TransactionContextPtr Wallet::makeTxSpend(Transaction::Type type, const uint256& asset, const PKey& dest, amount_t amount, qunit_t feeRateLimit, int32_t targetBlock, bool aggregate) {
	// create empty tx
	TxSpendPtr lTx = TransactionHelper::to<TxSpend>(TransactionFactory::create(type)); // TxSendPrivate -> TxSend
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lTx);
	// fill inputs
	std::list<Transaction::UnlinkedOutPtr> lUtxos, lFeeUtxos;
	amount_t lAmount = fillInputs(lTx, asset, amount, aggregate, lUtxos);

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[makeTxSpend]: creating spend tx: ") + 
		strprintf("to = %s, amount = %d, asset = %s#", const_cast<PKey&>(dest).toString(), amount, asset.toHex().substr(0, 10)));

	// fill output
	SKeyPtr lSChangeKey = changeKey();
	SKeyPtr lSKey = firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");

	// check if asset is proof asset and locktime is enabled
	if (asset == settings_->proofAsset() && settings_->proofAssetLockTime() > 0) {
		//
		if (!persistentStoreManager_) throw qbit::exception("E_TX_STOREMANAGER_ABSENT", "StoreManager is absent.");
		//
		BlockHeader lHeader;
		uint64_t lLockHeight = persistentStoreManager_->locate(MainChain::id())->currentHeight(lHeader);
		if (!lLockHeight) throw qbit::exception("E_TX_BASICHEIGHT_UNDEFINED", "Basic chain height is absent.");
		//
		lTx->addLockedOut(*lSKey, dest, asset, amount, lLockHeight + settings_->proofAssetLockTime() + 5 /*extra blocks*/);
	} else {
		// make spend-out
		lTx->addOut(*lSKey, dest, asset, amount);		
	}

	// make change
	if (asset != TxAssetType::qbitAsset() && lAmount > amount) {
		lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, asset, lAmount - amount, true);			
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
				lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, asset, lAmount - (amount + lFee), true);
			}
		} else { // rare cases
			return makeTxSpend(type, asset, dest, amount + lFee, feeRateLimit, targetBlock, aggregate);
		}
	} else {
		// add fee
		amount_t lFeeAmount = fillInputs(lTx, TxAssetType::qbitAsset(), lFee, aggregate, lFeeUtxos);
		lTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
		if (lFeeAmount > lFee) { // make change
			lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee, true);
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
	return makeTxSpend(Transaction::SPEND, asset, dest, amount, feeRateLimit, targetBlock, false);
}

TransactionContextPtr Wallet::createTxSpend(const uint256& asset, const PKey& dest, amount_t amount) {
	qunit_t lRate = settings_->maxFeeRate();
	return makeTxSpend(Transaction::SPEND, asset, dest, amount, lRate, -1, false);
}

TransactionContextPtr Wallet::createTxSpend(const uint256& asset, const PKey& dest, amount_t amount, bool aggregate) {
	qunit_t lRate = settings_->maxFeeRate();
	return makeTxSpend(Transaction::SPEND, asset, dest, amount, lRate, -1, aggregate);
}

// create spend private tx
TransactionContextPtr Wallet::createTxSpendPrivate(const uint256& asset, const PKey& dest, amount_t amount, qunit_t feeRateLimit, int32_t targetBlock) {
	return makeTxSpend(Transaction::SPEND_PRIVATE, asset, dest, amount, feeRateLimit, targetBlock, false);	
}

TransactionContextPtr Wallet::createTxSpendPrivate(const uint256& asset, const PKey& dest, amount_t amount) {
	qunit_t lRate = settings_->maxFeeRate();
	return makeTxSpend(Transaction::SPEND_PRIVATE, asset, dest, amount, lRate, -1, false);	
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
	amount_t lFeeAmount = fillInputs(lAssetTypeTx, TxAssetType::qbitAsset(), lFee, false, lUtxos);
	lAssetTypeTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		lAssetTypeTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee, true);
	}

	if (!lAssetTypeTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	// write to pending transactions
	pendingtxs_.write(lAssetTypeTx->id(), lAssetTypeTx);	
	
	return lCtx;
}

TransactionContextPtr Wallet::createTxAssetType(const PKey& dest, const std::string& shortName, const std::string& longName, amount_t supply, amount_t scale, int chunks, TxAssetType::Emission emission) {
	qunit_t lRate = settings_->maxFeeRate();
	return createTxAssetType(dest, shortName, longName, supply, scale, chunks, emission, lRate, -1);
}

TransactionContextPtr Wallet::createTxAssetType(const PKey& dest, const std::string& shortName, const std::string& longName, amount_t supply, amount_t scale, TxAssetType::Emission emission, qunit_t feeRateLimit, int32_t targetBlock) {
	return createTxAssetType(dest, shortName, longName, supply, scale, 1, emission, feeRateLimit, targetBlock);
}

TransactionContextPtr Wallet::createTxAssetType(const PKey& dest, const std::string& shortName, const std::string& longName, amount_t supply, amount_t scale, TxAssetType::Emission emission) {
	qunit_t lRate = settings_->maxFeeRate();
	return createTxAssetType(dest, shortName, longName, supply, scale, 1, emission, lRate, -1);
}

Transaction::UnlinkedOutPtr Wallet::findUnlinkedOutByEntity(const uint256& entity) {
	//
	db::DbTwoKeyContainerShared<uint256 /*entity*/, uint256 /*utxo*/, uint256 /*tx*/>::Iterator lEntity = assetEntities_.find(entity);
	lEntity.setKey2Empty();
	for (; lEntity.valid(); ++lEntity) {
		uint256 lUtxoId = *lEntity; // utxo hash
		Transaction::UnlinkedOutPtr lUtxo = findUnlinkedOut(lUtxoId);
		//
		if (lUtxo != nullptr) {
			return lUtxo;
		}
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
	amount_t lFeeAmount = fillInputs(lAssetEmissionTx, TxAssetType::qbitAsset(), lFee, false, lUtxos);
	lAssetEmissionTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		lAssetEmissionTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee, true);
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
	amount_t lFeeAmount = fillInputs(lDAppTx, TxAssetType::qbitAsset(), lFee, false, lUtxos);
	lDAppTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		lDAppTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee, true);
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
	amount_t lFeeAmount = fillInputs(lShardTx, TxAssetType::qbitAsset(), lFee, false, lInputsUtxos);
	lShardTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
	if (lFeeAmount > lFee) { // make change
		lShardTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee, true);
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
	amount_t lAmount = fillInputs(lTx, TxAssetType::qbitAsset(), amount, false, lUtxos);

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
			lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lAmount - (amount + lFee), true);
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
	amount_t lAmount = fillInputs(lTx, TxAssetType::qbitAsset(), amount+locked, false, lUtxos);

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
			lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lAmount - (amount + locked + lFee), true);
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