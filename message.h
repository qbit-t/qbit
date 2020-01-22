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

#define QBIT_MESSAGE_0 0x1a
#define QBIT_MESSAGE_1 0x1b
#define QBIT_MESSAGE_2 0x1c
#define QBIT_MESSAGE_3 0x1d

namespace qbit {

class Message {
public:
	enum Type {
		PING = 0x01,
		PONG = 0x02,
		STATE = 0x03,
		GLOBAL_STATE = 0x04,
		BLOCK = 0x05,
		TRANSACTION = 0x06,
		BLOCK_HEADER = 0x07,
		
		GET_BLOCK_HEADER = 0x10,
		GET_BLOCK_DATA = 0x11,
		GET_BLOCK_BY_HEIGHT = 0x12,
		GET_BLOCK_BY_ID = 0x13,
		GET_NETWORK_BLOCK = 0x14,

		BLOCK_BY_HEIGHT = 0x20,
		BLOCK_BY_ID = 0x21,
		NETWORK_BLOCK = 0x22,
		BLOCK_BY_HEIGHT_IS_ABSENT = 0x23,
		BLOCK_BY_ID_IS_ABSENT = 0x24,
		NETWORK_BLOCK_IS_ABSENT = 0x25,

		GET_PEERS = 0x30,
		PEER_EXISTS = 0x31,
		PEER_BANNED = 0x32,
		PEERS = 0x33
	};

public:
	Message() {}
	Message(Message::Type type, uint32_t size) {
		prolog_[0] = QBIT_MESSAGE_0; prolog_[1] = QBIT_MESSAGE_1; prolog_[2] = QBIT_MESSAGE_2; prolog_[3] = QBIT_MESSAGE_3;
		type_ = type;
		size_ = size;
	}

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void serializationOp(Stream& s, Operation ser_action) {
		READWRITE(prolog_);
		READWRITE(type_);
		READWRITE(size_);
	}

	bool valid() { return (prolog_[0] == QBIT_MESSAGE_0 && prolog_[1] == QBIT_MESSAGE_1 && prolog_[2] == QBIT_MESSAGE_2 && prolog_[3] == QBIT_MESSAGE_3); }

	Message::Type type() { return (Message::Type)type_; }
	uint32_t dataSize() { return size_; }

	static size_t size() { return sizeof(prolog_) + sizeof(type_) + sizeof(size_); }

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
			default: lMsg = "UNKNOWN"; break;
		}

		return lMsg += "/" + strprintf("%d", size_);
	}

private:
	char prolog_[4] = {0};
	unsigned char type_;
	uint32_t size_;
};

} // qbit

#endif