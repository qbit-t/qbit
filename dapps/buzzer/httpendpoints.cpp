#include "../../httprequesthandler.h"
#include "../../httpreply.h"
#include "../../httprequest.h"
#include "../../log/log.h"
#include "../../json.h"
#include "../../tinyformat.h"
#include "httpendpoints.h"
#include "../../vm/vm.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace qbit;

void HttpCreateBuzzer::process(const std::string& source, const HttpRequest& request, const json::Document& data, HttpReply& reply) {
	/* request
	{
		"jsonrpc": "1.0",
		"id": "curltext",
		"method": "createbuzzer",
		"params": [
			"<address>", 		-- (string) creators' address or '*'	
			"<@name>", 			-- (string) buzzer unique name
			"<alias>", 			-- (string) buzzer alias
			"<description>" 	-- (string) buzzer description 
		]
	}
	*/
	/* reply
	{
		"result": "<tx_buzzer_id>",		-- (string) txid = buzzer_id
		"error":						-- (object or null) error description
		{
			"code": "EFAIL", 
			"message": "<explanation>" 
		},
		"id": "curltext"				-- (string) request id
	}
	*/

	// id
	json::Value lId;
	if (!(const_cast<json::Document&>(data).find("id", lId) && lId.isString())) {
		reply = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}

	// params
	json::Value lParams;
	if (const_cast<json::Document&>(data).find("params", lParams) && lParams.isArray()) {
		// extract parameters
		PKey lAddress; // 0
		std::string lName; // 1
		std::string lAlias; // 2
		std::string lDescription; // 3

		if (lParams.size() == 4) {
			// param[0]
			json::Value lP0 = lParams[0];
			if (lP0.isString()) if (lP0.getString() != "*") lAddress.fromString(lP0.getString());
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

			// param[1]
			json::Value lP1 = lParams[1];
			if (lP1.isString()) lName = lP1.getString();
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

			// param[2]
			json::Value lP2 = lParams[2];
			if (lP2.isString()) lAlias = lP2.getString();
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

			// param[3]
			json::Value lP3 = lParams[3];
			if (lP3.isString()) lDescription = lP3.getString();
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }
		} else {
			reply = HttpReply::stockReply("E_PARAMS", "Insufficient or extra parameters"); 
			return;
		}

		// prepare reply
		json::Document lReply;
		lReply.loadFromString("{}");

		// process
		std::string lCode, lMessage;
		TransactionContextPtr lCtx = nullptr;
		try {
			// create tx
			if (lAddress.valid())
				lCtx = composer_->createTxBuzzer(lAddress, lName, lAlias, lDescription);
			else
				lCtx = composer_->createTxBuzzer(lName, lAlias, lDescription);

			if (lCtx->errors().size()) {
				reply = HttpReply::stockReply("E_TX_CREATE_BUZZER", *lCtx->errors().begin()); 
				return;
			}

			// push to memory pool
			IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate(MainChain::id()); // buzzer -> main chain
			if (lMempool) {
				//
				if (lMempool->pushTransaction(lCtx)) {
					// check for errors
					if (lCtx->errors().size()) {
						// unpack
						if (!unpackTransaction(lCtx->tx(), uint256(), 0, 0, 0, false, false, lReply, reply)) return;
						// rollback transaction
						wallet_->rollback(lCtx);
						// error
						lCode = "E_TX_MEMORYPOOL";
						lMessage = *lCtx->errors().begin(); 
						lCtx = nullptr;
					} else if (!lMempool->consensus()->broadcastTransaction(lCtx, wallet_->firstKey().createPKey().id())) {
						lCode = "E_TX_NOT_BROADCASTED";
						lMessage = "Transaction is not broadcasted"; 
					}
				} else {
					lCode = "E_TX_EXISTS";
					lMessage = "Transaction already exists";
					return;
				}
			} else {
				reply = HttpReply::stockReply("E_MEMPOOL", "Corresponding memory pool was not found"); 
				return;
			}

		}
		catch(qbit::exception& ex) {
			reply = HttpReply::stockReply(ex.code(), ex.what()); 
			return;
		}

		if (lCtx != nullptr) lReply.addString("result", lCtx->tx()->hash().toHex());
		if (!lCode.size() && !lMessage.size()) lReply.addObject("error").toNull();
		else {
			json::Value lError = lReply.addObject("error");
			lError.addString("code", lCode);
			lError.addString("message", lMessage);
		}
		lReply.addString("id", lId.getString());

		// pack
		pack(reply, lReply);
		// finalize
		finalize(reply);
	} else {
		reply = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}
}

