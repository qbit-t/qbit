// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXCUBIX_MEDIA_SUMMARY_H
#define QBIT_TXCUBIX_MEDIA_SUMMARY_H

#include "cubix.h"

namespace qbit {
namespace cubix {

//
class TxMediaSummary: public Entity {
public:
	TxMediaSummary() { type_ = TX_CUBIX_MEDIA_SUMMARY; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << size_;
	}
	
	virtual void deserialize(DataStream& s) {
		s >> timestamp_;
		s >> size_;
	}

	inline uint64_t timestamp() { return timestamp_; }
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	inline uint64_t size() { return size_; }
	inline void setSize(uint64_t size) { size_ = size; }

	Transaction::UnlinkedOutPtr addDataOut(const SKey& skey, const PKey& pkey) {
		//
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
							OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
							OP(QEQADDR) 	<<
							OP(QPEN) 		<<
							OP(QPTXO)		<< // use in entity-based pushUnlinkedOut's
							OP(QMOV)		<< REG(QR1) << CU16(TX_CUBIX_MEDIA_DATA) <<
							OP(QCMPE)		<< REG(QTH1) << REG(QR1) <<
							OP(QJMPT) 		<< TO(1000) <<
						 	OP(QMOV)		<< REG(QR1) << CU16(TX_CUBIX_MEDIA_HEADER) <<
						 	OP(QCMPE)		<< REG(QTH1) << REG(QR1) <<
			LAB(1000) <<	OP(QMOV) 		<< REG(QR0) << REG(QC0) <<	
							OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(chain(), TxAssetType::nullAsset(), out_.size()), // link
			pkey
		);

		// fill up for finalization
		assetOut_[TxAssetType::nullAsset()].push_back(lUTXO);

		out_.push_back(lOut);
		return lUTXO;
	}

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
		props["size"] = strprintf("%d", size_);
	}

	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior 

	virtual bool isContinuousData() { return true; }

	virtual std::string entityName() { return Entity::emptyName(); }

	virtual std::string name() { return "media_summary"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}

protected:
	uint64_t timestamp_;
	uint64_t size_;
};

typedef std::shared_ptr<TxMediaSummary> TxMediaSummaryPtr;

class TxMediaSummaryCreator: public TransactionCreator {
public:
	TxMediaSummaryCreator() {}
	TransactionPtr create() { return std::make_shared<TxMediaSummary>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxMediaSummaryCreator>(); }
};

}
}

#endif
