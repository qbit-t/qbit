// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_MESSAGE_H
#define QBIT_MESSAGE_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include "streams.h"
#include "tinyformat.h"
#include "uint256.h"

#define QBIT_MESSAGE_0 0x61
#define QBIT_MESSAGE_1 0x31
#define QBIT_MESSAGE_2 0x32
#define QBIT_MESSAGE_3 0x36

#define QBIT_TEST_MESSAGE_0 0x62
#define QBIT_TEST_MESSAGE_1 0x32
#define QBIT_TEST_MESSAGE_2 0x33
#define QBIT_TEST_MESSAGE_3 0x36

#define QBIT_MESSAGE_ENCRYPTED 0x8000
#define QBIT_MESSAGE_TYPE 0x7fff

namespace qbit {

extern bool gTestNet;
extern bool gSparingMode;

class Message {
public:
	enum Type {
		KEY_EXCHANGE = 0x0000,
		PING = 0x0001,
		PONG = 0x0002,
		STATE = 0x0003,
		GLOBAL_STATE = 0x0004,
		BLOCK = 0x0005,
		TRANSACTION = 0x0006,
		BLOCK_HEADER = 0x0007,
		BLOCK_HEADER_AND_STATE = 0x0008,
		TRANSACTION_DATA = 0x0009,
		TRANSACTIONS_DATA = 0x000a,
		
		GET_BLOCK_HEADER = 0x0010,
		GET_BLOCK_DATA = 0x0011,
		GET_BLOCK_BY_HEIGHT = 0x0012,
		GET_BLOCK_BY_ID = 0x0013,
		GET_NETWORK_BLOCK = 0x0014,
		GET_NETWORK_BLOCK_HEADER = 0x0015,
		GET_TRANSACTION_DATA = 0x0016,
		GET_TRANSACTIONS_DATA = 0x0017,
		PUSH_TRANSACTION = 0x0018,
		TRANSACTION_PUSHED = 0x0019,

		GET_SOME_QBITS = 0x001a,

		BLOCK_BY_HEIGHT = 0x0020,
		BLOCK_BY_ID = 0x0021,
		NETWORK_BLOCK = 0x0022,
		NETWORK_BLOCK_HEADER = 0x0023,

		BLOCK_BY_HEIGHT_IS_ABSENT = 0x0024,
		BLOCK_BY_ID_IS_ABSENT = 0x0025,
		NETWORK_BLOCK_IS_ABSENT = 0x0026,
		BLOCK_HEADER_IS_ABSENT = 0x0027,
		BLOCK_IS_ABSENT = 0x0028,
		NETWORK_BLOCK_HEADER_IS_ABSENT = 0x0029,
		TRANSACTION_IS_ABSENT = 0x002a,
		ENTITY_IS_ABSENT = 0x002b,

		GET_PEERS = 0x0040,
		PEER_EXISTS = 0x0041,
		PEER_BANNED = 0x0042,
		PEERS = 0x0043,

		CLIENT_SESSIONS_EXCEEDED = 0x004a,

		GET_UTXO_BY_ADDRESS = 0x0050,
		GET_UTXO_BY_ADDRESS_AND_ASSET = 0x0051,
		GET_UTXO_BY_TX = 0x0052,
		GET_UTXO_BY_ENTITY = 0x0053,
		GET_UTXO_BY_ENTITY_NAMES = 0x0054,
		GET_ENTITY_COUNT_BY_SHARDS = 0x055,
		GET_ENTITY = 0x0056,
		GET_STATE = 0x0057,
		GET_ENTITY_COUNT_BY_DAPP = 0x058,
		GET_ENTITY_NAMES = 0x059,

