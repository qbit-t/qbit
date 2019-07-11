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

	QS0		= 0x10,
	QS1		= 0x11,
	QS2		= 0x12,
	QS3		= 0x13,
	QS4		= 0x14,
	QS5		= 0x15,
	QS6		= 0x16,
	QS7		= 0x17,
	QS8		= 0x18,
	QS9		= 0x19,
	QS10	= 0x1a,
	QS11	= 0x1b,
	QS12	= 0x1c,
	QS13	= 0x1d,
	QS14	= 0x1e,
	QS15	= 0x1f,

	QC0		= 0x20,
	QC1		= 0x21,
	QC2		= 0x22,
	QC3		= 0x23,
	QC4		= 0x24,
	QC5		= 0x25,
	QC6		= 0x26,
	QC7		= 0x27,
	QC8		= 0x28,
	QC9		= 0x29,
	QC10	= 0x2a,
	QC11	= 0x2b,
	QC12	= 0x2c,
	QC13	= 0x2d,
	QC14	= 0x2e,
	QC15	= 0x2f,

	QTH0	= 0x30,
	QTH1	= 0x31
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
	/*
	static void extract(Container& data, int& pos, qbit::vector<unsigned char>& result) {
		result.resize(sizeof(value) + 1); result[0] = code;
		memcpy(&result[1], &data[pos], sizeof(value)); pos += sizeof(value);
	}
	*/
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

	/*
	static void extract(Container& data, int& pos, qbit::vector<unsigned char>& result) {
		result.resize(sizeof(value) + 1); result[0] = code;
		memcpy(&result[1], &data[pos], sizeof(value)); pos += sizeof(value);
	}
	*/
};

//
// variable length constant
class VarConstant : public ATOM {
public:
	qbit::vector<unsigned char> data_;

	VarConstant(qbit::vector<unsigned char> data) : ATOM(QVAR), data_(data) {}

	void serialize(Container& data) {
		ATOM::serialize(data);
		if (data_.size() > 256) {
			data.insert(data.end(), 0x00);
		} else {
			unsigned char lSize = _htole((unsigned char)data_.size()); // max 256 bytes
			data.insert(data.end(), lSize);
			data.insert(data.end(), data_.begin(), data_.end());
		}
	}

	static void extract(Container& data, int& pos, qbit::vector<unsigned char>& result) {
		unsigned char lSize = data[pos]; pos++;
		if (lSize > 0 && lSize <= 256) {
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
