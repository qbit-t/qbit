// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXASSETEMISSION_H
#define QBIT_TXASSETEMISSION_H

#include "entity.h"

#include <iostream>

namespace qbit {

//
// Asset emission transaction
// --------------------------
// in[0]  - qbit fee
// in[1]  - asset type
// out[0] - unlinked out a-la coinbase out
// out[1] - emission supply-amount (emission rest)
// out[2] - qbit fee change
class TxAssetEmission : public TxSpend {
public:
	TxAssetEmission() { type_ = Transaction::ASSET_EMISSION; }

	virtual bool isEntity() { return true; }
	virtual std::string entityName() { return Entity::emptyName(); }
	virtual inline void setChain(const uint256&) { chain_ = MainChain::id(); /* all entities live in mainchain */ }

	virtual In& addLimitedIn(const SKey& skey, UnlinkedOutPtr utxo) {
		Transaction::In lIn;
		lIn.out().setNull();
		lIn.out().setChain(MainChain::id());
		lIn.out().setAsset(utxo->out().asset());
		lIn.out().setTx(utxo->out().tx());
		lIn.out().setIndex(utxo->out().index());

		qbit::vector<unsigned char> lSource;
		lIn.out().serialize(lSource);
		
		uint256 lHash = Hash(lSource.begin(), lSource.end());
		uint512 lSig;

		if (!const_cast<SKey&>(skey).sign(lHash, lSig)) {
			throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
		}

		PKey lPKey = const_cast<SKey&>(skey).createPKey(); // pkey is allways the same
		lIn.setOwnership(ByteCode() <<
			OP(QMOV) 		<< REG(QS0) << CVAR(lPKey.get()) << 
			OP(QMOV) 		<< REG(QS1) << CU512(lSig) <<
			OP(QLHASH256) 	<< REG(QS2) <<
			OP(QCHECKSIG)	<<
			OP(QDTXO));

		// fill up for finalization
		assetIn_[utxo->out().asset()].push_back(utxo);

		in_.push_back(lIn);
		return in_[in_.size()-1];
	}

	virtual In& addPeggedIn(const SKey& skey, UnlinkedOutPtr utxo) {
		Transaction::In lIn;
		lIn.out().setNull();
		lIn.out().setChain(MainChain::id());
		lIn.out().setAsset(utxo->out().asset());
		lIn.out().setTx(utxo->out().tx());
		lIn.out().setIndex(utxo->out().index());

		qbit::vector<unsigned char> lSource;
		lIn.out().serialize(lSource);
		
		uint256 lHash = Hash(lSource.begin(), lSource.end());
		uint512 lSig;

		if (!const_cast<SKey&>(skey).sign(lHash, lSig)) {
			throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
		}

		PKey lPKey = const_cast<SKey&>(skey).createPKey(); // pkey is allways the same
		lIn.setOwnership(ByteCode() <<
			OP(QMOV) 		<< REG(QS0) << CVAR(lPKey.get()) << 
			OP(QMOV) 		<< REG(QS1) << CU512(lSig) <<
			OP(QLHASH256) 	<< REG(QS2) <<
			OP(QCHECKSIG));

		in_.push_back(lIn);
		return in_[in_.size()-1];
	}

	inline std::string name() { return "asset_emission"; }

	bool finalize(const SKey& skey) {
		bool lResult = TxSpend::finalize(skey);

		return lResult; 
	}
};

typedef std::shared_ptr<TxAssetEmission> TxAssetEmissionPtr;

class TxAssetEmissionCreator: public TransactionCreator {
public:
	TxAssetEmissionCreator() {}
	TransactionPtr create() { return std::make_shared<TxAssetEmission>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxAssetEmissionCreator>(); }
};

}

#endif
