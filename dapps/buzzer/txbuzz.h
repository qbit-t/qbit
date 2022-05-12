// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZ_H
#define QBIT_TXBUZZ_H

#include "txbuzzer.h"

namespace qbit {

#define TX_BUZZ_BODY_SIZE 1024 

typedef LimitedString<64> buzzer_tag_t;

#define TX_BUZZ_MY_IN 0

#define TX_BUZZ_REPLY_OUT 0
#define TX_BUZZ_LIKE_OUT 1
#define TX_BUZZ_REWARD_OUT 2
#define TX_BUZZ_PIN_OUT 3

//
class TxBuzz: public TxEvent {
public:
	TxBuzz() { type_ = TX_BUZZ; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << score_;
		s << buzzerInfo_;
		s << buzzerInfoChain_;
		s << body_;
		s << media_;
		s << signature_;
	}
	
	virtual void deserialize(DataStream& s) {
		s >> timestamp_;
		s >> score_;
		s >> buzzerInfo_;
		s >> buzzerInfoChain_;
		s >> body_;
		s >> media_;
		s >> signature_;

		if (body_.size() > TX_BUZZ_BODY_SIZE) body_.resize(TX_BUZZ_BODY_SIZE);
	}

	inline const std::vector<unsigned char>& body() const { return body_; }
	inline void body(std::string& body) {
		body.insert(body.end(), body_.begin(), body_.end());		
	}
	inline uint64_t timestamp() { return timestamp_; }

	virtual void setBody(const std::vector<unsigned char>& body, const SKey& skey) {
		if (body.size() <= TX_BUZZ_BODY_SIZE) {
			//
			body_ = body;

			//
			DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
			lSource << timestamp_;
			lSource << score_;
			lSource << buzzerInfo_;
			lSource << body_;
			lSource << media_;
			
			uint256 lHash = Hash(lSource.begin(), lSource.end());
			if (!const_cast<SKey&>(skey).sign(lHash, signature_)) {
				throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
			}

		} else throw qbit::exception("E_SIZE", "Body size is incorrect.");
	}

	inline static bool verifySignature(const PKey& pkey, unsigned short type, uint64_t timestamp, uint64_t score,
			const uint256& buzzerInfo, const std::vector<unsigned char>& body, const std::vector<BuzzerMediaPointer>& media, 
			const uint512& signature) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << timestamp;
		lSource << score;
		lSource << buzzerInfo;
		lSource << body;
		lSource << media;

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		return const_cast<PKey&>(pkey).verify(lHash, signature);
	}

	virtual void setBody(const std::string& body, const SKey& skey) {
		//
		std::vector<unsigned char> lBody;
		lBody.insert(lBody.end(), body.begin(), body.end()); 
		setBody(lBody, skey);
	}
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	inline uint64_t score() { return score_; }
	inline void setScore(uint64_t score) { score_ = score; }

	const uint512& signature() const { return signature_; }
	void setSignature(const uint512& signature) { signature_ = signature; }

	const uint256& buzzerInfo() const { return buzzerInfo_; }
	void setBuzzerInfo(const uint256& buzzerInfo) { buzzerInfo_ = buzzerInfo; }

	const uint256& buzzerInfoChain() const { return buzzerInfoChain_; }
	void setBuzzerInfoChain(const uint256& buzzerInfoChain) { buzzerInfoChain_ = buzzerInfoChain; }

	void addMediaPointer(const BuzzerMediaPointer& pointer) {
		media_.push_back(pointer);
	}

	void setMediaPointers(const std::vector<BuzzerMediaPointer>& pointers) {
		media_.clear();
		media_.insert(media_.end(), pointers.begin(), pointers.end());
	}

	const std::vector<BuzzerMediaPointer>& mediaPointers() { return media_; } 

	virtual void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
		props["score"] = strprintf("%d", score_);
		props["buzzer_info"] = buzzerInfo_.toHex();
		props["buzzer_info_chain"] = buzzerInfoChain_.toHex();

		if (body_.size()) {
			std::string lBody; lBody.insert(lBody.end(), body_.begin(), body_.end());
			props["body"] = lBody;
		}

		props["signature"] = strprintf("%s", signature_.toHex());

