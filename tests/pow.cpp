#include "pow.h"

using namespace qbit;
using namespace qbit::tests;

bool PowTest::execute() {
	BlockHeader current;
	uint32_t res = GetNextWorkRequired(current);
	return true;
}