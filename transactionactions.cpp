#include "transactionactions.h"
#include "vm/vm.h"
#include "log/log.h"
#include "tinyformat.h"

#include <iostream>

using namespace qbit;

TransactionAction::Result TxCoinBaseVerify::execute(TransactionContextPtr wrapper, ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	if (wrapper->tx()->type() == Transaction::COINBASE || wrapper->tx()->type() == Transaction::BASE ||
		wrapper->tx()->type() == Transaction::BLOCKBASE) {

		if (wrapper->tx()->out().size() == 1 && wrapper->tx()->in().size() == 1) {

			// coinbase has special semantics
			// we just need to check amount and push[out], no balance checks
			// 
			qasm::ByteCode lVerify;
			lVerify << wrapper->tx()->in()[0].ownership() << wrapper->tx()->out()[0].destination();

			BlockHeader lCurrentBlock;
			uint64_t lCurrentHeight = store->currentHeight(lCurrentBlock);

			VirtualMachine lVM(lVerify);
			lVM.getR(qasm::QTH0).set(wrapper->tx()->id()); // tx hash
			lVM.getR(qasm::QTH1).set((unsigned short)wrapper->tx()->type()); // tx type - 2b
			lVM.getR(qasm::QTH2).set(0); // input number
			lVM.getR(qasm::QTH3).set(0); // output number
			lVM.getR(qasm::QTH4).set(lCurrentHeight); // current height
			lVM.setTransaction(wrapper);
			lVM.setWallet(wallet);
			lVM.setTransactionStore(store);
			lVM.setEntityStore(entityStore);

			lVM.execute();

			//std::stringstream lS;
			//lVM.dumpState(lS);
			//std::cout << std::endl << lS.str() << std::endl;

			if (lVM.state() != VirtualMachine::FINISHED) {
				std::string lError = _getVMStateText(lVM.state()) + " | " + 
					qasm::_getCommandText(lVM.lastCommand()) + ":" + qasm::_getAtomText(lVM.lastAtom()); 
				gLog().write(Log::GENERAL_ERROR, std::string("[TxCoinBaseVerify]: ") + lError);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				return TransactionAction::GENERAL_ERROR; 
			} else if (lVM.getR(qasm::QA7).to<unsigned char>() != 0x01) {
				std::string lError = "INVALID_COINBASE";
				gLog().write(Log::GENERAL_ERROR, std::string("[TxCoinBaseVerify]: ") + lError);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				return TransactionAction::GENERAL_ERROR;
			}

			//
			// extract amount
			wrapper->addAmount(lVM.getR(qasm::QA0).to<uint64_t>());

			return TransactionAction::SUCCESS;
		} else {
			std::string lError = "INCONSISTENT_INOUTS";
			gLog().write(Log::GENERAL_ERROR, std::string("[TxCoinBaseVerify]: ") + lError);
			wrapper->tx()->setStatus(Transaction::DECLINED);
			wrapper->addError(lError);
			return TransactionAction::GENERAL_ERROR;
		}
	}

	return TransactionAction::CONTINUE;
}

