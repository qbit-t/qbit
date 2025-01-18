// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXCUBIX_MEDIA_HEADER_H
#define QBIT_TXCUBIX_MEDIA_HEADER_H

#include "cubix.h"

namespace qbit {
namespace cubix {

#define TX_CUBIX_MEDIA_HEADER_VERSION_1 1 // + orientation
#define TX_CUBIX_MEDIA_HEADER_VERSION_2 2 // + duration
#define TX_CUBIX_MEDIA_HEADER_VERSION_3 4 // + preview

//
class TxMediaHeader: public Entity {
public:
	enum MediaType {
		UNKNOWN = 0,
		IMAGE_JPEG = 1,
		IMAGE_PNG = 2,
		VIDEO_MJPEG = 3, // motion jpeg, mjpeg
		VIDEO_MP4 = 4, // video + multichannel audio, mp4 
		AUDIO_PCM = 5, // multichannel audio, wav
		AUDIO_AMR = 6, // voice, amr
		DOCUMENT_PDF = 7,
		AUDIO_MP3 = 8,
		AUDIO_M4A = 9,

		MAX_TYPES = 50
	};

public:
	TxMediaHeader() { type_ = TX_CUBIX_MEDIA_HEADER; version_ = TX_CUBIX_MEDIA_HEADER_VERSION_3; }

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

		if (version_ >= TX_CUBIX_MEDIA_HEADER_VERSION_1)
			s << orientation_;
		if (version_ >= TX_CUBIX_MEDIA_HEADER_VERSION_2)
			s << duration_;
		if (version_ >= TX_CUBIX_MEDIA_HEADER_VERSION_3)
			s << previewType_;
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

		if (version_ >= TX_CUBIX_MEDIA_HEADER_VERSION_1)
			s >> orientation_;
		if (version_ >= TX_CUBIX_MEDIA_HEADER_VERSION_2)
			s >> duration_;
		if (version_ >= TX_CUBIX_MEDIA_HEADER_VERSION_3)
			s >> previewType_;
	}

	inline TxMediaHeader::MediaType mediaType() { return (TxMediaHeader::MediaType)mediaType_; }
	inline void setMediaType(unsigned short type) { mediaType_ = type; }

	inline TxMediaHeader::MediaType previewType() { return (TxMediaHeader::MediaType)previewType_; }
	inline void setPreviewType(unsigned short type) { previewType_ = type; }

	inline uint64_t timestamp() { return timestamp_; }
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	inline uint64_t size() { return size_; }
	inline void setSize(uint64_t size) { size_ = size; }

	inline unsigned short orientation() { return orientation_; }
	inline void setOrientation(unsigned short orientation) { orientation_ = orientation; }

	inline unsigned int duration() { return duration_; }
	inline void setDuration(unsigned int duration) { duration_ = duration; }

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
		props["orientation"] = strprintf("%d", orientation_);
		props["duration"] = strprintf("%d", duration_);
		props["name"] = mediaName_;

		std::string lDescription; lDescription.insert(lDescription.end(), description_.begin(), description_.end());
		props["description"] = lDescription;
		props["type"] = mediaTypeToString();
		props["previewType"] = TxMediaHeader::mediaTypeAnyToString((TxMediaHeader::MediaType)previewType_);
		props["signature"] = signature_.toHex();
	}

	inline std::string mediaTypeToString() {
		return TxMediaHeader::mediaTypeAnyToString((TxMediaHeader::MediaType)mediaType_);
	}

	inline static std::string mediaTypeAnyToString(TxMediaHeader::MediaType type) {
		//
		switch(type) {
			case TxMediaHeader::MediaType::UNKNOWN: return "UNKNOWN";
			case TxMediaHeader::MediaType::IMAGE_PNG: return "image/png";
			case TxMediaHeader::MediaType::IMAGE_JPEG: return "image/jpeg";
			case TxMediaHeader::MediaType::VIDEO_MJPEG: return "video/mjpeg";
			case TxMediaHeader::MediaType::VIDEO_MP4: return "video/mp4";
			case TxMediaHeader::MediaType::AUDIO_PCM: return "audio/pcm";
			case TxMediaHeader::MediaType::AUDIO_AMR: return "audio/amr";
			case TxMediaHeader::MediaType::DOCUMENT_PDF: return "application/pdf";
			case TxMediaHeader::MediaType::AUDIO_MP3: return "audio/mp3";
			case TxMediaHeader::MediaType::AUDIO_M4A: return "audio/m4a";
		}

		return "UNKNOWN";
	}

