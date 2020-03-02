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
		
		GET_BLOCK_HEADER = 0x0010,
		GET_BLOCK_DATA = 0x0011,
		GET_BLOCK_BY_HEIGHT = 0x0012,
		GET_BLOCK_BY_ID = 0x0013,
		GET_NETWORK_BLOCK = 0x0014,
		GET_NETWORK_BLOCK_HEADER = 0x0015,

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

		GET_PEERS = 0x0030,
		PEER_EXISTS = 0x0031,
		PEER_BANNED = 0x0032,
		PEERS = 0x0033
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

	std::string toString() {
		std::string lMsg = "";
		switch(type_) {
			case PING: lMsg = "PING"; break;
			case PONG: lMsg = "PONG"; break;
			case STATE: lMsg = "STATE"; break;
			case GLOBAL_STATE: lMsg = "GLOBAL_STATE"; break;
			case BLOCK: lMsg = "BLOCK"; break;
			case TRANSACTION: lMsg = "TRANSACTION"; break;
			case BLOCK_HEADER: lMsg = "BLOCK_HEADER"; break;
			case GET_BLOCK_HEADER: lMsg = "GET_BLOCK_HEADER"; break;
			case GET_BLOCK_DATA: lMsg = "GET_BLOCK_DATA"; break;
			case GET_PEERS: lMsg = "GET_PEERS"; break;
			case PEER_EXISTS: lMsg = "PEER_EXISTS"; break;
			case PEER_BANNED: lMsg = "PEER_BANNED"; break;
			case PEERS: lMsg = "PEERS"; break;
			case GET_BLOCK_BY_HEIGHT: lMsg = "GET_BLOCK_BY_HEIGHT"; break;
			case GET_BLOCK_BY_ID: lMsg = "GET_BLOCK_BY_ID"; break;
			case GET_NETWORK_BLOCK: lMsg = "GET_NETWORK_BLOCK"; break;
			case BLOCK_BY_HEIGHT: lMsg = "BLOCK_BY_HEIGHT"; break;
			case BLOCK_BY_ID: lMsg = "BLOCK_BY_ID"; break;
			case NETWORK_BLOCK: lMsg = "NETWORK_BLOCK"; break;
			case BLOCK_BY_HEIGHT_IS_ABSENT: lMsg = "BLOCK_BY_HEIGHT_IS_ABSENT"; break;
			case BLOCK_BY_ID_IS_ABSENT: lMsg = "BLOCK_BY_ID_IS_ABSENT"; break;
			case NETWORK_BLOCK_IS_ABSENT: lMsg = "NETWORK_BLOCK_IS_ABSENT"; break;
			case BLOCK_HEADER_AND_STATE: lMsg = "BLOCK_HEADER_AND_STATE"; break;
			case BLOCK_HEADER_IS_ABSENT: lMsg = "BLOCK_HEADER_IS_ABSENT"; break;
			case BLOCK_IS_ABSENT: lMsg = "BLOCK_IS_ABSENT"; break;
			case GET_NETWORK_BLOCK_HEADER: lMsg = "GET_NETWORK_BLOCK_HEADER"; break;
			case NETWORK_BLOCK_HEADER: lMsg = "NETWORK_BLOCK_HEADER"; break;
			case NETWORK_BLOCK_HEADER_IS_ABSENT: lMsg = "NETWORK_BLOCK_HEADER_IS_ABSENT"; break;

			default: lMsg = "UNKNOWN"; break;
		}

		return lMsg += "/" + strprintf("%d/%s", size_, checksum_.toHex());
	}

private:
	char prolog_[4] = {0};	// 4
	unsigned short type_;	// 2
	uint32_t size_;			// 4
	uint160 checksum_;		// 20
};

} // qbit

#endif