
#include "unittest.h"
#include "keys.h"
#include "vm.h"
#include "transactions.h"
#include "blocks.h"

#include <iostream>

using namespace qbit;
using namespace qbit::tests;

int main()
{
	std::cout << "q-bit.technology | unit tests v0.1" << std::endl << std::endl;
	TestSuit lSuit;

	lSuit << new CreateKeySignAndVerify();
	lSuit << new VMMovCmp();
	lSuit << new VMMovCmpJmpFalse();
	lSuit << new VMMovCmpJmpTrue();
	lSuit << new VMLoop();
	lSuit << new VMCheckLHash256();
	lSuit << new VMCheckSig();
	lSuit << new TxVerify();
	lSuit << new BlockCreate();

	lSuit.execute();

	// memstat
	char stats[0x1000] = {0};
	_jm_threads_print_stats(stats);
	std::cout << std::endl << stats << std::endl;

	return 0;	
}
