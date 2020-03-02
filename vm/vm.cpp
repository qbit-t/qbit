#include "vm.h"
#include "../transaction.h"
#include "../utilstrencodings.h"
#include "../txassettype.h"

#include <iostream>

using namespace qbit;

std::string VirtualMachine::Register::toHex() { return HexStr(begin(), end()); }

template<> uint160 VirtualMachine::Register::to<uint160>() {
	if (getType() == qasm::QUI160) return uint160(getValue());
	return uint160();
}

template<> uint256 VirtualMachine::Register::to<uint256>() {
	if (getType() == qasm::QUI256) return uint256(getValue());
	return uint256();
}

template<> uint512 VirtualMachine::Register::to<uint512>() {
	if (getType() == qasm::QUI512) return uint512(getValue());
	return uint512();
}

template<> std::vector<unsigned char> VirtualMachine::Register::to<std::vector<unsigned char> >() {
	std::vector<unsigned char> lValue; 
	if (size()) lValue.insert(lValue.end(), begin(), end()); 
	return lValue;
}

std::string qbit::_getVMStateText(VirtualMachine::State state) {
	switch(state) {
		case VirtualMachine::UNDEFINED: return "UNDEFINED";
		case VirtualMachine::RUNNING: return "RUNNING";
		case VirtualMachine::FINISHED: return "FINISHED";
		case VirtualMachine::ILLEGAL_INSTRUCTION: return "ILLEGAL_INSTRUCTION";
		case VirtualMachine::ILLEGAL_OPERAND_TYPE: return "ILLEGAL_OPERAND_TYPE";
		case VirtualMachine::ILLEGAL_COMMAND: return "ILLEGAL_COMMAND";
		case VirtualMachine::UNKNOWN_ADDRESS: return "UNKNOWN_ADDRESS";
		case VirtualMachine::UNKNOWN_OBJECT: return "UNKNOWN_OBJECT";
		case VirtualMachine::INVALID_SIGNATURE: return "INVALID_SIGNATURE";
		case VirtualMachine::READONLY_REGISTER: return "READONLY_REGISTER";
		case VirtualMachine::ILLEGAL_REGISTER_STATE: return "ILLEGAL_REGISTER_STATE";
		case VirtualMachine::LOOPS_FORBIDDEN: return "LOOPS_FORBIDDEN";
		case VirtualMachine::INVALID_AMOUNT: return "INVALID_AMOUNT";
		case VirtualMachine::INVALID_PROOFRANGE: return "INVALID_PROOFRANGE";
		case VirtualMachine::INVALID_OUT: return "INVALID_OUT";
		case VirtualMachine::INVALID_ADDRESS: return "INVALID_ADDRESS";
		case VirtualMachine::INVALID_DESTINATION: return "INVALID_DESTINATION";
		case VirtualMachine::INVALID_COINBASE: return "INVALID_COINBASE";
		case VirtualMachine::INVALID_UTXO: return "INVALID_UTXO";
		case VirtualMachine::UNKNOWN_REFTX: return "UNKNOWN_REFTX";
		case VirtualMachine::INVALID_BALANCE: return "INVALID_BALANCE";
		case VirtualMachine::INVALID_FEE: return "INVALID_FEE";
		case VirtualMachine::INVALID_RESULT: return "INVALID_RESULT";
		case VirtualMachine::INVALID_ENTITY: return "INVALID_ENTITY";
		case VirtualMachine::ENTITY_NAME_EXISTS: return "ENTITY_NAME_EXISTS";
		case VirtualMachine::INVALID_CHAIN: return "INVALID_CHAIN";
	}

	return "ESTATE";
}

void VirtualMachine::dumpState(std::stringstream& s) {
	s << "VM state = " << _getVMStateText(state_) << std::endl;
	s << "VM registers:" << std::endl;

	for(int lReg = 0; lReg <= qasm::MAX_REG; lReg++) {
		if(registers_[lReg].getType() != qasm::QNONE) {
			s << "\t" << qasm::_getRegisterText((qasm::_register)lReg) << " " << qasm::_getAtomText(registers_[lReg].getType()) << 
					":" << HexStr(registers_[lReg].begin(), registers_[lReg].end()) << std::endl;
		}
	}
}

