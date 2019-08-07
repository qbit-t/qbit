// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_UNITTEST_ASSETS_H
#define QBIT_UNITTEST_ASSETS_H

#include <string>
#include <list>

#include "unittest.h"
#include "../itransactionstore.h"
#include "../iwallet.h"
#include "../ientitystore.h"
#include "../transactionvalidator.h"
#include "../transactionactions.h"
#include "../block.h"

#include "../txassettype.h"
#include "../txassetemission.h"

namespace qbit {
namespace tests {

class TxWalletA: public IWallet {
public:
	TxWalletA() {}

	SKey createKey(const std::list<std::string>& seed) {
		SKey lSKey(seed);
		lSKey.create();

		PKey lPKey = lSKey.createPKey();
		keys_[lPKey.id()] = lSKey;

		return lSKey;
	}

	SKey findKey(const PKey& pkey) {
		if (pkey.valid()) {
			std::map<uint160, SKey>::iterator lKey = keys_.find(pkey.id());
			if (lKey != keys_.end()) {
				return lKey->second;
			}
		}

		return SKey();
	}

	uint256 pushUnlinkedOut(Transaction::UnlinkedOutPtr out, TransactionContextPtr ctx) {
		if (utxo_.find(out->hash()) == utxo_.end()) {
			utxo_[out->hash()] = out;

			uint256 lAsset = out->out().asset();
			if (ctx == nullptr || !(ctx->tx()->type() == Transaction::ASSET_TYPE && out->out().asset() == ctx->tx()->id())) {
				assetUtxo_[lAsset][out->hash()] = out;

				//dumpUnlinkedOutByAsset();				
			}
		}

		return out->hash();
	}
	
	bool popUnlinkedOut(const uint256& hash) {
		std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lUTXO = utxo_.find(hash);
		if (lUTXO != utxo_.end()) {
			
			//if (assetUtxo_[lUTXO->second->out().asset()].find(hash) != assetUtxo_[lUTXO->second->out().asset()].end())
			//	std::cout << lUTXO->second->out().asset().toHex() << " " << lUTXO->second->out().toString() << " " << lUTXO->second->amount() << "\n";

			assetUtxo_[lUTXO->second->out().asset()].erase(hash);

			utxo_.erase(hash);
			//dumpUnlinkedOutByAsset();
			return true;
		}
		return false;
	}

	Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256& hash) {
		std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lIter = utxo_.find(hash);
		if (lIter != utxo_.end()) {
			return lIter->second;
		}

		return nullptr;
	}

	void dumpUnlinkedOutByAsset() {
		std::cout << "dumpUnlinkedOutByAsset ---- \n";
		for (std::map<uint256, std::map<uint256, Transaction::UnlinkedOutPtr>>::iterator lIter = assetUtxo_.begin();
			lIter != assetUtxo_.end(); lIter++) {
			for (std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lOutPtr = lIter->second.begin(); lOutPtr != lIter->second.end(); lOutPtr++) {
				std::cout << lIter->first.toHex() << " " << lOutPtr->second->out().toString() << " " << lOutPtr->second->amount() << "\n";
			}			
		}
	}

	Transaction::UnlinkedOutPtr findUnlinkedOutByAsset(const uint256& asset, amount_t amount) {
		std::map<uint256, std::map<uint256, Transaction::UnlinkedOutPtr>>::iterator lIter = assetUtxo_.find(asset);

		if (lIter != assetUtxo_.end()) {
			for (std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lOutPtr = lIter->second.begin(); lOutPtr != lIter->second.end(); lOutPtr++) {
				if (lOutPtr->second->amount() >= amount) return lOutPtr->second; 
			}
		}

		return nullptr;
	}

	amount_t balance() {
		return balance(TxAssetType::qbitAsset());
	}

	amount_t balance(const uint256& asset) {
		std::map<uint256, std::map<uint256, Transaction::UnlinkedOutPtr>>::iterator lIter = assetUtxo_.find(asset);
		amount_t lBalance = 0;
		if (lIter != assetUtxo_.end()) {
			for (std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lOutPtr = lIter->second.begin(); lOutPtr != lIter->second.end(); lOutPtr++) {
				lBalance += lOutPtr->second->amount();
			}
		}

		//dumpUnlinkedOutByAsset();

		return lBalance;
	}

