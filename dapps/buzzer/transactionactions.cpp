#include "transactionactions.h"
#include "../../vm/vm.h"
#include "../../log/log.h"
#include "../../tinyformat.h"
#include "txbuzzer.h"
#include "txbuzzerendorse.h"
#include "txbuzzermistrust.h"

#include <iostream>

using namespace qbit;

TransactionAction::Result TxBuzzerTimelockOutsVerify::execute(TransactionContextPtr wrapper, ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	//
	if (wrapper->tx()->type() == TX_BUZZER_ENDORSE || wrapper->tx()->type() == TX_BUZZER_MISTRUST) {	
		//
		uint32_t lIdx = 0;
		std::vector<Transaction::In>& lIns = wrapper->tx()->in();
		for(std::vector<Transaction::In>::iterator lInPtr = lIns.begin(); lInPtr != lIns.end(); lInPtr++, lIdx++) {
			//
			TransactionPtr lInTx = nullptr;
			Transaction::In& lIn = (*lInPtr);
			uint64_t lHeight;
			uint64_t lConfirms;
			bool lCoinbase;

			lInTx = store->locateTransaction(lIn.out().tx());
			if (!lInTx) { 
				// get linked store
				ITransactionStorePtr lStore = store->storeManager()->locate(lIn.out().chain());
				lInTx = lStore->locateTransaction(lIn.out().tx());

				// try to look on mempools
				if (!lInTx) {
					IMemoryPoolPtr lPool = wallet->mempoolManager()->locate(lIn.out().chain());
					lInTx = lPool->locateTransaction(lIn.out().tx());
				}
			}
			
			if (lInTx != nullptr && lInTx->type() == Transaction::FEE) {
				// 
				BlockHeader lCurrentBlock;
				//
				uint64_t lCurrentHeight;
				uint64_t lConfirms;
				bool lCoinbase = false;
				ITransactionStorePtr lStore = store->storeManager()->locate(MainChain::id());

				if (!lStore->transactionHeight(lInTx->id(), lCurrentHeight, lConfirms, lCoinbase)) {
					// not in chain - get current height
					lCurrentHeight = store->storeManager()->locate(MainChain::id())->currentHeight(lCurrentBlock);
				}

				if (lInTx->out().size() > TX_BUZZER_ENDORSE_FEE_LOCKED_OUT /**/) {
					qasm::ByteCode lVerify;
					lVerify << lInTx->out()[TX_BUZZER_ENDORSE_FEE_LOCKED_OUT /*second out*/].destination();

					VirtualMachine lVM(lVerify);
					lVM.getR(qasm::QTH0).set(wrapper->tx()->id());
					lVM.getR(qasm::QTH1).set((unsigned short)wrapper->tx()->type()); // tx type - 2b
					lVM.getR(qasm::QTH3).set(TX_BUZZER_ENDORSE_FEE_LOCKED_OUT); // input number
					lVM.getR(qasm::QTH4).set(lCurrentHeight); // height
					lVM.setTransaction(wrapper); 
					lVM.setWallet(wallet); // is is normai if 
					lVM.setTransactionStore(TxBlockStore::instance()); // stub
					lVM.setEntityStore(TxEntityStore::instance()); // stub

					lVM.execute();

					/*
					std::stringstream lS;
					lVM.dumpState(lS);
					gLog().write(Log::INFO, lS.str());
					*/

					if (lVM.state() != VirtualMachine::FINISHED) {
						std::string lError = _getVMStateText(lVM.state()) + " | " + 
							qasm::_getCommandText(lVM.lastCommand()) + ":" + qasm::_getAtomText(lVM.lastAtom()); 
						gLog().write(Log::GENERAL_ERROR, std::string("[TxBuzzerTimelockOutsVerify]: ") + lError);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						return TransactionAction::GENERAL_ERROR;
					} else if (lVM.getR(qasm::QC1).getType() == qasm::QNONE) {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_RESULT) + " | check height was not reached";
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::GENERAL_ERROR, std::string("[TxBuzzerTimelockOutsVerify]: ") + lError);
						return TransactionAction::GENERAL_ERROR; 
					} else if (lVM.getR(qasm::QR1).getType() != qasm::QNONE) {
						//
						uint64_t lTargetHeight;
						if (wrapper->tx()->type() == TX_BUZZER_ENDORSE)
							lTargetHeight = TxBuzzerEndorse::calcLockHeight(wallet->mempoolManager()->locate(MainChain::id())->consensus()->blockTime());
						else 
							lTargetHeight = TxBuzzerMistrust::calcLockHeight(wallet->mempoolManager()->locate(MainChain::id())->consensus()->blockTime());

						if (!(lVM.getR(qasm::QR1).to<uint64_t>() > lCurrentHeight && 
							lVM.getR(qasm::QR1).to<uint64_t>() - lCurrentHeight >= lTargetHeight)) {
							//
							std::string lError = _getVMStateText(VirtualMachine::INVALID_RESULT) + " | locked height must be at least H+360";
							wrapper->tx()->setStatus(Transaction::DECLINED);
							wrapper->addError(lError);
							gLog().write(Log::GENERAL_ERROR, std::string("[TxBuzzerTimelockOutsVerify]: ") + 
								strprintf("current = %d, target = %d, embedded = %d", lCurrentHeight, lTargetHeight, lVM.getR(qasm::QR1).to<uint64_t>()));
							gLog().write(Log::GENERAL_ERROR, std::string("[TxBuzzerTimelockOutsVerify]: ") + lError);
							return TransactionAction::GENERAL_ERROR;
						}

						if (lVM.getR(qasm::QA0).getType() != qasm::QNONE) {
							amount_t lAmount = lVM.getR(qasm::QA0).to<amount_t>();
							uint256 lAsset = lInTx->out()[TX_BUZZER_ENDORSE_FEE_LOCKED_OUT /*second out*/].asset();
							if (!(lAsset == store->settings()->proofAsset() && lAmount == store->settings()->oneVoteProofAmount())) {
								//
								std::string lError = _getVMStateText(VirtualMachine::INVALID_RESULT) + " | locked amount and/or asset is invalid";
								wrapper->tx()->setStatus(Transaction::DECLINED);
								wrapper->addError(lError);
								gLog().write(Log::GENERAL_ERROR, std::string("[TxBuzzerTimelockOutsVerify]: ") + 
									strprintf("amount = %d, asset = %d", lAmount, lAsset.toHex()));
								gLog().write(Log::GENERAL_ERROR, std::string("[TxBuzzerTimelockOutsVerify]: ") + lError);
								return TransactionAction::GENERAL_ERROR;
							}
						} else {
							//
							std::string lError = _getVMStateText(VirtualMachine::INVALID_RESULT) + " | locked amount is absent";
							wrapper->tx()->setStatus(Transaction::DECLINED);
							wrapper->addError(lError);
							gLog().write(Log::GENERAL_ERROR, std::string("[TxBuzzerTimelockOutsVerify]: ") + lError);
							return TransactionAction::GENERAL_ERROR;
						}
					} else {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_RESULT) + " | height was not set";
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::GENERAL_ERROR, std::string("[TxBuzzerTimelockOutsVerify]: ") + lError);
						return TransactionAction::GENERAL_ERROR;
					}

					return TransactionAction::CONTINUE; // continue any way
				}
			}
		}

		std::string lError = _getVMStateText(VirtualMachine::INVALID_RESULT) + " | fee with locked amount was not found";
		wrapper->tx()->setStatus(Transaction::DECLINED);
		wrapper->addError(lError);
		gLog().write(Log::GENERAL_ERROR, std::string("[TxBuzzerTimelockOutsVerify]: ") + lError);
		return TransactionAction::GENERAL_ERROR; 
	}

	return TransactionAction::CONTINUE;
}
