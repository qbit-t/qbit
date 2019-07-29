#include "vm.h"

using namespace qbit;
using namespace qbit::tests;

bool VMMovCmp::execute() {
	ByteCode lCode; lCode <<
						OP(QMOV)  << REG(QR0) << CU16(0x10) <<
						OP(QMOV)  << REG(QR1) << CU16(0x10) <<
						OP(QCMPE) << REG(QR0) << REG(QR1) <<
						OP(QRET);

	VirtualMachine lVM(lCode);
	lVM.execute();

	if (lVM.state() != VirtualMachine::FINISHED) { error_ = _getVMStateText(lVM.state()); return false; }
	else if (lVM.getR(qasm::QC0).to<char>() != 0x01) {
		error_ = std::string("C0 = ") + lVM.getR(qasm::QC0).toHex();
		return false;
	}

	return true;
}

bool VMMovCmpJmpFalse::execute() {
	ByteCode lCode; lCode <<
						OP(QMOV)  << REG(QR0) << CU16(0x10) <<
						OP(QMOV)  << REG(QR1) << CU16(0x11) <<
						OP(QCMPE) << REG(QR0) << REG(QR1) <<
						OP(QJMPF) << TO(1000) <<
						OP(QMOV)  << REG(QR2) << CU16(0x15) <<
						OP(QRET)  <<
		LAB(1000) <<	OP(QMOV)  << REG(QR2) << CU16(0x16) <<
						OP(QRET);

	VirtualMachine lVM(lCode, false /*allow loops*/);
	lVM.execute();

	if (lVM.state() != VirtualMachine::FINISHED) { error_ = _getVMStateText(lVM.state()); return false; }
	else if (lVM.getR(qasm::QR2).to<unsigned short>() != 0x16) {
		error_ = std::string("R2 = ") + lVM.getR(qasm::QR2).toHex();
		return false;
	}

	return true;
}

bool VMMovCmpJmpTrue::execute() {
	ByteCode lCode; lCode <<
						OP(QMOV)  << REG(QR0) << CU16(0x10) <<
						OP(QMOV)  << REG(QR1) << CU16(0x10) <<
						OP(QCMPE) << REG(QR0) << REG(QR1) <<
						OP(QJMPT) << TO(1000) <<
						OP(QMOV)  << REG(QR2) << CU16(0x15) <<
						OP(QRET)  <<
		LAB(1000) <<	OP(QMOV)  << REG(QR2) << CU16(0x16) <<
						OP(QRET);

	VirtualMachine lVM(lCode, false /*allow loops*/);
	lVM.execute();

	if (lVM.state() != VirtualMachine::FINISHED) { error_ = _getVMStateText(lVM.state()); return false; }
	else if (lVM.getR(qasm::QR2).to<unsigned short>() != 0x16) {
		error_ = std::string("R2 = ") + lVM.getR(qasm::QR2).toHex();
		return false;
	}

	return true;
}

bool VMLoop::execute() {
	ByteCode lCode; lCode <<
						OP(QMOV)  << REG(QR0) << CU32(1000000) <<
						OP(QMOV)  << REG(QR1) << CU32(0x00) <<
						OP(QMOV)  << REG(QR2) << CU32(0x01) <<
		LAB(1001) <<	OP(QCMP)  << REG(QR1) << REG(QR0) <<
						OP(QJMPE) << TO(1000) <<
						OP(QADD)  << REG(QR1) << REG(QR2) <<
						OP(QJMP)  << TO(1001) <<
		LAB(1000) <<	OP(QRET);

	VirtualMachine lVM(lCode, true /*allow loops*/);
	lVM.execute();

	if (lVM.state() != VirtualMachine::FINISHED) { error_ = _getVMStateText(lVM.state()); return false; }
	else if (lVM.getR(qasm::QR1).to<uint32_t>() != 1000000) {
		error_ = std::string("R1 = ") + lVM.getR(qasm::QR1).toHex();
		return false;
	}

	return true;
}