	std::map<uint160, SKey> keys_;
	std::map<uint256, Transaction::UnlinkedOutPtr> utxo_;
	std::map<uint256, std::map<uint256, Transaction::UnlinkedOutPtr>> assetUtxo_;
};

class TxStoreA: public ITransactionStore {
public:
	TxStoreA() {}

	TransactionPtr locateTransaction(const uint256& tx) 
	{
		return txs_[tx]; 
	}

	void pushTransaction(TransactionPtr tx) {

		txs_[tx->hash()] = tx;
	}

	void pushBlock(BlockPtr block) {
		for(TransactionsContainer::iterator lIter = block->transactions().begin(); lIter != block->transactions().end(); lIter++) {
			txs_[(*lIter)->id()] = *lIter;
		}
	}	

	uint256 pushUnlinkedOut(Transaction::UnlinkedOutPtr out) {
		utxo_[out->hash()] = out;
		return out->hash();
	}
	
	bool popUnlinkedOut(const uint256& hash) {
		std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lIter = utxo_.find(hash);
		if (lIter != utxo_.end()) {
			utxo_.erase(hash);
			return true;
		}
		
		return false;
	}

	Transaction::UnlinkedOutPtr findUnlinkedOut(const uint256& hash) {
		std::map<uint256, Transaction::UnlinkedOutPtr>::iterator lIter = utxo_.find(hash);
		if (lIter != utxo_.end()) {
			return lIter->second;
		}

		return nullptr;
	}

	std::map<uint256, TransactionPtr> txs_;
	std::map<uint256, Transaction::UnlinkedOutPtr> utxo_;
};

class EntityStoreA: public IEntityStore {
public:
	EntityStoreA() {}

	EntityPtr locateEntity(const uint256& tx) {
		return enities_[tx]; 
	}
	
	bool pushEntity(const uint256& tx, EntityPtr entity) {

		TxAssetTypePtr lAssetType = TransactionHelper::to<TxAssetType>(entity);
		if (lAssetType) {
			std::map<std::string, uint256>::iterator lI = assetTypes_.find(lAssetType->shortName());
			if (lI != assetTypes_.end() && lI->second != entity->id()) return false;
			if (lI == assetTypes_.end()) {
				assetTypes_[lAssetType->shortName()] = entity->id();
			}
		}

		enities_[tx] = entity;

		return true;
	}

	bool pushEntity(const uint256& tx, TransactionContextPtr wrapper) {

		return pushEntity(tx, TransactionHelper::to<Entity>(wrapper->tx()));
	}

