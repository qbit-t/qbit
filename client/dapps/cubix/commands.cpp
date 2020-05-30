#include "commands.h"

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/gil.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>

#include <iostream>
#include <iterator>

using namespace qbit;
using namespace qbit::cubix;

//
// UploadMediaCommand
//
void UploadMediaCommand::process(const std::vector<std::string>& args) {
	//
	size_ = 0;
	file_ = "";
	pos_ = 0;
	description_ = "";
	previewData_.resize(0);
	summarySent_ = false;
	feeSent_ = false;
	headerSent_ = false;
	peer_ = nullptr;

	if (args.size() >= 1) {
		//
		file_ = args[0];
		if (args.size() > 1)
			description_ = args[1];

		// try file
		boost::filesystem::path lPath(file_);
		if (!boost::filesystem::exists(lPath)) {
			gLog().writeClient(Log::CLIENT, std::string(": file does not exists"));	
			return;
		}

		// try size
		size_ = boost::filesystem::file_size(lPath);

		// prepare
		IComposerMethodPtr lCreateSummary = CubixLightComposer::CreateTxMediaSummary::instance(composer_, 
			size_, boost::bind(&UploadMediaCommand::summaryCreated, shared_from_this(), _1, _2));
		// async process
		lCreateSummary->process(boost::bind(&UploadMediaCommand::error, shared_from_this(), _1, _2));
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}

void UploadMediaCommand::summaryCreated(TransactionContextPtr ctx, Transaction::UnlinkedOutPtr utxo) {
	//
	prev_ = utxo;
	summary_ = TransactionHelper::to<TxMediaSummary>(ctx->tx());
	ctx_ = ctx;

	TransactionContextPtr lFee = ctx->locateByType(Transaction::FEE);
	// send tx and define peer to interact with
	if (!(peer_ = composer_->requestProcessor()->sendTransaction(ctx->tx()->chain(), lFee,
			SentTransaction::instance(
				boost::bind(&UploadMediaCommand::feeSent, shared_from_this(), _1, _2),
				boost::bind(&UploadMediaCommand::timeout, shared_from_this()))))) {
		gLog().writeClient(Log::CLIENT, std::string(": tx was not broadcasted, wallet re-init..."));
		composer_->wallet()->resetCache();
		composer_->wallet()->prepareCache();
		done_(nullptr);
	}
}

void UploadMediaCommand::feeSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
	//
	if (!checkSent("        fee: ", tx, errors)) { done_(nullptr); return; }

	feeSent_ = true;

	composer_->requestProcessor()->sendTransaction(peer_, ctx_,
			SentTransaction::instance(
				boost::bind(&UploadMediaCommand::summarySent, shared_from_this(), _1, _2),
				boost::bind(&UploadMediaCommand::timeout, shared_from_this())));

	if (summarySent_ && feeSent_) {
		startSendData();
	}
}

void UploadMediaCommand::summarySent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
	//
	if (!checkSent("    summary: ", tx, errors)) { done_(nullptr); return; }

	summarySent_ = true;

	if (summarySent_ && feeSent_) {
		startSendData();
	}
}

