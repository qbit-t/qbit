// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_COMMANDS_H
#define QBIT_BUZZER_COMMANDS_H

#include "../../icommand.h"
#include "../../../ipeer.h"
#include "../../../log/log.h"
#include "buzzercomposer.h"

namespace qbit {

class CreateBuzzerCommand;
typedef std::shared_ptr<CreateBuzzerCommand> CreateBuzzerCommandPtr;

class CreateBuzzerCommand: public ICommand, public std::enable_shared_from_this<CreateBuzzerCommand> {
public:
	CreateBuzzerCommand(BuzzerLightComposerPtr composer, doneTransactionsWithErrorFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("createBuzzer"); 
		return lSet;
	}

	void help() {
#if defined(CUBIX_MOD)
		std::cout << "createBuzzer <name> \"<alias>\" \"<description>\" \"[avatar_path]\" \"[-s <000x000>]\" \"[header_path]\"" << std::endl;
#else
		std::cout << "createBuzzer <name> \"<alias>\" \"<description>\"" << std::endl;
#endif
		std::cout << "\tCreate a new buzzer." << std::endl;
		std::cout << "\t<name>			- required, buzzer name should be prefixed with @ (up to 64 symbols)" << std::endl;
		std::cout << "\t<alias>			- required, buzzer alias, should be easy human-readable (up to 64 bytes)" << std::endl;
		std::cout << "\t<description>	- required, short description of your buzzer (up to 256 bytes)" << std::endl;
#if defined(CUBIX_MOD)		
		std::cout << "\t[avatar_path]	- optional, cubix powered, path to jpeg or png image, that will be used as avatar picture" << std::endl;
		std::cout << "\t[-s <000x000>]	- optional, preview size of avatar in format 000x000" << std::endl;
		std::cout << "\t[header_path]	- optional, cubix powered, path to jpeg or png image, that will be used header for your buzzer" << std::endl;
#endif		
		std::cout << "\texample:\n\t\t>createBuzzer @mybuzzer \"I'm buzzer\" \"Buzzing the world\"" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneTransactionsWithErrorFunction done) { 
		return std::make_shared<CreateBuzzerCommand>(composer, done); 
	}

	void setUploadAvatar(ICommandPtr uploadAvatar) {
		uploadAvatar_ = uploadAvatar;
	}

	void setUploadHeader(ICommandPtr uploadHeader) {
		uploadHeader_ = uploadHeader;
	}

	// callbacks
	void buzzerCreated(TransactionContextPtr, Transaction::UnlinkedOutPtr);
	void buzzerSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors);

	void avatarUploaded(TransactionPtr, const ProcessingError&);
	void headerUploaded(TransactionPtr, const ProcessingError&);

	void createBuzzerInfo();
	void buzzerInfoCreated(TransactionContextPtr);
	void buzzerInfoSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors);

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzer creation.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(nullptr, nullptr, ProcessingError(code, message));
	}

private:
	BuzzerLightComposerPtr composer_;
	doneTransactionsWithErrorFunction done_;

	ICommandPtr uploadAvatar_;
	ICommandPtr uploadHeader_;

	TransactionPtr avatarTx_;
	TransactionPtr headerTx_;

	TransactionPtr buzzerTx_;
	TransactionPtr buzzerInfoTx_;

	std::vector<std::string> args_;
	Transaction::UnlinkedOutPtr buzzerOut_;

	IPeerPtr peer_;
};

class CreateBuzzerInfoCommand;
typedef std::shared_ptr<CreateBuzzerInfoCommand> CreateBuzzerInfoCommandPtr;

class CreateBuzzerInfoCommand: public ICommand, public std::enable_shared_from_this<CreateBuzzerInfoCommand> {
public:
	CreateBuzzerInfoCommand(BuzzerLightComposerPtr composer, doneTransactionsWithErrorFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("createBuzzerInfo"); 
		return lSet;
	}

	void help() {
#if defined(CUBIX_MOD)
		std::cout << "createBuzzerInfo \"<alias>\" \"<description>\" \"[avatar_path]\" \"[-s <000x000>]\" \"[header_path]\"" << std::endl;
#else
		std::cout << "createBuzzerInfo \"<alias>\" \"<description>\"" << std::endl;
#endif
		std::cout << "\tCreate new buzzer info for current buzzer." << std::endl;
		std::cout << "\t<alias>			- required, buzzer alias, should be easy human-readable (up to 64 bytes)" << std::endl;
		std::cout << "\t<description>	- required, short description of your buzzer (up to 256 bytes)" << std::endl;
#if defined(CUBIX_MOD)		
		std::cout << "\t[avatar_path]	- optional, cubix powered, path to jpeg or png image, that will be used as avatar picture" << std::endl;
		std::cout << "\t[-s <000x000>]	- optional, preview size of avatar in format 000x000" << std::endl;
		std::cout << "\t[header_path]	- optional, cubix powered, path to jpeg or png image, that will be used header for your buzzer" << std::endl;
#endif		
		std::cout << "\texample:\n\t\t>createBuzzerInfo \"I'm buzzer\" \"Buzzing the world\"" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneTransactionsWithErrorFunction done) { 
		return std::make_shared<CreateBuzzerInfoCommand>(composer, done); 
	}

	void setUploadAvatar(ICommandPtr uploadAvatar) {
		uploadAvatar_ = uploadAvatar;
	}

	void setUploadHeader(ICommandPtr uploadHeader) {
		uploadHeader_ = uploadHeader;
	}

	// callbacks
	void avatarUploaded(TransactionPtr, const ProcessingError&);
	void headerUploaded(TransactionPtr, const ProcessingError&);

	void createBuzzerInfo();
	void buzzerInfoCreated(TransactionContextPtr);
	void buzzerInfoSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors);

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzer info creation.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(nullptr, nullptr, ProcessingError(code, message));
	}

private:
	BuzzerLightComposerPtr composer_;
	doneTransactionsWithErrorFunction done_;

	ICommandPtr uploadAvatar_;
	ICommandPtr uploadHeader_;

	TransactionPtr avatarTx_;
	TransactionPtr headerTx_;

	TransactionPtr buzzerInfoTx_;

	std::vector<std::string> args_;
};

class CreateBuzzCommand;
typedef std::shared_ptr<CreateBuzzCommand> CreateBuzzCommandPtr;

class CreateBuzzCommand: public ICommand, public std::enable_shared_from_this<CreateBuzzCommand> {
public:
	CreateBuzzCommand(BuzzerLightComposerPtr composer, doneWithErrorFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("createBuzz"); 
		lSet.insert("buzz"); 
		lSet.insert("bzz"); 
		return lSet;
	}

	void help() {
		std::cout << "createBuzz | buzz | bzz \"<message>\" [@buzzer1 @buzzer2 ...] [mediafile1 mediafile2 ...]" << std::endl;
		std::cout << "\tCreate new buzz - a message from your default buzzer. Message will hit all your buzzer subscribers." << std::endl;
		std::cout << "\t<message> 	- required, buzz message body; can contains multibyte sequences, i.e. 😀 (up to 512 bytes)" << std::endl;
		std::cout << "\t[@buzzer] 	- optional, notify/tag buzzers" << std::endl;
		std::cout << "\t[mediafile] - optional, media files to upload" << std::endl;
		std::cout << "\texample:\n\t\t>buzz \"It's sunny day! 🌞\" @buddy" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneWithErrorFunction done) { 
		return std::make_shared<CreateBuzzCommand>(composer, done); 
	}

	// retry
	bool retry() {
		//
		if (ctx_ == nullptr) return false;
		created(ctx_);
		return true;
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		ctx_ = ctx;
		//
		// push linked and newly created txs; order matters
		// NOTE: fee and parent tx should start processing on the one node
		IPeerPtr lPeer;
		for (std::list<TransactionContextPtr>::iterator lLinkedCtx = ctx->linkedTxs().begin(); lLinkedCtx != ctx->linkedTxs().end(); lLinkedCtx++) {
			if (!(lPeer = composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), *lLinkedCtx,
					SentTransaction::instance(
						boost::bind(&CreateBuzzCommand::feeSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
						boost::bind(&CreateBuzzCommand::timeout, shared_from_this()))))) {
				gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
				composer_->wallet()->resetCache();
				composer_->wallet()->prepareCache();
				done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
			}
		}

		if (!composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), ctx, 
				SentTransaction::instance(
					boost::bind(&CreateBuzzCommand::buzzSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&CreateBuzzCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzz creation.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

	void feeSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}
			// done
			return;	
		} else {
			std::cout << " fee: " << tx.toHex() << std::endl;
			//
			TransactionContextPtr lFee = ctx_->locateByType(Transaction::FEE);
			for (std::list<Transaction::NetworkUnlinkedOutPtr>::iterator lOut = lFee->externalOuts().begin(); 
																		lOut != lFee->externalOuts().end(); lOut++) {
				composer_->wallet()->updateOut(*lOut, ctx_->tx()->id(), ctx_->tx()->type());
			}
		}

		feeSent_ = true;
		if (feeSent_ && buzzSent_) done_(ProcessingError());
	}

	void buzzSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return;
		} else {
			std::cout << "buzz: " << tx.toHex() << std::endl;
		}

		buzzSent_ = true;
		if (feeSent_ && buzzSent_) done_(ProcessingError());
	}

	void setUploadMedia(ICommandPtr uploadMedia) {
		uploadMedia_ = uploadMedia;
	}

	void uploadNextMedia();
	void mediaUploaded(TransactionPtr, const ProcessingError&);

	void setMediaUploaded(doneTransactionWithErrorFunction mediaUploaded) {
		mediaUploaded_ = mediaUploaded;
	}

	void createBuzz();

private:
	BuzzerLightComposerPtr composer_;
	doneWithErrorFunction done_;
	doneTransactionWithErrorFunction mediaUploaded_;

	std::string body_;
	std::vector<std::string> buzzers_;

	ICommandPtr uploadMedia_;
	std::vector<BuzzerMediaPointer> mediaPointers_;
	std::vector<std::string> mediaFiles_;

	TransactionContextPtr ctx_;

	bool feeSent_ = false;
	bool buzzSent_ = false;
};

class BuzzerSubscribeCommand;
typedef std::shared_ptr<BuzzerSubscribeCommand> BuzzerSubscribeCommandPtr;

