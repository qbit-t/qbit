#include "buzzercommands.h"

#if defined(CUBIX_MOD)
#include "../cubix/cubixcommands.h"
#endif

#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace qbit;

//
// CreateBuzzerCommand
//
void CreateBuzzerCommand::process(const std::vector<std::string>& args) {
	//
	args_ = args;
	buzzerOut_ = nullptr;

	if (args.size() >= 3) {
		// prepare
		IComposerMethodPtr lCommander = BuzzerLightComposer::CreateTxBuzzer::instance(composer_, 
			std::string(args[0]),
			boost::bind(&CreateBuzzerCommand::buzzerCreated, shared_from_this(), _1, _2));
		// async process
		lCommander->process(boost::bind(&CreateBuzzerCommand::error, shared_from_this(), _1, _2));
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
		return;
	}
}

void CreateBuzzerCommand::buzzerCreated(TransactionContextPtr ctx, Transaction::UnlinkedOutPtr buzzerOut) {
	//
	buzzerOut_ = buzzerOut;
	buzzerTx_ = ctx->tx();
	//
	if (!composer_->requestProcessor()->sendTransaction(ctx,
			SentTransaction::instance(
				boost::bind(&CreateBuzzerCommand::buzzerSent, shared_from_this(), _1, _2),
				boost::bind(&CreateBuzzerCommand::timeout, shared_from_this())))) {
		composer_->wallet()->resetCache();
		composer_->wallet()->prepareCache();
		error("E_TX_NOT_SENT", "Transaction was not sent");
	}
}

void CreateBuzzerCommand::buzzerSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
	//
	if (errors.size()) {
		for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
				lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
			error("E_CREATE_BUZZER", lError->data());
		}
	} else {
		std::cout << "     buzzer: " << tx.toHex() << std::endl;
	}

	//	
#if defined(CUBIX_MOD)
	if (args_.size() > 3 && uploadAvatar_) {
		//
		// upload avatar
		std::vector<std::string> lArgs;
		lArgs.push_back(args_[3]);
		if (args_.size() > 6 && args_[4] == "-s") {
			lArgs.push_back(args_[4]);
			lArgs.push_back(args_[5]);
		}
		uploadAvatar_->process(lArgs);
	} else {
		createBuzzerInfo();
	}
#else
	createBuzzerInfo();
#endif
}

void CreateBuzzerCommand::avatarUploaded(TransactionPtr tx, const ProcessingError& err) {
	//
	if (tx && uploadHeader_) {
		//
		avatarTx_ = tx;

		// upload header
		std::vector<std::string> lArgs;
		if (args_.size() > 6) {
			lArgs.push_back(args_[6]);
		} else if (args_.size() > 4) {
			lArgs.push_back(args_[4]);
		}

		uploadHeader_->process(lArgs);
		return;
	} else if (!tx) {
		error(err.error(), err.message());
		return;
	}

	createBuzzerInfo();
}

void CreateBuzzerCommand::headerUploaded(TransactionPtr tx, const ProcessingError& err) {
	if (tx) {
		//
		headerTx_ = tx;
	} else {
		error(err.error(), err.message());
		return;
	}

	createBuzzerInfo();
}

void CreateBuzzerCommand::createBuzzerInfo() {
	//
	BuzzerMediaPointer lAvatar;
	BuzzerMediaPointer lHeader;

	if (avatarTx_) lAvatar = BuzzerMediaPointer(avatarTx_->chain(), avatarTx_->id());
	if (headerTx_) lHeader = BuzzerMediaPointer(headerTx_->chain(), headerTx_->id());

	// prepare
	IComposerMethodPtr lCommanderInfo = BuzzerLightComposer::CreateTxBuzzerInfo::instance(
		composer_, buzzerOut_, args_[1], args_[2], lAvatar, lHeader,
		boost::bind(&CreateBuzzerCommand::buzzerInfoCreated, shared_from_this(), _1));
	// async process
	lCommanderInfo->process(boost::bind(&CreateBuzzerCommand::error, shared_from_this(), _1, _2));
}

void CreateBuzzerCommand::buzzerInfoCreated(TransactionContextPtr ctx) {
	//
	buzzerInfoTx_ = ctx->tx();
	//
	if (!composer_->requestProcessor()->sendTransaction(ctx->tx()->chain(), ctx, 
			SentTransaction::instance(
				boost::bind(&CreateBuzzerCommand::buzzerInfoSent, shared_from_this(), _1, _2),
				boost::bind(&CreateBuzzerCommand::timeout, shared_from_this())))) {
		composer_->wallet()->resetCache();
		composer_->wallet()->prepareCache();
		error("E_TX_INFO_NOT_SENT", "Transaction was not sent");
	}
}

void CreateBuzzerCommand::buzzerInfoSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
	//
	if (errors.size()) {
		for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
				lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
			error("E_CREATE_BUZZER_INFO", lError->data());
		}

		return;			
	}

	std::cout << "buzzer info: " << tx.toHex() << std::endl;
	done_(buzzerTx_, buzzerInfoTx_, ProcessingError());
}

//
// CreateBuzzCommand
//
void CreateBuzzCommand::process(const std::vector<std::string>& args) {
	if (args.size() >= 1) {
		// reset
		feeSent_ = false;
		buzzSent_ = false;
		mediaFiles_.clear();
		buzzers_.clear();

		body_ = args[0];

		if (args.size() > 1) {
			//
			for (std::vector<std::string>::const_iterator lArg = ++args.begin(); lArg != args.end(); lArg++) {
				if ((*lArg)[0] == '@') buzzers_.push_back(*lArg);
				else mediaFiles_.push_back(*lArg);
			}
		}

		if (mediaFiles_.size() && uploadMedia_) {
			uploadNextMedia();
		} else {
			createBuzz();
		}
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments.");
	}
}

void CreateBuzzCommand::createBuzz() {
	// prepare
	IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzz::instance(composer_, body_, buzzers_, mediaPointers_,
		boost::bind(&CreateBuzzCommand::created, shared_from_this(), _1));
	// async process
	lCommand->process(boost::bind(&CreateBuzzCommand::error, shared_from_this(), _1, _2));
}

void CreateBuzzCommand::uploadNextMedia() {
	//
	if (mediaFiles_.size() && uploadMedia_) {
		std::string lFile = *mediaFiles_.begin();
		mediaFiles_.erase(mediaFiles_.begin());

		std::vector<std::string> lArgs; lArgs.push_back(lFile);
		uploadMedia_->process(lArgs);
	}	
}

void CreateBuzzCommand::mediaUploaded(TransactionPtr tx, const ProcessingError& err) {
	//
	if (!tx) {
		error(err.error(), err.message());
		return;
	}

	if (mediaUploaded_) mediaUploaded_(tx, err);

	mediaPointers_.push_back(BuzzerMediaPointer(tx->chain(), tx->id()));
	
	// pop next
	if (mediaFiles_.size() && uploadMedia_) {
		uploadNextMedia();
	} else {
		createBuzz();
	}
}