void UploadMediaCommand::startSendData() {
	//
	boost::filesystem::path lFile(file_);
	std::string lFileType = lFile.extension().native();

	std::string lType = boost::algorithm::to_lower_copy(lFileType);

	if (lType == ".jpeg" || lType == ".jpg" || lType == "jpeg" || lType == "jpg") {
		//
		mediaType_ = TxMediaHeader::IMAGE_JPEG;
	} else if (lType == ".png" || lType == "png") {
		//
		mediaType_ = TxMediaHeader::IMAGE_PNG;
	} else {
		gLog().writeClient(Log::CLIENT, std::string(": media type is not supported"));	
		return;		
	}

	if (size_ > CUBIX_MAX_DATA_CHUNK) {
		//
		int lWidth = 0;
		int lHeight = 0;
		if (mediaType_ == TxMediaHeader::IMAGE_JPEG) {
			//
			using backend_t = boost::gil::get_reader_backend<std::string, boost::gil::jpeg_tag>::type;
			backend_t lBackend = boost::gil::read_image_info(file_, boost::gil::jpeg_tag());

			lWidth = lBackend._info._width;
			lHeight = lBackend._info._height;
		} else if (mediaType_ == TxMediaHeader::IMAGE_PNG) {
			//
			using backend_t = boost::gil::get_reader_backend<std::string, boost::gil::png_tag>::type;
			backend_t lBackend = boost::gil::read_image_info(file_, boost::gil::png_tag());

			lWidth = lBackend._info._width;
			lHeight = lBackend._info._height;
		}

		// make preview
		boost::gil::rgb8_image_t lImage(lWidth, lHeight);

		// generate preview
		double lCoefficient = (double) lWidth / (double) CUBIX_PREVIEW_WIDTH;
		lHeight = ((double) lHeight) / lCoefficient;

		boost::gil::rgb8_image_t lPreviewImage(CUBIX_PREVIEW_WIDTH, lHeight);
		if (mediaType_ == TxMediaHeader::IMAGE_JPEG) {
			boost::gil::read_and_convert_view(file_, boost::gil::view(lImage), boost::gil::jpeg_tag());
			boost::gil::resize_view(boost::gil::const_view(lImage), boost::gil::view(lPreviewImage), boost::gil::bilinear_sampler());
		} else if (mediaType_ == TxMediaHeader::IMAGE_PNG) {
			boost::gil::read_and_convert_view(file_, boost::gil::view(lImage), boost::gil::png_tag());
			boost::gil::resize_view(boost::gil::const_view(lImage), boost::gil::view(lPreviewImage), boost::gil::bilinear_sampler());
		}

		// write image
		std::stringstream lStream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);

		if (mediaType_ == TxMediaHeader::Type::IMAGE_JPEG)
			boost::gil::write_view(lStream, boost::gil::view(lPreviewImage), boost::gil::image_write_info<boost::gil::jpeg_tag>(95));
		else if (mediaType_ == TxMediaHeader::IMAGE_PNG)
			boost::gil::write_view(lStream, boost::gil::view(lPreviewImage), boost::gil::png_tag());

		previewData_.insert(previewData_.end(), 
				std::istreambuf_iterator<char>(lStream), 
				std::istreambuf_iterator<char>());
	} else {
		// just read data into preview
		std::ifstream lStream(file_, std::ios::binary);
		previewData_.insert(previewData_.end(), 
				std::istreambuf_iterator<char>(lStream), 
				std::istreambuf_iterator<char>());
	}

	//
	if (size_ <= CUBIX_MAX_DATA_CHUNK) {
		// make media header and put data
		SKeyPtr lSKey = composer_->wallet()->firstKey();
		uint256 lNamePart = Random::generate(*lSKey);
		IComposerMethodPtr lCreateHeader = CubixLightComposer::CreateTxMediaHeader::instance(composer_, 
			size_, previewData_, lNamePart.toHex(), description_, mediaType_, summary_->chain(), prev_,
			boost::bind(&UploadMediaCommand::headerCreated, shared_from_this(), _1));
		// async process
		lCreateHeader->process(boost::bind(&UploadMediaCommand::error, shared_from_this(), _1, _2));
	} else {
		// make media data & continue
		mediaFile_ = std::ifstream(file_, std::ios::binary);
		continueSendData();
	}
}

bool UploadMediaCommand::readNextChunk(int64_t& pos, std::vector<unsigned char>& data) {
	//
	int lSize = (pos + CUBIX_MAX_DATA_CHUNK > size_ ? size_ - pos : CUBIX_MAX_DATA_CHUNK);
	data.resize(lSize);

	pos += lSize;
	mediaFile_.seekg(-pos, std::ios::end);
	mediaFile_.read((char*)&data[0], lSize);

	if (pos == size_) return true;
	return false;
}