class BuzzerSubscribeCommand: public ICommand, public std::enable_shared_from_this<BuzzerSubscribeCommand> {
public:
	BuzzerSubscribeCommand(BuzzerLightComposerPtr composer, doneWithErrorFunction done): composer_(composer), done_(done) {}

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

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneWithErrorFunction done) { 
		return std::make_shared<BuzzerSubscribeCommand>(composer, done); 
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		if (!composer_->requestProcessor()->sendTransaction(ctx,
				SentTransaction::instance(
					boost::bind(&BuzzerSubscribeCommand::sent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&BuzzerSubscribeCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void sent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return;
		} else {
			std::cout << tx.toHex() << std::endl;
		}

		done_(ProcessingError());
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzer subscription.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

private:
	BuzzerLightComposerPtr composer_;
	doneWithErrorFunction done_;
};

class BuzzerUnsubscribeCommand;
typedef std::shared_ptr<BuzzerUnsubscribeCommand> BuzzerUnsubscribeCommandPtr;

class BuzzerUnsubscribeCommand: public ICommand, public std::enable_shared_from_this<BuzzerUnsubscribeCommand> {
public:
	BuzzerUnsubscribeCommand(BuzzerLightComposerPtr composer, doneWithErrorFunction done): composer_(composer), done_(done) {}

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

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneWithErrorFunction done) { 
		return std::make_shared<BuzzerUnsubscribeCommand>(composer, done); 
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		if (!composer_->requestProcessor()->sendTransaction(ctx,
				SentTransaction::instance(
					boost::bind(&BuzzerUnsubscribeCommand::sent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&BuzzerUnsubscribeCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void sent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return;
		} else {
			std::cout << tx.toHex() << std::endl;
		}

		done_(ProcessingError());
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzer unsubscription.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

private:
	BuzzerLightComposerPtr composer_;
	doneWithErrorFunction done_;
};

class LoadBuzzfeedCommand;
typedef std::shared_ptr<LoadBuzzfeedCommand> LoadBuzzfeedCommandPtr;

class LoadBuzzfeedCommand: public ICommand, public std::enable_shared_from_this<LoadBuzzfeedCommand> {
public:
	LoadBuzzfeedCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done): 
		composer_(composer), buzzFeed_(buzzFeed), done_(done) {
		localBuzzFeed_ = Buzzfeed::instance(buzzFeed);
	}
	LoadBuzzfeedCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done): 
		composer_(composer), buzzFeed_(buzzFeed), buzzfeedReady_(buzzfeedReady), done_(done) {
		localBuzzFeed_ = Buzzfeed::instance(buzzFeed);
	}

	virtual void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzFeed"); 
		lSet.insert("feed"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzFeed | feed [more]" << std::endl;
		std::cout << "\tLoad current buzzfeed. Limited to last 30 events/buzzes/likes/rebuzzes." << std::endl;
		std::cout << "\t[more] - optional, flag to feed more from last buzzfeed item" << std::endl;
		std::cout << "\texample:\n\t\t>feed" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadBuzzfeedCommand>(composer, buzzFeed, done); 
	}
	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadBuzzfeedCommand>(composer, buzzFeed, buzzfeedReady, done); 
	}

	// callbacks
	void buzzfeedLoaded(const std::vector<BuzzfeedItem>& /*feed*/, const uint256& /*chain*/, int /*requests*/);
	void buzzesLoaded(const std::vector<BuzzfeedItem>& /*feed*/, const uint256& /*chain*/);

	void processPengingInfos();
	void buzzerInfoLoaded(const std::vector<TransactionPtr>&);

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

	void warning(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzes loading.");
	}

	void show();
	void display(const std::vector<BuzzfeedItemPtr>&);

	BuzzfeedPtr buzzfeed() {
		if (!appendFeed()) return buzzFeed_;
		return localBuzzFeed_;
	}

	virtual bool appendFeed() {
		if (from_.size()) return true;
		return false;
	}

protected:
	BuzzerLightComposerPtr composer_;
	BuzzfeedPtr buzzFeed_;
	BuzzfeedPtr localBuzzFeed_;
	buzzfeedReadyFunction buzzfeedReady_;
	doneWithErrorFunction done_;
	std::vector<uint256> chains_;
	std::map<uint256, int> loaded_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pending_;
	std::set<uint256> pendingLoaded_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pengindChainInfos_;
	std::map<uint256 /*info tx*/, Buzzer::Info> pendingInfos_;
	int pendingChainInfosLoaded_ = 0;
	std::map<uint256 /*chain*/, std::vector<BuzzfeedPublisherFrom>> from_;
};

class LoadBuzzfeedByBuzzCommand;
typedef std::shared_ptr<LoadBuzzfeedByBuzzCommand> LoadBuzzfeedByBuzzCommandPtr;

class LoadBuzzfeedByBuzzCommand: public LoadBuzzfeedCommand {
public:
	LoadBuzzfeedByBuzzCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done): 
		LoadBuzzfeedCommand (composer, buzzFeed, done) {
	}
	LoadBuzzfeedByBuzzCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done): 
		LoadBuzzfeedCommand (composer, buzzFeed, buzzfeedReady, done) {
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzFeedByBuzz"); 
		lSet.insert("feedByByzz"); 
		lSet.insert("bfeed"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzFeedByBuzz | feedByByzz | bfeed <buzz_id> [more]" << std::endl;
		std::cout << "\tLoad buzzfeed by given buzz. Limited to last 30 replies." << std::endl;
		std::cout << "\t<buzz_id> 	- required, buzz id to fetch thread" << std::endl;
		std::cout << "\t[more]		- optional, flag to feed more from last buzzfeed item" << std::endl;
		std::cout << "\texample:\n\t\t>bfeed" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadBuzzfeedByBuzzCommand>(composer, buzzFeed, done); 
	}
	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadBuzzfeedByBuzzCommand>(composer, buzzFeed, buzzfeedReady, done); 
	}

	virtual bool appendFeed() {
		if (from_) return true;
		return false;
	}

protected:
	uint64_t from_ = 0;
};

class LoadMessagesCommand;
typedef std::shared_ptr<LoadMessagesCommand> LoadMessagesCommandPtr;

class LoadMessagesCommand: public LoadBuzzfeedCommand {
public:
	LoadMessagesCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done): 
		LoadBuzzfeedCommand (composer, buzzFeed, done) {
	}
	LoadMessagesCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done): 
		LoadBuzzfeedCommand (composer, buzzFeed, buzzfeedReady, done) {
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("messagesByConversation"); 
		lSet.insert("messages"); 
		lSet.insert("msgs"); 
		return lSet;
	}

	void help() {
		std::cout << "messagesByConversation | messages | msgs <conversation_id> [more]" << std::endl;
		std::cout << "\tLoad messages by given conversation. Limited to last 50 msgs." << std::endl;
		std::cout << "\t<conversation_id> 	- required, conversation id to fetch thread" << std::endl;
		std::cout << "\t[more]				- optional, flag to feed more from last message" << std::endl;
		std::cout << "\texample:\n\t\t>msgs 28c9a2debf0996193684979723a5a7c859a0a3cb0876125ea066b1e5b90d9902" << std::endl << std::endl;
	}

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadMessagesCommand>(composer, buzzFeed, done); 
	}
	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadMessagesCommand>(composer, buzzFeed, buzzfeedReady, done); 
	}

	virtual bool appendFeed() {
		if (from_) return true;
		return false;
	}

protected:
	uint64_t from_ = 0;
};

class LoadBuzzfeedByBuzzerCommand;
typedef std::shared_ptr<LoadBuzzfeedByBuzzerCommand> LoadBuzzfeedByBuzzerCommandPtr;

class LoadBuzzfeedByBuzzerCommand: public LoadBuzzfeedCommand {
public:
	LoadBuzzfeedByBuzzerCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done): 
		LoadBuzzfeedCommand (composer, buzzFeed, done) {
	}
	LoadBuzzfeedByBuzzerCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done): 
		LoadBuzzfeedCommand (composer, buzzFeed, buzzfeedReady, done) {
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzFeedByBuzzer"); 
		lSet.insert("feedByByzzer"); 
		lSet.insert("rfeed"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzFeedByBuzzer | feedByByzzer | rfeed <@buzzer> [more]" << std::endl;
		std::cout << "\tLoad buzzfeed by given buzzer. Limited to last 30 events." << std::endl;
		std::cout << "\t<@buzzer>	- required, buzzer to fetch thread" << std::endl;
		std::cout << "\t[more]		- optional, flag to feed more from last buzzfeed item" << std::endl;
		std::cout << "\texample:\n\t\t>rfeed" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadBuzzfeedByBuzzerCommand>(composer, buzzFeed, done); 
	}
	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadBuzzfeedByBuzzerCommand>(composer, buzzFeed, buzzfeedReady, done); 
	}	
};

class LoadBuzzesGlobalCommand;
typedef std::shared_ptr<LoadBuzzesGlobalCommand> LoadBuzzesGlobalCommandPtr;

class LoadBuzzesGlobalCommand: public LoadBuzzfeedCommand {
public:
	LoadBuzzesGlobalCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done): 
		LoadBuzzfeedCommand (composer, buzzFeed, done) {
	}
	LoadBuzzesGlobalCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done): 
		LoadBuzzfeedCommand (composer, buzzFeed, buzzfeedReady, done) {
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzFeedGlobal"); 
		lSet.insert("feedGlobal"); 
		lSet.insert("gfeed"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzFeedGlobal | feedGlobal | gfeed [more]" << std::endl;
		std::cout << "\tLoad global buzzfeed. Ordered by most trusted buzzers." << std::endl;
		std::cout << "\t[more]		- optional, flag to feed more from last buzzfeed item" << std::endl;
		std::cout << "\texample:\n\t\t>gfeed" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadBuzzesGlobalCommand>(composer, buzzFeed, done); 
	}
	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadBuzzesGlobalCommand>(composer, buzzFeed, buzzfeedReady, done); 
	}

	virtual bool appendFeed() {
		if (last_.size()) return true;
		return false;
	}

private:
	std::map<uint256, BuzzfeedItemPtr> last_;
};

class LoadBuzzfeedByTagCommand;
typedef std::shared_ptr<LoadBuzzfeedByTagCommand> LoadBuzzfeedByTagCommandPtr;

class LoadBuzzfeedByTagCommand: public LoadBuzzfeedCommand {
public:
	LoadBuzzfeedByTagCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done): 
		LoadBuzzfeedCommand (composer, buzzFeed, done) {
	}
	LoadBuzzfeedByTagCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done): 
		LoadBuzzfeedCommand (composer, buzzFeed, buzzfeedReady, done) {
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzFeedByTag");
		lSet.insert("feedByTag"); 
		lSet.insert("tfeed"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzFeedByTag | feedByTag | tfeed <tag> [more]" << std::endl;
		std::cout << "\tLoad buzzfeed by tag. Limited to last 30 events." << std::endl;
		std::cout << "\t<tag>	- required, #tag" << std::endl;
		std::cout << "\t[more]	- optional, flag to feed more from last buzzfeed item" << std::endl;
		std::cout << "\texample:\n\t\t>tfeed" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadBuzzfeedByTagCommand>(composer, buzzFeed, done); 
	}
	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, buzzfeedReadyFunction buzzfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadBuzzfeedByTagCommand>(composer, buzzFeed, buzzfeedReady, done); 
	}

	virtual bool appendFeed() {
		if (last_.size()) return true;
		return false;
	}

