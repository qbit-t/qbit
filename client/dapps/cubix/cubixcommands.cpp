#include "cubixcommands.h"

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "exif.h"

#include "../../crypto/aes.h"
#include "../../crypto/sha256.h"

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
	previewWidth_ = 0;
	previewHeight_ = 0;

	args_ = args;

	if (preview_.size()) {
		args_.push_back("-s");
		args_.push_back(preview_);
	}

	if (args_.size() >= 1) {
		//
		file_ = args_[0];
		if (args_.size() > 1) {
			std::vector<std::string>::const_iterator lArg = ++(args_.begin());
			while (lArg != args_.end()) {
				if (*lArg == "-s") {
					//
					lArg++;
					//
					std::vector<std::string> lSize; 
					boost::split(lSize, std::string(*lArg), boost::is_any_of("x"));

					if (lSize.size() == 2) {
						if (!boost::conversion::try_lexical_convert<int>(std::string(lSize[0]), previewWidth_)) {
							error("E_WIDTH_INVALID", "Preview width is invalid.");
							return;
						}

						if (!boost::conversion::try_lexical_convert<int>(std::string(lSize[1]), previewHeight_)) {
							error("E_HEIGHT_INVALID", "Preview height is invalid.");
							return;
						}
					} else {
						error("E_SIZE_INVALID", "Preview size is invalid.");
						return;
					}

					//
				} else if (*lArg == "-p") {
					pkey_.fromString(*(++lArg));
				} else if (*lArg == "-d") {
					description_ = 	*(++lArg);
				}

				lArg++;
			}
		}

		// try file
		boost::filesystem::path lPath(file_);
		if (!boost::filesystem::exists(lPath)) {
			error("E_FILE_NOT_EXISTS", "File does not exists.");
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
		error("E_INCORRECT_AGRS", "Incorrect number of arguments.");
		return;
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
		composer_->wallet()->resetCache();
		composer_->wallet()->prepareCache();
		error("E_TX_NOT_SENT", "Transaction was not sent");
	}
}

void UploadMediaCommand::feeSent(const uint256& tx, const std::vector<TransactionContext::Error>& errors) {
	//
	if (!checkSent("        fee: ", tx, errors)) { return; }

	feeSent_ = true;

	//
	TransactionContextPtr lFee = ctx_->locateByType(Transaction::FEE);
	for (std::list<Transaction::NetworkUnlinkedOutPtr>::iterator lOut = lFee->externalOuts().begin(); 
																lOut != lFee->externalOuts().end(); lOut++) {
		//
		if (gLog().isEnabled(Log::CLIENT))
			gLog().write(Log::CLIENT, strprintf("[feeSent]: fee for %s", tx.toHex()));
		composer_->wallet()->updateOut(*lOut, ctx_->tx()->id(), ctx_->tx()->type());
	}

	//
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
	if (!checkSent("    summary: ", tx, errors)) { return; }

	summarySent_ = true;

	if (summarySent_ && feeSent_) {
		startSendData();
	}
}

