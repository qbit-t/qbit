#include "pow.h"
#include "../itransactionstore.h"
#include "../transactionstore.h"

using namespace qbit;
using namespace qbit::tests;

class TxStore: public ITransactionStore {
public:
	TxStore() {}

	virtual bool blockHeader(const uint256& id, BlockHeader& header) 
	{
		if(id == 0)
			return  false;
		if(id == 2222)
			header.prev_ = 1111;
		return true;

	}
};

bool PowTest::execute() {
	ITransactionStorePtr store;
	//store->open();
	store = std::make_shared<TxStore>(); 
	BlockHeader current;
	uint32_t res = GetNextWorkRequired(store, current);
	if(res != 553713663)
	{
		error_ = "Initial work incorrect.";
		return false;
	}
	return true;
}

bool PowTestOne::execute() {
	ITransactionStorePtr store;
	//store->open();
	store = std::make_shared<TxStore>(); 
	BlockHeader current;
	current.prev_ = 11111;
	uint32_t res = GetNextWorkRequired(store, current);
	if(res != 553713663)
	{
		error_ = "Initial work incorrect.";
		return false;
	}
	return true;
}

bool PowTestTwo::execute() {
	ITransactionStorePtr store;
	//store->open();
	store = std::make_shared<TxStore>(); 
	BlockHeader current;
	current.prev_ = 2222;
	uint32_t res = GetNextWorkRequired(store, current);
	if(res != 553713663)
	{
		error_ = "Initial work incorrect.";
		return false;
	}
	return true;
}