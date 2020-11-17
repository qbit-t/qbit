#include "lightwallet.h"
#include "mkpath.h"
#include "vm/vm.h"

using namespace qbit;

SKeyPtr LightWallet::createKey(const std::list<std::string>& seed) {
	//
	SKey lSKey(seed);
	lSKey.create();

	PKey lPKey = lSKey.createPKey();

	if (opened_) {
		keys_.open(settings_->keysCache());
		if (!keys_.read(lPKey.id(), lSKey)) {
			//
			if (secret_.size()) lSKey.encrypt(secret_);
			if (!keys_.write(lPKey.id(), lSKey)) {
				throw qbit::exception("WRITE_FAILED", "Write failed.");
			}
		}
	}

	SKeyPtr lSKeyPtr = SKey::instance(lSKey);
	if (lSKeyPtr->encrypted()) lSKeyPtr->decrypt(secret_);
	{
		boost::unique_lock<boost::recursive_mutex> lLock(keyMutex_);
		keysCache_[lPKey.id()] = lSKeyPtr;
	}

	return lSKeyPtr;
}

SKeyPtr LightWallet::findKey(const PKey& pkey) {
	if (pkey.valid()) {
		//
		boost::unique_lock<boost::recursive_mutex> lLock(keyMutex_);
		std::map<uint160 /*id*/, SKeyPtr>::iterator lSKeyIter = keysCache_.find(pkey.id());
		if (lSKeyIter != keysCache_.end())
			return lSKeyIter->second;

		SKey lSKey;
		if (keys_.read(pkey.id(), lSKey)) {
			SKeyPtr lSKeyPtr = SKey::instance(lSKey);
			if (lSKeyPtr->encrypted()) lSKeyPtr->decrypt(secret_);
			keysCache_[pkey.id()] = lSKeyPtr;

			return lSKeyPtr;
		}
	}

	return nullptr;
}

SKeyPtr LightWallet::firstKey() {
	//
	{
		boost::unique_lock<boost::recursive_mutex> lLock(keyMutex_);
		std::map<uint160 /*id*/, SKeyPtr>::iterator lSKeyIter = keysCache_.begin();
		if (lSKeyIter != keysCache_.end()) return lSKeyIter->second;
	}

	if (opened_) {
		db::DbContainer<uint160 /*id*/, SKey>::Iterator lKey = keys_.begin();
		if (lKey.valid()) {
			SKeyPtr lSKeyPtr = SKey::instance(*lKey);
			if (lSKeyPtr->encrypted()) lSKeyPtr->decrypt(secret_);

			uint160 lId;
			if (lKey.first(lId)) {
				boost::unique_lock<boost::recursive_mutex> lLock(keyMutex_);
				keysCache_[lId] = lSKeyPtr;
				return lSKeyPtr;
			}
		}
	}

	// try create from scratch
	return createKey(std::list<std::string>());
}

SKeyPtr LightWallet::changeKey() {
	//
	return firstKey();
}

