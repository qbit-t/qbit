// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZ_REBUZZ_H
#define QBIT_TXBUZZ_REBUZZ_H

#include "txbuzz.h"

namespace qbit {

#define TX_REBUZZ_MY_IN 0

//
class TxReBuzz: public TxBuzz {
public:
	TxReBuzz() { type_ = TX_REBUZZ; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << score_;
		s << buzzerInfo_;
		s << buzzerInfoChain_;
		s << body_;
		s << media_;
		s << signature_;

		s << buzzId_;
		s << buzzChainId_;
		s << buzzerId_;
	}
	
	virtual void deserialize(DataStream& s) {
		TxBuzz::deserialize(s);

		s >> buzzId_;
		s >> buzzChainId_;
		s >> buzzerId_;
	}

	std::string name() { return "rebuzz"; }

	inline uint256 buzzId() { return buzzId_; }
	inline void setBuzzId(const uint256& buzzId) { buzzId_ = buzzId; }

	inline uint256 buzzChainId() { return buzzChainId_; }
	inline void setBuzzChainId(const uint256& buzzChainId) { buzzChainId_ = buzzChainId; }

	inline uint256 buzzerId() { return buzzerId_; }
	inline void setBuzzerId(const uint256& buzzerId) { buzzerId_ = buzzerId; }

	virtual In& addBuzzIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	bool simpleRebuzz() {
		return !body_.size() && !media_.size();
	}

	virtual void setBody(const std::string& body, const SKey& skey) {
		//
		std::vector<unsigned char> lBody;
		lBody.insert(lBody.end(), body.begin(), body.end()); 
		setBody(lBody, skey);
	}

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
			lSource << buzzId_;
			lSource << buzzChainId_;
			
			uint256 lHash = Hash(lSource.begin(), lSource.end());
			if (!const_cast<SKey&>(skey).sign(lHash, signature_)) {
				throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
			}

		} else throw qbit::exception("E_SIZE", "Body size is incorrect.");
	}

	inline static bool verifySignature(const PKey& pkey, unsigned short type, uint64_t timestamp, uint64_t score,
		const uint256& buzzerInfo, const std::vector<unsigned char>& body, const std::vector<BuzzerMediaPointer>& media, 
		const uint256& buzzId, const uint256& buzzChainId, const uint512& signature) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << timestamp;
		lSource << score;
		lSource << buzzerInfo;
		lSource << body;
		lSource << media;
		lSource << buzzId;
		lSource << buzzChainId;

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		return const_cast<PKey&>(pkey).verify(lHash, signature);
	}

	virtual void properties(std::map<std::string, std::string>& props) {
		//
		TxBuzz::properties(props);

		props["buzzId"] = strprintf("%s", buzzId_.toHex());
		props["buzzChainId"] = strprintf("%s", buzzChainId_.toHex());
		props["buzzerId"] = strprintf("%s", buzzerId_.toHex());
	}	

protected:
	uint256 buzzId_;
	uint256 buzzChainId_;
	uint256 buzzerId_;
};

typedef std::shared_ptr<TxReBuzz> TxReBuzzPtr;

class TxReBuzzCreator: public TransactionCreator {
public:
	TxReBuzzCreator() {}
	TransactionPtr create() { return std::make_shared<TxReBuzz>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxReBuzzCreator>(); }
};

}

#endif
