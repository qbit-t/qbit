#include "cuckoo.h"

using namespace qbit;
using namespace qbit::tests;

bool CuckooHash::execute() {
	main_solv(0, 0);
	//main_verify(0, 0);
	return true;
}
