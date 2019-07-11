#include "qasm.h"

using namespace qbit;

unsigned char qasm::MAX_REG = 0x31;
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
		case QNOP: 		return "nop";
		case QRET: 		return "ret";
		case QMOV: 		return "mov";
		case QADD: 		return "add";
		case QSUB: 		return "sub";
		case QMUL: 		return "mul";
		case QDIV: 		return "div";
		case QCMP: 		return "cmp";
		case QCMPE:		return "cmpe";
		case QCMPNE:	return "cmpne";
		case QPUSHD:	return "pushd";
		case QPULLD:	return "pulld";
		case QLHASH256:	return "lhash256";
		case QCHECKSIG:	return "checksig";
		case QJMP:		return "jmp";
		case QJMPT:		return "jmpt";
		case QJMPF:		return "jmpf";
		case QJMPL:		return "jmpl";
		case QJMPG:		return "jmpg";
		case QJMPE:		return "jmpe";
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

		case QS0:	return "s00";
		case QS1:	return "s01";
		case QS2:	return "s02";
		case QS3:	return "s03";
		case QS4:	return "s04";
		case QS5:	return "s05";
		case QS6:	return "s06";
		case QS7:	return "s07";
		case QS8:	return "s08";
		case QS9:	return "s09";
		case QS10:	return "s10";
		case QS11:	return "s11";
		case QS12:	return "s12";
		case QS13:	return "s13";
		case QS14:	return "s14";
		case QS15:	return "s15";

		case QC0:	return "c00";
		case QC1:	return "c01";
		case QC2:	return "c02";
		case QC3:	return "c03";
		case QC4:	return "c04";
		case QC5:	return "c05";
		case QC6:	return "c06";
		case QC7:	return "c07";
		case QC8:	return "c08";
		case QC9:	return "c09";
		case QC10:	return "c10";
		case QC11:	return "c11";
		case QC12:	return "c12";
		case QC13:	return "c13";
		case QC14:	return "c14";
		case QC15:	return "c15";

		case QTH0:	return "th0";
		case QTH1:	return "th1";
	}

	return "EREG";
};
