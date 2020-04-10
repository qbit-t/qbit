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

#define QBIT_MESSAGE_0 0x1a
#define QBIT_MESSAGE_1 0x1b
#define QBIT_MESSAGE_2 0x1c
#define QBIT_MESSAGE_3 0x1d

namespace qbit {

class Message {
public:
	enum Type {
		PING = 0x0001,
		PONG = 0x0002,
		STATE = 0x0003,
		GLOBAL_STATE = 0x0004,
		BLOCK = 0x0005,
		TRANSACTION = 0x0006,
		BLOCK_HEADER = 0x0007,
		BLOCK_HEADER_AND_STATE = 0x0008,
		TRANSACTION_DATA = 0x0009,
		
		GET_BLOCK_HEADER = 0x0010,
		GET_BLOCK_DATA = 0x0011,
		GET_BLOCK_BY_HEIGHT = 0x0012,
		GET_BLOCK_BY_ID = 0x0013,
		GET_NETWORK_BLOCK = 0x0014,
		GET_NETWORK_BLOCK_HEADER = 0x0015,
		GET_TRANSACTION_DATA = 0x0016,
		PUSH_TRANSACTION = 0x0017,
		TRANSACTION_PUSHED = 0x0018,

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

		UTXO_BY_ADDRESS = 0x0060,
		UTXO_BY_ADDRESS_AND_ASSET = 0x0061,
		UTXO_BY_TX = 0x0062,
		UTXO_BY_ENTITY = 0x0063,
		UTXO_BY_ENTITY_NAMES = 0x0064,
		ENTITY_COUNT_BY_SHARDS = 0x065,
		ENTITY = 0x0066,

		CUSTOM_0000 = 0x1000,
		CUSTOM_0001 = 0x1001,
		CUSTOM_0002 = 0x1002,
		CUSTOM_0003 = 0x1003,
		CUSTOM_0004 = 0x1004,
		CUSTOM_0005 = 0x1005,
		CUSTOM_0006 = 0x1006,
		CUSTOM_0007 = 0x1007,
		CUSTOM_0008 = 0x1008,
		CUSTOM_0009 = 0x1009,
		CUSTOM_0010 = 0x100a,
		CUSTOM_0011 = 0x100b,
		CUSTOM_0012 = 0x100c,
		CUSTOM_0013 = 0x100d,
		CUSTOM_0014 = 0x100e,
		CUSTOM_0015 = 0x100f,
		CUSTOM_0016 = 0x1010,
		CUSTOM_0017 = 0x1011,
		CUSTOM_0018 = 0x1012,
		CUSTOM_0019 = 0x1013,
		CUSTOM_0020 = 0x1014,
		CUSTOM_0021 = 0x1015,
		CUSTOM_0022 = 0x1016,
		CUSTOM_0023 = 0x1017,
		CUSTOM_0024 = 0x1018,
		CUSTOM_0025 = 0x1019,
		CUSTOM_0026 = 0x101a,
		CUSTOM_0027 = 0x101b,
		CUSTOM_0028 = 0x101c,
		CUSTOM_0029 = 0x101d,
		CUSTOM_0030 = 0x101e,
		CUSTOM_0031 = 0x101f		
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
	Message(Message::Type type, uint32_t size, const uint160& checksum) {
		prolog_[0] = QBIT_MESSAGE_0; prolog_[1] = QBIT_MESSAGE_1; prolog_[2] = QBIT_MESSAGE_2; prolog_[3] = QBIT_MESSAGE_3;
		type_ = type;
		size_ = size;
		if (size_ > sizeof(uint160)) checksum_ = checksum;
	}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(prolog_);
		READWRITE(type_);
		READWRITE(size_);
		READWRITE(checksum_);
	}

	bool valid() { return (prolog_[0] == QBIT_MESSAGE_0 && prolog_[1] == QBIT_MESSAGE_1 && prolog_[2] == QBIT_MESSAGE_2 && prolog_[3] == QBIT_MESSAGE_3); }

	Message::Type type() { return (Message::Type)type_; }
	uint32_t dataSize() { return size_; }
	uint160 checkSum() { return checksum_; }

	static size_t size() { return sizeof(prolog_) + sizeof(type_) + sizeof(size_) + (sizeof(uint8_t) * 160/8); }
	static void registerMessageType(Message::Type type, const std::string& name);

	std::string toString();

private:
	char prolog_[4] = {0};	// 4
	unsigned short type_;	// 2
	uint32_t size_;			// 4
	uint160 checksum_;		// 20
};

//
typedef std::map<Message::Type /*type*/, std::string /*description*/> MessageTypes;
extern MessageTypes gMessageTypes; // TODO: init on startup

} // qbit

#endif