void LightWallet::removeAllKeys() {
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

bool LightWallet::open(const std::string& secret) {
	if (!opened_ && !gLightDaemon) {
		try {
			if (mkpath(std::string(settings_->dataPath() + "/wallet").c_str(), 0777)) return false;

			keys_.open();
			utxo_.open();
			outs_.open();
			index_.open();
			value_.open();
			secret_ = secret;

			opened_ = true;
		}
		catch(const qbit::db::exception& ex) {
			gLog().write(Log::ERROR, std::string("[wallet/open]: ") + ex.what());
			return false;
		}
		catch(const std::exception& ex) {
			gLog().write(Log::ERROR, std::string("[wallet/open]: ") + ex.what());
			return false;
		}
	}

	return true;
}

bool LightWallet::close() {
	gLog().write(Log::INFO, std::string("[wallet/close]: closing wallet data..."));

	settings_.reset();
	requestProcessor_.reset();

	if (opened_) {
		keys_.close();
		utxo_.close();
		outs_.close();
		index_.close();
	}

	return true;
}

void LightWallet::refetchTransactions() {
	//
	status_ = IWallet::FETCHING_TXS;
	for (std::map<uint256 /*id*/, Transaction::NetworkUnlinkedOutPtr /*utxo*/>::iterator lOut = utxoCache_.begin(); lOut != utxoCache_.end(); lOut++) {
		uint256 lUtxoId = lOut->second->utxo().hash();
		if (lOut->second->utxo().amount() == 0) {
			// probably blinded out
			Transaction::NetworkUnlinkedOut lUtxoObj;
			if (!utxo_.read(lUtxoId, lUtxoObj)) {
				// request for tx
				if (status_ == IWallet::FETCHING_UTXO) status_ = IWallet::FETCHING_TXS;
				if (!requestProcessor_->loadTransaction(lUtxoObj.utxo().out().chain(), lUtxoObj.utxo().out().tx(), shared_from_this())) {
					status_ = IWallet::TX_INFO_NOT_FOUND;
				}
			} else {
				utxoCache_[lUtxoId] = Transaction::NetworkUnlinkedOut::instance(lUtxoObj); // re-create with new confirms
				utxoCache_[lUtxoId]->setUtxo(lUtxoObj.utxo()); // update utxo
			}
		}
	}

	if (status_ == IWallet::FETCHING_TXS) {
		status_ = IWallet::OPENED;
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: cache is ready"));
	} else {
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: tx info was not found, retry..."));
	}
}

TransactionContextPtr LightWallet::processTransaction(TransactionPtr tx) {
	//
	// process tx
	// extract our outs 
	uint32_t lIdx = 0;
	TransactionContextPtr lWrapper = TransactionContext::instance(tx);
	std::vector<Transaction::Out>& lOuts = tx->out();
	for(std::vector<Transaction::Out>::iterator lOutPtr = lOuts.begin(); lOutPtr != lOuts.end(); lOutPtr++, lIdx++) {

		Transaction::Out& lOut = (*lOutPtr);
		VirtualMachine lVM(lOut.destination());
		lVM.getR(qasm::QTH0).set(tx->id());
		lVM.getR(qasm::QTH1).set((unsigned short)tx->type());
		lVM.getR(qasm::QTH3).set(lIdx); // out number
		lVM.setTransaction(lWrapper);
		lVM.setWallet(shared_from_this());
		lVM.setTransactionStore(TxBlockStore::instance());
		lVM.setEntityStore(TxEntityStore::instance());

		lVM.execute();

		// WARNING:
		// VM state is not important - pushUnlinkedOut should be happened in case if we have our outs
	}

	return lWrapper;
}

void LightWallet::handleReply(TransactionPtr tx) {
	//
	processTransaction(tx);

	status_ = IWallet::OPENED;
	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: cache is ready"));
}

void LightWallet::handleReply(const std::vector<Transaction::NetworkUnlinkedOut>& outs, const PKey& pkey) {
	//
	if (gLog().isEnabled(Log::WALLET)) 
		gLog().write(Log::WALLET, strprintf("[prepareCache]: outs.size() = %d", outs.size()));
	//
	for (std::vector<Transaction::NetworkUnlinkedOut>::const_iterator lOut = outs.begin(); lOut != outs.end(); lOut++) {
		Transaction::NetworkUnlinkedOut& lOutRef = const_cast<Transaction::NetworkUnlinkedOut&>(*lOut);
		uint256 lUtxoId = lOutRef.utxo().hash();
		//
		if (gLog().isEnabled(Log::WALLET)) 
			gLog().write(Log::WALLET, strprintf("[prepareCache]: amount = %d, %s", lOutRef.utxo().amount(), lOutRef.utxo().out().toString()));

		if (lOutRef.utxo().amount() != 0) {
			// make map
			assetsCache_[lOutRef.utxo().out().asset()].insert(std::multimap<amount_t, uint256>::value_type(lOutRef.utxo().amount(), lUtxoId));
			utxoCache_[lUtxoId] = Transaction::NetworkUnlinkedOut::instance(lOutRef);
		} else {
			// probably blinded out (re-cache)
			Transaction::NetworkUnlinkedOut lUtxoObj;
			// pre-cache
			utxoCache_[lUtxoId] = Transaction::NetworkUnlinkedOut::instance(lOutRef);
			// request for tx
			if (status_ == IWallet::FETCHING_UTXO) status_ = IWallet::FETCHING_TXS;
			if (!requestProcessor_->loadTransaction(lOutRef.utxo().out().chain(), lOutRef.utxo().out().tx(), shared_from_this())) {
				status_ = IWallet::TX_INFO_NOT_FOUND;
			}
		}
	}

	if (status_ == IWallet::FETCHING_UTXO) {
		status_ = IWallet::OPENED;
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: cache is ready"));
	} else if (status_ == IWallet::FETCHING_TXS) {
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: fetching txs..."));
	} else {
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: tx info was not found, retry..."));
	}
}

bool LightWallet::prepareCache() {
	//
	resetCache();

	//
	if (opened_) {
		//
		status_ = IWallet::FETCHING_UTXO;

		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: selecting utxo..."));
		if (!requestProcessor_->selectUtxoByAddress(firstKey()->createPKey(), MainChain::id(), shared_from_this())) {
			status_ = IWallet::CACHE_NOT_READY;
			//
			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: utxo cache is NOT ready, retry..."));
			return false;
		}

		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: fetching utxo..."));
	}

	return true;
}

bool LightWallet::pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr ctx) {
	//
	uint256 lUtxoId = utxo->hash();

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: ") + 
		strprintf("try to push utxo = %s, tx = %s, ctx = %s/%d/%s#", 
			utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), ctx->tx()->confirms(), utxo->out().chain().toHex().substr(0, 10)));

	// update utxo db
	std::map<uint256 /*id*/, Transaction::NetworkUnlinkedOutPtr /*utxo*/>::iterator lItem = utxoCache_.find(lUtxoId);
	if (lItem != utxoCache_.end()) {
		//
		uint256 lAssetId = utxo->out().asset();
		if (ctx->tx()->isValue(utxo)) {
			boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
			removeFromAssetsCache(Transaction::UnlinkedOut::instance(lItem->second->utxo()));
			assetsCache_[lAssetId].insert(std::multimap<amount_t, uint256>::value_type(utxo->amount(), lUtxoId));
		}

		lItem->second->setUtxo(*utxo);
		if (opened_) {
			lItem->second->setConfirms(ctx->tx()->confirms() /*updated*/);
			utxo_.write(lUtxoId, *lItem->second);
			updateIn(lItem->second);
		}

		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: pushed ") + 
			strprintf("utxo = %s, tx = %s, ctx = %s/%d/%s#", 
				utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), ctx->tx()->confirms(), utxo->out().chain().toHex().substr(0, 10)));
	} else {
		Transaction::NetworkUnlinkedOutPtr lNetworkUtxo;
		{
			boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
			// add new one
			lNetworkUtxo = Transaction::NetworkUnlinkedOut::instance(*utxo, /*settings_->mainChainMaturity()*/ 
																			ctx->tx()->confirms(), false);
			utxoCache_[lUtxoId] = lNetworkUtxo;
			assetsCache_[utxo->out().asset()].insert(std::multimap<amount_t, uint256>::value_type(utxo->amount(), lUtxoId));
		}

		if (opened_) {
			utxo_.write(lUtxoId, *lNetworkUtxo);
			updateIn(lNetworkUtxo);
		}

		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: ") + 
			strprintf("PUSHED utxo = %s, tx = %s, ctx = %s/%d/%s#", 
				utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), ctx->tx()->confirms(), utxo->out().chain().toHex().substr(0, 10)));
	}

	// notify
	walletReceiveTransaction(utxo, ctx->tx());

	return true;
}