		int lCount = 0;
		for (std::vector<BuzzerMediaPointer>::iterator lMedia = media_.begin(); lMedia != media_.end(); lMedia++, lCount++) {
			//
			props[strprintf("media_%d", lCount)] = strprintf("%s/%s", lMedia->tx().toHex(), lMedia->chain().toHex());
		}
	}

	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior 

	inline void extractTags(std::map<uint160, std::string>& tags) {
		// extract #tags
		unsigned char lChar;
		std::vector<unsigned char>::iterator lIt = body_.begin();
		while(lIt != body_.end() && (lIt = std::find(lIt, body_.end(), '#')) != body_.end()) {
			std::vector<unsigned char> lTag;
			std::string lStringTag;
			do {
				lTag.push_back(::tolower(*lIt)); 
				lStringTag.push_back(*lIt); lIt++;
			} while(lIt != body_.end() && 
						(
							(*lIt >= 'a' && *lIt <= 'z') || 
							(*lIt >= 'A' && *lIt <= 'Z') ||
							(*lIt >= '0' && *lIt <= '9') ||
							(*lIt == 0x04 && (lIt+1) != body_.end() && *(lIt+1) >= 0x00 && *(lIt+1) <= 0xFF) // fast-russian
						));

			tags[Hash160(lTag.begin(), lTag.end())] = lStringTag;
		}
	}

	//
	Transaction::UnlinkedOutPtr addReBuzzOut(const SKey& skey, const PKey& pkey) {
		//
		return Transaction::UnlinkedOutPtr(); // this out is not needed
	}

	Transaction::UnlinkedOutPtr addBuzzLikeOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzSpecialOut(skey, pkey, TX_BUZZ_LIKE);
	}

	Transaction::UnlinkedOutPtr addBuzzReplyOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzSpecialOut(skey, pkey, TX_BUZZ_REPLY);
	}

	Transaction::UnlinkedOutPtr addBuzzRewardOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzSpecialOut(skey, pkey, TX_BUZZ_REWARD);
	}

	Transaction::UnlinkedOutPtr addBuzzPinOut(const SKey& skey, const PKey& pkey) {
		//
		// TODO: implement buzz_pin transaction
		/*
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
			OP(QEQADDR) 	<<
			OP(QPEN) 		<<
			OP(QPTXO)		<< // use in entity-based pushUnlinkedOut's
			OP(QMOV)		<< REG(QR1) << CU16(TX_BUZZ_PIN) <<
			OP(QCMPE)		<< REG(QTH1) << REG(QR1) <<
			OP(QMOV) 		<< REG(QR0) << REG(QC0) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(chain(), TxAssetType::nullAsset(), out_.size()), // link
			pkey
		);

		out_.push_back(lOut);
		return lUTXO;
		*/

		return nullptr;
	}

	Transaction::UnlinkedOutPtr addBuzzHideOut(const SKey& skey, const PKey& pkey) {
		//
		// TODO: implement hide_buzz transaction
		/*
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
			OP(QEQADDR) 	<<
			OP(QPEN) 		<<
			OP(QPTXO)		<< // use in entity-based pushUnlinkedOut's
			OP(QMOV)		<< REG(QR1) << CU16(TX_BUZZ_PIN) <<
			OP(QCMPE)		<< REG(QTH1) << REG(QR1) <<
			OP(QMOV) 		<< REG(QR0) << REG(QC0) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(chain(), TxAssetType::nullAsset(), out_.size()), // link
			pkey
		);

		out_.push_back(lOut);
		return lUTXO;
		*/

		return nullptr;
	}

	//
	// in[0] - mybuzzer.out[TX_BUZZER_MY_OUT] - my buzzer with signature
	virtual In& addMyBuzzerIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	//
	// in[1] - @buzzer.out[TX_BUZZER_BUZZ_OUT] or @buzzer.out[TX_BUZZER_REPLY_OUT]
	// in[2] - @buzzer.out[TX_BUZZER_BUZZ_OUT] or @buzzer.out[TX_BUZZER_REPLY_OUT]	
	virtual In& addBuzzerIn(const SKey& skey, UnlinkedOutPtr utxo) {
		Transaction::In lIn;
		lIn.out().setNull();
		lIn.out().setChain(utxo->out().chain());
		lIn.out().setAsset(utxo->out().asset());
		lIn.out().setTx(utxo->out().tx());
		lIn.out().setIndex(utxo->out().index());

		lIn.setOwnership(ByteCode() <<
			OP(QDETXO)); // entity/check/push

		in_.push_back(lIn);
		return in_[in_.size()-1];
	}

	virtual std::string name() { return "buzz"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}

protected:
	//
	Transaction::UnlinkedOutPtr addBuzzSpecialOut(const SKey& skey, const PKey& pkey, unsigned short specialOut) {
		//
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QPTXO)		<< // use in entity-based pushUnlinkedOut's
			OP(QMOV)		<< REG(QR1) << CU16(specialOut) <<
			OP(QCMPE)		<< REG(QTH1) << REG(QR1) <<
			OP(QMOV) 		<< REG(QR0) << REG(QC0) <<	
			OP(QRET));

		Transaction::UnlinkedOutPtr lUTXO = Transaction::UnlinkedOut::instance(
			Transaction::Link(MainChain::id(), TxAssetType::nullAsset(), out_.size()), // link
			pkey
		);

		out_.push_back(lOut);
		return lUTXO;
	}	

protected:
	uint64_t timestamp_;
	uint64_t score_;
	uint256 buzzerInfo_;
	uint256 buzzerInfoChain_;
	std::vector<unsigned char> body_;
	std::vector<BuzzerMediaPointer> media_;
	uint512 signature_;

	// non-persistent
	std::list<std::string> tags_;
	std::list<std::string> buzzers_;
};

typedef std::shared_ptr<TxBuzz> TxBuzzPtr;

class TxBuzzCreator: public TransactionCreator {
public:
	TxBuzzCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzz>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzCreator>(); }
};

}

#endif