void HttpBuzz::process(const std::string& source, const HttpRequest& request, const json::Document& data, HttpReply& reply) {
	/* request
	{
		"jsonrpc": "1.0",
		"id": "curltext",
		"method": "buzz",
		"params": [
			"<address>", 		-- (string) creators' address or '*'
			"<body>" 			-- (string) buzz body
		]
	}
	*/
	/* reply
	{
		"result": "<tx_buzz>",			-- (string) txid = buzz_id
		"error":						-- (object or null) error description
		{
			"code": "EFAIL", 
			"message": "<explanation>" 
		},
		"id": "curltext"				-- (string) request id
	}
	*/

	// id
	json::Value lId;
	if (!(const_cast<json::Document&>(data).find("id", lId) && lId.isString())) {
		reply = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}

	// params
	json::Value lParams;
	if (const_cast<json::Document&>(data).find("params", lParams) && lParams.isArray()) {
		// extract parameters
		PKey lAddress; // 0
		std::string lBody; // 1

		if (lParams.size() == 2) {
			// param[0]
			json::Value lP0 = lParams[0];
			if (lP0.isString()) if (lP0.getString() != "*") lAddress.fromString(lP0.getString());
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

			// param[1]
			json::Value lP1 = lParams[1];
			if (lP1.isString()) lBody = lP1.getString();
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }
		} else {
			reply = HttpReply::stockReply("E_PARAMS", "Insufficient or extra parameters"); 
			return;
		}

		// prepare reply
		json::Document lReply;
		lReply.loadFromString("{}");

		// process
		std::string lCode, lMessage;
		TransactionContextPtr lCtx = nullptr;
		try {
			// create tx
			if (lAddress.valid()) 
				lCtx = composer_->createTxBuzz(lAddress, lBody);
			else
				lCtx = composer_->createTxBuzz(lBody);

			if (lCtx->errors().size()) {
				reply = HttpReply::stockReply("E_TX_CREATE_BUZZ", *lCtx->errors().begin()); 
				return;
			}

			// push linked and newly created txs; order matters
			for (std::list<TransactionContextPtr>::iterator lLinkedCtx = lCtx->linkedTxs().begin(); lLinkedCtx != lCtx->linkedTxs().end(); lLinkedCtx++) {
				//
				IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate((*lLinkedCtx)->tx()->chain());
				if (lMempool) {
					//
					if (lMempool->pushTransaction(*lLinkedCtx)) {
						// check for errors
						if ((*lLinkedCtx)->errors().size()) {
							// unpack
							if (!unpackTransaction((*lLinkedCtx)->tx(), uint256(), 0, 0, 0, false, false, lReply, reply)) return;
							// rollback transaction
							wallet_->rollback((*lLinkedCtx));
							// error
							lCode = "E_TX_MEMORYPOOL";
							lMessage = *((*lLinkedCtx)->errors().begin()); 
							lCtx = nullptr;
							break;
						} else if (!lMempool->consensus()->broadcastTransaction((*lLinkedCtx), wallet_->firstKey().createPKey().id())) {
							lCode = "E_TX_NOT_BROADCASTED";
							lMessage = "Transaction is not broadcasted"; 
						}
					} else {
						lCode = "E_TX_EXISTS";
						lMessage = "Transaction already exists";
						return;
					}
				} else {
					reply = HttpReply::stockReply("E_MEMPOOL", "Corresponding memory pool was not found"); 
					return;
				}
			}

			// push to memory pool
			if (lCtx) {
				IMemoryPoolPtr lBuzzMempool = wallet_->mempoolManager()->locate(lCtx->tx()->chain());
				if (lBuzzMempool) {
					//
					if (lBuzzMempool->pushTransaction(lCtx)) {
						// check for errors
						if (lCtx->errors().size()) {
							// unpack
							if (!unpackTransaction(lCtx->tx(), uint256(), 0, 0, 0, false, false, lReply, reply)) return;
							// rollback transaction
							wallet_->rollback(lCtx);
							// error
							lCode = "E_TX_MEMORYPOOL";
							lMessage = *lCtx->errors().begin(); 
							lCtx = nullptr;
						} else if (!lBuzzMempool->consensus()->broadcastTransaction(lCtx, wallet_->firstKey().createPKey().id())) {
							lCode = "E_TX_NOT_BROADCASTED";
							lMessage = "Transaction is not broadcasted"; 
						}
					} else {
						lCode = "E_TX_EXISTS";
						lMessage = "Transaction already exists";
						return;
					}
				} else {
					reply = HttpReply::stockReply("E_MEMPOOL", "Corresponding memory pool was not found"); 
					return;
				}
			}
		}
		catch(qbit::exception& ex) {
			reply = HttpReply::stockReply(ex.code(), ex.what()); 
			return;
		}

		if (lCtx != nullptr) lReply.addString("result", lCtx->tx()->hash().toHex());
		if (!lCode.size() && !lMessage.size()) lReply.addObject("error").toNull();
		else {
			json::Value lError = lReply.addObject("error");
			lError.addString("code", lCode);
			lError.addString("message", lMessage);
		}
		lReply.addString("id", lId.getString());

		// pack
		pack(reply, lReply);
		// finalize
		finalize(reply);
	} else {
		reply = HttpReply::stockReply(HttpReply::bad_request);
		return;
	}
}