void VirtualMachine::qmov() {
	// two operands instruction
	// mov r0, 0x10
	// mov r0, r1

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		qasm::_register lLReg = qasm::REG::extract(code_, pos_);

		if (lLReg == qasm::QS2 || lLReg == qasm::QS7 || lLReg == qasm::QC0 || 
			lLReg == qasm::QA7 || lLReg == qasm::QP0 || lLReg == qasm::QE0 ||
			(lLReg >= qasm::QTH0 && lLReg <= qasm::QTH15)) 
		{
			state_ = VirtualMachine::READONLY_REGISTER;
			return;
		}
		else if ((lLReg == qasm::QS0 || lLReg == qasm::QA0 || lLReg == qasm::QA1 || lLReg == qasm::QA2 || lLReg == qasm::QD0) && 
			registers_[lLReg].getType() != qasm::QNONE) {
			state_ = VirtualMachine::ILLEGAL_REGISTER_STATE; // this registers can be assigned just once
			return;
		}

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QUI8: registers_[lLReg].set(qasm::CU8::extract(code_, pos_)); break;
			case qasm::QUI16: registers_[lLReg].set(qasm::CU16::extract(code_, pos_)); break;
			case qasm::QUI32: registers_[lLReg].set(qasm::CU32::extract(code_, pos_)); break;
			case qasm::QUI64: registers_[lLReg].set(qasm::CU64::extract(code_, pos_)); break;
						
			case qasm::QUI160: registers_[lLReg].set(qasm::CU160::extract(code_, pos_)); break;
			case qasm::QUI256: registers_[lLReg].set(qasm::CU256::extract(code_, pos_)); break;
			case qasm::QUI512: registers_[lLReg].set(qasm::CU512::extract(code_, pos_)); break;

			case qasm::QVAR: {
				qbit::vector<unsigned char> lValue;
				qasm::CVAR::extract(code_, pos_, lValue);
				registers_[lLReg].set(lValue);
			}
			break;
			case qasm::QREG: {
				qasm::_register lRReg = qasm::REG::extract(code_, pos_);
				registers_[lLReg].set(registers_[lRReg]);
			}
			break;

			default:
				state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
			break;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qmov(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QUI8: line.addParam(qasm::CU8::extract(code_, pos_)); break;
			case qasm::QUI16: line.addParam(qasm::CU16::extract(code_, pos_)); break;
			case qasm::QUI32: line.addParam(qasm::CU32::extract(code_, pos_)); break;
			case qasm::QUI64: line.addParam(qasm::CU64::extract(code_, pos_)); break;
						
			case qasm::QUI160: line.addParam(qasm::CU160::extract(code_, pos_)); break;
			case qasm::QUI256: line.addParam(qasm::CU256::extract(code_, pos_)); break;
			case qasm::QUI512: line.addParam(qasm::CU512::extract(code_, pos_)); break;

			case qasm::QVAR: {
				qbit::vector<unsigned char> lValue;
				qasm::CVAR::extract(code_, pos_, lValue);
				line.addParam(lValue);
			}
			break;
			case qasm::QREG: {
				line.addParam(qasm::REG::extract(code_, pos_));
			}
			break;
		}
	}
}

void VirtualMachine::qadd() {
	// two operands instruction
	// add r0, r1 -> result r0

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		qasm::_register lLReg = qasm::REG::extract(code_, pos_);

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				qasm::_register lRReg = qasm::REG::extract(code_, pos_);
				registers_[lLReg].add(registers_[lRReg]);
			}
			break;
			default:
				state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
			break;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qadd(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));
			}
			break;
		}
	}
}

void VirtualMachine::qsub() {
	// two operands instruction
	// sub r0, r1 -> result r0

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		qasm::_register lLReg = qasm::REG::extract(code_, pos_);

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				qasm::_register lRReg = qasm::REG::extract(code_, pos_);
				registers_[lLReg].sub(registers_[lRReg]);
			}
			break;
			default:
				state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
			break;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}	
}

void VirtualMachine::qsub(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));
			}
			break;
		}
	}	
}

void VirtualMachine::qmul() {
	// two operands instruction
	// mul r0, r1 -> result r0

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		qasm::_register lLReg = qasm::REG::extract(code_, pos_);

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				qasm::_register lRReg = qasm::REG::extract(code_, pos_);
				registers_[lLReg].mul(registers_[lRReg]);
			}
			break;
			default:
				state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
			break;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}		
}

void VirtualMachine::qmul(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));
			}
			break;
		}
	}		
}

void VirtualMachine::qdiv() {
	// two operands instruction
	// div r0, r1 -> result r0

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		qasm::_register lLReg = qasm::REG::extract(code_, pos_);

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				qasm::_register lRReg = qasm::REG::extract(code_, pos_);
				registers_[lLReg].div(registers_[lRReg]);
			}
			break;
			default:
				state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
			break;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}		
}

void VirtualMachine::qdiv(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));
			}
			break;
		}
	}		
}

void VirtualMachine::qcmp() {
	// two operands instruction, result -> c0 {-1, 0, 1}
	// cmp r0, r1

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		qasm::_register lLReg = qasm::REG::extract(code_, pos_);

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				qasm::_register lRReg = qasm::REG::extract(code_, pos_);

				if (registers_[lLReg].greate(registers_[lRReg])) registers_[qasm::QC0].set((char)1); // 0x01
				else if (registers_[lLReg].less(registers_[lRReg])) registers_[qasm::QC0].set((char)-1); //0xff
				else registers_[qasm::QC0].set((char)0); // 0x00
			}
			break;
			default:
				state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
			break;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qcmp(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));
			}
			break;
		}
	}
}

