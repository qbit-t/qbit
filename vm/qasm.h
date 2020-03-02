// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ASM_H
#define QBIT_ASM_H

#include "../uint256.h"
#include "../crypto/sha256.h"
#include "../hash.h"
#include "../containers.h"

#include "../crypto/common.h"
#include "../prevector.h"
#include "../serialize.h"

#include <assert.h>
#include <climits>
#include <limits>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>
#include <list>

namespace qbit {
namespace qasm {

enum _command {
	QNOP		= 0x00, // zero operands
	QRET		= 0x01, // zero operands
	QMOV		= 0x02, // two operands
	QADD		= 0x03, // two operands
	QSUB		= 0x04, // two operands
	QMUL		= 0x05, // two operands
	QDIV		= 0x06, // two operands

	QCMP		= 0x10, // two operands
	QCMPE		= 0x11, // two operands
	QCMPNE		= 0x12, // two operands

	QPUSHD		= 0x20, // zero operands
	QPULLD		= 0x21, // zero operands
	QLHASH256	= 0x22, // one operand
	QCHECKSIG	= 0x23, // zero operands
	QCHECKP		= 0x24, // zero operands
	QCHECKA		= 0x25, // zero operands
	QATXOP		= 0x26, // zero operands
	QATXOA		= 0x27, // zero operands
	QDTXO		= 0x28, // zero operands
	QEQADDR		= 0x29, // zero operands
	QATXO		= 0x2a, // zero operands
	QPAT		= 0x2b, // 
	QPEN		= 0x2c, // 
	QPTXO		= 0x2d, // 
	QDETXO		= 0x2e, //
	QTIFMC		= 0x2f, // 

	QJMP		= 0x30, // one operand
	QJMPT		= 0x31, // one operand
	QJMPF		= 0x32, // one operand
	QJMPL		= 0x33,
	QJMPG		= 0x34,
	QJMPE		= 0x35
};

enum _atom {
	QNONE	= 0x00,
	QOP		= 0x01,
	
	QREG	= 0x02,
	
	QI8		= 0x03,
	QI16	= 0x04,
	QI32	= 0x05,
	QI64	= 0x06,

	QUI8	= 0x07,
	QUI16	= 0x08,
	QUI32	= 0x09,
	QUI64	= 0x0a,

	QVAR	= 0x0b,
	
	QUI160	= 0x0d,
	QUI256	= 0x0e,
	QUI512	= 0x0f,
	
	QTO		= 0x0c,
	QLAB	= 0xcc
};

enum _register {
	QR0		= 0x00,
	QR1		= 0x01,
	QR2		= 0x02,
	QR3		= 0x03,
	QR4		= 0x04,
	QR5		= 0x05,
	QR6		= 0x06,
	QR7		= 0x07,
	QR8		= 0x08,
	QR9		= 0x09,
	QR10	= 0x0a,
	QR11	= 0x0b,
	QR12	= 0x0c,
	QR13	= 0x0d,
	QR14	= 0x0e,
	QR15	= 0x0f,
	QR16	= 0x10,
	QR17	= 0x11,
	QR18	= 0x12,
	QR19	= 0x13,
	QR20	= 0x14,
	QR21	= 0x15,
	QR22	= 0x16,
	QR23	= 0x17,
	QR24	= 0x18,
	QR25	= 0x19,
	QR26	= 0x1a,
	QR27	= 0x1b,
	QR28	= 0x1c,
	QR29	= 0x1d,
	QR30	= 0x1e,
	QR31	= 0x1f,

	QS0		= 0x20,
	QS1		= 0x21,
	QS2		= 0x22,
	QS3		= 0x23,
	QS4		= 0x24,
	QS5		= 0x25,
	QS6		= 0x26,
	QS7		= 0x27,

	QC0		= 0x28,
	QD0		= 0x29,

	QA0		= 0x2a,
	QA1		= 0x2b,
	QA2		= 0x2c,
	QA3		= 0x2d,
	QA4		= 0x2e,
	QA5		= 0x2f,
	QA6		= 0x30,
	QA7		= 0x31,

