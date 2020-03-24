// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_REQUEST_PROCESSOR_H
#define QBIT_BUZZER_REQUEST_PROCESSOR_H

#include "../../../ipeer.h"
#include "../../../irequestprocessor.h"
#include "../../../dapps/buzzer/peerextension.h"

#include <boost/atomic.hpp>

namespace qbit {

// forward
class BuzzerRequestProcessor;
typedef std::shared_ptr<BuzzerRequestProcessor> BuzzerRequestProcessorPtr;

class BuzzerRequestProcessor: public std::enable_shared_from_this<BuzzerRequestProcessor> {
public:
	BuzzerRequestProcessor(IRequestProcessorPtr requestProcessor) : requestProcessor_(requestProcessor) {}

	//
	// make instance
	static BuzzerRequestProcessorPtr instance(IRequestProcessorPtr requestProcessor) {
		return std::make_shared<BuzzerRequestProcessor>(requestProcessor);
	}

	bool loadSubscription(const uint256& chain, const uint256& subscriber, const uint256& publisher, ILoadTransactionHandlerPtr handler) {
		//
		std::map<uint32_t, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			for (std::map<uint32_t, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				if (lPeer->second->extension()) {
					std::static_pointer_cast<BuzzerPeerExtension>(lPeer->second->extension())->loadSubscription(chain, subscriber, publisher, handler);
					return true;
				}
			}
		}

		return false;
	}

private:
	IRequestProcessorPtr requestProcessor_;
};

} // qbit

#endif
