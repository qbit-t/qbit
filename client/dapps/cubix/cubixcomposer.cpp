#include "cubixcomposer.h"

#include "../../../txdapp.h"
#include "../../../txshard.h"
#include "../../../timestamp.h"

using namespace qbit;
using namespace qbit::cubix;

bool CubixLightComposer::open() {
	opened_ = true;
	return true;
}

bool CubixLightComposer::close() {
	opened_ = false;
	return true;
}

//
// CreateTxMediaSummary
//
void CubixLightComposer::CreateTxMediaSummary::process(errorFunction error) {
	//
	error_ = error;
	destinations_ = 0;
	collected_ = 0;

	//
	int lDestinations;
	if (!composer_->requestProcessor()->selectEntityCountByDApp(composer_->dAppName(), 
		SelectEntityCountByDApp::instance(
			boost::bind(&CubixLightComposer::CreateTxMediaSummary::entitiesCountByDAppLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
			boost::bind(&CubixLightComposer::CreateTxMediaSummary::timeout, shared_from_this())), destinations_)
	) { 
		error_("E_ENTITY_COUNT_REQUEST", "Entity count for cubix was not collected. Retry, please.");
	}
}

void CubixLightComposer::CreateTxMediaSummary::mergeInfo(const std::map<uint256, uint32_t>& info) {
	//
	for (std::map<uint256, uint32_t>::const_iterator lInfo = info.begin(); lInfo != info.end(); lInfo++) {
		//
		dAppInfo_[lInfo->second] = lInfo->first;	
	}
}

void CubixLightComposer::CreateTxMediaSummary::entitiesCountByDAppLoaded(const std::map<uint256, uint32_t>& info, const std::string&) {
	//
	if (++collected_ >= destinations_) {
		//
		mergeInfo(info);

		if (!dAppInfo_.size()) {
			error_("E_CUBIX_INFO_IS_ABSENT", "Cubix chain infos was not found."); return;
		}

		//
		SKeyPtr lSKey = composer_->wallet()->firstKey();
		PKey lPKey = lSKey->createPKey();	
		if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

		// create empty tx
		tx_ = TransactionHelper::to<TxMediaSummary>(TransactionFactory::create(TX_CUBIX_MEDIA_SUMMARY));
		// create context
		ctx_ = TransactionContext::instance(tx_);
		// set chain
		tx_->setChain(dAppInfo_.begin()->second); // sorted asc
		// set proposed size
		tx_->setSize(size_);
		// set timestamp
		tx_->setTimestamp(qbit::getMedianMicroseconds());
		// add out
		Transaction::UnlinkedOutPtr lDataOut = tx_->addDataOut(*lSKey, lPKey);

		// prepare fee tx
		amount_t lFeeAmount = size_ / CUBIX_FEE_RATE;
		if (lFeeAmount < CUBIX_MIN_FEE) lFeeAmount = CUBIX_MIN_FEE;

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
			tx_->addIn(*lSKey, *(lFeeUtxos.begin())); // qbit fee that was exact for fee - in
			tx_->addFeeOut(*lSKey, TxAssetType::qbitAsset(), lFeeAmount); // to the miner
		} else {
			error_("E_FEE_UTXO_ABSENT", "Fee utxo for buzz was not found."); return;
		}

		// push linked tx, which need to be pushed and broadcasted before this
		ctx_->addLinkedTx(lFee);

		if (!tx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

		created_(ctx_, lDataOut);

	} else {
		mergeInfo(info);
	}
}

//
// CreateTxMediaData
//
void CubixLightComposer::CreateTxMediaData::process(errorFunction error) {
	//
	error_ = error;

	//
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	
	if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	// create empty tx
	tx_ = TransactionHelper::to<TxMediaData>(TransactionFactory::create(TX_CUBIX_MEDIA_DATA));
	// create context
	ctx_ = TransactionContext::instance(tx_);
	// set chain
	tx_->setChain(chain_);
	// set timestamp
	tx_->setTimestamp(qbit::getMedianMicroseconds());
	// set data
	tx_->setData(data_, *lSKey);
	// add in
	tx_->addPrevDataIn(*lSKey, prev_); //
	// add out
	Transaction::UnlinkedOutPtr lDataOut = tx_->addDataOut(*lSKey, lPKey);

	if (!tx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }
	created_(ctx_, lDataOut);
}

//
// CreateTxMediaHeader
//
void CubixLightComposer::CreateTxMediaHeader::process(errorFunction error) {
	//
	error_ = error;

	//
	SKeyPtr lSKey = composer_->wallet()->firstKey();
	PKey lPKey = lSKey->createPKey();	
	if (!lSKey->valid()) { error_("E_KEY", "Secret key is invalid."); return; }

	// create empty tx
	tx_ = TransactionHelper::to<TxMediaHeader>(TransactionFactory::create(TX_CUBIX_MEDIA_HEADER));
	// create context
	ctx_ = TransactionContext::instance(tx_);
	// set chain
	tx_->setChain(chain_);
	// set timestamp
	tx_->setTimestamp(qbit::getMedianMicroseconds());
	// set size
	tx_->setSize(size_);
	// set name
	tx_->setMediaName(name_);
	// set name
	tx_->setDescription(description_);
	// type
	tx_->setMediaType(type_);
	// orientation
	tx_->setOrientation(orientation_);
	// duration
	tx_->setDuration(duration_);
	// set data
	tx_->setData(data_, *lSKey);
	// add in
	tx_->addPrevDataIn(*lSKey, prev_); //

	if (!tx_->finalize(*lSKey)) { error_("E_TX_FINALIZE", "Transaction finalization failed."); return; }

	created_(ctx_);
}