void VirtualMachine::qcmpe() {
	// two operands instruction, result -> c0
	// cmpe r0, r1

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		qasm::_register lLReg = qasm::REG::extract(code_, pos_);

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QUI8: registers_[qasm::QC0].set((char)registers_[lLReg].equal<unsigned char>(qasm::CU8::extract(code_, pos_))); break;
			case qasm::QUI16: registers_[qasm::QC0].set((char)registers_[lLReg].equal<unsigned short>(qasm::CU16::extract(code_, pos_))); break;
			case qasm::QUI32: registers_[qasm::QC0].set((char)registers_[lLReg].equal<uint32_t>(qasm::CU32::extract(code_, pos_))); break;
			case qasm::QUI64: registers_[qasm::QC0].set((char)registers_[lLReg].equal<uint64_t>(qasm::CU64::extract(code_, pos_))); break;
						
			case qasm::QUI160: registers_[qasm::QC0].set((char)registers_[lLReg].equal<uint160>(qasm::CU160::extract(code_, pos_))); break;
			case qasm::QUI256: registers_[qasm::QC0].set((char)registers_[lLReg].equal<uint256>(qasm::CU256::extract(code_, pos_))); break;
			case qasm::QUI512: registers_[qasm::QC0].set((char)registers_[lLReg].equal<uint512>(qasm::CU512::extract(code_, pos_))); break;

			case qasm::QVAR: {
				qbit::vector<unsigned char> lValue;
				qasm::CVAR::extract(code_, pos_, lValue);
				registers_[lLReg].set(lValue);
			}
			break;			
			case qasm::QREG: {
				qasm::_register lRReg = qasm::REG::extract(code_, pos_);
				registers_[qasm::QC0].set((char)registers_[lLReg].equals(registers_[lRReg]));
			}
			break;
			default:
				state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
			break;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qcmpe(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QUI8: line.addParam(qasm::CU8::extract(code_, pos_)); break;
			case qasm::QUI16: line.addParam(qasm::CU16::extract(code_, pos_)); break;
			case qasm::QUI32: line.addParam(qasm::CU32::extract(code_, pos_)); break;
			case qasm::QUI64: line.addParam(qasm::CU64::extract(code_, pos_)); break;
						
			case qasm::QUI160: line.addParam(qasm::CU160::extract(code_, pos_)); break;
			case qasm::QUI256: line.addParam(qasm::CU256::extract(code_, pos_)); break;
			case qasm::QUI512: line.addParam(qasm::CU512::extract(code_, pos_)); break;

			case qasm::QVAR: {
				qbit::vector<unsigned char> lValue;
				qasm::CVAR::extract(code_, pos_, lValue);
				line.addParam(lValue);
			}
			break;			
			case qasm::QREG: {
				line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));
			}
			break;
		}
	}
}

void VirtualMachine::qcmpne() {
	// two operands instruction, result -> c0
	// cmpne r0, r1

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		qasm::_register lLReg = qasm::REG::extract(code_, pos_);

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				qasm::_register lRReg = qasm::REG::extract(code_, pos_);
				registers_[qasm::QC0].set((char)
					!registers_[lLReg].equals(registers_[lRReg])
					);
			}
			break;
			default:
				state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
			break;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qcmpne(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
			case qasm::QREG: {
				line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));
			}
			break;
		}
	}
}

void VirtualMachine::qpushd() {}
void VirtualMachine::qpushd(DisassemblyLine& line) {}
void VirtualMachine::qpulld() {}
void VirtualMachine::qpulld(DisassemblyLine& line) {}

void VirtualMachine::qlhash256() {
	// one operand instruction
	// lhash256 s0

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		qasm::_register lLReg = qasm::REG::extract(code_, pos_);

		Register& lTH2 = registers_[qasm::QTH2];
		if (wrapper_ != 0 && lTH2.getType() != qasm::QNONE) {
			qbit::vector<unsigned char> lSource;
			wrapper_->tx()->in()[lTH2.to<uint32_t>()].out().serialize(lSource);

			uint256 lHash = Hash(lSource.begin(), lSource.end());
			registers_[lLReg].set(lHash);
		} else { state_ = VirtualMachine::UNKNOWN_OBJECT; }

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qlhash256(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		line.addParam(qasm::_getRegisterText(qasm::REG::extract(code_, pos_)));
	}
}

void VirtualMachine::qchecksig() {
	// null operands instruction
	// checksig #s0 - key, s1 - sig, s2 - message -> result c0

	Register& lS0 = registers_[qasm::QS0]; // PKey.get()
	Register& lS1 = registers_[qasm::QS1]; // uint512
	Register& lS2 = registers_[qasm::QS2]; // uint256

	PKey lPKey(context_);
	lPKey.set<unsigned char*>(lS0.begin(), lS0.end());

	if (!lPKey.verify(lS2.to<uint256>(), lS1.to<uint512>())) {
		// gneral failure - there is no option to continue normal execution
		// because script is a very flexible and there are exists some cases when intruder is able to
		// compose illegal spend conditions, which will be accepted by high level processing
		// so, we need to break the currect execution
		state_ = VirtualMachine::INVALID_SIGNATURE;

		// S7 - r/o
		registers_[qasm::QS7].set((unsigned char)0x00);
	} else {
		// S7 - r/o
		registers_[qasm::QS7].set((unsigned char)0x01);
		if (wrapper_ != 0) {
			wrapper_->addAddress(lPKey); // collect addresses
		} else {
			state_ = VirtualMachine::UNKNOWN_OBJECT;
		}
	}
}

void VirtualMachine::qchecksig(DisassemblyLine& line) {
	//
}

void VirtualMachine::qjmp() {
	// one operand instruction
	// jmp :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else {
			// goto
			if (!allowLoops_ && pos_ > lPos) state_ = VirtualMachine::LOOPS_FORBIDDEN;
			else pos_ = lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qjmp(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		line.addLabel(qasm::TO::extract(code_, pos_));
	}
}

void VirtualMachine::qjmpt() {
	// one operand instruction, if c0 = 1
	// jmpt :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else if (registers_[qasm::QC0].equal<unsigned char>(0x1)) {
			// goto
			if (pos_ > lPos && !allowLoops_) state_ = VirtualMachine::LOOPS_FORBIDDEN;
			else pos_ = lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qjmpt(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		line.addLabel(qasm::TO::extract(code_, pos_));
	}
}

