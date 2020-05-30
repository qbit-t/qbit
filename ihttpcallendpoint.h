// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_IHTTP_CALL_ENDPOINT_H
#define QBIT_IHTTP_CALL_ENDPOINT_H

//
// allocator.h _MUST_ be included BEFORE all other
//
#include "allocator.h"
#include "httpreply.h"
#include "httprequest.h"
#include "iwallet.h"
#include "json.h"
#include "ipeermanager.h"
#include "vm/vm.h"

#include <boost/lexical_cast.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>

namespace qbit {

class IHttpCallEnpoint {
public:
	IHttpCallEnpoint() {}

	virtual void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&) { throw qbit::exception("NOT_IMPL", "IHttpCallEnpoint::process - not implemented."); }
	virtual std::string method() { throw qbit::exception("NOT_IMPL", "IHttpCallEnpoint::method - not implemented."); }
	
	void setWallet(IWalletPtr wallet) { wallet_ = wallet; }
	void setPeerManager(IPeerManagerPtr peerManager) { peerManager_ = peerManager; }

	void pack(HttpReply& reply, json::Document& data) {
		// pack
		data.writeToString(reply.content); // TODO: writeToStringPlain for minimizing output
		reply.content += "\n"; // extra line
	}

	void finalize(HttpReply& reply) {
		reply.status = HttpReply::ok;
		reply.headers.resize(2);
		reply.headers[0].name = "Content-Length";
		reply.headers[0].value = boost::lexical_cast<std::string>(reply.content.size());
		reply.headers[1].name = "Content-Type";
		reply.headers[1].value = "application/json";		
	}

protected:
	template<typename to> bool convert(const std::string& data, to& value) {
		return boost::conversion::try_lexical_convert<to>(data, value);
	}