	QTH0	= 0x32,
	QTH1	= 0x33,
	QTH2	= 0x34,
	QTH3	= 0x35,
	QTH4	= 0x36,
	QTH5	= 0x37,
	QTH6	= 0x38,
	QTH7	= 0x39,
	QTH8	= 0x3a,
	QTH9	= 0x3b,
	QTH10	= 0x3c,
	QTH11	= 0x3d,
	QTH12	= 0x3e,
	QTH13	= 0x3f,
	QTH14	= 0x40,
	QTH15	= 0x41,

	QP0		= 0x42,

	QE0		= 0x43
};

extern int LAB_LEN;

extern unsigned char MAX_REG;

extern std::string _getCommandText(_command);
extern std::string _getRegisterText(_register);
extern std::string _getAtomText(_atom);

typedef prevector<256, unsigned char> Container;

extern unsigned char	_htole(unsigned char	v);
extern unsigned short	_htole(unsigned short	v);
extern uint32_t			_htole(uint32_t			v);
extern uint64_t			_htole(uint64_t			v);
extern uint160			_htole(uint160&			v);
extern uint256			_htole(uint256&			v);

extern unsigned char	_letoh(unsigned char	v);
extern unsigned short	_letoh(unsigned short	v);
extern uint32_t			_letoh(uint32_t			v);
extern uint64_t			_letoh(uint64_t			v);
extern uint160			_letoh(uint160&			v);
extern uint256			_letoh(uint256&			v);

//
// instruction type
class ATOM {
public:
	ATOM(_atom atom) : atom_(atom) {}

	void serialize(Container& data) { data.insert(data.end(), (unsigned char)atom_); }

public:
	_atom atom_;
};

//
// register
class REG : public ATOM {
public:
	_register register_;

	REG(_register r) : ATOM(QREG), register_(r) {}

	void serialize(Container& data) {
		ATOM::serialize(data);
		data.insert(data.end(), (unsigned char)register_);
	}

	static _register extract(Container& data, int& pos) {
		if (data[pos] <= MAX_REG) return (_register)data[pos++];
		pos++;
		return QR0; // default reg
	}
};

extern unsigned int INCORRECT_LABLE;

//
// label definition / 01: move ...
class LAB : public ATOM {
public:
	unsigned short label_;

	LAB(unsigned short label) : ATOM(QLAB), label_(label) {}

	void serialize(Container& data) {
		ATOM::serialize(data); // triple QLAB = cccccc
		data.insert(data.end(), QLAB);
		data.insert(data.end(), QLAB);
		unsigned short lData = _htole(label_);
		data.insert(data.end(), ((unsigned char*)&lData), ((unsigned char*)&lData)+sizeof(lData));
	}

	static unsigned short extract(Container& data, int& pos) {
		unsigned short lValue;
		if (data[pos++] == QLAB && data[pos++] == QLAB) {
			memcpy(&lValue, &data[pos], sizeof(unsigned short)); pos += sizeof(unsigned short);
			return _letoh(lValue);
		}

		return INCORRECT_LABLE;
	}
};

//
// label jump declaration / jmpe :01
class TO : public ATOM {
public:
	unsigned short labelTo_;

	TO(unsigned short label) : ATOM(QTO), labelTo_(label) {}

	void serialize(Container& data) {
		ATOM::serialize(data);
		unsigned short lData = _htole(labelTo_);
		data.insert(data.end(), ((unsigned char*)&lData), ((unsigned char*)&lData)+sizeof(lData));
	}

	static unsigned short extract(Container& data, int& pos) {
		unsigned short lValue;
		memcpy(&lValue, &data[pos], sizeof(unsigned short)); pos += sizeof(unsigned short);
		return _letoh(lValue);
	}
};

//
// macro-instruction
class OP : public ATOM {
public:
	_command code_;

	OP(_command code) : ATOM(QOP), code_(code) {}

	void serialize(Container& data) {
		ATOM::serialize(data);
		data.insert(data.end(), (unsigned char)code_);
	}

	static _command extract(Container& data, int& pos) {
		return (_command)data[pos++];
	}
};

//
// fixed length constant
template<class value, _atom code>
class Constant : public ATOM {
public:
	value data_;

	Constant(value data) : ATOM(code), data_(data) {}

