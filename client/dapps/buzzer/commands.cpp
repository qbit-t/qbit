#include "commands.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace qbit;

void CreateBuzzerCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 3) {
		// prepare
		IComposerMethodPtr lCreateBuzzer = BuzzerLightComposer::CreateTxBuzzer::instance(composer_, 
			std::string(args[0]), std::string(args[1]), std::string(args[2]),
			boost::bind(&CreateBuzzerCommand::created, shared_from_this(), _1));
		// async process
		lCreateBuzzer->process(boost::bind(&CreateBuzzerCommand::error, shared_from_this(), _1, _2));
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}

void CreateBuzzCommand::process(const std::vector<std::string>& args) {
	if (args.size() >= 1) {
		// reset
		feeSent_ = false;
		buzzSent_ = false;

		std::vector<std::string> lBuzzers;
		if (args.size() > 1) lBuzzers.insert(lBuzzers.end(), ++args.begin(), args.end());

		// prepare
		IComposerMethodPtr lCreateBuzz = BuzzerLightComposer::CreateTxBuzz::instance(composer_, args[0], lBuzzers,
			boost::bind(&CreateBuzzCommand::created, shared_from_this(), _1));
		// async process
		lCreateBuzz->process(boost::bind(&CreateBuzzCommand::error, shared_from_this(), _1, _2));
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}

void BuzzerSubscribeCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 1) {
		// prepare
		IComposerMethodPtr lCreateBuzz = BuzzerLightComposer::CreateTxBuzzerSubscribe::instance(composer_, args[0],
			boost::bind(&BuzzerSubscribeCommand::created, shared_from_this(), _1));
		// async process
		lCreateBuzz->process(boost::bind(&BuzzerSubscribeCommand::error, shared_from_this(), _1, _2));
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}

void BuzzerUnsubscribeCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 1) {
		// prepare
		IComposerMethodPtr lCreateBuzz = BuzzerLightComposer::CreateTxBuzzerUnsubscribe::instance(composer_, args[0],
			boost::bind(&BuzzerUnsubscribeCommand::created, shared_from_this(), _1));
		// async process
		lCreateBuzz->process(boost::bind(&BuzzerUnsubscribeCommand::error, shared_from_this(), _1, _2));
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}

void LoadBuzzfeedCommand::process(const std::vector<std::string>& args) {
	// clean-up
	chains_.clear();
	loaded_.clear();

	// args - from
	uint64_t lFrom = 0; // most recent
	if (args.size() == 1) {
		lFrom = (uint64_t)(boost::lexical_cast<uint64_t>(args[0]));
	}

	// collect all sources
	composer_->requestProcessor()->collectChains(chains_);

	// spead requests
	for (std::vector<uint256>::iterator lChain = chains_.begin(); lChain != chains_.end(); lChain++) {
		//
		IComposerMethodPtr lCreateBuzz = BuzzerLightComposer::LoadBuzzfeed::instance(composer_, *lChain, lFrom,
			boost::bind(&LoadBuzzfeedCommand::buzzfeedLoaded, shared_from_this(), _1, _2));
		// async process
		lCreateBuzz->process(boost::bind(&LoadBuzzfeedCommand::error, shared_from_this(), _1, _2));
	}
}

void LoadBuzzfeedCommand::display(BuzzfeedItemPtr item) {
	//
	std::list<BuzzfeedItemPtr> lFeed;
	item->feed(lFeed);

	for (std::list<BuzzfeedItemPtr>::iterator lBuzz = lFeed.begin(); lBuzz != lFeed.end(); lBuzz++) {
		std::cout << (*lBuzz)->toString() << std::endl << std::endl;

		display(*lBuzz);
	}
}

void LoadBuzzfeedCommand::buzzfeedLoaded(const std::vector<BuzzfeedItem>& feed, const uint256& chain) {
	//
	loaded_.insert(chain);
	std::cout << strprintf("chain: %s/%d, n = %d", chain.toHex(), feed.size(), loaded_.size()) << std::endl;

	// for real client - mobile, for example, we can start to display feeds as soon as they arrived
	if (loaded_.size() < chains_.size()) {
		// merge feed
		buzzFeed_->merge(feed);
	} else {
		// merge and notify
		buzzFeed_->merge(feed, true);
		// get list
		std::list<BuzzfeedItemPtr> lFeed;
		buzzFeed_->feed(lFeed);
		// display
		std::cout << std::endl;
		display(buzzFeed_->toItem());

		done_();
	}
}

void BuzzfeedListCommand::display(BuzzfeedItemPtr item) {
	//
	std::list<BuzzfeedItemPtr> lFeed;
	item->feed(lFeed);

	for (std::list<BuzzfeedItemPtr>::iterator lBuzz = lFeed.begin(); lBuzz != lFeed.end(); lBuzz++) {
		std::cout << (*lBuzz)->toString() << std::endl << std::endl;

		display(*lBuzz);
	}
}

void BuzzfeedListCommand::process(const std::vector<std::string>& args) {
	// display
	std::cout << std::endl;
	display(buzzFeed_->toItem());

	done_();
}

void BuzzLikeCommand::process(const std::vector<std::string>& args) {
	if (args.size() == 1) {
		// prepare
		uint256 lBuzzId;
		lBuzzId.setHex(args[0]);
		//
		BuzzfeedItemPtr lItem = buzzFeed_->locateBuzz(lBuzzId);
		//
		if (lItem) {
			IComposerMethodPtr lCreateBuzz = BuzzerLightComposer::CreateTxBuzzLike::instance(composer_, lItem->buzzChainId(), lItem->buzzId(),
				boost::bind(&BuzzLikeCommand::created, shared_from_this(), _1));
			// async process
			lCreateBuzz->process(boost::bind(&BuzzLikeCommand::error, shared_from_this(), _1, _2));
		} else {
			gLog().writeClient(Log::CLIENT, std::string(": buzz was not found in local feed"));
			done_();
		}
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
		done_();
	}
}

void CreateBuzzReplyCommand::process(const std::vector<std::string>& args) {
	if (args.size() >= 2) {
		// reset
		feeSent_ = false;
		buzzSent_ = false;

		// prepare
		uint256 lBuzzId;
		lBuzzId.setHex(args[0]);

		std::vector<std::string> lBuzzers;
		if (args.size() >= 3) lBuzzers.insert(lBuzzers.end(), ++(++args.begin()), args.end());

		//
		BuzzfeedItemPtr lItem = buzzFeed_->locateBuzz(lBuzzId);
		//
		if (lItem) {
			IComposerMethodPtr lCreateBuzz = BuzzerLightComposer::CreateTxBuzzReply::instance(composer_, lItem->buzzChainId(), lItem->buzzId(), args[1], lBuzzers,
				boost::bind(&CreateBuzzReplyCommand::created, shared_from_this(), _1));
			// async process
			lCreateBuzz->process(boost::bind(&CreateBuzzReplyCommand::error, shared_from_this(), _1, _2));
		} else {
			gLog().writeClient(Log::CLIENT, std::string(": buzz was not found in local feed"));
			done_();
		}
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
		done_();
	}
}