bool LightWallet::popUnlinkedOut(const uint256& hash, TransactionContextPtr ctx) {
	//
	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[popUnlinkedOut]: ") + 
		strprintf("try to pop utxo = %s, tx = ?, ctx = %s/%s#", 
			hash.toHex(), ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));

	Transaction::NetworkUnlinkedOutPtr lUtxoPtr = findNetworkUnlinkedOut(hash);
	if (lUtxoPtr) {

		// correct balance
		uint256 lAssetId = lUtxoPtr->utxo().out().asset();

		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[popUnlinkedOut]: ") + 
			strprintf("popped utxo = %s, tx = %s, ctx = %s/%s#", 
				hash.toHex(), lUtxoPtr->utxo().out().tx().toHex(), ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));

		{
			boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
			// try cleanup
			std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(lAssetId);
			if (lAsset != assetsCache_.end()) {
				std::pair<std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator,
							std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator> lRange = lAsset->second.equal_range(lUtxoPtr->utxo().amount());
				for (std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator lItem = lRange.first; lItem != lRange.second; lItem++) {
					if (lItem->second == hash) {
						lAsset->second.erase(lItem);
						break;
					}
				}
			}
		}

		if (opened_) utxo_.remove(hash);
		utxoCache_.erase(hash);

		return true;
	}

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[popUnlinkedOut]: ") + 
		strprintf("link NOT FOUND for utxo = %s, tx = ?, ctx = %s/%s#",
			hash.toHex(), ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));

	return false;
}

Transaction::NetworkUnlinkedOutPtr LightWallet::findNetworkUnlinkedOut(const uint256& out) {
	//
	Transaction::NetworkUnlinkedOutPtr lUtxoPtr = nullptr;
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
	std::map<uint256 /*id*/, Transaction::NetworkUnlinkedOutPtr /*utxo*/>::iterator lItem = utxoCache_.find(out);
	if (lItem != utxoCache_.end()) {
		lUtxoPtr = lItem->second;
	}

	return lUtxoPtr;
}

void LightWallet::removeFromAssetsCache(Transaction::UnlinkedOutPtr utxo) {
	//
	uint256 lId = utxo->hash();
	uint256 lAssetId = utxo->out().asset();

	// try clean-up cache
	std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(lAssetId);
	if (lAsset != assetsCache_.end()) {
		std::pair<std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator,
					std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator> lRange = lAsset->second.equal_range(utxo->amount());
		for (std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator lItem = lRange.first; lItem != lRange.second; lItem++) {
			if (lItem->second == lId) {
				lAsset->second.erase(lItem);
				if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[removeUnlinkedOut]: ") + 
					strprintf("REMOVED utxo = %s, tx = %s/%s#", 
						lItem->second.toHex(), utxo->out().tx().toHex(), utxo->out().chain().toHex().substr(0, 10)));
				break;
			}
		}
	}
}

void LightWallet::removeUnlinkedOut(std::list<Transaction::UnlinkedOutPtr>& utxos) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
	for (std::list<Transaction::UnlinkedOutPtr>::iterator lUtxo = utxos.begin(); lUtxo != utxos.end(); lUtxo++) { 
		uint256 lId = (*lUtxo)->hash();

		utxoCache_.erase(lId);
		if (opened_) utxo_.remove(lId);

		removeFromAssetsCache(*lUtxo);
	}
}

void LightWallet::removeUnlinkedOut(Transaction::UnlinkedOutPtr utxo) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
	uint256 lId = utxo->hash();

	utxoCache_.erase(lId);
	if (opened_) utxo_.remove(lId);

	removeFromAssetsCache(utxo);
}

