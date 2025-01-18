// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXCUBIX_MEDIA_DATA_H
#define QBIT_TXCUBIX_MEDIA_DATA_H

#include "cubix.h"
#include "../../containers.h"

namespace qbit {
namespace cubix {

//
class TxMediaData: public Entity {
public:
	TxMediaData() { type_ = TX_CUBIX_MEDIA_DATA; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << data_;
		s << signature_;
	}
	
	virtual void deserialize(DataStream& s) {
		s >> timestamp_;
		s >> data_;
		s >> signature_;
	}

	inline uint64_t timestamp() { return timestamp_; }
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	inline std::vector<unsigned char>& data() { return data_; }
	inline void setData(const std::vector<unsigned char>& data, const SKey& skey) {
		data_.insert(data_.end(), data.begin(), data.end());

		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << timestamp_;
		lSource << data_;
		
		uint256 lHash = Hash(lSource.begin(), lSource.end());
		if (!const_cast<SKey&>(skey).sign(lHash, signature_)) {
			throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
		}
	}

	inline bool verifySignature() {
		//
		PKey lPKey;
		if (extractAddress(lPKey)) {
			DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
			lSource << timestamp_;
			lSource << data_;

			uint256 lHash = Hash(lSource.begin(), lSource.end());
			return lPKey.verify(lHash, signature_);
		}

		return false;
	}

	inline bool extractAddress(PKey& pkey) {
		//
		Transaction::In& lIn = (*in().begin());
		VirtualMachine lVM(lIn.ownership());
		lVM.execute();

		if (lVM.getR(qasm::QS0).getType() != qasm::QNONE) {		
			pkey.set<unsigned char*>(lVM.getR(qasm::QS0).begin(), lVM.getR(qasm::QS0).end());

			return true;
		}

		return false;
	}	

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

	virtual In& addPrevDataIn(const SKey& skey, UnlinkedOutPtr utxo) {
		Transaction::In lIn;
		lIn.out().setNull();
		lIn.out().setChain(utxo->out().chain());
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
			OP(QDETXO)); // entity/check/push

		in_.push_back(lIn);
		return in_[in_.size()-1];
	}

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
		props["data"] = HexStr(data_.begin(), data_.end());
		props["signature"] = signature_.toHex();
	}

	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior 

	virtual bool isContinuousData() { return true; }

	virtual bool isFeeFree() { return true; }

	virtual std::string entityName() { return Entity::emptyName(); }

	virtual std::string name() { return "media_data"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}

protected:
	uint64_t timestamp_;
	std::vector<unsigned char> data_;
	uint512 signature_;
};

typedef std::shared_ptr<TxMediaData> TxMediaDataPtr;

class TxMediaDataCreator: public TransactionCreator {
public:
	TxMediaDataCreator() {}
	TransactionPtr create() { return std::make_shared<TxMediaData>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxMediaDataCreator>(); }
};

}
}

#endif
