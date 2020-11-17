// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_LIGHT_COMPOSER_H
#define QBIT_LIGHT_COMPOSER_H

#include "handlers.h"
#include "../iwallet.h"
#include "../irequestprocessor.h"

namespace qbit {

// forward
class LightComposer;
typedef std::shared_ptr<LightComposer> LightComposerPtr;

//
class LightComposer: public std::enable_shared_from_this<LightComposer> {
public:
	class Balance: public IComposerMethod, public std::enable_shared_from_this<Balance> {
	public:
		Balance(LightComposerPtr composer, const uint256& asset, balanceReadyFunction balance): composer_(composer), asset_(asset), balance_(balance) {}
		void process(errorFunction error) {
			error_ = error;

			if (!asset_.isNull()) {
				//
				composer_->requestProcessor()->loadTransaction(MainChain::id(), asset_, 
					LoadTransaction::instance(
						boost::bind(&LightComposer::Balance::assetLoaded, shared_from_this(), _1),
						boost::bind(&LightComposer::Balance::timeout, shared_from_this()))
				);
			} else {
				assetLoaded(nullptr);		
			}
		}

		static IComposerMethodPtr instance(LightComposerPtr composer, const uint256& asset, balanceReadyFunction balance) {
			return std::make_shared<Balance>(composer, asset, balance); 
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during creating buzzer.");
		}

		//
		void assetLoaded(TransactionPtr asset) {
			//
			amount_t lScale = QBIT;
			uint256 lAsset = TxAssetType::qbitAsset();

			if (asset) {
				if (asset->type() == Transaction::ASSET_TYPE) {
					TxAssetTypePtr lAssetType = TransactionHelper::to<TxAssetType>(asset);
					lScale = lAssetType->scale();
					lAsset = asset->id();
				}
			}

			double lBalance = ((double)composer_->wallet()->balance(lAsset)) / lScale;
			double lPendingBalance = ((double)composer_->wallet()->pendingBalance(lAsset)) / lScale;
			balance_(lBalance, lPendingBalance - lBalance, lScale);
		}

	private:
		LightComposerPtr composer_;
		uint256 asset_;
		balanceReadyFunction balance_;
		errorFunction error_;
	};

	class SendToAddress: public IComposerMethod, public std::enable_shared_from_this<SendToAddress> {
	public:
		SendToAddress(LightComposerPtr composer, const uint256& asset, const PKey& address, double amount, transactionCreatedFunction created): 
			composer_(composer), asset_(asset), address_(address), amount_(amount), created_(created) {}
		SendToAddress(LightComposerPtr composer, const uint256& asset, const PKey& address, double amount, int feeRate, transactionCreatedFunction created): 
			composer_(composer), asset_(asset), address_(address), amount_(amount), feeRate_(feeRate), created_(created) {}
		void process(errorFunction error) {
			error_ = error;

			if (!asset_.isNull()) {
				//
				composer_->requestProcessor()->loadTransaction(MainChain::id(), asset_, 
					LoadTransaction::instance(
						boost::bind(&LightComposer::SendToAddress::assetLoaded, shared_from_this(), _1),
						boost::bind(&LightComposer::SendToAddress::timeout, shared_from_this()))
				);
			} else {
				assetLoaded(nullptr);		
			}
		}

		static IComposerMethodPtr instance(LightComposerPtr composer, const uint256& asset, const PKey& address, double amount, int feeRate, transactionCreatedFunction created) {
			return std::make_shared<SendToAddress>(composer, asset, address, amount, feeRate, created); 
		}

		static IComposerMethodPtr instance(LightComposerPtr composer, const uint256& asset, const PKey& address, double amount, transactionCreatedFunction created) {
			return std::make_shared<SendToAddress>(composer, asset, address, amount, created); 
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during creating buzzer.");
		}

		//
		void assetLoaded(TransactionPtr asset) {
			//
			amount_t lScale = QBIT;
			uint256 lAsset = TxAssetType::qbitAsset();

			if (asset) {
				if (asset->type() == Transaction::ASSET_TYPE) {
					TxAssetTypePtr lAssetType = TransactionHelper::to<TxAssetType>(asset);
					lScale = lAssetType->scale();	
					lAsset = lAssetType->id();
				}
			}

			try {
				TransactionContextPtr lCtx;
				if (feeRate_ == -1) 
					lCtx = composer_->wallet()->createTxSpend(lAsset, address_, (amount_t)(amount_ * (double)lScale));
				else
					lCtx = composer_->wallet()->createTxSpend(lAsset, address_, (amount_t)(amount_ * (double)lScale), feeRate_);
				if (lCtx) { 
					lCtx->setScale(lScale);
					created_(lCtx);
				} else error_("E_TX_CREATE", "Transaction creation error.");
			}
			catch(qbit::exception& ex) {
				error_(ex.code(), ex.what());
			}
			catch(std::exception& ex) {
				error_("E_TX_CREATE", ex.what());
			}
		}

