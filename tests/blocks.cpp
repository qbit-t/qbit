#include "blocks.h"

using namespace qbit;
using namespace qbit::tests;

TransactionPtr BlockCreate::createTx0() {
	// 0
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));

	SKey lKey0(lSeed0);
	lKey0.create();
	
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TxCoinBase::as(TransactionFactory::create(Transaction::COINBASE));
	lTx->initialize(lPKey0, 10);

	return lTx;
}

TransactionPtr BlockCreate::createTx1(uint256 tx0) {
	// 0
	// make pair (pubkey, key)
	std::list<std::string> lSeed0;
	lSeed0.push_back(std::string("fitness"));
	lSeed0.push_back(std::string("exchange"));
	lSeed0.push_back(std::string("glance"));
	lSeed0.push_back(std::string("diamond"));
	lSeed0.push_back(std::string("crystal"));
	lSeed0.push_back(std::string("cinnamon"));
	lSeed0.push_back(std::string("border"));
	lSeed0.push_back(std::string("arrange"));
	lSeed0.push_back(std::string("attitude"));
	lSeed0.push_back(std::string("between"));
	lSeed0.push_back(std::string("broccoli"));
	lSeed0.push_back(std::string("cannon"));
	lSeed0.push_back(std::string("crane"));
	lSeed0.push_back(std::string("double"));
	lSeed0.push_back(std::string("eyebrow"));
	lSeed0.push_back(std::string("frequent"));
	lSeed0.push_back(std::string("gravity"));
	lSeed0.push_back(std::string("hidden"));
	lSeed0.push_back(std::string("identify"));
	lSeed0.push_back(std::string("innocent"));
	lSeed0.push_back(std::string("jealous"));
	lSeed0.push_back(std::string("language"));
	lSeed0.push_back(std::string("leopard"));
	lSeed0.push_back(std::string("lobster"));

	SKey lKey0(lSeed0);
	lKey0.create();
	
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxSpendPtr lTx = TxSpend::as(TransactionFactory::create(Transaction::SPEND));

	lTx->in().resize(1); // add input
	lTx->in()[0].out().setHash(tx0);
	lTx->in()[0].out().setIndex(0);

	if (!lTx->initIn(lTx->in()[0], lPKey0, lKey0)) { error_ = "Signing failed"; return nullptr; }

	lTx->out().resize(1); // add out
	lTx->out()[0].setAmount(5);

	if (!lTx->initOut(lTx->out()[0], lPKey0)) { error_ = "Destination failed"; return nullptr; }

	return lTx;
}

bool BlockCreate::execute() {

	//
	// create & check
	TransactionPtr lTx0 = createTx0();
	TransactionPtr lTx1 = createTx1(lTx0->hash());

	BlockPtr lBlock = Block::create();
	lBlock->append(lTx0);
	lBlock->append(lTx1);

	//
	// serialize
	DataStream lStream(SER_NETWORK, 0);
	BlockSerializer::serialize<DataStream>(lStream, lBlock);	

	std::cout << std::endl << "original->" << lBlock->toString() << std::endl;

	BlockPtr lBlock2 = BlockDeserializer::deserialize<DataStream>(lStream);

	std::cout << std::endl << "restored->" << lBlock2->toString() << std::endl;

	if (!lBlock->equals(lBlock2)) { error_ = "Block is not identical"; return false; }
	return true;
}
