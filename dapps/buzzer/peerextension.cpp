#include "peerextension.h"

using namespace qbit;

void BuzzerPeerExtension::processTransaction(TransactionContextPtr ctx) {
	
}

bool BuzzerPeerExtension::processMessage(Message& message, std::list<DataStream>::iterator data, const boost::system::error_code& error) {
	//
	return false;	
}
