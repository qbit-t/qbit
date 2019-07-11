// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_VM_H
#define QBIT_VM_H

#include "qasm.h"
#include "../helpers/secure.h"

#include "../transaction.h"

#include <sstream>

namespace qbit {

class VirtualMachine {
public:
	enum State {
		UNDEFINED				= 0x00,
		RUNNING					= 0x01,
		FINISHED				= 0x02,
		ILLEGAL_INSTRUCTION		= 0x10,
		ILLEGAL_OPERAND_TYPE	= 0x11,
		ILLEGAL_COMMAND			= 0x12,
		UNKNOWN_ADDRESS			= 0x13,
		UNKNOWN_OBJECT			= 0x14,
		INVALID_SIGNATURE		= 0x15,
		READONLY_REGISTER		= 0x16
	};

	class Register {
	public:
		Register() { data_[0] = qasm::QNONE; }

		void set(unsigned char v) { setData(qasm::QUI8, (unsigned char*)&v, sizeof(v)); }
		void set(unsigned short v) { setData(qasm::QUI16, (unsigned char*)&v, sizeof(v)); }
		void set(uint32_t v) { setData(qasm::QUI32, (unsigned char*)&v, sizeof(v)); }
		void set(uint64_t v) { setData(qasm::QUI64, (unsigned char*)&v, sizeof(v)); }
		void set(uint160 v) { setData(qasm::QUI160, v.begin(), v.size()); }
		void set(uint256 v) { setData(qasm::QUI256, v.begin(), v.size()); }
		void set(uint512 v) { setData(qasm::QUI512, v.begin(), v.size()); }

		void set(char v) { setData(qasm::QI8, (unsigned char*)&v, sizeof(v)); }
		void set(short v) { setData(qasm::QI16, (unsigned char*)&v, sizeof(v)); }
		void set(int32_t v) { setData(qasm::QI32, (unsigned char*)&v, sizeof(v)); }
		void set(int64_t v) { setData(qasm::QI64, (unsigned char*)&v, sizeof(v)); }

		void set(qbit::vector<unsigned char>& v) { setData(qasm::QVAR, (unsigned char*)&v[0], v.size()); }
		void set(const Register& other) { data_.resize(other.data_.size()); memcpy(&data_[0], &other.data_[0], other.data_.size()); }

		qasm::_atom getType() { return (qasm::_atom)data_[0]; }

		void get(qbit::vector<unsigned char>& value) { value.insert(value.end(), data_.begin(), data_.end()); }
		prevector<64, unsigned char>& get() { return data_; }

		unsigned char* begin() { if (data_.size()) { if (getType() == qasm::QVAR) return &data_[2]; return &data_[1]; } return 0; }
		unsigned char* end() { if (data_.size()) return &data_[data_.size()]; return 0; }

		std::string toHex();

		template<class type>
		type to() {
			switch(getType()) {
				case qasm::QUI8: return (type)(*((unsigned char*)getValue()));
				case qasm::QUI16: return (type)(*((unsigned short*)getValue()));
				case qasm::QUI32: return (type)(*((uint32_t*)getValue()));
				case qasm::QUI64: return (type)(*((uint64_t*)getValue()));

				case qasm::QI8: return (type)(*((char*)getValue()));
				case qasm::QI16: return (type)(*((short*)getValue()));
				case qasm::QI32: return (type)(*((int32_t*)getValue()));
				case qasm::QI64: return (type)(*((int64_t*)getValue()));
			}

			return 0x00;
		}

		inline void add(Register& other) {
			switch(getType()) {
				case qasm::QUI8: set(to<unsigned char>() + other.to<unsigned char>()); break;
				case qasm::QUI16: set(to<unsigned short>() + other.to<unsigned short>()); break;
				case qasm::QUI32: set(to<uint32_t>() + other.to<uint32_t>()); break;
				case qasm::QUI64: set(to<uint64_t>() + other.to<uint64_t>()); break;

				case qasm::QI8: set(to<char>() + other.to<char>()); break;
				case qasm::QI16: set(to<short>() + other.to<short>()); break;
				case qasm::QI32: set(to<int32_t>() + other.to<int32_t>()); break;
				case qasm::QI64: set(to<int64_t>() + other.to<int64_t>()); break;
			}
		}

