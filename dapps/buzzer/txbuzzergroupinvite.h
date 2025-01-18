// Copyright (c) 2019-2024 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_TXBUZZER_GROUP_INVITE_H
#define QBIT_TXBUZZER_GROUP_INVITE_H

#include "txbuzzer.h"

#define TX_BUZZER_GROUP_INVITE_MY_IN 0
#define TX_BUZZER_GROUP_INVITE_GROUP_IN 1
#define TX_BUZZER_GROUP_INVITE_MEMBER_IN 2

#define TX_BUZZER_GROUP_INVITE_ACCEPT_OUT 0
#define TX_BUZZER_GROUP_INVITE_DECLINE_OUT 1

namespace qbit {

//
class TxBuzzerGroupInvite: public TxEvent {
public:
	TxBuzzerGroupInvite() { type_ = TX_BUZZER_GROUP_INVITE; }

	ADD_INHERITABLE_SERIALIZE_METHODS;

	template<typename Stream> void serialize(Stream& s) {
		s << timestamp_;
		s << member_;
		s << signature_;
	}
	
	inline void deserialize(DataStream& s) {
		s >> timestamp_;
		s >> member_;
		s >> signature_;
	}

	virtual std::string entityName() { return Entity::emptyName(); }
	virtual inline void setChain(const uint256& chain) { chain_ = chain; } // override default entity behavior 
	inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }
	inline uint64_t timestamp() { return timestamp_; }

	void setMemberAddress(const PKey& pkey) {
		//
		member_.set<const unsigned char*>(pkey.begin(), pkey.end());
	}

	void inviterAddress(PKey& pkey) {
		extractAddress(pkey, TX_BUZZER_GROUP_INVITE_MY_IN);
	}

	void groupAddress(PKey& pkey) {
		extractAddress(pkey, TX_BUZZER_GROUP_INVITE_GROUP_IN);
	}

	void memberAddress(PKey& pkey) {
		pkey.set<const unsigned char*>(member_.begin(), member_.end());
	}

	/*
	const uint256& group() const { return group_; }
	void setGroup(const uint256& group) { group_ = group; }
	*/
	/*
	const uint256& member() const { return member_; }
	void setMember(const uint256& member) { member_ = member; }
	*/

	const uint512& signature() const { return signature_; }
	void setSignature(const uint512& signature) { signature_ = signature; }

	virtual bool isFeeFree() { return false; }

	inline void makeSignature(const SKey& skey, const uint256& group, const uint256& inviter, const uint256& member) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << group;
		lSource << inviter;
		lSource << member;

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		if (!const_cast<SKey&>(skey).sign(lHash, signature_)) {
			throw qbit::exception("INVALID_SIGNATURE", "Signature creation failed.");
		}
	}

	inline static bool verifySignature(const PKey& pkey, uint64_t timestamp,
		const uint256& group, const uint256& inviter, const uint256& member, const uint512& signature) {
		//
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << group;
		lSource << inviter;
		lSource << member;

		uint256 lHash = Hash(lSource.begin(), lSource.end());
		return const_cast<PKey&>(pkey).verify(lHash, signature);
	}

	//
	Transaction::UnlinkedOutPtr addAcceptOut(const SKey& skey, const PKey& pkey) {
		//
		return addDirectSpecialOut(skey, pkey, TX_BUZZER_GROUP_INVITE_ACCEPT_OUT);
	}

	//
	Transaction::UnlinkedOutPtr addDeclineOut(const SKey& skey, const PKey& pkey) {
		//
		return addDirectSpecialOut(skey, pkey, TX_BUZZER_GROUP_INVITE_DECLINE_OUT);
	}

	// in[2] - member
	virtual In& addMemberIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	// in[0] - buzzer
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

	// in[1] - group
	virtual In& addGroupIn(const SKey& skey, UnlinkedOutPtr utxo) {
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

	inline std::string name() { return "buzzer_group_invite"; }

	bool isValue(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() != TxAssetType::nullAsset()); 
	}

	bool isEntity(UnlinkedOutPtr utxo) {
		return (utxo->out().asset() == TxAssetType::nullAsset()); 
	}	

	inline void properties(std::map<std::string, std::string>& props) {
		//
		props["timestamp"] = strprintf("%d", timestamp_);
		props["signature"] = strprintf("%s", signature_.toHex());

		PKey lPKey;
		extractAddress(lPKey, TX_BUZZER_GROUP_INVITE_GROUP_IN);

		props["group"] = lPKey.toString();
		props["member"] = member_.toString();
	}

protected:
	//
	Transaction::UnlinkedOutPtr addDirectSpecialOut(const SKey& skey, const PKey& pkey, unsigned short specialOut) {
		//
		Transaction::Out lOut;
		lOut.setAsset(TxAssetType::nullAsset());
		lOut.setDestination(ByteCode() <<
			OP(QMOV) 		<< REG(QD0) << CVAR(const_cast<PKey&>(pkey).get()) << 
			OP(QEQADDR) 	<<
			OP(QPEN) 		<<
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

	inline bool extractAddress(PKey& pkey, size_t in) {
		//
		Transaction::In& lIn = *(in()[in]);
		VirtualMachine lVM(lIn.ownership());
		lVM.execute();

		if (lVM.getR(qasm::QS0).getType() != qasm::QNONE) {		
			pkey.set<unsigned char*>(lVM.getR(qasm::QS0).begin(), lVM.getR(qasm::QS0).end());

			return true;
		}

		return false;
	}	

protected:
	uint64_t timestamp_;
	uint512 signature_;
	PKey member_;
};

typedef std::shared_ptr<TxBuzzerGroupInvite> TxBuzzerGroupInvitePtr;

class TxBuzzerGroupInviteCreator: public TransactionCreator {
public:
	TxBuzzerGroupInviteCreator() {}
	TransactionPtr create() { return std::make_shared<TxBuzzerGroupInvite>(); }

	static TransactionCreatorPtr instance() { return std::make_shared<TxBuzzerGroupInviteCreator>(); }
};

}

#endif