TransactionAction::Result TxSpendVerify::execute(TransactionContextPtr wrapper, ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	//
	// all transaction types _must_ pass through this checker
	{
		uint32_t lIdx = 0;
		std::vector<Transaction::In>& lIns = wrapper->tx()->in();
		if (!lIns.size()) {
			std::string lError = "tx should contain at least one input";
			gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendVerify]: ") + lError);
			wrapper->tx()->setStatus(Transaction::DECLINED);
			wrapper->addError(lError);
			return TransactionAction::GENERAL_ERROR;
		}

		BlockHeader lCurrentBlock;
		uint64_t lCurrentHeight = store->currentHeight(lCurrentBlock);
		if (store->synchronizing()) lCurrentHeight = ULONG_MAX; // special case from height

		for(std::vector<Transaction::In>::iterator lInPtr = lIns.begin(); lInPtr != lIns.end(); lInPtr++, lIdx++) {
			//
			TransactionPtr lInTx = nullptr;
			Transaction::In& lIn = (*lInPtr);
			uint64_t lHeight;
			uint64_t lConfirms;
			bool lCoinbase;

			// if 'in' in the storage already (commited to the block) - we check conditions
			// otherwise we just pass - origin tx is in the mempool and it is not coinbase
			if (!store->synchronizing() && wrapper->tx()->type() != Transaction::FEE && 
								store->transactionHeight(lIn.out().tx(), lHeight, lConfirms, lCoinbase) &&
								// in case of large cross-block data (like media files & etc)
								!wrapper->tx()->isContinuousData()) {
				// check maturity
				if (lCoinbase && lConfirms < wallet->mempoolManager()->locate(lIn.out().chain())->consensus()->coinbaseMaturity()) {
					std::string lError = strprintf("coinbase tx is not MATURE %d/%s", lConfirms, lIn.out().tx().toHex());
					gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendVerify]: ") + lError);
					wrapper->tx()->setStatus(Transaction::DECLINED);
					wrapper->addError(lError);
					return TransactionAction::GENERAL_ERROR;
				} else if (!lCoinbase && lConfirms < wallet->mempoolManager()->locate(lIn.out().chain())->consensus()->maturity()) {
					std::string lError = strprintf("tx is not MATURE %d/%s", lConfirms, lIn.out().tx().toHex());
					gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendVerify]: ") + lError);
					wrapper->tx()->setStatus(Transaction::DECLINED);
					wrapper->addError(lError);
					return TransactionAction::GENERAL_ERROR;
				}
			}

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
			
			if (lInTx != nullptr) {

				// sanity
				if (lIn.out().index() < lInTx->out().size()) {
					qasm::ByteCode lVerifyUsage;
					lVerifyUsage << lIn.ownership() << lInTx->out()[lIn.out().index()].destination();

					VirtualMachine lVM(lVerifyUsage);
					lVM.getR(qasm::QTH0).set(wrapper->tx()->id());
					lVM.getR(qasm::QTH1).set((unsigned short)wrapper->tx()->type()); // tx type - 2b
					lVM.getR(qasm::QTH2).set(lIdx); // input number
					lVM.getR(qasm::QTH4).set(lCurrentHeight); // current height
					lVM.setTransaction(wrapper);
					lVM.setWallet(wallet);
					lVM.setTransactionStore(store);
					lVM.setEntityStore(entityStore);

					lVM.execute();

					//std::stringstream lS;
					//lVM.dumpState(lS);
					//std::cout << std::endl << lS.str() << std::endl;

					if (lVM.state() != VirtualMachine::FINISHED) {
						std::string lError = _getVMStateText(lVM.state()) + " | " + 
							qasm::_getCommandText(lVM.lastCommand()) + ":" + qasm::_getAtomText(lVM.lastAtom()); 
						gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendVerify]: ") + lError);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						return TransactionAction::GENERAL_ERROR; 
					} 
					else if (
								lVM.getR(qasm::QE0).getType() != qasm::QNONE && 
								lVM.getR(qasm::QS7).to<unsigned char>() != 0x01
							) {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_SIGNATURE);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendVerify]: ") + lError);
					} 
					else if (lVM.getR(qasm::QA7).to<unsigned char>() != 0x01 && 
						(
							wrapper->tx()->type() == Transaction::SPEND || 
							wrapper->tx()->type() == Transaction::SPEND_PRIVATE ||
							wrapper->tx()->type() == Transaction::ASSET_TYPE || 
							wrapper->tx()->type() == Transaction::ASSET_EMISSION ||
							wrapper->tx()->type() == Transaction::FEE ||
							lIn.out().asset() != TxAssetType::nullAsset()
						)) {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_AMOUNT);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendVerify]: ") + lError);
					} 
					else if (lVM.getR(qasm::QP0).to<unsigned char>() != 0x01) {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_UTXO);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendVerify]: ") + lError);
					}
					else if (lVM.getR(qasm::QE0).to<unsigned char>() != 0x01 && 
						(
							wrapper->tx()->type() == Transaction::SPEND || 
							wrapper->tx()->type() == Transaction::SPEND_PRIVATE ||
							wrapper->tx()->type() == Transaction::ASSET_TYPE || 
							wrapper->tx()->type() == Transaction::ASSET_EMISSION ||
							wrapper->tx()->type() == Transaction::FEE ||
							lIn.out().asset() != TxAssetType::nullAsset()
						)) {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_ADDRESS);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendVerify]: ") + lError);
					}
					else if (lVM.getR(qasm::QR0).to<unsigned char>() != 0x01) {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_RESULT);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendVerify]: ") + lError);

						std::stringstream lS;
						lVM.dumpState(lS);
						gLog().write(Log::GENERAL_ERROR, "[State]: " + lS.str());
					}

					if (wrapper->errors().size())
						return TransactionAction::GENERAL_ERROR;
					
					// extract commit
					if (lVM.getR(qasm::QA1).getType() != qasm::QNONE)
						wrapper->commitIn()[lIn.out().asset()].push_back(lVM.getR(qasm::QA1).to<std::vector<unsigned char>>());

					// push link
					store->addLink(lInTx->id(), wrapper->tx()->id());
				} else {
					std::string lError = "INCONSISTENT_INOUTS";
					gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendVerify]: ") + lError);
					wrapper->tx()->setStatus(Transaction::DECLINED);
					wrapper->addError(lError);
					return TransactionAction::GENERAL_ERROR;					
				}
			} else {
				std::string lError = _getVMStateText(VirtualMachine::UNKNOWN_REFTX);
				std::string lErrorText = strprintf("[TxSpendVerify]: %s - %s/%s#", lError, lIn.out().tx().toHex(), lIn.out().chain().toHex().substr(0, 10));
				gLog().write(Log::GENERAL_ERROR, lErrorText);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lErrorText);
				return TransactionAction::GENERAL_ERROR;
			}
		}

		return TransactionAction::CONTINUE; // continue any way
	}

	return TransactionAction::CONTINUE;
}

