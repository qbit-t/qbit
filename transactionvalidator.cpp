#include "transactionvalidator.h"
#include "vm/vm.h"

#include <iostream>

using namespace qbit;

bool TransactionValidator::validate(TransactionPtr tx) {

	uint32_t lIdx = 0;
	std::vector<Transaction::In>& lIns = tx->in();
	for(std::vector<Transaction::In>::iterator lInPtr = lIns.begin(); lInPtr != lIns.end(); lInPtr++, lIdx++) {

		Transaction::In& lIn = (*lInPtr);
		TransactionPtr lInTx = store_->locateTransaction(lIn.out().hash());
		if (lInTx != nullptr) {

			// sanity
			if (lIn.out().index() < lInTx->out().size()) {
				qasm::ByteCode lVerifyUsage;
				lVerifyUsage << lIn.ownership() << lInTx->out()[lIn.out().index()].destination();

				VirtualMachine lVM(lVerifyUsage);
				lVM.getR(qasm::QTH0).set(tx->hash());
				lVM.getR(qasm::QTH1).set((unsigned short)tx->type()); // tx type - 2b
				lVM.setTransaction(tx);
				lVM.setInput(lIdx);

				lVM.execute();

				//std::stringstream lOut;
				//lVM.dumpState(lOut);
				//std::cout << std::endl << lOut.str();				

				if (lVM.state() != VirtualMachine::FINISHED) {
					std::string lError = _getVMStateText(lVM.state()) + " | " + 
						qasm::_getCommandText(lVM.lastCommand()) + ":" + qasm::_getAtomText(lVM.lastAtom()); 
					tx->setStatus(Transaction::DECLINED);
					tx->setError(lError);
					return false; 
				}
				else if (lVM.getR(qasm::QS15).to<unsigned char>() != 0x01 && lVM.getR(qasm::QR0).to<unsigned char>() != 0x01) {
					std::string lError = "INVALID_SIGNATURE";
					tx->setStatus(Transaction::DECLINED);
					tx->setError(lError);
					return false;
				}

				return true;
			}
		} else {
			tx->setStatus(Transaction::PENDING);
		}
	}

	return false;
}
