// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_COMMANDS_H
#define QBIT_BUZZER_COMMANDS_H

#include "../../icommand.h"
#include "../../../ipeer.h"
#include "../../../log/log.h"
#include "composer.h"

namespace qbit {

class CreateBuzzerCommand;
typedef std::shared_ptr<CreateBuzzerCommand> CreateBuzzerCommandPtr;

class CreateBuzzerCommand: public ICommand, public std::enable_shared_from_this<CreateBuzzerCommand> {
public:
	CreateBuzzerCommand(BuzzerLightComposerPtr composer, doneFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("createBuzzer"); 
		return lSet;
	}

	void help() {
		std::cout << "createBuzzer <name> \"<alias>\" \"<description>\"" << std::endl;
		std::cout << "\tCreate a new buzzer." << std::endl;
		std::cout << "\t<name>         - required, buzzer anem should be prefixed with @ (up to 64 symbols)" << std::endl;
		std::cout << "\t<alias>        - required, buzzer alias, should be easy human-readable (up to 64 bytes)" << std::endl;
		std::cout << "\t<description>  - required, short description of your buzzer (up to 256 bytes)" << std::endl;
		std::cout << "\texample:\n\t\t>createBuzzer @mybuzzer \"I'm buzzer\" \"Buzzing the world\"" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneFunction done) { 
		return std::make_shared<CreateBuzzerCommand>(composer, done); 
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		if (composer_->requestProcessor()->broadcastTransaction(ctx)) {
			std::cout << ctx->tx()->id().toHex() << std::endl;		
		} else {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
		}

		done_();
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_();
	}	

private:
	BuzzerLightComposerPtr composer_;
	doneFunction done_;
};

class CreateBuzzCommand;
typedef std::shared_ptr<CreateBuzzCommand> CreateBuzzCommandPtr;

class CreateBuzzCommand: public ICommand, public std::enable_shared_from_this<CreateBuzzCommand> {
public:
	CreateBuzzCommand(BuzzerLightComposerPtr composer, doneFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("createBuzz"); 
		lSet.insert("buzz"); 
		return lSet;
	}

	void help() {
		std::cout << "createBuzz | buzz \"<message>\"" << std::endl;
		std::cout << "\tCreate new buzz - a message from your default buzzer. Message will hit all your buzzer subscribers." << std::endl;
		std::cout << "\t<message> - required, buzz message body; can contains multibyte sequences, i.e. 😀 (up to 512 bytes)" << std::endl;
		std::cout << "\texample:\n\t\t>buzz \"It's sunny day! 🌞\"" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneFunction done) { 
		return std::make_shared<CreateBuzzCommand>(composer, done); 
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		// push linked and newly created txs; order matters
		for (std::list<TransactionContextPtr>::iterator lLinkedCtx = ctx->linkedTxs().begin(); lLinkedCtx != ctx->linkedTxs().end(); lLinkedCtx++) {
			if (!composer_->requestProcessor()->sendTransaction(*lLinkedCtx,
					SentTransaction::instance(
						boost::bind(&CreateBuzzCommand::feeSent, shared_from_this(), _1, _2),
						boost::bind(&CreateBuzzCommand::timeout, shared_from_this())))) {
				gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
				composer_->wallet()->resetCache();
				composer_->wallet()->prepareCache();
			}
		}

		if (!composer_->requestProcessor()->sendTransaction(ctx,
				SentTransaction::instance(
					boost::bind(&CreateBuzzCommand::buzzSent, shared_from_this(), _1, _2),
					boost::bind(&CreateBuzzCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
		}
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzz creation.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_();
	}

	void feeSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
			}			
		} else {
			std::cout << " fee: " << tx.toHex() << std::endl;
		}

		feeSent_ = true;
		if (feeSent_ && buzzSent_) done_();
	}

	void buzzSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
			}			
		} else {
			std::cout << "buzz: " << tx.toHex() << std::endl;
		}

		buzzSent_ = true;
		if (feeSent_ && buzzSent_) done_();
	}

private:
	BuzzerLightComposerPtr composer_;
	doneFunction done_;

	bool feeSent_ = false;
	bool buzzSent_ = false;
};

class BuzzerSubscribeCommand;
typedef std::shared_ptr<BuzzerSubscribeCommand> BuzzerSubscribeCommandPtr;

