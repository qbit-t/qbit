// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_ICOMMAND_H
#define QBIT_ICOMMAND_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../allocator.h"
#include "../iwallet.h"
#include "../irequestprocessor.h"

#include <vector>

namespace qbit {

class ICommand {
public:
	ICommand() {}

	virtual void process(const std::vector<std::string>&) { throw qbit::exception("NOT_IMPL", "ICommand::process - not implemented."); }
	virtual std::set<std::string> name() { throw qbit::exception("NOT_IMPL", "ICommand::name - not implemented."); }
	virtual void help() { throw qbit::exception("NOT_IMPL", "ICommand::help - not implemented."); }
	virtual void terminate() { throw qbit::exception("NOT_IMPL", "ICommand::terminate - not implemented."); }

	void setWallet(IWalletPtr wallet) { wallet_ = wallet; }
	void setRequestProcessor(IRequestProcessorPtr requestProcessor) { requestProcessor_ = requestProcessor; }

protected:
	IWalletPtr wallet_;
	IRequestProcessorPtr requestProcessor_;
};

typedef std::shared_ptr<ICommand> ICommandPtr;

} // qbit

#endif
