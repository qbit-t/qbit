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

namespace qbit {

class Message {
public:
	enum Type {
		PING = 0x01,
		PONG = 0x02,
		STATE = 0x03,
		BLOCK = 0x04,
		TRANSACTION = 0x05,
		GET_BLOCK_HEADER = 0x06,
		GET_BLOCK_DATA = 0x07,
		GET_PEERS = 0x08,
		PEER_EXISTS = 0x09,
		PEER_BANNED = 0x10,
		PEERS = 0x11
	};

public:
	Message() {}
	Message(Message::Type type, uint32_t size) {
		prolog_[0] = 0x1a; prolog_[1] = 0x1b; prolog_[2] = 0x1c; prolog_[3] = 0x1d;
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

	bool valid() { return (prolog_[0] == 0x1a && prolog_[1] == 0x1b && prolog_[2] == 0x1c && prolog_[3] == 0x1d); }

	Message::Type type() { return (Message::Type)type_; }
	uint32_t dataSize() { return size_; }

	static size_t size() { return sizeof(prolog_) + sizeof(type_) + sizeof(size_); }

	std::string toString() {
		std::string lMsg = "";
		switch(type_) {
			case PING: lMsg = "PING"; break;
			case PONG: lMsg = "PONG"; break;
			case STATE: lMsg = "STATE"; break;
			case TRANSACTION: lMsg = "TRANSACTION"; break;
			case GET_BLOCK_HEADER: lMsg = "GET_BLOCK_HEADER"; break;
			case GET_BLOCK_DATA: lMsg = "GET_BLOCK_DATA"; break;
			case GET_PEERS: lMsg = "GET_PEERS"; break;
			case PEER_EXISTS: lMsg = "PEER_EXISTS"; break;
			case PEER_BANNED: lMsg = "PEER_BANNED"; break;
			case PEERS: lMsg = "PEERS"; break;
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