class BuzzerSubscribeCommand: public ICommand, public std::enable_shared_from_this<BuzzerSubscribeCommand> {
public:
	BuzzerSubscribeCommand(BuzzerLightComposerPtr composer, doneFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzerSubscribe"); 
		lSet.insert("bsub"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzerSubscribe | bsub <publisher>" << std::endl;
		std::cout << "\tSubscribe on publishers' messages. You will hit all publisher buzzs'." << std::endl;
		std::cout << "\t<publisher> - required, publishers buzzer - name prefixed with @" << std::endl;
		std::cout << "\texample:\n\t\t>bsub @other" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneFunction done) { 
		return std::make_shared<BuzzerSubscribeCommand>(composer, done); 
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		if (composer_->requestProcessor()->broadcastTransaction(ctx)) {
			std::cout << ctx->tx()->id().toHex() << std::endl;		
		} else {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
		}

		done_();
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_();
	}	

private:
	BuzzerLightComposerPtr composer_;
	doneFunction done_;
};

class BuzzerUnsubscribeCommand;
typedef std::shared_ptr<BuzzerUnsubscribeCommand> BuzzerUnsubscribeCommandPtr;

class BuzzerUnsubscribeCommand: public ICommand, public std::enable_shared_from_this<BuzzerUnsubscribeCommand> {
public:
	BuzzerUnsubscribeCommand(BuzzerLightComposerPtr composer, doneFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzerUnsubscribe"); 
		lSet.insert("busub"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzerUnsubscribe | busub <publisher>" << std::endl;
		std::cout << "\tUnsubscribe from publisher." << std::endl;
		std::cout << "\t<publisher> - required, publishers buzzer - name prefixed with @" << std::endl;
		std::cout << "\texample:\n\t\t>busub @other" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneFunction done) { 
		return std::make_shared<BuzzerUnsubscribeCommand>(composer, done); 
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		if (composer_->requestProcessor()->broadcastTransaction(ctx)) {
			std::cout << ctx->tx()->id().toHex() << std::endl;		
		} else {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
		}

		done_();
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_();
	}	

private:
	BuzzerLightComposerPtr composer_;
	doneFunction done_;
};

class LoadBuzzfeedCommand;
typedef std::shared_ptr<LoadBuzzfeedCommand> LoadBuzzfeedCommandPtr;

class LoadBuzzfeedCommand: public ICommand, public std::enable_shared_from_this<LoadBuzzfeedCommand> {
public:
	LoadBuzzfeedCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneFunction done): composer_(composer), buzzFeed_(buzzFeed), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzFeed"); 
		lSet.insert("feed"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzFeed | feed [time]" << std::endl;
		std::cout << "\tLoad current buzzfeed. Limited to last 30 events/buzzes/likes/rebuzzes." << std::endl;
		std::cout << "\t[time] - options, time in seconds" << std::endl;
		std::cout << "\texample:\n\t\t>feed" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneFunction done) { 
		return std::make_shared<LoadBuzzfeedCommand>(composer, buzzFeed, done); 
	}

	// callbacks
	void buzzfeedLoaded(const std::vector<BuzzfeedItem>& /*feed*/, const uint256& /*chain*/);

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_();
	}	

private:
	BuzzerLightComposerPtr composer_;
	BuzzfeedPtr buzzFeed_;
	doneFunction done_;
	std::vector<uint256> chains_;
	std::set<uint256> loaded_; 
};

class BuzzfeedListCommand;
typedef std::shared_ptr<BuzzfeedListCommand> BuzzfeedListCommandPtr;

class BuzzfeedListCommand: public ICommand, public std::enable_shared_from_this<BuzzfeedListCommand> {
public:
	BuzzfeedListCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneFunction done): composer_(composer), buzzFeed_(buzzFeed), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzList"); 
		lSet.insert("list"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzList | list" << std::endl;
		std::cout << "\tShow current buzzfeed." << std::endl;
		std::cout << "\texample:\n\t\t>list" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneFunction done) { 
		return std::make_shared<BuzzfeedListCommand>(composer, buzzFeed, done); 
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_();
	}	

private:
	BuzzerLightComposerPtr composer_;
	BuzzfeedPtr buzzFeed_;
	doneFunction done_;
};

} // qbit

#endif