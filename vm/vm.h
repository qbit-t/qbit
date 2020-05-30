// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_VM_H
#define QBIT_VM_H

#include "qasm.h"
#include "../helpers/secure.h"

#include "../transaction.h"
#include "../iwallet.h"
#include "../itransactionstore.h"
#include "../ientitystore.h"
#include "../transactioncontext.h"

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
		READONLY_REGISTER		= 0x16,
		ILLEGAL_REGISTER_STATE	= 0x17,
		LOOPS_FORBIDDEN			= 0x18,
		INVALID_AMOUNT			= 0x19,
		INVALID_PROOFRANGE		= 0x1a,
		INVALID_OUT 			= 0x1b,
		INVALID_ADDRESS			= 0x1c,
		INVALID_DESTINATION		= 0x1d,
		INVALID_COINBASE		= 0x1e,
		INVALID_UTXO			= 0x1f,
		UNKNOWN_REFTX			= 0x20,
		INVALID_BALANCE			= 0x21,
		INVALID_FEE				= 0x22,
		INVALID_RESULT			= 0x23,
		INVALID_ENTITY			= 0x24,
		ENTITY_NAME_EXISTS		= 0x25,
		INVALID_CHAIN			= 0x26
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

		unsigned char* begin() { return &data_[1]; }
		unsigned char* end() { return &data_[data_.size()]; }
		size_t size() { return data_.size() - 1; }

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

		inline unsigned char* getValue() { return &data_[1]; }
		inline void setData(qasm::_atom type, unsigned char* v, size_t len) {
			if (type == qasm::QVAR) {
				len = len > qasm::VarConstant::MAX_SIZE ? qasm::VarConstant::MAX_SIZE : len;
			}

			data_.resize(len + 1); // type(1b) | data
			data_[0] = type;
			memcpy(&data_[1], v, len);
		}

	protected:
		prevector<64, unsigned char> data_;
	};

	VirtualMachine(const qasm::ByteCode& code) : state_(UNDEFINED), pos_(0), code_(code), allowLoops_(false), dryRun_(false) 
	{ registers_.resize(qasm::MAX_REG + 1); }

	VirtualMachine(const qasm::ByteCode& code, bool allowLoops) : VirtualMachine(code) { allowLoops_ = allowLoops; }

	State execute();

	Register& getR(qasm::_register reg) { return registers_.at(reg); }
	void dumpState(std::stringstream& s);

	State state() { return state_; }
	qasm::_command lastCommand() { return lastCommand_; }
	qasm::_atom lastAtom() { return lastAtom_; }

	void setTransaction(TransactionContextPtr transaction) { wrapper_ = transaction; }
	void setContext(ContextPtr context) { context_ = context; }
	void setWallet(IWalletPtr wallet) { wallet_ = wallet; }
	void setTransactionStore(ITransactionStorePtr store) { store_ = store; }
	void setEntityStore(IEntityStorePtr entityStore) { entityStore_ = entityStore; }

	void setDryRun() { dryRun_ = true; }