private:
	std::map<uint256, BuzzfeedItemPtr> last_;
};

class LoadHashTagsCommand;
typedef std::shared_ptr<LoadHashTagsCommand> LoadHashTagsCommandPtr;

class LoadHashTagsCommand: public ICommand, public std::enable_shared_from_this<LoadHashTagsCommand> {
public:
	LoadHashTagsCommand(BuzzerLightComposerPtr composer, hashTagsLoadedWithErrorFunction done): 
		composer_(composer), done_(done) {
	}

	LoadHashTagsCommand(BuzzerLightComposerPtr composer, bool display, hashTagsLoadedWithErrorFunction done): 
		composer_(composer), display_(display), done_(done) {
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("searchHashTags");
		lSet.insert("searchTags"); 
		lSet.insert("st"); 
		return lSet;
	}

	void help() {
		std::cout << "searchHashTags | searchTags | st <#tag_part>" << std::endl;
		std::cout << "\tSearch up to 5 tags by given pattern." << std::endl;
		std::cout << "\t<#tag_part>	- required, #tag part" << std::endl;
		std::cout << "\texample:\n\t\t>st #my" << std::endl << std::endl;
	}	

	void tagsLoaded(const std::vector<Buzzer::HashTag>& /*feed*/, const uint256& /*chain*/, int /*count*/);

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(std::string(), std::vector<Buzzer::HashTag>(), ProcessingError(code, message));
	}

	void warning(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during tags loading.");
	}

	void merge(const std::vector<Buzzer::HashTag>& feed, const uint256& chain) {
		//
		std::map<uint256, std::map<uint160, Buzzer::HashTag>>::iterator lChain = feed_.find(chain);
		if (!feed_.size() || lChain == feed_.end() || loaded_[chain] == 1) {
			for (std::vector<Buzzer::HashTag>::const_iterator lItem = feed.begin(); lItem != feed.end(); lItem++) {
				//
				feed_[chain][lItem->hash()] = *lItem;
			}
		} else {
			if (loaded_[chain] > 1) {
				for (std::vector<Buzzer::HashTag>::const_iterator lItem = feed.begin(); lItem != feed.end(); lItem++) {
					if (lChain->second.find(lItem->hash()) == lChain->second.end())
						feed_[chain].erase(lItem->hash());
				}
			}
		}
	}

	void display() {
		if (!display_) return;

		std::map<uint160, Buzzer::HashTag> lUniqueTags;

		for (std::map<uint256, std::map<uint160, Buzzer::HashTag>>::iterator lItem = feed_.begin(); lItem != feed_.end(); lItem++) {
			for (std::map<uint160, Buzzer::HashTag>::iterator lTagsSet = lItem->second.begin(); lTagsSet != lItem->second.end(); lTagsSet++) {
				lUniqueTags[lTagsSet->first] = lTagsSet->second;
			}
		}

		for (std::map<uint160, Buzzer::HashTag>::iterator lItem = lUniqueTags.begin(); lItem != lUniqueTags.end(); lItem++) {
			std::cout << lItem->second.tag() << std::endl;
		}

		std::cout << std::endl;
	}

	static ICommandPtr instance(BuzzerLightComposerPtr composer, hashTagsLoadedWithErrorFunction done) { 
		return std::make_shared<LoadHashTagsCommand>(composer, done);
	}

	static ICommandPtr instance(BuzzerLightComposerPtr composer, bool display, hashTagsLoadedWithErrorFunction done) { 
		return std::make_shared<LoadHashTagsCommand>(composer, display, done);
	}

private:
	BuzzerLightComposerPtr composer_;
	std::string tag_;
	BuzzfeedPtr localBuzzFeed_;
	hashTagsLoadedWithErrorFunction done_;
	std::vector<uint256> chains_;
	std::map<uint256, int> loaded_;
	std::map<uint256, std::map<uint160, Buzzer::HashTag>> feed_;
	bool display_ = true;
};

class LoadEventsfeedCommand;
typedef std::shared_ptr<LoadEventsfeedCommand> LoadEventsfeedCommandPtr;

class LoadEventsfeedCommand: public ICommand, public std::enable_shared_from_this<LoadEventsfeedCommand> {
public:
	LoadEventsfeedCommand(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneWithErrorFunction done): 
		composer_(composer), eventsFeed_(eventsFeed), done_(done) {
		localEventsFeed_ = Eventsfeed::instance(eventsFeed);
	}

	LoadEventsfeedCommand(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, eventsfeedReadyFunction eventsfeedReady, doneWithErrorFunction done): 
		composer_(composer), eventsFeed_(eventsFeed), eventsfeedReady_(eventsfeedReady), done_(done) {
		localEventsFeed_ = Eventsfeed::instance(eventsFeed);
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("eventsFeed"); 
		lSet.insert("efeed"); 
		return lSet;
	}

	void help() {
		std::cout << "eventsFeed | efeed [more]" << std::endl;
		std::cout << "\tLoad current eventsfeed. Limited to last 100 events/buzzes/likes/rebuzzes." << std::endl;
		std::cout << "\t[more] - optional, flag to feed more from last event" << std::endl;
		std::cout << "\texample:\n\t\t>efeed" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadEventsfeedCommand>(composer, eventsFeed, done); 
	}

	static ICommandPtr instance(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, eventsfeedReadyFunction eventsfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadEventsfeedCommand>(composer, eventsFeed, eventsfeedReady, done); 
	}

	// callbacks
	void eventsfeedLoaded(const std::vector<EventsfeedItem>& /*feed*/, const uint256& /*chain*/, int /*requests*/);
	void buzzesLoaded(const std::vector<BuzzfeedItem>& /*feed*/, const uint256& /*chain*/);

	void processPengingInfos();
	void buzzerInfoLoaded(const std::vector<TransactionPtr>&);
	void show();

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during events loading.");
	}

	virtual void display(EventsfeedItemPtr);

	virtual EventsfeedPtr eventsfeed() {
		if (!appendFeed()) return eventsFeed_;
		return localEventsFeed_;
	}

	virtual bool appendFeed() {
		if (!from_) return false;
		return true;
	}

protected:
	BuzzerLightComposerPtr composer_;
	EventsfeedPtr eventsFeed_;
	EventsfeedPtr localEventsFeed_;
	doneWithErrorFunction done_;
	eventsfeedReadyFunction eventsfeedReady_;
	std::vector<uint256> chains_;
	std::map<uint256, int> loaded_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pending_;
	std::set<uint256> pendingLoaded_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pengindChainInfos_;
	std::map<uint256 /*info tx*/, Buzzer::Info> pendingInfos_;
	int pendingChainInfosLoaded_ = 0;
	uint64_t from_ = 0;	
};

class LoadConversationsCommand;
typedef std::shared_ptr<LoadConversationsCommand> LoadConversationsCommandPtr;

class LoadConversationsCommand: public ICommand, public std::enable_shared_from_this<LoadConversationsCommand> {
public:

	LoadConversationsCommand(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversationsFeed, doneWithErrorFunction done): 
		composer_(composer), conversationsFeed_(conversationsFeed), done_(done) {
		localConversationsFeed_ = Conversationsfeed::instance(conversationsFeed);
	}

	LoadConversationsCommand(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversationsFeed, conversationsReadyFunction conversationsfeedReady, doneWithErrorFunction done): 
		composer_(composer), conversationsFeed_(conversationsFeed), conversationsfeedReady_(conversationsfeedReady), done_(done) {
		localConversationsFeed_ = Conversationsfeed::instance(conversationsFeed);
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("conversationsFeed"); 
		lSet.insert("conversations"); 
		lSet.insert("cfeed"); 
		return lSet;
	}

	void help() {
		std::cout << "conversations | cfeed [buzzer_id] [more]" << std::endl;
		std::cout << "\tLoad conversations by buzzer. Limited to last 50 conversations." << std::endl;
		std::cout << "\t[buzzer_id]	- required, buzz id to fetch thread" << std::endl;
		std::cout << "\t[more]		- optional, flag to feed more from last item" << std::endl;
		std::cout << "\texample:\n\t\t>cfeed" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversationsFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadConversationsCommand>(composer, conversationsFeed, done); 
	}

	static ICommandPtr instance(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversationsFeed, conversationsReadyFunction conversationsfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadConversationsCommand>(composer, conversationsFeed, conversationsfeedReady, done); 
	}

	// callbacks
	void eventsfeedLoaded(const std::vector<ConversationItem>& /*feed*/, const uint256& /*chain*/, int /*requests*/);
	void buzzesLoaded(const std::vector<BuzzfeedItem>& /*feed*/, const uint256& /*chain*/);

	void processPengingInfos();
	void buzzerInfoLoaded(const std::vector<TransactionPtr>&);
	void show();

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during conversation loading.");
	}

	virtual void display(ConversationItemPtr);

	virtual ConversationsfeedPtr conversationsfeed() {
		if (!appendFeed()) return conversationsFeed_;
		return localConversationsFeed_;
	}

	virtual bool appendFeed() {
		if (!from_) return false;
		return true;
	}

protected:
	BuzzerLightComposerPtr composer_;
	ConversationsfeedPtr conversationsFeed_;
	ConversationsfeedPtr localConversationsFeed_;
	doneWithErrorFunction done_;
	conversationsReadyFunction conversationsfeedReady_;
	std::vector<uint256> chains_;
	std::map<uint256, int> loaded_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pending_;
	std::set<uint256> pendingLoaded_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pengindChainInfos_;
	std::map<uint256 /*info tx*/, Buzzer::Info> pendingInfos_;
	int pendingChainInfosLoaded_ = 0;
	uint64_t from_ = 0;	
};

class LoadMistrustsByBuzzerCommand;
typedef std::shared_ptr<LoadMistrustsByBuzzerCommand> LoadMistrustsByBuzzerCommandPtr;

class LoadMistrustsByBuzzerCommand: public LoadEventsfeedCommand {
public:
	LoadMistrustsByBuzzerCommand(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneWithErrorFunction done): 
		LoadEventsfeedCommand(composer, eventsFeed, done) {
	}

	LoadMistrustsByBuzzerCommand(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, eventsfeedReadyFunction eventsfeedReady, doneWithErrorFunction done): 
		LoadEventsfeedCommand(composer, eventsFeed, eventsfeedReady, done) {
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("mistrustsByBuzzer"); 
		lSet.insert("bmfeed"); 
		return lSet;
	}

	void help() {
		std::cout << "mistrustsByBuzzer | bmfeed <buzzer_id> or <@buzzer> [more]" << std::endl;
		std::cout << "\tLoad mistrusts by buzzer. Limited to last 30 events." << std::endl;
		std::cout << "\t<buzzer_id>	- required, buzz id to fetch thread or @buzzer name" << std::endl;
		std::cout << "\t[more]		- optional, flag to feed more from last item" << std::endl;
		std::cout << "\texample:\n\t\t>bmfeed" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadMistrustsByBuzzerCommand>(composer, eventsFeed, done); 
	}