amount_t LightWallet::pendingBalance(const uint256& asset) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
	//
	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + strprintf("computing balance for %s", asset.toHex()));
	//
	amount_t lBalance = 0;
	std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(asset);
	if (lAsset != assetsCache_.end()) {
		for (std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator lAmount = lAsset->second.begin(); 
			lAmount != lAsset->second.end(); lAmount++) {

			if (lAmount->first == 0) // amount is zero
				continue;

			Transaction::NetworkUnlinkedOutPtr lUtxo = findNetworkUnlinkedOut(lAmount->second);

			if (lUtxo == nullptr) {
				//
				if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + 
							strprintf("utxo NOT FOUND %s/%s", lAmount->second.toHex(), asset.toHex()));
				// delete from store
				utxo_.remove(lAmount->second);
				utxoCache_.erase(lAmount->second);
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

amount_t LightWallet::balance(const uint256& asset) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
	//
	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + strprintf("computing balance for %s", asset.toHex()));
	//
	amount_t lBalance = 0;
	std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(asset);
	if (lAsset != assetsCache_.end()) {
		for (std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::iterator lAmount = lAsset->second.begin(); 
			lAmount != lAsset->second.end();) {

			if (lAmount->first == 0) // amount is zero
				continue;

			Transaction::NetworkUnlinkedOutPtr lUtxo = findNetworkUnlinkedOut(lAmount->second);

			if (lUtxo == nullptr) {
				//
				if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + 
							strprintf("utxo NOT FOUND %s/%s", lAmount->second.toHex(), asset.toHex()));
				// delete from store
				if (opened_) utxo_.remove(lAmount->second);
				utxoCache_.erase(lAmount->second);
				lAsset->second.erase(lAmount);
				lAmount++;
			} else {
				//
				if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: ") + 
							strprintf("utxo FOUND %d/%s/%s", lAmount->first, lAmount->second.toHex(), asset.toHex()));

				// extra check
				/*if (lAsset->second.size() > 1)*/ {
					//
					if (!lUtxo->coinbase() && lUtxo->confirms() < settings_->mainChainMaturity()) {
						if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: transaction is NOT MATURE ") + 
							strprintf("%d/%d/%s/%s#", lUtxo->confirms(), settings_->mainChainMaturity(), 
								lUtxo->utxo().out().tx().toHex(), lUtxo->utxo().out().chain().toHex().substr(0, 10)));
						lAmount++;
						continue; // skip UTXO
					} else if (lUtxo->coinbase() && lUtxo->confirms() < settings_->mainChainCoinbaseMaturity()) {
						//
						// NOTICE: this case is not for light wallet (supposely)
						//
						if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: COINBASE transaction is NOT MATURE ") + 
							strprintf("%d/%d/%s/%s#", lUtxo->confirms(), settings_->mainChainCoinbaseMaturity(), 
								lUtxo->utxo().out().tx().toHex(), lUtxo->utxo().out().chain().toHex().substr(0, 10)));
						lAmount++;
						continue; // skip UTXO
					} else if (lUtxo->utxo().lock()) {
						uint64_t lHeight = requestProcessor_->locateHeight(MainChain::id()); // only main chain can contain value transactions

						if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: check lock ") + 
							strprintf("%d/%d/%s/%s#", lUtxo->utxo().lock(), lHeight, 
								lUtxo->utxo().out().tx().toHex(), lUtxo->utxo().out().chain().toHex().substr(0, 10)));

						if (lUtxo->utxo().lock() > lHeight) {
							if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[balance]: timelock is not REACHED ") + 
								strprintf("%d/%d/%s/%s#", lUtxo->utxo().lock(), lHeight, 
									lUtxo->utxo().out().tx().toHex(), lUtxo->utxo().out().chain().toHex().substr(0, 10)));
							lAmount++;
							continue; // skip UTXO
						}
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

void LightWallet::collectUnlinkedOutsByAsset(const uint256& asset, amount_t amount, std::list<Transaction::UnlinkedOutPtr>& list) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
	std::map<uint256 /*asset*/, std::multimap<amount_t /*amount*/, uint256 /*utxo*/>>::iterator lAsset = assetsCache_.find(asset);
	amount_t lTotal = 0;
	if (lAsset != assetsCache_.end()) {
		for (std::multimap<amount_t /*amount*/, uint256 /*utxo*/>::reverse_iterator lAmount = lAsset->second.rbegin(); 
			lAmount != lAsset->second.rend();) {
			
			Transaction::NetworkUnlinkedOutPtr lUtxo = findNetworkUnlinkedOut(lAmount->second);
			if (lUtxo == nullptr) {
				// delete from store
				if (opened_) utxo_.remove(lAmount->second);
				utxoCache_.erase(lAmount->second);
				lAsset->second.erase(std::next(lAmount).base());
				continue;
			}

			// extra check
			/*if (lAsset->second.size() > 1)*/ {
				//
				if (!lUtxo->coinbase() && lUtxo->confirms() < settings_->mainChainMaturity()) {
					if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: transaction is NOT MATURE ") + 
						strprintf("%d/%d/%s/%s#", lUtxo->confirms(), settings_->mainChainMaturity(), 
							lUtxo->utxo().out().tx().toHex(), lUtxo->utxo().out().chain().toHex().substr(0, 10)));
					lAmount++;
					continue; // skip UTXO
				} else if (lUtxo->coinbase() && lUtxo->confirms() < settings_->mainChainCoinbaseMaturity()) {
					//
					// NOTICE: this case is not for light wallet (supposely)
					//					
					if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: COINBASE transaction is NOT MATURE ") + 
						strprintf("%d/%d/%s/%s#", lUtxo->confirms(), settings_->mainChainCoinbaseMaturity(), 
							lUtxo->utxo().out().tx().toHex(), lUtxo->utxo().out().chain().toHex().substr(0, 10)));
					lAmount++;
					continue; // skip UTXO
				} else if (lUtxo->utxo().lock()) {
					uint64_t lHeight = requestProcessor_->locateHeight(MainChain::id()); // only main chain can contain value transactions
					if (lUtxo->utxo().lock() > lHeight) {
						if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: timelock is not REACHED ") + 
							strprintf("%d/%d/%s/%s#", lUtxo->utxo().lock(), lHeight, 
								lUtxo->utxo().out().tx().toHex(), lUtxo->utxo().out().chain().toHex().substr(0, 10)));
						lAmount++;
						continue; // skip UTXO
					}
				}
			}

			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: tx found for amount ") + 
				strprintf("%d/%s/%s/%s#", lAmount->first, lUtxo->utxo().out().hash().toHex(), 
					lUtxo->utxo().out().tx().toHex(), lUtxo->utxo().out().chain().toHex().substr(0, 10)));

			lTotal += lAmount->first;
			list.push_back(Transaction::UnlinkedOut::instance(lUtxo->utxo()));

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

