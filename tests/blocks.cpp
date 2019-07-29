#include "blocks.h"

using namespace qbit;
using namespace qbit::tests;

TransactionPtr BlockCreate::createTx0(uint256& utxo) {
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

	SKey lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxCoinBasePtr lTx = TxCoinBase::as(TransactionFactory::create(Transaction::COINBASE));
	lTx->addIn();
	unsigned char* asset0 = (unsigned char*)"01234567890123456789012345678901";
	Transaction::UnlinkedOut lUTXO = lTx->addOut(lKey0, lPKey0, uint256(asset0), 10);
	lUTXO.out().setTx(lTx->id());

	lTx->finalize(lKey0); // bool

	//std::cout << std::endl << lTx->toString() << std::endl;

	store_->pushTransaction(lTx);
	store_->pushUnlinkedOut(lUTXO);
	utxo = wallet_->pushUnlinkedOut(lUTXO);
	return lTx;
}

TransactionPtr BlockCreate::createTx1(uint256 utxo) {
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

	SKey lKey0 = wallet_->createKey(lSeed0);
	PKey lPKey0 = lKey0.createPKey();

	// 1.0
	// create transaction
	TxSpendPtr lTx = TxSpend::as(TransactionFactory::create(Transaction::SPEND));

	Transaction::UnlinkedOut lUTXO;
	wallet_->findUnlinkedOut(utxo, lUTXO);
	lTx->addIn(lKey0, lUTXO);

	unsigned char* asset0 = (unsigned char*)"01234567890123456789012345678901";
	Transaction::UnlinkedOut lUTXO1 = lTx->addOut(lKey0, lPKey0, uint256(asset0), 10);
	lUTXO1.out().setTx(lTx->id());

	lTx->finalize(lKey0); // bool

	store_->pushTransaction(lTx);
	store_->pushUnlinkedOut(lUTXO);
	wallet_->pushUnlinkedOut(lUTXO);

	return lTx;
}

bool BlockCreate::execute() {

	//
	// create & check
	uint256 utxo; 
	TransactionPtr lTx0 = createTx0(utxo);
	TransactionPtr lTx1 = createTx1(utxo);

	BlockPtr lBlock = Block::create();
	lBlock->append(lTx0);
	lBlock->append(lTx1);

	//
	// serialize
	DataStream lStream(SER_NETWORK, 0);
	Block::Serializer::serialize<DataStream>(lStream, lBlock);	

	//std::cout << std::endl << "original->" << lBlock->toString() << std::endl;

	BlockPtr lBlock2 = Block::Deserializer::deserialize<DataStream>(lStream);

	//std::cout << std::endl << "restored->" << lBlock2->toString() << std::endl;

	if (!lBlock->equals(lBlock2)) { error_ = "Block is not identical"; return false; }
	return true;
}