void VirtualMachine::qjmpf() {
	// one operand instruction, if c0 = 0
	// jmpf :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else if (registers_[qasm::QC0].equal<unsigned char>(0x0)) {
			// goto
			if (pos_ > lPos && !allowLoops_) state_ = VirtualMachine::LOOPS_FORBIDDEN;
			else pos_ = lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qjmpf(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		line.addLabel(qasm::TO::extract(code_, pos_));
	}
}

void VirtualMachine::qjmpl() {
	// one operand instruction, if c0 = -1
	// jmpl :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else if (registers_[qasm::QC0].equal<unsigned char>(0xff)) {
			// goto
			if (pos_ > lPos && !allowLoops_) state_ = VirtualMachine::LOOPS_FORBIDDEN;
			else pos_ = lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qjmpl(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		line.addLabel(qasm::TO::extract(code_, pos_));
	}
}

void VirtualMachine::qjmpg() {
	// one operand instruction, if c0 = 1
	// jmpg :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else if (registers_[qasm::QC0].equal<unsigned char>(0x01)) {
			// goto
			if (pos_ > lPos && !allowLoops_) state_ = VirtualMachine::LOOPS_FORBIDDEN;
			else pos_ = lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qjmpg(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		line.addLabel(qasm::TO::extract(code_, pos_));
	}
}

void VirtualMachine::qjmpe() {
	// one operand instruction, if c0 = 0
	// jmpg :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else if (registers_[qasm::QC0].equal<unsigned char>(0x00)) {
			// goto
			if (!allowLoops_ && pos_ > lPos) state_ = VirtualMachine::LOOPS_FORBIDDEN;
			else pos_ = lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qjmpe(DisassemblyLine& line) {
	//
	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		line.addLabel(qasm::TO::extract(code_, pos_));
	}
}

int VirtualMachine::locateLabel(unsigned short label) {
	std::map<unsigned short, int>::iterator lLab = labels_.find(label);
	if (lLab != labels_.end()) return lLab->second;

	bool lFound = false;
	int lPos = 0, lTemp = 0;

	for(lPos = pos_; !lFound && lPos < code_.size() - qasm::LAB_LEN/*triple QLAB*/; lPos++) {
		if (code_[lPos] == qasm::QLAB && code_[lPos+1] == qasm::QLAB && code_[lPos+2] == qasm::QLAB) {
			lPos++; // shift QLAB
			if (qasm::LAB::extract(code_, lPos) == label) { 
				labels_[label] = lPos; lFound = true; break;
			}
		}
	}

	if (lFound) return lPos;
	return qasm::INCORRECT_LABLE;
}

void VirtualMachine::qtifmc() {
	// null operands instruction
	if (dryRun_) return; // skip processing

	if (wrapper_) {
		if (wrapper_->tx()->chain() == MainChain::id()) {
			state_ = VirtualMachine::INVALID_CHAIN; // general failure
		}
	}
}

void VirtualMachine::qtifmc(DisassemblyLine& line) {
}

void VirtualMachine::qchecka() {
	// null operands instruction
	// checka #verify amount [a0 - amount, a1 - commitment, a2 - blinding key => result a7]

	if (dryRun_) return; // skip processing

	Register& lA0 = registers_[qasm::QA0]; // amount
	Register& lA1 = registers_[qasm::QA1]; // commitment
	Register& lA2 = registers_[qasm::QA2]; // blinding key

	Context lContext;
	qbit::vector<unsigned char> lCommitment;

	if (!lContext.createCommitment(lCommitment, lA2.to<uint256>(), lA0.to<uint64_t>())) {
		state_ = VirtualMachine::INVALID_AMOUNT; // general failure
		return;
	}

	// better performance, than std::vector::operator ==()
	if (lCommitment.size() == lA1.size() && !memcmp(&lCommitment[0], lA1.begin(), lA1.size())) {
		// A7 - r/o
		registers_[qasm::QA7].set((unsigned char)0x01);		
	} else {
		state_ = VirtualMachine::INVALID_AMOUNT; // general failure
		// A7 - r/o
		registers_[qasm::QA7].set((unsigned char)0x00);		
	}
}

void VirtualMachine::qchecka(DisassemblyLine& line) {
}

void VirtualMachine::qcheckp() {
	// null operands instruction
	// checkp #verify amount [a0 - amount, a1 - commitment, a2 - blinding key => result a7]

	if (dryRun_) return; // skip processing

	Register& lA0 = registers_[qasm::QA0]; // zero amount
	Register& lA1 = registers_[qasm::QA1]; // commitment
	Register& lA2 = registers_[qasm::QA2]; // range proof

	// a0 must cointain uint64|0x00
	if (!(lA0.getType() == qasm::QUI64 && lA0.to<uint64_t>() == (uint64_t)0x00)) {
		state_ = VirtualMachine::INVALID_AMOUNT; // general failure
		return;
	}

	Context lContext;
	if (!lContext.verifyRangeProof(lA1.begin(), lA2.begin(), lA2.size())) {
		state_ = VirtualMachine::INVALID_PROOFRANGE; // general failure
		// A7 - r/o
		registers_[qasm::QA7].set((unsigned char)0x00);		
	} else {
		// A7 - r/o
		registers_[qasm::QA7].set((unsigned char)0x01);		
	}
}