//
bool LightWallet::isTimelockReached(const uint256& utxo) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
	Transaction::NetworkUnlinkedOut lOut;
	std::map<uint256 /*id*/, Transaction::NetworkUnlinkedOutPtr /*utxo*/>::iterator lUTXO = utxoCache_.find(utxo);

	uint64_t lTimelock = 0;
	if (lUTXO == utxoCache_.end()) {
		if (outs_.read(utxo, lOut)) {
			lTimelock = lOut.utxo().lock();
		}
	} else {
		lTimelock = lUTXO->second->utxo().lock();
	}

	//
	uint64_t lHeight = requestProcessor_->locateHeight(MainChain::id()); // only main chain can contain value transactions
	//if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[isTimelockReached]: ") +
	//	strprintf("height = %d, timelock = %d, utxo = %s", lHeight, lTimelock, utxo.toHex()));
	if (lTimelock <= lHeight)
		return true;

	//if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[isTimelockReached]: ") +
	//	strprintf("utxo = %s was NOT found", utxo.toHex()));
	return false;
}

// clean-up assets utxo
void LightWallet::cleanUp() {
}

void LightWallet::cleanUpData() {
	//
	if (!opened_) return;

	utxo_.close();

	// clean up
	std::string lPath = settings_->dataPath() + "/wallet/utxo";
	rmpath(lPath.c_str());

	// re-open
	try {
		utxo_.open();
	}
	catch(const std::exception& ex) {
		gLog().write(Log::ERROR, std::string("[wallet/cleanUpData]: ") + ex.what());
	}
}

// rollback tx
bool LightWallet::rollback(TransactionContextPtr ctx) {
	//
	gLog().write(Log::WALLET, std::string("[rollback]: rollback tx ") + 
		strprintf("%s/%s#", ctx->tx()->id().toHex(), ctx->tx()->chain().toHex().substr(0, 10)));
	// recover new utxos
	for (std::list<Transaction::UnlinkedOutPtr>::iterator lUtxo = ctx->newUtxo().begin(); lUtxo != ctx->newUtxo().end(); lUtxo++) {
		// locate utxo
		uint256 lUtxoId = (*lUtxo)->out().hash();
		Transaction::NetworkUnlinkedOutPtr lUtxoPtr = findNetworkUnlinkedOut(lUtxoId);
		if (lUtxoPtr) {
			//
			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[rollback]: remove utxo ") + 
				strprintf("%s/%s/%s#", 
					lUtxoId.toHex(), (*lUtxo)->out().tx().toHex(), (*lUtxo)->out().chain().toHex().substr(0, 10)));

			// remove entry
			removeUnlinkedOut(Transaction::UnlinkedOut::instance(lUtxoPtr->utxo()));
		}
	}

	ctx->newUtxo().clear();

	// recover used utxos
	for (std::list<Transaction::UnlinkedOutPtr>::iterator lUtxo = ctx->usedUtxo().begin(); lUtxo != ctx->usedUtxo().end(); lUtxo++) {
		// locate utxo
		uint256 lUtxoId = (*lUtxo)->hash();
		uint256 lAssetId = (*lUtxo)->out().asset();

		if (!findNetworkUnlinkedOut(lUtxoId)) {
			//
			if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[rollback]: reconstruct utxo ") + 
				strprintf("%s/%s/%s#", 
					lUtxoId.toHex(), (*lUtxo)->out().tx().toHex(), (*lUtxo)->out().chain().toHex().substr(0, 10)));
			// recover
			cacheUnlinkedOut((*lUtxo));
		}
	}

	ctx->usedUtxo().clear();
	
	return true;
}

// fill inputs
amount_t LightWallet::fillInputs(TxSpendPtr tx, const uint256& asset, amount_t amount, std::list<Transaction::UnlinkedOutPtr>& utxos) {
	// collect utxo's
	collectUnlinkedOutsByAsset(asset, amount, utxos);

	if (!utxos.size()) throw qbit::exception("E_AMOUNT", "Insufficient amount.");

	// amount
	amount_t lAmount = 0;

	// fill ins 
	for (std::list<Transaction::UnlinkedOutPtr>::iterator lUtxo = utxos.begin(); lUtxo != utxos.end(); lUtxo++) {
		// locate skey
		SKeyPtr lSKey = findKey((*lUtxo)->address());
		if (lSKey && lSKey->valid()) { 
			tx->addIn(*lSKey, *lUtxo);
			lAmount += (*lUtxo)->amount();
		}
	}	

	return lAmount;
}

