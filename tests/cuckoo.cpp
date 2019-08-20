#include "cuckoo.h"

using namespace qbit;
using namespace qbit::tests;

bool CuckooHash::execute() {
	printf("\nmain solv\n");
	main_solv(0, 0);
	printf("solv\n");
	solver(68 /*nonce*/);
	char input[] = "Solution 6330 6fc9 a191 a9f2 b847 e22a ed86 11ee8 12c42 1a7da 1b479 1c032 27a88 29dee 2b5c1 2c03c 2ea3a 30440 31827 397a1 3f561 40979 42995 42e05 42fd5 47e43 4dbec 5108e 512de 5203b 56793 5d269 5fd3c 60a06 65942 6bdea 6d92a 6f1d6 71797 757c5 782e3 7edb4";
	//main_verify(0, 0, input);
	return true;
}