void VirtualMachine::qcheckp(DisassemblyLine& line) {
}

void VirtualMachine::qptxo() {
	//
	if (dryRun_) return; // skip processing

	Register& lTH1 	= registers_[qasm::QTH1]; // tx type
	Register& lTH3 	= registers_[qasm::QTH3]; // current out number

	if (lTH1.getType() != qasm::QNONE && lTH3.getType() != qasm::QNONE) {
		//
		uint32_t lIndex = lTH3.to<uint32_t>();
		if (lIndex < wrapper_->tx()->out().size()) {
			// make link
			Transaction::Link lLink;
			uint256 lAsset = wrapper_->tx()->out()[lIndex].asset();
			// fix unlinked out
			if (lAsset == TxAssetType::nullAsset() && lTH1.to<unsigned short>() == Transaction::ASSET_TYPE) {
				lAsset = wrapper_->tx()->id();
			}

			lLink.setChain(wrapper_->tx()->chain());
			lLink.setAsset(lAsset);
			lLink.setTx(wrapper_->tx()->id());
			lLink.setIndex(lIndex);

			Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
				lLink // link
			);

			store_->pushUnlinkedOut(lUTXO, wrapper_); // push public
		} else {
			state_ = VirtualMachine::INVALID_OUT;
		}
/*
	} else {
		state_ = VirtualMachine::UNKNOWN_OBJECT;
*/
	}
}

void VirtualMachine::qptxo(DisassemblyLine& line) {
}

void VirtualMachine::qatxoa() {
	// null operands instruction
	// atxoa #push for d0 public amount - [th3 - out number, d0 - address, a0 - amount, a7 - check amount result]

	if (dryRun_) return; // skip processing

	Register& lD0 	= registers_[qasm::QD0]; // PKey.get()
	Register& lP0 	= registers_[qasm::QP0]; // dtxo -> p0 undefined 
	Register& lA0 	= registers_[qasm::QA0]; // amount
	Register& lA1 	= registers_[qasm::QA1]; // commit
	Register& lA2 	= registers_[qasm::QA2]; // blind
	Register& lA7 	= registers_[qasm::QA7]; // check amount result
	Register& lTH1 	= registers_[qasm::QTH1]; // tx type
	Register& lTH3 	= registers_[qasm::QTH3]; // current out number

	if (lA7.to<unsigned char>() == 0x01 && lP0.getType() == qasm::QNONE) { // amount checked by checka AND there was not spend 

		PKey lPKey(context_);
		lPKey.set<unsigned char*>(lD0.begin(), lD0.end());

		if (lPKey.valid()) {
			if (wrapper_ != 0 && store_ != 0 && lTH3.getType() != qasm::QNONE) {
				uint32_t lIndex = lTH3.to<uint32_t>();
				if (lIndex < wrapper_->tx()->out().size()) {
					// make link
					Transaction::Link lLink;
					uint256 lAsset = wrapper_->tx()->out()[lIndex].asset();
					// fix unlinked out
					if (lAsset == TxAssetType::nullAsset() && lTH1.to<unsigned short>() == Transaction::ASSET_TYPE) {
						lAsset = wrapper_->tx()->id();
					}

					lLink.setChain(wrapper_->tx()->chain());
					lLink.setAsset(lAsset);
					lLink.setTx(wrapper_->tx()->id());
					lLink.setIndex(lIndex);

					Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
						lLink, // link
						lPKey,
						lA0.to<uint64_t>(), // amount
						lA2.to<uint256>(), // blinding key
						lA1.to<std::vector<unsigned char> >() // commit
					);

					SKey lSKey = wallet_->findKey(lPKey);
					if (lSKey.valid())
						wallet_->pushUnlinkedOut(lUTXO, wrapper_); // push own UTXO
					store_->pushUnlinkedOut(lUTXO, wrapper_); // push public
				} else {
					state_ = VirtualMachine::INVALID_OUT;
				}

			} else {
				state_ = VirtualMachine::UNKNOWN_OBJECT;
			}
		}
	}
}

void VirtualMachine::qatxoa(DisassemblyLine& line) {
}

void VirtualMachine::qatxo() {
	// null operands instruction
	// atxo #push for d0 public

	if (dryRun_) return; // skip processing

	Register& lD0 	= registers_[qasm::QD0]; // PKey.get()
	Register& lP0 	= registers_[qasm::QP0]; // dtxo -> p0 undefined 
	Register& lTH3 	= registers_[qasm::QTH3]; // current out number

	if (lP0.getType() == qasm::QNONE) { // amount checked by checka AND there was not spend 

		PKey lPKey(context_);
		lPKey.set<unsigned char*>(lD0.begin(), lD0.end());

		if (lPKey.valid()) {
			if (wrapper_ != 0 && store_ != 0 && lTH3.getType() != qasm::QNONE) {
				uint32_t lIndex = lTH3.to<uint32_t>();
				if (lIndex < wrapper_->tx()->out().size()) {
					// make link
					Transaction::Link lLink;
					lLink.setChain(wrapper_->tx()->chain());
					lLink.setAsset(wrapper_->tx()->out()[lIndex].asset());
					lLink.setTx(wrapper_->tx()->id());
					lLink.setIndex(lIndex);

					Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
						lLink, // link
						lPKey
					);

					SKey lSKey = wallet_->findKey(lPKey);
					if (lSKey.valid())
						wallet_->pushUnlinkedOut(lUTXO, wrapper_); // push own UTXO
					store_->pushUnlinkedOut(lUTXO, wrapper_); // push public
				} else {
					state_ = VirtualMachine::INVALID_OUT;
				}

			} else {
				state_ = VirtualMachine::UNKNOWN_OBJECT;
			}
		}
	}
}