	static ICommandPtr instance(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, eventsfeedReadyFunction eventsfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadMistrustsByBuzzerCommand>(composer, eventsFeed, eventsfeedReady, done); 
	}

	void display(EventsfeedItemPtr item) {
		//
		std::vector<EventsfeedItemPtr> lFeed;
		item->feed(lFeed);

		for (std::vector<EventsfeedItemPtr>::iterator lEvent = lFeed.begin(); lEvent != lFeed.end(); lEvent++) {
			std::cout << (*lEvent)->toFeedingString() << std::endl << std::endl;
		}
	}

	bool appendFeed() {
		if (fromBuzzer_.isNull()) return false;
		return true;
	}

private:
	uint256 fromBuzzer_;
};

class LoadEndorsementsByBuzzerCommand;
typedef std::shared_ptr<LoadEndorsementsByBuzzerCommand> LoadEndorsementsByBuzzerCommandPtr;

class LoadEndorsementsByBuzzerCommand: public LoadEventsfeedCommand {
public:
	LoadEndorsementsByBuzzerCommand(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneWithErrorFunction done): 
		LoadEventsfeedCommand(composer, eventsFeed, done) {
	}

	LoadEndorsementsByBuzzerCommand(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, eventsfeedReadyFunction eventsfeedReady, doneWithErrorFunction done): 
		LoadEventsfeedCommand(composer, eventsFeed, eventsfeedReady, done) {
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("endorsementsByBuzzer"); 
		lSet.insert("befeed"); 
		return lSet;
	}

	void help() {
		std::cout << "endorsementsByBuzzer | befeed <buzzer_id> or <@buzzer> [more]" << std::endl;
		std::cout << "\tLoad endorsements by buzzer. Limited to last 30 events." << std::endl;
		std::cout << "\t<buzzer_id>	- required, buzz id to fetch thread or @buzzer name" << std::endl;
		std::cout << "\t[more]		- optional, flag to feed more from last item" << std::endl;
		std::cout << "\texample:\n\t\t>befeed" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadEndorsementsByBuzzerCommand>(composer, eventsFeed, done); 
	}

	static ICommandPtr instance(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, eventsfeedReadyFunction eventsfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadEndorsementsByBuzzerCommand>(composer, eventsFeed, eventsfeedReady, done); 
	}

	void display(EventsfeedItemPtr item) {
		//
		std::vector<EventsfeedItemPtr> lFeed;
		item->feed(lFeed);

		for (std::vector<EventsfeedItemPtr>::iterator lEvent = lFeed.begin(); lEvent != lFeed.end(); lEvent++) {
			std::cout << (*lEvent)->toFeedingString() << std::endl << std::endl;
		}
	}

	bool appendFeed() {
		if (fromBuzzer_.isNull()) return false;
		return true;
	}

private:
	uint256 fromBuzzer_;
};

class LoadSubscriptionsByBuzzerCommand;
typedef std::shared_ptr<LoadSubscriptionsByBuzzerCommand> LoadSubscriptionsByBuzzerCommandPtr;

class LoadSubscriptionsByBuzzerCommand: public LoadEventsfeedCommand {
public:
	LoadSubscriptionsByBuzzerCommand(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneWithErrorFunction done): 
		LoadEventsfeedCommand(composer, eventsFeed, done) {
	}

	LoadSubscriptionsByBuzzerCommand(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, eventsfeedReadyFunction eventsfeedReady, doneWithErrorFunction done): 
		LoadEventsfeedCommand(composer, eventsFeed, eventsfeedReady, done) {
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("subscriptionsByBuzzer"); 
		lSet.insert("bsubs"); 
		return lSet;
	}

	void help() {
		std::cout << "subscriptionsByBuzzer | bsubs <buzzer_id> or <@buzzer> [more]" << std::endl;
		std::cout << "\tLoad subscriptions by buzzer. Limited to last 30 events." << std::endl;
		std::cout << "\t<buzzer_id>	- required, buzz id to fetch thread or <@buzzer> name" << std::endl;
		std::cout << "\t[more]		- optional, flag to feed more from last item" << std::endl;
		std::cout << "\texample:\n\t\t>bsubs @me" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadSubscriptionsByBuzzerCommand>(composer, eventsFeed, done); 
	}

	static ICommandPtr instance(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, eventsfeedReadyFunction eventsfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadSubscriptionsByBuzzerCommand>(composer, eventsFeed, eventsfeedReady, done); 
	}

	void display(EventsfeedItemPtr item) {
		//
		std::vector<EventsfeedItemPtr> lFeed;
		item->feed(lFeed);

		for (std::vector<EventsfeedItemPtr>::iterator lEvent = lFeed.begin(); lEvent != lFeed.end(); lEvent++) {
			std::cout << (*lEvent)->toFeedingString() << std::endl << std::endl;
		}
	}

	bool appendFeed() {
		if (fromBuzzer_.isNull()) return false;
		return true;
	}

private:
	uint256 fromBuzzer_;
};

class LoadFollowersByBuzzerCommand;
typedef std::shared_ptr<LoadFollowersByBuzzerCommand> LoadFollowersByBuzzerCommandPtr;

class LoadFollowersByBuzzerCommand: public LoadEventsfeedCommand {
public:
	LoadFollowersByBuzzerCommand(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneWithErrorFunction done): 
		LoadEventsfeedCommand(composer, eventsFeed, done) {
	}

	LoadFollowersByBuzzerCommand(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, eventsfeedReadyFunction eventsfeedReady, doneWithErrorFunction done): 
		LoadEventsfeedCommand(composer, eventsFeed, eventsfeedReady, done) {
	}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("followersByBuzzer"); 
		lSet.insert("bflws"); 
		return lSet;
	}

	void help() {
		std::cout << "followersByBuzzer | bflws <buzzer_id> or <@buzzer> [more]" << std::endl;
		std::cout << "\tLoad followers by buzzer. Limited to last 30 events." << std::endl;
		std::cout << "\t<buzzer_id>	- required, buzz id to fetch thread or <@buzzer> name" << std::endl;
		std::cout << "\t[more]		- optional, flag to feed more from last item" << std::endl;
		std::cout << "\texample:\n\t\t>bflws @me" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneWithErrorFunction done) { 
		return std::make_shared<LoadFollowersByBuzzerCommand>(composer, eventsFeed, done); 
	}

	static ICommandPtr instance(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, eventsfeedReadyFunction eventsfeedReady, doneWithErrorFunction done) { 
		return std::make_shared<LoadFollowersByBuzzerCommand>(composer, eventsFeed, eventsfeedReady, done); 
	}

	void display(EventsfeedItemPtr item) {
		//
		std::vector<EventsfeedItemPtr> lFeed;
		item->feed(lFeed);

		for (std::vector<EventsfeedItemPtr>::iterator lEvent = lFeed.begin(); lEvent != lFeed.end(); lEvent++) {
			std::cout << (*lEvent)->toFeedingString() << std::endl << std::endl;
		}
	}

	bool appendFeed() {
		if (fromBuzzer_.isNull()) return false;
		return true;
	}

private:
	uint256 fromBuzzer_;
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

	void buzzesLoaded(const std::vector<BuzzfeedItem>& /*feed*/, const uint256& /*chain*/);

	void processPengingInfos();
	void buzzerInfoLoaded(const std::vector<TransactionPtr>&);
	void show();

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzes loading.");
	}

	void display(const std::vector<BuzzfeedItemPtr>&);

private:
	BuzzerLightComposerPtr composer_;
	BuzzfeedPtr buzzFeed_;
	doneFunction done_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pending_;
	std::set<uint256> pendingLoaded_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pengindChainInfos_;
	std::map<uint256 /*info tx*/, Buzzer::Info> pendingInfos_;
	int pendingChainInfosLoaded_ = 0;
};

class EventsfeedListCommand;
typedef std::shared_ptr<EventsfeedListCommand> EventsfeedListCommandPtr;

class EventsfeedListCommand: public ICommand, public std::enable_shared_from_this<EventsfeedListCommand> {
public:
	EventsfeedListCommand(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneFunction done): composer_(composer), eventsFeed_(eventsFeed), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("eventsList"); 
		lSet.insert("elist"); 
		return lSet;
	}

	void help() {
		std::cout << "eventsList | elist" << std::endl;
		std::cout << "\tShow current eventsfeed." << std::endl;
		std::cout << "\texample:\n\t\t>elist" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, EventsfeedPtr eventsFeed, doneFunction done) { 
		return std::make_shared<EventsfeedListCommand>(composer, eventsFeed, done); 
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_();
	}

	void buzzesLoaded(const std::vector<BuzzfeedItem>& /*feed*/, const uint256& /*chain*/);

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzes loading.");
	}

	void display(EventsfeedItemPtr);

private:
	BuzzerLightComposerPtr composer_;
	EventsfeedPtr eventsFeed_;
	doneFunction done_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pending_;
	std::set<uint256> pendingLoaded_;	
};

class ConversationsListCommand;
typedef std::shared_ptr<ConversationsListCommand> ConversationsListCommandPtr;

class ConversationsListCommand: public ICommand, public std::enable_shared_from_this<ConversationsListCommand> {
public:
	ConversationsListCommand(BuzzerLightComposerPtr composer, ConversationsfeedPtr eventsFeed, doneFunction done): composer_(composer), eventsFeed_(eventsFeed), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("conversationsList"); 
		lSet.insert("clist"); 
		return lSet;
	}

	void help() {
		std::cout << "conversationsList | clist" << std::endl;
		std::cout << "\tShow current conversations." << std::endl;
		std::cout << "\texample:\n\t\t>clist" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, ConversationsfeedPtr eventsFeed, doneFunction done) { 
		return std::make_shared<ConversationsListCommand>(composer, eventsFeed, done); 
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_();
	}

	void buzzesLoaded(const std::vector<BuzzfeedItem>& /*feed*/, const uint256& /*chain*/);

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzes loading.");
	}

	void display(ConversationItemPtr);

private:
	BuzzerLightComposerPtr composer_;
	ConversationsfeedPtr eventsFeed_;
	doneFunction done_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pending_;
	std::set<uint256> pendingLoaded_;	
};

class BuzzLikeCommand;
typedef std::shared_ptr<BuzzLikeCommand> BuzzLikeCommandPtr;

