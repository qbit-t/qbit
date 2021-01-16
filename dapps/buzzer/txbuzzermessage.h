// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_MESSAGE_H
#define QBIT_TXBUZZER_MESSAGE_H

#include "txbuzz.h"

#include "../../crypto/aes.h"
#include "../../crypto/sha256.h"

namespace qbit {

#define TX_BUZZER_MY_IN 0
#define TX_BUZZER_CONVERSATION_IN 1

#define TX_BUZZER_MESSAGE_REPLY_OUT 0

//
class TxBuzzerMessage: public TxBuzz {
public:
	TxBuzzerMessage() { type_ = TX_BUZZER_MESSAGE; }

	std::string name() { return "buzzer_message"; }

	virtual In& addConversationIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	Transaction::UnlinkedOutPtr addMessageReplyOut(const SKey& skey, const PKey& pkey) {
		//
		return addBuzzSpecialOut(skey, pkey, TX_BUZZER_MESSAGE_REPLY);
	}

	virtual void setBody(const std::vector<unsigned char>& body, const SKey& skey, const PKey& counterparty) {
		if (body.size() <= TX_BUZZ_BODY_SIZE) {
			//
			uint256 lNonce = const_cast<SKey&>(skey).shared(counterparty);
			encrypt(lNonce, body);

			//
			DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
			lSource << timestamp_;
			lSource << body_;
			lSource << media_;
			
			uint256 lHash = Hash(lSource.begin(), lSource.end());
			if (!const_cast<SKey&>(skey).sign(lHash, signature_)) {
				throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
			}

		} else throw qbit::exception("E_SIZE", "Body size is incorrect.");
	}

	inline static bool verifySignature(const PKey& pkey, uint64_t timestamp, 
			const std::vector<unsigned char>& body, const std::vector<BuzzerMediaPointer>& media, 
			const uint512& signature) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << timestamp;
		lSource << body;
		lSource << media;

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		return const_cast<PKey&>(pkey).verify(lHash, signature);
	}

	virtual void setBody(const std::string& body, const SKey& skey, const PKey& counterparty) {
		//
		std::vector<unsigned char> lBody;
		lBody.insert(lBody.end(), body.begin(), body.end()); 
		setBody(lBody, skey, counterparty);
	}	

	virtual void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
		props["score"] = strprintf("%d", score_);
		props["buzzer_info"] = buzzerInfo_.toHex();
		props["buzzer_info_chain"] = buzzerInfoChain_.toHex();

		if (body_.size()) {
			props["body"] = HexStr(body_.begin(), body_.end());
		}

		props["signature"] = strprintf("%s", signature_.toHex());

		int lCount = 0;
		for (std::vector<BuzzerMediaPointer>::iterator lMedia = media_.begin(); lMedia != media_.end(); lMedia++, lCount++) {
			//
			props[strprintf("media_%d", lCount)] = strprintf("%s/%s", lMedia->tx().toHex(), lMedia->chain().toHex());
		}
	}

	static bool decrypt(const uint256& nonce, const std::vector<unsigned char>& cypher, std::vector<unsigned char>& body) {
		// prepare secret
		unsigned char lMix[AES_BLOCKSIZE] = {0};
		memcpy(lMix, nonce.begin() + 8, AES_BLOCKSIZE);

		// decrypt
		body.resize(cypher.size() + 1, 0);
		AES256CBCDecrypt lDecrypt(nonce.begin(), lMix, true);
		unsigned lLen = lDecrypt.Decrypt(&cypher[0], cypher.size(), &body[0]);
		body.resize(lLen);

		return lLen > 0;
	}	

private:
	void encrypt(const uint256& nonce, const std::vector<unsigned char>& body) {
		// prepare secret
		unsigned char lMix[AES_BLOCKSIZE] = {0};
		memcpy(lMix, nonce.begin() + 8 /*shift*/, AES_BLOCKSIZE);

		// make cypher
		std::vector<unsigned char> lCypher; lCypher.resize(body.size() + AES_BLOCKSIZE /*padding*/, 0);
		AES256CBCEncrypt lEncrypt(nonce.begin(), lMix, true);
		unsigned lLen = lEncrypt.Encrypt(&body[0], body.size(), &lCypher[0]);
		lCypher.resize(lLen);

		// set
		body_.clear();
		body_.insert(body_.end(), lCypher.begin(), lCypher.end());
	}
};

typedef std::shared_ptr<TxBuzzerMessage> TxBuzzerMessagePtr;

class TxBuzzerMessageCreator: public TransactionCreator {
public:
	TxBuzzerMessageCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerMessage>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerMessageCreator>(); }
};

}

#endif
