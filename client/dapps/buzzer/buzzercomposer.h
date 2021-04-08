// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_LIGHT_COMPOSER_H
#define QBIT_BUZZER_LIGHT_COMPOSER_H

#include "../../handlers.h"
#include "../../../dapps/buzzer/txbuzzer.h"
#include "../../../dapps/buzzer/txbuzz.h"
#include "../../../dapps/buzzer/txbuzzlike.h"
#include "../../../dapps/buzzer/txbuzzreward.h"
#include "../../../dapps/buzzer/txbuzzreply.h"
#include "../../../dapps/buzzer/txrebuzz.h"
#include "../../../dapps/buzzer/txrebuzznotify.h"
#include "../../../dapps/buzzer/txbuzzersubscribe.h"
#include "../../../dapps/buzzer/txbuzzerunsubscribe.h"
#include "../../../dapps/buzzer/txbuzzerendorse.h"
#include "../../../dapps/buzzer/txbuzzermistrust.h"
#include "../../../dapps/buzzer/txbuzzerinfo.h"
#include "../../../dapps/buzzer/txbuzzerconversation.h"
#include "../../../dapps/buzzer/txbuzzeracceptconversation.h"
#include "../../../dapps/buzzer/txbuzzerdeclineconversation.h"
#include "../../../dapps/buzzer/txbuzzermessage.h"
#include "../../../dapps/buzzer/txbuzzermessagereply.h"
#include "../../../dapps/buzzer/buzzer.h"
#include "../../../dapps/buzzer/buzzfeed.h"
#include "../../../dapps/buzzer/eventsfeed.h"
#include "../../../dapps/buzzer/handlers.h"
#include "../../../txshard.h"
#include "../../../lightwallet.h"
#include "../../../db/containers.h"
#include "buzzerrequestprocessor.h"

namespace qbit {

// forward
class BuzzerLightComposer;
typedef std::shared_ptr<BuzzerLightComposer> BuzzerLightComposerPtr;

//
class BuzzerLightComposer: public std::enable_shared_from_this<BuzzerLightComposer> {
public:
	class CreateTxBuzzer: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzer> {
	public:
		CreateTxBuzzer(BuzzerLightComposerPtr composer, const std::string& buzzer, transactionCreatedWithUTXOFunction created): 
			composer_(composer), buzzer_(buzzer), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& buzzer, transactionCreatedWithUTXOFunction created) {
			return std::make_shared<CreateTxBuzzer>(composer, buzzer, created); 
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during creating buzzer.");
		}

		//
		void utxoByDAppLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void dAppInstancesCountByShardsLoaded(const std::map<uint32_t, uint256>&, const std::string&);
		void shardLoaded(TransactionPtr);
		void utxoByShardLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void assetNamesLoaded(const std::string&, const std::vector<IEntityStore::EntityName>&);
		// NOTICE: refactored
		// void buzzerEntityLoaded(EntityPtr);

	private:
		BuzzerLightComposerPtr composer_;
		std::string buzzer_;
		transactionCreatedWithUTXOFunction created_;
		errorFunction error_;

		std::string buzzerName_;

		TxBuzzerPtr buzzerTx_;
		TransactionContextPtr ctx_;
		TxShardPtr shardTx_;
		Transaction::UnlinkedOutPtr buzzerOut_;
	};

	class CreateTxBuzzerInfo: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzerInfo> {
	public:
		CreateTxBuzzerInfo(BuzzerLightComposerPtr composer, Transaction::UnlinkedOutPtr buzzer,
			const std::string& alias, 
			const std::string& description,
			const BuzzerMediaPointer& avatar,
			const BuzzerMediaPointer& header, 
			transactionCreatedFunction created): 
			composer_(composer),
			buzzer_(buzzer),
			alias_(alias), 
			description_(description), 
			avatar_(avatar),
			header_(header), 
			created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, Transaction::UnlinkedOutPtr buzzer,
				const std::string& alias, 
				const std::string& description, 
				const BuzzerMediaPointer& avatar,
				const BuzzerMediaPointer& header, 
				transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzerInfo>(composer, buzzer, alias, description, avatar, header, created); 
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzer info creation.");
		}

		//
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

	private:
		BuzzerLightComposerPtr composer_;
		Transaction::UnlinkedOutPtr buzzer_;
		std::string alias_;
		std::string description_;
		BuzzerMediaPointer avatar_;
		BuzzerMediaPointer header_;	
		transactionCreatedFunction created_;
		errorFunction error_;