void LightWallet::cacheUnlinkedOut(Transaction::UnlinkedOutPtr utxo) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
	uint256 lUtxo = utxo->hash();
	utxoCache_[lUtxo] = 
		Transaction::NetworkUnlinkedOut::instance(*utxo, settings_->mainChainMaturity(), false);
	assetsCache_[utxo->out().asset()].insert(std::multimap<amount_t, uint256>::value_type(utxo->amount(), lUtxo));
}

// make tx spend...
TransactionContextPtr LightWallet::makeTxSpend(Transaction::Type type, const uint256& asset, const PKey& dest, amount_t amount, qunit_t feeRateLimit) {
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
	Transaction::UnlinkedOutPtr lMainOut = lTx->addOut(*lSKey, dest, asset, amount);

	// make change
	if (asset != TxAssetType::qbitAsset() && lAmount > amount) {
		lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, asset, lAmount - amount);			
	}

	// change
	Transaction::UnlinkedOutPtr lChangeUtxo = nullptr;

	// TODO: try to estimate fee
	qunit_t lRate = feeRateLimit;
	amount_t lFee = lRate * lCtx->size(); 
	if (asset == TxAssetType::qbitAsset()) { // if qbit sending
		// try to check amount
		if (lAmount >= amount + lFee) {
			lTx->addFeeOut(*lSKey, asset, lFee); // to miner

			if (lAmount > amount + lFee) { // make change
				lChangeUtxo = lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, asset, lAmount - (amount + lFee), true);
			}
		} else { // rare cases
			return makeTxSpend(type, asset, dest, amount + lFee, feeRateLimit);
		}
	} else {
		// add fee
		amount_t lFeeAmount = fillInputs(lTx, TxAssetType::qbitAsset(), lFee, lFeeUtxos);
		lTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner
		if (lFeeAmount > lFee) { // make change
			lChangeUtxo = lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee, true);
		}
	}

	if (!lTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	// add out
	lCtx->addExternalOut(Transaction::NetworkUnlinkedOut::instance(*lMainOut, type, 0, false, lFee));
	lCtx->addAmount(amount); // amount info
	lCtx->addFee(lFee); // fee info

	// cache used outs - for rollback
	for (std::list<Transaction::UnlinkedOutPtr>::iterator lOut = lUtxos.begin(); lOut != lUtxos.end(); lOut++) {
		//
		lCtx->addUsedUnlinkedOut(*lOut);
	}

	for (std::list<Transaction::UnlinkedOutPtr>::iterator lOut = lFeeUtxos.begin(); lOut != lFeeUtxos.end(); lOut++) {
		//
		lCtx->addUsedUnlinkedOut(*lOut);
	}

	// we good
	removeUnlinkedOut(lUtxos);
	removeUnlinkedOut(lFeeUtxos);

	if (lChangeUtxo) { 
		// cache new utxo
		lCtx->addNewUnlinkedOut(lChangeUtxo);
		cacheUnlinkedOut(lChangeUtxo);

		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[makeTxSpend]: ") + 
			strprintf("PUSHED utxo = %s, amount = %d, tx = %s/%s#", 
				lChangeUtxo->hash().toHex(), lChangeUtxo->amount(), lTx->id().toHex(), lChangeUtxo->out().chain().toHex().substr(0, 10)));		
	}

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[makeTxSpend]: spend tx created: ") + 
		strprintf("to = %s, amount = %d/%d, fee = %d, asset = %s#", const_cast<PKey&>(dest).toString(), amount, lAmount, lFee, asset.toHex().substr(0, 10)));

	return lCtx;
}

// create spend tx
TransactionContextPtr LightWallet::createTxSpend(const uint256& asset, const PKey& dest, amount_t amount, qunit_t feeRateLimit, int32_t targetBlock) {
	return makeTxSpend(Transaction::SPEND, asset, dest, amount, feeRateLimit);
}

TransactionContextPtr LightWallet::createTxSpend(const uint256& asset, const PKey& dest, amount_t amount) {
	qunit_t lRate = settings_->maxFeeRate();
	return makeTxSpend(Transaction::SPEND, asset, dest, amount, lRate);
}

// create spend private tx
TransactionContextPtr LightWallet::createTxSpendPrivate(const uint256& asset, const PKey& dest, amount_t amount, qunit_t feeRateLimit, int32_t targetBlock) {
	return makeTxSpend(Transaction::SPEND_PRIVATE, asset, dest, amount, feeRateLimit);	
}

TransactionContextPtr LightWallet::createTxSpendPrivate(const uint256& asset, const PKey& dest, amount_t amount) {
	qunit_t lRate = settings_->maxFeeRate();
	return makeTxSpend(Transaction::SPEND_PRIVATE, asset, dest, amount, lRate);	
}

TransactionContextPtr LightWallet::createTxFee(const PKey& dest, amount_t amount) {
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
	Transaction::UnlinkedOutPtr lMainOut = lTx->addExternalOut(*lSKey, dest, TxAssetType::qbitAsset(), amount); // out[0] - use to pay fee in shards

	amount_t lRate = settings_->maxFeeRate();
	amount_t lFee = (lRate * lCtx->size()) / 2; // half fee 

	// change
	Transaction::UnlinkedOutPtr lChangeUtxo = nullptr;

	// try to check amount
	if (lAmount >= amount + lFee) {
		lTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner

		if (lAmount > amount + lFee) { // make change
			lChangeUtxo = lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lAmount - (amount + lFee), true);
		}
	} else { // rare cases
		return createTxFee(dest, amount + lFee);
	}

	if (!lTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	// add out
	lCtx->addExternalOut(Transaction::NetworkUnlinkedOut::instance(*lMainOut, Transaction::FEE, 0, false, lFee));

	// we good
	removeUnlinkedOut(lUtxos);

	if (lChangeUtxo) { 
		cacheUnlinkedOut(lChangeUtxo);
	}

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[createTxFee]: fee tx created: ") + 
		strprintf("to = %s, amount = %d/%d", const_cast<PKey&>(dest).toString(), amount, lAmount));

	return lCtx;
}

