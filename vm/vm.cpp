#include "vm.h"
#include "../transaction.h"
#include "../utilstrencodings.h"

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
	}

	return "UNDEFINED";
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

		if (lLReg == qasm::QS15) {
			state_ = VirtualMachine::READONLY_REGISTER;
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

void VirtualMachine::qcmpe() {
	// two operands instruction, result -> c0
	// cmpe r0, r1

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		qasm::_register lLReg = qasm::REG::extract(code_, pos_);

		lastAtom_ = nextAtom();
		switch (lastAtom_) {
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

void VirtualMachine::qpushd() {}
void VirtualMachine::qpulld() {}

void VirtualMachine::qlhash256() {
	// one operand instruction
	// lhash256 s0

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QREG) {
		qasm::_register lLReg = qasm::REG::extract(code_, pos_);

		if (transaction_ != 0) {
			qbit::vector<unsigned char> lSource;
			transaction_->in()[input_].out().serialize(lSource);

			uint256 lHash = Hash(lSource.begin(), lSource.end());
			registers_[lLReg].set(lHash);
		} else { state_ = VirtualMachine::UNKNOWN_OBJECT; }

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
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

		// S15 - r/o
		registers_[qasm::QS15].set((unsigned char)0x00);
	} else {
		// S15 - r/o
		registers_[qasm::QS15].set((unsigned char)0x01);
	}
}

void VirtualMachine::qjmp() {
	// one operand instruction
	// jmp :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		//std::cout << lLabel;
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else {
			// goto
			pos_ = lPos;
			//std::cout << "->" << lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qjmpt() {
	// one operand instruction, if c0 = 1
	// jmpt :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		//std::cout << lLabel;
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else if (registers_[qasm::QC0].equal<unsigned char>(0x1)) {
			// goto
			pos_ = lPos;
			//std::cout << "->" << lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qjmpf() {
	// one operand instruction, if c0 = 0
	// jmpf :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		//std::cout << lLabel;
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else if (registers_[qasm::QC0].equal<unsigned char>(0x0)) {
			// goto
			pos_ = lPos;
			//std::cout << "->" << lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qjmpl() {
	// one operand instruction, if c0 = -1
	// jmpl :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		//std::cout << lLabel;
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else if (registers_[qasm::QC0].equal<unsigned char>(0xff)) {
			// goto
			pos_ = lPos;
			//std::cout << "->" << lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}
}

void VirtualMachine::qjmpg() {
	// one operand instruction, if c0 = 1
	// jmpg :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		//std::cout << lLabel;
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else if (registers_[qasm::QC0].equal<unsigned char>(0x01)) {
			// goto
			pos_ = lPos;
			//std::cout << "->" << lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
	}	
}

void VirtualMachine::qjmpe() {
	// one operand instruction, if c0 = 0
	// jmpg :lab

	lastAtom_ = nextAtom();
	if (lastAtom_ == qasm::QTO) {
		unsigned short lLabel = qasm::TO::extract(code_, pos_);
		//std::cout << lLabel;
		int lPos = locateLabel(lLabel);

		if (lPos == qasm::INCORRECT_LABLE) state_ = VirtualMachine::UNKNOWN_ADDRESS;
		else if (registers_[qasm::QC0].equal<unsigned char>(0x00)) {
			// goto
			pos_ = lPos;
			//std::cout << "->" << lPos;
		}

	} else {
		state_ = VirtualMachine::ILLEGAL_OPERAND_TYPE;
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
				default:
					state_ = VirtualMachine::ILLEGAL_COMMAND;
				break;
			}

			// std::cout << std::endl;
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

