// Copyright (c) 2019-2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZ_REWARD_H
#define QBIT_TXBUZZ_REWARD_H

#include "txbuzzer.h"

namespace qbit {

#define TX_BUZZ_REWARD_IN 1

//
class TxBuzzReward: public TxEvent {
public:
	TxBuzzReward() { type_ = TX_BUZZ_REWARD; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << score_;
		s << amount_;
		s << rewardTx_;
		s << buzzerInfo_;
		s << buzzerInfoChain_;
		s << signature_;
	}
	
	inline void deserialize(DataStream& s) {
		s >> timestamp_;
		s >> score_;
		s >> amount_;
		s >> rewardTx_;
		s >> buzzerInfo_;
		s >> buzzerInfoChain_;
		s >> signature_;
	}

	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior 
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }
	inline uint64_t timestamp() { return timestamp_; }

	inline uint64_t score() { return score_; }
	inline void setScore(uint64_t score) { score_ = score; }

	inline uint64_t amount() { return amount_; }
	inline void setAmount(uint64_t amount) { amount_ = amount; }

	const uint256& rewardTx() const { return rewardTx_; }
	void setRewardTx(const uint256& rewardTx) { rewardTx_ = rewardTx; }

	const uint256& buzzerInfo() const { return buzzerInfo_; }
	void setBuzzerInfo(const uint256& buzzerInfo) { buzzerInfo_ = buzzerInfo; }

	const uint256& buzzerInfoChain() const { return buzzerInfoChain_; }
	void setBuzzerInfoChain(const uint256& buzzerInfoChain) { buzzerInfoChain_ = buzzerInfoChain; }

	const uint512& signature() const { return signature_; }
	void setSignature(const uint512& signature) { signature_ = signature; }

	virtual bool isFeeFree() { return true; }

	inline void makeSignature(const SKey& skey, const uint256& buzz) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << timestamp_;
		lSource << score_;
		lSource << amount_;
		lSource << buzzerInfo_;
		lSource << buzz;
		
		uint256 lHash = Hash(lSource.begin(), lSource.end());
		if (!const_cast<SKey&>(skey).sign(lHash, signature_)) {
			throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
		}
	}

	inline static bool verifySignature(const PKey& pkey, unsigned short type, uint64_t timestamp, uint64_t score,
		uint64_t amount, const uint256& buzzerInfo, const uint256& buzz, const uint512& signature) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << timestamp;
		lSource << score;
		lSource << amount;
		lSource << buzzerInfo;
		lSource << buzz;

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		return const_cast<PKey&>(pkey).verify(lHash, signature);
	}	

	virtual In& addBuzzRewardIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	inline std::string name() { return "buzz_reward"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
		props["score"] = strprintf("%d", score_);
		props["amount"] = strprintf("%d", amount_);
		props["reward_tx"] = strprintf("%s", rewardTx_.toHex());
		props["buzzer_info"] = buzzerInfo_.toHex();
		props["buzzer_info_chain"] = buzzerInfoChain_.toHex();
		props["signature"] = strprintf("%s", signature_.toHex());
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

	inline bool extractAmount(TransactionPtr reward, amount_t& amount) {
		//
		Transaction::Out& lOut = reward->out()[0]; // first out is amount that was sent
		VirtualMachine lVM(lOut.destination());
		lVM.execute();

		if (lVM.getR(qasm::QA0).getType() != qasm::QNONE) {
			amount = lVM.getR(qasm::QA0).to<amount_t>();
			return true;
		}

		return false;
	}

private:
	uint64_t timestamp_;
	uint64_t score_;
	uint64_t amount_;
	uint256 rewardTx_;
	uint256 buzzerInfo_;
	uint256 buzzerInfoChain_;
	uint512 signature_;
};

typedef std::shared_ptr<TxBuzzReward> TxBuzzRewardPtr;

class TxBuzzRewardCreator: public TransactionCreator {
public:
	TxBuzzRewardCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzReward>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzRewardCreator>(); }
};

}

#endif