class BuzzLikeCommand: public ICommand, public std::enable_shared_from_this<BuzzLikeCommand> {
public:
	BuzzLikeCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done): composer_(composer), buzzFeed_(buzzFeed), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzLike"); 
		lSet.insert("like"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzLike | like <buzz_id>" << std::endl;
		std::cout << "\tLike published buzz." << std::endl;
		std::cout << "\t<buzz_id> - required, publishers buzz - buzz tx." << std::endl;
		std::cout << "\texample:\n\t\t>like a9756f1d84c0e803bdd6993bfdfaaf6ef19ef24accc6d4006e5a874cda6c7bd1" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done) { 
		return std::make_shared<BuzzLikeCommand>(composer, buzzFeed, done); 
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		ctx_ = ctx;
		//
		if (!composer_->requestProcessor()->sendTransaction(ctx,
				SentTransaction::instance(
					boost::bind(&BuzzLikeCommand::sent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&BuzzLikeCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void sent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));

				composer_->wallet()->rollback(ctx_); // rollback tx
				composer_->wallet()->resetCache();
				composer_->wallet()->prepareCache();

				done_(ProcessingError("E_SENT_TX", lError->data()));
				break;
			}

			return;
		} else {
			std::cout << tx.toHex() << std::endl;
		}

		done_(ProcessingError());
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzz like action.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

private:
	BuzzerLightComposerPtr composer_;
	BuzzfeedPtr buzzFeed_;
	doneWithErrorFunction done_;
	TransactionContextPtr ctx_;
};

class BuzzRewardCommand;
typedef std::shared_ptr<BuzzRewardCommand> BuzzRewardCommandPtr;

class BuzzRewardCommand: public ICommand, public std::enable_shared_from_this<BuzzRewardCommand> {
public:
	BuzzRewardCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done): composer_(composer), buzzFeed_(buzzFeed), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzReward"); 
		lSet.insert("reward"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzReward | reward <buzz_id> <amount>" << std::endl;
		std::cout << "\tSent reward to the buzz publisher." << std::endl;
		std::cout << "\t<buzz_id> - required, publishers buzz - buzz tx." << std::endl;
		std::cout << "\t<amount>  - required, reward in qBIT." << std::endl;
		std::cout << "\texample:\n\t\t>reward a9756f1d84c0e803bdd6993bfdfaaf6ef19ef24accc6d4006e5a874cda6c7bd1 3000" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done) { 
		return std::make_shared<BuzzRewardCommand>(composer, buzzFeed, done); 
	}

	// retry
	bool retry() {
		//
		if (ctx_ == nullptr) return false;
		created(ctx_);
		return true;
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		ctx_ = ctx;
		//
		// push linked and newly created txs; order matters
		IPeerPtr lPeer = nullptr;
		for (std::list<TransactionContextPtr>::iterator lLinkedCtx = ctx->linkedTxs().begin(); lLinkedCtx != ctx->linkedTxs().end(); lLinkedCtx++) {
			if (!(lPeer = composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), *lLinkedCtx,
					SentTransaction::instance(
						boost::bind(&BuzzRewardCommand::spendSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
						boost::bind(&BuzzRewardCommand::timeout, shared_from_this()))))) {
				gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
				composer_->wallet()->resetCache();
				composer_->wallet()->prepareCache();
				done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
			}
		}

		if (!composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), ctx,
				SentTransaction::instance(
					boost::bind(&BuzzRewardCommand::rewardSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&BuzzRewardCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}		
	}

	void spendSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return;
		} else {
			std::cout << "spend: " << tx.toHex() << std::endl;
			//
			TransactionContextPtr lSpend = ctx_->locateByType(Transaction::SPEND);
			for (std::list<Transaction::NetworkUnlinkedOutPtr>::iterator lOut = lSpend->externalOuts().begin(); 
																		lOut != lSpend->externalOuts().end(); lOut++) {
				composer_->wallet()->updateOut(*lOut, ctx_->tx()->id(), ctx_->tx()->type());
			}
		}

		spendSent_ = true;
		if (spendSent_ && rewardSent_) done_(ProcessingError());
	}

	void rewardSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return;
		} else {
			std::cout << "reward: " << tx.toHex() << std::endl;
		}

		rewardSent_ = true;
		if (spendSent_ && rewardSent_) done_(ProcessingError());
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzz like action.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

private:
	BuzzerLightComposerPtr composer_;
	BuzzfeedPtr buzzFeed_;
	doneWithErrorFunction done_;

	TransactionContextPtr ctx_;

	bool spendSent_ = false;
	bool rewardSent_ = false;	
};

class CreateBuzzReplyCommand;
typedef std::shared_ptr<CreateBuzzReplyCommand> CreateBuzzReplyCommandPtr;

class CreateBuzzReplyCommand: public ICommand, public std::enable_shared_from_this<CreateBuzzReplyCommand> {
public:
	CreateBuzzReplyCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done): composer_(composer), buzzFeed_(buzzFeed), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("createBuzzReply"); 
		lSet.insert("reply");
		lSet.insert("rep"); 
		return lSet;
	}

	void help() {
		std::cout << "createBuzzReply | reply | rep <buzz_id> \"<message>\" [@buzzer1 @buzzer2 ...] [mediafile1 mediafile2 ...]" << std::endl;
		std::cout << "\tReply buzz 	- a reply message from your default buzzer to the given buzz. Message will hit all your buzzer subscribers." << std::endl;
		std::cout << "\t<buzz_id> 	- required, buzzer id" << std::endl;
		std::cout << "\t<message> 	- required, buzz message body; can contains multibyte sequences, i.e. 😀 (up to 512 bytes)" << std::endl;
		std::cout << "\t[@buzzer] 	- optional, notify/tag buzzers" << std::endl;
		std::cout << "\t[mediafile] - optional, media file to upload" << std::endl;
		std::cout << "\texample:\n\t\t>reply d1bb559d3a912a0838163c8ea76eb753e5b99e3b24162bb56d2a6426fb3b7f83 \"No, it's a funny day! 🎈\" @buddy" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done) {
		return std::make_shared<CreateBuzzReplyCommand>(composer, buzzFeed, done); 
	}

	// retry
	bool retry() {
		//
		if (ctx_ == nullptr) return false;
		created(ctx_);
		return true;
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		ctx_ = ctx;
		//
		// push linked and newly created txs; order matters
		IPeerPtr lPeer = nullptr;
		for (std::list<TransactionContextPtr>::iterator lLinkedCtx = ctx->linkedTxs().begin(); lLinkedCtx != ctx->linkedTxs().end(); lLinkedCtx++) {
			if (!(lPeer = composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), *lLinkedCtx,
					SentTransaction::instance(
						boost::bind(&CreateBuzzReplyCommand::feeSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
						boost::bind(&CreateBuzzReplyCommand::timeout, shared_from_this()))))) {
				gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
				composer_->wallet()->resetCache();
				composer_->wallet()->prepareCache();
				done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
			}
		}

		if (!composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), ctx,
				SentTransaction::instance(
					boost::bind(&CreateBuzzReplyCommand::buzzSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&CreateBuzzReplyCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzz creation.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

	void feeSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return;
		} else {
			std::cout << " fee: " << tx.toHex() << std::endl;
			//
			TransactionContextPtr lFee = ctx_->locateByType(Transaction::FEE);
			for (std::list<Transaction::NetworkUnlinkedOutPtr>::iterator lOut = lFee->externalOuts().begin(); 
																		lOut != lFee->externalOuts().end(); lOut++) {
				composer_->wallet()->updateOut(*lOut, ctx_->tx()->id(), ctx_->tx()->type());
			}
		}

		feeSent_ = true;
		if (feeSent_ && buzzSent_) done_(ProcessingError());
	}

	void buzzSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return;
		} else {
			std::cout << "buzz: " << tx.toHex() << std::endl;
		}

		buzzSent_ = true;
		if (feeSent_ && buzzSent_) done_(ProcessingError());
	}

	void setUploadMedia(ICommandPtr uploadMedia) {
		uploadMedia_ = uploadMedia;
	}

	void uploadNextMedia();
	void mediaUploaded(TransactionPtr, const ProcessingError&);

	void setMediaUploaded(doneTransactionWithErrorFunction mediaUploaded) {
		mediaUploaded_ = mediaUploaded;
	}

	void createBuzz();

private:
	BuzzerLightComposerPtr composer_;
	BuzzfeedPtr buzzFeed_;
	doneWithErrorFunction done_;
	doneTransactionWithErrorFunction mediaUploaded_;

	uint256 buzzId_;
	std::string body_;
	std::vector<std::string> buzzers_;

	ICommandPtr uploadMedia_;
	std::vector<BuzzerMediaPointer> mediaPointers_;
	std::vector<std::string> mediaFiles_;

	TransactionContextPtr ctx_;

	bool feeSent_ = false;
	bool buzzSent_ = false;
};	

class CreateReBuzzCommand;
typedef std::shared_ptr<CreateReBuzzCommand> CreateReBuzzCommandPtr;

class CreateReBuzzCommand: public ICommand, public std::enable_shared_from_this<CreateReBuzzCommand> {
public:
	CreateReBuzzCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done): composer_(composer), buzzFeed_(buzzFeed), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("createReBuzz"); 
		lSet.insert("rebuzz");
		lSet.insert("reb"); 
		return lSet;
	}

	void help() {
		std::cout << "createReBuzz | rebuzz | reb <buzz_id> \"[message]\" [@buzzer1 @buzzer2 ...] [mediafile1 mediafile2 ...]" << std::endl;
		std::cout << "\tRebuzz 		- rebuzz original buzz to your own timeline. Message will hit all your buzzer subscribers." << std::endl;
		std::cout << "\t<buzz_id> 	- required, buzzer id" << std::endl;
		std::cout << "\t[message] 	- optional, your rebuzz message body (up to 512 bytes)" << std::endl;
		std::cout << "\t[@buzzer] 	- optional, notify/tag buzzers" << std::endl;
		std::cout << "\t[mediafile] - optional, media file to upload" << std::endl;
		std::cout << "\texample:\n\t\t>rebuzz d1bb559d3a912a0838163c8ea76eb753e5b99e3b24162bb56d2a6426fb3b7f83 \"Check this out! 👇\" @buddy" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done) {
		return std::make_shared<CreateReBuzzCommand>(composer, buzzFeed, done); 
	}

	// retry
	bool retry() {
		//
		if (ctx_ == nullptr) return false;
		created(ctx_);
		return true;
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		ctx_ = ctx;
		//
		// push linked and newly created txs; order matters

		TransactionContextPtr lFee = ctx->locateByType(Transaction::FEE);
		TransactionContextPtr lNotify = ctx->locateByType(TX_BUZZ_REBUZZ_NOTIFY);

		IPeerPtr lPeer = nullptr;
		if (!(lPeer = composer_->requestProcessor()->sendTransaction(ctx->tx()->chain(), lFee,
				SentTransaction::instance(
					boost::bind(&CreateReBuzzCommand::feeSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&CreateReBuzzCommand::timeout, shared_from_this()))))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}

		// only in case if chains is different
		if (lNotify->tx()->chain() != ctx->tx()->chain()) {
			if (!composer_->requestProcessor()->sendTransaction(lNotify,
					SentTransaction::instance(
						boost::bind(&CreateReBuzzCommand::notifySent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
						boost::bind(&CreateReBuzzCommand::timeout, shared_from_this())))) {
				gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
				composer_->wallet()->resetCache();
				composer_->wallet()->prepareCache();
				done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
			}
		} else {
			notifySent_ = true;
		}

		if (!composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), ctx,
				SentTransaction::instance(
					boost::bind(&CreateReBuzzCommand::buzzSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&CreateReBuzzCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzz creation.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

	void feeSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		checkSent("   fee: ", tx, errors);
		
		//
		TransactionContextPtr lFee = ctx_->locateByType(Transaction::FEE);
		for (std::list<Transaction::NetworkUnlinkedOutPtr>::iterator lOut = lFee->externalOuts().begin(); 
																	lOut != lFee->externalOuts().end(); lOut++) {
			composer_->wallet()->updateOut(*lOut, ctx_->tx()->id(), ctx_->tx()->type());
		}

		//
		feeSent_ = true;
		if (feeSent_ && buzzSent_ && notifySent_) done_(ProcessingError());
	}

	void notifySent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		checkSent("notify: ", tx, errors);
		notifySent_ = true;
		if (feeSent_ && buzzSent_ && notifySent_) done_(ProcessingError());
	}

	void buzzSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		checkSent("  buzz: ", tx, errors);
		buzzSent_ = true;
		if (feeSent_ && buzzSent_ && notifySent_) done_(ProcessingError());
	}

	bool checkSent(const std::string& msg, const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return false;
		} else {
			std::cout << msg << tx.toHex() << std::endl;
		}

		return true;
	}

	void setUploadMedia(ICommandPtr uploadMedia) {
		uploadMedia_ = uploadMedia;
	}

	void setMediaUploaded(doneTransactionWithErrorFunction mediaUploaded) {
		mediaUploaded_ = mediaUploaded;
	}

	void uploadNextMedia();
	void mediaUploaded(TransactionPtr, const ProcessingError&);

	void createBuzz();	

