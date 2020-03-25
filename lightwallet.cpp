#include "lightwallet.h"
#include "mkpath.h"
#include "vm/vm.h"

using namespace qbit;

SKey LightWallet::createKey(const std::list<std::string>& seed) {
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

SKey LightWallet::findKey(const PKey& pkey) {
	if (pkey.valid()) {
		SKey lSKey;
		if (keys_.read(pkey.id(), lSKey)) {
			return lSKey;
		}
	}

	return SKey();
}

SKey LightWallet::firstKey() {
	db::DbContainer<uint160 /*id*/, SKey>::Iterator lKey = keys_.begin();
	if (lKey.valid()) {
		return *lKey;
	}
	
	// try create from scratch
	return createKey(std::list<std::string>());
}

SKey LightWallet::changeKey() {
	return firstKey();
}

bool LightWallet::open() {
	if (!opened_) {
		try {
			if (mkpath(std::string(settings_->dataPath() + "/wallet").c_str(), 0777)) return false;

			keys_.open();
			utxo_.open();

			opened_ = true;
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

	keys_.close();
	utxo_.close();

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
	for (std::vector<Transaction::NetworkUnlinkedOut>::const_iterator lOut = outs.begin(); lOut != outs.end(); lOut++) {
		Transaction::NetworkUnlinkedOut& lOutRef = const_cast<Transaction::NetworkUnlinkedOut&>(*lOut);
		uint256 lUtxoId = lOutRef.utxo().hash();
		if (lOutRef.utxo().amount() != 0) {
			// make map
			assetsCache_[lOutRef.utxo().out().asset()].insert(std::multimap<amount_t, uint256>::value_type(lOutRef.utxo().amount(), lUtxoId));
			utxoCache_[lUtxoId] = Transaction::NetworkUnlinkedOut::instance(lOutRef);
		} else {
			// probably blinded out
			Transaction::NetworkUnlinkedOut lUtxoObj;
			if (!utxo_.read(lUtxoId, lUtxoObj)) {
				// pre-cache
				utxoCache_[lUtxoId] = Transaction::NetworkUnlinkedOut::instance(lOutRef);
				// request for tx
				if (status_ == IWallet::FETCHING_UTXO) status_ = IWallet::FETCHING_TXS;
				if (!requestProcessor_->loadTransaction(lOutRef.utxo().out().chain(), lOutRef.utxo().out().tx(), shared_from_this())) {
					status_ = IWallet::TX_INFO_NOT_FOUND;
				}
			} else {
				utxoCache_[lUtxoId] = Transaction::NetworkUnlinkedOut::instance(lOutRef); // re-create with new confirms
				utxoCache_[lUtxoId]->setUtxo(lUtxoObj.utxo()); // update utxo
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
	status_ = IWallet::FETCHING_UTXO;

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: selecting utxo..."));
	if (!requestProcessor_->selectUtxoByAddress(firstKey().createPKey(), MainChain::id(), shared_from_this())) {
		status_ = IWallet::CACHE_NOT_READY;
		//
		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: utxo cache is NOT ready, retry..."));
		return false;
	}

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[prepareCache]: fetching utxo..."));
	return true;
}

bool LightWallet::pushUnlinkedOut(Transaction::UnlinkedOutPtr utxo, TransactionContextPtr ctx) {
	//
	uint256 lUtxoId = utxo->hash();

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: ") + 
		strprintf("try to push utxo = %s, tx = %s, ctx = %s/%s#", 
			utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));

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
		utxo_.write(lUtxoId, *lItem->second);

		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: pushed ") + 
			strprintf("utxo = %s, tx = %s, ctx = %s/%s#", 
				utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));
	} else {
		{
			boost::unique_lock<boost::recursive_mutex> lLock(cacheMutex_);
			// add new one
			utxoCache_[lUtxoId] = Transaction::NetworkUnlinkedOut::instance(*utxo, settings_->mainChainMaturity(), false);
			assetsCache_[utxo->out().asset()].insert(std::multimap<amount_t, uint256>::value_type(utxo->amount(), lUtxoId));
		}

		if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[pushUnlinkedOut]: ") + 
			strprintf("PUSHED utxo = %s, tx = %s, ctx = %s/%s#", 
				utxo->hash().toHex(), utxo->out().tx().toHex(), ctx->tx()->id().toHex(), utxo->out().chain().toHex().substr(0, 10)));
	}

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

		utxo_.remove(hash);
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
		utxo_.remove(lId);

		removeFromAssetsCache(*lUtxo);
	}
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
				utxo_.remove(lAmount->second);
				utxoCache_.erase(lAmount->second);
				lAsset->second.erase(std::next(lAmount).base());
				continue;
			}

			// extra check
			{
				//
				if (!lUtxo->coinbase() && lUtxo->confirms() < settings_->mainChainMaturity()) {
					gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: transaction is NOT MATURE ") + 
						strprintf("%d/%d/%s/%s#", lUtxo->confirms(), settings_->mainChainMaturity(), 
							lUtxo->utxo().out().tx().toHex(), lUtxo->utxo().out().chain().toHex().substr(0, 10)));
					lAmount++;
					continue; // skip UTXO
				} else if (lUtxo->coinbase() && lUtxo->confirms() < settings_->mainChainCoinbaseMaturity()) {
					gLog().write(Log::WALLET, std::string("[collectUnlinkedOutsByAsset]: COINBASE transaction is NOT MATURE ") + 
						strprintf("%d/%d/%s/%s#", lUtxo->confirms(), settings_->mainChainCoinbaseMaturity(), 
							lUtxo->utxo().out().tx().toHex(), lUtxo->utxo().out().chain().toHex().substr(0, 10)));
					lAmount++;
					continue; // skip UTXO
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

// clean-up assets utxo
void LightWallet::cleanUp() {
}

void LightWallet::cleanUpData() {
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
	// stub
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
		SKey lSKey = findKey((*lUtxo)->address());
		if (lSKey.valid()) { 
			tx->addIn(lSKey, *lUtxo);
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
	SKey lSChangeKey = changeKey();
	SKey lSKey = firstKey();
	if (!lSKey.valid() || !lSChangeKey.valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");
	lTx->addOut(lSKey, dest, asset, amount);

	// make change
	if (asset != TxAssetType::qbitAsset() && lAmount > amount) {
		lTx->addOut(lSChangeKey, lSChangeKey.createPKey()/*change*/, asset, lAmount - amount);			
	}

	// change
	Transaction::UnlinkedOutPtr lChangeUtxo = nullptr;

	// TODO: try to estimate fee
	qunit_t lRate = feeRateLimit;
	amount_t lFee = lRate * lCtx->size(); 
	if (asset == TxAssetType::qbitAsset()) { // if qbit sending
		// try to check amount
		if (lAmount >= amount + lFee) {
			lTx->addFeeOut(lSKey, asset, lFee); // to miner

			if (lAmount > amount + lFee) { // make change
				lChangeUtxo = lTx->addOut(lSChangeKey, lSChangeKey.createPKey()/*change*/, asset, lAmount - (amount + lFee));
			}
		} else { // rare cases
			return makeTxSpend(type, asset, dest, amount + lFee, feeRateLimit);
		}
	} else {
		// add fee
		amount_t lFeeAmount = fillInputs(lTx, TxAssetType::qbitAsset(), lFee, lFeeUtxos);
		lTx->addFeeOut(lSKey, TxAssetType::qbitAsset(), lFee); // to miner
		if (lFeeAmount > lFee) { // make change
			lChangeUtxo = lTx->addOut(lSChangeKey, lSChangeKey.createPKey()/*change*/, TxAssetType::qbitAsset(), lFeeAmount - lFee);
		}
	}

	if (!lTx->finalize(lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	// we good
	removeUnlinkedOut(lUtxos);
	removeUnlinkedOut(lFeeUtxos);

	if (lChangeUtxo) { 
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
TransactionContextPtr LightWallet::createTxSpend(const uint256& asset, const PKey& dest, amount_t amount, qunit_t feeRateLimit) {
	return makeTxSpend(Transaction::SPEND, asset, dest, amount, feeRateLimit);
}

TransactionContextPtr LightWallet::createTxSpend(const uint256& asset, const PKey& dest, amount_t amount) {
	qunit_t lRate = settings_->maxFeeRate();
	return makeTxSpend(Transaction::SPEND, asset, dest, amount, lRate);
}

// create spend private tx
TransactionContextPtr LightWallet::createTxSpendPrivate(const uint256& asset, const PKey& dest, amount_t amount, qunit_t feeRateLimit) {
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
	SKey lSChangeKey = changeKey();
	SKey lSKey = firstKey();
	if (!lSKey.valid() || !lSChangeKey.valid()) throw qbit::exception("E_KEY", "Secret key is invalid.");
	lTx->addExternalOut(lSKey, dest, TxAssetType::qbitAsset(), amount); // out[0] - use to pay fee in shards

	amount_t lRate = settings_->maxFeeRate();
	amount_t lFee = (lRate * lCtx->size()) / 2; // half fee 

	// change
	Transaction::UnlinkedOutPtr lChangeUtxo = nullptr;

	// try to check amount
	if (lAmount >= amount + lFee) {
		lTx->addFeeOut(lSKey, TxAssetType::qbitAsset(), lFee); // to miner

		if (lAmount > amount + lFee) { // make change
			lChangeUtxo = lTx->addOut(lSChangeKey, lSChangeKey.createPKey()/*change*/, TxAssetType::qbitAsset(), lAmount - (amount + lFee));
		}
	} else { // rare cases
		return createTxFee(dest, amount + lFee);
	}

	if (!lTx->finalize(lSKey)) throw qbit::exception("E_TX_FINALIZE", "Transaction finalization failed.");

	// we good
	removeUnlinkedOut(lUtxos);

	if (lChangeUtxo) { 
		cacheUnlinkedOut(lChangeUtxo);
	}

	if (gLog().isEnabled(Log::WALLET)) gLog().write(Log::WALLET, std::string("[createTxFee]: fee tx created: ") + 
		strprintf("to = %s, amount = %d/%d", const_cast<PKey&>(dest).toString(), amount, lAmount));

	return lCtx;
}