	std::map<uint256, EntityPtr> enities_;
	std::map<std::string, uint256> assetTypes_;
};

class TxAssetVerify: public Unit {
public:
	TxAssetVerify(): Unit("TxAssetVerify") {
		store_ = std::make_shared<TxStoreA>(); 
		wallet_ = std::make_shared<TxWalletA>(); 
		entityStore_ = std::make_shared<EntityStoreA>(); 

		seedMy_.push_back(std::string("fitness"));
		seedMy_.push_back(std::string("exchange"));
		seedMy_.push_back(std::string("glance"));
		seedMy_.push_back(std::string("diamond"));
		seedMy_.push_back(std::string("crystal"));
		seedMy_.push_back(std::string("cinnamon"));
		seedMy_.push_back(std::string("border"));
		seedMy_.push_back(std::string("arrange"));
		seedMy_.push_back(std::string("attitude"));
		seedMy_.push_back(std::string("between"));
		seedMy_.push_back(std::string("broccoli"));
		seedMy_.push_back(std::string("cannon"));
		seedMy_.push_back(std::string("crane"));
		seedMy_.push_back(std::string("double"));
		seedMy_.push_back(std::string("eyebrow"));
		seedMy_.push_back(std::string("frequent"));
		seedMy_.push_back(std::string("gravity"));
		seedMy_.push_back(std::string("hidden"));
		seedMy_.push_back(std::string("identify"));
		seedMy_.push_back(std::string("innocent"));
		seedMy_.push_back(std::string("jealous"));
		seedMy_.push_back(std::string("language"));
		seedMy_.push_back(std::string("leopard"));
		seedMy_.push_back(std::string("lobster"));		

		seedBob_.push_back(std::string("fitness"));
		seedBob_.push_back(std::string("exchange"));
		seedBob_.push_back(std::string("glance"));
		seedBob_.push_back(std::string("diamond"));
		seedBob_.push_back(std::string("crystal"));
		seedBob_.push_back(std::string("cinnamon"));
		seedBob_.push_back(std::string("border"));
		seedBob_.push_back(std::string("arrange"));
		seedBob_.push_back(std::string("attitude"));
		seedBob_.push_back(std::string("between"));
		seedBob_.push_back(std::string("broccoli"));
		seedBob_.push_back(std::string("cannon"));
		seedBob_.push_back(std::string("crane"));
		seedBob_.push_back(std::string("double"));
		seedBob_.push_back(std::string("eyebrow"));
		seedBob_.push_back(std::string("frequent"));
		seedBob_.push_back(std::string("gravity"));
		seedBob_.push_back(std::string("hidden"));
		seedBob_.push_back(std::string("identify"));
		seedBob_.push_back(std::string("innocent"));
		seedBob_.push_back(std::string("jealous"));
		seedBob_.push_back(std::string("language"));
		seedBob_.push_back(std::string("leopard"));
		seedBob_.push_back(std::string("catfish"));		
	}

	uint256 create_qbitTx(BlockPtr);
	TransactionPtr create_assetTypeTx(uint256, BlockPtr);

	bool execute();

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;

	std::list<std::string> seedMy_;
	std::list<std::string> seedBob_;
};

class TxAssetEmissionVerify: public Unit {
public:
	TxAssetEmissionVerify(): Unit("TxAssetEmissionVerify") {
		store_ = std::make_shared<TxStoreA>(); 
		wallet_ = std::make_shared<TxWalletA>(); 
		entityStore_ = std::make_shared<EntityStoreA>(); 

		seedMy_.push_back(std::string("fitness"));
		seedMy_.push_back(std::string("exchange"));
		seedMy_.push_back(std::string("glance"));
		seedMy_.push_back(std::string("diamond"));
		seedMy_.push_back(std::string("crystal"));
		seedMy_.push_back(std::string("cinnamon"));
		seedMy_.push_back(std::string("border"));
		seedMy_.push_back(std::string("arrange"));
		seedMy_.push_back(std::string("attitude"));
		seedMy_.push_back(std::string("between"));
		seedMy_.push_back(std::string("broccoli"));
		seedMy_.push_back(std::string("cannon"));
		seedMy_.push_back(std::string("crane"));
		seedMy_.push_back(std::string("double"));
		seedMy_.push_back(std::string("eyebrow"));
		seedMy_.push_back(std::string("frequent"));
		seedMy_.push_back(std::string("gravity"));
		seedMy_.push_back(std::string("hidden"));
		seedMy_.push_back(std::string("identify"));
		seedMy_.push_back(std::string("innocent"));
		seedMy_.push_back(std::string("jealous"));
		seedMy_.push_back(std::string("language"));
		seedMy_.push_back(std::string("leopard"));
		seedMy_.push_back(std::string("lobster"));		

		seedBob_.push_back(std::string("fitness"));
		seedBob_.push_back(std::string("exchange"));
		seedBob_.push_back(std::string("glance"));
		seedBob_.push_back(std::string("diamond"));
		seedBob_.push_back(std::string("crystal"));
		seedBob_.push_back(std::string("cinnamon"));
		seedBob_.push_back(std::string("border"));
		seedBob_.push_back(std::string("arrange"));
		seedBob_.push_back(std::string("attitude"));
		seedBob_.push_back(std::string("between"));
		seedBob_.push_back(std::string("broccoli"));
		seedBob_.push_back(std::string("cannon"));
		seedBob_.push_back(std::string("crane"));
		seedBob_.push_back(std::string("double"));
		seedBob_.push_back(std::string("eyebrow"));
		seedBob_.push_back(std::string("frequent"));
		seedBob_.push_back(std::string("gravity"));
		seedBob_.push_back(std::string("hidden"));
		seedBob_.push_back(std::string("identify"));
		seedBob_.push_back(std::string("innocent"));
		seedBob_.push_back(std::string("jealous"));
		seedBob_.push_back(std::string("language"));
		seedBob_.push_back(std::string("leopard"));
		seedBob_.push_back(std::string("catfish"));		
	}