	private:
		LightComposerPtr composer_;
		uint256 asset_;
		PKey address_;
		double amount_ = 0;
		int feeRate_ = -1;
		transactionCreatedFunction created_;
		errorFunction error_;
	};

	class SendPrivateToAddress: public IComposerMethod, public std::enable_shared_from_this<SendPrivateToAddress> {
	public:
		SendPrivateToAddress(LightComposerPtr composer, const uint256& asset, const PKey& address, double amount, transactionCreatedFunction created): 
			composer_(composer), asset_(asset), address_(address), amount_(amount), created_(created) {}
		SendPrivateToAddress(LightComposerPtr composer, const uint256& asset, const PKey& address, double amount, int feeRate, transactionCreatedFunction created): 
			composer_(composer), asset_(asset), address_(address), amount_(amount), feeRate_(feeRate), created_(created) {}
		void process(errorFunction error) {
			error_ = error;

			if (!asset_.isNull()) {
				//
				composer_->requestProcessor()->loadTransaction(MainChain::id(), asset_, 
					LoadTransaction::instance(
						boost::bind(&LightComposer::SendPrivateToAddress::assetLoaded, shared_from_this(), _1),
						boost::bind(&LightComposer::SendPrivateToAddress::timeout, shared_from_this()))
				);
			} else {
				assetLoaded(nullptr);		
			}
		}

		static IComposerMethodPtr instance(LightComposerPtr composer, const uint256& asset, const PKey& address, double amount, int feeRate, transactionCreatedFunction created) {
			return std::make_shared<SendPrivateToAddress>(composer, asset, address, amount, feeRate, created); 
		}

		static IComposerMethodPtr instance(LightComposerPtr composer, const uint256& asset, const PKey& address, double amount, transactionCreatedFunction created) {
			return std::make_shared<SendPrivateToAddress>(composer, asset, address, amount, created); 
		}

		// 
		void timeout() {
			error_("E_TIMEOUT", "Timeout expired during creating buzzer.");
		}

		//
		void assetLoaded(TransactionPtr asset) {
			//
			amount_t lScale = QBIT;
			uint256 lAsset = TxAssetType::qbitAsset();

			if (asset) {
				if (asset->type() == Transaction::ASSET_TYPE) {
					TxAssetTypePtr lAssetType = TransactionHelper::to<TxAssetType>(asset);
					lScale = lAssetType->scale();	
					lAsset = lAssetType->id();
				}
			}

			try {
				TransactionContextPtr lCtx;
				if (feeRate_ == -1) 
					lCtx = composer_->wallet()->createTxSpendPrivate(lAsset, address_, (amount_t)(amount_ * (double)lScale));
				else
					lCtx = composer_->wallet()->createTxSpendPrivate(lAsset, address_, (amount_t)(amount_ * (double)lScale), feeRate_);
				if (lCtx) {
					lCtx->setScale(lScale);
					created_(lCtx);
				} else error_("E_TX_CREATE", "Transaction creation error.");
			}
			catch(qbit::exception& ex) {
				error_(ex.code(), ex.what());
			}
			catch(std::exception& ex) {
				error_("E_TX_CREATE", ex.what());
			}
		}

	private:
		LightComposerPtr composer_;
		uint256 asset_;
		PKey address_;
		double amount_ = 0;
		int feeRate_ = -1;
		transactionCreatedFunction created_;
		errorFunction error_;
	};

public:
	LightComposer(ISettingsPtr settings, IWalletPtr wallet, IRequestProcessorPtr requestProcessor): 
		settings_(settings), wallet_(wallet), requestProcessor_(requestProcessor) {
	}

	static LightComposerPtr instance(ISettingsPtr settings, IWalletPtr wallet, IRequestProcessorPtr requestProcessor) {
		return std::make_shared<LightComposer>(settings, wallet, requestProcessor); 
	}

	// composer management
	bool open() { return true; }
	bool close() { return true; }
	bool isOpened() { return true; }

	IWalletPtr wallet() { return wallet_; }
	ISettingsPtr settings() { return settings_; }
	IRequestProcessorPtr requestProcessor() { return requestProcessor_; }

private:
	// various settings, command line args & config file
	ISettingsPtr settings_;
	// wallet
	IWalletPtr wallet_;
	// request processor
	IRequestProcessorPtr requestProcessor_;
};

} // qbit

#endif