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
			boost::bind(&CreateBuzzerCommand::buzzerCreated, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		// async process
		lCommander->process(boost::bind(&CreateBuzzerCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
	if (!(peer_ = composer_->requestProcessor()->sendTransaction(ctx,
			SentTransaction::instance(
				boost::bind(&CreateBuzzerCommand::buzzerSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&CreateBuzzerCommand::timeout, shared_from_this()))))) {
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
		if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("     buzzer: %s", tx.toHex()));
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
		uploadAvatar_->process(lArgs, peer_);
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

		uploadHeader_->process(lArgs, peer_);
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
		boost::bind(&CreateBuzzerCommand::buzzerInfoCreated, shared_from_this(), boost::placeholders::_1));
	// async process
	lCommanderInfo->process(boost::bind(&CreateBuzzerCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}

void CreateBuzzerCommand::buzzerInfoCreated(TransactionContextPtr ctx) {
	//
	buzzerInfoTx_ = ctx->tx();
	//
	if (!composer_->requestProcessor()->sendTransaction(peer_, ctx->tx()->chain(), ctx, 
			SentTransaction::instance(
				boost::bind(&CreateBuzzerCommand::buzzerInfoSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
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
// CreateBuzzerGroupCommand
//
void CreateBuzzerGroupCommand::process(const std::vector<std::string>& args) {
	//
	args_ = args;
	buzzerOut_ = nullptr;

	if (args.size() >= 2) {
		//
		uint256 lPKey; lPKey.setHex(args[1]);
		// prepare
		IComposerMethodPtr lCommander = BuzzerLightComposer::CreateTxBuzzerGroup::instance(composer_, 
			std::string(args[0]),
			lPKey,
			boost::bind(&CreateBuzzerGroupCommand::buzzerCreated, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		// async process
		lCommander->process(boost::bind(&CreateBuzzerGroupCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
		return;
	}
}

void CreateBuzzerGroupCommand::buzzerCreated(TransactionContextPtr ctx, Transaction::UnlinkedOutPtr buzzerOut) {
	//
	buzzerOut_ = buzzerOut;
	buzzerTx_ = ctx->tx();
	//
	if (!(peer_ = composer_->requestProcessor()->sendTransaction(ctx,
			SentTransaction::instance(
				boost::bind(&CreateBuzzerGroupCommand::buzzerSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&CreateBuzzerGroupCommand::timeout, shared_from_this()))))) {
		composer_->wallet()->resetCache();
		composer_->wallet()->prepareCache();
		error("E_TX_NOT_SENT", "Transaction was not sent");
	}
}

void CreateBuzzerGroupCommand::buzzerSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
	//
	if (errors.size()) {
		for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
				lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
			error("E_CREATE_BUZZER", lError->data());
		}
	} else {
		std::cout << "      group: " << tx.toHex() << std::endl;
		if (gLog().isEnabled(Log::CLIENT)) gLog().write(Log::CLIENT, strprintf("      group: %s", tx.toHex()));
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
		uploadAvatar_->process(lArgs, peer_);
	} else {
		createBuzzerInfo();
	}
#else
	createBuzzerInfo();
#endif
}

void CreateBuzzerGroupCommand::avatarUploaded(TransactionPtr tx, const ProcessingError& err) {
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

		uploadHeader_->process(lArgs, peer_);
		return;
	} else if (!tx) {
		error(err.error(), err.message());
		return;
	}

	createBuzzerInfo();
}

void CreateBuzzerGroupCommand::headerUploaded(TransactionPtr tx, const ProcessingError& err) {
	if (tx) {
		//
		headerTx_ = tx;
	} else {
		error(err.error(), err.message());
		return;
	}

	createBuzzerInfo();
}

void CreateBuzzerGroupCommand::createBuzzerInfo() {
	//
	BuzzerMediaPointer lAvatar;
	BuzzerMediaPointer lHeader;

	if (avatarTx_) lAvatar = BuzzerMediaPointer(avatarTx_->chain(), avatarTx_->id());
	if (headerTx_) lHeader = BuzzerMediaPointer(headerTx_->chain(), headerTx_->id());

	// prepare
	IComposerMethodPtr lCommanderInfo = BuzzerLightComposer::CreateTxBuzzerGroupInfo::instance(
		composer_, buzzerTx_, buzzerKeysTx_, args_[1], args_[2], lAvatar,
		boost::bind(&CreateBuzzerGroupCommand::buzzerInfoCreated, shared_from_this(), boost::placeholders::_1));
	// async process
	lCommanderInfo->process(boost::bind(&CreateBuzzerGroupCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}

void CreateBuzzerGroupCommand::buzzerInfoCreated(TransactionContextPtr ctx) {
	//
	buzzerInfoTx_ = ctx->tx();
	//
	if (!composer_->requestProcessor()->sendTransaction(peer_, ctx->tx()->chain(), ctx, 
			SentTransaction::instance(
				boost::bind(&CreateBuzzerGroupCommand::buzzerInfoSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&CreateBuzzerGroupCommand::timeout, shared_from_this())))) {
		composer_->wallet()->resetCache();
		composer_->wallet()->prepareCache();
		error("E_TX_INFO_NOT_SENT", "Transaction was not sent");
	}
}

void CreateBuzzerGroupCommand::buzzerInfoSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
	//
	if (errors.size()) {
		for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
				lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
			error("E_CREATE_BUZZER_INFO", lError->data());
		}

		return;			
	}

	std::cout << " group info: " << tx.toHex() << std::endl;
	done_(buzzerTx_, buzzerInfoTx_, ProcessingError());
}

//
// CreateBuzzerInfoCommand
//
void CreateBuzzerInfoCommand::process(const std::vector<std::string>& args) {
	//
	args_ = args;

	if (args.size() >= 2) {
		//
#if defined(CUBIX_MOD)
		if (args_.size() > 2 && uploadAvatar_) {
			//
			// upload avatar
			std::vector<std::string> lArgs;
			lArgs.push_back(args_[2]);
			if (args_.size() > 5 && args_[3] == "-s") {
				lArgs.push_back(args_[3]);
				lArgs.push_back(args_[4]);
			}
			uploadAvatar_->process(lArgs);
		} else {
			createBuzzerInfo();
		}
#else
		createBuzzerInfo();
#endif
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
		return;
	}
}

void CreateBuzzerInfoCommand::avatarUploaded(TransactionPtr tx, const ProcessingError& err) {
	//
	if (tx && uploadHeader_) {
		//
		avatarTx_ = tx;

		// upload header
		std::vector<std::string> lArgs;
		if (args_.size() > 5) {
			lArgs.push_back(args_[5]);
		} else if (args_.size() > 3) {
			lArgs.push_back(args_[3]);
		}

		uploadHeader_->process(lArgs);
		return;
	} else if (!tx) {
		error(err.error(), err.message());
		return;
	}

	createBuzzerInfo();
}

void CreateBuzzerInfoCommand::headerUploaded(TransactionPtr tx, const ProcessingError& err) {
	if (tx) {
		//
		headerTx_ = tx;
	} else {
		error(err.error(), err.message());
		return;
	}

	createBuzzerInfo();
}

void CreateBuzzerInfoCommand::createBuzzerInfo() {
	//
	BuzzerMediaPointer lAvatar;
	BuzzerMediaPointer lHeader;

	if (avatarTx_) lAvatar = BuzzerMediaPointer(avatarTx_->chain(), avatarTx_->id());
	if (headerTx_) lHeader = BuzzerMediaPointer(headerTx_->chain(), headerTx_->id());

	// prepare
	IComposerMethodPtr lCommanderInfo = BuzzerLightComposer::CreateTxBuzzerInfo::instance(
		composer_, Transaction::UnlinkedOut::instance(composer_->buzzerUtxo()[0]), args_[0], args_[1], lAvatar, lHeader,
		boost::bind(&CreateBuzzerInfoCommand::buzzerInfoCreated, shared_from_this(), boost::placeholders::_1));
	// async process
	lCommanderInfo->process(boost::bind(&CreateBuzzerInfoCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}

void CreateBuzzerInfoCommand::buzzerInfoCreated(TransactionContextPtr ctx) {
	//
	buzzerInfoTx_ = ctx->tx();
	//
	if (!composer_->requestProcessor()->sendTransaction(ctx->tx()->chain(), ctx, 
			SentTransaction::instance(
				boost::bind(&CreateBuzzerInfoCommand::buzzerInfoSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&CreateBuzzerInfoCommand::timeout, shared_from_this())))) {
		composer_->wallet()->resetCache();
		composer_->wallet()->prepareCache();
		error("E_TX_INFO_NOT_SENT", "Transaction was not sent");
	}
}

void CreateBuzzerInfoCommand::buzzerInfoSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
	//
	if (errors.size()) {
		for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
				lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
			error("E_CREATE_BUZZER_INFO", lError->data());
		}

		return;			
	}

	std::cout << "buzzer info: " << tx.toHex() << std::endl;
	done_(nullptr, buzzerInfoTx_, ProcessingError());
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
		peer_ = nullptr;

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
		boost::bind(&CreateBuzzCommand::created, shared_from_this(), boost::placeholders::_1));
	// async process
	lCommand->process(boost::bind(&CreateBuzzCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}

void CreateBuzzCommand::uploadNextMedia() {
	//
	if (mediaFiles_.size() && uploadMedia_) {
		std::string lFile = *mediaFiles_.begin();
		mediaFiles_.erase(mediaFiles_.begin());

		std::vector<std::string> lArgs; lArgs.push_back(lFile);
		uploadMedia_->process(lArgs, peer_);
	}
}

void CreateBuzzCommand::mediaUploaded(TransactionPtr tx, const ProcessingError& err) {
	//
	if (!tx) {
		error(err.error(), err.message());
		return;
	}

	// get interactive peer
	peer_ = uploadMedia_->peer();

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
			boost::bind(&BuzzerSubscribeCommand::created, shared_from_this(), boost::placeholders::_1));
		// async process
		lCommand->process(boost::bind(&BuzzerSubscribeCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
			boost::bind(&BuzzerUnsubscribeCommand::created, shared_from_this(), boost::placeholders::_1));
		// async process
		lCommand->process(boost::bind(&BuzzerUnsubscribeCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments.");
	}
}

//
// LoadHashTagsCommand
//
void LoadHashTagsCommand::process(const std::vector<std::string>& args) {
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	feed_.clear();
	commands_.clear();

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
			3 /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadHashTagsCommand::tagsLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		// async process
		lCommand->process(boost::bind(&LoadHashTagsCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		//
		commands_.push_back(lCommand);
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

		processing_ = false;

		done_(tag_, lTags, ProcessingError());
	}
}

//
// LoadBuzzfeedByTagCommand
//
void LoadBuzzfeedByTagCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localBuzzFeed_->clear();
	commands_.clear();
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
			G_BUZZFEED_PEERS_CONFIRMATIONS /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadBuzzfeedByTagCommand::buzzfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		// async process
		lCommand->process(boost::bind(&LoadBuzzfeedByTagCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		//
		commands_.push_back(lCommand);
	}
}

//
// LoadBuzzesGlobalCommand
//
void LoadBuzzesGlobalCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localBuzzFeed_->clear();
	commands_.clear();
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
			G_BUZZFEED_PEERS_CONFIRMATIONS /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadBuzzesGlobalCommand::buzzfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		// async process
		lCommand->process(boost::bind(&LoadBuzzesGlobalCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		//
		commands_.push_back(lCommand);
	}
}

//
// LoadBuzzfeedByBuzzerCommand
//
void LoadBuzzfeedByBuzzerCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localBuzzFeed_->clear();
	commands_.clear();
	pendingChainInfosLoaded_ = 0;
	//buzzFeed_->clear();

	std::string lBuzzer;
	// args - from
	fromAny_.clear();
	if (args.size() >= 1) {
		//
		lBuzzer = args[0];

		//
		if (args.size() > 1 && args[1] == "more") {
			//
			buzzFeed_->locateLastTimestamp(fromAny_);
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
		std::map<uint256 /*chain*/, std::vector<BuzzfeedPublisherFrom>>::iterator lFromChain = fromAny_.find(*lChain);
		if (lFromChain != fromAny_.end())
			lFrom = lFromChain->second;

		//
		//if (!lFrom.size()) continue;
		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadBuzzesByBuzzer::instance(
			composer_, 
			*lChain, 
			lFrom,
			lBuzzer,
			G_BUZZFEED_PEERS_CONFIRMATIONS /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadBuzzfeedByBuzzerCommand::buzzfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		// async process
		lCommand->process(boost::bind(&LoadBuzzfeedByBuzzerCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		//
		commands_.push_back(lCommand);
	}
}

//
// LoadBuzzfeedByBuzzCommand
//
void LoadBuzzfeedByBuzzCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localBuzzFeed_->clear();
	commands_.clear();
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
			G_BUZZFEED_PEERS_CONFIRMATIONS /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadBuzzfeedByBuzzCommand::buzzfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		// async process
		lCommand->process(boost::bind(&LoadBuzzfeedByBuzzCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		//
		commands_.push_back(lCommand);
	}
}

//
// LoadMessagesCommand
//
void LoadMessagesCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localBuzzFeed_->clear();
	commands_.clear();
	pendingChainInfosLoaded_ = 0;
	//buzzFeed_->clear();

	uint256 lConversationId;
	// args - from
	from_ = 0; // most recent
	if (args.size() >= 1) {
		//
		lConversationId.setHex(args[0]);

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
		gLog().write(Log::CLIENT, strprintf("[LoadMessagesCommand]: %d / %s",
										from_, lConversationId.toHex()));

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadMessages::instance(
			composer_, 
			*lChain, 
			from_,
			lConversationId,
			G_BUZZFEED_PEERS_CONFIRMATIONS /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadMessagesCommand::buzzfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		// async process
		lCommand->process(boost::bind(&LoadMessagesCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		//
		commands_.push_back(lCommand);
	}
}

//
// LoadBuzzfeedCommand
//
void LoadBuzzfeedCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localBuzzFeed_->clear();
	commands_.clear();
	pendingChainInfosLoaded_ = 0;

	// args - from
	fromAny_.clear();
	if (args.size() == 1) {
		if (args[0] == "more") {
			//
			buzzFeed_->locateLastTimestamp(fromAny_);
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
		std::map<uint256 /*chain*/, std::vector<BuzzfeedPublisherFrom>>::iterator lFromChain = fromAny_.find(*lChain);
		if (lFromChain != fromAny_.end())
			lFrom = lFromChain->second;

		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadBuzzfeed::instance(
			composer_,
			*lChain,
			lFrom,
			G_BUZZFEED_PEERS_CONFIRMATIONS /*to be sure that the feed is not doctored*/,
			boost::bind(&LoadBuzzfeedCommand::buzzfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		// async process
		lCommand->process(boost::bind(&LoadBuzzfeedCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		//
		commands_.push_back(lCommand);
	}
}

void LoadBuzzfeedCommand::display(const std::vector<BuzzfeedItemPtr>& feed) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	for (std::vector<BuzzfeedItemPtr>::const_iterator lBuzz = feed.begin(); lBuzz != feed.end(); lBuzz++) {
		std::cout << (*lBuzz)->toString() << std::endl << std::endl;
	}
}

void LoadBuzzfeedCommand::buzzesLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	pendingLoaded_.insert(chain);
	std::cout << strprintf("pending, chain: %s/%d, n = %d", chain.toHex(), feed.size(), pendingLoaded_.size()) << std::endl;
	if (gLog().isEnabled(Log::CLIENT))
		gLog().write(Log::CLIENT, strprintf("pending, chain: %s/%d, n = %d", chain.toHex(), feed.size(), pendingLoaded_.size()));

	if (pendingLoaded_.size() < pending_.size()) {
		// merge feed
		buzzfeed()->merge(feed, 1 /*exact ONE*/);
	} else {
		// merge and notify
		buzzfeed()->merge(feed, 1 /*exact ONE*/, true);
		//
		processPengingInfos();
	}
}

void LoadBuzzfeedCommand::buzzfeedLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain, int requests) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	loaded_[chain] = loaded_[chain] + 1;
	std::cout << strprintf("[buzzfeed] - chain: %s/%d, n = %d/%d/%d", chain.toHex(), feed.size(), loaded_.size(), loaded_[chain], requests) << std::endl;
	if (gLog().isEnabled(Log::CLIENT))
		gLog().write(Log::CLIENT, strprintf("[buzzfeed] - chain: %s/%d, n = %d/%d/%d", chain.toHex(), feed.size(), loaded_.size(), loaded_[chain], requests));

	//
	int lReady = 0;
	for (std::map<uint256, int>::iterator lChainLoaded = loaded_.begin(); lChainLoaded != loaded_.end(); lChainLoaded++) {
		lReady += lChainLoaded->second;
	}

	// for real client - mobile, for example, we can start to display feeds as soon as they arrived
	if (gLog().isEnabled(Log::CLIENT))
		gLog().write(Log::CLIENT, strprintf("[buzzfeed] - chain: %s/%d, ready = %d/%d", chain.toHex(), feed.size(), lReady, (requests * chains_.size())));
	//
	if (lReady < requests * (int) chains_.size()) {
		// merge feed
		buzzfeed()->merge(feed, requests);
	} else {
		// merge and notify
		buzzfeed()->merge(feed, requests, true);
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
						boost::bind(&LoadBuzzfeedCommand::buzzesLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
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
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
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
					boost::bind(&LoadBuzzfeedCommand::buzzerInfoLoaded, shared_from_this(), boost::placeholders::_1),
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
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
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
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
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

	processing_ = false;

	done_(ProcessingError());
}

//
// LoadEndorsementsByBuzzerCommand
//
void LoadEndorsementsByBuzzerCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localEventsFeed_->clear();
	commands_.clear();
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
				G_BUZZFEED_PEERS_CONFIRMATIONS,
				boost::bind(&LoadEndorsementsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		else
			lCommand = BuzzerLightComposer::LoadEndorsementsByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzer, 
				G_BUZZFEED_PEERS_CONFIRMATIONS,
				boost::bind(&LoadEndorsementsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));

		// async process
		lCommand->process(boost::bind(&LoadEndorsementsByBuzzerCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		//
		commands_.push_back(lCommand);
	}
}

//
// LoadMistrustsByBuzzerCommand
//
void LoadMistrustsByBuzzerCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localEventsFeed_->clear();
	commands_.clear();
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
				G_BUZZFEED_PEERS_CONFIRMATIONS,
				boost::bind(&LoadMistrustsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		else
			 lCommand = BuzzerLightComposer::LoadMistrustsByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzer,
				G_BUZZFEED_PEERS_CONFIRMATIONS,
				boost::bind(&LoadMistrustsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));

		// async process
		lCommand->process(boost::bind(&LoadMistrustsByBuzzerCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		//
		commands_.push_back(lCommand);
	}
}

//
// LoadSubscriptionsByBuzzerCommand
//
void LoadSubscriptionsByBuzzerCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localEventsFeed_->clear();
	commands_.clear();
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
				G_BUZZFEED_PEERS_CONFIRMATIONS,
				boost::bind(&LoadSubscriptionsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		else
			lCommand = BuzzerLightComposer::LoadSubscriptionsByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzer,
				G_BUZZFEED_PEERS_CONFIRMATIONS,
				boost::bind(&LoadSubscriptionsByBuzzerCommand::eventsfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));

		// async process
		lCommand->process(boost::bind(&LoadSubscriptionsByBuzzerCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		//
		commands_.push_back(lCommand);
	}
}

//
// LoadFollowersByBuzzerCommand
//
void LoadFollowersByBuzzerCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

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
	commands_.clear();

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
				G_BUZZFEED_PEERS_CONFIRMATIONS,
				boost::bind(&LoadFollowersByBuzzerCommand::eventsfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		else
			lCommand = BuzzerLightComposer::LoadFollowersByBuzzer::instance(
				composer_, 
				*lChain, 
				fromBuzzer_, 
				lBuzzer,
				G_BUZZFEED_PEERS_CONFIRMATIONS,
				boost::bind(&LoadFollowersByBuzzerCommand::eventsfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));

		// async process
		lCommand->process(boost::bind(&LoadFollowersByBuzzerCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		//
		commands_.push_back(lCommand);
	}
}

//
// LoadEventsfeedCommand
//
void LoadEventsfeedCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localEventsFeed_->clear();
	commands_.clear();

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

	//
	if (gLog().isEnabled(Log::CLIENT))
		gLog().write(Log::CLIENT, strprintf("[eventsfeed] - loading for %d chains...", chains_.size()));	

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		if (gLog().isEnabled(Log::CLIENT))
			gLog().write(Log::CLIENT, strprintf("[eventsfeed] - loading for %s chain", (*lChain).toHex()));
		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadEventsfeed::instance(
			composer_, *lChain, from_, G_BUZZFEED_PEERS_CONFIRMATIONS,
			boost::bind(&LoadEventsfeedCommand::eventsfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		// async process
		lCommand->process(boost::bind(&LoadEventsfeedCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		// backup
		commands_.push_back(lCommand);
	}
}

void LoadEventsfeedCommand::display(EventsfeedItemPtr item) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	std::vector<EventsfeedItemPtr> lFeed;
	item->feed(lFeed);

	for (std::vector<EventsfeedItemPtr>::iterator lEvent = lFeed.begin(); lEvent != lFeed.end(); lEvent++) {
		std::cout << (*lEvent)->toString() << std::endl << std::endl;
	}
}

void LoadEventsfeedCommand::buzzesLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
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
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	loaded_[chain] = loaded_[chain] + 1;
	std::cout << strprintf("[eventsfeed] - chain: %s/%d, n = %d/%d/%d", chain.toHex(), feed.size(), loaded_.size(), loaded_[chain], requests) << std::endl;
	if (gLog().isEnabled(Log::CLIENT))
		gLog().write(Log::CLIENT, strprintf("[eventsfeed] - chain: %s/%d, n = %d/%d/%d", chain.toHex(), feed.size(), loaded_.size(), loaded_[chain], requests));

	//
	int lReady = 0;
	for (std::map<uint256, int>::iterator lChainLoaded = loaded_.begin(); lChainLoaded != loaded_.end(); lChainLoaded++) {
		lReady += lChainLoaded->second;
	}

	// for real client - mobile, for example, we can start to display feeds as soon as they arrived
	if (lReady < requests * chains_.size()) {
		// merge feed
		eventsfeed()->merge(feed, requests);
	} else {
		//
		if (gLog().isEnabled(Log::CLIENT))
			gLog().write(Log::CLIENT, strprintf("[eventsfeed] - merge: chains = %d, commands = %d, n = %d/%d", chains_.size(), commands_.size(), loaded_.size(), requests));
		// merge and notify
		eventsfeed()->merge(feed, requests, true);
		// if we have postponed items, request missing
		eventsfeed()->collectPendingItems(pending_);

		if (pending_.size()) {
			for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pending_.begin(); lChain != pending_.end(); lChain++) {
				//
				std::vector<uint256> lBuzzes(lChain->second.begin(), lChain->second.end());
				if (!composer_->buzzerRequestProcessor()->selectBuzzes(lChain->first, lBuzzes, 
					SelectBuzzFeed::instance(
						boost::bind(&LoadEventsfeedCommand::buzzesLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
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
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	pendingInfos_ = eventsFeed_->buzzer()->pendingInfos(); // collect
	eventsFeed_->buzzer()->collectPengingInfos(pengindChainInfos_); // prepare

	if (pengindChainInfos_.size()) {
		for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pengindChainInfos_.begin(); lChain != pengindChainInfos_.end(); lChain++) {
			//
			std::vector<uint256> lInfos(lChain->second.begin(), lChain->second.end());
			if (!composer_->requestProcessor()->loadTransactions(lChain->first, lInfos, 
				LoadTransactions::instance(
					boost::bind(&LoadEventsfeedCommand::buzzerInfoLoaded, shared_from_this(), boost::placeholders::_1),
					boost::bind(&LoadEventsfeedCommand::timeout, shared_from_this()))
			)) error("E_LOAD_PENDING_BUZZER_INFOS", "Buzzer infos failed to load.");
		}
	} else {
		show();
	}
}

void LoadEventsfeedCommand::buzzerInfoLoaded(const std::vector<TransactionPtr>& txs) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
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
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
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

	processing_ = false;

	done_(ProcessingError());
}

//
// LoadConversationsCommand
//
void LoadConversationsCommand::process(const std::vector<std::string>& args) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (processing_) return;
	processing_ = true;

	// clean-up
	chains_.clear();
	loaded_.clear();
	pending_.clear();
	pendingLoaded_.clear();
	pendingInfos_.clear();
	pengindChainInfos_.clear();
	localConversationsFeed_->clear();
	commands_.clear();
	pendingChainInfosLoaded_ = 0;

	// args - from
	uint256 lBuzzerId;
	from_ = 0; // most recent
	if (args.size() >= 1) {
		if (args[0] == "more") {
			//
			from_ = conversationsFeed_->locateLastTimestamp();
		} else {
			lBuzzerId.setHex(args[0]);
		}

		if (args.size() > 1 && args[1] == "more") {
			from_ = conversationsFeed_->locateLastTimestamp();
		}
	}

	if (lBuzzerId.isNull()) {
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
		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadConversations::instance(
			composer_, 
			*lChain, 
			lBuzzerId, 
			from_,
			CONVERSATIONSFEED_CONFIRMATIONS,
			boost::bind(&LoadConversationsCommand::eventsfeedLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));

		// async process
		lCommand->process(boost::bind(&LoadConversationsCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		// backup
		commands_.push_back(lCommand);
	}
}

void LoadConversationsCommand::display(ConversationItemPtr item) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	std::vector<ConversationItemPtr> lFeed;
	item->feed(lFeed);

	for (std::vector<ConversationItemPtr>::iterator lEvent = lFeed.begin(); lEvent != lFeed.end(); lEvent++) {
		std::cout << (*lEvent)->toString() << std::endl << std::endl;
	}
}

void LoadConversationsCommand::buzzesLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	pendingLoaded_.insert(chain);
	std::cout << strprintf("pending, chain: %s/%d, n = %d", chain.toHex(), feed.size(), pendingLoaded_.size()) << std::endl;

	/*
	if (pendingLoaded_.size() < pending_.size()) {
		// merge feed
		conversationsfeed()->merge(feed);
	} else {
		// merge and notify
		conversationsfeed()->merge(feed);
		// process pending info
		processPengingInfos();
	}
	*/

	processPengingInfos();
}

void LoadConversationsCommand::eventsfeedLoaded(const std::vector<ConversationItem>& feed, const uint256& chain, int requests) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	loaded_[chain] = loaded_[chain] + 1;
	std::cout << strprintf("[conversations] - chain: %s/%d, n = %d/%d/%d", chain.toHex(), feed.size(), loaded_.size(), loaded_[chain], requests) << std::endl;
	if (gLog().isEnabled(Log::CLIENT))
		gLog().write(Log::CLIENT, strprintf("[conversations] - chain: %s/%d, n = %d/%d/%d", chain.toHex(), feed.size(), loaded_.size(), loaded_[chain], requests));

	//
	int lReady = 0;
	for (std::map<uint256, int>::iterator lChainLoaded = loaded_.begin(); lChainLoaded != loaded_.end(); lChainLoaded++) {
		lReady += lChainLoaded->second;
	}

	// for real client - mobile, for example, we can start to display feeds as soon as they arrived
	if (lReady < requests * chains_.size()) {
		// merge feed
		conversationsfeed()->merge(feed, chain);
	} else {
		// merge and notify
		conversationsfeed()->merge(feed, chain, true);
		// if we have postponed items, request missing
		conversationsfeed()->collectPendingItems(pending_);

		if (pending_.size()) {
			for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pending_.begin(); lChain != pending_.end(); lChain++) {
				//
				std::vector<uint256> lBuzzes(lChain->second.begin(), lChain->second.end());
				if (!composer_->buzzerRequestProcessor()->selectBuzzes(lChain->first, lBuzzes, 
					SelectBuzzFeed::instance(
						boost::bind(&LoadConversationsCommand::buzzesLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
						boost::bind(&LoadConversationsCommand::timeout, shared_from_this()))
				)) error("E_LOAD_PENDING_EVENTSFEED", "Eventsfeed pending items loading failed.");	
			}
		} else {
			processPengingInfos();
		}
	}
}

void LoadConversationsCommand::processPengingInfos() {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	pendingInfos_ = conversationsFeed_->buzzer()->pendingInfos(); // collect
	conversationsFeed_->buzzer()->collectPengingInfos(pengindChainInfos_); // prepare

	if (pengindChainInfos_.size()) {
		for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pengindChainInfos_.begin(); lChain != pengindChainInfos_.end(); lChain++) {
			//
			std::vector<uint256> lInfos(lChain->second.begin(), lChain->second.end());
			if (!composer_->requestProcessor()->loadTransactions(lChain->first, lInfos, 
				LoadTransactions::instance(
					boost::bind(&LoadConversationsCommand::buzzerInfoLoaded, shared_from_this(), boost::placeholders::_1),
					boost::bind(&LoadConversationsCommand::timeout, shared_from_this()))
			)) error("E_LOAD_PENDING_BUZZER_INFOS", "Buzzer infos failed to load.");
		}
	} else {
		show();
	}
}

void LoadConversationsCommand::buzzerInfoLoaded(const std::vector<TransactionPtr>& txs) {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
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
				conversationsFeed_->buzzer()->pushBuzzerInfo(lInfo);
		}
	}

	if (pendingChainInfosLoaded_ >= pengindChainInfos_.size()) {
		show();
	}
}

void LoadConversationsCommand::show() {
	//
	boost::unique_lock<boost::recursive_mutex> lLock(mutex_);
	//
	if (conversationsfeedReady_) {
		//
		conversationsfeedReady_(conversationsFeed_, localConversationsFeed_);
	} else {
		// get list
		std::vector<ConversationItemPtr> lFeed;
		// if 'more' provided
		if (appendFeed()) {
			std::vector<ConversationItemPtr> lUpFeed;
			localConversationsFeed_->feed(lUpFeed);
			conversationsFeed_->mergeAppend(lUpFeed);
		}

		conversationsFeed_->feed(lFeed);
		// display
		std::cout << std::endl;
		display(conversationsFeed_->toItem());
	}

	processing_ = false;

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
		buzzFeed_->merge(feed, 1 /*exact ONE*/);
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
					boost::bind(&BuzzfeedListCommand::buzzesLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
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
					boost::bind(&BuzzfeedListCommand::buzzerInfoLoaded, shared_from_this(), boost::placeholders::_1),
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
					boost::bind(&EventsfeedListCommand::buzzesLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
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
// ConversationsListCommand
//
void ConversationsListCommand::display(ConversationItemPtr item) {
	//
	std::vector<ConversationItemPtr> lFeed;
	item->feed(lFeed);

	for (std::vector<ConversationItemPtr>::iterator lEvent = lFeed.begin(); lEvent != lFeed.end(); lEvent++) {
		std::cout << (*lEvent)->toString() << std::endl << std::endl;
	}
}

void ConversationsListCommand::buzzesLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	//
	pendingLoaded_.insert(chain);
	std::cout << strprintf("pending, chain: %s/%d, n = %d", chain.toHex(), feed.size(), pendingLoaded_.size()) << std::endl;

	/*
	if (pendingLoaded_.size() < pending_.size()) {
		// merge feed
		eventsFeed_->merge(feed);
	} else {
		// merge and notify
		eventsFeed_->merge(feed);
		// get list
		std::vector<ConversationItemPtr> lFeed;
		eventsFeed_->feed(lFeed);
		// display
		std::cout << std::endl;
		display(eventsFeed_->toItem());

		done_();
	}
	*/

	display(eventsFeed_->toItem());
	done_();
}

void ConversationsListCommand::process(const std::vector<std::string>& args) {
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
					boost::bind(&ConversationsListCommand::buzzesLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&ConversationsListCommand::timeout, shared_from_this()))
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
				boost::bind(&BuzzLikeCommand::created, shared_from_this(), boost::placeholders::_1));
			// async process
			lCommand->process(boost::bind(&BuzzLikeCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
// BuzzHideCommand
//
void BuzzHideCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 1) {
		// prepare
		uint256 lBuzzId;
		lBuzzId.setHex(args[0]);
		//
		BuzzfeedItemPtr lItem = buzzFeed_->locateBuzz(lBuzzId);
		//
		if (composer_->buzzerId() != lItem->buzzerId()) {
			error("E_BUZZ_OWNER_IS_INCORRECT", "Incorrect buzz owner");
			return;
		}
		//
		if (lItem) {
			IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzHide::instance(composer_, lItem->buzzChainId(), lItem->buzzId(),
				boost::bind(&BuzzHideCommand::created, shared_from_this(), boost::placeholders::_1));
			// async process
			lCommand->process(boost::bind(&BuzzHideCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
// BuzzerHideCommand
//
void BuzzerHideCommand::process(const std::vector<std::string>& args) {
	//
	IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzerHide::instance(composer_,
		boost::bind(&BuzzerHideCommand::created, shared_from_this(), boost::placeholders::_1));
	// async process
	lCommand->process(boost::bind(&BuzzerHideCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}

//
// BuzzerBlockCommand
//
void BuzzerBlockCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 2) {
		// prepare
		uint256 lBuzzerId;
		lBuzzerId.setHex(args[0]);
		//
		uint256 lChainId;
		lChainId.setHex(args[1]);
		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzerBlock::instance(composer_, lBuzzerId, lChainId,
			boost::bind(&BuzzerBlockCommand::created, shared_from_this(), boost::placeholders::_1));
		// async process
		lCommand->process(boost::bind(&BuzzerBlockCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
		return;
	}
}

//
// BuzzerUnBlockCommand
//
void BuzzerUnBlockCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 2) {
		// prepare
		uint256 lBuzzerId;
		lBuzzerId.setHex(args[0]);
		//
		uint256 lChainId;
		lChainId.setHex(args[1]);
		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzerUnBlock::instance(composer_, lBuzzerId, lChainId,
			boost::bind(&BuzzerUnBlockCommand::created, shared_from_this(), boost::placeholders::_1));
		// async process
		lCommand->process(boost::bind(&BuzzerUnBlockCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
				boost::bind(&BuzzRewardCommand::created, shared_from_this(), boost::placeholders::_1));
			// async process
			lCommand->process(boost::bind(&BuzzRewardCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
		peer_ = nullptr;

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
			boost::bind(&CreateBuzzReplyCommand::created, shared_from_this(), boost::placeholders::_1));
		// async process
		lCommand->process(boost::bind(&CreateBuzzReplyCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
		uploadMedia_->process(lArgs, peer_);
	}	
}

void CreateBuzzReplyCommand::mediaUploaded(TransactionPtr tx, const ProcessingError& err) {
	//
	if (!tx) {
		error(err.error(), err.message());
		return;
	}

	//
	peer_ = uploadMedia_->peer();

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
		peer_ = nullptr;

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
			boost::bind(&CreateReBuzzCommand::created, shared_from_this(), boost::placeholders::_1));
		// async process
		lCreateReBuzz->process(boost::bind(&CreateReBuzzCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
		uploadMedia_->process(lArgs, peer_);
	}	
}

void CreateReBuzzCommand::mediaUploaded(TransactionPtr tx, const ProcessingError& err) {
	//
	if (!tx) {
		error(err.error(), err.message());
		return;
	}

	//
	peer_ = uploadMedia_->peer();

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
			boost::bind(&LoadBuzzerTrustScoreCommand::trustScoreLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3, boost::placeholders::_4),
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
			boost::bind(&BuzzerEndorseCommand::created, shared_from_this(), boost::placeholders::_1));
		// async process
		lCommand->process(boost::bind(&BuzzerEndorseCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
			boost::bind(&BuzzerMistrustCommand::created, shared_from_this(), boost::placeholders::_1));
		// async process
		lCommand->process(boost::bind(&BuzzerMistrustCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
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
		if (!composer_->buzzerRequestProcessor()->subscribeBuzzThread(chain_, buzzId_, signature_, G_BUZZFEED_PEERS_CONFIRMATIONS, peers_)) {
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
	if (args.size() >= 1) {
		//
		std::string lBuzzer = args[0];
		if (args.size() > 1) loadUtxo_ = args[1] == "-utxo";
		//
		IComposerMethodPtr lCommand = BuzzerLightComposer::LoadBuzzerInfo::instance(
			composer_, 
			lBuzzer,
			boost::bind(&LoadBuzzerInfoCommand::buzzerAndInfoLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
		
		// async process
		lCommand->process(boost::bind(&LoadBuzzerInfoCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
		return;
	}
}

void LoadBuzzerInfoCommand::buzzerAndInfoLoaded(EntityPtr buzzer, TransactionPtr info, const std::string& name) {
	//
	if (!buzzer) {
		error("E_BUZZER_NOT_FOUND", "Buzzer was not found.");
		return;
	}

	buzzer_ = buzzer;
	info_ = info;

	if (!loadUtxo_) {
		done_(buzzer, info, std::vector<Transaction::NetworkUnlinkedOut>(), name, ProcessingError());
	} else {
		// 
		if (!composer_->requestProcessor()->selectUtxoByTransaction(buzzer_->chain(), buzzer_->id(), 
			SelectUtxoByTransaction::instance(
				boost::bind(&LoadBuzzerInfoCommand::utxoByBuzzerLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadBuzzerInfoCommand::timeout, shared_from_this()))
		)) { error("E_LOAD_UTXO_BY_BUZZER", "Buzzer loading failed."); return; }
	}
}

void LoadBuzzerInfoCommand::utxoByBuzzerLoaded(const std::vector<Transaction::NetworkUnlinkedOut>& outs, const uint256& tx) {
	//
	if (tx == buzzer_->id()) {
		//
		done_(buzzer_, info_, outs, buzzer_->entityName(), ProcessingError());
	} else {
		error("E_LOAD_UTXO_BY_BUZZER", "Buzzer loading failed.");	
	}
}

//
// CreateBuzzerConversationCommand
//
void CreateBuzzerConversationCommand::process(const std::vector<std::string>& args) {
	//
	feeSent_ = false;
	conversationSent_ = false;

	if (args.size() == 1) {
		// prepare
		IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzerConversation::instance(composer_, args[0],
			boost::bind(&CreateBuzzerConversationCommand::created, shared_from_this(), boost::placeholders::_1));
		// async process
		lCommand->process(boost::bind(&CreateBuzzerConversationCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

//
// AcceptConversationCommand
//
void AcceptConversationCommand::process(const std::vector<std::string>& args) {
	//
	if (args.size() == 1) {
		//
		uint256 lConversationId;
		lConversationId.setHex(args[0]);
		ConversationItemPtr lConversation = conversations_->locateItem(lConversationId);
		//
		if (gLog().isEnabled(Log::CLIENT))
			gLog().write(Log::CLIENT, strprintf("[AcceptConversationCommand]: %s", lConversationId.toHex()));
		//
		if (lConversation) {
			IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzerAcceptConversation::instance(
				composer_, 
				lConversation->conversationChainId(),
				lConversation->conversationId(),
				boost::bind(&AcceptConversationCommand::created, shared_from_this(), boost::placeholders::_1));
			// async process
			lCommand->process(boost::bind(&AcceptConversationCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		} else {
			error("E_CONVERSATION_NOT_FOUND_IN_FEED", "Conversation was not found in local feed");
			return;
		}

		// prepare
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

//
// DeclineConversationCommand
//
void DeclineConversationCommand::process(const std::vector<std::string>& args) {
	//
	if (args.size() == 1) {
		//
		uint256 lConversationId;
		lConversationId.setHex(args[0]);
		ConversationItemPtr lConversation = conversations_->locateItem(lConversationId);
		//
		if (lConversation) {
			IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzerDeclineConversation::instance(
				composer_, 
				lConversation->conversationChainId(),
				lConversation->conversationId(),
				boost::bind(&DeclineConversationCommand::created, shared_from_this(), boost::placeholders::_1));
			// async process
			lCommand->process(boost::bind(&DeclineConversationCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
		} else {
			error("E_CONVERSATION_NOT_FOUND_IN_FEED", "Conversation was not found in local feed");
			return;
		}

		// prepare
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

//
// CreateBuzzerMessageCommand
//
void CreateBuzzerMessageCommand::process(const std::vector<std::string>& args) {
	if (args.size() >= 2) {
		// reset
		feeSent_ = false;
		messageSent_ = false;
		mediaPointers_.clear();
		mediaFiles_.clear();
		peer_ = nullptr;

		std::vector<std::string>::const_iterator lArg = args.begin();

		// prepare
		conversationId_.setHex(*(lArg++));
		body_ = *(lArg++);

		if (args.size() > 1) {
			//
			for (; lArg != args.end(); lArg++) {
				mediaFiles_.push_back(*lArg);
			}
		}

		//
		ConversationItemPtr lItem = conversations_->locateItem(conversationId_);
		if (!lItem) {
			error("E_CONVERSATION_NOT_FOUND_IN_FEED", "Conversation was not found in local feed.");
			return;
		}

		// locate counterpart pkey
		if (!composer_->getCounterparty(conversationId_, pkey_)) {
			// load conversation and extract counterparty key
			if (!composer_->requestProcessor()->loadTransaction(lItem->conversationChainId(), conversationId_, 
				LoadTransaction::instance(
					boost::bind(&CreateBuzzerMessageCommand::conversationLoaded, shared_from_this(), boost::placeholders::_1),
					boost::bind(&CreateBuzzerMessageCommand::timeout, shared_from_this()))
			)) { error("E_LOAD_UTXO_BY_CONVERSATION", "Conversation loading failed."); return; }
		} else {
			createMessage(pkey_);
		}
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments.");
	}
}

void CreateBuzzerMessageCommand::conversationLoaded(TransactionPtr conversation) {
	//
	TxBuzzerConversationPtr lConversation = TransactionHelper::to<TxBuzzerConversation>(conversation);
	//
	uint256 lInitiator = lConversation->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx(); 
	uint256 lCounterparty = lConversation->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx(); 

	// define counterparty
	if (composer_->buzzerId() == lInitiator) {
		lConversation->counterpartyAddress(pkey_);
	} else {
		lConversation->initiatorAddress(pkey_);
	}

	// save conversation -> pkey for counterparty
	composer_->writeCounterparty(conversationId_, pkey_);
	// continue...
	createMessage(pkey_);
}

void CreateBuzzerMessageCommand::createMessage(const PKey& pkey) {
	//
	if (mediaFiles_.size() && uploadMedia_) {
		uploadNextMedia(pkey);
	} else {
		createBuzz(pkey);
	}
}

void CreateBuzzerMessageCommand::createBuzz(const PKey& pkey) {
	//
	ConversationItemPtr lItem = conversations_->locateItem(conversationId_);
	//
	if (lItem) {
		IComposerMethodPtr lCommand = BuzzerLightComposer::CreateTxBuzzerMessage::instance(
			composer_, 
			pkey,
			lItem->conversationChainId(), 
			lItem->conversationId(), 
			body_, 
			mediaPointers_,
			boost::bind(&CreateBuzzerMessageCommand::created, shared_from_this(), boost::placeholders::_1));
		// async process
		lCommand->process(boost::bind(&CreateBuzzerMessageCommand::error, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
	} else {
		error("E_CONVERSATION_NOT_FOUND_IN_FEED", "Conversation was not found in local feed");
	}
}

void CreateBuzzerMessageCommand::uploadNextMedia(const PKey& pkey) {
	//
	if (mediaFiles_.size() && uploadMedia_) {
		std::string lFile = *mediaFiles_.begin();
		mediaFiles_.erase(mediaFiles_.begin());

		std::vector<std::string> lArgs;
		lArgs.push_back(lFile);
		lArgs.push_back("-p");
		lArgs.push_back(pkey.toString());
		uploadMedia_->process(lArgs, peer_);
	}	
}

void CreateBuzzerMessageCommand::mediaUploaded(TransactionPtr tx, const ProcessingError& err) {
	//
	if (!tx) {
		error(err.error(), err.message());
		return;
	}

	//
	peer_ = uploadMedia_->peer();

	if (mediaUploaded_) mediaUploaded_(tx, err);

	mediaPointers_.push_back(BuzzerMediaPointer(tx->chain(), tx->id()));
	
	// pop next
	if (mediaFiles_.size() && uploadMedia_) {
		uploadNextMedia(pkey_);
	} else {
		createBuzz(pkey_);
	}
}

//
// SelectConversationCommand
//
void SelectConversationCommand::process(const std::vector<std::string>& args) {
	//
	if (args.size() == 1) {
		//
		uint256 lConversationId;
		lConversationId.setHex(args[0]);

		//
		buzzFeed_->setRootBuzzId(lConversationId);
		composer_->buzzer()->setConversation(buzzFeed_);

		done_(ProcessingError());
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

//
// ConversationListCommand
//
void ConversationListCommand::display(const std::vector<BuzzfeedItemPtr>& feed) {
	//
	for (std::vector<BuzzfeedItemPtr>::const_iterator lBuzz = feed.begin(); lBuzz != feed.end(); lBuzz++) {
		std::cout << (*lBuzz)->toString() << std::endl << std::endl;
	}
}

void ConversationListCommand::buzzesLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	//
	pendingLoaded_.insert(chain);
	std::cout << strprintf("pending, chain: %s/%d, n = %d", chain.toHex(), feed.size(), pendingLoaded_.size()) << std::endl;

	if (pendingLoaded_.size() < pending_.size()) {
		// merge feed
		buzzFeed_->merge(feed, 1 /*exact ONE*/);
	} else {
		// merge and notify
		buzzFeed_->merge(feed, 1 /*exact ONE*/, true);
		//
		processPengingInfos();
	}
}

void ConversationListCommand::process(const std::vector<std::string>& args) {
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
					boost::bind(&ConversationListCommand::buzzesLoaded, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&ConversationListCommand::timeout, shared_from_this()))
			)) error("E_LOAD_PENDING_BUZZFEED", "Buzzfeed pending items loading failed.");	
		}
	} else {
		//
		processPengingInfos();
	}
}

void ConversationListCommand::processPengingInfos() {
	//
	pendingInfos_ = buzzFeed_->buzzer()->pendingInfos(); // collect
	buzzFeed_->buzzer()->collectPengingInfos(pengindChainInfos_); // prepare

	if (pengindChainInfos_.size()) {
		for (std::map<uint256 /*chain*/, std::set<uint256>/*items*/>::iterator lChain = pengindChainInfos_.begin(); lChain != pengindChainInfos_.end(); lChain++) {
			//
			std::vector<uint256> lInfos(lChain->second.begin(), lChain->second.end());
			if (!composer_->requestProcessor()->loadTransactions(lChain->first, lInfos, 
				LoadTransactions::instance(
					boost::bind(&ConversationListCommand::buzzerInfoLoaded, shared_from_this(), boost::placeholders::_1),
					boost::bind(&ConversationListCommand::timeout, shared_from_this()))
			)) error("E_LOAD_PENDING_BUZZER_INFOS", "Buzzer infos failed to load.");
		}
	} else {
		// show
		show();
		// we are done
		done_();
	}
}

void ConversationListCommand::buzzerInfoLoaded(const std::vector<TransactionPtr>& txs) {
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

void ConversationListCommand::show() {
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
// DecryptBuzzerMessageCommand
//
void DecryptBuzzerMessageCommand::process(const std::vector<std::string>& args) {
	//
	if (args.size() == 1) {
		//
		messageId_.setHex(args[0]);
		//
		BuzzfeedItemPtr lMessage = conversation_->locateBuzz(messageId_);
		if (lMessage) {
			ConversationItemPtr lConversation = conversations_->locateItem(lMessage->rootBuzzId());
			if (lConversation) {
				//
				// load conversation and extract counterparty key
				if (!composer_->requestProcessor()->loadTransaction(lConversation->conversationChainId(), lConversation->conversationId(), 
					LoadTransaction::instance(
						boost::bind(&DecryptBuzzerMessageCommand::conversationLoaded, shared_from_this(), boost::placeholders::_1),
						boost::bind(&DecryptBuzzerMessageCommand::timeout, shared_from_this()))
				)) { error("E_LOAD_UTXO_BY_CONVERSATION", "Conversation loading failed."); return; }

			} else {
				error("E_CONVERSATION_NOT_FOUND_IN_FEED", "Conversation was not found in local feed");		
			}
		} else {
			error("E_MESSAGE_NOT_FOUND_IN_FEED", "Message was not found in feed");	
		}
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

void DecryptBuzzerMessageCommand::conversationLoaded(TransactionPtr conversation) {
	//
	TxBuzzerConversationPtr lConversation = TransactionHelper::to<TxBuzzerConversation>(conversation);
	//
	PKey lCounterpartyKey;
	uint256 lInitiator = lConversation->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx(); 
	uint256 lCounterparty = lConversation->in()[TX_BUZZER_CONVERSATION_BUZZER_IN].out().tx(); 

	// define counterparty
	if (composer_->buzzerId() == lInitiator) {
		lConversation->counterpartyAddress(lCounterpartyKey);
	} else {
		lConversation->initiatorAddress(lCounterpartyKey);
	}

	BuzzfeedItemPtr lMessage = conversation_->locateBuzz(messageId_);
	if (lMessage) {
		if (lMessage->decrypt(lCounterpartyKey)) {
			if (done_) done_(lCounterpartyKey.toString(), lMessage->decryptedBuzzBodyString(), ProcessingError()); // pass pkey to the caller
		} else {
			error("E_MESSAGE_DECRYPTION_FAILED", "Message decryption failed");
		}
	}
}

//
// DecryptMessageBodyCommand
//
void DecryptMessageBodyCommand::process(const std::vector<std::string>& args) {
	//
	if (args.size() >= 1) {
		//
		conversationId_.setHex(args[0]);

		//
		hexBody_ = "";
		if (args.size() > 1) {
			hexBody_ = args[1];
		}

		// locate counterpart pkey
		PKey lPKey;
		if (composer_->getCounterparty(conversationId_, lPKey)) {
			//
			ConversationItemPtr lConversationItem = conversations_->locateItem(conversationId_);
			if (lConversationItem) {
				//
				std::string lBody;
				bool lResult = hexBody_.size() ? lConversationItem->decrypt(lPKey, hexBody_, lBody) :
												 lConversationItem->decrypt(lPKey, lBody);
				if (lResult) {
					if (done_) done_(lPKey.toString(), lBody, ProcessingError()); // pass body
				} else {
					error("E_MESSAGE_DECRYPTION_FAILED", "Message decryption failed");
				}
			} else {
				error("E_CONVERSATION_NOT_FOUND_IN_FEED", "Conversation was not found in local feed");
			}
		} else {
			ConversationItemPtr lConversation = conversations_->locateItem(conversationId_);
			if (lConversation) {
				//
				// load conversation and extract counterparty key
				if (!composer_->requestProcessor()->loadTransaction(
					lConversation->conversationChainId(),
					lConversation->conversationId(),
					true, // try mempool
					LoadTransaction::instance(
						boost::bind(&DecryptMessageBodyCommand::conversationLoaded, shared_from_this(), boost::placeholders::_1),
						boost::bind(&DecryptMessageBodyCommand::timeout, shared_from_this()))
				)) { error("E_LOAD_UTXO_BY_CONVERSATION", "Conversation loading failed."); return; }

			} else {
				error("E_CONVERSATION_NOT_FOUND_IN_FEED", "Conversation was not found in local feed");
			}
		}
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

void DecryptMessageBodyCommand::conversationLoaded(TransactionPtr conversation) {
	//
	if (!conversation) error("E_MESSAGE_DECRYPTION_FAILED", "Message decryption failed");

	//
	TxBuzzerConversationPtr lConversation = TransactionHelper::to<TxBuzzerConversation>(conversation);
	//
	PKey lCounterpartyKey;
	uint256 lInitiator = lConversation->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx(); 

	// define counterparty
	if (composer_->buzzerId() == lInitiator) {
		lConversation->counterpartyAddress(lCounterpartyKey);
	} else {
		lConversation->initiatorAddress(lCounterpartyKey);
	}

	ConversationItemPtr lConversationItem = conversations_->locateItem(conversationId_);
	if (lConversationItem) {
		//
		std::string lBody;
		bool lResult = hexBody_.size() ? lConversationItem->decrypt(lCounterpartyKey, hexBody_, lBody) :
										 lConversationItem->decrypt(lCounterpartyKey, lBody);
		if (lResult) {
			if (done_) done_(lCounterpartyKey.toString(), lBody, ProcessingError()); // pass pkey to the caller
		} else {
			error("E_MESSAGE_DECRYPTION_FAILED", "Message decryption failed");
		}
	}
}

//
// LoadCounterpartyKeyCommand
//
void LoadCounterpartyKeyCommand::process(const std::vector<std::string>& args) {
	//
	if (args.size() == 1) {
		//
		conversationId_.setHex(args[0]);

		// locate counterpart pkey
		PKey lPKey;
		if (!composer_->getCounterparty(conversationId_, lPKey)) {
			ConversationItemPtr lConversation = conversations_->locateItem(conversationId_);
			if (lConversation) {
				//
				// load conversation and extract counterparty key
				if (!composer_->requestProcessor()->loadTransaction(
					lConversation->conversationChainId(),
					lConversation->conversationId(),
					true, // try mempool
					LoadTransaction::instance(
						boost::bind(&LoadCounterpartyKeyCommand::conversationLoaded, shared_from_this(), boost::placeholders::_1),
						boost::bind(&LoadCounterpartyKeyCommand::timeout, shared_from_this()))
				)) { error("E_LOAD_UTXO_BY_CONVERSATION", "Conversation loading failed."); return; }

			} else {
				error("E_CONVERSATION_NOT_FOUND_IN_FEED", "Conversation was not found in local feed");
			}
		} else {
			done_(ProcessingError());
		}
	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments");
	}
}

void LoadCounterpartyKeyCommand::conversationLoaded(TransactionPtr conversation) {
	//
	if (!conversation) error("E_KEY_LOAD_FAILED", "Counterparty pkey loading failed.");

	//
	TxBuzzerConversationPtr lConversation = TransactionHelper::to<TxBuzzerConversation>(conversation);
	//
	PKey lCounterpartyKey;
	uint256 lInitiator = lConversation->in()[TX_BUZZER_CONVERSATION_MY_IN].out().tx(); 

	// define counterparty
	if (composer_->buzzerId() == lInitiator) {
		lConversation->counterpartyAddress(lCounterpartyKey);
	} else {
		lConversation->initiatorAddress(lCounterpartyKey);
	}

	// save conversation -> pkey for counterparty
	composer_->writeCounterparty(conversationId_, lCounterpartyKey);
	// done
	done_(ProcessingError());
}