//
// BuzzerSubscribeCommand
//
void BuzzerSubscribeCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 1) {
		// prepare
		IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzerSubscribe::instance(composer_, args[0],
			boost::bind(&BuzzerSubscribeCommand::created, shared_from_this(), _1));
		// async process
		lCommand->process(boost::bind(&BuzzerSubscribeCommand::error, shared_from_this(), _1, _2));
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments.");
	}
}

//
// BuzzerUnsubscribeCommand
//
void BuzzerUnsubscribeCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 1) {
		// prepare
		IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzerUnsubscribe::instance(composer_, args[0],
			boost::bind(&BuzzerUnsubscribeCommand::created, shared_from_this(), _1));
		// async process
		lCommand->process(boost::bind(&BuzzerUnsubscribeCommand::error, shared_from_this(), _1, _2));
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments.");
	}
}

//
// LoadHashTagsCommand
//
void LoadHashTagsCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();
	feed_.clear();

	// args - from
	uint64_t lTimeframeFrom = 0;
	uint64_t lScoreFrom = 0;
	uint160 lPublisherTs;
	BuzzfeedItemPtr lLast = nullptr;

	if (args.size() == 1) {
		tag_ = args[0];
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments.");
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(composer_->dAppName(), chains_);

	// 
	if (!chains_.size()) {
		error("E_CHAINS_ABSENT", "Chains is absent. Requesting state...");

		composer_->requestProcessor()->requestState();
	}

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadHashTags::instance(
			composer_, 
			*lChain, 
			tag_,
			2 /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadHashTagsCommand::tagsLoaded, shared_from_this(), _1, _2, _3));
		// async process
		lCommand->process(boost::bind(&LoadHashTagsCommand::error, shared_from_this(), _1, _2));
	}
}

void LoadHashTagsCommand::tagsLoaded(const std::vector<Buzzer::HashTag>& feed, const uint256& chain, int requests) {
	//
	loaded_[chain] = loaded_[chain] + 1;
	std::cout << strprintf("chain: %s/%d, n = %d/%d/%d", chain.toHex(), feed.size(), loaded_.size(), loaded_[chain], requests) << std::endl;

	//
	int lReady = 0;
	for (std::map<uint256, int>::iterator lChainLoaded = loaded_.begin(); lChainLoaded != loaded_.end(); lChainLoaded++) {
		lReady += lChainLoaded->second;
	}

	// for real client - mobile, for example, we can start to display feeds as soon as they arrived
	if (lReady < requests * chains_.size()) {
		// merge feed
		merge(feed, chain);
	} else {
		// merge feed
		merge(feed, chain);
		// display
		display();
		// done
		std::vector<Buzzer::HashTag> lTags;
		std::map<uint160, Buzzer::HashTag> lUniqueTags;

		for (std::map<uint256, std::map<uint160, Buzzer::HashTag>>::iterator lItem = feed_.begin(); lItem != feed_.end(); lItem++) {
			for (std::map<uint160, Buzzer::HashTag>::iterator lTagsSet = lItem->second.begin(); lTagsSet != lItem->second.end(); lTagsSet++) {
				lUniqueTags[lTagsSet->first] = lTagsSet->second;
			}
		}

		for (std::map<uint160, Buzzer::HashTag>::iterator lTagsSet = lUniqueTags.begin(); lTagsSet != lUniqueTags.end(); lTagsSet++) {
			lTags.push_back(lTagsSet->second);
		}

		done_(tag_, lTags, ProcessingError());
	}
}

//
// LoadBuzzfeedByTagCommand
//
void LoadBuzzfeedByTagCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localBuzzFeed_->clear();
	pendingChainInfosLoaded_ = 0;

	// args - from
	std::string lTag;
	last_.clear();

	if (args.size() >= 1) {
		//
		lTag = args[0];
		if (args.size() > 1 && args[1] == "more") {
			//
			buzzFeed_->collectLastItemsByChains(last_);
		}
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(composer_->dAppName(), chains_);

	// 
	if (!chains_.size()) {
		error("E_CHAINS_ABSENT", "Chains is absent. Requesting state...");

		composer_->requestProcessor()->requestState();
	}

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		uint64_t lTimeframeFrom = 0;
		uint64_t lScoreFrom = 0;
		uint64_t lTimestampFrom = 0;
		uint256 lPublisher;

		std::map<uint256, BuzzfeedItemPtr>::iterator lLast = last_.find(*lChain);
		if (lLast != last_.end()) {
			lTimeframeFrom = lLast->second->actualTimeframe();
			lScoreFrom = lLast->second->actualScore();
			lTimestampFrom = lLast->second->actualTimestamp();
			lPublisher = lLast->second->actualPublisher();
		}

		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadBuzzesByTag::instance(
			composer_, 
			*lChain, 
			lTag, lTimeframeFrom, lScoreFrom, lTimestampFrom, lPublisher,
			2 /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadBuzzfeedByTagCommand::buzzfeedLoaded, shared_from_this(), _1, _2, _3));
		// async process
		lCommand->process(boost::bind(&LoadBuzzfeedByTagCommand::error, shared_from_this(), _1, _2));
	}
}

//
// LoadBuzzesGlobalCommand
//
void LoadBuzzesGlobalCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localBuzzFeed_->clear();
	pendingChainInfosLoaded_ = 0;

	// args - from
	last_.clear();

	if (args.size() == 1) {
		if (args[0] == "more") {
			//
			buzzFeed_->collectLastItemsByChains(last_);
		}
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(composer_->dAppName(), chains_);

	// 
	if (!chains_.size()) {
		error("E_CHAINS_ABSENT", "Chains is absent. Requesting state...");

		composer_->requestProcessor()->requestState();
	}

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		uint64_t lTimeframeFrom = 0;
		uint64_t lScoreFrom = 0;
		uint64_t lTimestampFrom = 0;
		uint256 lPublisher;

		std::map<uint256, BuzzfeedItemPtr>::iterator lLast = last_.find(*lChain);
		if (lLast != last_.end()) {
			lTimeframeFrom = lLast->second->actualTimeframe();
			lScoreFrom = lLast->second->actualScore();
			lTimestampFrom = lLast->second->actualTimestamp();
			lPublisher = lLast->second->actualPublisher();
		}

		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadBuzzesGlobal::instance(
			composer_, 
			*lChain, 
			lTimeframeFrom, lScoreFrom, lTimestampFrom, lPublisher,
			2 /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadBuzzesGlobalCommand::buzzfeedLoaded, shared_from_this(), _1, _2, _3));
		// async process
		lCommand->process(boost::bind(&LoadBuzzesGlobalCommand::error, shared_from_this(), _1, _2));
	}
}