	bool unpackTransaction(TransactionPtr tx, const uint256& block, uint64_t height, uint64_t confirms, uint32_t index, bool coinbase, bool mempool, json::Document& reply, HttpReply& httpReply) {
		// process
		TransactionPtr lTx = tx;
		TransactionContextPtr lCtx = TransactionContext::instance(lTx);

		// try to lookup transaction
		ITransactionStoreManagerPtr lStoreManager = wallet_->storeManager();
		IMemoryPoolManagerPtr lMempoolManager = wallet_->mempoolManager();
		if (lStoreManager && lMempoolManager) {
			//
			std::vector<ITransactionStorePtr> lStorages = lStoreManager->storages();
			std::vector<IMemoryPoolPtr> lMempools = lMempoolManager->pools();
			//
			json::Value lObject = reply.addObject("result");
			lObject.addString("id", lTx->id().toHex());
			lObject.addString("chain", lTx->chain().toHex());
			lObject.addString("type", lTx->name());
			lObject.addInt("size", lCtx->size());
			lObject.addInt("version", (int)lTx->version());
			lObject.addInt64("timelock", lTx->timeLock());

			if (mempool) {
				lObject.addBool("mempool", mempool);
			} else if (!block.isNull()) {
				lObject.addString("block", const_cast<uint256&>(block).toHex());
				lObject.addInt64("height", height);
				lObject.addInt64("confirmations", confirms);
				lObject.addInt("index", index);
			}

			std::map<std::string, std::string> lProps;
			lTx->properties(lProps);

			json::Value lProperties = lObject.addObject("properties");
			for (std::map<std::string, std::string>::iterator lProp = lProps.begin(); lProp != lProps.end(); lProp++) {
				lProperties.addString(lProp->first, lProp->second);
			}

			// in
			json::Value lInArray = lObject.addArray("in");
			std::vector<Transaction::In>& lIns = lTx->in();
			for(std::vector<Transaction::In>::iterator lInPtr = lIns.begin(); lInPtr != lIns.end(); lInPtr++) {
				//
				Transaction::In& lIn = (*lInPtr);
				json::Value lItem = lInArray.newArrayItem();
				lItem.toObject(); // make object

				lItem.addString("chain", lIn.out().chain().toHex());
				lItem.addString("asset", lIn.out().asset().toHex());
				lItem.addString("tx", lIn.out().tx().toHex());
				lItem.addInt("index", lIn.out().index());

				// try to locate ltxo
				uint256 lUtxoId = lIn.out().hash();
				for (std::vector<ITransactionStorePtr>::iterator lStore = lStorages.begin(); lStore != lStorages.end(); lStore++) {
					Transaction::UnlinkedOutPtr lUtxoPtr = (*lStore)->findUnlinkedOut(lUtxoId);
					if (!lUtxoPtr) lUtxoPtr = (*lStore)->findLinkedOut(lUtxoId);
					if (lUtxoPtr) {
						if (lUtxoPtr->address().valid()) lItem.addString("address", lUtxoPtr->address().toString());
					}
				}

				lItem.addString("ltxo", lUtxoId.toHex());

				json::Value lOwnership = lItem.addObject("ownership");
				lOwnership.addString("raw", HexStr(lIn.ownership().begin(), lIn.ownership().end()));

				json::Value lDisassembly = lOwnership.addArray("qasm");

				qasm::ByteCode lCode; lCode << lIn.ownership();
				VirtualMachine lVM(lCode);
				std::list<VirtualMachine::DisassemblyLine> lLines;
				lVM.disassemble(lLines);

				for (std::list<VirtualMachine::DisassemblyLine>::iterator lLine = lLines.begin(); lLine != lLines.end(); lLine++) {
					//
					json::Value lLineItem = lDisassembly.newArrayItem();
					lLineItem.setString((*lLine).toString());
				}
			}

			// out
			uint32_t lIdx = 0;
			json::Value lOutArray = lObject.addArray("out");
			std::vector<Transaction::Out>& lOuts = lTx->out();
			for(std::vector<Transaction::Out>::iterator lOutPtr = lOuts.begin(); lOutPtr != lOuts.end(); lOutPtr++, lIdx++) {
				//
				Transaction::Out& lOut = (*lOutPtr);
				json::Value lItem = lOutArray.newArrayItem();
				lItem.toObject(); // make object

				lItem.addString("asset", lOut.asset().toHex());

				// locate asset and asset parameters
				amount_t lScale = QBIT;
				if (!lOut.asset().isNull() && lOut.asset() != TxAssetType::qbitAsset() && 
														lOut.asset() != TxAssetType::nullAsset()) {
					// locate asset type
					EntityPtr lAssetEntity = wallet_->persistentStore()->entityStore()->locateEntity(lOut.asset());
					if (lAssetEntity && lAssetEntity->type() == Transaction::ASSET_TYPE) {
						TxAssetTypePtr lAssetType = TransactionHelper::to<TxAssetType>(lAssetEntity);
						lScale = lAssetType->scale();
					} else {
						httpReply = HttpReply::stockReply("E_ASSET", "Asset type was not found"); 
						return false;
					}
				}

				// try to find wallet utxo
				Transaction::Link lUtxo(lTx->chain(), lOut.asset(), lTx->id(), lIdx);
				uint256 lUtxoId = lUtxo.hash();
				Transaction::UnlinkedOutPtr lUtxoPtr = wallet_->findUnlinkedOut(lUtxoId);
				if (!lUtxoPtr) lUtxoPtr = wallet_->findLinkedOut(lUtxoId);
				// if found
				if (lUtxoPtr) {
					// extract amount
					if (lUtxoPtr->address().valid()) lItem.addString("address", lUtxoPtr->address().toString());
					if (lUtxoPtr->amount() > 0) {
						lItem.addUInt64("amount", lUtxoPtr->amount());
						lItem.addString("value", strprintf(TxAssetType::scaleFormat(lScale), 
														((double)lUtxoPtr->amount() / (double)lScale)));
					}

					lItem.addString("utxo", lUtxoPtr->out().hash().toHex());
					lItem.addBool("wallet", true);
				} else {
					// try to locate in storages
					for (std::vector<ITransactionStorePtr>::iterator lStore = lStorages.begin(); lStore != lStorages.end(); lStore++) {
						lUtxoPtr = (*lStore)->findUnlinkedOut(lUtxoId);
						if (!lUtxoPtr) lUtxoPtr = (*lStore)->findLinkedOut(lUtxoId);
						if (lUtxoPtr) {
							if (lUtxoPtr->address().valid()) lItem.addString("address", lUtxoPtr->address().toString());
							if (lUtxoPtr->amount() > 0) {
								lItem.addUInt64("amount", lUtxoPtr->amount());
								lItem.addString("value", strprintf(TxAssetType::scaleFormat(lScale), 
															((double)lUtxoPtr->amount() / (double)lScale)));
							}

							lItem.addString("utxo", lUtxoPtr->out().hash().toHex());
						}
					}
				}

				json::Value lDestination = lItem.addObject("destination");
				lDestination.addString("raw", HexStr(lOut.destination().begin(), lOut.destination().end()));

				json::Value lDisassembly = lDestination.addArray("qasm");

				qasm::ByteCode lCode; lCode << lOut.destination();
				VirtualMachine lVM(lCode);
				std::list<VirtualMachine::DisassemblyLine> lLines;
				lVM.disassemble(lLines);

				for (std::list<VirtualMachine::DisassemblyLine>::iterator lLine = lLines.begin(); lLine != lLines.end(); lLine++) {
					//
					json::Value lLineItem = lDisassembly.newArrayItem();
					lLineItem.setString((*lLine).toString());
				}

			}

			// raw
			DataStream lStream(SER_NETWORK, PROTOCOL_VERSION);
			Transaction::Serializer::serialize<DataStream>(lStream, lTx);
			lObject.addString("raw", HexStr(lStream.begin(), lStream.end()));
		}

		return true;
	}

protected:
	IWalletPtr wallet_;
	IPeerManagerPtr peerManager_;
};

typedef std::shared_ptr<IHttpCallEnpoint> IHttpCallEnpointPtr;

} // qbit

#endif