void UploadMediaCommand::continueSendData() {
	//
	// at the top
	if (pos_ == size_) {
		// make media header and put data
		SKeyPtr lSKey = composer_->wallet()->firstKey();
		uint256 lNamePart = Random::generate(*lSKey);
		IComposerMethodPtr lCreateHeader = CubixLightComposer::CreateTxMediaHeader::instance(composer_, 
			size_, previewData_, lNamePart.toHex(), description_, mediaType_, summary_->chain(), prev_,
			boost::bind(&UploadMediaCommand::headerCreated, shared_from_this(), _1));
		// async process
		lCreateHeader->process(boost::bind(&UploadMediaCommand::error, shared_from_this(), _1, _2));
	} else {
		//
		std::vector<unsigned char> lData;
		readNextChunk(pos_, lData);
		//
		IComposerMethodPtr lCreateData = CubixLightComposer::CreateTxMediaData::instance(composer_, 
			lData, summary_->chain(), prev_,
			boost::bind(&UploadMediaCommand::dataCreated, shared_from_this(), _1, _2));
		// async process
		lCreateData->process(boost::bind(&UploadMediaCommand::error, shared_from_this(), _1, _2));		
	}
}

void UploadMediaCommand::dataCreated(TransactionContextPtr ctx, Transaction::UnlinkedOutPtr utxo) {
	//
	prev_ = utxo;

	composer_->requestProcessor()->sendTransaction(peer_, ctx,
			SentTransaction::instance(
				boost::bind(&UploadMediaCommand::dataSent, shared_from_this(), _1, _2),
				boost::bind(&UploadMediaCommand::timeout, shared_from_this())));
}

void UploadMediaCommand::dataSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
	//
	if (!checkSent("       data: ", tx, errors)) { done_(nullptr); return; }

	// continue send
	continueSendData();
}

void UploadMediaCommand::headerCreated(TransactionContextPtr ctx) {
	//
	header_ = TransactionHelper::to<TxMediaHeader>(ctx->tx());

	composer_->requestProcessor()->sendTransaction(peer_, ctx,
			SentTransaction::instance(
				boost::bind(&UploadMediaCommand::headerSent, shared_from_this(), _1, _2),
				boost::bind(&UploadMediaCommand::timeout, shared_from_this())));
}

void UploadMediaCommand::headerSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
	//
	if (!checkSent("     header: ", tx, errors)) { done_(nullptr); return; }

	headerSent_ = true;

	if (summarySent_ && feeSent_ && headerSent_) {
		std::cout << "      chain: " << header_->chain().toHex() << std::endl;
		std::cout << "       file: " << header_->mediaName() << std::endl;
		done_(header_);
	}
}

//
// DownloadMediaCommand
//
void DownloadMediaCommand::process(const std::vector<std::string>& args) {
	//
	if (args.size() >= 2) {
		//
		std::vector<std::string> lParts;
		boost::split(lParts, args[0], boost::is_any_of("/"));

		if (lParts.size() < 2) {
			gLog().writeClient(Log::CLIENT, std::string(": <file> is not supported yet"));
			return;	
		}

		headerTx_.setHex(lParts[0]);
		chain_.setHex(lParts[1]);

		localFile_ = args[1];

		if (args.size() > 2) {
			if (args[2] == "preview") previewOnly_ = true;			
		}

		// try file
		boost::filesystem::path lPath(localFile_);
		if (boost::filesystem::exists(lPath)) {
			gLog().writeClient(Log::CLIENT, std::string(": file already exists"));	
			return;
		}

		if (!composer_->requestProcessor()->loadTransaction(chain_, headerTx_, 
				LoadTransaction::instance(
					boost::bind(&DownloadMediaCommand::headerLoaded, shared_from_this(), _1),
					boost::bind(&DownloadMediaCommand::timeout, shared_from_this()))
			))	error("E_MEDIA_NOT_LOADED", "Header was not loaded.");

	} else {
		gLog().writeClient(Log::CLIENT, std::string(": incorrect number of arguments"));
	}
}