//
// LoadBuzzfeedByBuzzerCommand
//
void LoadBuzzfeedByBuzzerCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localBuzzFeed_->clear();
	pendingChainInfosLoaded_ = 0;
	//buzzFeed_->clear();

	std::string lBuzzer;
	// args - from
	from_.clear();
	if (args.size() >= 1) {
		//
		lBuzzer = args[0];

		//
		if (args.size() > 1 && args[1] == "more") {
			//
			buzzFeed_->locateLastTimestamp(from_);
		}
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(composer_->dAppName(), chains_);

	// 
	if (!chains_.size()) {
		error("E_CHAINS_ABSENT", "Chains is absent. Requesting state...");

		composer_->requestProcessor()->requestState();
	}

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		std::vector<BuzzfeedPublisherFrom> lFrom;
		std::map<uint256 /*chain*/, std::vector<BuzzfeedPublisherFrom>>::iterator lFromChain = from_.find(*lChain);
		if (lFromChain != from_.end())
			lFrom = lFromChain->second;

		//
		if (!lFrom.size()) continue;
		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadBuzzesByBuzzer::instance(
			composer_, 
			*lChain, 
			lFrom,
			lBuzzer,
			2 /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadBuzzfeedByBuzzerCommand::buzzfeedLoaded, shared_from_this(), _1, _2, _3));
		// async process
		lCommand->process(boost::bind(&LoadBuzzfeedByBuzzerCommand::error, shared_from_this(), _1, _2));
	}
}

//
// LoadBuzzfeedByBuzzCommand
//
void LoadBuzzfeedByBuzzCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localBuzzFeed_->clear();
	pendingChainInfosLoaded_ = 0;
	//buzzFeed_->clear();

	uint256 lBuzzId;
	// args - from
	from_ = 0; // most recent
	if (args.size() >= 1) {
		//
		lBuzzId.setHex(args[0]);

		//
		if (args.size() > 1 && args[1] == "more") {
			//
			from_ = buzzFeed_->locateLastTimestamp();
		}
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(composer_->dAppName(), chains_);

	// 
	if (!chains_.size()) {
		error("E_CHAINS_ABSENT", "Chains is absent. Requesting state...");

		composer_->requestProcessor()->requestState();
	}

	if (gLog().isEnabled(Log::CLIENT))
		gLog().write(Log::CLIENT, strprintf("[LoadBuzzfeedByBuzzCommand]: %d / %s",
										from_, lBuzzId.toHex()));

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadBuzzesByBuzz::instance(
			composer_, 
			*lChain, 
			from_,
			lBuzzId,
			2 /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadBuzzfeedByBuzzCommand::buzzfeedLoaded, shared_from_this(), _1, _2, _3));
		// async process
		lCommand->process(boost::bind(&LoadBuzzfeedByBuzzCommand::error, shared_from_this(), _1, _2));
	}
}

//
// LoadBuzzfeedCommand
//
void LoadBuzzfeedCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localBuzzFeed_->clear();
	pendingChainInfosLoaded_ = 0;

	// args - from
	from_.clear();
	if (args.size() == 1) {
		if (args[0] == "more") {
			//
			buzzFeed_->locateLastTimestamp(from_);
		}
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(composer_->dAppName(), chains_);

	// 
	if (!chains_.size()) {
		error("E_CHAINS_ABSENT", "Chains is absent. Requesting state...");

		composer_->requestProcessor()->requestState();
	}

	/*
	for (std::vector<BuzzfeedPublisherFrom>::iterator lItem = from_.begin(); lItem != from_.end(); lItem++) 
		if (gLog().isEnabled(Log::CLIENT))
			gLog().write(Log::CLIENT, strprintf("[LoadBuzzfeedCommand]: %d / %s",
											lItem->from(), lItem->publisher().toHex()));
	*/

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		std::vector<BuzzfeedPublisherFrom> lFrom;
		std::map<uint256 /*chain*/, std::vector<BuzzfeedPublisherFrom>>::iterator lFromChain = from_.find(*lChain);
		if (lFromChain != from_.end())
			lFrom = lFromChain->second;

		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadBuzzfeed::instance(
			composer_, 
			*lChain, 
			lFrom, 
			2 /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadBuzzfeedCommand::buzzfeedLoaded, shared_from_this(), _1, _2, _3));
		// async process
		lCommand->process(boost::bind(&LoadBuzzfeedCommand::error, shared_from_this(), _1, _2));
	}
}

void LoadBuzzfeedCommand::display(const std::vector<BuzzfeedItemPtr>& feed) {
	//
	for (std::vector<BuzzfeedItemPtr>::const_iterator lBuzz = feed.begin(); lBuzz != feed.end(); lBuzz++) {
		std::cout << (*lBuzz)->toString() << std::endl << std::endl;
	}
}

void LoadBuzzfeedCommand::buzzesLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	//
	pendingLoaded_.insert(chain);
	std::cout << strprintf("pending, chain: %s/%d, n = %d", chain.toHex(), feed.size(), pendingLoaded_.size()) << std::endl;
	if (gLog().isEnabled(Log::CLIENT))
		gLog().write(Log::CLIENT, strprintf("pending, chain: %s/%d, n = %d", chain.toHex(), feed.size(), pendingLoaded_.size()));

	if (pendingLoaded_.size() < pending_.size()) {
		// merge feed
		buzzfeed()->merge(feed);
	} else {
		// merge and notify
		buzzfeed()->merge(feed, true);
		//
		processPengingInfos();
	}
}

void LoadBuzzfeedCommand::buzzfeedLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain, int requests) {
	//
	loaded_[chain] = loaded_[chain] + 1;
	std::cout << strprintf("chain: %s/%d, n = %d/%d/%d", chain.toHex(), feed.size(), loaded_.size(), loaded_[chain], requests) << std::endl;
	if (gLog().isEnabled(Log::CLIENT))
		gLog().write(Log::CLIENT, strprintf("chain: %s/%d, n = %d/%d/%d", chain.toHex(), feed.size(), loaded_.size(), loaded_[chain], requests));

	//
	int lReady = 0;
	for (std::map<uint256, int>::iterator lChainLoaded = loaded_.begin(); lChainLoaded != loaded_.end(); lChainLoaded++) {
		lReady += lChainLoaded->second;
	}

	// for real client - mobile, for example, we can start to display feeds as soon as they arrived
	if (lReady < requests * chains_.size()) {
		// merge feed
		buzzfeed()->merge(feed);
	} else {
		// merge and notify
		buzzfeed()->merge(feed, true);
		// cross-merge in case pending items arrived
		buzzfeed()->crossMerge();
		// if we have postponed items, request missing
		buzzfeed()->collectPendingItems(pending_);

		if (pending_.size()) {
			//
			bool lProcessed = false;
			for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pending_.begin(); lChain != pending_.end(); lChain++) {
				//
				std::vector<uint256> lBuzzes(lChain->second.begin(), lChain->second.end());
				if (!composer_->buzzerRequestProcessor()->selectBuzzes(lChain->first, lBuzzes, 
					SelectBuzzFeed::instance(
						boost::bind(&LoadBuzzfeedCommand::buzzesLoaded, shared_from_this(), _1, _2),
						boost::bind(&LoadBuzzfeedCommand::timeout, shared_from_this()))
				)) { 
					warning("E_LOAD_PENDING_BUZZFEED", "Buzzfeed pending items loading failed."); 
				} else lProcessed = true; // at least 1
			}

			if (!lProcessed) {
				processPengingInfos();
			}
		} else {
			//
			processPengingInfos();
		}
	}
}

