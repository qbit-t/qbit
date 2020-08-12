// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CUBIX_COMMANDS_H
#define QBIT_CUBIX_COMMANDS_H

#include "../../icommand.h"
#include "../../../ipeer.h"
#include "../../../log/log.h"
#include "cubixcomposer.h"

namespace qbit {
namespace cubix {

class UploadMediaCommand;
typedef std::shared_ptr<UploadMediaCommand> UploadMediaCommandPtr;

class UploadMediaCommand: public ICommand, public std::enable_shared_from_this<UploadMediaCommand> {
public:
	UploadMediaCommand(CubixLightComposerPtr composer, progressUploadFunction progress, doneTransactionWithErrorFunction done): 
		composer_(composer), progress_(progress), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("uploadMedia"); 
		lSet.insert("upload"); 
		return lSet;
	}

	void help() {
		std::cout << "uploadMedia | upload \"<file>\" [-s <000x000>] [\"description\"]" << std::endl;
		std::cout << "\tUpload media file to the cubix network." << std::endl;
		std::cout << "\t<file> 			- path to media file (jpeg, png)" << std::endl;
		std::cout << "\t[-s <000x000>] 	- optional, preview with specified size" << std::endl;
		std::cout << "\t[description]	- optional, media file description" << std::endl;
		std::cout << "\texample:\n\t\t>upload \"./my_photo.png\" \"My photo\"" << std::endl << std::endl;
	}	

	static ICommandPtr instance(CubixLightComposerPtr composer, progressUploadFunction progress, doneTransactionWithErrorFunction done) { 
		return std::make_shared<UploadMediaCommand>(composer, progress, done); 
	}

	// callbacks
	void summaryCreated(TransactionContextPtr ctx, Transaction::UnlinkedOutPtr utxo);
	void dataCreated(TransactionContextPtr ctx, Transaction::UnlinkedOutPtr utxo);
	void headerCreated(TransactionContextPtr ctx);

	void feeSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors);
	void summarySent(const uint256& tx, const std::vector<TransactionContext::Error>& errors);
	void dataSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors);
	void headerSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors);

	void startSendData();
	void continueSendData();

	bool readNextChunk(int64_t&, std::vector<unsigned char>&);

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during media upload.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(nullptr, ProcessingError(code, message));
	}

	bool checkSent(const std::string& msg, const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
		//
		if (errors.size()) {
			for (std::vector<TransactionContext::Error>::iterator lError = const_cast<std::vector<TransactionContext::Error>&>(errors).begin(); 
					lError != const_cast<std::vector<TransactionContext::Error>&>(errors).end(); lError++) {
				gLog().writeClient(Log::CLIENT, strprintf("[error]: %s", lError->data()));
				//
				done_(nullptr, ProcessingError("E_UPLOAD_DATA", lError->data()));
			}

			return false;
		} else {
			std::cout << msg << tx.toHex() << std::endl;
		}

		return true;
	}

	void setDone(doneTransactionWithErrorFunction done) {
		done_ = done;
	}

private:
	CubixLightComposerPtr composer_;
	doneTransactionWithErrorFunction done_;
	progressUploadFunction progress_;

	uint64_t size_ = 0;
	std::string file_;
	std::string description_;

	TxMediaHeader::Type mediaType_;
	std::vector<unsigned char> previewData_;
	std::ifstream mediaFile_;
	Transaction::UnlinkedOutPtr prev_;
	int64_t pos_ = 0;
	unsigned short orientation_ = 0;

	TxMediaHeaderPtr header_;
	TxMediaSummaryPtr summary_;
	TransactionContextPtr ctx_;

	int previewWidth_ = 0;
	int previewHeight_ = 0;

	IPeerPtr peer_;

	bool feeSent_ = false;
	bool summarySent_ = false;
	bool headerSent_ = false;
};

class DownloadMediaCommand;
typedef std::shared_ptr<DownloadMediaCommand> DownloadMediaCommandPtr;

class DownloadMediaCommand: public ICommand, public std::enable_shared_from_this<DownloadMediaCommand> {
public:
	DownloadMediaCommand(CubixLightComposerPtr composer, progressFunction progress, doneDownloadWithErrorFunction done): 
		composer_(composer), progress_(progress), done_(done) {}

	void process(const std::vector<std::string>&);
	std::set<std::string> name() {
		std::set<std::string> lSet;
		lSet.insert("downloadMedia"); 
		lSet.insert("download"); 
		return lSet;
	}

	void help() {
		std::cout << "downloadMedia | download [<header>/<chain>] \"<local_file>\" [preview] [skip]" << std::endl;
		std::cout << "\tDownload media from cubix network." << std::endl;
		std::cout << "\t<header> 	 - required, header tx" << std::endl;
		std::cout << "\t<chain>		 - required, cubix chain" << std::endl;
		std::cout << "\t<local_file> - required, local path/file name without extension" << std::endl;
		std::cout << "\t[preview]	 - optional, download preview only" << std::endl;
		std::cout << "\t[skip]	 	 - optional, skip if already exists" << std::endl;
		std::cout << "\texample:\n\t\t>download 1a2b6913c6229d9a18cd7fd657f03c660dfe7d127cd98913bd41d145046a4d01/9f8b3cd91d2deae6209e0fef18e05e962002aceccfdd5da7cfdac85b74b4e7e8 \"./my_photo.png\" preview" << std::endl << std::endl;
	}	

	static ICommandPtr instance(CubixLightComposerPtr composer, progressFunction progress, doneDownloadWithErrorFunction done) { 
		return std::make_shared<DownloadMediaCommand>(composer, progress, done); 
	}

	// callbacks
	void headerLoaded(TransactionPtr);
	void dataLoaded(TransactionPtr);

	// data
	TxMediaHeader::Type mediaType() { return header_->mediaType(); }
	void headerData(std::vector<unsigned char>& data) {
		data.insert(data.end(), header_->data().begin(), header_->data().end());
	}

	void timeout() {
		error("E_TIMEOUT", "Timeout expired during media download.");
	}

	void error(const std::string& code, const std::string& message) {
		gLog().writeClient(Log::CLIENT, strprintf(": %s | %s", code, message));
		done_(nullptr, std::string(), std::string(), 0, ProcessingError(code, message));
	}	

private:
	CubixLightComposerPtr composer_;
	doneDownloadWithErrorFunction done_;
	progressFunction progress_;

	uint256 headerTx_;
	uint256 nextDataTx_;
	uint256 chain_;
	TxMediaHeaderPtr header_;

	int64_t pos_ = 0;

	std::ofstream outLocalFile_;

	std::string localFileName_;
	std::string localPreviewFileName_;

	std::string localFile_;
	
	bool previewOnly_ = false;
	bool skipIfExists_ = false;
};

}
}

#endif