void DownloadMediaCommand::headerLoaded(TransactionPtr tx) {
	//
	if (tx) {
		// check tx
		if (headerTx_ != tx->id()) {
			error("E_TX_INCORRECT", "Transaction is incorrect.");
			return;
		}

		//
		header_ = TransactionHelper::to<TxMediaHeader>(tx);
		if (!header_->verifySignature()) {
			error("E_MEDIA_SIGNATURE_IS_INCORREST", "Header signature is incorrect.");
			return;
		}

		std::cout << "            header: " << header_->id().toHex() << std::endl;		
		std::cout << "              file: " << header_->mediaName() << std::endl;

		bool lOriginal = (header_->size() <= CUBIX_MAX_DATA_CHUNK);
		std::string lLocalPreviewFile = localFile_ + (lOriginal ? "" : "-preview") + header_->mediaTypeToExtension();

		// try file
		boost::filesystem::path lPath(localFile_);
		if (boost::filesystem::exists(lPath)) {
			boost::filesystem::remove(lPath);
		}

		std::ofstream lPreviewFile = std::ofstream(lLocalPreviewFile, std::ios::binary);
		lPreviewFile.write((char*)&header_->data()[0], header_->data().size());
		lPreviewFile.close();

		if (lOriginal) std::cout << "     original file: " << lLocalPreviewFile << std::endl;	
		else std::cout << "local preview file: " << lLocalPreviewFile << std::endl;	

		//
		if (previewOnly_) {
			done_(header_);
		} else {
			// load data
			if (header_->size() > CUBIX_MAX_DATA_CHUNK) {
				// try file
				boost::filesystem::path lLocalPath(localFile_ + header_->mediaTypeToExtension());
				if (boost::filesystem::exists(lLocalPath)) {
					boost::filesystem::remove(lLocalPath);
				}				
				//
				outLocalFile_ = std::ofstream(localFile_ + header_->mediaTypeToExtension(), std::ios::binary);
				//
				nextDataTx_ = header_->in()[0].out().tx();
				if (!composer_->requestProcessor()->loadTransaction(chain_, nextDataTx_, 
						LoadTransaction::instance(
							boost::bind(&DownloadMediaCommand::dataLoaded, shared_from_this(), _1),
							boost::bind(&DownloadMediaCommand::timeout, shared_from_this()))
					))	error("E_MEDIA_DATA_NOT_LOADED", "Media data was not loaded.");
			} else {
				done_(nullptr);
			}
		}
	} else {
		error("E_MEDIA_HEADER_NOT_FOUND", "Media header was not found.");
	}
}

void DownloadMediaCommand::dataLoaded(TransactionPtr tx) {
	//
	if (tx) {
		// check tx
		if (nextDataTx_ != tx->id()) {
			error("E_TX_INCORRECT", "Transaction is incorrect.");
			return;
		}

		//
		if (tx->type() == TX_CUBIX_MEDIA_SUMMARY) {
			// done
			std::cout << "           summary: " << tx->id().toHex() << std::endl;		
			outLocalFile_.close();
			done_(header_);
		} else {
			//
			TxMediaDataPtr lData = TransactionHelper::to<TxMediaData>(tx);
			if (!lData->verifySignature()) {
				error("E_MEDIA_SIGNATURE_IS_INCORREST", "Data signature is incorrect.");
				return;
			}

			//
			outLocalFile_.write((char*)&lData->data()[0], lData->data().size());
			std::cout << "              data: " << lData->id().toHex() << std::endl;		


			//
			nextDataTx_ = lData->in()[0].out().tx();
			if (!composer_->requestProcessor()->loadTransaction(chain_, nextDataTx_, 
					LoadTransaction::instance(
						boost::bind(&DownloadMediaCommand::dataLoaded, shared_from_this(), _1),
						boost::bind(&DownloadMediaCommand::timeout, shared_from_this()))
				))	error("E_MEDIA_DATA_NOT_LOADED", "Media data was not loaded.");
		}
	} else {
		error("E_MEDIA_DATA_NOT_FOUND", "Media data was not found.");
	}
}