void LoadBuzzfeedCommand::processPengingInfos() {
	//
	pendingInfos_ = buzzFeed_->buzzer()->pendingInfos(); // collect
	buzzFeed_->buzzer()->collectPengingInfos(pengindChainInfos_); // prepare

	if (pengindChainInfos_.size()) {
		//
		bool lProcessed = false;
		for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pengindChainInfos_.begin(); lChain != pengindChainInfos_.end(); lChain++) {
			//
			std::vector<uint256> lInfos(lChain->second.begin(), lChain->second.end());
			if (!composer_->requestProcessor()->loadTransactions(lChain->first, lInfos,
				LoadTransactions::instance(
					boost::bind(&LoadBuzzfeedCommand::buzzerInfoLoaded, shared_from_this(), _1),
					boost::bind(&LoadBuzzfeedCommand::timeout, shared_from_this()))
			)) { 
				warning("E_LOAD_PENDING_BUZZER_INFOS", "Buzzer infos failed to load."); 
				pendingChainInfosLoaded_++;
			} else lProcessed = true; // at least 1
		}

		if (!lProcessed) {
			show();
		}
	} else {
		show();
	}
}

void LoadBuzzfeedCommand::buzzerInfoLoaded(const std::vector<TransactionPtr>& txs) {
	//
	pendingChainInfosLoaded_++;
	//
	if (txs.size()) {
		//
		for (std::vector<TransactionPtr>::const_iterator lTx = txs.begin(); lTx != txs.end(); lTx++) {
			//
			TxBuzzerInfoPtr lInfo = TransactionHelper::to<TxBuzzerInfo>(*lTx);
			//
			std::map<uint256 /*info tx*/, Buzzer::Info>::iterator lItem = pendingInfos_.find(lInfo->id());
			if (lItem != pendingInfos_.end() && lInfo->verifySignature() && // signature and ...
					lInfo->in()[TX_BUZZER_INFO_MY_IN].out().tx() == lItem->second.id()) // ... expected buzzer is match
				buzzFeed_->buzzer()->pushBuzzerInfo(lInfo);
		}
	}

	if (pendingChainInfosLoaded_ >= pengindChainInfos_.size()) {
		show();
	}
}

void LoadBuzzfeedCommand::show() {
	//
	if (buzzfeedReady_) {
		//
		buzzfeedReady_(buzzFeed_, localBuzzFeed_);
	} else {
		// get list
		std::vector<BuzzfeedItemPtr> lFeed;
		// if 'more' provided
		if (appendFeed()) {
			std::vector<BuzzfeedItemPtr> lUpFeed;
			localBuzzFeed_->feed(lUpFeed);
			buzzFeed_->mergeAppend(lUpFeed);
		}

		buzzFeed_->feed(lFeed);

		// display
		std::cout << std::endl;
		display(lFeed);
		//
	}

	done_(ProcessingError());
}

//
// LoadEndorsementsByBuzzerCommand
//
void LoadEndorsementsByBuzzerCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localEventsFeed_->clear();
	pendingChainInfosLoaded_ = 0;
	eventsFeed_->clear();

	// args - from
	fromBuzzer_.setNull();
	EventsfeedItemPtr lLast = nullptr;
	uint256 lBuzzerId;
	std::string lBuzzer;

	if (args.size() >= 1) {
		//
		if (*args[0].begin() == '@') lBuzzer = args[0];
		else lBuzzerId.setHex(args[0]);

		//
		if (args.size() > 1 && args[1] == "more") {
			//
			lLast = eventsFeed_->last();
			if (lLast) fromBuzzer_ = lLast->buzzerId();
		}
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(composer_->dAppName(), chains_);

	// 
	if (!chains_.size()) {
		error("E_CHAINS_ABSENT", "Chains is absent. Requesting state...");

		composer_->requestProcessor()->requestState();
	}

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		IComposerMethodPtr lCommand;
		if (!lBuzzerId.isNull())
			lCommand = BuzzerLightComposer::LoadEndorsementsByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzerId, 
				2,
				boost::bind(&LoadEndorsementsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), _1, _2, _3));
		else
			lCommand = BuzzerLightComposer::LoadEndorsementsByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzer, 
				2,
				boost::bind(&LoadEndorsementsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), _1, _2, _3));

		// async process
		lCommand->process(boost::bind(&LoadEndorsementsByBuzzerCommand::error, shared_from_this(), _1, _2));
	}
}

//
// LoadMistrustsByBuzzerCommand
//
void LoadMistrustsByBuzzerCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localEventsFeed_->clear();
	pendingChainInfosLoaded_ = 0;
	eventsFeed_->clear();

	// args - from
	fromBuzzer_.setNull();
	EventsfeedItemPtr lLast = nullptr;
	uint256 lBuzzerId;
	std::string lBuzzer;

	if (args.size() >= 1) {
		//
		if (*args[0].begin() == '@') lBuzzer = args[0];
		else lBuzzerId.setHex(args[0]);

		//
		if (args.size() > 1 && args[1] == "more") {
			//
			lLast = eventsFeed_->last();
			if (lLast) fromBuzzer_ = lLast->buzzerId();
		}
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(composer_->dAppName(), chains_);

	// 
	if (!chains_.size()) {
		error("E_CHAINS_ABSENT", "Chains is absent. Requesting state...");

		composer_->requestProcessor()->requestState();
	}

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		IComposerMethodPtr lCommand;
		if (!lBuzzerId.isNull())
			 lCommand = BuzzerLightComposer::LoadMistrustsByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzerId,
				2,
				boost::bind(&LoadMistrustsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), _1, _2, _3));
		else
			 lCommand = BuzzerLightComposer::LoadMistrustsByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzer,
				2,
				boost::bind(&LoadMistrustsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), _1, _2, _3));

		// async process
		lCommand->process(boost::bind(&LoadMistrustsByBuzzerCommand::error, shared_from_this(), _1, _2));
	}
}