		inline void sub(Register& other) {
			switch(getType()) {
				case qasm::QUI8: set(to<unsigned char>() - other.to<unsigned char>()); break;
				case qasm::QUI16: set(to<unsigned short>() - other.to<unsigned short>()); break;
				case qasm::QUI32: set(to<uint32_t>() - other.to<uint32_t>()); break;
				case qasm::QUI64: set(to<uint64_t>() - other.to<uint64_t>()); break;

				case qasm::QI8: set(to<char>() - other.to<char>()); break;
				case qasm::QI16: set(to<short>() - other.to<short>()); break;
				case qasm::QI32: set(to<int32_t>() - other.to<int32_t>()); break;
				case qasm::QI64: set(to<int64_t>() - other.to<int64_t>()); break;
			}
		}

		inline void mul(Register& other) {
			switch(getType()) {
				case qasm::QUI8: set(to<unsigned char>() * other.to<unsigned char>()); break;
				case qasm::QUI16: set(to<unsigned short>() * other.to<unsigned short>()); break;
				case qasm::QUI32: set(to<uint32_t>() * other.to<uint32_t>()); break;
				case qasm::QUI64: set(to<uint64_t>() * other.to<uint64_t>()); break;

				case qasm::QI8: set(to<char>() * other.to<char>()); break;
				case qasm::QI16: set(to<short>() * other.to<short>()); break;
				case qasm::QI32: set(to<int32_t>() * other.to<int32_t>()); break;
				case qasm::QI64: set(to<int64_t>() * other.to<int64_t>()); break;
			}
		}

		inline void div(Register& other) {
			switch(getType()) {
				case qasm::QUI8: set(to<unsigned char>() / other.to<unsigned char>()); break;
				case qasm::QUI16: set(to<unsigned short>() / other.to<unsigned short>()); break;
				case qasm::QUI32: set(to<uint32_t>() / other.to<uint32_t>()); break;
				case qasm::QUI64: set(to<uint64_t>() / other.to<uint64_t>()); break;

				case qasm::QI8: set(to<char>() / other.to<char>()); break;
				case qasm::QI16: set(to<short>() / other.to<short>()); break;
				case qasm::QI32: set(to<int32_t>() / other.to<int32_t>()); break;
				case qasm::QI64: set(to<int64_t>() / other.to<int64_t>()); break;
			}
		}

		inline bool greate(Register& other) {
			switch(getType()) {
				case qasm::QUI8: return to<unsigned char>() > other.to<unsigned char>();
				case qasm::QUI16: return to<unsigned short>() > other.to<unsigned short>();
				case qasm::QUI32: return to<uint32_t>() > other.to<uint32_t>();
				case qasm::QUI64: return to<uint64_t>() > other.to<uint64_t>();
				
				case qasm::QI8: return to<char>() > other.to<char>();
				case qasm::QI16: return to<short>() > other.to<short>();
				case qasm::QI32: return to<int32_t>() > other.to<int32_t>();
				case qasm::QI64: return to<int64_t>() > other.to<int64_t>();
			}

			return false;
		}

		inline bool less(Register& other) {
			switch(getType()) {
				case qasm::QUI8: return to<unsigned char>() < other.to<unsigned char>();
				case qasm::QUI16: return to<unsigned short>() < other.to<unsigned short>();
				case qasm::QUI32: return to<uint32_t>() < other.to<uint32_t>();
				case qasm::QUI64: return to<uint64_t>() < other.to<uint64_t>();
				
				case qasm::QI8: return to<char>() < other.to<char>();
				case qasm::QI16: return to<short>() < other.to<short>();
				case qasm::QI32: return to<int32_t>() < other.to<int32_t>();
				case qasm::QI64: return to<int64_t>() < other.to<int64_t>();
			}

			return false;
		}