public:
	class DisassemblyLine {
	public:
		DisassemblyLine() {}

		std::string label() { return label_; }
		std::string instruction() { return instruction_; }
		std::list<std::string>& params() { return params_; }

		std::string toString() {
			std::string lParams;
			for (std::list<std::string>::iterator lParam = params_.begin(); lParam != params_.end(); lParam++) {
				if (lParams.size()) lParams += ", ";
				lParams += *lParam;
			}

			return strprintf("%-5s %-15s %s", label_, instruction_, lParams);
		}

		void setLabel(unsigned short  label) { label_ = strprintf("%d:", label); }
		void setInstruction(const std::string& instruction) { instruction_ = instruction; }
		void addParam(const std::string& param) { params_.push_back(param); }

		void addLabel(unsigned short lab) {
			params_.push_back(strprintf(":%d", lab));
		}
		void addParam(unsigned char v) {
			params_.push_back(strprintf("cu08(%#x)", v));
		}
		void addParam(unsigned short v) {
			params_.push_back(strprintf("cu16(%#x)", v));
		}
		void addParam(uint32_t v) {
			params_.push_back(strprintf("cu32(%#x)", v));
		}
		void addParam(uint64_t v) {
			params_.push_back(strprintf("cu64(%#x)", v));
		}
		void addParam(uint160 v) {
			params_.push_back(strprintf("cu160(0x%s)", v.toHex()));
		}
		void addParam(uint256 v) {
			params_.push_back(strprintf("cu256(0x%s)", v.toHex()));
		}
		void addParam(uint512 v) {
			params_.push_back(strprintf("cu512(0x%s)", v.toHex()));
		}
		void addParam(char v) {
			params_.push_back(strprintf("c08(%#x)", v));
		}
		void addParam(short v) {
			params_.push_back(strprintf("c16(%#x)", v));
		}
		void addParam(int32_t v) {
			params_.push_back(strprintf("c32(%#x)", v));
		}
		void addParam(int64_t v) {
			params_.push_back(strprintf("c64(%#x)", v));
		}
		void addParam(qbit::vector<unsigned char>& v) {
			params_.push_back(strprintf("var(0x%s)", HexStr(v.begin(), v.end())));
		}

	private:
		std::string label_;
		std::string instruction_;
		std::list<std::string> params_;
	};

	void disassemble(std::list<DisassemblyLine>&);

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
	inline void qchecka();
	inline void qcheckp();
	inline void qatxop();
	inline void qatxoa();
	inline void qatxo();
	inline void qdtxo();
	inline void qdetxo();
	inline void qeqaddr();
	inline void qpat();
	inline void qpen();
	inline void qptxo();
	inline void qtifmc();
	inline void qcheckh();

	// disasm
	inline void qmov(DisassemblyLine&);
	inline void qadd(DisassemblyLine&);
	inline void qsub(DisassemblyLine&);
	inline void qmul(DisassemblyLine&);
	inline void qdiv(DisassemblyLine&);
	inline void qcmp(DisassemblyLine&);
	inline void qcmpe(DisassemblyLine&);
	inline void qcmpne(DisassemblyLine&);
	inline void qpushd(DisassemblyLine&);
	inline void qpulld(DisassemblyLine&);
	inline void qlhash256(DisassemblyLine&);
	inline void qchecksig(DisassemblyLine&);
	inline void qjmp(DisassemblyLine&);
	inline void qjmpt(DisassemblyLine&);
	inline void qjmpf(DisassemblyLine&);
	inline void qjmpl(DisassemblyLine&);
	inline void qjmpg(DisassemblyLine&);
	inline void qjmpe(DisassemblyLine&);
	inline void qchecka(DisassemblyLine&);
	inline void qcheckp(DisassemblyLine&);
	inline void qatxop(DisassemblyLine&);
	inline void qatxoa(DisassemblyLine&);
	inline void qatxo(DisassemblyLine&);
	inline void qdtxo(DisassemblyLine&);
	inline void qdetxo(DisassemblyLine&);
	inline void qeqaddr(DisassemblyLine&);
	inline void qpat(DisassemblyLine&);
	inline void qpen(DisassemblyLine&);
	inline void qptxo(DisassemblyLine&);
	inline void qtifmc(DisassemblyLine&);
	inline void qcheckh(DisassemblyLine&);	

	inline qasm::_atom nextAtom() { return (qasm::_atom)code_[pos_++]; }

	inline int locateLabel(unsigned short);

private:
	State state_;
	int pos_;
	qasm::ByteCode code_; // program byte-code
	bool allowLoops_; // allow loops
	std::map<unsigned short, int> labels_; // labels and positions

private:
	// all registers
	qbit::vector<Register> registers_;

	// signature context
	ContextPtr context_;

	// transaction context (wrapped tx)
	TransactionContextPtr wrapper_;

	// wallet
	IWalletPtr wallet_;

	// transaction store
	ITransactionStorePtr store_;

	// entity store
	IEntityStorePtr entityStore_;

	// last executed command
	qasm::_command lastCommand_;

	// last defined atom
	qasm::_atom lastAtom_;

	// skip extended processing (for quick info extractors)
	bool dryRun_;
};

extern std::string _getVMStateText(VirtualMachine::State);

template<> uint160 VirtualMachine::Register::to<uint160>();
template<> uint256 VirtualMachine::Register::to<uint256>();
template<> uint512 VirtualMachine::Register::to<uint512>();
template<> std::vector<unsigned char> VirtualMachine::Register::to<std::vector<unsigned char> >();

} // qbit

#endif