//
// LoadSubscriptionsByBuzzerCommand
//
void LoadSubscriptionsByBuzzerCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localEventsFeed_->clear();
	pendingChainInfosLoaded_ = 0;
	eventsFeed_->clear();

	// args - from
	fromBuzzer_.setNull();
	EventsfeedItemPtr lLast = nullptr;
	uint256 lBuzzerId;
	std::string lBuzzer;

	if (args.size() >= 1) {
		//
		if (*args[0].begin() == '@') lBuzzer = args[0];
		else lBuzzerId.setHex(args[0]);

		//
		if (args.size() > 1 && args[1] == "more") {
			//
			lLast = eventsFeed_->last();
			if (lLast) fromBuzzer_ = lLast->buzzerId();
		}
	} else {
		lBuzzerId = composer_->buzzerId();
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(composer_->dAppName(), chains_);

	// 
	if (!chains_.size()) {
		error("E_CHAINS_ABSENT", "Chains is absent. Requesting state...");

		composer_->requestProcessor()->requestState();
	}

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		IComposerMethodPtr lCommand;
		if (!lBuzzerId.isNull())
			lCommand = BuzzerLightComposer::LoadSubscriptionsByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzerId,
				2,
				boost::bind(&LoadSubscriptionsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), _1, _2, _3));
		else
			lCommand = BuzzerLightComposer::LoadSubscriptionsByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzer,
				2,
				boost::bind(&LoadSubscriptionsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), _1, _2, _3));

		// async process
		lCommand->process(boost::bind(&LoadSubscriptionsByBuzzerCommand::error, shared_from_this(), _1, _2));
	}
}

//
// LoadFollowersByBuzzerCommand
//
void LoadFollowersByBuzzerCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localEventsFeed_->clear();
	pendingChainInfosLoaded_ = 0;
	eventsFeed_->clear();

	// args - from
	fromBuzzer_.setNull();
	EventsfeedItemPtr lLast = nullptr;
	uint256 lBuzzerId;
	std::string lBuzzer;

	if (args.size() >= 1) {
		//
		if (*args[0].begin() == '@') lBuzzer = args[0];
		else lBuzzerId.setHex(args[0]);

		//
		if (args.size() > 1 && args[1] == "more") {
			//
			lLast = eventsFeed_->last();
			if (lLast) fromBuzzer_ = lLast->buzzerId();
		}
	} else {
		lBuzzerId = composer_->buzzerId();
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(composer_->dAppName(), chains_);

	// 
	if (!chains_.size()) {
		error("E_CHAINS_ABSENT", "Chains is absent. Requesting state...");

		composer_->requestProcessor()->requestState();
	}

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		IComposerMethodPtr lCommand;
		if (!lBuzzerId.isNull())
			lCommand = BuzzerLightComposer::LoadFollowersByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzerId,
				2,
				boost::bind(&LoadFollowersByBuzzerCommand::eventsfeedLoaded, shared_from_this(), _1, _2, _3));
		else
			lCommand = BuzzerLightComposer::LoadFollowersByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzer,
				2,
				boost::bind(&LoadFollowersByBuzzerCommand::eventsfeedLoaded, shared_from_this(), _1, _2, _3));

		// async process
		lCommand->process(boost::bind(&LoadFollowersByBuzzerCommand::error, shared_from_this(), _1, _2));
	}
}

//
// LoadEventsfeedCommand
//
void LoadEventsfeedCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localEventsFeed_->clear();
	pendingChainInfosLoaded_ = 0;
	from_ = 0;

	// args - from
	from_ = 0; // most recent
	if (args.size() == 1) {
		if (args[0] == "more") {
			//
			from_ = eventsFeed_->locateLastTimestamp();
		}
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(composer_->dAppName(), chains_);

	// 
	if (!chains_.size()) {
		error("E_CHAINS_ABSENT", "Chains is absent. Requesting state...");

		composer_->requestProcessor()->requestState();
	}

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadEventsfeed::instance(composer_, *lChain, from_, 2,
			boost::bind(&LoadEventsfeedCommand::eventsfeedLoaded, shared_from_this(), _1, _2, _3));
		// async process
		lCommand->process(boost::bind(&LoadEventsfeedCommand::error, shared_from_this(), _1, _2));
	}
}

void LoadEventsfeedCommand::display(EventsfeedItemPtr item) {
	//
	std::vector<EventsfeedItemPtr> lFeed;
	item->feed(lFeed);

	for (std::vector<EventsfeedItemPtr>::iterator lEvent = lFeed.begin(); lEvent != lFeed.end(); lEvent++) {
		std::cout << (*lEvent)->toString() << std::endl << std::endl;
	}
}

void LoadEventsfeedCommand::buzzesLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	//
	pendingLoaded_.insert(chain);
	std::cout << strprintf("pending, chain: %s/%d, n = %d", chain.toHex(), feed.size(), pendingLoaded_.size()) << std::endl;

	if (pendingLoaded_.size() < pending_.size()) {
		// merge feed
		eventsfeed()->merge(feed);
	} else {
		// merge and notify
		eventsfeed()->merge(feed);
		// process pending info
		processPengingInfos();
	}
}

void LoadEventsfeedCommand::eventsfeedLoaded(const std::vector<EventsfeedItem>& feed, const uint256& chain, int requests) {
	//
	loaded_[chain] = loaded_[chain] + 1;
	std::cout << strprintf("chain: %s/%d, n = %d/%d/%d", chain.toHex(), feed.size(), loaded_.size(), loaded_[chain], requests) << std::endl;

	//
	int lReady = 0;
	for (std::map<uint256, int>::iterator lChainLoaded = loaded_.begin(); lChainLoaded != loaded_.end(); lChainLoaded++) {
		lReady += lChainLoaded->second;
	}

	// for real client - mobile, for example, we can start to display feeds as soon as they arrived
	if (lReady < requests * chains_.size()) {
		// merge feed
		eventsfeed()->merge(feed);
	} else {
		// merge and notify
		eventsfeed()->merge(feed, true);
		// if we have postponed items, request missing
		eventsfeed()->collectPendingItems(pending_);

		if (pending_.size()) {
			for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pending_.begin(); lChain != pending_.end(); lChain++) {
				//
				std::vector<uint256> lBuzzes(lChain->second.begin(), lChain->second.end());
				if (!composer_->buzzerRequestProcessor()->selectBuzzes(lChain->first, lBuzzes, 
					SelectBuzzFeed::instance(
						boost::bind(&LoadEventsfeedCommand::buzzesLoaded, shared_from_this(), _1, _2),
						boost::bind(&LoadEventsfeedCommand::timeout, shared_from_this()))
				)) error("E_LOAD_PENDING_EVENTSFEED", "Eventsfeed pending items loading failed.");	
			}
		} else {
			processPengingInfos();
		}
	}
}

void LoadEventsfeedCommand::processPengingInfos() {
	//
	pendingInfos_ = eventsFeed_->buzzer()->pendingInfos(); // collect
	eventsFeed_->buzzer()->collectPengingInfos(pengindChainInfos_); // prepare

	if (pengindChainInfos_.size()) {
		for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pengindChainInfos_.begin(); lChain != pengindChainInfos_.end(); lChain++) {
			//
			std::vector<uint256> lInfos(lChain->second.begin(), lChain->second.end());
			if (!composer_->requestProcessor()->loadTransactions(lChain->first, lInfos, 
				LoadTransactions::instance(
					boost::bind(&LoadEventsfeedCommand::buzzerInfoLoaded, shared_from_this(), _1),
					boost::bind(&LoadEventsfeedCommand::timeout, shared_from_this()))
			)) error("E_LOAD_PENDING_BUZZER_INFOS", "Buzzer infos failed to load.");
		}
	} else {
		show();
	}
}

