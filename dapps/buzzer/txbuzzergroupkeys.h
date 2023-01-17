// Copyright (c) 2019-2022 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_GROUP_KEYS_H
#define QBIT_TXBUZZER_GROUP_KEYS_H

#include "txbuzzer.h"
#include "../../seedwords.h"

namespace qbit {

//
#define TX_BUZZER_GROUP_IN 0
#define TX_BUZZER_GROUP_PREV_KEYS 1

#define TX_BUZZER_GROUP_KEYS_COUNT 16 

//
class TxBuzzerGroupKeys: public TxEvent {
public:
	TxBuzzerGroupKeys() { type_ = TX_BUZZER_GROUP_KEYS; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << encryptedRing_;
	}
	
	inline void deserialize(DataStream& s) {
		s >> timestamp_;
		s >> encryptedRing_;
	}

	virtual std::string entityName() { return Entity::emptyName(); }
	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior 

	inline uint64_t timestamp() { return timestamp_; }
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

	virtual In& addGroupIn(const SKey& skey, UnlinkedOutPtr utxo) {
		return addSignedIn(skey, utxo);
	}

	virtual In& addPreviousKeysIn(const SKey& skey, UnlinkedOutPtr utxo) {
		return addSignedIn(skey, utxo);
	}

	void makeRing(const uint256& nonce) {
		//
		for (int lIdx = 0; lIdx < TX_BUZZER_GROUP_KEYS_COUNT; lIdx++) {
			//
			ring_.push_back(makeKey(nonce));
		}
	}

	void makeRing(const uint256& nonce, const uint256& key0) {
		//
		ring_.clear();
		ring_.push_back(key0);
		//
		for (int lIdx = 1; lIdx < TX_BUZZER_GROUP_KEYS_COUNT; lIdx++) {
			//
			ring_.push_back(makeKey(nonce));
		}
	}

	uint256 key0() { if (ring_.size()) return *ring_.begin(); return uint256(); }
	uint256 keyN(int& index) {
		//
		if (index + 1 < ring_.size()) return ring_[++index];
		index = 1;
		return ring_[index];
	}

	void encryptRing(const uint256& secret) {
		// prepare secret
		unsigned char lMix[AES_BLOCKSIZE] = {0};
		// prepare padding
		uint160 lNonce = Hash160(secret.begin(), secret.end());
		memcpy(lMix, lNonce.begin(), AES_BLOCKSIZE);
		// buffer
		std::vector<unsigned char> lSource;
		// fill-up
		for (int lIdx = 0; lIdx < TX_BUZZER_GROUP_KEYS_COUNT; lIdx++) {
			//
			lSource.insert(lSource.end(), ring_[lIdx].begin(), ring_[lIdx].end());
		}
		// make cypher
		std::vector<unsigned char> lCypher; lCypher.resize(lSource.size() + AES_BLOCKSIZE /*padding*/, 0);
		AES256CBCEncrypt lEncrypt(secret.begin(), lMix, true);
		unsigned lLen = lEncrypt.Encrypt((unsigned char*)&lSource[0], lSource.size(), &lCypher[0]);
		lCypher.resize(lLen);

		// set
		encryptedRing_.clear();
		encryptedRing_.insert(encryptedRing_.end(), lCypher.begin(), lCypher.end());
	}

	void decryptRing(const uint256& secret) {
		// prepare secret
		unsigned char lMix[AES_BLOCKSIZE] = {0};
		// prepare padding
		uint160 lNonce = Hash160(secret.begin(), secret.end());
		memcpy(lMix, lNonce.begin(), AES_BLOCKSIZE);
		// decrypt
		std::vector<unsigned char> lCypher; lCypher.resize(encryptedRing_.size() + 1, 0);
		AES256CBCDecrypt lDecrypt(secret.begin(), lMix, true);
		unsigned lLen = lDecrypt.Decrypt((unsigned char*)&encryptedRing_[0], encryptedRing_.size(), &lCypher[0]);
		lCypher.resize(lLen);

		// set
		for (int lPos = 0; lPos < lCypher.size(); lPos += (256/8)) {
			//
			uint256 lKey((unsigned char*)&lCypher[lPos]);
			ring_.push_back(lKey);
		}
	}

	Transaction::UnlinkedOutPtr addKeysOut(const SKey& skey, const PKey& pkey) {
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
			OP(QEQADDR) 	<<
			OP(QPEN) 		<<
			OP(QPTXO)		<< // use in entity-based pushUnlinkedOut's
			OP(QMOV)		<< REG(QR1) << CU16(TX_BUZZER_GROUP_KEYS) <<
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

	inline std::string name() { return "buzzer_group_keys"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
		props["ring"] = HexStr(encryptedRing_.begin(), encryptedRing_.end());
	}

private:
	inline In& addSignedIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	uint256 makeKey(const uint256& nonce) {
		//
		qbit::SeedPhrase lPhrase;
		std::list<std::string> lWords = lPhrase.generate();
		//
		std::basic_string<unsigned char> lPhraseRaw;
		for(std::list<std::string>::iterator lItem = lWords.begin(); lItem != lWords.end(); lItem++) {
			lPhraseRaw.insert(lPhraseRaw.end(), (*lItem).begin(), (*lItem).end());
		}
		//
		lPhraseRaw.insert(lPhraseRaw.end(), nonce.toHex().begin(), nonce.toHex().end());
		//
		return Hash(lPhraseRaw.begin(), lPhraseRaw.end());
	}

private:
	uint64_t timestamp_;
	std::vector<uint256> ring_; // ring_[0] is allways the same for group info encryption
	std::vector<unsigned char> encryptedRing_;
};

typedef std::shared_ptr<TxBuzzerGroupKeys> TxBuzzerGroupKeysPtr;

class TxBuzzerGroupKeysCreator: public TransactionCreator {
public:
	TxBuzzerGroupKeysCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerGroupKeys>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerGroupKeysCreator>(); }
};

}

#endif
