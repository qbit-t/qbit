
#include "unittest.h"
#include "keys.h"
#include "vm.h"
#include "transactions.h"
#include "blocks.h"
#include "dbs.h"
#include "cuckoo.h"

#include "../log/log.h"

#include <iostream>

using namespace qbit;
using namespace qbit::tests;

int main()
{
	std::cout << "q-bit.technology | unit tests v0.1" << std::endl << std::endl;
	TestSuit lSuit;

	gLog().enable(Log::ALL);

	lSuit << new CreateKeySignAndVerify();
	lSuit << new VMMovCmp();
	lSuit << new VMMovCmpJmpFalse();
	lSuit << new VMMovCmpJmpTrue();
	lSuit << new VMLoop();
	lSuit << new VMCheckLHash256();
	lSuit << new VMCheckSig();
	lSuit << new TxVerify();
	lSuit << new BlockCreate();
	lSuit << new DbContainerCreate();
	lSuit << new DbEntityContainerCreate();
	lSuit << new DbContainerIterator();
	lSuit << new DbMultiContainerIterator();
	lSuit << new DbContainerTransaction();
	lSuit << new DbEntityContainerTransaction();
	lSuit << new DbMultiContainerTransaction();
	lSuit << new DbMultiContainerRemove();
	lSuit << new DbEntityContainerRemove();
	lSuit << new DbContainerRemove();
	lSuit << new CuckooHash();

	lSuit.execute();

	// memstat
	//char stats[0x1000] = {0};
	//_jm_threads_print_stats(stats);
	//std::cout << std::endl << stats << std::endl;

	return 0;	
}
