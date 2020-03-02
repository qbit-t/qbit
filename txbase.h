// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBASE_H
#define QBIT_TXBASE_H

#include "transaction.h"
#include "vm/vm.h"

namespace qbit {

//
// Base transaction
class TxBase : public Transaction {
public:
	TxBase() {
		type_ = Transaction::BASE;
	}

	In& addIn() {
		in_.resize(1);
		in_[0].out_.setNull();
		in_[0].ownership_ = ByteCode() <<
			OP(QMOV) << REG(QR0) << CU8(0x00);

		return in_[0];
	}

	Transaction::UnlinkedOutPtr addOut(const SKey& skey, const PKey& pkey, const uint256& asset, amount_t amount) {
		qbit::vector<unsigned char> lCommitment;

		uint256 lBlind = const_cast<SKey&>(skey).shared(pkey);
		Math::mul(lBlind, lBlind, Random::generate());

		if (!const_cast<SKey&>(skey).context()->createCommitment(lCommitment, lBlind, amount)) {
			throw qbit::exception("INVALID_COMMITMENT", "Commitment creation failed.");
		}

		out_.resize(1);
		out_[0].asset_ = asset;
		out_[0].destination_ = ByteCode() <<
			OP(QMOV) 	<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) <<
			OP(QMOV) 	<< REG(QA0) << CU64(amount) <<
			OP(QMOV) 	<< REG(QA1) << CVAR(lCommitment) <<
			OP(QMOV) 	<< REG(QA2) << CU256(lBlind) <<
			OP(QCHECKA) <<
			OP(QMOV) 	<< REG(QR0) << CU8(0x01) <<	
			OP(QRET);

		return Transaction::UnlinkedOut::instance(
			Transaction::Link(chain(), asset, 0), // link
			pkey,
			amount, // amount
			lBlind, // blinding key
			lCommitment // commit
		);
	}

	inline bool extractAddressAndAmount(PKey& pkey, amount_t& amount) {
		//
		Transaction::Out& lOut = (*out().begin());
		VirtualMachine lVM(lOut.destination());
		lVM.execute();

		// all ...base transactions are NON-private or blinded
		if (lVM.getR(qasm::QD0).getType() != qasm::QNONE && 
			lVM.getR(qasm::QA0).getType() != qasm::QNONE) {		
			amount = lVM.getR(qasm::QA0).to<uint64_t>();
			pkey.set<unsigned char*>(lVM.getR(qasm::QD0).begin(), lVM.getR(qasm::QD0).end());

			return true;
		}

		return false;
	}

	inline std::string name() { return "base"; }

	inline void serialize(DataStream& s) {}
	inline void deserialize(DataStream& s) {}

	bool isValue(UnlinkedOutPtr) { return true; }
	bool isEntity(UnlinkedOutPtr) { return false; }
};

typedef std::shared_ptr<TxBase> TxBasePtr;

class TxBaseCreator: public TransactionCreator {
public:
	TxBaseCreator() {}
	TransactionPtr create() { return std::make_shared<TxBase>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBaseCreator>(); }
};

}

#endif