void VirtualMachine::qatxo(DisassemblyLine& line) {
}

void VirtualMachine::qatxop() {
	// null operands instruction
	// atxop #push for d0 private amount - [th3 - out number, d0 - address, a1 - commit, a2 - proof, a7 - check amount result]

	if (dryRun_) return; // skip processing

	Register& lD0 	= registers_[qasm::QD0]; // PKey.get()
	Register& lP0 	= registers_[qasm::QP0]; // dtxo -> p0 undefined 	
	Register& lA1 	= registers_[qasm::QA1]; // commit
	Register& lA2 	= registers_[qasm::QA2]; // proof
	Register& lA7 	= registers_[qasm::QA7]; // checkp amount result
	Register& lTH3 	= registers_[qasm::QTH3]; // current out number

	// for incoming transactions only
	if (lA7.to<unsigned char>() == 0x01 && lP0.getType() == qasm::QNONE) { // amount checked by checkp AND there was not spend 

		PKey lPKey(context_);
		lPKey.set<unsigned char*>(lD0.begin(), lD0.end());

		// only in case if we have destination for one of _our_ public keys - addresses
		// so, try to rewind proof range and extract actual amount

		if (wrapper_ != 0 && store_ != 0 && lTH3.getType() != qasm::QNONE) {
			uint32_t lIndex = lTH3.to<uint32_t>();
			if (lIndex < wrapper_->tx()->out().size()) {
				// make link
				Transaction::Link lLink;
				uint256 lHash = wrapper_->tx()->id(); // tx hash
				lLink.setChain(wrapper_->tx()->chain());
				lLink.setAsset(wrapper_->tx()->out()[lIndex].asset());
				lLink.setTx(lHash);
				lLink.setIndex(lIndex);

				std::vector<PKey>& lInAddresses = wrapper_->addresses();
				if (!lInAddresses.size()) { // no cached in-addresses, doing hard
					uint32_t lIdx = 0;
					for (std::vector<Transaction::In>::iterator lInPtr = wrapper_->tx()->in().begin(); lInPtr != wrapper_->tx()->in().end(); lInPtr++, lIdx++) {
						Transaction::In& lIn = (*lInPtr);

						VirtualMachine lVM(lIn.ownership()); // ownership script
						lVM.setDryRun();
						lVM.getR(qasm::QTH0).set(lHash);
						lVM.getR(qasm::QTH1).set((unsigned short)wrapper_->tx()->type()); // tx type - 2b
						lVM.getR(qasm::QTH2).set(lIdx); // input number
						lVM.setTransaction(wrapper_);
						lVM.setWallet(wallet_);
						lVM.setTransactionStore(store_);

						lVM.execute();

						if (lVM.state() == VirtualMachine::RUNNING || lVM.state() == VirtualMachine::FINISHED) {
							// s0 contains sender's address, extract it
							Register& lIS0 = lVM.getR(qasm::QS0);
							if (lIS0.getType() == qasm::QVAR) {
								PKey lIPKey(context_);
								lIPKey.set<unsigned char*>(lIS0.begin(), lIS0.end());
								if (lIPKey.valid()) {
									wrapper_->addAddress(lIPKey);
								} else {
									state_ = VirtualMachine::INVALID_ADDRESS;
									break;
								}
							} else {
								state_ = VirtualMachine::INVALID_ADDRESS;
								break;
							}
						}
					}
				}

				if (lInAddresses.size()) { // search and apply: try to select appropriate nonce
					uint64_t lAmount = 0x00;
					uint256 lBlindFactor;

					SKey lSKey = (wallet_ ? wallet_->findKey(lPKey) : SKey());
					if (lSKey.valid()) { // potentially our future input

						for (std::vector<PKey>::iterator lPKeyPtr = lInAddresses.begin(); lPKeyPtr != lInAddresses.end(); lPKeyPtr++) {
							uint256 lNonce = lSKey.shared(*lPKeyPtr);
							if (lSKey.context()->rewindRangeProof(&lAmount, lBlindFactor, lNonce, lA1.begin(), lA2.begin(), lA2.size())) {
								// success
								Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
									lLink, // link
									lPKey,
									lAmount, // amount
									lBlindFactor, // blinding key
									lA1.to<std::vector<unsigned char> >(), // commit
									lNonce
								);

								wallet_->pushUnlinkedOut(lUTXO, wrapper_); // push UTXO
							}
						}
					}

					// public 
					store_->pushUnlinkedOut(Transaction::UnlinkedOut::instance(lLink, lPKey), wrapper_); // push public

				} else {
					state_ = VirtualMachine::INVALID_DESTINATION;
				}

			} else {
				state_ = VirtualMachine::INVALID_OUT;
			}

		} else {
			state_ = VirtualMachine::UNKNOWN_OBJECT;
		}

	} else {
		state_ = VirtualMachine::INVALID_AMOUNT; // general failure
	}
}