TransactionAction::Result TxSpendOutVerify::execute(TransactionContextPtr wrapper, ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	//
	// all transaction types _must_ pass through this checker
	if (wrapper->tx()->type() != Transaction::ASSET_TYPE) {

		BlockHeader lCurrentBlock;
		uint64_t lCurrentHeight = store->currentHeight(lCurrentBlock);
		if (store->synchronizing()) lCurrentHeight = ULONG_MAX; // special case from height		

		uint32_t lIdx = 0;
		std::vector<Transaction::Out>& lOuts = wrapper->tx()->out();
		for(std::vector<Transaction::Out>::iterator lOutPtr = lOuts.begin(); lOutPtr != lOuts.end(); lOutPtr++, lIdx++) {

			Transaction::Out& lOut = (*lOutPtr);
			VirtualMachine lVM(lOut.destination());
			lVM.getR(qasm::QTH0).set(wrapper->tx()->id());
			lVM.getR(qasm::QTH1).set((unsigned short)wrapper->tx()->type()); // tx type - 2b
			lVM.getR(qasm::QTH3).set(lIdx); // out number
			lVM.getR(qasm::QTH4).set(lCurrentHeight); // current height	
			lVM.setTransaction(wrapper);
			lVM.setWallet(wallet);
			lVM.setTransactionStore(store);
			lVM.setEntityStore(entityStore);

			lVM.execute();

			//std::stringstream lS;
			//lVM.dumpState(lS);
			//std::cout << std::endl << lS.str() << std::endl;			

			if (lVM.state() != VirtualMachine::FINISHED && 
					!(wrapper->tx()->type() == Transaction::FEE && lVM.state() == VirtualMachine::INVALID_CHAIN)) {
				std::string lError = _getVMStateText(lVM.state()) + " | " + 
					qasm::_getCommandText(lVM.lastCommand()) + ":" + qasm::_getAtomText(lVM.lastAtom()); 
				gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendOutVerify]: ") + lError);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				return TransactionAction::GENERAL_ERROR; 
			} else if (lVM.getR(qasm::QA7).to<unsigned char>() != 0x01 &&
				(
					wrapper->tx()->type() == Transaction::SPEND || 
					wrapper->tx()->type() == Transaction::SPEND_PRIVATE ||
					wrapper->tx()->type() == Transaction::ASSET_EMISSION ||
					wrapper->tx()->type() == Transaction::FEE ||
					lOut.asset() != TxAssetType::nullAsset()
				)) {
				std::string lError = _getVMStateText(VirtualMachine::INVALID_AMOUNT);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendOutVerify]: ") + lError);
			} 

			if (wrapper->errors().size())
				return TransactionAction::GENERAL_ERROR;
			
			// extract commit
			if (lVM.getR(qasm::QA1).getType() != qasm::QNONE)
				wrapper->commitOut()[lOut.asset()].push_back(lVM.getR(qasm::QA1).to<std::vector<unsigned char>>());

			// extract fee
			if (lVM.getR(qasm::QD0).to<unsigned short>() == PKey::miner()) {
				wrapper->addFee(lVM.getR(qasm::QA0).to<uint64_t>());
			}

			// strict time-locked txs
			if (lOut.asset() == store->settings()->proofAsset() && store->settings()->proofAssetLockTime() > 0) {
				//
				bool lProcess = true;

				// check if change
				std::vector<Transaction::In>& lIns = wrapper->tx()->in();
				if (lVM.getR(qasm::QA6).getType() != qasm::QNONE && lVM.getR(qasm::QA6).to<unsigned char>() == 0x01 /*change*/) {
					// get address
					VirtualMachine lVMIn(lIns.begin()->ownership());
					lVMIn.execute();
					if (lVMIn.getR(qasm::QS0) != lVM.getR(qasm::QD0)) {
						//
						std::string lError = "Proof asset change address is not equal to the spend address.";
						gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendOutVerify]: ") + lError);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						return TransactionAction::GENERAL_ERROR;
					} else lProcess = false;
				}

				// check if balance out
				if (lVM.getR(qasm::QD0).to<unsigned short>() == 0xffff) lProcess = false;

				//
				if (lProcess /*not change*/ && wrapper->tx()->type() != Transaction::ASSET_EMISSION /*emission is lock-free*/ &&
					(!wrapper->blockTimestamp() ||
						wrapper->blockTimestamp() >= store->settings()->proofFrom()) /*check height only AFTER initial timestamp*/) {
					//
					if (lVM.getR(qasm::QR1).getType() != qasm::QNONE) {
						//
						uint64_t lHeight = lVM.getR(qasm::QR1).to<uint64_t>();
						//
						if (wrapper->context() != TransactionContext::STORE_REINDEX && !(lHeight > lCurrentHeight && 
								lHeight - lCurrentHeight >= store->settings()->proofAssetLockTime())) {
							//
							// get address and check if dest == source and skip if any
							// because mistrust \ endorse tx conditions was already checked and processed earliar
							//
							VirtualMachine lVMIn(lIns.begin()->ownership());
							lVMIn.execute();
							if (lVMIn.getR(qasm::QS0) != lVM.getR(qasm::QD0)) {
								//
								std::string lError = _getVMStateText(VirtualMachine::INVALID_RESULT) + strprintf(" | locked height must be at least H+%d", store->settings()->proofAssetLockTime());
								wrapper->tx()->setStatus(Transaction::DECLINED);
								wrapper->addError(lError);
								gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendOutVerify]: ") + 
									strprintf("current = %d, embedded = %d", lCurrentHeight, lHeight));
								gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendOutVerify]: ") + lError);
								return TransactionAction::GENERAL_ERROR;
							}
						}

					} else {
						std::string lError = "Time-lock height is absent";
						gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendOutVerify]: ") + lError);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						return TransactionAction::GENERAL_ERROR;
					}
				}
			}			
		}

		if (!wrapper->tx()->isFeeFree() && wrapper->fee() == 0) {
			std::string lError = "Fee is absent";
			gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendOutVerify]: ") + lError);
			wrapper->tx()->setStatus(Transaction::DECLINED);
			wrapper->addError(lError);
			return TransactionAction::GENERAL_ERROR;
		}

		return TransactionAction::CONTINUE; // continue any way
	}

	return TransactionAction::CONTINUE;
}

