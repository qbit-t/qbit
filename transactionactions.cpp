#include "transactionactions.h"
#include "vm/vm.h"
#include "log/log.h"

#include <iostream>

using namespace qbit;

TransactionAction::Result TxCoinBaseVerify::execute(TransactionContextPtr wrapper, ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	if (wrapper->tx()->type() == Transaction::COINBASE) {

		if (wrapper->tx()->out().size() == 1 && wrapper->tx()->in().size() == 1) {

			// coinbase has special semantics
			// we just need to check amount and push[out], no balance checks
			// 
			qasm::ByteCode lVerify;
			lVerify << wrapper->tx()->in()[0].ownership() << wrapper->tx()->out()[0].destination();

			VirtualMachine lVM(lVerify);
			lVM.getR(qasm::QTH0).set(wrapper->tx()->id()); // tx hash
			lVM.getR(qasm::QTH1).set((unsigned short)wrapper->tx()->type()); // tx type - 2b
			lVM.getR(qasm::QTH2).set(0); // input number
			lVM.getR(qasm::QTH3).set(0); // output number
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
				gLog().write(Log::ERROR, std::string("[TxCoinBaseVerify]: ") + lError);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				return TransactionAction::ERROR; 
			} else if (lVM.getR(qasm::QA7).to<unsigned char>() != 0x01) {
				std::string lError = "INVALID_COINBASE";
				gLog().write(Log::ERROR, std::string("[TxCoinBaseVerify]: ") + lError);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				return TransactionAction::ERROR; 
			}

			//
			// maybe amount checks?

			return TransactionAction::SUCCESS;
		} else {
			std::string lError = "INCONSISTENT_INOUTS";
			gLog().write(Log::ERROR, std::string("[TxCoinBaseVerify]: ") + lError);
			wrapper->tx()->setStatus(Transaction::DECLINED);
			wrapper->addError(lError);
			return TransactionAction::ERROR;
		}
	}

	return TransactionAction::CONTINUE;
}

TransactionAction::Result TxSpendVerify::execute(TransactionContextPtr wrapper, ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	//
	// all transaction types _must_ pass through this checker
	if (wrapper->tx()->type() == Transaction::SPEND || wrapper->tx()->type() == Transaction::SPEND_PRIVATE || 
		wrapper->tx()->type() == Transaction::ASSET_TYPE || wrapper->tx()->type() == Transaction::ASSET_EMISSION) {

		uint32_t lIdx = 0;
		std::vector<Transaction::In>& lIns = wrapper->tx()->in();
		for(std::vector<Transaction::In>::iterator lInPtr = lIns.begin(); lInPtr != lIns.end(); lInPtr++, lIdx++) {

			Transaction::In& lIn = (*lInPtr);
			TransactionPtr lInTx = store->locateTransaction(lIn.out().tx());
			if (lInTx != nullptr) {

				// sanity
				if (lIn.out().index() < lInTx->out().size()) {
					qasm::ByteCode lVerifyUsage;
					lVerifyUsage << lIn.ownership() << lInTx->out()[lIn.out().index()].destination();

					VirtualMachine lVM(lVerifyUsage);
					lVM.getR(qasm::QTH0).set(wrapper->tx()->id());
					lVM.getR(qasm::QTH1).set((unsigned short)wrapper->tx()->type()); // tx type - 2b
					lVM.getR(qasm::QTH2).set(lIdx); // input number
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
						gLog().write(Log::ERROR, std::string("[TxVerify]: ") + lError);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						return TransactionAction::ERROR; 
					} 
					else if (lVM.getR(qasm::QS7).to<unsigned char>() != 0x01) {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_SIGNATURE);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::ERROR, std::string("[TxVerify]: ") + lError);
					} 
					else if (lVM.getR(qasm::QA7).to<unsigned char>() != 0x01) {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_AMOUNT);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::ERROR, std::string("[TxVerify]: ") + lError);
					} 
					else if (lVM.getR(qasm::QP0).to<unsigned char>() != 0x01) {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_UTXO);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::ERROR, std::string("[TxVerify]: ") + lError);
					}
					else if (lVM.getR(qasm::QE0).to<unsigned char>() != 0x01) {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_ADDRESS);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::ERROR, std::string("[TxVerify]: ") + lError);
					}
					else if (lVM.getR(qasm::QR0).to<unsigned char>() != 0x01) {
						std::string lError = _getVMStateText(VirtualMachine::INVALID_RESULT);
						wrapper->tx()->setStatus(Transaction::DECLINED);
						wrapper->addError(lError);
						gLog().write(Log::ERROR, std::string("[TxVerify]: ") + lError);
					}

					if (wrapper->errors().size())
						return TransactionAction::ERROR;
					
					// extract commit
					wrapper->commitIn()[lIn.out().asset()].push_back(lVM.getR(qasm::QA1).to<std::vector<unsigned char>>());

					// push link
					store->addLink(lInTx->id(), wrapper->tx()->id());
				} else {
					std::string lError = "INCONSISTENT_INOUTS";
					gLog().write(Log::ERROR, std::string("[TxSpendVerify]: ") + lError);
					wrapper->tx()->setStatus(Transaction::DECLINED);
					wrapper->addError(lError);
					return TransactionAction::ERROR;					
				}
			} else {
				std::string lError = _getVMStateText(VirtualMachine::UNKNOWN_REFTX);
				gLog().write(Log::ERROR, std::string("[TxSpendVerify]: ") + lError);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				return TransactionAction::ERROR;
			}
		}

		return TransactionAction::CONTINUE; // continue any way
	}

	return TransactionAction::CONTINUE;
}