		UTXO_BY_ADDRESS = 0x0060,
		UTXO_BY_ADDRESS_AND_ASSET = 0x0061,
		UTXO_BY_TX = 0x0062,
		UTXO_BY_ENTITY = 0x0063,
		UTXO_BY_ENTITY_NAMES = 0x0064,
		ENTITY_COUNT_BY_SHARDS = 0x065,
		ENTITY = 0x0066,
		ENTITY_COUNT_BY_DAPP = 0x0067,
		ENTITY_NAMES = 0x0068,

		CUSTOM_0000 = 0x1000,	// buzzer
		CUSTOM_0001 = 0x1001,	//
		CUSTOM_0002 = 0x1002,	//
		CUSTOM_0003 = 0x1003,	//
		CUSTOM_0004 = 0x1004,	//
		CUSTOM_0005 = 0x1005,	//
		CUSTOM_0006 = 0x1006,	//
		CUSTOM_0007 = 0x1007,	//
		CUSTOM_0008 = 0x1008,	//
		CUSTOM_0009 = 0x1009,	//
		CUSTOM_0010 = 0x100a,	//
		CUSTOM_0011 = 0x100b,	//
		CUSTOM_0012 = 0x100c,	//
		CUSTOM_0013 = 0x100d,	//
		CUSTOM_0014 = 0x100e,	//
		CUSTOM_0015 = 0x100f,	//
		CUSTOM_0016 = 0x1010,	//
		CUSTOM_0017 = 0x1011,	//
		CUSTOM_0018 = 0x1012,	//
		CUSTOM_0019 = 0x1013,	//
		CUSTOM_0020 = 0x1014,	//
		CUSTOM_0021 = 0x1015,	//
		CUSTOM_0022 = 0x1016,	//
		CUSTOM_0023 = 0x1017,	//
		CUSTOM_0024 = 0x1018,	//
		CUSTOM_0025 = 0x1019,	//
		CUSTOM_0026 = 0x101a,	//
		CUSTOM_0027 = 0x101b,	//
		CUSTOM_0028 = 0x101c,	//
		CUSTOM_0029 = 0x101d,	//
		CUSTOM_0030 = 0x101e,	//
		CUSTOM_0031 = 0x101f,	//
		CUSTOM_0032 = 0x1020,	//
		CUSTOM_0033 = 0x1021,	//
		CUSTOM_0034 = 0x1022,	//
		CUSTOM_0035 = 0x1023,	//
		CUSTOM_0036 = 0x1024,	//
		CUSTOM_0037 = 0x1025,	//
		CUSTOM_0038 = 0x1026,	//
		CUSTOM_0039 = 0x1027,	//
		CUSTOM_0040 = 0x1028,	//
		CUSTOM_0041 = 0x1029,	//
		CUSTOM_0042 = 0x102a,	//
		CUSTOM_0043 = 0x102b,	//
		CUSTOM_0044 = 0x102c,	//
		CUSTOM_0045 = 0x102d,	//
		CUSTOM_0046 = 0x102e,	//
		CUSTOM_0047 = 0x102f,	//
		CUSTOM_0048 = 0x1030,	//
		CUSTOM_0049 = 0x1031,	//
		CUSTOM_0050 = 0x1032,	//
		CUSTOM_0051 = 0x1033,	//
		CUSTOM_0052 = 0x1034,	//
		CUSTOM_0053 = 0x1035,	//
		CUSTOM_0054 = 0x1036,	//
		CUSTOM_0055 = 0x1037,	//
		CUSTOM_0056 = 0x1038,	//
		CUSTOM_0057 = 0x1039,	//
		CUSTOM_0058 = 0x103a,	//
		CUSTOM_0059 = 0x103b,	//
		CUSTOM_0060 = 0x103c,	//
		CUSTOM_0061 = 0x103d,	//
		CUSTOM_0062 = 0x103e,	//
		CUSTOM_0063 = 0x103f,	//

