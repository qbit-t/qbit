#include "transaction.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"
#include "vm/vm.h"
#include "txassettype.h"

qbit::TransactionTypes qbit::gTxTypes;

void qbit::Transaction::registerTransactionType(Transaction::Type type, TransactionCreatorPtr creator) {
	qbit::gTxTypes[type] = creator;
}

uint256 qbit::Transaction::hash() {
	HashWriter lStream(SER_GETHASH, PROTOCOL_VERSION);
	Transaction::Serializer::serialize<HashWriter>(lStream, this);
    return (id_ = lStream.GetHash());	
}

std::string qbit::Transaction::Link::toString() const
{
    return strprintf("link(asset=%s..., tx=%s..., index=%u)", asset_.toString().substr(0,15), tx_.toString().substr(0,15), index_);
}

std::string qbit::Transaction::In::toString() const
{
	std::string str;
	str += "in(";
	str += out_.toString();
	str += strprintf(", owner=%s...)", HexStr(ownership_).substr(0, 30));
	return str;
}

std::string qbit::Transaction::Out::toString() const
{
    return strprintf("out(asset=%s..., destination=%s...)", asset_.toString().substr(0,15), HexStr(destination_).substr(0, 30));
}

std::string qbit::Transaction::toString() 
{
	std::string str;
	str += strprintf("%s(hash=%s..., ins=%u, outs=%u)\n",
		name(),
		hash().toString().substr(0,15),
		in_.size(),
		out_.size());
	
	for (const auto& tx_in : in_)
		str += "    " + tx_in.toString() + "\n";
	for (const auto& tx_out : out_)
		str += "    " + tx_out.toString() + "\n";
	return str;
}

//
//
using namespace qbit;

bool qbit::TxSpend::finalize(const SKey& skey) {
	for (_assetMap::iterator lGroup = assetIn_.begin(); lGroup != assetIn_.end(); lGroup++) {

		uint256 lBlindResult; // blinging = sum(in.blind) - sum(out.blind)
		amount_t lInAmount = 0;
		amount_t lOutAmount = 0;

		// sum inputs
		std::list<Transaction::UnlinkedOutPtr> lInList = lGroup->second;
		for (std::list<Transaction::UnlinkedOutPtr>::iterator lIUtxo = lInList.begin(); lIUtxo != lInList.end(); lIUtxo++) {
			if (!Math::add(lBlindResult, lBlindResult, (*lIUtxo)->blind())) {
				return false;
			}

			lInAmount += (*lIUtxo)->amount();
		}

		// sum outputs
		_assetMap::iterator lOutListPtr = assetOut_.find(lGroup->first);
		if (lOutListPtr != assetOut_.end()) {
			std::list<Transaction::UnlinkedOutPtr> lOutList = assetOut_[lGroup->first];
			for (std::list<Transaction::UnlinkedOutPtr>::iterator lOUtxo = lOutList.begin(); lOUtxo != lOutList.end(); lOUtxo++) {

				uint256 lBlind = (*lOUtxo)->blind();
				Math::neg(lBlind, lBlind);
				if (!Math::add(lBlindResult, lBlindResult, lBlind)) {
					return false;
				}
		
				lOutAmount += (*lOUtxo)->amount();
			}
		}

		if (!(lOutAmount == lInAmount && lOutAmount - lInAmount == 0 && lOutAmount != 0)) return false;

		//
		// add resulting commitment
		qbit::vector<unsigned char> lCommitment;
		if (!const_cast<SKey&>(skey).context()->createCommitment(lCommitment, lBlindResult, (uint64_t)0x00)) return false;

		Transaction::Out lOut;
		lOut.setAsset(lGroup->first);
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CU16(0xFFFF) << 
			OP(QMOV) 		<< REG(QA0) << CU64(0x00) <<
			OP(QMOV) 		<< REG(QA1) << CVAR(lCommitment) <<	
			OP(QMOV) 		<< REG(QA2) << CU256(lBlindResult) <<	
			OP(QCHECKA) 	<<			
			OP(QRET));

		out_.push_back(lOut); // add balancing out
	}

	//for (std::vector<Out>::iterator lOutIter = out_.begin(); lOutIter != out_.end(); lOutIter++) {
	//	if (lOutIter->asset() == TxAssetType::nullAsset()) lOutIter->setAsset(hash()); 
	//}

	// double check
	//for (std::vector<Out>::iterator lOutIter = out_.begin(); lOutIter != out_.end(); lOutIter++) {
	//	if (lOutIter->asset() == TxAssetType::nullAsset()) lOutIter->setAsset(hash()); 
	//}

	// adjust utxos tx references
	for (_assetMap::iterator lIter = assetOut_.begin(); lIter != assetOut_.end(); lIter++) {
		std::list<Transaction::UnlinkedOutPtr> lOutList = lIter->second;
		for (std::list<Transaction::UnlinkedOutPtr>::iterator lOUtxo = lOutList.begin(); lOUtxo != lOutList.end(); lOUtxo++) {
			if ((*lOUtxo)->out().asset() == TxAssetType::nullAsset()) (*lOUtxo)->out().setAsset(hash());
			(*lOUtxo)->out().setTx(hash());
			//std::cout << (*lOUtxo)->out().tx().toHex() << "\n";
		}
	}

	return true;
}

