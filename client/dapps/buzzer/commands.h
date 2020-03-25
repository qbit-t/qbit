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
			//
			if (!composer_->requestProcessor()->broadcastTransaction(*lLinkedCtx)) {
				gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
				composer_->wallet()->resetCache();
				composer_->wallet()->prepareCache();

				done_();
				return;
			} else {
				std::cout << " fee: " << (*lLinkedCtx)->tx()->id().toHex() << std::endl;
			}
		}

		if (composer_->requestProcessor()->broadcastTransaction(ctx)) {
			std::cout << "buzz: " << ctx->tx()->id().toHex() << std::endl;		
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

} // qbit

#endif