void LoadEventsfeedCommand::buzzerInfoLoaded(const std::vector<TransactionPtr>& txs) {
	//
	pendingChainInfosLoaded_++;
	//
	if (txs.size()) {
		//
		for (std::vector<TransactionPtr>::const_iterator lTx = txs.begin(); lTx != txs.end(); lTx++) {
			//
			TxBuzzerInfoPtr lInfo = TransactionHelper::to<TxBuzzerInfo>(*lTx);
			//
			std::map<uint256 /*info tx*/, Buzzer::Info>::iterator lItem = pendingInfos_.find(lInfo->id());
			if (lItem != pendingInfos_.end() && lInfo->verifySignature() && // signature and ...
					lInfo->in()[TX_BUZZER_INFO_MY_IN].out().tx() == lItem->second.id()) // ... expected buzzer is match
				eventsFeed_->buzzer()->pushBuzzerInfo(lInfo);
		}
	}

	if (pendingChainInfosLoaded_ >= pengindChainInfos_.size()) {
		show();
	}
}

void LoadEventsfeedCommand::show() {
	if (eventsfeedReady_) {
		//
		eventsfeedReady_(eventsFeed_, localEventsFeed_);
	} else {
		// get list
		std::vector<EventsfeedItemPtr> lFeed;
		// if 'more' provided
		if (appendFeed()) {
			std::vector<EventsfeedItemPtr> lUpFeed;
			localEventsFeed_->feed(lUpFeed);
			eventsFeed_->mergeAppend(lUpFeed);
		}

		eventsFeed_->feed(lFeed);
		// display
		std::cout << std::endl;
		display(eventsFeed_->toItem());
	}

	done_(ProcessingError());
}

//
// BuzzfeedListCommand
//
void BuzzfeedListCommand::display(const std::vector<BuzzfeedItemPtr>& feed) {
	//
	for (std::vector<BuzzfeedItemPtr>::const_iterator lBuzz = feed.begin(); lBuzz != feed.end(); lBuzz++) {
		std::cout << (*lBuzz)->toString() << std::endl << std::endl;
	}
}

void BuzzfeedListCommand::buzzesLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	//
	pendingLoaded_.insert(chain);
	std::cout << strprintf("pending, chain: %s/%d, n = %d", chain.toHex(), feed.size(), pendingLoaded_.size()) << std::endl;

	if (pendingLoaded_.size() < pending_.size()) {
		// merge feed
		buzzFeed_->merge(feed);
	} else {
		// merge and notify
		buzzFeed_->merge(feed, true);
		//
		processPengingInfos();
	}
}

void BuzzfeedListCommand::process(const std::vector<std::string>& args) {
	//
	pending_.clear();
	pendingLoaded_.clear();	
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	pendingChainInfosLoaded_ = 0;

	// if we have postponed items, request missing
	buzzFeed_->collectPendingItems(pending_);

	if (pending_.size()) {
		for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pending_.begin(); lChain != pending_.end(); lChain++) {
			//
			std::vector<uint256> lBuzzes(lChain->second.begin(), lChain->second.end());
			if (!composer_->buzzerRequestProcessor()->selectBuzzes(lChain->first, lBuzzes, 
				SelectBuzzFeed::instance(
					boost::bind(&BuzzfeedListCommand::buzzesLoaded, shared_from_this(), _1, _2),
					boost::bind(&BuzzfeedListCommand::timeout, shared_from_this()))
			)) error("E_LOAD_PENDING_BUZZFEED", "Buzzfeed pending items loading failed.");	
		}
	} else {
		//
		processPengingInfos();
	}
}

void BuzzfeedListCommand::processPengingInfos() {
	//
	pendingInfos_ = buzzFeed_->buzzer()->pendingInfos(); // collect
	buzzFeed_->buzzer()->collectPengingInfos(pengindChainInfos_); // prepare

	if (pengindChainInfos_.size()) {
		for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pengindChainInfos_.begin(); lChain != pengindChainInfos_.end(); lChain++) {
			//
			std::vector<uint256> lInfos(lChain->second.begin(), lChain->second.end());
			if (!composer_->requestProcessor()->loadTransactions(lChain->first, lInfos, 
				LoadTransactions::instance(
					boost::bind(&BuzzfeedListCommand::buzzerInfoLoaded, shared_from_this(), _1),
					boost::bind(&BuzzfeedListCommand::timeout, shared_from_this()))
			)) error("E_LOAD_PENDING_BUZZER_INFOS", "Buzzer infos failed to load.");	
		}
	} else {
		// show
		show();
		// we are done
		done_();
	}
}

void BuzzfeedListCommand::buzzerInfoLoaded(const std::vector<TransactionPtr>& txs) {
	//
	pendingChainInfosLoaded_++;
	//
	if (txs.size()) {
		//
		for (std::vector<TransactionPtr>::const_iterator lTx = txs.begin(); lTx != txs.end(); lTx++) {
			//
			TxBuzzerInfoPtr lInfo = TransactionHelper::to<TxBuzzerInfo>(*lTx);
			//
			std::map<uint256 /*info tx*/, Buzzer::Info>::iterator lItem = pendingInfos_.find(lInfo->id());
			if (lItem != pendingInfos_.end() && lInfo->verifySignature() && // signature and ...
					lInfo->in()[TX_BUZZER_INFO_MY_IN].out().tx() == lItem->second.id()) // ... expected buzzer is match
				buzzFeed_->buzzer()->pushBuzzerInfo(lInfo);
		}
	}

	if (pendingChainInfosLoaded_ >= pengindChainInfos_.size()) {
		// show
		show();
		// we are done
		done_();
	}
}

void BuzzfeedListCommand::show() {
	// get list
	std::vector<BuzzfeedItemPtr> lFeed;
	buzzFeed_->feed(lFeed);
	// display
	std::cout << std::endl;
	display(lFeed);
	//
	done_();
}

//
// EventsfeedListCommand
//
void EventsfeedListCommand::display(EventsfeedItemPtr item) {
	//
	std::vector<EventsfeedItemPtr> lFeed;
	item->feed(lFeed);

	for (std::vector<EventsfeedItemPtr>::iterator lEvent = lFeed.begin(); lEvent != lFeed.end(); lEvent++) {
		std::cout << (*lEvent)->toString() << std::endl << std::endl;
	}
}

