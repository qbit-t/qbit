// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_REQUEST_PROCESSOR_H
#define QBIT_BUZZER_REQUEST_PROCESSOR_H

#include "../../../ipeer.h"
#include "../../../irequestprocessor.h"
#include "../../../dapps/buzzer/peerextension.h"
#include "../../../dapps/buzzer/buzzfeed.h"
#include "../../../dapps/buzzer/conversationsfeed.h"
#include "../../../dapps/buzzer/txrebuzz.h"

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
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->loadSubscription(chain, subscriber, publisher, handler);
					return true;
				}
			}
		}

		return false;
	}

	bool loadTrustScore(const uint256& chain, const uint256& buzzer, ILoadTrustScoreHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->loadTrustScore(chain, buzzer, handler);
					return true;
				}
			}
		}

		return false;
	}

	bool loadBuzzerAndInfo(const std::string& buzzer, ILoadBuzzerAndInfoHandlerPtr handler) {
		//
		std::map<uint256, std::map<uint32_t, IPeerPtr>> lOrder;
		requestProcessor_->collectPeersByDApp("buzzer", lOrder);

		//
		if (lOrder.size()) {
			// use nearest
			std::list<IPeerPtr> lDests;
			for (std::map<uint256, std::map<uint32_t, IPeerPtr>>::iterator lItem = lOrder.begin(); lItem != lOrder.end(); lItem++) {
				//
				if (!lDests.size() || (lDests.size() && (*lDests.rbegin())->addressId() != lItem->second.begin()->second->addressId())) {
					lDests.push_back(lItem->second.begin()->second);
				}
			}

			std::list<IPeerPtr>::iterator lPeer = lDests.begin();
			if (lPeer != lDests.end()) {
				IPeerExtensionPtr lExtension = (*lPeer)->extension("buzzer");
				if (lExtension) { 
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->loadBuzzerAndInfo(buzzer, handler);
					return true;
				}
			}
		}

		return false;
	}

	bool selectBuzzerEndorse(const uint256& chain, const uint256& actor, const uint256& buzzer, ILoadEndorseMistrustHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectBuzzerEndorse(chain, actor, buzzer, handler);
					return true;
				}
			}
		}

		return false;
	}

	bool selectBuzzerMistrust(const uint256& chain, const uint256& actor, const uint256& buzzer, ILoadEndorseMistrustHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectBuzzerMistrust(chain, actor, buzzer, handler);
					return true;
				}
			}
		}

		return false;
	}

	bool selectBuzzes(const uint256& chain, const std::vector<uint256>& buzzes, ISelectBuzzFeedHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		if (lOrder.size()) {
			// use nearest
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectBuzzes(chain, buzzes, handler);
					return true;
				}
			}
		}

		return false;
	}

	int selectBuzzfeed(const uint256& chain, const std::vector<BuzzfeedPublisherFrom>& from, const uint256& subscriber, int requests, ISelectBuzzFeedHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// WARNING: if attacker have intent to buzzfeed manipulation, we need to be sure, that the all items
			// which should be in feed - present and not filtered by attacker node; for this reason now we polling 
			// several nodes with the given shard 
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectBuzzfeed(chain, from, subscriber, handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int selectEventsfeed(const uint256& chain, uint64_t from, const uint256& subscriber, int requests, ISelectEventsFeedHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// use nearest
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectEventsfeed(chain, from, subscriber, handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int selectConversations(const uint256& chain, uint64_t from, const uint256& buzzer, int requests, ISelectConversationsFeedByEntityHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// use nearest
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectConversations(chain, from, buzzer, handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int selectBuzzfeedByBuzz(const uint256& chain, uint64_t from, const uint256& buzz, int requests, ISelectBuzzFeedByEntityHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// WARNING: if attacker have intent to buzzfeed manipulation, we need to be sure, that the all items
			// which should be in feed - present and not filtered by attacker node; for this reason now we polling 
			// several nodes with the given shard 
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectBuzzfeedByBuzz(chain, from, buzz, handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int selectMessages(const uint256& chain, uint64_t from, const uint256& conversation, int requests, ISelectBuzzFeedByEntityHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// WARNING: if attacker have intent to buzzfeed manipulation, we need to be sure, that the all items
			// which should be in feed - present and not filtered by attacker node; for this reason now we polling 
			// several nodes with the given shard 
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectMessages(chain, from, conversation, handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int selectBuzzfeedByBuzzer(const uint256& chain, uint64_t from, const uint256& buzzer, int requests, ISelectBuzzFeedByEntityHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// WARNING: if attacker have intent to buzzfeed manipulation, we need to be sure, that the all items
			// which should be in feed - present and not filtered by attacker node; for this reason now we polling 
			// several nodes with the given shard 
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectBuzzfeedByBuzzer(chain, from, buzzer, handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int selectMistrustsByBuzzer(const uint256& chain, const uint256& from, const uint256& buzzer, int requests, ISelectEventsFeedByEntityHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// use nearest
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectMistrustsByBuzzer(chain, from, buzzer, handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}	

	int selectEndorsementsByBuzzer(const uint256& chain, const uint256& from, const uint256& buzzer, int requests, ISelectEventsFeedByEntityHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// use nearest
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectEndorsementsByBuzzer(chain, from, buzzer, handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int selectSubscriptionsByBuzzer(const uint256& chain, const uint256& from, const uint256& buzzer, int requests, ISelectEventsFeedByEntityHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// use nearest
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectSubscriptionsByBuzzer(chain, from, buzzer, handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int selectFollowersByBuzzer(const uint256& chain, const uint256& from, const uint256& buzzer, int requests, ISelectEventsFeedByEntityHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// use nearest
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectFollowersByBuzzer(chain, from, buzzer, handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int selectBuzzfeedGlobal(const uint256& chain, uint64_t timeframeFrom, uint64_t scoreFrom, uint64_t timestampFrom, const uint256& publisher, int requests, ISelectBuzzFeedHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// nearest some
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectBuzzfeedGlobal(
						chain, 
						timeframeFrom, 
						scoreFrom, 
						timestampFrom,
						publisher, 
						handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int selectBuzzfeedByTag(const uint256& chain, const std::string& tag, uint64_t timeframeFrom, uint64_t scoreFrom, uint64_t timestampFrom, const uint256& publisher, int requests, ISelectBuzzFeedHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// nearest some
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectBuzzfeedByTag(
						chain, 
						tag,
						timeframeFrom, 
						scoreFrom, 
						timestampFrom,
						publisher, 
						handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int selectHashTags(const uint256& chain, const std::string& tag, int requests, ISelectHashTagsHandlerPtr handler) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// nearest some
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->selectHashTags(
						chain, 
						tag,
						handler);
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int subscribeBuzzThread(const uint256& chain, const uint256& buzzId, const uint512& signature, int requests, 
																							std::list<uint160>& peers) {
		//
		std::map<IRequestProcessor::KeyOrder, IPeerPtr> lOrder;
		requestProcessor_->collectPeersByChain(chain, lOrder);

		int lCount = 0;
		if (lOrder.size()) {
			// nearest some
			for (std::map<IRequestProcessor::KeyOrder, IPeerPtr>::iterator lPeer = lOrder.begin(); lPeer != lOrder.end(); lPeer++) {
				IPeerExtensionPtr lExtension = lPeer->second->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->subscribeBuzzThread(
						chain, 
						buzzId,
						signature);
					// collect peers
					peers.push_back(lPeer->second->addressId());
					// for now just 1 active feed
					if (++lCount == requests /*may be 2/3 nearest - but it doubles traffic*/) break;
				}
			}
		}

		return lCount;
	}

	int unsubscribeBuzzThread(const uint256& chain, const uint256& buzzId, const std::list<uint160>& peers) {
		//
		int lCount = 0;
		for (std::list<uint160>::const_iterator lId = peers.begin(); lId != peers.end(); lId++) {
			//
			IPeerPtr lPeer = requestProcessor_->locatePeer(*lId);
			if (lPeer) {
				IPeerExtensionPtr lExtension = lPeer->extension("buzzer");
				if (lExtension) {
					std::static_pointer_cast<BuzzerPeerExtension>(lExtension)->unsubscribeBuzzThread(
						chain, 
						buzzId);
					lCount++;
				}				
			}
		}

		return lCount;
	}

private:
	IRequestProcessorPtr requestProcessor_;
};

} // qbit

#endif