TransactionAction::Result TxSpendOutVerify::execute(TransactionContextPtr wrapper, ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	//
	// all transaction types _must_ pass through this checker
	if (wrapper->tx()->type() == Transaction::SPEND || wrapper->tx()->type() == Transaction::SPEND_PRIVATE || 
		wrapper->tx()->type() == Transaction::ASSET_EMISSION) {

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
				gLog().write(Log::ERROR, std::string("[TxSpendOutVerify]: ") + lError);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				return TransactionAction::ERROR; 
			} 
			else if (lVM.getR(qasm::QA7).to<unsigned char>() != 0x01) {
				std::string lError = _getVMStateText(VirtualMachine::INVALID_AMOUNT);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				gLog().write(Log::ERROR, std::string("[TxSpendOutVerify]: ") + lError);
			} 

			if (wrapper->errors().size())
				return TransactionAction::ERROR;
			
			// extract commit
			wrapper->commitOut()[lOut.asset()].push_back(lVM.getR(qasm::QA1).to<std::vector<unsigned char>>());

			// extract fee
			if (lVM.getR(qasm::QD0).to<unsigned short>() == PKey::miner()) {
				wrapper->addFee(lVM.getR(qasm::QA0).to<uint64_t>());
			}
		}

		return TransactionAction::CONTINUE; // continue any way
	}

	return TransactionAction::CONTINUE;
}

TransactionAction::Result TxBalanceVerify::execute(TransactionContextPtr wrapper, ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	//
	// all transaction types _must_ pass through this checker
	if (wrapper->tx()->type() == Transaction::SPEND || wrapper->tx()->type() == Transaction::SPEND_PRIVATE ||
		wrapper->tx()->type() == Transaction::ASSET_TYPE || wrapper->tx()->type() == Transaction::ASSET_EMISSION) {

		ContextPtr lContext = Context::instance();
		for(TransactionContext::_commitMap::iterator lInPtr = wrapper->commitIn().begin(); lInPtr != wrapper->commitIn().end(); lInPtr++) {

			TransactionContext::_commitMap::iterator lOutPtr = wrapper->commitOut().find(lInPtr->first);
			if (lOutPtr != wrapper->commitOut().end()) {

				if(!lContext->verifyTally(lInPtr->second, lOutPtr->second)) {
					std::string lError = _getVMStateText(VirtualMachine::INVALID_BALANCE);
					wrapper->tx()->setStatus(Transaction::DECLINED);
					wrapper->addError(lError);
					gLog().write(Log::ERROR, std::string("[TxSpendBalanceVerify]: ") + lError);
					return TransactionAction::ERROR;	
				}
			} else {
				std::string lError = _getVMStateText(VirtualMachine::INVALID_AMOUNT);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				gLog().write(Log::ERROR, std::string("[TxSpendBalanceVerify]: ") + lError);
				return TransactionAction::ERROR;				
			}
		}

		if (wrapper->fee() == 0) {
			std::string lError = _getVMStateText(VirtualMachine::INVALID_FEE);
			wrapper->tx()->setStatus(Transaction::DECLINED);
			wrapper->addError(lError);
			gLog().write(Log::ERROR, std::string("[TxSpendBalanceVerify]: ") + lError);
			return TransactionAction::ERROR;			
		}

		return TransactionAction::SUCCESS;
	}

	return TransactionAction::CONTINUE;
}

TransactionAction::Result TxAssetTypeVerify::execute(TransactionContextPtr wrapper, ITransactionStorePtr store, IWalletPtr wallet, IEntityStorePtr entityStore) {
	//
	// all transaction types _must_ pass through this checker
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
				gLog().write(Log::ERROR, std::string("[TxAssetTypeVerify]: ") + lError);
				wrapper->tx()->setStatus(Transaction::DECLINED);
				wrapper->addError(lError);
				return TransactionAction::ERROR; 
			} 
			else if (lVM.getR(qasm::QA7).to<unsigned char>() != 0x01) {
				if (lTx->emission() == TxAssetType::LIMITED) {
					std::string lError = _getVMStateText(VirtualMachine::INVALID_AMOUNT);
					wrapper->tx()->setStatus(Transaction::DECLINED);
					wrapper->addError(lError);
					gLog().write(Log::ERROR, std::string("[TxAssetTypeVerify]: ") + lError);
				}
			} 

			if (wrapper->errors().size())
				return TransactionAction::ERROR;
			
			// extract commit
			if (lVM.getR(qasm::QA7).to<unsigned char>() == 0x01)
				wrapper->commitOut()[lOut.asset()].push_back(lVM.getR(qasm::QA1).to<std::vector<unsigned char>>());

			// extract fee
			if (lVM.getR(qasm::QD0).to<unsigned short>() == PKey::miner()) {
				wrapper->addFee(lVM.getR(qasm::QA0).to<uint64_t>());
			}
		}

		return TransactionAction::CONTINUE; // continue any way
	}

	return TransactionAction::CONTINUE;
}
