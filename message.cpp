#include "message.h"

using namespace qbit;

bool qbit::gTestNet = false;

qbit::MessageTypes qbit::gMessageTypes;

void Message::registerMessageType(Message::Type type, const std::string& name) {
	qbit::gMessageTypes.insert(MessageTypes::value_type(type, name));
}

std::string Message::toString() {
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
		case TRANSACTION_DATA: lMsg = "TRANSACTION_DATA"; break;
		case GET_TRANSACTION_DATA: lMsg = "GET_TRANSACTION_DATA"; break;
		case TRANSACTION_IS_ABSENT: lMsg = "TRANSACTION_IS_ABSENT"; break;
		case GET_UTXO_BY_ADDRESS: lMsg = "GET_UTXO_BY_ADDRESS"; break;
		case GET_UTXO_BY_ADDRESS_AND_ASSET: lMsg = "GET_UTXO_BY_ADDRESS_AND_ASSET"; break;
		case GET_UTXO_BY_TX: lMsg = "GET_UTXO_BY_TX"; break;
		case UTXO_BY_ADDRESS: lMsg = "UTXO_BY_ADDRESS"; break;
		case UTXO_BY_ADDRESS_AND_ASSET: lMsg = "UTXO_BY_ADDRESS_AND_ASSET"; break;
		case UTXO_BY_TX: lMsg = "UTXO_BY_TX"; break;
		case GET_UTXO_BY_ENTITY: lMsg = "GET_UTXO_BY_ENTITY"; break;
		case GET_ENTITY_COUNT_BY_SHARDS: lMsg = "GET_ENTITY_COUNT_BY_SHARDS"; break;
		case GET_ENTITY: lMsg = "GET_ENTITY"; break;
		case UTXO_BY_ENTITY: lMsg = "UTXO_BY_ENTITY"; break;
		case ENTITY_COUNT_BY_SHARDS: lMsg = "ENTITY_COUNT_BY_SHARDS"; break;
		case ENTITY: lMsg = "ENTITY"; break;
		case ENTITY_IS_ABSENT: lMsg = "ENTITY_IS_ABSENT"; break;
		case CLIENT_SESSIONS_EXCEEDED: lMsg = "CLIENT_SESSIONS_EXCEEDED"; break;
		case PUSH_TRANSACTION: lMsg = "PUSH_TRANSACTION"; break;
		case TRANSACTION_PUSHED: lMsg = "TRANSACTION_PUSHED"; break;
		case GET_UTXO_BY_ENTITY_NAMES: lMsg = "GET_UTXO_BY_ENTITY_NAMES"; break;
		case UTXO_BY_ENTITY_NAMES: lMsg = "UTXO_BY_ENTITY_NAMES"; break;
		case GET_STATE: lMsg = "GET_STATE"; break;
		case GET_ENTITY_COUNT_BY_DAPP: lMsg = "GET_ENTITY_COUNT_BY_DAPP"; break;
		case ENTITY_COUNT_BY_DAPP: lMsg = "ENTITY_COUNT_BY_DAPP"; break;
		case GET_TRANSACTIONS_DATA: lMsg = "GET_TRANSACTIONS_DATA"; break;
		case GET_ENTITY_NAMES: lMsg = "GET_ENTITY_NAMES"; break;
		case ENTITY_NAMES: lMsg = "ENTITY_NAMES"; break;

		default:  {
			lMsg = "UNKNOWN";

			qbit::MessageTypes::iterator lType = gMessageTypes.find((Message::Type)type_);
			if (lType != qbit::gMessageTypes.end()) lMsg = lType->second;			
		}
		break;
	}

	return lMsg += "/" + strprintf("%d/%d[%d:%d:%d]/%s", size_, version_, 
			UNPACK_MAJOR(version_), 
			UNPACK_MINOR(version_), 
			UNPACK_REVISION(version_), checksum_.toHex());
}