TransactionAction::Result TxBalanceVerify::execute(TransactionContextPtr wrapper, ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	//
	// all transaction types _must_ pass through this checker
	{

		ContextPtr lContext = Context::instance();
		for(TransactionContext::_commitMap::iterator lInPtr = wrapper->commitIn().begin(); lInPtr != wrapper->commitIn().end(); lInPtr++) {

			TransactionContext::_commitMap::iterator lOutPtr = wrapper->commitOut().find(lInPtr->first);
			if (lOutPtr != wrapper->commitOut().end()) {

				if(!lContext->verifyTally(lInPtr->second, lOutPtr->second)) {
					std::string lError = _getVMStateText(VirtualMachine::INVALID_BALANCE);
					wrapper->tx()->setStatus(Transaction::DECLINED);
					wrapper->addError(lError);
					gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendBalanceVerify]: ") + lError);
					return TransactionAction::GENERAL_ERROR;	
				}
			} else {
				std::string lError = strprintf("%s: for %s", _getVMStateText(VirtualMachine::INVALID_AMOUNT), lInPtr->first.toHex());
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendBalanceVerify]: ") + lError);
				return TransactionAction::GENERAL_ERROR;				
			}
		}

		if (!wrapper->tx()->isFeeFree() && wrapper->fee() == 0) {
			std::string lError = _getVMStateText(VirtualMachine::INVALID_FEE);
			wrapper->tx()->setStatus(Transaction::DECLINED);
			wrapper->addError(lError);
			gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendBalanceVerify]: ") + lError);
			return TransactionAction::GENERAL_ERROR;			
		}

		return TransactionAction::SUCCESS;
	}

	return TransactionAction::CONTINUE;
}