		TxBuzzerInfoPtr tx_;
		TransactionContextPtr ctx_;
	};

	class CreateTxBuzz: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzz> {
	public:
		CreateTxBuzz(BuzzerLightComposerPtr composer, const std::string& body, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created): composer_(composer), body_(body), mediaPointers_(mediaPointers), created_(created) {}
		CreateTxBuzz(BuzzerLightComposerPtr composer, const std::string& body, const std::vector<std::string>& buzzers, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created): composer_(composer), body_(body), buzzers_(buzzers), mediaPointers_(mediaPointers), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& body, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzz>(composer, body, mediaPointers, created); 
		} 

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& body, const std::vector<std::string>& buzzers, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzz>(composer, body, buzzers, mediaPointers, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzz creation.");
		}

		//
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

		//
		void utxoByBuzzersListLoaded(const std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>&);

	private:
		BuzzerLightComposerPtr composer_;
		std::string body_;
		std::vector<std::string> buzzers_;
		std::vector<BuzzerMediaPointer> mediaPointers_;
		transactionCreatedFunction created_;
		errorFunction error_;

		TxBuzzPtr buzzTx_;
		TransactionContextPtr ctx_;
	};

	class CreateTxBuzzerSubscribe: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzerSubscribe> {
	public:
		CreateTxBuzzerSubscribe(BuzzerLightComposerPtr composer, const std::string& publisher, transactionCreatedFunction created): composer_(composer), publisher_(publisher), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& publisher, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzerSubscribe>(composer, publisher, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzer subscription.");
		}

		//
		void publisherLoaded(EntityPtr);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);		
		void utxoByPublisherLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

	private:
		BuzzerLightComposerPtr composer_;
		std::string publisher_;
		transactionCreatedFunction created_;
		errorFunction error_;		

		EntityPtr publisherTx_;
		TxBuzzerSubscribePtr buzzerSubscribeTx_;
		TransactionContextPtr ctx_;
	};	

	class CreateTxBuzzerUnsubscribe: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzerUnsubscribe> {
	public:
		CreateTxBuzzerUnsubscribe(BuzzerLightComposerPtr composer, const std::string& publisher, transactionCreatedFunction created): composer_(composer), publisher_(publisher), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& publisher, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzerUnsubscribe>(composer, publisher, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzer subscription.");
		}

		//
		void publisherLoaded(EntityPtr);
		void subscriptionLoaded(TransactionPtr);
		void utxoBySubscriptionLoaded(const std::vector<Transaction::NetworkUnlinkedOut>&, const uint256&);

	private:
		BuzzerLightComposerPtr composer_;
		std::string publisher_;
		transactionCreatedFunction created_;
		errorFunction error_;		

		TxBuzzerUnsubscribePtr buzzerUnsubscribeTx_;
		TransactionContextPtr ctx_;
		uint256 shardTx_;
	};	

	class LoadBuzzfeed: public IComposerMethod, public std::enable_shared_from_this<LoadBuzzfeed> {
	public:
		LoadBuzzfeed(BuzzerLightComposerPtr composer, const uint256& chain, const std::vector<BuzzfeedPublisherFrom>& from, int requests, buzzfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), requests_(requests), loaded_(loaded) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const std::vector<BuzzfeedPublisherFrom>& from, int requests, buzzfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadBuzzfeed>(composer, chain, from, requests, loaded); 
		} 

		//
		void buzzfeedLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
			loaded_(feed, chain, count_);
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzfeed load.");
		}

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		std::vector<BuzzfeedPublisherFrom> from_;
		buzzfeedLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadBuzzesByBuzz: public IComposerMethod, public std::enable_shared_from_this<LoadBuzzesByBuzz> {
	public:
		LoadBuzzesByBuzz(BuzzerLightComposerPtr composer, const uint256& chain, uint64_t from, const uint256& buzz, int requests, buzzfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), buzz_(buzz), requests_(requests), loaded_(loaded) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, uint64_t from, const uint256& buzz, int requests, buzzfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadBuzzesByBuzz>(composer, chain, from, buzz, requests, loaded); 
		} 

		//
		void buzzfeedLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain, const uint256& entity) {
			loaded_(feed, chain, count_);
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzfeed load.");
		}

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint64_t from_;
		uint256 buzz_;
		buzzfeedLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadMessages: public IComposerMethod, public std::enable_shared_from_this<LoadMessages> {
	public:
		LoadMessages(BuzzerLightComposerPtr composer, const uint256& chain, uint64_t from, const uint256& conversation, int requests, buzzfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), conversation_(conversation), requests_(requests), loaded_(loaded) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, uint64_t from, const uint256& conversation, int requests, buzzfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadMessages>(composer, chain, from, conversation, requests, loaded); 
		} 

		//
		void messagesLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain, const uint256& entity) {
			loaded_(feed, chain, count_);
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzfeed load.");
		}

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint64_t from_;
		uint256 conversation_;
		buzzfeedLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadBuzzesByBuzzer: public IComposerMethod, public std::enable_shared_from_this<LoadBuzzesByBuzzer> {
	public:
		LoadBuzzesByBuzzer(BuzzerLightComposerPtr composer, const uint256& chain, const std::vector<BuzzfeedPublisherFrom>& from, const std::string& buzzer, int requests, buzzfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), buzzer_(buzzer), requests_(requests), loaded_(loaded) {}
		LoadBuzzesByBuzzer(BuzzerLightComposerPtr composer, const uint256& chain, const std::vector<BuzzfeedPublisherFrom>& from, const uint256& buzzerId, int requests, buzzfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), buzzerId_(buzzerId), requests_(requests), loaded_(loaded) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const std::vector<BuzzfeedPublisherFrom>& from, const std::string& buzzer, int requests, buzzfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadBuzzesByBuzzer>(composer, chain, from, buzzer, requests, loaded); 
		} 

		//
		void buzzfeedLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain, const uint256& entity) {
			loaded_(feed, chain, count_);
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzfeed load.");
		}

		void publisherLoaded(EntityPtr);

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		std::vector<BuzzfeedPublisherFrom> from_;
		std::string buzzer_;
		uint256 buzzerId_;
		buzzfeedLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadBuzzesGlobal: public IComposerMethod, public std::enable_shared_from_this<LoadBuzzesGlobal> {
	public:
		LoadBuzzesGlobal(BuzzerLightComposerPtr composer, const uint256& chain, 
			uint64_t timeframeFrom, uint64_t scoreFrom, uint64_t timestampFrom, const uint256& publisher, 
			int requests, buzzfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), timeframeFrom_(timeframeFrom), scoreFrom_(scoreFrom), timestampFrom_(timestampFrom),
			publisher_(publisher), requests_(requests), loaded_(loaded) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, 
			uint64_t timeframeFrom, uint64_t scoreFrom, uint64_t timestampFrom, const uint256& publisher, 
			int requests, buzzfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadBuzzesGlobal>(composer, chain, timeframeFrom, scoreFrom, timestampFrom, publisher, 
																										requests, loaded); 
		} 

		//
		void buzzfeedLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
			loaded_(feed, chain, count_);
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzfeed load.");
		}

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint64_t timeframeFrom_;
		uint64_t scoreFrom_;
		uint64_t timestampFrom_;
		uint256 publisher_;
		buzzfeedLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadBuzzesByTag: public IComposerMethod, public std::enable_shared_from_this<LoadBuzzesByTag> {
	public:
		LoadBuzzesByTag(BuzzerLightComposerPtr composer, const uint256& chain, 
			const std::string& tag, uint64_t timeframeFrom, uint64_t scoreFrom, uint64_t timestampFrom, const uint256& publisher, 
			int requests, buzzfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), tag_(tag), timeframeFrom_(timeframeFrom), scoreFrom_(scoreFrom), timestampFrom_(timestampFrom), publisher_(publisher),
			requests_(requests), loaded_(loaded) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const std::string& tag,
			uint64_t timeframeFrom, uint64_t scoreFrom, uint64_t timestampFrom, const uint256& publisher, 
			int requests, buzzfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadBuzzesByTag>(composer, chain, tag, timeframeFrom, 
					scoreFrom, timestampFrom, publisher, requests, loaded); 
		} 

		//
		void buzzfeedLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
			loaded_(feed, chain, count_);
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzfeed load.");
		}

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		std::string tag_;
		uint64_t timeframeFrom_;
		uint64_t scoreFrom_;
		uint64_t timestampFrom_;
		uint256 publisher_;
		buzzfeedLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadHashTags: public IComposerMethod, public std::enable_shared_from_this<LoadHashTags> {
	public:
		LoadHashTags(BuzzerLightComposerPtr composer, const uint256& chain, const std::string& tag, 
			int requests, hashTagsLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), tag_(tag),
			requests_(requests), loaded_(loaded) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const std::string& tag, 
			int requests, hashTagsLoadedRequestFunction loaded) {
			return std::make_shared<LoadHashTags>(composer, chain, tag, requests, loaded); 
		} 

		//
		void loaded(const std::vector<Buzzer::HashTag>& feed, const uint256& chain) {
			loaded_(feed, chain, count_);
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzfeed load.");
		}

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		std::string tag_;
		hashTagsLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadEventsfeed: public IComposerMethod, public std::enable_shared_from_this<LoadEventsfeed> {
	public:
		LoadEventsfeed(BuzzerLightComposerPtr composer, const uint256& chain, uint64_t from, int requests, eventsfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), requests_(requests), loaded_(loaded) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, uint64_t from, int requests, eventsfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadEventsfeed>(composer, chain, from, requests, loaded); 
		} 

		//
		void eventsfeedLoaded(const std::vector<EventsfeedItem>& feed, const uint256& chain) {
			loaded_(feed, chain, count_);
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during eventsfeed load.");
		}

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint64_t from_;
		eventsfeedLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadConversations: public IComposerMethod, public std::enable_shared_from_this<LoadConversations> {
	public:
		LoadConversations(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzzer, uint64_t from, int requests, conversationsLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), buzzer_(buzzer), from_(from), requests_(requests), loaded_(loaded) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzzer, uint64_t from, int requests, conversationsLoadedRequestFunction loaded) {
			return std::make_shared<LoadConversations>(composer, chain, buzzer, from, requests, loaded); 
		} 

		//
		void conversationsLoaded(const std::vector<ConversationItem>& feed, const uint256& chain, const uint256& entity) {
			loaded_(feed, chain, count_);
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during eventsfeed load.");
		}

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint256 buzzer_;
		uint64_t from_;
		conversationsLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadMistrustsByBuzzer: public IComposerMethod, public std::enable_shared_from_this<LoadMistrustsByBuzzer> {
	public:
		LoadMistrustsByBuzzer(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const uint256& buzzer, int requests, eventsfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), buzzer_(buzzer), requests_(requests), loaded_(loaded) {}
		LoadMistrustsByBuzzer(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const std::string& buzzer, int requests, eventsfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), buzzerName_(buzzer), requests_(requests), loaded_(loaded) {}

		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const uint256& buzzer, int requests, eventsfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadMistrustsByBuzzer>(composer, chain, from, buzzer, requests, loaded); 
		} 
		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const std::string& buzzer, int requests, eventsfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadMistrustsByBuzzer>(composer, chain, from, buzzer, requests, loaded);
		} 

		//
		void eventsfeedLoaded(const std::vector<EventsfeedItem>& feed, const uint256& chain, const uint256& entity) {
			loaded_(feed, chain, count_);
		}
		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during eventsfeed load.");
		}
		//
		void publisherLoaded(EntityPtr);

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint256 from_;
		uint256 buzzer_;
		std::string buzzerName_;
		eventsfeedLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadEndorsementsByBuzzer: public IComposerMethod, public std::enable_shared_from_this<LoadEndorsementsByBuzzer> {
	public:
		LoadEndorsementsByBuzzer(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const uint256& buzzer, int requests, eventsfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), buzzer_(buzzer), requests_(requests), loaded_(loaded) {}
		LoadEndorsementsByBuzzer(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const std::string& buzzer, int requests, eventsfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), buzzerName_(buzzer), requests_(requests), loaded_(loaded) {}

		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const uint256& buzzer, int requests, eventsfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadEndorsementsByBuzzer>(composer, chain, from, buzzer, requests, loaded); 
		} 
		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const std::string& buzzer, int requests, eventsfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadEndorsementsByBuzzer>(composer, chain, from, buzzer, requests, loaded); 
		} 

		//
		void eventsfeedLoaded(const std::vector<EventsfeedItem>& feed, const uint256& chain, const uint256& entity) {
			loaded_(feed, chain, count_);
		}
		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during eventsfeed load.");
		}
		//
		void publisherLoaded(EntityPtr);

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint256 from_;
		uint256 buzzer_;
		std::string buzzerName_;
		eventsfeedLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadSubscriptionsByBuzzer: public IComposerMethod, public std::enable_shared_from_this<LoadSubscriptionsByBuzzer> {
	public:
		LoadSubscriptionsByBuzzer(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const uint256& buzzer, int requests, eventsfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), buzzer_(buzzer), requests_(requests), loaded_(loaded) {}
		LoadSubscriptionsByBuzzer(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const std::string& buzzer, int requests, eventsfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), buzzerName_(buzzer), requests_(requests), loaded_(loaded) {}

		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const uint256& buzzer, int requests, eventsfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadSubscriptionsByBuzzer>(composer, chain, from, buzzer, requests, loaded); 
		} 
		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const std::string& buzzer, int requests, eventsfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadSubscriptionsByBuzzer>(composer, chain, from, buzzer, requests, loaded); 
		} 

		//
		void eventsfeedLoaded(const std::vector<EventsfeedItem>& feed, const uint256& chain, const uint256& entity) {
			loaded_(feed, chain, count_);
		}
		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during eventsfeed load.");
		}
		//
		void publisherLoaded(EntityPtr);

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint256 from_;
		uint256 buzzer_;
		std::string buzzerName_;
		eventsfeedLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class LoadFollowersByBuzzer: public IComposerMethod, public std::enable_shared_from_this<LoadFollowersByBuzzer> {
	public:
		LoadFollowersByBuzzer(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const uint256& buzzer, int requests, eventsfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), buzzer_(buzzer), requests_(requests), loaded_(loaded) {}
		LoadFollowersByBuzzer(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const std::string& buzzer, int requests, eventsfeedLoadedRequestFunction loaded): 
			composer_(composer), chain_(chain), from_(from), buzzerName_(buzzer), requests_(requests), loaded_(loaded) {}

		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const uint256& buzzer, int requests, eventsfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadFollowersByBuzzer>(composer, chain, from, buzzer, requests, loaded); 
		} 
		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& from, const std::string& buzzer, int requests, eventsfeedLoadedRequestFunction loaded) {
			return std::make_shared<LoadFollowersByBuzzer>(composer, chain, from, buzzer, requests, loaded); 
		} 

		//
		void eventsfeedLoaded(const std::vector<EventsfeedItem>& feed, const uint256& chain, const uint256& entity) {
			loaded_(feed, chain, count_);
		}
		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during eventsfeed load.");
		}
		//
		void publisherLoaded(EntityPtr);

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint256 from_;
		uint256 buzzer_;
		std::string buzzerName_;
		eventsfeedLoadedRequestFunction loaded_;
		errorFunction error_;
		int count_;
		int requests_;
	};

	class CreateTxBuzzLike: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzLike> {
	public:
		CreateTxBuzzLike(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, transactionCreatedFunction created): composer_(composer), chain_(chain), buzz_(buzz), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzLike>(composer, chain, buzz, created);
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzz like creation.");
		}

		//
		void utxoByBuzzLoaded(const std::vector<Transaction::NetworkUnlinkedOut>&, const uint256&);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);		

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint256 buzz_;
		transactionCreatedFunction created_;
		errorFunction error_;

		std::vector<Transaction::NetworkUnlinkedOut> buzzUtxo_;
	};	

	class CreateTxBuzzReward: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzReward> {
	public:
		CreateTxBuzzReward(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, 
			const PKey& dest, amount_t reward, transactionCreatedFunction created): 
				composer_(composer), chain_(chain), buzz_(buzz), dest_(dest), reward_(reward), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, 
			const PKey& dest, amount_t reward, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzReward>(composer, chain, buzz, dest, reward, created);
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzz reward creation.");
		}

		//
		void utxoByBuzzLoaded(const std::vector<Transaction::NetworkUnlinkedOut>&, const uint256&);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);		

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint256 buzz_;
		PKey dest_;
		amount_t reward_;
		transactionCreatedFunction created_;
		errorFunction error_;

		std::vector<Transaction::NetworkUnlinkedOut> buzzUtxo_;
	};	

	class CreateTxBuzzReply: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzReply> {
	public:
		CreateTxBuzzReply(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, const std::string& body, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created): composer_(composer), chain_(chain), buzz_(buzz), body_(body), mediaPointers_(mediaPointers), created_(created) {}
		CreateTxBuzzReply(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, const std::string& body, const std::vector<std::string>& buzzers, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created): composer_(composer), chain_(chain), buzz_(buzz), body_(body), buzzers_(buzzers), mediaPointers_(mediaPointers), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, const std::string& body, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzReply>(composer, chain, buzz, body, mediaPointers, created); 
		} 

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, const std::string& body, const std::vector<std::string>& buzzers, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzReply>(composer, chain, buzz, body, buzzers, mediaPointers, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzz reply creation.");
		}

		//
		void utxoByBuzzLoaded(const std::vector<Transaction::NetworkUnlinkedOut>&, const uint256&);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

		//
		void utxoByBuzzersListLoaded(const std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>&);

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint256 buzz_;
		std::string body_;
		std::vector<std::string> buzzers_;
		std::vector<BuzzerMediaPointer> mediaPointers_;
		transactionCreatedFunction created_;
		errorFunction error_;

		TxBuzzReplyPtr buzzTx_;
		TransactionContextPtr ctx_;
		std::vector<Transaction::NetworkUnlinkedOut> buzzUtxo_;		
	};

	class CreateTxRebuzz: public IComposerMethod, public std::enable_shared_from_this<CreateTxRebuzz> {
	public:
		CreateTxRebuzz(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, const std::string& body, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created): composer_(composer), chain_(chain), buzz_(buzz), body_(body), mediaPointers_(mediaPointers), created_(created) {}
		CreateTxRebuzz(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, const std::string& body, const std::vector<std::string>& buzzers, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created): composer_(composer), chain_(chain), buzz_(buzz), body_(body), buzzers_(buzzers), mediaPointers_(mediaPointers), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, const std::string& body, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created) {
			return std::make_shared<CreateTxRebuzz>(composer, chain, buzz, body, mediaPointers, created); 
		} 

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& buzz, const std::string& body, const std::vector<std::string>& buzzers, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created) {
			return std::make_shared<CreateTxRebuzz>(composer, chain, buzz, body, buzzers, mediaPointers, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during rebuzz creation.");
		}

		//
		void buzzLoaded(TransactionPtr);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

		//
		void utxoByBuzzersListLoaded(const std::vector<ISelectUtxoByEntityNamesHandler::EntityUtxo>&);

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint256 buzz_;
		std::string body_;
		std::vector<std::string> buzzers_;
		std::vector<BuzzerMediaPointer> mediaPointers_;
		transactionCreatedFunction created_;
		errorFunction error_;

		TxReBuzzPtr buzzTx_;
		TxReBuzzNotifyPtr notifyTx_;
		TransactionContextPtr ctx_;
	};

	class CreateTxBuzzerEndorse: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzerEndorse> {
	public:
		CreateTxBuzzerEndorse(BuzzerLightComposerPtr composer, const std::string& publisher, amount_t points, transactionCreatedFunction created): composer_(composer), publisher_(publisher), points_(points), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& publisher, amount_t points, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzerEndorse>(composer, publisher, points, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzer endorsement creation.");
		}

		//
		void publisherLoaded(EntityPtr);
		void endorseTxLoaded(const uint256&);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);		
		void utxoByPublisherLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

	private:
		BuzzerLightComposerPtr composer_;
		std::string publisher_;
		amount_t points_;
		transactionCreatedFunction created_;
		errorFunction error_;		

		TxBuzzerEndorsePtr buzzerEndorseTx_;
		TransactionContextPtr ctx_;
		std::vector<Transaction::UnlinkedOut> publisherUtxo_;
		uint256 publisherId_;
	};	

	class CreateTxBuzzerMistrust: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzerMistrust> {
	public:
		CreateTxBuzzerMistrust(BuzzerLightComposerPtr composer, const std::string& publisher, amount_t points, transactionCreatedFunction created): composer_(composer), publisher_(publisher), points_(points), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& publisher, amount_t points, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzerMistrust>(composer, publisher, points, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzer mistrust creation.");
		}

		//
		void publisherLoaded(EntityPtr);
		void mistrustTxLoaded(const uint256&);		
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);		
		void utxoByPublisherLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

	private:
		BuzzerLightComposerPtr composer_;
		std::string publisher_;
		amount_t points_;
		transactionCreatedFunction created_;
		errorFunction error_;		

		TxBuzzerMistrustPtr buzzerMistrustTx_;
		TransactionContextPtr ctx_;
		std::vector<Transaction::UnlinkedOut> publisherUtxo_;
		uint256 publisherId_;
	};

	class LoadBuzzerInfo: public IComposerMethod, public std::enable_shared_from_this<LoadBuzzerInfo> {
	public:
		LoadBuzzerInfo(BuzzerLightComposerPtr composer, const std::string& buzzer, buzzerAndInfoByBuzzerLoadedFunction loaded): 
			composer_(composer), buzzer_(buzzer), loaded_(loaded) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& buzzer, buzzerAndInfoByBuzzerLoadedFunction loaded) {
			return std::make_shared<LoadBuzzerInfo>(composer, buzzer,loaded); 
		} 

		//
		void transactionLoaded(EntityPtr entity, TransactionPtr info);

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during buzzer info load.");
		}

	private:
		BuzzerLightComposerPtr composer_;
		std::string buzzer_;
		buzzerAndInfoByBuzzerLoadedFunction loaded_;
		errorFunction error_;
	};

	class CreateTxBuzzerConversation: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzerConversation> {
	public:
		CreateTxBuzzerConversation(BuzzerLightComposerPtr composer, const std::string& counterparty, transactionCreatedFunction created): composer_(composer), counterparty_(counterparty), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const std::string& counterparty, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzerConversation>(composer, counterparty, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during making conversation.");
		}

		//
		void counterpartyLoaded(EntityPtr);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);		
		void utxoByCounterpartyLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

	private:
		BuzzerLightComposerPtr composer_;
		std::string counterparty_;
		transactionCreatedFunction created_;
		errorFunction error_;		

		EntityPtr counterpartyTx_;
		std::vector<Transaction::UnlinkedOut> counterpartyUtxo_;
		TxBuzzerConversationPtr conversationTx_;
		TransactionContextPtr ctx_;
	};

	class CreateTxBuzzerAcceptConversation: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzerAcceptConversation> {
	public:
		CreateTxBuzzerAcceptConversation(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& conversation, transactionCreatedFunction created): composer_(composer), chain_(chain), conversation_(conversation), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& conversation, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzerAcceptConversation>(composer, chain, conversation, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired accepting conversation.");
		}

		//
		void utxoByConversationLoaded(const std::vector<Transaction::NetworkUnlinkedOut>&, const uint256&);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void conversationLoaded(TransactionPtr);
		void createMessage(const PKey&);

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint256 conversation_;
		transactionCreatedFunction created_;
		errorFunction error_;

		TxBuzzerAcceptConversationPtr acceptTx_;
		TransactionContextPtr ctx_;
		std::vector<Transaction::NetworkUnlinkedOut> conversationUtxo_;
	};

	class CreateTxBuzzerDeclineConversation: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzerDeclineConversation> {
	public:
		CreateTxBuzzerDeclineConversation(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& conversation, transactionCreatedFunction created): composer_(composer), chain_(chain), conversation_(conversation), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const uint256& chain, const uint256& conversation, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzerDeclineConversation>(composer, chain, conversation, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired accepting conversation.");
		}

		//
		void utxoByConversationLoaded(const std::vector<Transaction::NetworkUnlinkedOut>&, const uint256&);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

	private:
		BuzzerLightComposerPtr composer_;
		uint256 chain_;
		uint256 conversation_;
		transactionCreatedFunction created_;
		errorFunction error_;

		TxBuzzerDeclineConversationPtr declineTx_;
		TransactionContextPtr ctx_;
		std::vector<Transaction::NetworkUnlinkedOut> conversationUtxo_;
	};

	class CreateTxBuzzerMessage: public IComposerMethod, public std::enable_shared_from_this<CreateTxBuzzerMessage> {
	public:
		CreateTxBuzzerMessage(BuzzerLightComposerPtr composer, const PKey& pkey, const uint256& chain, const uint256& conversation, const std::string& body, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created): composer_(composer), chain_(chain), conversation_(conversation), body_(body), mediaPointers_(mediaPointers), created_(created) {
			//
			pkey_.set<const unsigned char*>(pkey.begin(), pkey.end());
		}

		void process(errorFunction);

		static IComposerMethodPtr instance(BuzzerLightComposerPtr composer, const PKey& pkey, const uint256& chain, const uint256& conversation, const std::string& body, const std::vector<BuzzerMediaPointer>& mediaPointers, transactionCreatedFunction created) {
			return std::make_shared<CreateTxBuzzerMessage>(composer, pkey, chain, conversation, body, mediaPointers, created); 
		} 

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during conversation message creation.");
		}

		//
		void utxoByConversationLoaded(const std::vector<Transaction::NetworkUnlinkedOut>&, const uint256&);
		void utxoByBuzzerLoaded(const std::vector<Transaction::UnlinkedOut>&, const std::string&);
		void saveBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>&, const std::string&);

	private:
		BuzzerLightComposerPtr composer_;
		PKey pkey_;
		uint256 chain_;
		uint256 conversation_;
		std::string body_;
		std::vector<BuzzerMediaPointer> mediaPointers_;
		transactionCreatedFunction created_;
		errorFunction error_;

		TxBuzzerMessagePtr messageTx_;
		TransactionContextPtr ctx_;
		std::vector<Transaction::NetworkUnlinkedOut> conversationUtxo_;		
	};