		CUSTOM_0064 = 0x1042,	// cubix
		CUSTOM_0065 = 0x1043,	//
		CUSTOM_0066 = 0x1044,	//
		CUSTOM_0067 = 0x1045,	//
		CUSTOM_0068 = 0x1046,	//
		CUSTOM_0069 = 0x1047,	//
		CUSTOM_0070 = 0x1048,	//
		CUSTOM_0071 = 0x1049,	//
		CUSTOM_0072 = 0x104a,	//
		CUSTOM_0073 = 0x104b,	//
		CUSTOM_0074 = 0x104c,	//
		CUSTOM_0075 = 0x104d,	//
		CUSTOM_0076 = 0x104e,	//
		CUSTOM_0077 = 0x104f	//		
	};

public:
	class CustomType {
	public:
		CustomType(Type type, std::string name): type_(type), name_(name) {}

		inline Type type() { return type_; }
		inline std::string name() { return name_; }

	private:
		Type type_;
		std::string name_;
	};

public:
	Message() {}
	Message(bool enc, Message::Type type, uint32_t size, const uint160& checksum) {
		if (!gTestNet) { prolog_[0] = QBIT_MESSAGE_0; prolog_[1] = QBIT_MESSAGE_1; prolog_[2] = QBIT_MESSAGE_2; prolog_[3] = QBIT_MESSAGE_3; }
		else { prolog_[0] = QBIT_TEST_MESSAGE_0; prolog_[1] = QBIT_TEST_MESSAGE_1; prolog_[2] = QBIT_TEST_MESSAGE_2; prolog_[3] = QBIT_TEST_MESSAGE_3; }
		version_ = QBIT_VERSION;
		type_ = type;
		size_ = size;
		if (size_ > sizeof(uint160)) checksum_ = checksum;

		if (enc) type_ |= QBIT_MESSAGE_ENCRYPTED;
	}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(prolog_);
		READWRITE(version_);
		READWRITE(type_);
		READWRITE(size_);
		READWRITE(checksum_);
	}

	bool valid() { 
		if (!gTestNet)
			return (prolog_[0] == QBIT_MESSAGE_0 && prolog_[1] == QBIT_MESSAGE_1 && prolog_[2] == QBIT_MESSAGE_2 && prolog_[3] == QBIT_MESSAGE_3) &&
				UNPACK_MAJOR(version_) == QBIT_VERSION_MAJOR && UNPACK_MINOR(version_) == QBIT_VERSION_MINOR &&
				UNPACK_REVISION(version_) == QBIT_VERSION_REVISION;
		return (prolog_[0] == QBIT_TEST_MESSAGE_0 && prolog_[1] == QBIT_TEST_MESSAGE_1 && prolog_[2] == QBIT_TEST_MESSAGE_2 && prolog_[3] == QBIT_TEST_MESSAGE_3) &&
			UNPACK_MAJOR(version_) == QBIT_VERSION_MAJOR && UNPACK_MINOR(version_) == QBIT_VERSION_MINOR &&
			UNPACK_REVISION(version_) == QBIT_VERSION_REVISION;
	}

	Message::Type type() { if (encrypted()) return (Message::Type)(type_ & QBIT_MESSAGE_TYPE); else return (Message::Type)type_; }
	uint32_t dataSize() { return size_; }
	uint160 checkSum() { return checksum_; }
	inline bool encrypted() { return (type_ & QBIT_MESSAGE_ENCRYPTED) != 0; }

	static size_t size() { return sizeof(prolog_) + sizeof(version_) + sizeof(type_) + sizeof(size_) + (sizeof(uint8_t) * 160/8); }
	static void registerMessageType(Message::Type type, const std::string& name);

	std::string toString();

private:
	char prolog_[4] = {0};	// 4
	uint32_t version_ = 0; 	// 4
	unsigned short type_;	// 2
	uint32_t size_;			// 4
	uint160 checksum_;		// 20
};

//
typedef std::map<Message::Type /*type*/, std::string /*description*/> MessageTypes;
extern MessageTypes gMessageTypes; // TODO: init on startup

} // qbit

#endif