void VirtualMachine::qatxop(DisassemblyLine& line) {
}

void VirtualMachine::qdtxo() {
	// null operands instruction
	// dtxo # th2 - in index, p0 - result

	if (dryRun_) return; // skip processing

	Register& lS0 	= registers_[qasm::QS0]; // our pubkey
	Register& lTH2	= registers_[qasm::QTH2]; // index

	if (lTH2.getType() != qasm::QNONE && wrapper_ != 0 && store_ != 0) {

		uint256 lHash = wrapper_->tx()->in()[lTH2.to<uint32_t>()].out().hash();
		if (store_->popUnlinkedOut(lHash, wrapper_)) {

			PKey lPKey(context_);
			lPKey.set<unsigned char*>(lS0.begin(), lS0.end());			
			SKey lSKey = (wallet_ ? wallet_->findKey(lPKey) : SKey());
			if (lSKey.valid()) {
				if (wallet_->popUnlinkedOut(lHash, wrapper_)) {
					registers_[qasm::QP0].set((unsigned char)0x01); // wallet_ successfully popped
				} else {
					state_ = VirtualMachine::INVALID_UTXO;		
					registers_[qasm::QP0].set((unsigned char)0x00);
				}
			} else {
				registers_[qasm::QP0].set((unsigned char)0x01); // store_ has successfully popped
			}
		} else {
			state_ = VirtualMachine::INVALID_UTXO;
			registers_[qasm::QP0].set((unsigned char)0x00);
		}
	} else {
		state_ = VirtualMachine::UNKNOWN_OBJECT;
		registers_[qasm::QP0].set((unsigned char)0x00);
	}
}

void VirtualMachine::qdtxo(DisassemblyLine& line) {
}

void VirtualMachine::qdetxo() {
	// null operands instruction
	// dtxo # th2 - in index, p0 - result

	if (dryRun_) return; // skip processing

	Register& lS0 	= registers_[qasm::QS0]; // our pubkey
	Register& lTH2	= registers_[qasm::QTH2]; // index

	if (lTH2.getType() != qasm::QNONE && wrapper_ != 0 && store_ != 0) {

		uint256 lHash = wrapper_->tx()->in()[lTH2.to<uint32_t>()].out().hash();
		
		if (wrapper_->tx()->isEntity()) { 
			registers_[qasm::QP0].set((unsigned char)0x01); // wallet_ successfully popped
			return;
		}

		if (store_->popUnlinkedOut(lHash, wrapper_)) {

			PKey lPKey(context_);
			lPKey.set<unsigned char*>(lS0.begin(), lS0.end());			
			SKey lSKey = (wallet_ ? wallet_->findKey(lPKey) : SKey());
			if (lSKey.valid()) {
				if (wallet_->popUnlinkedOut(lHash, wrapper_)) {
					registers_[qasm::QP0].set((unsigned char)0x01); // wallet_ successfully popped
				} else {
					state_ = VirtualMachine::INVALID_UTXO;		
					registers_[qasm::QP0].set((unsigned char)0x00);
				}
			} else {
				registers_[qasm::QP0].set((unsigned char)0x01); // store_ has successfully popped
			}
		} else {
			state_ = VirtualMachine::INVALID_UTXO;
			registers_[qasm::QP0].set((unsigned char)0x00);
		}
	} else {
		state_ = VirtualMachine::UNKNOWN_OBJECT;
		registers_[qasm::QP0].set((unsigned char)0x00);
	}
}

void VirtualMachine::qdetxo(DisassemblyLine& line) {
}

void VirtualMachine::qeqaddr() {

	Register& lD0 = registers_[qasm::QD0];
	Register& lS0 = registers_[qasm::QS0];

	if (lD0.getType() != qasm::QNONE && lS0.getType() != qasm::QNONE) {
		registers_[qasm::QE0].set((unsigned char)lS0.equals(lD0));
	}
}

void VirtualMachine::qeqaddr(DisassemblyLine& line) {
}

void VirtualMachine::qpat() {
	// strict
	Register& lTH0 = registers_[qasm::QTH0];
	Register& lTH1 = registers_[qasm::QTH1];

	if (lTH0.getType() != qasm::QNONE && lTH1.getType() != qasm::QNONE && wrapper_->tx()->isEntity()
		/*(lTH1.to<unsigned short>() == Transaction::ASSET_EMISSION || 
			lTH1.to<unsigned short>() == Transaction::ASSET_TYPE)*/) {
		// push transaction type
		if (entityStore_ != 0) {
			if (!entityStore_->pushEntity(lTH0.to<uint256>(), wrapper_)) {
				state_ = VirtualMachine::ENTITY_NAME_EXISTS;
			}
		} else {
			state_ = VirtualMachine::UNKNOWN_OBJECT;
		}
	} else {
			state_ = VirtualMachine::INVALID_ENTITY;
	}
}

void VirtualMachine::qpat(DisassemblyLine& line) {
}

void VirtualMachine::qpen() {
	// not strict
	Register& lTH0 = registers_[qasm::QTH0];
	Register& lTH1 = registers_[qasm::QTH1];

	if (lTH0.getType() != qasm::QNONE && lTH1.getType() != qasm::QNONE && wrapper_->tx()->isEntity()) {
		// push transaction type
		if (entityStore_ != 0) {
			if (!entityStore_->pushEntity(lTH0.to<uint256>(), wrapper_)) {
				state_ = VirtualMachine::ENTITY_NAME_EXISTS;
			}
		} else {
			state_ = VirtualMachine::UNKNOWN_OBJECT;
		}
	}
}

