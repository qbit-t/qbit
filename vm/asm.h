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
namespace asm {

enum OpCode {
	QNULL = 0x20,
	QPUSH = 0x21,
	QPOP  = 0x22,
	QCALL = 0x23,
	QDUP  = 0x24,

	QHASH160  = 0x30,
	QHASH256  = 0x31,
	QVERIFYEQ = 0x32,
	QCHECKSIG = 0x33
};

enum Type {
	QOP  = 0x01,
	QC8  = 0x02,
	QI16 = 0x03,
	QI32 = 0x04,
	QI64 = 0x05,
	QVAR = 0x06
};

typedef prevector<256, unsigned char> Container;

class ATOM {
public:
	ATOM(Type type) : type_(type) {}

	void serialize(Container& data) { data.insert(data.end(), (unsigned char)type_); }

public:
	Type type_;
};

class OP : public ATOM {
public:
	OpCode code_;

	OP(OpCode code) : ATOM(QOP), code_(code) {}

	void serialize(Container& data) {
		ATOM::serialize(data);
		data.insert(data.end(), (unsigned char)code_);
	}
};

unsigned char  	_htole(unsigned char  	v) { return v; }
unsigned short 	_htole(unsigned short 	v) { return htole16(v); }
uint32_t 	_htole(uint32_t 	v) { return htole32(v); }
uint64_t        _htole(uint64_t         v) { return htole64(v); }

template<class value, int code>
class Constant : public ATOM {
public:
        value data_;

        Constant(value data) : ATOM(code), data_(data) {}

        void serialize(Container& data) {
                ATOM::serialize(data);

                value lData = _htole(data_);
                for (int lIdx = 0; lIdx < sizeof(value); lIdx++)
                        data.insert(data.end(), ((unsigned char*)&lData)[lIdx]);
        }
};

class VarConstant : public ATOM {
public:
	quark::vector<unsigned char> data_;

	VarConstant(quark::vector<unsigned char> data) : ATOM(QVAR), data_(data) {}

	void serialize(Container& data) {
		ATOM::serialize(data);

		unsigned char lSize = _htole((unsigned char)data_.size()); // max 256 bytes
		for (int lIdx = 0; lIdx < lSize; lIdx++)
			data.insert(data.end(), data_[lIdx]);
	}
};

typedef Constant<unsigned char,  QC8>  	C8;
typedef Constant<unsigned short, QI16> 	C16;
typedef Constant<uint32_t,       QI32> 	C32;
typedef Constant<uint64_t,       QI64> 	C64;
typedef VarConstant			CVAR;

class ByteCode : public Container {
public:
	ByteCode() {}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITEAS(Container, *this);
	}

	Script& operator << (OP op)
	{
		op.serialize(*this);
		return *this;
	}

	Script& operator << (C8 v)
	{
		v.serialize(*this);
		return *this;
	}

	Script& operator << (C16 v)
	{
		v.serialize(*this);
		return *this;
	}

	Script& operator << (C32 v)
 	{
		v.serialize(*this);
		return *this;
	}

	Script& operator << (C64 v)
	{
		v.serialize(*this);
		return *this;
	}

	Script& operator << (CVAR v)
	{
		v.serialize(*this);
		return *this;
	}
};

} // asm
} // qbit

#endif