	TxAssetEmissionVerify(const std::string& name): Unit(name) {
		store_ = std::make_shared<TxStoreA>(); 
		wallet_ = std::make_shared<TxWalletA>(); 
		entityStore_ = std::make_shared<EntityStoreA>(); 

		seedMy_.push_back(std::string("fitness"));
		seedMy_.push_back(std::string("exchange"));
		seedMy_.push_back(std::string("glance"));
		seedMy_.push_back(std::string("diamond"));
		seedMy_.push_back(std::string("crystal"));
		seedMy_.push_back(std::string("cinnamon"));
		seedMy_.push_back(std::string("border"));
		seedMy_.push_back(std::string("arrange"));
		seedMy_.push_back(std::string("attitude"));
		seedMy_.push_back(std::string("between"));
		seedMy_.push_back(std::string("broccoli"));
		seedMy_.push_back(std::string("cannon"));
		seedMy_.push_back(std::string("crane"));
		seedMy_.push_back(std::string("double"));
		seedMy_.push_back(std::string("eyebrow"));
		seedMy_.push_back(std::string("frequent"));
		seedMy_.push_back(std::string("gravity"));
		seedMy_.push_back(std::string("hidden"));
		seedMy_.push_back(std::string("identify"));
		seedMy_.push_back(std::string("innocent"));
		seedMy_.push_back(std::string("jealous"));
		seedMy_.push_back(std::string("language"));
		seedMy_.push_back(std::string("leopard"));
		seedMy_.push_back(std::string("lobster"));		

		seedBob_.push_back(std::string("fitness"));
		seedBob_.push_back(std::string("exchange"));
		seedBob_.push_back(std::string("glance"));
		seedBob_.push_back(std::string("diamond"));
		seedBob_.push_back(std::string("crystal"));
		seedBob_.push_back(std::string("cinnamon"));
		seedBob_.push_back(std::string("border"));
		seedBob_.push_back(std::string("arrange"));
		seedBob_.push_back(std::string("attitude"));
		seedBob_.push_back(std::string("between"));
		seedBob_.push_back(std::string("broccoli"));
		seedBob_.push_back(std::string("cannon"));
		seedBob_.push_back(std::string("crane"));
		seedBob_.push_back(std::string("double"));
		seedBob_.push_back(std::string("eyebrow"));
		seedBob_.push_back(std::string("frequent"));
		seedBob_.push_back(std::string("gravity"));
		seedBob_.push_back(std::string("hidden"));
		seedBob_.push_back(std::string("identify"));
		seedBob_.push_back(std::string("innocent"));
		seedBob_.push_back(std::string("jealous"));
		seedBob_.push_back(std::string("language"));
		seedBob_.push_back(std::string("leopard"));
		seedBob_.push_back(std::string("catfish"));		
	}

	uint256 create_qbitTx(BlockPtr);
	TransactionPtr create_assetTypeTx(uint256 qbit_utxo, BlockPtr, uint256& limitedOut, uint256& feeOut);
	TransactionPtr create_assetEmissionTx(uint256 qbit_utxo, uint256 assetType_utxo, BlockPtr);

	bool checkBlock(BlockPtr);

	virtual bool execute();

	ITransactionStorePtr store_; 
	IWalletPtr wallet_;
	IEntityStorePtr entityStore_;

	std::list<std::string> seedMy_;
	std::list<std::string> seedBob_;
};

class TxAssetEmissionSpend: public TxAssetEmissionVerify {
public:
	TxAssetEmissionSpend(): TxAssetEmissionVerify("TxAssetEmissionSpend") {
	}

	bool execute();
};

}
}

#endif