public:
	BuzzerLightComposer(ISettingsPtr settings, BuzzerPtr buzzer, IWalletPtr wallet, IRequestProcessorPtr requestProcessor, BuzzerRequestProcessorPtr buzzerRequestProcessor): 
		settings_(settings), buzzer_(buzzer), wallet_(wallet), requestProcessor_(requestProcessor), buzzerRequestProcessor_(buzzerRequestProcessor),
		workingSettings_(settings->dataPath() + "/wallet/buzzer/settings"),
		subscriptions_(settings->dataPath() + "/wallet/buzzer/subscriptions"),
		contacts_(settings->dataPath() + "/wallet/buzzer/contacts") {
		opened_ = false;
	}

	static BuzzerLightComposerPtr instance(ISettingsPtr settings, BuzzerPtr buzzer, IWalletPtr wallet, IRequestProcessorPtr requestProcessor, BuzzerRequestProcessorPtr buzzerRequestProcessor) {
		return std::make_shared<BuzzerLightComposer>(settings, buzzer, wallet, requestProcessor, buzzerRequestProcessor); 
	}

	// composer management
	bool open();
	bool close();
	bool isOpened() { return opened_; }

	std::string dAppName() { return "buzzer"; }
	IWalletPtr wallet() { return wallet_; }
	ISettingsPtr settings() { return settings_; }
	IRequestProcessorPtr requestProcessor() { return requestProcessor_; }
	BuzzerRequestProcessorPtr buzzerRequestProcessor() { return buzzerRequestProcessor_; }
	BuzzerPtr buzzer() { return buzzer_; }

	bool verifySecret(const std::string& /*secret*/);
	void setSecret(const std::string& /*secret*/);

	void addSubscription(const uint256& publisher, const PKey& key) {
		//
		subscriptions_.write(publisher, key);

		// share
		writeSubscription(publisher, key);
	}

	void removeSubscription(const uint256& publisher) {
		//
		subscriptions_.remove(publisher);

		// share
		writeSubscriptions();
	}

	bool subscriptionExists(const uint256& publisher) {
		//
		PKey lPKey;
		if (subscriptions_.read(publisher, lPKey)) {
			return true;
		}

		return false;
	}

	bool haveSubscriptions() {
		//
		db::DbContainer<uint256 /*publisher*/, PKey /*pubkey*/>::Iterator lBegin = subscriptions_.begin();
		if (lBegin.valid()) {
			++lBegin;
			if (lBegin.valid()) {
				return true;
			}
		}

		return false;
	}

	Buzzer::VerificationResult verifyPublisherStrict(BuzzfeedItemPtr);
	Buzzer::VerificationResult verifyPublisherLazy(BuzzfeedItemPtr);
	Buzzer::VerificationResult verifyEventPublisher(EventsfeedItemPtr);
	Buzzer::VerificationResult verifyConversationPublisher(ConversationItemPtr);
	Buzzer::VerificationResult verifyMyThreadUpdates(const uint256&, uint64_t, const uint512&);

	bool writeBuzzerTx(TxBuzzerPtr buzzerTx) {
		//
		buzzerTx_ = buzzerTx;
		// load buzzer tx
		if (!open()) return false;

		// save our buzzer
		DataStream lStream(SER_DISK, PROTOCOL_VERSION);
		Transaction::Serializer::serialize<DataStream>(lStream, buzzerTx_);
		std::string lBuzzerTxHex = HexStr(lStream.begin(), lStream.end());
		workingSettings_.write("buzzerTx", lBuzzerTxHex);
		workingSettings_.remove("buzzerUtxo");
		// 
		cachedWorkingSettings_.erase("buzzerTx");
		cachedWorkingSettings_["buzzerTx"] = lBuzzerTxHex;
		cachedWorkingSettings_.erase("buzzerUtxo");
		//
		buzzerUtxo_.clear();

		requestProcessor_->clearDApps();
		requestProcessor_->addDAppInstance(State::DAppInstance("buzzer", buzzerTx_->id()));

		instanceChanged_(buzzerTx_->id());

		writeWorkingSetings();

		return true;
	}

	void proposeDAppInstance() {
		//
		requestProcessor_->addDAppInstance(State::DAppInstance("buzzer", uint256()));
	}

	bool writeBuzzerInfoTx(TxBuzzerInfoPtr buzzerInfoTx) {
		//
		buzzerInfoTx_ = buzzerInfoTx;
		// load buzzer tx
		if (!open()) return false;

		// save our buzzer
		DataStream lStream(SER_DISK, PROTOCOL_VERSION);
		Transaction::Serializer::serialize<DataStream>(lStream, buzzerInfoTx_);
		std::string lBuzzerTxHex = HexStr(lStream.begin(), lStream.end());
		workingSettings_.write("buzzerInfoTx", lBuzzerTxHex);

		//
		cachedWorkingSettings_.erase("buzzerInfoTx");
		cachedWorkingSettings_["buzzerInfoTx"] = lBuzzerTxHex;

		buzzer_->pushBuzzerInfo(buzzerInfoTx_);

		writeWorkingSetings();

		return true;
	}

	bool writeBuzzerUtxo(const std::vector<Transaction::UnlinkedOut>& utxo) {
		//
		buzzerUtxo_ = utxo;
		// load buzzer tx
		if (!open()) return false;

		// save our buzzer
		DataStream lStream(SER_DISK, PROTOCOL_VERSION);
		lStream << buzzerUtxo_;
		std::string lBuzzerUtxoHex = HexStr(lStream.begin(), lStream.end());
		workingSettings_.write("buzzerUtxo", lBuzzerUtxoHex);

		//
		cachedWorkingSettings_.erase("buzzerUtxo");
		cachedWorkingSettings_["buzzerUtxo"] = lBuzzerUtxoHex;

		writeWorkingSetings();

		return true;
	}

	TxBuzzerPtr buzzerTx() { open(); return buzzerTx_; }
	TxBuzzerInfoPtr buzzerInfoTx() { open(); return buzzerInfoTx_; }

	uint256 buzzerId() {
		//
		buzzerTx();
		if (buzzerTx_) {
			return buzzerTx_->id();
		}

		return uint256();
	}

	uint256 buzzerChain() {
		//
		buzzerTx();
		if (buzzerTx_) {
			Transaction::In& lShardIn = *(++(buzzerTx_->in().begin())); // second in
			return lShardIn.out().tx();			
		}

		return uint256();
	}

	std::vector<Transaction::UnlinkedOut>& buzzerUtxo() { open(); return buzzerUtxo_; }

	void setInstanceChangedCallback(instanceChangedFunction function) {
		instanceChanged_ = function;
	}

	void writeWorkingSetings();
	void checkWorkingSettings();

	void writeSubscription(const uint256&, const PKey&);
	void writeSubscriptions();
	void checkSubscriptions();

	void writeCounterparty(const uint256&, const PKey&);
	bool getCounterparty(const uint256&, PKey&);

	bool getSubscription(const uint256&, PKey&);

	void addContact(const std::string& buzzer, const std::string& address) {
		contacts_.write(buzzer, address);
	}

	void removeContact(const std::string& buzzer) {
		contacts_.remove(buzzer);
	}

	void selectContacts(std::map<std::string, std::string>& contacts) {
		//
		db::DbContainer<std::string /*buzzer*/, std::string /*packed-pkey*/>::Iterator lItem = contacts_.begin();
		for (; lItem.valid(); ++lItem) {
			//
			std::string lBuzzer;
			if (lItem.first(lBuzzer)) {
				contacts[lBuzzer] = *lItem;
			}
		}
	}

	void checkSubscription(const uint256& /*chain*/, const uint256& /*publisher*/);
	void subscriptionLoaded(TransactionPtr /*subscription*/);
	void publisherLoaded(TransactionPtr /*publisher*/);
	void timeout() {}