bool qbit::TxSpendPrivate::finalize(const SKey& skey) {
	for (_assetMap::iterator lGroup = assetIn_.begin(); lGroup != assetIn_.end(); lGroup++) {

		uint256 lBlindResult; // blinging = sum(in.blind) - sum(out.blind)
		amount_t lInAmount = 0;
		amount_t lOutAmount = 0;

		// sum inputs
		std::list<Transaction::UnlinkedOutPtr> lInList = lGroup->second;
		for (std::list<Transaction::UnlinkedOutPtr>::iterator lIUtxo = lInList.begin(); lIUtxo != lInList.end(); lIUtxo++) {
			if (!Math::add(lBlindResult, lBlindResult, (*lIUtxo)->blind())) {
				return false;
			}

			lInAmount += (*lIUtxo)->amount();
		}

		// sum outputs
		_assetMap::iterator lOutListPtr = assetOut_.find(lGroup->first);
		if (lOutListPtr != assetOut_.end()) {
			std::list<Transaction::UnlinkedOutPtr> lOutList = assetOut_[lGroup->first];
			for (std::list<Transaction::UnlinkedOutPtr>::iterator lOUtxo = lOutList.begin(); lOUtxo != lOutList.end(); lOUtxo++) {

				uint256 lBlind = (*lOUtxo)->blind();
				Math::neg(lBlind, lBlind);
				if (!Math::add(lBlindResult, lBlindResult, lBlind)) {
					return false;
				}
		
				lOutAmount += (*lOUtxo)->amount();
			}
		}

		if (!(lOutAmount == lInAmount && lOutAmount - lInAmount == 0 && lOutAmount != 0)) return false;

		//
		// add resulting commitment
		qbit::vector<unsigned char> lCommitment;
		if (!const_cast<SKey&>(skey).context()->createCommitment(lCommitment, lBlindResult, (uint64_t)0x00)) return false;

		qbit::vector<unsigned char> lRangeProof;
		if (!const_cast<SKey&>(skey).context()->createRangeProof(lRangeProof, lCommitment, lBlindResult, Random::generate(), (uint64_t)0x00)) return false;

		Transaction::Out lOut;
		lOut.setAsset(lGroup->first);
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CU16(0xFFFF) << 
			OP(QMOV) 		<< REG(QA0) << CU64(0x00) <<
			OP(QMOV) 		<< REG(QA1) << CVAR(lCommitment) <<	
			OP(QMOV) 		<< REG(QA2) << CVAR(lRangeProof) <<	
			OP(QCHECKP) 	<<
			OP(QRET));

		out_.push_back(lOut); // add balancing out
	}

	//for (std::vector<Out>::iterator lOutIter = out_.begin(); lOutIter != out_.end(); lOutIter++) {
	//	if (lOutIter->asset() == TxAssetType::nullAsset()) lOutIter->setAsset(hash()); 
	//}

	// adjust utxos tx references
	for (_assetMap::iterator lIter = assetOut_.begin(); lIter != assetOut_.end(); lIter++) {
		//size_t lIndex = 0;
		std::list<Transaction::UnlinkedOutPtr> lOutList = lIter->second;
		for (std::list<Transaction::UnlinkedOutPtr>::iterator lOUtxo = lOutList.begin(); lOUtxo != lOutList.end(); lOUtxo++) {
			if ((*lOUtxo)->out().asset() == TxAssetType::nullAsset()) (*lOUtxo)->out().setAsset(hash());
			(*lOUtxo)->out().setTx(hash());
			//(*lOUtxo)->out().setIndex(lIndex++);
		}
	}

	return true;	
}