void VirtualMachine::qpen(DisassemblyLine& line) {
}

VirtualMachine::State VirtualMachine::execute()
{
	state_ = VirtualMachine::RUNNING;

	while(pos_ < code_.size() && state_ == VirtualMachine::RUNNING) {
		lastAtom_ = nextAtom();
		if (lastAtom_ == qasm::QOP) {
			lastCommand_ = qasm::OP::extract(code_, pos_);

			switch(lastCommand_) {
				case qasm::QNOP: break;
				case qasm::QRET: state_ = VirtualMachine::FINISHED; break;
				case qasm::QMOV: qmov(); break;
				case qasm::QADD: qadd(); break;
				case qasm::QSUB: qsub(); break;
				case qasm::QMUL: qmul(); break;
				case qasm::QDIV: qdiv(); break;
				case qasm::QCMP: qcmp(); break;
				case qasm::QCMPE: qcmpe(); break;
				case qasm::QCMPNE: qcmpne(); break;
				case qasm::QPUSHD: qpushd(); break;
				case qasm::QPULLD: qpulld(); break;
				case qasm::QLHASH256: qlhash256(); break;
				case qasm::QCHECKSIG: qchecksig(); break;
				case qasm::QJMP: qjmp(); break;
				case qasm::QJMPT: qjmpt(); break;
				case qasm::QJMPF: qjmpf(); break;
				case qasm::QJMPL: qjmpl(); break;
				case qasm::QJMPG: qjmpg(); break;
				case qasm::QJMPE: qjmpe(); break;
				case qasm::QCHECKA: qchecka(); break;
				case qasm::QCHECKP: qcheckp(); break;
				case qasm::QATXOP: qatxop(); break;
				case qasm::QATXOA: qatxoa(); break;
				case qasm::QATXO: qatxo(); break;
				case qasm::QDTXO: qdtxo(); break;
				case qasm::QDETXO: qdetxo(); break;
				case qasm::QEQADDR: qeqaddr(); break;
				case qasm::QPAT: qpat(); break;
				case qasm::QPEN: qpen(); break;
				case qasm::QPTXO: qptxo(); break;
				case qasm::QTIFMC: qtifmc(); break;
				default:
					state_ = VirtualMachine::ILLEGAL_COMMAND;
				break;
			}

		} else if (lastAtom_ == qasm::QLAB) {
			unsigned short lLabel = qasm::LAB::extract(code_, pos_);
			labels_[lLabel] = pos_; // next code pointer
		} else {
			// illegal instruction
			state_ = VirtualMachine::ILLEGAL_INSTRUCTION;
		}
	}

	return state_;
}

void VirtualMachine::disassemble(std::list<DisassemblyLine>& disassembly) {
	//
	while(pos_ < code_.size()) {
		// new line
		DisassemblyLine lLine;

		lastAtom_ = nextAtom();
		if (lastAtom_ == qasm::QOP) {
			lastCommand_ = qasm::OP::extract(code_, pos_);
			lLine.setInstruction(qasm::_getCommandText(lastCommand_));

			switch(lastCommand_) {
				case qasm::QMOV: qmov(lLine); break;
				case qasm::QADD: qadd(lLine); break;
				case qasm::QSUB: qsub(lLine); break;
				case qasm::QMUL: qmul(lLine); break;
				case qasm::QDIV: qdiv(lLine); break;
				case qasm::QCMP: qcmp(lLine); break;
				case qasm::QCMPE: qcmpe(lLine); break;
				case qasm::QCMPNE: qcmpne(lLine); break;
				case qasm::QPUSHD: qpushd(lLine); break;
				case qasm::QPULLD: qpulld(lLine); break;
				case qasm::QLHASH256: qlhash256(lLine); break;
				case qasm::QCHECKSIG: qchecksig(lLine); break;
				case qasm::QJMP: qjmp(lLine); break;
				case qasm::QJMPT: qjmpt(lLine); break;
				case qasm::QJMPF: qjmpf(lLine); break;
				case qasm::QJMPL: qjmpl(lLine); break;
				case qasm::QJMPG: qjmpg(lLine); break;
				case qasm::QJMPE: qjmpe(lLine); break;
				case qasm::QCHECKA: qchecka(lLine); break;
				case qasm::QCHECKP: qcheckp(lLine); break;
				case qasm::QATXOP: qatxop(lLine); break;
				case qasm::QATXOA: qatxoa(lLine); break;
				case qasm::QATXO: qatxo(lLine); break;
				case qasm::QDTXO: qdtxo(lLine); break;
				case qasm::QDETXO: qdetxo(lLine); break;
				case qasm::QEQADDR: qeqaddr(lLine); break;
				case qasm::QPAT: qpat(lLine); break;
				case qasm::QPEN: qpen(lLine); break;
				case qasm::QPTXO: qptxo(lLine); break;
				case qasm::QTIFMC: qtifmc(lLine); break;
				default:
					return;
			}

		} else if (lastAtom_ == qasm::QLAB) {
			lLine.setLabel(qasm::LAB::extract(code_, pos_));
		}

		disassembly.push_back(lLine);
	}
}