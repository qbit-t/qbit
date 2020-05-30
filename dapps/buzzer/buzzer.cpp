#include "buzzer.h"

using namespace qbit;

void Buzzer::buzzerInfoLoaded(TransactionPtr tx) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	if (tx) {
		TxBuzzerInfoPtr lInfo = TransactionHelper::to<TxBuzzerInfo>(tx);
		//
		std::map<uint256 /*info tx*/, Buzzer::Info>::iterator lItem = pendingInfos_.find(tx->id());
		if (lItem != pendingInfos_.end() && lInfo->verifySignature() && // signature and ...
				lInfo->in()[TX_BUZZER_INFO_MY_IN].out().tx() == lItem->second.id()) // ... expected buzzer is match
			pushBuzzerInfo(lInfo);
	}
}

void Buzzer::resolveBuzzerInfos() {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	for (std::map<uint256 /*info tx*/, Buzzer::Info>::iterator lItem = pendingInfos_.begin(); 
																			lItem != pendingInfos_.end(); lItem++) {
		//
		if (loadingPendingInfos_.find(lItem->first) == loadingPendingInfos_.end()) {
			requestProcessor_->loadTransaction(lItem->second.chain(), lItem->first, 
				LoadTransaction::instance(
					boost::bind(&Buzzer::buzzerInfoLoaded, shared_from_this(), _1),
					boost::bind(&Buzzer::timeout, shared_from_this()))
			);

			//
			loadingPendingInfos_.insert(lItem->first);
		}
	}
}

void Buzzer::collectPengingInfos(std::map<uint256 /*chain*/, std::set<uint256>/*items*/>& items) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	for (std::map<uint256 /*info tx*/, Buzzer::Info>::iterator lItem = pendingInfos_.begin(); 
																			lItem != pendingInfos_.end(); lItem++) {
		//
		items[lItem->second.chain()].insert(lItem->first);
	}
}