void EventsfeedListCommand::buzzesLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	//
	pendingLoaded_.insert(chain);
	std::cout << strprintf("pending, chain: %s/%d, n = %d", chain.toHex(), feed.size(), pendingLoaded_.size()) << std::endl;

	if (pendingLoaded_.size() < pending_.size()) {
		// merge feed
		eventsFeed_->merge(feed);
	} else {
		// merge and notify
		eventsFeed_->merge(feed);
		// get list
		std::vector<EventsfeedItemPtr> lFeed;
		eventsFeed_->feed(lFeed);
		// display
		std::cout << std::endl;
		display(eventsFeed_->toItem());

		done_();
	}
}

void EventsfeedListCommand::process(const std::vector<std::string>& args) {
	//
	pending_.clear();
	pendingLoaded_.clear();	

	// if we have postponed items, request missing
	eventsFeed_->collectPendingItems(pending_);

	if (pending_.size()) {
		for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pending_.begin(); lChain != pending_.end(); lChain++) {
			//
			std::vector<uint256> lBuzzes(lChain->second.begin(), lChain->second.end());
			if (!composer_->buzzerRequestProcessor()->selectBuzzes(lChain->first, lBuzzes, 
				SelectBuzzFeed::instance(
					boost::bind(&EventsfeedListCommand::buzzesLoaded, shared_from_this(), _1, _2),
					boost::bind(&EventsfeedListCommand::timeout, shared_from_this()))
			)) error("E_LOAD_PENDING_BUZZFEED", "Buzzfeed pending items loading failed.");	
		}
	} else {
		//
		std::cout << std::endl;
		display(eventsFeed_->toItem());

		done_();
	}
}

//
// BuzzLikeCommand
//
void BuzzLikeCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 1) {
		// prepare
		uint256 lBuzzId;
		lBuzzId.setHex(args[0]);
		//
		BuzzfeedItemPtr lItem = buzzFeed_->locateBuzz(lBuzzId);
		//
		if (lItem) {
			IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzLike::instance(composer_, lItem->buzzChainId(), lItem->buzzId(),
				boost::bind(&BuzzLikeCommand::created, shared_from_this(), _1));
			// async process
			lCommand->process(boost::bind(&BuzzLikeCommand::error, shared_from_this(), _1, _2));
		} else {
			error("E_BUZZ_NOT_FOUND_IN_FEED", "Buzz was not found in local feed");
			return;
		}
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
		return;
	}
}

//
// BuzzRewardCommand
//
void BuzzRewardCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 2) {
		// prepare
		uint256 lBuzzId;
		lBuzzId.setHex(args[0]);

		amount_t lReward;
		if (!boost::conversion::try_lexical_convert<uint64_t>(std::string(args[1]), lReward)) {
			error("E_REWARD_AMOUNT_INVALID", "Reward amount is invalid.");
			return;
		}

		//
		BuzzfeedItemPtr lItem = buzzFeed_->locateBuzz(lBuzzId);
		//
		if (lItem) {
			//
			TxBuzzerInfoPtr lInfo = buzzFeed_->buzzer()->locateBuzzerInfo(lItem->buzzerInfoId()); 
			if (!lInfo) {
				error("E_BUZZER_INFO_NOT_FOUND", "Buzzer info was not found.");
				return;
			}
			//
			PKey lAddress;
			if(!lInfo->extractAddress(lAddress)) {
				error("E_BUZZER_ADDRESS_WAS_NOT_FOUND", "Buzzer address was not found.");
				return;
			}
			//
			IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzReward::instance(composer_, 
				lItem->buzzChainId(), lItem->buzzId(), lAddress, lReward,
				boost::bind(&BuzzRewardCommand::created, shared_from_this(), _1));
			// async process
			lCommand->process(boost::bind(&BuzzRewardCommand::error, shared_from_this(), _1, _2));
		} else {
			error("E_BUZZ_NOT_FOUND_IN_FEED", "Buzz was not found in local feed");
			return;
		}
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
		return;
	}
}

//
// CreateBuzzReplyCommand
//
void CreateBuzzReplyCommand::process(const std::vector<std::string>& args) {
	if (args.size() >= 2) {
		// reset
		feeSent_ = false;
		buzzSent_ = false;
		mediaPointers_.clear();
		mediaFiles_.clear();

		std::vector<std::string>::const_iterator lArg = args.begin();

		// prepare
		buzzId_.setHex(*(lArg++));
		body_ = *(lArg++);

		if (args.size() > 1) {
			//
			for (; lArg != args.end(); lArg++) {
				if ((*lArg)[0] == '@') buzzers_.push_back(*lArg);
				else mediaFiles_.push_back(*lArg);
			}
		}

		if (mediaFiles_.size() && uploadMedia_) {
			uploadNextMedia();
		} else {
			createBuzz();
		}
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments.");
	}
}

void CreateBuzzReplyCommand::createBuzz() {
	//
	BuzzfeedItemPtr lItem = buzzFeed_->locateBuzz(buzzId_);
	//
	if (lItem) {
		IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzReply::instance(composer_, lItem->buzzChainId(), lItem->buzzId(), body_, buzzers_, mediaPointers_,
			boost::bind(&CreateBuzzReplyCommand::created, shared_from_this(), _1));
		// async process
		lCommand->process(boost::bind(&CreateBuzzReplyCommand::error, shared_from_this(), _1, _2));
	} else {
		error("E_BUZZ_NOT_FOUND_IN_FEED", "Buzz was not found in local feed");
	}
}

void CreateBuzzReplyCommand::uploadNextMedia() {
	//
	if (mediaFiles_.size() && uploadMedia_) {
		std::string lFile = *mediaFiles_.begin();
		mediaFiles_.erase(mediaFiles_.begin());

		std::vector<std::string> lArgs; lArgs.push_back(lFile);
		uploadMedia_->process(lArgs);
	}	
}

void CreateBuzzReplyCommand::mediaUploaded(TransactionPtr tx, const ProcessingError& err) {
	//
	if (!tx) {
		error(err.error(), err.message());
		return;
	}

	if (mediaUploaded_) mediaUploaded_(tx, err);

	mediaPointers_.push_back(BuzzerMediaPointer(tx->chain(), tx->id()));
	
	// pop next
	if (mediaFiles_.size() && uploadMedia_) {
		uploadNextMedia();
	} else {
		createBuzz();
	}
}