TransactionContextPtr LightWallet::createTxFeeLockedChange(const PKey& dest, amount_t amount, amount_t locked, uint64_t height) {
	// create empty tx
	TxFeePtr lTx = TransactionHelper::to<TxFee>(TransactionFactory::create(Transaction::FEE)); // TxSendPrivate -> TxSend
	// create context
	TransactionContextPtr lCtx = TransactionContext::instance(lTx);
	// fill inputs
	std::list<Transaction::UnlinkedOutPtr> lUtxos;	
	amount_t lAmount = fillInputs(lTx, TxAssetType::qbitAsset(), amount + locked, lUtxos);

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[createTxFeeLockedChange]: creating fee tx: ") + 
		strprintf("to = %s, amount = %d", const_cast<PKey&>(dest).toString(), amount + locked));

	// fill output
	SKeyPtr lSChangeKey = changeKey();
	SKeyPtr lSKey = firstKey();
	if (!lSKey->valid() || !lSChangeKey->valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");
	Transaction::UnlinkedOutPtr lMainOut = lTx->addExternalOut(*lSKey, dest, TxAssetType::qbitAsset(), amount); // out[0] - use to pay fee in shards

	Transaction::UnlinkedOutPtr lLockedUtxo =
		lTx->addLockedOut(*lSKey, dest, TxAssetType::qbitAsset(), locked, height); // out[1] - locked

	amount_t lRate = settings_->maxFeeRate();
	amount_t lFee = (lRate * lCtx->size()) / 2; // half fee 

	// change
	Transaction::UnlinkedOutPtr lChangeUtxo = nullptr;

	// try to check amount
	if (lAmount >= amount + locked + lFee) {
		lTx->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFee); // to miner

		if (lAmount > amount + locked + lFee) { // make change
			lChangeUtxo = lTx->addOut(*lSChangeKey, lSChangeKey->createPKey()/*change*/, TxAssetType::qbitAsset(), lAmount - (amount + locked + lFee), true);
		}
	} else { // rare cases
		return createTxFeeLockedChange(dest, amount + lFee, locked, height);
	}

	if (!lTx->finalize(*lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	if (opened_) {
		// add outs
		lCtx->addExternalOut(Transaction::NetworkUnlinkedOut::instance(*lMainOut, Transaction::FEE, 0, false, lFee));
		lCtx->addExternalOut(Transaction::NetworkUnlinkedOut::instance(*lLockedUtxo, Transaction::FEE, 0, false, lFee));
	}

	// we good
	removeUnlinkedOut(lUtxos);

	if (lChangeUtxo) { 
		cacheUnlinkedOut(lChangeUtxo);
	}

	if (lLockedUtxo) { 
		cacheUnlinkedOut(lLockedUtxo);
	}

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[createTxFeeLockedChange]: fee tx created: ") + 
		strprintf("to = %s, amount = [%d/%d]%d", const_cast<PKey&>(dest).toString(), amount, locked, lAmount));

	return lCtx;
}

void LightWallet::updateOut(Transaction::NetworkUnlinkedOutPtr out, const uint256& parent, unsigned short type) {
	//
	out->setParent(parent);
	out->setParentType(type);
	out->setDirection(Transaction::NetworkUnlinkedOut::Direction::OUT);

	//
	Transaction::NetworkUnlinkedOut lOut;
	if (!outs_.read(out->utxo().hash(), lOut)) {
		// try to reconstrust timstamp
		uint64_t lTimestamp = out->confirms();
		if (!lTimestamp) { 
			lTimestamp = qbit::getMedianMicroseconds();
		} else {
			lTimestamp = qbit::getMedianMicroseconds() - lTimestamp * settings_->mainChainBlockTime() * 1000 /*microseconds*/;
		}

		out->setTimestamp(lTimestamp);
		index_.write(strprintf("%d", lTimestamp), out->utxo().hash());
		//
		// TODO: only send, sendprivate?
		//
		value_.write(Transaction::NetworkUnlinkedOut::Direction::OUT, strprintf("%d", lTimestamp), out->utxo().hash());
	} else {
		out->setTimestamp(lOut.timestamp());
	}

	//
	if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[updateOut]: timestamp = %d, confirms = %d", out->timestamp(), out->confirms()));
	//
	outs_.write(out->utxo().hash(), *out);
	outUpdated(out);
}

void LightWallet::updateOuts(TransactionPtr tx) {
	//
	uint32_t lIdx = 0;
	for (std::vector<Transaction::Out>::iterator lOut = tx->out().begin(); lOut != tx->out().end(); lOut++, lIdx++) {
		//
		// make link
		Transaction::Link lLink;
		lLink.setChain(tx->chain());
		lLink.setAsset(lOut->asset());
		lLink.setTx(tx->id());
		lLink.setIndex(lIdx);

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			lLink // link
		);

		Transaction::NetworkUnlinkedOut lPersistedOut;
		if (outs_.read(lUTXO->hash(), lPersistedOut)) {
			lPersistedOut.setConfirms(1); // trick
			outs_.write(lUTXO->hash(), lPersistedOut); // update
			outUpdated(Transaction::NetworkUnlinkedOut::instance(lPersistedOut));
		}
	}
}