		inline bool equal(Register& other) {
			switch(getType()) {
				case qasm::QUI8: return to<unsigned char>() == other.to<unsigned char>();
				case qasm::QUI16: return to<unsigned short>() == other.to<unsigned short>();
				case qasm::QUI32: return to<uint32_t>() == other.to<uint32_t>();
				case qasm::QUI64: return to<uint64_t>() == other.to<uint64_t>();
				
				case qasm::QI8: return to<char>() == other.to<char>();
				case qasm::QI16: return to<short>() == other.to<short>();
				case qasm::QI32: return to<int32_t>() == other.to<int32_t>();
				case qasm::QI64: return to<int64_t>() == other.to<int64_t>();
			}

			return false;
		}

		template<class type>
		inline bool equal(type other) {
			return to<type>() == other;
		}

		inline bool equals(const Register& other) {
			return data_.size() == other.data_.size() && !memcmp(&data_[0], &other.data_[0], data_.size());
		}

		inline bool operator == (const Register& other) {
			return equals(other);
		}

		inline bool operator != (const Register& other) {
			return !equals(other);
		}

	protected:

		unsigned char* getValue() { if (getType() == qasm::QVAR) return &data_[2]; return &data_[1]; }
		void setData(qasm::_atom type, unsigned char* v, size_t len) {
			if (type == qasm::QVAR) {
				int lLen = len > 256 ? 256 : len;
				data_.resize(lLen + 1 + 1); // type(1b) | len(1b) | data
				data_[0] = type;
				data_[1] = lLen;
				memcpy(&data_[2], v, lLen);
			} else {
				data_.resize(len + 1); // type(1b) | data
				data_[0] = type;
				memcpy(&data_[1], v, len);
			}
		}

	protected:
		prevector<64, unsigned char> data_;
	};

	VirtualMachine(const qasm::ByteCode& code) : state_(UNDEFINED), pos_(0), code_(code) 
	{ input_ = 0; registers_.resize(16+16+16+2); }

	State execute();

	Register& getR(qasm::_register reg) { return registers_.at(reg); }
	void dumpState(std::stringstream& s);

	State state() { return state_; }
	qasm::_command lastCommand() { return lastCommand_; }
	qasm::_atom lastAtom() { return lastAtom_; }

	void setTransaction(TransactionPtr transaction) { transaction_ = transaction; }
	void setInput(uint32_t input) { input_ = input; }
	void setContext(ContextPtr context) { context_ = context; }

protected:
	inline void qmov();
	inline void qadd();
	inline void qsub();
	inline void qmul();
	inline void qdiv();
	inline void qcmp();
	inline void qcmpe();
	inline void qcmpne();
	inline void qpushd();
	inline void qpulld();
	inline void qlhash256();
	inline void qchecksig();
	inline void qjmp();
	inline void qjmpt();
	inline void qjmpf();
	inline void qjmpl();
	inline void qjmpg();
	inline void qjmpe();

	qasm::_atom nextAtom() { return (qasm::_atom)code_[pos_++]; }

	inline int locateLabel(unsigned short);

private:
	State state_;
	int pos_;
	qasm::ByteCode code_; // program byte-code
	std::map<unsigned short, int> labels_; // labels and positions

private:
	// all registers
	qbit::vector<Register> registers_;

	// transaction
	TransactionPtr transaction_;
	uint32_t input_;

	// context
	ContextPtr context_; 

	qasm::_command lastCommand_;
	qasm::_atom lastAtom_;
};

extern std::string _getVMStateText(VirtualMachine::State);

template<> uint160 VirtualMachine::Register::to<uint160>();
template<> uint256 VirtualMachine::Register::to<uint256>();
template<> uint512 VirtualMachine::Register::to<uint512>();

} // qbit

#endif