void UploadMediaCommand::encrypt(const uint256& nonce, const std::vector<unsigned char>& data, std::vector<unsigned char>& cypher) {
	//
	// prepare secret
	unsigned char lMix[AES_BLOCKSIZE] = {0};
	memcpy(lMix, nonce.begin() + 8 /*shift*/, AES_BLOCKSIZE);

	// make cypher
	cypher.resize(data.size() + AES_BLOCKSIZE /*padding*/, 0);
	AES256CBCEncrypt lEncrypt(nonce.begin(), lMix, true);
	unsigned lLen = lEncrypt.Encrypt(&data[0], data.size(), &cypher[0]);
	cypher.resize(lLen);
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
		error("E_INCORRECT_MEDIA_TYPE", "Media type is not supported");
		return;
	}

	//
	if (progress_) progress_(file_, 0, size_);	

	try {
		if (size_ > CUBIX_MAX_DATA_CHUNK || (previewWidth_ && previewHeight_)) {
			//
			int lWidth = 0;
			int lHeight = 0;
			//
			if (mediaType_ == TxMediaHeader::IMAGE_JPEG) {
				//
				using backend_t = boost::gil::get_reader_backend<std::string, boost::gil::jpeg_tag>::type;
				backend_t lBackend = boost::gil::read_image_info(file_, boost::gil::jpeg_tag());

				lWidth = lBackend._info._width;
				lHeight = lBackend._info._height;

				//
				if (gLog().isEnabled(Log::CLIENT))
					gLog().write(Log::CLIENT, strprintf("[startSendData]: reading exif for %s", file_));

				std::vector<unsigned char> lExifData;
				std::ifstream lStream(file_, std::ios::binary);
				std::istreambuf_iterator<char> lBegin(lStream);

				lExifData.insert(lExifData.end(), lBegin, std::istreambuf_iterator<char>());

				//
				int lCode;
				easyexif::EXIFInfo lExif;
				if (!(lCode = lExif.parseFrom(&lExifData[0], lExifData.size()))) {
					orientation_ = lExif.Orientation;
					if (gLog().isEnabled(Log::CLIENT))
						gLog().write(Log::CLIENT, strprintf("[startSendData]: orientation = %d for %s", orientation_, file_));
				} else {
					if (gLog().isEnabled(Log::CLIENT))
						gLog().write(Log::CLIENT, strprintf("[startSendData/error]: code = %d for %s", lCode, file_));
				}

			} else if (mediaType_ == TxMediaHeader::IMAGE_PNG) {
				//
				using backend_t = boost::gil::get_reader_backend<std::string, boost::gil::png_tag>::type;
				backend_t lBackend = boost::gil::read_image_info(file_, boost::gil::png_tag());

				lWidth = lBackend._info._width;
				lHeight = lBackend._info._height;
			}

			// make preview
			boost::gil::rgb8_image_t lImage(lWidth, lHeight);

			//
			int lNewWidth = 0;
			int lNewHeight = 0;
			int lTargetWidth = lWidth;
			int lTargetHeight = lHeight;

			// generate preview
			if (previewWidth_ && previewHeight_) {
				double lCoefficient = (double) lTargetWidth / (double) previewWidth_;
				lNewWidth = previewWidth_;
				lNewHeight = ((double) lTargetHeight) / lCoefficient;
			} else {
				//
				double lCoefficient = (double) lTargetWidth / (double) CUBIX_PREVIEW_WIDTH;
				lNewWidth = CUBIX_PREVIEW_WIDTH;
				lNewHeight = ((double) lTargetHeight) / lCoefficient;
			}

			boost::gil::rgb8_image_t lPreviewImage(lNewWidth, lNewHeight);
			if (mediaType_ == TxMediaHeader::IMAGE_JPEG) {
				boost::gil::read_and_convert_view(file_, boost::gil::view(lImage), boost::gil::jpeg_tag());

				double lAngle = 0.0;
				if (orientation_ == 6) lAngle = 90.0 * 3.1415/180.0;
				else if (orientation_ == 3) lAngle = -180.0 * 3.1415/180.0;
				else if (orientation_ == 8) lAngle = -90.0 * 3.1415/180.0;

				if (orientation_ == 6 || orientation_ == 8) {
					// exchange width and height
					lTargetWidth = lHeight;
					lTargetHeight = lWidth;
				}

				if (lAngle != 0.0) {
					//
					double lCoefficient = 0.0;
					// recalculate
					if (previewWidth_ && previewHeight_) {
						lCoefficient = (double) lTargetWidth / (double) previewWidth_;
						lNewWidth = previewWidth_;
						lNewHeight = ((double) lTargetHeight) / lCoefficient;
					} else {
						//
						lCoefficient = (double) lTargetWidth / (double) CUBIX_PREVIEW_WIDTH;
						lNewWidth = CUBIX_PREVIEW_WIDTH;
						lNewHeight = ((double) lTargetHeight) / lCoefficient;
					}

					if (orientation_ == 6 || orientation_ == 8) {
						// rotation needed 90:-90, exchange width and height
						// 1. scale
						boost::gil::rgb8_image_t lOriginalScaledImage(lNewHeight, lNewWidth);
						boost::gil::resample_subimage(
							boost::gil::const_view(lImage),
							boost::gil::view(lOriginalScaledImage),
							0, 0, lWidth, lHeight,
							0.0,
							boost::gil::bilinear_sampler()
						);

						// 2. rotate
						lPreviewImage = boost::gil::rgb8_image_t(lNewWidth, lNewHeight);
						cubix::resample_subimage(
							boost::gil::const_view(lOriginalScaledImage),
							boost::gil::view(lPreviewImage),
							0, 0, lNewHeight, lNewWidth,
							lAngle,
							boost::gil::bilinear_sampler()
						);
					} else {
						// rotation needed 180:-180, width and height should remains
						// 1. scale
						boost::gil::rgb8_image_t lOriginalScaledImage(lNewWidth, lNewHeight);
						boost::gil::resample_subimage(
							boost::gil::const_view(lImage),
							boost::gil::view(lOriginalScaledImage),
							0, 0, lWidth, lHeight,
							0.0,
							boost::gil::bilinear_sampler()
						);

						// 2. rotate
						lPreviewImage = boost::gil::rgb8_image_t(lNewWidth, lNewHeight);
						cubix::resample_subimage(
							boost::gil::const_view(lOriginalScaledImage),
							boost::gil::view(lPreviewImage),
							0, 0, lNewWidth, lNewHeight,
							lAngle,
							boost::gil::bilinear_sampler()
						);
					}
				} else {
					boost::gil::resize_view(
						boost::gil::const_view(lImage),
						boost::gil::view(lPreviewImage),
						boost::gil::bilinear_sampler()
					);
				}

			} else if (mediaType_ == TxMediaHeader::IMAGE_PNG) {
				//boost::gil::rgb8_image_t lPreviewImage(lNewWidth, lNewHeight);
				boost::gil::read_and_convert_view(file_, boost::gil::view(lImage), boost::gil::png_tag());
				boost::gil::resize_view(boost::gil::const_view(lImage), boost::gil::view(lPreviewImage), boost::gil::bilinear_sampler());
			}

			// write image
			std::stringstream lStream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
			if (mediaType_ == TxMediaHeader::Type::IMAGE_JPEG)
				boost::gil::write_view(lStream, boost::gil::view(lPreviewImage), boost::gil::image_write_info<boost::gil::jpeg_tag>(95));
			else if (mediaType_ == TxMediaHeader::IMAGE_PNG)
				boost::gil::write_view(lStream, boost::gil::view(lPreviewImage), boost::gil::png_tag());

			if (pkey_.valid()) {
				// data
				std::vector<unsigned char> lPreviewData;
				lPreviewData.insert(lPreviewData.end(),
						std::istreambuf_iterator<char>(lStream),
						std::istreambuf_iterator<char>());

				// make cypher
				SKeyPtr lSKey = composer_->wallet()->firstKey();
				uint256 lNonce = lSKey->shared(pkey_);
				encrypt(lNonce, lPreviewData, previewData_);
			} else {
				//
				previewData_.insert(previewData_.end(),
						std::istreambuf_iterator<char>(lStream),
						std::istreambuf_iterator<char>());
			}
		} else {
			// just read data into preview
			std::ifstream lStream(file_, std::ios::binary);

			if (pkey_.valid()) {
				// data
				std::vector<unsigned char> lPreviewData;
				lPreviewData.insert(lPreviewData.end(),
						std::istreambuf_iterator<char>(lStream),
						std::istreambuf_iterator<char>());

				// make cypher
				SKeyPtr lSKey = composer_->wallet()->firstKey();
				uint256 lNonce = lSKey->shared(pkey_);
				encrypt(lNonce, lPreviewData, previewData_);
			} else {
				previewData_.insert(previewData_.end(),
						std::istreambuf_iterator<char>(lStream),
						std::istreambuf_iterator<char>());
			}
		}
	} catch (const std::ios_base::failure& err) {
		error("E_IMAGE_IO_ERROR", err.what());
		return;
	}

	//
	if (progress_) progress_(file_, 0, size_);

	//
	if (size_ <= CUBIX_MAX_DATA_CHUNK && !previewWidth_ && !previewHeight_) {
		// make media header and put data
		SKeyPtr lSKey = composer_->wallet()->firstKey();
		uint256 lNamePart = Random::generate(*lSKey);
		IComposerMethodPtr lCreateHeader = CubixLightComposer::CreateTxMediaHeader::instance(composer_, 
			size_, previewData_, orientation_, lNamePart.toHex(), description_, mediaType_, summary_->chain(), prev_,
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
			size_, previewData_, orientation_, lNamePart.toHex(), description_, mediaType_, summary_->chain(), prev_,
			boost::bind(&UploadMediaCommand::headerCreated, shared_from_this(), _1));
		// async process
		lCreateHeader->process(boost::bind(&UploadMediaCommand::error, shared_from_this(), _1, _2));
	} else {
		//
		std::vector<unsigned char> lData;
		if (pkey_.valid()) {
			// read
			std::vector<unsigned char> lTemp;
			readNextChunk(pos_, lTemp);
			// make cypher
			SKeyPtr lSKey = composer_->wallet()->firstKey();
			uint256 lNonce = lSKey->shared(pkey_);
			encrypt(lNonce, lTemp, lData);
		} else {
			readNextChunk(pos_, lData);
		}

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
	if (!checkSent("       data: ", tx, errors)) { return; }

	// inform about progress
	if (progress_) progress_(file_, pos_, size_);

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
	if (!checkSent("     header: ", tx, errors)) { return; }

	headerSent_ = true;

	if (progress_) progress_(file_, pos_, size_);

	if (summarySent_ && feeSent_ && headerSent_) {
		std::cout << "      chain: " << header_->chain().toHex() << std::endl;
		std::cout << "       file: " << header_->mediaName() << std::endl;
		done_(header_, ProcessingError());
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
			error("E_DOWNLOAD", "<file> is not supported");
			return;	
		}

		headerTx_.setHex(lParts[0]);
		chain_.setHex(lParts[1]);
		previewOnly_ = false;
		skipIfExists_ = false;
		localPreviewFileName_ = "";
		localFileName_ = "";

		localFile_ = args[1];

		if (args.size() > 2) {
			std::vector<std::string>::const_iterator lArg = ++(++(args.begin()));
			while (lArg != args.end()) {
				if (*lArg == "-p") {
					pkey_.fromString(*(++lArg));
				} else if (*lArg == "-preview") {
					previewOnly_ = true;
				} else if (*lArg == "-skip") {
					skipIfExists_ = true;
				}

				lArg++;
			}
		}

		/*
		if (gLog().isEnabled(Log::CLIENT))
			gLog().write(Log::CLIENT, strprintf("[downloadMediaCommand/%X]: header = %s/%s, file = %s", 
						this, headerTx_.toHex(), chain_.toHex(), localFile_));
		*/

		// try file
		if (skipIfExists_) {
			//
			std::string lPreviewFile = localFile_ + "-preview";
			std::list<std::string> lExtensions;
			cubix::TxMediaHeader::mediaExtensions(lExtensions);

			bool lPreviewExists = false;
			unsigned short lOrientation = 0;
			for (std::list<std::string>::iterator lExtension = lExtensions.begin(); lExtension != lExtensions.end(); lExtension++) {
				localPreviewFileName_ = lPreviewFile + *lExtension;
				boost::filesystem::path lPath(localPreviewFileName_);
				if (boost::filesystem::exists(lPath)) {
					lPreviewExists = true;

					std::ifstream lPreviewFileMeta = std::ifstream(localPreviewFileName_ + ".meta", std::ios::binary);
					lPreviewFileMeta.read((char*)&lOrientation, 2);
					lPreviewFileMeta.close();

					break;
				}
			}

			bool lOriginalExists = false;
			if (lPreviewExists/* && !lOriginalExists*/) {
				for (std::list<std::string>::iterator lExtension = lExtensions.begin(); lExtension != lExtensions.end(); lExtension++) {
					localFileName_ = localFile_ + *lExtension;
					boost::filesystem::path lPath(localFileName_);
					if (boost::filesystem::exists(lPath)) {
						lOriginalExists = true;
						break;
					}
				}
			}

			if (!lOriginalExists) { lOriginalExists = previewOnly_; localFileName_ = ""; }
			if (lPreviewExists && lOriginalExists) {
				/*
				if (gLog().isEnabled(Log::CLIENT))
					gLog().write(Log::CLIENT, strprintf("[downloadMediaCommand/EXISTS/%X]: header = %s/%s, file = %s", 
								this, headerTx_.toHex(), chain_.toHex(), localPreviewFileName_));
				*/
				if (done_) done_(nullptr, localPreviewFileName_, localFileName_, lOrientation, ProcessingError());
				return;
			}
		}

		if (!composer_->requestProcessor()->loadTransaction(chain_, headerTx_, true /*try mempool*/,
				LoadTransaction::instance(
					boost::bind(&DownloadMediaCommand::headerLoaded, shared_from_this(), _1),
					boost::bind(&DownloadMediaCommand::timeout, shared_from_this()))
			))	error("E_MEDIA_NOT_LOADED", "Header was not loaded.");

	} else {
		error("E_INCORRECT_AGRS", "Incorrect number of arguments.");
	}
}