bool VMCheckLHash256::execute() {
	// 0
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));

	SKey lKey0(lSeed0);
	lKey0.create();

	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TxCoinBase::as(TransactionFactory::create(Transaction::COINBASE));
	lTx->addIn();

	// 1.1
	// rewrite link data
	unsigned char* asset0 = (unsigned char*)"01234567890123456789012345678901";
	unsigned char* hash0 = (unsigned char*)"01234567890123456789012345678901";
	lTx->in()[0].out().setAsset(uint256(asset0)); 
	lTx->in()[0].out().setTx(uint256(hash0)); 
	lTx->in()[0].out().setIndex(0);

	// 1.2
	// prepare sample
	qbit::vector<unsigned char> lSource;
	lTx->in()[0].out().serialize(lSource);
	uint256 lHash = Hash(lSource.begin(), lSource.end());

	// 2.0
	// Make code
	ByteCode lCode; lCode <<
						OP(QLHASH256) << REG(QS0) <<
						OP(QRET);

	VirtualMachine lVM(lCode);
	lVM.getR(qasm::QTH2).set(0); // input number
	lVM.setTransaction(TransactionContext::instance(lTx)); // parametrization
	lVM.execute();

	if (lVM.state() != VirtualMachine::FINISHED) { error_ = _getVMStateText(lVM.state()); return false; }
	else if (lVM.getR(qasm::QS0).to<uint256>() != lHash) {
		error_ = std::string("S0 = ") + lVM.getR(qasm::QS0).toHex();
		return false;
	}

	return true;
}

bool VMCheckSig::execute() {
	// 0
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));

	SKey lKey0(lSeed0);
	lKey0.create();
	
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TxCoinBase::as(TransactionFactory::create(Transaction::COINBASE));
	lTx->addIn();

	// 1.1
	// rewrite link data
	unsigned char* asset0 = (unsigned char*)"01234567890123456789012345678901";
	unsigned char* hash0 = (unsigned char*)"01234567890123456789012345678901";
	lTx->in()[0].out().setAsset(uint256(asset0)); 
	lTx->in()[0].out().setTx(uint256(hash0)); 
	lTx->in()[0].out().setIndex(0);

	// 1.2
	// make sig
	qbit::vector<unsigned char> lSource;
	lTx->in()[0].out().serialize(lSource);
	uint256 lHash = Hash(lSource.begin(), lSource.end());
	uint512 lSig;

	if (!lKey0.sign(lHash, lSig)) { error_ = "Sighning error"; return false; }

	lTx->in()[0].setOwnership(ByteCode() <<
			OP(QMOV) 		<< REG(QS0) << CVAR(lPKey0.get()) << // pkey.get() - serialized pubkey, pkey.id().get() - hash160 pkey
			OP(QMOV) 		<< REG(QS1) << CU512(lSig) <<
			OP(QLHASH256) 	<< REG(QS2) <<
			OP(QCHECKSIG)	<<  // s7 result
			OP(QRET));

	// 1.3
	VirtualMachine lVM(lTx->in()[0].ownership());
	lVM.getR(qasm::QTH2).set(0); // input number
	lVM.setTransaction(TransactionContext::instance(lTx)); // parametrization
	lVM.execute();

	//std::stringstream lS;
	//lVM.dumpState(lS);
	//std::cout << std::endl << lS.str() << std::endl;

	if (lVM.state() != VirtualMachine::FINISHED) {
		error_ = _getVMStateText(lVM.state()) + " / " + 
			qasm::_getCommandText(lVM.lastCommand()) + ":" + qasm::_getAtomText(lVM.lastAtom()); 
		return false; 
	}
	else if (lVM.getR(qasm::QS7).to<unsigned char>() != 0x01) {
		error_ = std::string("S7 = ") + lVM.getR(qasm::QS7).toHex();
		return false;
	}

	return true;
}