private:
	BuzzerLightComposerPtr composer_;
	BuzzfeedPtr buzzFeed_;
	doneWithErrorFunction done_;
	doneTransactionWithErrorFunction mediaUploaded_;

	uint256 buzzId_;
	std::string body_;
	std::vector<std::string> buzzers_;

	ICommandPtr uploadMedia_;
	std::vector<BuzzerMediaPointer> mediaPointers_;
	std::vector<std::string> mediaFiles_;

	TransactionContextPtr ctx_;

	bool feeSent_ = false;
	bool notifySent_ = false;
	bool buzzSent_ = false;
};

class LoadBuzzerTrustScoreCommand;
typedef std::shared_ptr<LoadBuzzerTrustScoreCommand> LoadBuzzerTrustScoreCommandPtr;

class LoadBuzzerTrustScoreCommand: public ICommand, public std::enable_shared_from_this<LoadBuzzerTrustScoreCommand> {
public:
	LoadBuzzerTrustScoreCommand(BuzzerLightComposerPtr composer, doneTrustScoreWithErrorFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("loadTrustScore"); 
		lSet.insert("loadScore"); 
		lSet.insert("score"); 
		return lSet;
	}

	void help() {
		std::cout << "loadTrustScore | loadScore | score [buzzer_id/chain_id]" << std::endl;
		std::cout << "\tLoad buzzer trust score." << std::endl;
		std::cout << "\t[buzzer_id/chain_id] - optional, buzzer id to get score for" << std::endl;
		std::cout << "\texample:\n\t\t>score" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneTrustScoreWithErrorFunction done) { 
		return std::make_shared<LoadBuzzerTrustScoreCommand>(composer, done); 
	}

	// callbacks
	void trustScoreLoaded(amount_t endorsements, amount_t mistrusts, uint32_t subscriptions, uint32_t followers) {
		//
		double lScore = (double)endorsements / (double)mistrusts;
		std::cout << 
			strprintf(TxAssetType::scaleFormat(QBIT), lScore) << std::endl;
		//
		if (done_) done_(endorsements, mistrusts, subscriptions, followers, ProcessingError());
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzer creation.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		if (done_) done_(0, 0, 0, 0, ProcessingError(code, message));
	}

	void terminate() {
		// reset callbacks
		done_ = 0;
	}

private:
	BuzzerLightComposerPtr composer_;
	doneTrustScoreWithErrorFunction done_;
};

class BuzzerEndorseCommand;
typedef std::shared_ptr<BuzzerEndorseCommand> BuzzerEndorseCommandPtr;