void LightWallet::updateIn(Transaction::NetworkUnlinkedOutPtr out) {
	//
	Transaction::NetworkUnlinkedOut lOut;
	if (!outs_.read(out->utxo().hash(), lOut)) {
		//
		// try to reconstrust timstamp
		uint64_t lTimestamp = out->confirms();
		if (!lTimestamp) { 
			lTimestamp = qbit::getMedianMicroseconds();
		} else {
			lTimestamp = qbit::getMedianMicroseconds() - lTimestamp * settings_->mainChainBlockTime() * 1000 /*microseconds*/;
		}

		out->setTimestamp(lTimestamp);
		index_.write(strprintf("%d", lTimestamp), out->utxo().hash());
		//
		// TODO: only send, sendprivate?
		//
		value_.write(Transaction::NetworkUnlinkedOut::Direction::IN, strprintf("%d", lTimestamp), out->utxo().hash());
	} else {
		out->setTimestamp(lOut.timestamp());
	}

	//
	if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[updateIn]: timestamp = %d, confirms = %d", out->timestamp(), out->confirms()));
	//
	out->setDirection(Transaction::NetworkUnlinkedOut::Direction::IN);
	outs_.write(out->utxo().hash(), *out);
	inUpdated(out);
}

void LightWallet::selectLog(uint64_t from, std::vector<Transaction::NetworkUnlinkedOutPtr>& items) {
	//
	db::DbContainer<
		std::string /*timestamp*/,
		uint256 /*utxo*/>::Iterator lFrom;

	if (!from) lFrom = index_.last();
	else lFrom = index_.find(strprintf("%d", from));

	uint256 lAsset = TxAssetType::qbitAsset(); // default

	for (; items.size() < 10 && lFrom.valid(); --lFrom) {
		//
		uint256 lHash = *lFrom;
		std::string lTimestamp;
		lFrom.first(lTimestamp);

		//
		//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[selectLog]: timestamp = %s, from = %d", lTimestamp, from));
		//
		Transaction::NetworkUnlinkedOut lOut;
		if (outs_.read(lHash, lOut) && !lOut.utxo().change() /*exclude change*/ && lOut.utxo().out().asset() == lAsset) {
			//
			items.push_back(Transaction::NetworkUnlinkedOut::instance(lOut));
		}
	}

	//
	//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[selectLog]: items.size = %d", items.size()));	
}

void LightWallet::selectIns(uint64_t from, std::vector<Transaction::NetworkUnlinkedOutPtr>& items) {
	//
	db::DbTwoKeyContainer<
		unsigned short /*direction*/,
		std::string /*timestamp*/,
		uint256 /*utxo*/>::Iterator lFrom;

	if (!from) lFrom = value_.last();
	else lFrom = value_.find(Transaction::NetworkUnlinkedOut::Direction::IN, strprintf("%d", from));

	uint256 lAsset = TxAssetType::qbitAsset(); // default

	lFrom.setKey2Empty(); // off limits
	for (; items.size() < 10 && lFrom.valid(); --lFrom) {
		//
		unsigned short lDirection;
		if (lFrom.first(lDirection) && lDirection != Transaction::NetworkUnlinkedOut::Direction::IN) {
			continue;
		}

		//
		uint256 lHash = *lFrom;
		Transaction::NetworkUnlinkedOut lOut;
		if (outs_.read(lHash, lOut) && !lOut.utxo().change() /*exclude change*/ && lOut.utxo().out().asset() == lAsset) {
			//
			items.push_back(Transaction::NetworkUnlinkedOut::instance(lOut));
		}
	}

	//
	//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[selectIns]: items.size = %d", items.size()));	
}

void LightWallet::selectOuts(uint64_t from, std::vector<Transaction::NetworkUnlinkedOutPtr>& items) {
	//
	db::DbTwoKeyContainer<
		unsigned short /*direction*/,
		std::string /*timestamp*/,
		uint256 /*utxo*/>::Iterator lFrom;

	if (!from) lFrom = value_.last();
	else lFrom = value_.find(Transaction::NetworkUnlinkedOut::Direction::OUT, strprintf("%d", from));

	uint256 lAsset = TxAssetType::qbitAsset(); // default

	lFrom.setKey2Empty(); // off limits
	for (; items.size() < 10 && lFrom.valid(); --lFrom) {
		//
		unsigned short lDirection;
		if (lFrom.first(lDirection) && lDirection != Transaction::NetworkUnlinkedOut::Direction::OUT) {
			continue;
		}
		//
		uint256 lHash = *lFrom;
		Transaction::NetworkUnlinkedOut lOut;
		if (outs_.read(lHash, lOut) && (lOut.parentType() == Transaction::SPEND || lOut.parentType() == Transaction::SPEND_PRIVATE) &&
				lOut.utxo().out().asset() == lAsset) {
			//
			//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[selectOuts]: parentType = %d", lOut.parentType()));
			items.push_back(Transaction::NetworkUnlinkedOut::instance(lOut));
		}
	}

	//
	//if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("[selectOuts]: items.size = %d", items.size()));
}
