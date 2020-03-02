#include "httprequesthandler.h"
#include "httpreply.h"
#include "httprequest.h"
#include "log/log.h"
#include "json.h"
#include "tinyformat.h"
#include "httpendpoints.h"
#include "vm/vm.h"

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
		lReply.addString("result", strprintf(TxAssetType::scaleFormat(lScale), lBalance));
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
			if (lP2.isString()) lValue = (double)(boost::lexical_cast<double>(lP2.getString()));
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

		// prepare reply
		json::Document lReply;
		lReply.loadFromString("{}");

		// process
		std::string lCode, lMessage;
		TransactionContextPtr lCtx = nullptr;
		try {
			// create tx
			lCtx = wallet_->createTxSpend(lAsset, lAddress, (amount_t)(lValue * (double)lScale));
			if (lCtx->errors().size()) {
				reply = HttpReply::stockReply("E_TX_CREATE_SPEND", *lCtx->errors().begin()); 
				return;
			}

			// push to memory pool
			IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate(MainChain::id()); // all spend txs - to the main chain
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

void HttpGetPeerInfo::process(const std::string& source, const HttpRequest& request, const json::Document& data, HttpReply& reply) {
	/* request
	{
		"jsonrpc": "1.0",
		"id": "curltext",
		"method": "getpeerinfo",
		"params": []
	}
	*/
	/* reply
	{
		"result": [
			{
				"id": "<peer_id>",				-- (string) peer default address id (uint160)
				"endpoint": "address:port",		-- (string) peer endpoint
				"outbound": true|false,			-- (bool) is outbound connection
				"roles": "<peer_roles>",		-- (string) peer roles
				"status": "<peer_status>",		-- (string) peer status
				"latency": <latency>,			-- (int) latency, ms
				"time": "<peer_time>",			-- (string) peer_time, s
				"chains": [
					{
						"id": "<chain_id>",		-- (string) chain id
						...
					}
				]
			},
			...
		],
		"error":								-- (object or null) error description
		{
			"code": "EFAIL", 
			"message": "<explanation>" 
		},
		"id": "curltext"						-- (string) request id
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

		// prepare reply
		json::Document lReply;
		lReply.loadFromString("{}");
		//lReply.addString("result", strprintf(QBIT_FORMAT, lBalance));
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

void HttpCreateDApp::process(const std::string& source, const HttpRequest& request, const json::Document& data, HttpReply& reply) {
	/* request
	{
		"jsonrpc": "1.0",
		"id": "curltext",
		"method": "createdapp",
		"params": [
			"<address>", 		-- (string) owners' address
			"<short_name>",		-- (string) dapp short name, should be unique
			"<description>",	-- (string) dapp description
			"<sharding>"		-- (string, optional) static|dynamic, default = 'static'
		]
	}
	*/
	/* reply
	{
		"result": "<tx_dapp_id>",		-- (string) txid = dapp_id
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
		std::string lShortName; // 1
		std::string lDescription; // 2
		std::string lSharding = "static"; // 3

		if (lParams.size() == 3 || lParams.size() == 4) {
			// param[0]
			json::Value lP0 = lParams[0];
			if (lP0.isString()) lAddress.fromString(lP0.getString());
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

			// param[1]
			json::Value lP1 = lParams[1];
			if (lP1.isString()) lShortName = lP1.getString();
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

			// param[2]
			json::Value lP2 = lParams[2];
			if (lP2.isString()) lDescription = lP2.getString();
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

			// param[3]
			if (lParams.size() == 4) {
				json::Value lP3 = lParams[3];
				if (lP3.isString()) lSharding = lP3.getString();
				else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }
			}
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
			lCtx = wallet_->createTxDApp(lAddress, lShortName, lDescription, (lSharding == "static" ? TxDApp::STATIC : TxDApp::DYNAMIC));
			if (lCtx->errors().size()) {
				reply = HttpReply::stockReply("E_TX_CREATE_DAPP", *lCtx->errors().begin()); 
				return;
			}

			// push to memory pool
			IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate(MainChain::id()); // dapp -> main chain
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

void HttpCreateShard::process(const std::string& source, const HttpRequest& request, const json::Document& data, HttpReply& reply) {
	/* request
	{
		"jsonrpc": "1.0",
		"id": "curltext",
		"method": "createshard",
		"params": [
			"<address>", 		-- (string) creators' address (not owner)
			"<dapp_name>", 		-- (string) dapp name
			"<short_name>",		-- (string) shard short name, should be unique
			"<description>"		-- (string) shard description
		]
	}
	*/
	/* reply
	{
		"result": "<tx_shard_id>",		-- (string) txid = shard_id
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
		std::string lDAppName; // 1
		std::string lShortName; // 2
		std::string lDescription; // 3

		if (lParams.size() == 4) {
			// param[0]
			json::Value lP0 = lParams[0];
			if (lP0.isString()) lAddress.fromString(lP0.getString());
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

			// param[1]
			json::Value lP1 = lParams[1];
			if (lP1.isString()) lDAppName = lP1.getString();
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

			// param[2]
			json::Value lP2 = lParams[2];
			if (lP2.isString()) lShortName = lP2.getString();
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
			lCtx = wallet_->createTxShard(lAddress, lDAppName, lShortName, lDescription);
			if (lCtx->errors().size()) {
				reply = HttpReply::stockReply("E_TX_CREATE_SHARD", *lCtx->errors().begin()); 
				return;
			}

			// push to memory pool
			IMemoryPoolPtr lMempool = wallet_->mempoolManager()->locate(MainChain::id()); // dapp -> main chain
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

void HttpGetTransaction::process(const std::string& source, const HttpRequest& request, const json::Document& data, HttpReply& reply) {
	/* request
	{
		"jsonrpc": "1.0",
		"id": "curltext",
		"method": "gettransaction",
		"params": [
			"<tx_id>" 			-- (string) tx hash (id)
		]
	}
	*/
	/* reply
	{
		"result": {
			"id": "<tx_id>",				-- (string) tx hash (id)
			"chain": "<chain_id>",			-- (string) chain / shard hash (id)
			"type": "<tx_type>",			-- (string) tx type: COINBASE, SPEND, SPEND_PRIVATE & etc.
			"version": <version>,			-- (int) version (0-256)  
			"timelock: <lock_time>,			-- (int64) lock time (future block)
			"block": "<block_id>",			-- (string) block hash (id), optional
			"height": <height>,				-- (int64) block height, optional
			"index": <index>,				-- (int) tx block index
			"mempool": false|true,			-- (bool) mempool presence, optional
			"properties": {
				...							-- (object) tx type-specific properties
			},
			"in": [
				{
					"chain": "<chain_id>",	-- (string) source chain/shard hash (id)
					"asset": "<asset_id>",	-- (string) source asset hash (id)
					"tx": "<tx_id>",		-- (string) source tx hash (id)
					"index": <index>,		-- (int) source tx out index
					"ownership": {
						"raw": "<hex>",		-- (string) ownership script (hex)
						"qasm": [			-- (array, string) ownership script disassembly
							"<qasm> <p0>, ... <pn>",
							...
						]
					}
				}
			],
			"out": [
				{
					"asset": "<asset_id>",	-- (string) destination asset hash (id)
					"destination": {
						"raw": "<hex>",		-- (string) destination script (hex)
						"qasm": [			-- (array, string) destination script disassembly
							"<qasm> <p0>, ... <pn>",
							...
						]
					}
				}
			]
		},
		"error":								-- (object or null) error description
		{
			"code": "EFAIL", 
			"message": "<explanation>" 
		},
		"id": "curltext"						-- (string) request id
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
		uint256 lTxId; // 0

		if (lParams.size() == 1) {
			// param[0]
			json::Value lP0 = lParams[0];
			if (lP0.isString()) lTxId.setHex(lP0.getString());
			else { reply = HttpReply::stockReply(HttpReply::bad_request); return; }

		} else {
			reply = HttpReply::stockReply("E_PARAMS", "Insufficient or extra parameters"); 
			return;
		}

		// prepare reply
		json::Document lReply;
		lReply.loadFromString("{}");

		// process
		TransactionPtr lTx;
		std::string lCode, lMessage;

		// try to lookup transaction
		ITransactionStoreManagerPtr lStoreManager = wallet_->storeManager();
		IMemoryPoolManagerPtr lMempoolManager = wallet_->mempoolManager();
		if (lStoreManager && lMempoolManager) {
			//
			uint256 lBlock;
			uint64_t lHeight = 0;
			uint64_t lConfirms = 0;
			uint32_t lIndex = 0;
			bool lCoinbase = false;
			bool lMempool = false;

			std::vector<ITransactionStorePtr> lStorages = lStoreManager->storages();
			for (std::vector<ITransactionStorePtr>::iterator lStore = lStorages.begin(); lStore != lStorages.end(); lStore++) {
				lTx = (*lStore)->locateTransaction(lTxId);
				if (lTx && (*lStore)->transactionInfo(lTxId, lBlock, lHeight, lConfirms, lIndex, lCoinbase)) {
					break;
				}
			}

			// try mempool
			if (!lTx) {
				std::vector<IMemoryPoolPtr> lMempools = lMempoolManager->pools();
				for (std::vector<IMemoryPoolPtr>::iterator lPool = lMempools.begin(); lPool != lMempools.end(); lPool++) {
					lTx = (*lPool)->locateTransaction(lTxId);
					if (lTx) {
						lMempool = true;
						break;
					}
				}
			}

			if (lTx) {
				//
				if (!unpackTransaction(lTx, lBlock, lHeight, lConfirms, lIndex, lCoinbase, lMempool, lReply, reply)) 
					return;
			} else {
				reply = HttpReply::stockReply("E_TX_NOT_FOUND", "Transaction not found"); 
				return;
			}
		} else {
			reply = HttpReply::stockReply("E_STOREMANAGER", "Transactions store manager not found"); 
			return;
		}

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