class BuzzerEndorseCommand: public ICommand, public std::enable_shared_from_this<BuzzerEndorseCommand> {
public:
	BuzzerEndorseCommand(BuzzerLightComposerPtr composer, doneWithErrorFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzerEndorse"); 
		lSet.insert("endorse"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzerEndorse | endorse <publisher> [points]" << std::endl;
		std::cout << "\tEndorse publisher up for the given points." << std::endl;
		std::cout << "\t<publisher> - required, publishers buzzer - name prefixed with @" << std::endl;
		std::cout << "\t[points] 	- optional, points. Default " << BUZZER_MIN_EM_STEP << std::endl;
		std::cout << "\texample:\n\t\t>endorse @other" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneWithErrorFunction done) { 
		return std::make_shared<BuzzerEndorseCommand>(composer, done); 
	}

	// retry
	bool retry() {
		//
		if (ctx_ == nullptr) return false;
		created(ctx_);
		return true;
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		ctx_ = ctx;
		//
		// push linked and newly created txs; order matters
		IPeerPtr lPeer = nullptr;
		TransactionContextPtr lFee = ctx->locateByType(Transaction::FEE);
		if (!(lPeer = composer_->requestProcessor()->sendTransaction(ctx->tx()->chain(), lFee,
				SentTransaction::instance(
					boost::bind(&BuzzerEndorseCommand::feeSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&BuzzerEndorseCommand::timeout, shared_from_this()))))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}

		if (!composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), ctx,
				SentTransaction::instance(
					boost::bind(&BuzzerEndorseCommand::endorseSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&BuzzerEndorseCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void feeSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		checkSent("    fee: ", tx, errors);

		//
		TransactionContextPtr lFee = ctx_->locateByType(Transaction::FEE);
		for (std::list<Transaction::NetworkUnlinkedOutPtr>::iterator lOut = lFee->externalOuts().begin(); 
																	lOut != lFee->externalOuts().end(); lOut++) {
			composer_->wallet()->updateOut(*lOut, ctx_->tx()->id(), ctx_->tx()->type());
		}

		//
		feeSent_ = true;
		if (feeSent_ && endorseSent_) done_(ProcessingError());
	}

	void endorseSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		checkSent("endorse: ", tx, errors);
		endorseSent_ = true;
		if (feeSent_ && endorseSent_) done_(ProcessingError());
	}

	bool checkSent(const std::string& msg, const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return false;
		} else {
			std::cout << msg << tx.toHex() << std::endl;
		}

		return true;
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzer endorsement.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}	

private:
	BuzzerLightComposerPtr composer_;
	doneWithErrorFunction done_;

	TransactionContextPtr ctx_;

	bool feeSent_;
	bool endorseSent_;	
};

class BuzzerMistrustCommand;
typedef std::shared_ptr<BuzzerMistrustCommand> BuzzerMistrustCommandPtr;

class BuzzerMistrustCommand: public ICommand, public std::enable_shared_from_this<BuzzerMistrustCommand> {
public:
	BuzzerMistrustCommand(BuzzerLightComposerPtr composer, doneWithErrorFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzerMistrust"); 
		lSet.insert("mistrust"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzerMistrust | mistrust <publisher> [points]" << std::endl;
		std::cout << "\tMistrust publisher up for the given points." << std::endl;
		std::cout << "\t<publisher> - required, publishers buzzer - name prefixed with @" << std::endl;
		std::cout << "\t[points] 	- optional, points. Default " << BUZZER_MIN_EM_STEP << std::endl;
		std::cout << "\texample:\n\t\t>mistrust @other" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneWithErrorFunction done) { 
		return std::make_shared<BuzzerMistrustCommand>(composer, done); 
	}

	// retry
	bool retry() {
		//
		if (ctx_ == nullptr) return false;
		created(ctx_);
		return true;
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		ctx_ = ctx;
		//
		// push linked and newly created txs; order matters
		IPeerPtr lPeer = nullptr;
		TransactionContextPtr lFee = ctx->locateByType(Transaction::FEE);
		if (!(lPeer = composer_->requestProcessor()->sendTransaction(ctx->tx()->chain(), lFee,
				SentTransaction::instance(
					boost::bind(&BuzzerMistrustCommand::feeSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&BuzzerMistrustCommand::timeout, shared_from_this()))))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}

		if (!composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), ctx,
				SentTransaction::instance(
					boost::bind(&BuzzerMistrustCommand::mistrustSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&BuzzerMistrustCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void feeSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		checkSent("     fee: ", tx, errors);

		//
		TransactionContextPtr lFee = ctx_->locateByType(Transaction::FEE);
		for (std::list<Transaction::NetworkUnlinkedOutPtr>::iterator lOut = lFee->externalOuts().begin(); 
																	lOut != lFee->externalOuts().end(); lOut++) {
			composer_->wallet()->updateOut(*lOut, ctx_->tx()->id(), ctx_->tx()->type());
		}

		//
		feeSent_ = true;
		if (feeSent_ && mistrustSent_) done_(ProcessingError());
	}

	void mistrustSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		checkSent("mistrust: ", tx, errors);
		mistrustSent_ = true;
		if (feeSent_ && mistrustSent_) done_(ProcessingError());
	}

	bool checkSent(const std::string& msg, const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return false;
		} else {
			std::cout << msg << tx.toHex() << std::endl;
		}

		return true;
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzer mistrust.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}	

private:
	BuzzerLightComposerPtr composer_;
	doneWithErrorFunction done_;

	TransactionContextPtr ctx_;

	bool feeSent_;
	bool mistrustSent_;
};

class BuzzSubscribeCommand;
typedef std::shared_ptr<BuzzSubscribeCommand> BuzzSubscribeCommandPtr;

class BuzzSubscribeCommand: public ICommand, public std::enable_shared_from_this<BuzzSubscribeCommand> {
public:
	BuzzSubscribeCommand(BuzzerLightComposerPtr composer, doneWithErrorFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzSubscribe"); 
		lSet.insert("buzzsub"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzSubscribe | buzzsub <chain> <buzz_id> <nonce>" << std::endl;
		std::cout << "\tSubscribe on buzz thread updates. You will get all replies and updatef from all participants for this thread." << std::endl;
		std::cout << "\t<chain>   - required, chain/shard id" << std::endl;
		std::cout << "\t<buzz_id> - required, buzz id" << std::endl;
		std::cout << "\t<nonce>   - required, random seed number" << std::endl;
		std::cout << "\texample:\n\t\t>buzzsub 0a05ac78b737adafdf9c77f64ab8e8223b8dcfa6b1dcae239fe1a09960eb69cc fe42a6aff2717010d8d6122cd3c7c461743f54473c7f4e1e5ceaa6c51472997f 198642712432" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneWithErrorFunction done) { 
		return std::make_shared<BuzzSubscribeCommand>(composer, done); 
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzz subscription.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

	const std::list<uint160>& peers() const { return peers_; }
	const uint512& signature() const { return signature_; }

private:
	BuzzerLightComposerPtr composer_;
	doneWithErrorFunction done_;
	uint256 chain_;
	uint256 buzzId_;
	uint64_t nonce_;
	uint512 signature_;
	std::list<uint160> peers_;
};

class BuzzUnsubscribeCommand;
typedef std::shared_ptr<BuzzUnsubscribeCommand> BuzzUnsubscribeCommandPtr;

class BuzzUnsubscribeCommand: public ICommand, public std::enable_shared_from_this<BuzzUnsubscribeCommand> {
public:
	BuzzUnsubscribeCommand(BuzzerLightComposerPtr composer, doneWithErrorFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzUnsubscribe"); 
		lSet.insert("buzzusub"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzUnsubscribe | buzzusub <chain> <buzz_id> <peer1> <peer2> ..." << std::endl;
		std::cout << "\tUnsubscribe from buzz thread updates." << std::endl;
		std::cout << "\t<chain>   - required, chain/shard id" << std::endl;
		std::cout << "\t<buzz_id> - required, buzz id" << std::endl;
		std::cout << "\t<peer1>   - required, peer id to drop subscription from" << std::endl;
		std::cout << "\texample:\n\t\t>buzzusub 0a05ac78b737adafdf9c77f64ab8e8223b8dcfa6b1dcae239fe1a09960eb69cc fe42a6aff2717010d8d6122cd3c7c461743f54473c7f4e1e5ceaa6c51472997f ff2717010d8d6122cd3c7c461743f5447" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneWithErrorFunction done) { 
		return std::make_shared<BuzzUnsubscribeCommand>(composer, done); 
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzz unsubscription.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

private:
	BuzzerLightComposerPtr composer_;
	doneWithErrorFunction done_;
	uint256 chain_;
	uint256 buzzId_;
};

class LoadBuzzerInfoCommand;
typedef std::shared_ptr<LoadBuzzerInfoCommand> LoadBuzzerInfoCommandPtr;

class LoadBuzzerInfoCommand: public ICommand, public std::enable_shared_from_this<LoadBuzzerInfoCommand> {
public:
	LoadBuzzerInfoCommand(BuzzerLightComposerPtr composer, buzzerInfoAndUtxoDoneWithErrorFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("buzzerInfo"); 
		lSet.insert("info"); 
		return lSet;
	}

	void help() {
		std::cout << "buzzerInfo | info <@buzzer>" << std::endl;
		std::cout << "\tLoad buzzer and buzzer info." << std::endl;
		std::cout << "\t<@buzzer>	- required, buzzer name, prefixed with @" << std::endl;
		std::cout << "\t[-utxo]		- optional, load buzzer utxo" << std::endl;
		std::cout << "\texample:\n\t\t>info @buzzer" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, buzzerInfoAndUtxoDoneWithErrorFunction done) { 
		return std::make_shared<LoadBuzzerInfoCommand>(composer, done); 
	}

	// callbacks
	void buzzerAndInfoLoaded(EntityPtr, TransactionPtr, const std::string&);
	void utxoByBuzzerLoaded(const std::vector<Transaction::NetworkUnlinkedOut>&, const uint256&);

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzer endorsement.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(nullptr, nullptr, std::vector<Transaction::NetworkUnlinkedOut>(), std::string(), ProcessingError(code, message));
	}	

private:
	BuzzerLightComposerPtr composer_;
	buzzerInfoAndUtxoDoneWithErrorFunction done_;
	EntityPtr buzzer_;
	TransactionPtr info_;
	bool loadUtxo_ = false;
};

class CreateBuzzerConversationCommand;
typedef std::shared_ptr<CreateBuzzerConversationCommand> CreateBuzzerConversationCommandPtr;

class CreateBuzzerConversationCommand: public ICommand, public std::enable_shared_from_this<CreateBuzzerConversationCommand> {
public:
	CreateBuzzerConversationCommand(BuzzerLightComposerPtr composer, doneWithErrorFunction done): composer_(composer), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("createConversation"); 
		lSet.insert("convc"); 
		return lSet;
	}

	void help() {
		std::cout << "createConversation | convc <buzzer>" << std::endl;
		std::cout << "\tCreate conversation with buzzer." << std::endl;
		std::cout << "\t<buzzer> - required, buzzer - name prefixed with @" << std::endl;
		std::cout << "\texample:\n\t\t>convc @other" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, doneWithErrorFunction done) { 
		return std::make_shared<CreateBuzzerConversationCommand>(composer, done); 
	}

	// retry
	bool retry() {
		//
		if (ctx_ == nullptr) return false;
		created(ctx_);
		return true;
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		ctx_ = ctx;
		//
		// push linked and newly created txs; order matters
		IPeerPtr lPeer = nullptr;
		TransactionContextPtr lFee = ctx->locateByType(Transaction::FEE);
		if (!(lPeer = composer_->requestProcessor()->sendTransaction(ctx->tx()->chain(), lFee,
				SentTransaction::instance(
					boost::bind(&CreateBuzzerConversationCommand::feeSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&CreateBuzzerConversationCommand::timeout, shared_from_this()))))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}

		if (!composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), ctx,
				SentTransaction::instance(
					boost::bind(&CreateBuzzerConversationCommand::conversationSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&CreateBuzzerConversationCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void feeSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		checkSent("         fee: ", tx, errors);

		//
		TransactionContextPtr lFee = ctx_->locateByType(Transaction::FEE);
		for (std::list<Transaction::NetworkUnlinkedOutPtr>::iterator lOut = lFee->externalOuts().begin(); 
																	lOut != lFee->externalOuts().end(); lOut++) {
			composer_->wallet()->updateOut(*lOut, ctx_->tx()->id(), ctx_->tx()->type());
		}

		//
		feeSent_ = true;
		if (feeSent_ && conversationSent_) done_(ProcessingError());
	}

	void conversationSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		checkSent("conversation: ", tx, errors);
		conversationSent_ = true;
		if (feeSent_ && conversationSent_) done_(ProcessingError());
	}

	bool checkSent(const std::string& msg, const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return false;
		} else {
			std::cout << msg << tx.toHex() << std::endl;
		}

		return true;
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during conversation creation.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}	

private:
	BuzzerLightComposerPtr composer_;
	doneWithErrorFunction done_;

	TransactionContextPtr ctx_;

	bool feeSent_;
	bool conversationSent_;
};

class AcceptConversationCommand;
typedef std::shared_ptr<AcceptConversationCommand> AcceptConversationCommandPtr;

class AcceptConversationCommand: public ICommand, public std::enable_shared_from_this<AcceptConversationCommand> {
public:
	AcceptConversationCommand(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, doneWithErrorFunction done): composer_(composer), conversations_(conversations), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("acceptConversation"); 
		lSet.insert("accept");
		lSet.insert("acc"); 
		return lSet;
	}

	void help() {
		std::cout << "acceptConversation | accept | acc <conversation_id>" << std::endl;
		std::cout << "\tAccept pending conversation." << std::endl;
		std::cout << "\t<conversation_id> - required, conversation id" << std::endl;
		std::cout << "\texample:\n\t\t>accept d1bb559d3a912a0838163c8ea76eb753e5b99e3b24162bb56d2a6426fb3b7f83" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, doneWithErrorFunction done) {
		return std::make_shared<AcceptConversationCommand>(composer, conversations, done); 
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		ctx_ = ctx;
		if (!composer_->requestProcessor()->sendTransaction(ctx->tx()->chain(), ctx,
				SentTransaction::instance(
					boost::bind(&AcceptConversationCommand::acceptSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&AcceptConversationCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during accept creation.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

	void acceptSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return;
		} else {
			std::cout << "accept: " << tx.toHex() << std::endl;
		}

		done_(ProcessingError());
	}

	void createBuzz();

private:
	BuzzerLightComposerPtr composer_;
	ConversationsfeedPtr conversations_;
	doneWithErrorFunction done_;

	TransactionContextPtr ctx_;
};	

class DeclineConversationCommand;
typedef std::shared_ptr<DeclineConversationCommand> DeclineConversationCommandPtr;

class DeclineConversationCommand: public ICommand, public std::enable_shared_from_this<DeclineConversationCommand> {
public:
	DeclineConversationCommand(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, doneWithErrorFunction done): composer_(composer), conversations_(conversations), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("declineConversation"); 
		lSet.insert("decline");
		lSet.insert("dec"); 
		return lSet;
	}

	void help() {
		std::cout << "declineConversation | decline | dec <conversation_id>" << std::endl;
		std::cout << "\tDecline pending conversation." << std::endl;
		std::cout << "\t<conversation_id> - required, conversation id" << std::endl;
		std::cout << "\texample:\n\t\t>decline d1bb559d3a912a0838163c8ea76eb753e5b99e3b24162bb56d2a6426fb3b7f83" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, doneWithErrorFunction done) {
		return std::make_shared<DeclineConversationCommand>(composer, conversations, done); 
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		ctx_ = ctx;
		if (!composer_->requestProcessor()->sendTransaction(ctx->tx()->chain(), ctx,
				SentTransaction::instance(
					boost::bind(&DeclineConversationCommand::declineSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&DeclineConversationCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during decline creation.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

	void declineSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return;
		} else {
			std::cout << "decline: " << tx.toHex() << std::endl;
		}

		done_(ProcessingError());
	}

	void createBuzz();

private:
	BuzzerLightComposerPtr composer_;
	ConversationsfeedPtr conversations_;
	doneWithErrorFunction done_;

	TransactionContextPtr ctx_;
};	

class CreateBuzzerMessageCommand;
typedef std::shared_ptr<CreateBuzzerMessageCommand> CreateBuzzerMessageCommandPtr;

class CreateBuzzerMessageCommand: public ICommand, public std::enable_shared_from_this<CreateBuzzerMessageCommand> {
public:
	CreateBuzzerMessageCommand(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, doneWithErrorFunction done): composer_(composer), conversations_(conversations), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("createMessage"); 
		lSet.insert("message");
		lSet.insert("msg"); 
		return lSet;
	}

	void help() {
		std::cout << "createMessage | message | msg <conversation_id> \"<message>\" [mediafile1 mediafile2 ...]" << std::endl;
		std::cout << "\tSend message to the conversation. Message body and media will be encrypted." << std::endl;
		std::cout << "\t<buzz_id> 	- required, conversation id" << std::endl;
		std::cout << "\t<message> 	- required, message body; can contains multibyte sequences, i.e. 😀 (up to 512 bytes)" << std::endl;
		std::cout << "\t[mediafile]	- optional, media file to upload" << std::endl;
		std::cout << "\texample:\n\t\t>msg d1bb559d3a912a0838163c8ea76eb753e5b99e3b24162bb56d2a6426fb3b7f83 \"Hi, Bob, and welcome!\"" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, doneWithErrorFunction done) {
		return std::make_shared<CreateBuzzerMessageCommand>(composer, conversations, done); 
	}

	// retry
	bool retry() {
		//
		if (ctx_ == nullptr) return false;
		created(ctx_);
		return true;
	}

	// callbacks
	void created(TransactionContextPtr ctx) {
		//
		ctx_ = ctx;
		//
		// push linked and newly created txs; order matters
		IPeerPtr lPeer = nullptr;
		for (std::list<TransactionContextPtr>::iterator lLinkedCtx = ctx->linkedTxs().begin(); lLinkedCtx != ctx->linkedTxs().end(); lLinkedCtx++) {
			if (!(lPeer = composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), *lLinkedCtx,
					SentTransaction::instance(
						boost::bind(&CreateBuzzerMessageCommand::feeSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
						boost::bind(&CreateBuzzerMessageCommand::timeout, shared_from_this()))))) {
				gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
				composer_->wallet()->resetCache();
				composer_->wallet()->prepareCache();
				done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
			}
		}

		if (!composer_->requestProcessor()->sendTransaction(lPeer, ctx->tx()->chain(), ctx,
				SentTransaction::instance(
					boost::bind(&CreateBuzzerMessageCommand::messageSent, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2),
					boost::bind(&CreateBuzzerMessageCommand::timeout, shared_from_this())))) {
			gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
			composer_->wallet()->resetCache();
			composer_->wallet()->prepareCache();
			done_(ProcessingError("E_TX_NOT_SENT", "Transaction was not sent."));
		}
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzz creation.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

	void feeSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return;
		} else {
			std::cout << "    fee: " << tx.toHex() << std::endl;
			//
			TransactionContextPtr lFee = ctx_->locateByType(Transaction::FEE);
			for (std::list<Transaction::NetworkUnlinkedOutPtr>::iterator lOut = lFee->externalOuts().begin(); 
																			lOut != lFee->externalOuts().end(); lOut++) {
				composer_->wallet()->updateOut(*lOut, ctx_->tx()->id(), ctx_->tx()->type());
			}
		}

		feeSent_ = true;
		if (feeSent_ && messageSent_) done_(ProcessingError());
	}

	void messageSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				done_(ProcessingError("E_SENT_TX", lError->data()));
			}

			return;
		} else {
			std::cout << "message: " << tx.toHex() << std::endl;
		}

		messageSent_ = true;
		if (feeSent_ && messageSent_) done_(ProcessingError());
	}

