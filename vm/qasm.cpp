#include "qasm.h"

using namespace qbit;

unsigned char qasm::MAX_REG = 0x44;
unsigned int qasm::INCORRECT_LABLE = 0xffff;
int qasm::LAB_LEN = 3;

unsigned char	qasm::_htole(unsigned char    v) { return v; }
unsigned short	qasm::_htole(unsigned short   v) { return htole16(v); }
uint32_t		qasm::_htole(uint32_t         v) { return htole32(v); }
uint64_t		qasm::_htole(uint64_t         v) { return htole64(v); }
uint160			qasm::_htole(uint160&         v) { return v; }
uint256			qasm::_htole(uint256&         v) { return v; }

unsigned char	qasm::_letoh(unsigned char    v) { return v; }
unsigned short	qasm::_letoh(unsigned short   v) { return le16toh(v); }
uint32_t		qasm::_letoh(uint32_t         v) { return le32toh(v); }
uint64_t		qasm::_letoh(uint64_t         v) { return le64toh(v); }
uint160			qasm::_letoh(uint160&         v) { return v; }
uint256			qasm::_letoh(uint256&         v) { return v; }

std::string qasm::_getCommandText(_command cmd) {
	switch(cmd) {
		case QNOP: 			return "nop";
		case QRET: 			return "ret";
		case QMOV: 			return "mov";
		case QADD: 			return "add";
		case QSUB: 			return "sub";
		case QMUL: 			return "mul";
		case QDIV: 			return "div";
		case QCMP: 			return "cmp";
		case QCMPE:			return "cmpe";
		case QCMPNE:		return "cmpne";
		case QPUSHD:		return "pushd";
		case QPULLD:		return "pulld";
		case QLHASH256:		return "lhash256";
		case QCHECKSIG:		return "checksig";
		case QJMP:			return "jmp";
		case QJMPT:			return "jmpt";
		case QJMPF:			return "jmpf";
		case QJMPL:			return "jmpl";
		case QJMPG:			return "jmpg";
		case QJMPE:			return "jmpe";
		case QCHECKP:		return "checkp";
		case QCHECKA:		return "checka";
		case QATXOP:		return "atxop";
		case QATXOA:		return "atxoa";
		case QDTXO:			return "dtxo";
		case QDETXO:		return "detxo";
		case QEQADDR:		return "eqaddr";
		case QATXO:			return "atxo";
		case QPAT:			return "pat";
		case QPEN:			return "pen";
		case QPTXO:			return "ptxo";
		case QTIFMC:		return "tifmc";
		case QCHECKH:		return "checkh";
	}

	return "EOP";
};

std::string qasm::_getAtomText(_atom atom) {
	switch(atom) {
		case QNONE:		return "none";
		case QOP: 		return "op";
		case QREG:		return "reg";
		case QI8:		return "c8";
		case QI16:		return "c16";
		case QI32:		return "c32";
		case QI64:		return "c64";
		case QUI8:		return "cu8";
		case QUI16:		return "cu16";
		case QUI32:		return "cu32";
		case QUI64:		return "cu64";
		case QVAR:		return "cvar";
		case QTO:		return "to";
		case QLAB:		return "lab";
		case QUI256:	return "cu256";
		case QUI160:	return "cu160";
		case QUI512:	return "cu512";
	}

	return "EATOM";
}

std::string qasm::_getRegisterText(_register reg) {
	switch(reg) {
		case QR0: 	return "r00";
		case QR1: 	return "r01";
		case QR2: 	return "r02";
		case QR3: 	return "r03";
		case QR4: 	return "r04";
		case QR5: 	return "r05";
		case QR6: 	return "r06";
		case QR7: 	return "r07";
		case QR8: 	return "r08";
		case QR9: 	return "r09";
		case QR10:	return "r10";
		case QR11:	return "r11";
		case QR12:	return "r12";
		case QR13:	return "r13";
		case QR14:	return "r14";
		case QR15:	return "r15";
		case QR16:	return "r16";
		case QR17:	return "r17";
		case QR18:	return "r18";
		case QR19:	return "r19";
		case QR20:	return "r20";
		case QR21:	return "r21";
		case QR22:	return "r22";
		case QR23:	return "r23";
		case QR24:	return "r24";
		case QR25:	return "r25";
		case QR26:	return "r26";
		case QR27:	return "r27";
		case QR28:	return "r28";
		case QR29:	return "r29";
		case QR30:	return "r30";
		case QR31:	return "r31";

		case QS0:	return "s00";
		case QS1:	return "s01";
		case QS2:	return "s02";
		case QS3:	return "s03";
		case QS4:	return "s04";
		case QS5:	return "s05";
		case QS6:	return "s06";
		case QS7:	return "s07";

		case QC0:	return "c00";
		case QD0:	return "d00";

		case QA0:	return "a00";
		case QA1:	return "a01";
		case QA2:	return "a02";
		case QA3:	return "a03";
		case QA4:	return "a04";
		case QA5:	return "a05";
		case QA6:	return "a06";
		case QA7:	return "a07";

		case QTH0:	return "t00";
		case QTH1:	return "t01";
		case QTH2:	return "t02";
		case QTH3:	return "t03";
		case QTH4:	return "t04";
		case QTH5:	return "t05";
		case QTH6:	return "t06";
		case QTH7:	return "t07";
		case QTH8:	return "t08";
		case QTH9:	return "t09";
		case QTH10:	return "t10";
		case QTH11:	return "t11";
		case QTH12:	return "t12";
		case QTH13:	return "t13";
		case QTH14:	return "t14";
		case QTH15:	return "t15";

		case QP0:	return "p00";

		case QE0: 	return "e00";
		case QC1: 	return "c01";
	}

	return "EREG";
};
