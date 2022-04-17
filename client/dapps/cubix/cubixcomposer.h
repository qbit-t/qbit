// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CUBIX_LIGHT_COMPOSER_H
#define QBIT_CUBIX_LIGHT_COMPOSER_H

#include "../../handlers.h"
#include "../../../dapps/cubix/txmediaheader.h"
#include "../../../dapps/cubix/txmediadata.h"
#include "../../../dapps/cubix/txmediasummary.h"

#include "../../../txshard.h"
#include "../../../db/containers.h"
#include "../../../random.h"
#include "../../../requestprocessor.h"

namespace qbit {
namespace cubix {

// forward
class CubixLightComposer;
typedef std::shared_ptr<CubixLightComposer> CubixLightComposerPtr;

//
class CubixLightComposer: public std::enable_shared_from_this<CubixLightComposer> {
public:
	class CreateTxMediaSummary: public IComposerMethod, public std::enable_shared_from_this<CreateTxMediaSummary> {
	public:
		CreateTxMediaSummary(CubixLightComposerPtr composer, uint64_t size, transactionCreatedWithOutFunction created): 
			composer_(composer), size_(size), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(CubixLightComposerPtr composer, uint64_t size, transactionCreatedWithOutFunction created) {
			return std::make_shared<CreateTxMediaSummary>(composer, size, created); 
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during creating media summary.");
		}

		//
		void entitiesCountByDAppLoaded(const std::map<uint256, uint32_t>&, const std::string&);
		void mergeInfo(const std::map<uint256, uint32_t>&);

	private:
		CubixLightComposerPtr composer_;
		uint64_t size_;
		transactionCreatedWithOutFunction created_;
		errorFunction error_;

		TxMediaSummaryPtr tx_;
		TransactionContextPtr ctx_;
		int destinations_ = 0;
		int collected_ = 0;
		std::map<uint32_t, uint256> dAppInfo_;
	};

	class CreateTxMediaData: public IComposerMethod, public std::enable_shared_from_this<CreateTxMediaData> {
	public:
		CreateTxMediaData(CubixLightComposerPtr composer, const std::vector<unsigned char>& data, const uint256& chain, Transaction::UnlinkedOutPtr prev, transactionCreatedWithOutFunction created): 
			composer_(composer), data_(data), chain_(chain), prev_(prev), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(CubixLightComposerPtr composer, const std::vector<unsigned char>& data, const uint256& chain, Transaction::UnlinkedOutPtr prev, transactionCreatedWithOutFunction created) {
			return std::make_shared<CreateTxMediaData>(composer, data, chain, prev, created); 
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during creating media data.");
		}

	private:
		CubixLightComposerPtr composer_;
		std::vector<unsigned char> data_;
		uint256 chain_;
		Transaction::UnlinkedOutPtr prev_;
		transactionCreatedWithOutFunction created_;
		errorFunction error_;

		TxMediaDataPtr tx_;
		TransactionContextPtr ctx_;
	};

	class CreateTxMediaHeader: public IComposerMethod, public std::enable_shared_from_this<CreateTxMediaHeader> {
	public:
		CreateTxMediaHeader(CubixLightComposerPtr composer, 
			uint64_t size, 
			const std::vector<unsigned char>& data,
			unsigned short orientation, 
			const std::string& name,
			const std::string& description,
			TxMediaHeader::Type type,
			TxMediaHeader::Type previewType,
			const uint256& chain, 
			Transaction::UnlinkedOutPtr prev,
			transactionCreatedFunction created): 
			composer_(composer), size_(size), data_(data), orientation_(orientation), name_(name), description_(description), type_(type), previewType_(previewType), chain_(chain), prev_(prev), created_(created) {}
		CreateTxMediaHeader(CubixLightComposerPtr composer,
			uint64_t size,
			const std::vector<unsigned char>& data,
			unsigned short orientation,
			unsigned int duration,
			const std::string& name,
			const std::string& description,
			TxMediaHeader::Type type,
			TxMediaHeader::Type previewType,
			const uint256& chain,
			Transaction::UnlinkedOutPtr prev,
			transactionCreatedFunction created):
			composer_(composer), size_(size), data_(data), orientation_(orientation), duration_(duration), name_(name), description_(description), type_(type), previewType_(previewType), chain_(chain), prev_(prev), created_(created) {}
		void process(errorFunction);

		static IComposerMethodPtr instance(
				CubixLightComposerPtr composer, uint64_t size,
				const std::vector<unsigned char>& data, unsigned short orientation,
				const std::string& name, const std::string& description, TxMediaHeader::Type type, TxMediaHeader::Type previewType, const uint256& chain, Transaction::UnlinkedOutPtr prev, transactionCreatedFunction created) {
			return std::make_shared<CreateTxMediaHeader>(composer, size, data, orientation, name, description, type, previewType, chain, prev, created);
		}
		static IComposerMethodPtr instance(
				CubixLightComposerPtr composer, uint64_t size,
				const std::vector<unsigned char>& data, unsigned short orientation, unsigned int duration,
				const std::string& name, const std::string& description, TxMediaHeader::Type type, TxMediaHeader::Type previewType, const uint256& chain, Transaction::UnlinkedOutPtr prev, transactionCreatedFunction created) {
			return std::make_shared<CreateTxMediaHeader>(composer, size, data, orientation, duration, name, description, type, previewType, chain, prev, created);
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during creating media header.");
		}

	private:
		CubixLightComposerPtr composer_;
		uint64_t size_;
		std::vector<unsigned char> data_;
		unsigned short orientation_ = 0;
		unsigned int duration_ = 0;
		std::string name_;
		std::string description_;
		TxMediaHeader::Type type_;
		TxMediaHeader::Type previewType_;
		uint256 chain_;
		Transaction::UnlinkedOutPtr prev_;
		transactionCreatedFunction created_;
		errorFunction error_;

		TxMediaHeaderPtr tx_;
		TransactionContextPtr ctx_;
	};

public:
	CubixLightComposer(ISettingsPtr settings, IWalletPtr wallet, IRequestProcessorPtr requestProcessor): 
		settings_(settings), wallet_(wallet), requestProcessor_(requestProcessor) {
		opened_ = false;

		dAppInstance_ = Random::generate(); // stub
	}

	static CubixLightComposerPtr instance(ISettingsPtr settings, IWalletPtr wallet, IRequestProcessorPtr requestProcessor) {
		return std::make_shared<CubixLightComposer>(settings, wallet, requestProcessor); 
	}

	// composer management
	bool open();
	bool close();
	bool isOpened() { return opened_; }

	std::string dAppName() { return "cubix"; }
	IWalletPtr wallet() { return wallet_; }
	ISettingsPtr settings() { return settings_; }
	IRequestProcessorPtr requestProcessor() { return requestProcessor_; }

	void setDAppSharedInstance(const uint256& instance) {
		//
		dAppInstance_ = instance;
		requestProcessor_->addDAppInstance(State::DAppInstance(dAppName(), dAppInstance_));
	}

private:
	// various settings, command line args & config file
	ISettingsPtr settings_;
	// wallet
	IWalletPtr wallet_;
	// request processor
	IRequestProcessorPtr requestProcessor_;
	// instance, shared
	uint256 dAppInstance_; //
	// flag
	bool opened_;
};

}
}

#endif