	inline std::string mediaTypeToExtension() {
		return mediaTypeAnyToExtension((TxMediaHeader::MediaType)mediaType_);
	}

	inline std::string mediaTypeAnyToExtension(TxMediaHeader::MediaType type) {
		//
		switch(type) {
		    case TxMediaHeader::MediaType::UNKNOWN: return ".unknown";
		    case TxMediaHeader::MediaType::IMAGE_PNG: return ".png";
		    case TxMediaHeader::MediaType::IMAGE_JPEG: return ".jpeg";
		    case TxMediaHeader::MediaType::VIDEO_MJPEG: return ".mjpeg";
		    case TxMediaHeader::MediaType::VIDEO_MP4: return ".mp4";
		    case TxMediaHeader::MediaType::AUDIO_PCM: return ".wav";
		    case TxMediaHeader::MediaType::AUDIO_AMR: return ".amr";
		    case TxMediaHeader::MediaType::DOCUMENT_PDF: return ".pdf";
		    case TxMediaHeader::MediaType::AUDIO_MP3: return ".mp3";
		    case TxMediaHeader::MediaType::AUDIO_M4A: return ".m4a";
		}

		return "UNKNOWN";
	}

	inline std::string previewMediaTypeToExtension() {
		//
		if (version_ >= TX_CUBIX_MEDIA_HEADER_VERSION_3) {
			if ((TxMediaHeader::MediaType)previewType_ == TxMediaHeader::MediaType::UNKNOWN) return mediaTypeToExtension();
			std::string lPreviewType = mediaTypeAnyToExtension((TxMediaHeader::MediaType)previewType_);
			if (lPreviewType == "UNKNOWN") return mediaTypeToExtension();
			return lPreviewType;
		} else {
			switch(mediaType_) {
				case TxMediaHeader::IMAGE_PNG: return ".png";
				default: return ".jpeg";
			}
		}

		return "UNKNOWN";
	}

	static TxMediaHeader::MediaType extensionToMediaType(const std::string& ext) {
		//
		if (ext == ".jpeg" || ext == ".jpg" || ext == "jpeg" || ext == "jpg") {
			return TxMediaHeader::MediaType::IMAGE_JPEG;
		} else if (ext == ".png" || ext == "png") {
			return TxMediaHeader::MediaType::IMAGE_PNG;
		} else if (ext == ".mjpeg" || ext == "mjpeg") {
			return TxMediaHeader::MediaType::VIDEO_MJPEG;
		} else if (ext == ".mp4" || ext == "mp4") {
			return TxMediaHeader::MediaType::VIDEO_MP4;
		} else if (ext == ".wav" || ext == "wav") {
			return TxMediaHeader::MediaType::AUDIO_PCM;
		} else if (ext == ".amr" || ext == "amr") {
			return TxMediaHeader::MediaType::AUDIO_AMR;
		} else if (ext == ".pdf" || ext == "pdf") {
			return TxMediaHeader::MediaType::DOCUMENT_PDF;
		} else if (ext == ".mp3" || ext == "mp3") {
			return TxMediaHeader::MediaType::AUDIO_MP3;
		} else if (ext == ".m4a" || ext == "m4a") {
			return TxMediaHeader::MediaType::AUDIO_M4A;
		}

		return TxMediaHeader::MediaType::UNKNOWN;
	}

	static void mediaExtensions(std::list<std::string>& list) {
		list.push_back(".png");
		list.push_back(".jpeg");
		list.push_back(".mjpeg");
		list.push_back(".mp4");
		list.push_back(".wav");
		list.push_back(".amr");
		list.push_back(".pdf");
		list.push_back(".mp3");
		list.push_back(".m4a");
	}

	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior

	virtual bool isContinuousData() { return true; } 

	virtual bool isFeeFree() { return true; }

	virtual std::string entityName() { return mediaName_; }

	virtual std::string name() { return "media_header"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}

protected:
	// version 0
	unsigned short mediaType_ = TxMediaHeader::MediaType::UNKNOWN;
	uint64_t timestamp_;
	std::string mediaName_;
	std::vector<unsigned char> description_;
	std::vector<unsigned char> data_;
	uint64_t size_ = 0;
	uint512 signature_;

	// version 1
	unsigned short orientation_ = 0;
	// version 2
	uint32_t duration_ = 0;
	// version 3
	unsigned short previewType_ = TxMediaHeader::MediaType::UNKNOWN;

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
