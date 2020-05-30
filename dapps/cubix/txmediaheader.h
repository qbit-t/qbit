// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXCUBIX_MEDIA_HEADER_H
#define QBIT_TXCUBIX_MEDIA_HEADER_H

#include "cubix.h"

namespace qbit {
namespace cubix {

//
class TxMediaHeader: public Entity {
public:
	enum Type {
		UNKNOWN = 0,
		IMAGE_JPEG = 1,
		IMAGE_PNG = 2,
		VIDEO_MJPEG = 3
	};

public:
	TxMediaHeader() { type_ = TX_CUBIX_MEDIA_HEADER; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		cubix_media_name_t lName(mediaName_);
		s << mediaType_;
		s << timestamp_;
		s << size_;
		s << lName;
		s << description_;
		s << data_;
		s << signature_;
	}
	
	virtual void deserialize(DataStream& s) {
		cubix_media_name_t lName(mediaName_);
		s >> mediaType_;
		s >> timestamp_;
		s >> size_;
		s >> lName;
		s >> description_;
		s >> data_;
		s >> signature_;

		if (description_.size() > TX_CUBIX_MEDIA_DESCRIPTION_SIZE) description_.resize(TX_CUBIX_MEDIA_DESCRIPTION_SIZE);
	}

	inline Type mediaType() { return (Type)mediaType_; }
	inline void setMediaType(unsigned short type) { mediaType_ = type; }

	inline uint64_t timestamp() { return timestamp_; }
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	inline uint64_t size() { return size_; }
	inline void setSize(uint64_t size) { size_ = size; }

	inline std::vector<unsigned char>& description() { return description_; }
	inline void setDescription(const std::vector<unsigned char>& description) {
		if (description.size() <= TX_CUBIX_MEDIA_DESCRIPTION_SIZE) 
			description_ = description;
		else throw qbit::exception("E_SIZE", "Description size is incorrect.");
	}
	inline void setDescription(const std::string& description) {
		if (description.size() <= TX_CUBIX_MEDIA_DESCRIPTION_SIZE) 
			description_.insert(description_.end(), description.begin(), description.end()); 
		else throw qbit::exception("E_SIZE", "Description size is incorrect.");
	}

	inline std::string mediaName() { return mediaName_; }
	inline void setMediaName(const std::string& name) {
		if (name.size() <= TX_CUBIX_MEDIA_NAME_SIZE) 
			mediaName_ = name; 
		else throw qbit::exception("E_SIZE", "Media name size is incorrect.");
	}

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
		props["size"] = strprintf("%d", size_);
		props["name"] = mediaName_;

		std::string lDescription; lDescription.insert(lDescription.end(), description_.begin(), description_.end());
		props["description"] = lDescription;
		props["type"] = mediaTypeToString();
		props["signature"] = signature_.toHex();
	}

	inline std::string mediaTypeToString() {
		//
		switch(mediaType_) {
			case UNKNOWN: return "UNKNOWN";
			case IMAGE_PNG: return "image/png";
			case IMAGE_JPEG: return "image/jpeg";
			case VIDEO_MJPEG: return "video/mjpeg";
		}

		return "UNKNOWN";
	}

	inline std::string mediaTypeToExtension() {
		//
		switch(mediaType_) {
			case UNKNOWN: return ".unknown";
			case IMAGE_PNG: return ".png";
			case IMAGE_JPEG: return ".jpeg";
			case VIDEO_MJPEG: return ".mjpeg";
		}

		return "UNKNOWN";
	}

	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior

	virtual bool isContinuousData() { return true; } 

	virtual bool isFeeFee() { return true; }

	virtual std::string entityName() { return mediaName_; }

	virtual std::string name() { return "media_header"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}

protected:
	unsigned short mediaType_ = Type::UNKNOWN;
	uint64_t timestamp_;
	std::string mediaName_;
	std::vector<unsigned char> description_;
	std::vector<unsigned char> data_;
	uint64_t size_ = 0;
	uint512 signature_;
};

typedef std::shared_ptr<TxMediaHeader> TxMediaHeaderPtr;

class TxMediaHeaderCreator: public TransactionCreator {
public:
	TxMediaHeaderCreator() {}
	TransactionPtr create() { return std::make_shared<TxMediaHeader>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxMediaHeaderCreator>(); }
};

}
}

#endif
