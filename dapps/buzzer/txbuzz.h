// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZ_H
#define QBIT_TXBUZZ_H

#include "txbuzzer.h"

namespace qbit {

#define TX_BUZZ_BODY_SIZE 512 

typedef LimitedString<64> buzzer_tag_t;

#define TX_BUZZ_REPLY_OUT 0
#define TX_BUZZ_REBUZZ_OUT 1
#define TX_BUZZ_LIKE_OUT 2
#define TX_BUZZ_PIN_OUT 3

//
class TxBuzz: public Entity {
public:
	TxBuzz() { type_ = TX_BUZZ; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << body_;
	}
	
	inline void deserialize(DataStream& s) {
		s >> timestamp_;
		s >> body_;

		if (body_.size() > TX_BUZZ_BODY_SIZE) body_.resize(TX_BUZZ_BODY_SIZE);
	}

	inline std::vector<unsigned char>& body() { return body_; }
	inline void body(std::string& body) {
		body.insert(body.end(), body_.begin(), body_.end());		
	}
	inline uint64_t timestamp() { return timestamp_; }

	inline void setBody(const std::vector<unsigned char>& body) {
		if (body.size() <= TX_BUZZ_BODY_SIZE) 
			body_ = body;
		else throw qbit::exception("E_SIZE", "Body size is incorrect.");
	}
	inline void setBody(const std::string& body) {
		if (body.size() <= TX_BUZZ_BODY_SIZE) 
			body_.insert(body_.end(), body.begin(), body.end()); 
		else throw qbit::exception("E_SIZE", "Body size is incorrect.");
	}
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);

		std::string lBody; lBody.insert(lBody.end(), body_.begin(), body_.end());
		props["body"] = lBody;
	}

	virtual std::string entityName() { return Entity::emptyName(); }
	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior 

	inline void parseBody() {
		// TODO:
		// 1. extract #tags
		// 2. extract @buzzers
	}

	//
	Transaction::UnlinkedOutPtr addReBuzzOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzSpecialOut(skey, pkey, TX_REBUZZ);
	}

	Transaction::UnlinkedOutPtr addBuzzLikeOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzSpecialOut(skey, pkey, TX_BUZZ_LIKE);
	}

	Transaction::UnlinkedOutPtr addBuzzReplyOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzSpecialOut(skey, pkey, TX_BUZZ_REPLY);
	}

	Transaction::UnlinkedOutPtr addBuzzPinOut(const SKey& skey, const PKey& pkey) {
		//
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
	}

	virtual In& addBuzzerIn(const SKey& skey, UnlinkedOutPtr utxo) {
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
			OP(QDETXO)); // entity/check/push

		in_.push_back(lIn);
		return in_[in_.size()-1];
	}

	inline std::string name() { return "buzz"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}

private:
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

private:
	uint64_t timestamp_;
	std::vector<unsigned char> body_;

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