//
// CreateReBuzzCommand
//
void CreateReBuzzCommand::process(const std::vector<std::string>& args) {
	if (args.size() >= 1) {
		// reset
		feeSent_ = false;
		buzzSent_ = false;
		mediaPointers_.clear();
		mediaFiles_.clear();

		//
		std::vector<std::string>::const_iterator lArg = args.begin();

		// prepare
		buzzId_.setHex(*(lArg++));
		if (lArg != args.end() && (*lArg)[0] != '@') body_ = *(lArg++);

		if (lArg != args.end()) {
			//
			for (; lArg != args.end(); lArg++) {
				if ((*lArg)[0] == '@') buzzers_.push_back(*lArg);
				else mediaFiles_.push_back(*lArg);
			}
		}

		if (mediaFiles_.size() && uploadMedia_) {
			uploadNextMedia();
		} else {
			createBuzz();
		}

	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

void CreateReBuzzCommand::createBuzz() {
	//
	BuzzfeedItemPtr lItem = buzzFeed_->locateBuzz(buzzId_);
	//
	if (lItem) {
		IComposerMethodPtr lCreateReBuzz = BuzzerLightComposer::CreateTxRebuzz::instance(
			composer_, lItem->buzzChainId(), lItem->buzzId(), body_, buzzers_, mediaPointers_,
			boost::bind(&CreateReBuzzCommand::created, shared_from_this(), _1));
		// async process
		lCreateReBuzz->process(boost::bind(&CreateReBuzzCommand::error, shared_from_this(), _1, _2));
	} else {
		error("E_BUZZ_NOT_FOUND_IN_FEED", "Buzz was not found in local feed");
	}
}

void CreateReBuzzCommand::uploadNextMedia() {
	//
	if (mediaFiles_.size() && uploadMedia_) {
		std::string lFile = *mediaFiles_.begin();
		mediaFiles_.erase(mediaFiles_.begin());

		std::vector<std::string> lArgs; lArgs.push_back(lFile);
		uploadMedia_->process(lArgs);
	}	
}

void CreateReBuzzCommand::mediaUploaded(TransactionPtr tx, const ProcessingError& err) {
	//
	if (!tx) {
		error(err.error(), err.message());
		return;
	}

	if (mediaUploaded_) mediaUploaded_(tx, err);

	mediaPointers_.push_back(BuzzerMediaPointer(tx->chain(), tx->id()));
	
	// pop next
	if (mediaFiles_.size() && uploadMedia_) {
		uploadNextMedia();
	} else {
		createBuzz();
	}
}

//
// LoadBuzzerTrustScoreCommand
//
void LoadBuzzerTrustScoreCommand::process(const std::vector<std::string>& args) {
	//
	uint256 lBuzzer = composer_->buzzerId();
	uint256 lBuzzerChain = composer_->buzzerChain();

	//
	if (args.size() > 0) {
		std::vector<std::string> lParts;
		boost::split(lParts, args[0], boost::is_any_of("/"));

		if (lParts.size() < 2) {
			error("E_LOAD_TRUSTSCORE", "Buzzer info is absent");
			return;	
		}

		lBuzzer.setHex(lParts[0]);
		lBuzzerChain.setHex(lParts[1]);
	}

	if (!composer_->buzzerRequestProcessor()->loadTrustScore(lBuzzerChain, lBuzzer, 
		LoadTrustScore::instance(
			boost::bind(&LoadBuzzerTrustScoreCommand::trustScoreLoaded, shared_from_this(), _1, _2, _3, _4),
			boost::bind(&LoadBuzzerTrustScoreCommand::timeout, shared_from_this()))
	)) error("E_LOAD_TRUSTSCORE", "Trust score loading failed.");	
}

//
// BuzzerEndorseCommand
//
void BuzzerEndorseCommand::process(const std::vector<std::string>& args) {
	//
	feeSent_ = false;
	endorseSent_ = false;

	if (args.size() >= 1) {
		//
		amount_t lPoints = BUZZER_MIN_EM_STEP;
		if (args.size() > 1) lPoints = (amount_t)(boost::lexical_cast<uint64_t>(args[1])); 

		// prepare
		IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzerEndorse::instance(composer_, args[0], lPoints,
			boost::bind(&BuzzerEndorseCommand::created, shared_from_this(), _1));
		// async process
		lCommand->process(boost::bind(&BuzzerEndorseCommand::error, shared_from_this(), _1, _2));
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

//
// BuzzerMistrustCommand
//
void BuzzerMistrustCommand::process(const std::vector<std::string>& args) {
	//
	feeSent_ = false;
	mistrustSent_ = false;

	if (args.size() >= 1) {
		//
		amount_t lPoints = BUZZER_MIN_EM_STEP;
		if (args.size() > 1) lPoints = (amount_t)(boost::lexical_cast<uint64_t>(args[1])); 

		// prepare
		IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzerMistrust::instance(composer_, args[0], lPoints,
			boost::bind(&BuzzerMistrustCommand::created, shared_from_this(), _1));
		// async process
		lCommand->process(boost::bind(&BuzzerMistrustCommand::error, shared_from_this(), _1, _2));
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

//
// BuzzSubscribeCommand
//
void BuzzSubscribeCommand::process(const std::vector<std::string>& args) {
	//
	chain_.setNull();
	buzzId_.setNull();
	nonce_ = 0;
	peers_.clear();

	if (args.size() == 3) {
		//
		std::vector<std::string>::const_iterator lArg = args.begin();

		// prepare
		chain_.setHex(*(lArg++));
		buzzId_.setHex(*(lArg++));
		nonce_ = (uint64_t)(boost::lexical_cast<uint64_t>(*lArg)); 

		// make signature
		DataStream lSource(SER_NETWORK, PROTOCOL_VERSION);
		lSource << buzzId_;
		lSource << nonce_;
		
		uint256 lHash = Hash(lSource.begin(), lSource.end());
		SKeyPtr lSKey = composer_->wallet()->firstKey();
		if (!lSKey->sign(lHash, signature_)) {
			error("INVALID_SIGNATURE", "Signature creation failed.");
		}

		// send
		if (!composer_->buzzerRequestProcessor()->subscribeBuzzThread(chain_, buzzId_, signature_, 2, peers_)) {
			error("E_BUZZ_SUBSCRIBE", "Subscription for buzz thread updates failed.");
			return;
		}
		//
		done_(ProcessingError());

	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

//
// BuzzUnsubscribeCommand
//
void BuzzUnsubscribeCommand::process(const std::vector<std::string>& args) {
	//
	if (args.size() >= 3) {
		//
		std::list<uint160> lPeers;
		//
		std::vector<std::string>::const_iterator lArg = args.begin();

		// prepare
		chain_.setHex(*(lArg++));
		buzzId_.setHex(*(lArg++));

		for (; lArg != args.end(); lArg++) {
			uint160 lId;
			lId.setHex(*lArg);
			lPeers.push_back(lId);
		}

		// send
		composer_->buzzerRequestProcessor()->unsubscribeBuzzThread(chain_, buzzId_, lPeers);
		//
		done_(ProcessingError());

	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

//
// LoadBuzzerInfoCommand
//
void LoadBuzzerInfoCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 1) {

		std::string lBuzzer = args[0];
		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadBuzzerInfo::instance(
			composer_, 
			lBuzzer,
			boost::bind(&LoadBuzzerInfoCommand::buzzerAndInfoLoaded, shared_from_this(), _1, _2, _3));
		
		// async process
		lCommand->process(boost::bind(&LoadBuzzerInfoCommand::error, shared_from_this(), _1, _2));
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
		return;
	}
}