	void serialize(Container& data) {
		ATOM::serialize(data);

		value lData = _htole(data_);
		data.insert(data.end(), ((unsigned char*)&lData), ((unsigned char*)&lData)+sizeof(value));
	}

	static value extract(Container& data, int& pos) {
		value lValue;
		memcpy(&lValue, &data[pos], sizeof(value)); pos += sizeof(value);
		return _letoh(lValue);
	}
};

template<class value, _atom code>
class UintNConstant : public ATOM {
public:
	value data_;

	UintNConstant(value data) : ATOM(code), data_(data) {}

	void serialize(Container& data) {
		ATOM::serialize(data);
		data.insert(data.end(), data_.begin(), data_.end());
	}

	static value extract(Container& data, int& pos) {
		value lValue;
		memcpy(lValue.begin(), &data[pos], lValue.size()); pos += lValue.size();
		return lValue;
	}
};

//
// variable length constant
class VarConstant : public ATOM {
public:
	static const size_t MAX_SIZE = 16384;
public:
	qbit::vector<unsigned char> data_;

	VarConstant(const qbit::vector<unsigned char>& data) : ATOM(QVAR), data_(data) {}

	void serialize(Container& data) {
		ATOM::serialize(data);
		if (data_.size() > MAX_SIZE) {
			data.insert(data.end(), 0x00);
			data.insert(data.end(), 0x00);
		} else {
			unsigned short lSize = _htole((unsigned short)data_.size()); 
			data.insert(data.end(), ((unsigned char*)&lSize)[0]);
			data.insert(data.end(), ((unsigned char*)&lSize)[1]);
			data.insert(data.end(), data_.begin(), data_.end());
		}
	}

	static void extract(Container& data, int& pos, qbit::vector<unsigned char>& result) {
		unsigned short lSize;
		memcpy(&lSize, &data[pos], sizeof(lSize)); pos += sizeof(lSize);

		lSize = _letoh(lSize);
		if (lSize > 0 && lSize <= MAX_SIZE) {
			result.insert(result.end(), (unsigned char*)&data[pos], ((unsigned char*)&data[pos]) + lSize);
		}
		pos += lSize;
	}
};

typedef Constant<unsigned char,		QUI8>  	CU8;
typedef Constant<unsigned short,	QUI16> 	CU16;
typedef Constant<uint32_t,			QUI32> 	CU32;
typedef Constant<uint64_t,			QUI64> 	CU64;
typedef UintNConstant<uint160,		QUI160> CU160;
typedef UintNConstant<uint256,		QUI256> CU256;
typedef UintNConstant<uint512,		QUI512> CU512;

typedef VarConstant							CVAR;

class ByteCode : public Container {
public:
	ByteCode() {}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITEAS(Container, *this);
	}

	ByteCode& operator << (ByteCode& code)
	{
		insert(end(), code.begin(), code.end());
		return *this;
	}

	ByteCode& operator << (TO l)
	{
		l.serialize(*this);
		return *this;
	}

	ByteCode& operator << (LAB l)
	{
		l.serialize(*this);
		return *this;
	}

	ByteCode& operator << (REG r)
	{
		r.serialize(*this);
		return *this;
	}

	ByteCode& operator << (OP op)
	{
		op.serialize(*this);
		return *this;
	}

	ByteCode& operator << (CU8 v)
	{
		v.serialize(*this);
		return *this;
	}

	ByteCode& operator << (CU16 v)
	{
		v.serialize(*this);
		return *this;
	}

	ByteCode& operator << (CU32 v)
 	{
		v.serialize(*this);
		return *this;
	}

	ByteCode& operator << (CU64 v)
	{
		v.serialize(*this);
		return *this;
	}

	ByteCode& operator << (CU160 v)
	{
		v.serialize(*this);
		return *this;
	}

	ByteCode& operator << (CU256 v)
	{
		v.serialize(*this);
		return *this;
	}

	ByteCode& operator << (CU512 v)
	{
		v.serialize(*this);
		return *this;
	}

	ByteCode& operator << (CVAR v)
	{
		v.serialize(*this);
		return *this;
	}
};

} // asm
} // qbit

#endif
