#include "httprequesthandler.h"
#include "httpreply.h"
#include "httprequest.h"
#include "log/log.h"
#include "json.h"
#include "tinyformat.h"
#include "httpendpoints.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace qbit;

void HttpGetKey::process(const std::string& source, const HttpRequest& request, const json::Document& data, HttpReply& reply) {
	/* request
	{
		"jsonrpc": "1.0",
		"id": "curltext",
		"method": "getkey",
		"params": [
			"<address_id>" 	-- (string, optional) address
		]
	}
	*/
	/* reply
	{
		"result":						-- (object) address details
		{
			"address": "<address>",		-- (string) address, base58 encoded
			"pkey": "<public_key>",		-- (string) public key, hex encoded
			"skey": "<secret_key>",		-- (string) secret key, hex encoded
			"seed": []					-- (string array) seed words
		},
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
		std::string lAddress; // 0
		if (lParams.size()) {
			// param[0]
			json::Value lP0 = lParams[0];
			if (lP0.isString()) lAddress = lP0.getString();
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }
		}

		// process
		SKey lKey;
		if (lAddress.size()) {
			PKey lPKey; 
			if (!lPKey.fromString(lAddress)) {
				reply = HttpReply::stockReply("E_PKEY_INVALID", "Public key is invalid"); 
				return;
			}

			lKey = wallet_->findKey(lPKey);
			if (!lKey.valid()) {
				reply = HttpReply::stockReply("E_SKEY_NOT_FOUND", "Key was not found"); 
				return;
			}
		} else {
			lKey = wallet_->firstKey();
			if (!lKey.valid()) {
				reply = HttpReply::stockReply("E_SKEY_IS_ABSENT", "Key is absent"); 
				return;
			}
		}

		PKey lPFoundKey = lKey.createPKey();

		// prepare reply
		json::Document lReply;
		lReply.loadFromString("{}");
		
		json::Value lKeyObject = lReply.addObject("result");
		lKeyObject.addString("address", lPFoundKey.toString());
		lKeyObject.addString("pkey", lPFoundKey.toHex());
		lKeyObject.addString("skey", lKey.toHex());

		json::Value lKeyArrayObject = lKeyObject.addArray("seed");
		for (std::vector<SKey::Word>::iterator lWord = lKey.seed().begin(); lWord != lKey.seed().end(); lWord++) {
			json::Value lItem = lKeyArrayObject.newArrayItem();
			lItem.setString((*lWord).wordA());
		}

		lReply.addObject("error").toNull();
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

void HttpGetBalance::process(const std::string& source, const HttpRequest& request, const json::Document& data, HttpReply& reply) {
	/* request
	{
		"jsonrpc": "1.0",
		"id": "curltext",
		"method": "getbalance",
		"params": [
			"<asset_id>" 				-- (string, optional) asset
		]
	}
	*/
	/* reply
	{
		"result": "1.0",				-- (string) corresponding asset balance
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
		uint256 lAsset; // 0
		if (lParams.size()) {
			// param[0]
			json::Value lP0 = lParams[0];
			if (lP0.isString()) lAsset.setHex(lP0.getString());
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }
		}

		// process
		amount_t lScale = QBIT;
		if (!lAsset.isNull() && lAsset != TxAssetType::qbitAsset()) {
			// locate asset type
			EntityPtr lAssetEntity = wallet_->persistentStore()->entityStore()->locateEntity(lAsset);
			if (lAssetEntity && lAssetEntity->type() == Transaction::ASSET_TYPE) {
				TxAssetTypePtr lAssetType = TransactionHelper::to<TxAssetType>(lAssetEntity);
				lScale = lAssetType->scale();
			} else {
				reply = HttpReply::stockReply("E_ASSET", "Asset type was not found"); 
				return;
			}
		}

		// process
		double lBalance = 0.0;
		IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate(MainChain::id()); // main chain only
		IConsensus::ChainState lState = lMempool->consensus()->chainState();

		if (lState == IConsensus::SYNCHRONIZED) {
			if (!lAsset.isNull()) lBalance = ((double)wallet_->balance(lAsset)) / lScale;
			else lBalance = ((double)wallet_->balance()) / lScale;
		} else if (lState == IConsensus::SYNCHRONIZING) {
			reply = HttpReply::stockReply("E_NODE_SYNCHRONIZING", "Synchronization is in progress..."); 
			return;
		} else {
			reply = HttpReply::stockReply("E_NODE_NOT_SYNCHRONIZED", "Not synchronized"); 
			return;
		}

		// prepare reply
		json::Document lReply;
		lReply.loadFromString("{}");
		lReply.addString("result", strprintf(QBIT_FORMAT, lBalance));
		lReply.addObject("error").toNull();
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

void HttpSendToAddress::process(const std::string& source, const HttpRequest& request, const json::Document& data, HttpReply& reply) {
	/* request
	{
		"jsonrpc": "1.0",
		"id": "curltext",
		"method": "sendtoaddress",
		"params": [
			"<asset_id>", 	-- (string) asset or "*"
			"<address>", 	-- (string) address
			"0.1"			-- (string) amount
		]
	}
	*/
	/* reply
	{
		"result": "<tx_id>",			-- (string) txid
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
		std::string lAssetString;	// 0
		PKey lAddress; // 1
		double lValue; // 2

		if (lParams.size() == 3) {
			// param[0]
			json::Value lP0 = lParams[0];
			if (lP0.isString()) lAssetString = lP0.getString();
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

			// param[1]
			json::Value lP1 = lParams[1];
			if (lP1.isString()) lAddress.fromString(lP1.getString());
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

			// param[2]
			json::Value lP2 = lParams[2];
			if (lP2.isString()) lValue = (amount_t)(boost::lexical_cast<double>(lP2.getString()));
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }
		} else {
			reply = HttpReply::stockReply("E_PARAMS", "Insufficient or extra parameters"); 
			return;
		}

		uint256 lAsset;
		if (lAssetString == "*") lAsset = TxAssetType::qbitAsset();
		else lAsset.setHex(lAssetString);

		// locate scale
		amount_t lScale = QBIT;
		if (!lAsset.isNull() && lAsset != TxAssetType::qbitAsset()) {
			// locate asset type
			EntityPtr lAssetEntity = wallet_->persistentStore()->entityStore()->locateEntity(lAsset);
			if (lAssetEntity && lAssetEntity->type() == Transaction::ASSET_TYPE) {
				TxAssetTypePtr lAssetType = TransactionHelper::to<TxAssetType>(lAssetEntity);
				lScale = lAssetType->scale();
			} else {
				reply = HttpReply::stockReply("E_ASSET", "Asset type was not found"); 
				return;
			}
		}

		// process
		std::string lCode, lMessage;
		TransactionContextPtr lCtx = nullptr;
		try {
			// create tx
			lCtx = wallet_->createTxSpend(lAsset, lAddress, (amount_t)(lValue * lScale));
			if (lCtx->errors().size()) {
				reply = HttpReply::stockReply("E_TX_CREATE_SPEND", *lCtx->errors().begin()); 
				return;
			}

			// push to memory pool
			IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate(MainChain::id()); // all spend txs - to the main chain
			if (lMempool) {
				//
				if (lMempool->pushTransaction(lCtx)) {
					if (lCtx->errors().size()) {
						reply = HttpReply::stockReply("E_TX_MEMORYPOOL", *lCtx->errors().begin()); 
						return;
					}

					if (!lMempool->consensus()->broadcastTransaction(lCtx, wallet_->firstKey().createPKey().id())) {
						lCode = "E_TX_CREATE_SPEND_BCAST";
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

		// prepare reply
		json::Document lReply;
		lReply.loadFromString("{}");
		lReply.addString("result", lCtx->tx()->hash().toHex());
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