	void setUploadMedia(ICommandPtr uploadMedia) {
		uploadMedia_ = uploadMedia;
	}

	void uploadNextMedia(const PKey&);
	void mediaUploaded(TransactionPtr, const ProcessingError&);

	void setMediaUploaded(doneTransactionWithErrorFunction mediaUploaded) {
		mediaUploaded_ = mediaUploaded;
	}

	void createBuzz(const PKey&);
	void conversationLoaded(TransactionPtr);
	void createMessage(const PKey&);

private:
	BuzzerLightComposerPtr composer_;
	ConversationsfeedPtr conversations_;
	doneWithErrorFunction done_;
	doneTransactionWithErrorFunction mediaUploaded_;
	PKey pkey_;

	uint256 conversationId_;
	std::string body_;

	ICommandPtr uploadMedia_;
	std::vector<BuzzerMediaPointer> mediaPointers_;
	std::vector<std::string> mediaFiles_;

	TransactionContextPtr ctx_;

	bool feeSent_ = false;
	bool messageSent_ = false;
};	

class SelectConversationCommand;
typedef std::shared_ptr<SelectConversationCommand> SelectConversationCommandPtr;

class SelectConversationCommand: public ICommand, public std::enable_shared_from_this<SelectConversationCommand> {
public:
	SelectConversationCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done): composer_(composer), buzzFeed_(buzzFeed), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("selectConversation"); 
		lSet.insert("select");
		lSet.insert("selc"); 
		return lSet;
	}

	void help() {
		std::cout << "selectConversation | select | selc <conversation_id>" << std::endl;
		std::cout << "\tSelect conversation for interactive messaging." << std::endl;
		std::cout << "\t<conversation_id> - required, conversation id" << std::endl;
		std::cout << "\texample:\n\t\t>select d1bb559d3a912a0838163c8ea76eb753e5b99e3b24162bb56d2a6426fb3b7f83" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneWithErrorFunction done) {
		return std::make_shared<SelectConversationCommand>(composer, buzzFeed, done); 
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

private:
	BuzzerLightComposerPtr composer_;
	BuzzfeedPtr buzzFeed_;
	doneWithErrorFunction done_;

	TransactionContextPtr ctx_;
};	

class ConversationListCommand;
typedef std::shared_ptr<ConversationListCommand> ConversationListCommandPtr;

class ConversationListCommand: public ICommand, public std::enable_shared_from_this<ConversationListCommand> {
public:
	ConversationListCommand(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneFunction done): composer_(composer), buzzFeed_(buzzFeed), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("conversationList"); 
		lSet.insert("cclist"); 
		lSet.insert("ccl"); 
		return lSet;
	}

	void help() {
		std::cout << "conversationList | cclist | ccl" << std::endl;
		std::cout << "\tShow current conversation messages." << std::endl;
		std::cout << "\texample:\n\t\t>ccl" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, BuzzfeedPtr buzzFeed, doneFunction done) { 
		return std::make_shared<ConversationListCommand>(composer, buzzFeed, done); 
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_();
	}

	void buzzesLoaded(const std::vector<BuzzfeedItem>& /*feed*/, const uint256& /*chain*/);

	void processPengingInfos();
	void buzzerInfoLoaded(const std::vector<TransactionPtr>&);
	void show();

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during buzzes loading.");
	}

	void display(const std::vector<BuzzfeedItemPtr>&);

private:
	BuzzerLightComposerPtr composer_;
	BuzzfeedPtr buzzFeed_;
	doneFunction done_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pending_;
	std::set<uint256> pendingLoaded_;
	std::map<uint256 /*chain*/, std::set<uint256>/*items*/> pengindChainInfos_;
	std::map<uint256 /*info tx*/, Buzzer::Info> pendingInfos_;
	int pendingChainInfosLoaded_ = 0;
};

class DecryptBuzzerMessageCommand;
typedef std::shared_ptr<DecryptBuzzerMessageCommand> DecryptBuzzerMessageCommandPtr;

class DecryptBuzzerMessageCommand: public ICommand, public std::enable_shared_from_this<DecryptBuzzerMessageCommand> {
public:
	DecryptBuzzerMessageCommand(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, BuzzfeedPtr conversation, doneStringStringWithErrorFunction done): 
		composer_(composer), conversations_(conversations), conversation_(conversation), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("decryptMessage"); 
		lSet.insert("decrypt");
		return lSet;
	}

	void help() {
		std::cout << "decryptMessage | decrypt <message_id>" << std::endl;
		std::cout << "\tDecrypt message." << std::endl;
		std::cout << "\t<message_id> - required, message id" << std::endl;
		std::cout << "\texample:\n\t\t>decrypt d1bb559d3a912a0838163c8ea76eb753e5b99e3b24162bb56d2a6426fb3b7f83" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, BuzzfeedPtr conversation, doneStringStringWithErrorFunction done) {
		return std::make_shared<DecryptBuzzerMessageCommand>(composer, conversations, conversation, done); 
	}

	void terminate() {
		// reset callbacks
		done_ = 0;
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during message decryption.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		if (done_) done_("", "", ProcessingError(code, message));
	}

	void conversationLoaded(TransactionPtr);

private:
	BuzzerLightComposerPtr composer_;
	ConversationsfeedPtr conversations_;
	BuzzfeedPtr conversation_;
	doneStringStringWithErrorFunction done_;

	uint256 messageId_;
};	

class DecryptMessageBodyCommand;
typedef std::shared_ptr<DecryptMessageBodyCommand> DecryptMessageBodyCommandPtr;

class DecryptMessageBodyCommand: public ICommand, public std::enable_shared_from_this<DecryptMessageBodyCommand> {
public:
	DecryptMessageBodyCommand(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, doneStringStringWithErrorFunction done): 
		composer_(composer), conversations_(conversations), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("decryptBody"); 
		return lSet;
	}

	void help() {
		std::cout << "decryptBody <conversation_id>" << std::endl;
		std::cout << "\tDecrypt conversation body (last message)." << std::endl;
		std::cout << "\t<conversation_id> - required, conversation id" << std::endl;
		std::cout << "\texample:\n\t\t>decrypt d1bb559d3a912a0838163c8ea76eb753e5b99e3b24162bb56d2a6426fb3b7f83" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, doneStringStringWithErrorFunction done) {
		return std::make_shared<DecryptMessageBodyCommand>(composer, conversations, done); 
	}

	void terminate() {
		// reset callbacks
		done_ = 0;
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during message decryption.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		if (done_) done_("", "", ProcessingError(code, message));
	}

	void conversationLoaded(TransactionPtr);

private:
	BuzzerLightComposerPtr composer_;
	ConversationsfeedPtr conversations_;
	BuzzfeedPtr conversation_;
	doneStringStringWithErrorFunction done_;

	uint256 conversationId_;
	std::string hexBody_;
};	

class LoadCounterpartyKeyCommand;
typedef std::shared_ptr<LoadCounterpartyKeyCommand> LoadCounterpartyKeyCommandPtr;

class LoadCounterpartyKeyCommand: public ICommand, public std::enable_shared_from_this<LoadCounterpartyKeyCommand> {
public:
	LoadCounterpartyKeyCommand(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, doneWithErrorFunction done): 
		composer_(composer), conversations_(conversations), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("loadCounterpartyKey"); 
		lSet.insert("loadCKey"); 
		return lSet;
	}

	void help() {
		std::cout << "loadCKey <conversation_id>" << std::endl;
		std::cout << "\tLoad counterarty pkey." << std::endl;
		std::cout << "\t<conversation_id> - required, conversation id" << std::endl;
		std::cout << "\texample:\n\t\t>loadCKey d1bb559d3a912a0838163c8ea76eb753e5b99e3b24162bb56d2a6426fb3b7f83" << std::endl << std::endl;
	}	

	static ICommandPtr instance(BuzzerLightComposerPtr composer, ConversationsfeedPtr conversations, doneWithErrorFunction done) {
		return std::make_shared<LoadCounterpartyKeyCommand>(composer, conversations, done); 
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during load pkey.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(ProcessingError(code, message));
	}

	void conversationLoaded(TransactionPtr);

private:
	BuzzerLightComposerPtr composer_;
	ConversationsfeedPtr conversations_;
	BuzzfeedPtr conversation_;
	doneWithErrorFunction done_;

	uint256 conversationId_;
};	

} // qbit

#endif