void DownloadMediaCommand::decrypt(const uint256& nonce, const std::vector<unsigned char>& cypher, std::vector<unsigned char>& data) {
	//
	unsigned char lMix[AES_BLOCKSIZE] = {0};
	memcpy(lMix, nonce.begin() + 8, AES_BLOCKSIZE);

	// decrypt
	data.resize(cypher.size() + 1, 0);
	AES256CBCDecrypt lDecrypt(nonce.begin(), lMix, true);
	unsigned lLen = lDecrypt.Decrypt(&cypher[0], cypher.size(), &data[0]);
	data.resize(lLen);
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
		localFileName_ = localFile_ + header_->mediaTypeToExtension();
		localPreviewFileName_ = localFile_ + "-preview" + header_->mediaTypeToExtension();

		/*
		if (gLog().isEnabled(Log::CLIENT))
			gLog().write(Log::CLIENT, strprintf("[downloadMediaCommand/LOADED/%X]: header = %s|%s, file = %s", 
						this, headerTx_.toHex(), header_->id().toHex(), localPreviewFileName_));
		*/
		// try file
		boost::filesystem::path lPath(localFileName_);
		if (boost::filesystem::exists(lPath)) {
			boost::filesystem::remove(lPath);
		}

		boost::filesystem::path lPathPreview(localPreviewFileName_);
		if (boost::filesystem::exists(lPathPreview)) {
			boost::filesystem::remove(lPathPreview);
		}

		size_t lFirstSize = 0;
		std::ofstream lPreviewFile = std::ofstream(localPreviewFileName_, std::ios::binary);
		if (pkey_.valid()) {
			std::vector<unsigned char> lData;
			SKeyPtr lSKey = composer_->wallet()->firstKey();
			uint256 lNonce = lSKey->shared(pkey_);
			decrypt(lNonce, header_->data(), lData);
			//
			lFirstSize = lData.size();
			lPreviewFile.write((char*)&lData[0], lData.size());
		} else {
			//
			lFirstSize = header_->data().size();
			lPreviewFile.write((char*)&header_->data()[0], header_->data().size());
		}

		lPreviewFile.close();

		std::ofstream lPreviewFileMeta = std::ofstream(localPreviewFileName_ + ".meta", std::ios::binary);
		unsigned short lOrientation = header_->orientation();
		lPreviewFileMeta.write((char*)&lOrientation, 2);
		lPreviewFileMeta.close();

		if (lOriginal) std::cout << "     original file: " << localFileName_ << std::endl;	
		else std::cout << "local preview file: " << localPreviewFileName_ << std::endl;	

		//
		if (previewOnly_ && header_->size() > CUBIX_MAX_DATA_CHUNK) {
			if (done_) done_(header_, localPreviewFileName_, std::string(), header_->orientation(), ProcessingError());
		} else {
			// load data
			if (header_->size() > CUBIX_MAX_DATA_CHUNK || lFirstSize != header_->size()) {
				// try file
				boost::filesystem::path lLocalPath(localFile_ + header_->mediaTypeToExtension());
				if (boost::filesystem::exists(lLocalPath)) {
					boost::filesystem::remove(lLocalPath);
				}
				//
				outLocalFile_ = std::ofstream(localFile_ + header_->mediaTypeToExtension(), std::ios::binary);

				//
				if (progress_) progress_(pos_, header_->size());

				//
				nextDataTx_ = header_->in()[0].out().tx();
				if (!composer_->requestProcessor()->loadTransaction(chain_, nextDataTx_, true /*try mempool*/,
						LoadTransaction::instance(
							boost::bind(&DownloadMediaCommand::dataLoaded, shared_from_this(), _1),
							boost::bind(&DownloadMediaCommand::timeout, shared_from_this()))
					))	error("E_MEDIA_DATA_NOT_LOADED", "Media data was not loaded.");
			} else {
				if (done_) done_(header_, localPreviewFileName_, localPreviewFileName_, header_->orientation(), ProcessingError());
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
			if (done_) done_(header_, localPreviewFileName_, localFileName_, header_->orientation(), ProcessingError());
		} else {
			//
			TxMediaDataPtr lData = TransactionHelper::to<TxMediaData>(tx);
			if (!lData->verifySignature()) {
				error("E_MEDIA_SIGNATURE_IS_INCORREST", "Data signature is incorrect.");
				return;
			}

			//
			pos_ += lData->data().size();
			if (progress_) progress_(pos_, header_->size());

			//
			if (pkey_.valid()) {
				std::vector<unsigned char> lTemp;
				SKeyPtr lSKey = composer_->wallet()->firstKey();
				uint256 lNonce = lSKey->shared(pkey_);
				decrypt(lNonce, lData->data(), lTemp);
				//
				outLocalFile_.write((char*)&lTemp[0], lTemp.size());

			} else {
				//
				outLocalFile_.write((char*)&lData->data()[0], lData->data().size());
			}

			//
			std::cout << "              data: " << lData->id().toHex() << std::endl;

			//
			nextDataTx_ = lData->in()[0].out().tx();
			if (!composer_->requestProcessor()->loadTransaction(chain_, nextDataTx_, true /*try mempool*/, 
					LoadTransaction::instance(
						boost::bind(&DownloadMediaCommand::dataLoaded, shared_from_this(), _1),
						boost::bind(&DownloadMediaCommand::timeout, shared_from_this()))
				))	error("E_MEDIA_DATA_NOT_LOADED", "Media data was not loaded.");
		}
	} else {
		error("E_MEDIA_DATA_NOT_FOUND", "Media data was not found.");
	}
}