TransactionAction::Result TxAssetTypeVerify::execute(TransactionContextPtr wrapper, ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	//
	if (wrapper->tx()->type() == Transaction::ASSET_TYPE) {

		TxAssetTypePtr lTx = TransactionHelper::to<TxAssetType>(wrapper->tx());
		uint32_t lIdx = 0;

		std::vector<Transaction::Out>& lOuts = wrapper->tx()->out();
		for(std::vector<Transaction::Out>::iterator lOutPtr = lOuts.begin(); lOutPtr != lOuts.end(); lOutPtr++, lIdx++) {

			Transaction::Out& lOut = (*lOutPtr);
			VirtualMachine lVM(lOut.destination());
			lVM.getR(qasm::QTH0).set(wrapper->tx()->id());
			lVM.getR(qasm::QTH1).set((unsigned short)wrapper->tx()->type()); // tx type - 2b
			lVM.getR(qasm::QTH3).set(lIdx); // out number
			lVM.setTransaction(wrapper);
			lVM.setWallet(wallet);
			lVM.setTransactionStore(store);
			lVM.setEntityStore(entityStore);

			lVM.execute();

			//std::stringstream lS;
			//lVM.dumpState(lS);
			//std::cout << std::endl << lS.str() << std::endl;			

			if (lVM.state() != VirtualMachine::FINISHED) {
				std::string lError = _getVMStateText(lVM.state()) + " | " + 
					qasm::_getCommandText(lVM.lastCommand()) + ":" + qasm::_getAtomText(lVM.lastAtom()); 
				gLog().write(Log::GENERAL_ERROR, std::string("[TxAssetTypeVerify]: ") + lError);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				return TransactionAction::GENERAL_ERROR; 
			} 
			else if (lVM.getR(qasm::QA7).to<unsigned char>() != 0x01) {
				if (lTx->emission() == TxAssetType::LIMITED) {
					std::string lError = _getVMStateText(VirtualMachine::INVALID_AMOUNT);
					wrapper->tx()->setStatus(Transaction::DECLINED);
					wrapper->addError(lError);
					gLog().write(Log::GENERAL_ERROR, std::string("[TxAssetTypeVerify]: ") + lError);
				}
			} 

			if (wrapper->errors().size())
				return TransactionAction::GENERAL_ERROR;
			
			// extract commit
			if (lVM.getR(qasm::QA7).to<unsigned char>() == 0x01)
				wrapper->commitOut()[lOut.asset()].push_back(lVM.getR(qasm::QA1).to<std::vector<unsigned char>>());

			// extract fee
			if (lVM.getR(qasm::QD0).to<unsigned short>() == PKey::miner()) {
				wrapper->addFee(lVM.getR(qasm::QA0).to<uint64_t>());
			}
		}

		if (!wrapper->tx()->isFeeFree() && wrapper->fee() == 0) {
			std::string lError = "Fee is absent";
			gLog().write(Log::GENERAL_ERROR, std::string("[TxSpendOutVerify]: ") + lError);
			wrapper->tx()->setStatus(Transaction::DECLINED);
			wrapper->addError(lError);
			return TransactionAction::GENERAL_ERROR;
		}

		return TransactionAction::CONTINUE; // continue any way
	}

	return TransactionAction::CONTINUE;
}