private:
	// various settings, command line args & config file
	ISettingsPtr settings_;
	//
	BuzzerPtr buzzer_;
	// wallet
	IWalletPtr wallet_;
	// buzzer tx
	TxBuzzerPtr buzzerTx_;
	// buzzer info tx
	TxBuzzerInfoPtr buzzerInfoTx_;
	// utxos for buzzer tx
	std::vector<Transaction::UnlinkedOut> buzzerUtxo_;
	// request processor
	IRequestProcessorPtr requestProcessor_;
	// special processor
	BuzzerRequestProcessorPtr buzzerRequestProcessor_;
	// instance changed function
	instanceChangedFunction instanceChanged_;

	// persistent settings
	db::DbContainer<std::string /*name*/, std::string /*data*/> workingSettings_;
	// subscriptions
	db::DbContainer<uint256 /*publisher*/, PKey /*pubkey*/> subscriptions_;
	// contacts
	db::DbContainer<std::string /*buzzer*/, std::string /*packed-pkey*/> contacts_;

	// flag
	bool opened_ = false;

	// daemon mode
	std::map<std::string /*name*/, std::string /*data*/> cachedWorkingSettings_;
	std::map<uint256 /*publisher*/, PKey /*pubkey*/> cachedSubscriptions_;
	std::map<uint256 /*publisher*/, bool> absentSubscriptions_;
	uint64_t workingSettingsTime_ = 0;
	uint64_t subscriptionTime_ = 0;
	boost::recursive_mutex cacheMutex_;
};

} // qbit

#endif
