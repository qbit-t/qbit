#include "pushdesktopnotification.h"
#include "client.h"
#include "../../client/dapps/cubix/cubixcommands.h"
#include "../../client/dapps/buzzer/buzzercommands.h"
#include "applicationpath.h"
#include "notificator.h"

using namespace buzzer;

std::map<qbit::EventsfeedItem::Key, PushNotificationPtr> PushNotification::instances_;

QString PushNotification::getId() {
	// unsigned
	return QString::fromStdString(buzz_->buzzId().toHex());
}

QString PushNotification::getChain() {
	// unsigned
	return QString::fromStdString(buzz_->buzzChainId().toHex());
}

QString PushNotification::getConversationId() {
	//
	if (buzz_->type() == qbit::TX_BUZZER_MESSAGE || buzz_->type() == qbit::TX_BUZZER_MESSAGE_REPLY) {
		if (buzz_->buzzers().size()) {
			return QString::fromStdString(buzz_->buzzers()[0].eventId().toHex());
		}
	}

	return QString::fromStdString(buzz_->buzzId().toHex());
}

QString PushNotification::getAlias() {
	//
	uint256 lInfoId;
	if (buzz_->buzzers().size()) {
		lInfoId = buzz_->buzzers()[0].buzzerInfoId();
	} else {
		lInfoId = buzz_->publisherInfo();
	}

	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	qbit::TxBuzzerInfoPtr lInfo = lClient->getBuzzer()->locateBuzzerInfo(lInfoId);
	if (lInfo) {
		std::string lAlias(lInfo->alias().begin(), lInfo->alias().end());
		return QString::fromStdString(lAlias);
	}

	return QString();
}

QString PushNotification::getBuzzer() {
	//
	uint256 lInfoId;
	if (buzz_->buzzers().size()) {
		lInfoId = buzz_->buzzers()[0].buzzerInfoId();
	} else {
		lInfoId = buzz_->publisherInfo();
	}

	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	qbit::TxBuzzerInfoPtr lInfo = lClient->getBuzzer()->locateBuzzerInfo(lInfoId);
	if (lInfo) {
		return QString::fromStdString(lInfo->myName());
	}

	return QString();
}

QString PushNotification::getComment() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	if (buzz_->type() == qbit::TX_BUZZ_LIKE) {
		if (buzz_->buzzers().size() > 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.like.many");
			return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
		} else if (buzz_->buzzers().size() == 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.like.one");
			return lString;
		}
	} else if (buzz_->type() == qbit::TX_REBUZZ) {
		if (buzz_->buzzers().size() > 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.rebuzz.many");
			return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
		} else if (buzz_->buzzers().size() == 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.rebuzz.one");
			return lString;
		}
	} else if (buzz_->type() == qbit::TX_BUZZ_REWARD) {
		if (buzz_->buzzers().size() > 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.reward.many");
			return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
		} else if (buzz_->buzzers().size() == 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.reward.one");
			return lString;
		}
	} else if (buzz_->type() == qbit::TX_BUZZER_MISTRUST) {
		if (buzz_->buzzers().size() > 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.mistrust.many");
			return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
		} else if (buzz_->buzzers().size() == 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.mistrust.one");
			return lString;
		}
	} else if (buzz_->type() == qbit::TX_BUZZER_ENDORSE) {
		if (buzz_->buzzers().size() > 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.endorse.many");
			return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
		} else if (buzz_->buzzers().size() == 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.endorse.one");
			return lString;
		}
	} else if (buzz_->type() == qbit::TX_BUZZ_REPLY) {
		if (buzz_->buzzers().size() > 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.reply.many");
			return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
		} else if (buzz_->buzzers().size() == 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.reply.one");
			return lString;
		}
	} else if (buzz_->type() == qbit::TX_REBUZZ_REPLY) {
		if (buzz_->buzzers().size() > 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.rebuzz.many");
			return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
		} else if (buzz_->buzzers().size() == 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.rebuzz.one");
			return lString;
		}
	} else if (buzz_->type() == qbit::TX_BUZZER_SUBSCRIBE) {
		if (buzz_->buzzers().size() > 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.subscribe.many");
			return lString.replace(QString("{count}"), QString::number(buzz_->buzzers().size()));
		} else if (buzz_->buzzers().size() == 1) {
			QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.subscribe.one");
			return lString;
		}
	} else if (buzz_->type() == qbit::TX_BUZZER_CONVERSATION) {
		QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.conversation.one");
		return lString;
	} else if (buzz_->type() == qbit::TX_BUZZER_ACCEPT_CONVERSATION) {
		QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.conversation.accepted.one");
		return lString;
	} else if (buzz_->type() == qbit::TX_BUZZER_DECLINE_CONVERSATION) {
		QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.conversation.declined.one");
		return lString;
	} else if (buzz_->type() == qbit::TX_BUZZER_MESSAGE || buzz_->type() == qbit::TX_BUZZER_MESSAGE_REPLY) {
		QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.conversation.message.one");
		return lString;
	}

	return "";
}

QString PushNotification::getText() {
	if (buzz_->type() == qbit::TX_BUZZER_MESSAGE || buzz_->type() == qbit::TX_BUZZER_MESSAGE_REPLY) {
		// Client* lClient = static_cast<Client*>(gApplication->getClient());
		// QString lString = gApplication->getLocalization(lClient->locale(), "Buzzer.event.conversation.message.encrypted.notification");
		return message_;
	}

	return QString::fromStdString(buzz_->buzzBodyString());
}

void PushNotification::makeNotification() {
	//
	buzzer::Notificator::showMessage(shared_from_this(), autohide_);
	instances_.erase(buzz_->key());
}

void PushNotification::avatarDownloadDone(qbit::TransactionPtr /*tx*/,
										  const std::string& previewFile,
										  const std::string& /*originalFile*/,
										  unsigned short /*orientation*/,
										  unsigned int /*duration*/,
										  uint64_t /*size*/,
										  const qbit::cubix::doneDownloadWithErrorFunctionExtraArgs& extra,
										  const qbit::ProcessingError& result) {
	//
	if (result.success()) {
		//
		if ((qbit::cubix::TxMediaHeader::MediaType)extra.type == qbit::cubix::TxMediaHeader::MediaType::IMAGE_JPEG ||
				(qbit::cubix::TxMediaHeader::MediaType)extra.type == qbit::cubix::TxMediaHeader::MediaType::IMAGE_PNG)
			avatarFile_ = previewFile;

		if (buzz_->type() == qbit::TX_BUZZER_MESSAGE || buzz_->type() == qbit::TX_BUZZER_MESSAGE_REPLY) {
			// try to decrypt
			loadMessage();
		} else {
			// push notification
			makeNotification();
		}
	} else {
		qbit::gLog().write(qbit::Log::CLIENT, strprintf("[downloadAvatar/error]: %s - %s", result.error(), result.message()));
	}
}

void PushNotification::mediaDownloadDone(qbit::TransactionPtr /*tx*/,
										 const std::string& previewFile,
										 const std::string& /*originalFile*/,
										 unsigned short /*orientation*/,
										 unsigned int /*duration*/,
										 uint64_t /*size*/,
										 const qbit::cubix::doneDownloadWithErrorFunctionExtraArgs& extra,
										 const qbit::ProcessingError& result) {
	//
	if (result.success()) {
		//
		if ((qbit::cubix::TxMediaHeader::MediaType)extra.type == qbit::cubix::TxMediaHeader::MediaType::IMAGE_JPEG ||
				(qbit::cubix::TxMediaHeader::MediaType)extra.type == qbit::cubix::TxMediaHeader::MediaType::IMAGE_PNG) {
			mediaFile_ = previewFile;
		}
	} else {
		qbit::gLog().write(qbit::Log::CLIENT, strprintf("[downloadMedia/error]: %s - %s", result.error(), result.message()));
	}

	// anyway
	loadAvatar();
}

void PushNotification::messageDone(const std::string& /*key*/, const std::string& body, const qbit::ProcessingError& result) {
	//
	if (!result.success()) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());
		message_ = gApplication->getLocalization(lClient->locale(), "Buzzer.event.conversation.message.encrypted.notification");
	} else {
		message_ = QString::fromStdString(body);
	}

	// push notification
	makeNotification();
}

void PushNotification::loadAvatar() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	downloadAvatar_ =
			qbit::cubix::DownloadMediaCommand::instance(lClient->getCubixComposer(),
												  boost::bind(&PushNotification::downloadProgress, this, _1, _2),
												  boost::bind(&PushNotification::avatarDownloadDone, this, _1, _2, _3, _4, _5, _6, _7, _8));
	// locate buzzer info and avatar url
	uint256 lInfoId;
	if (buzz_->buzzers().size()) {
		lInfoId = buzz_->buzzers()[0].buzzerInfoId();
	} else {
		lInfoId = buzz_->publisherInfo();
	}

	std::string lUrl;
	std::string lHeader;
	qbit::BuzzerMediaPointer lMedia;
	if (lClient->getBuzzer()->locateAvatarMedia(lInfoId, lMedia)) {
		lUrl = lMedia.url();
		lHeader = lMedia.tx().toHex();
	}

	// temp path
	std::string lPath = buzzer::ApplicationPath::tempFilesDir().toStdString();

	// process
	std::vector<std::string> lArgs;
	lArgs.push_back(lUrl);
	lArgs.push_back(lPath + "/" + lHeader);
	lArgs.push_back("-preview");
	lArgs.push_back("-skip");
	downloadAvatar_->process(lArgs);
}

void PushNotification::loadMedia() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	qbit::BuzzerMediaPointer lMedia;
	if (buzz_->buzzMedia().size()) lMedia = buzz_->buzzMedia()[0];
	else {
		lMedia = buzz_->buzzers()[0].buzzMedia()[0];
	}

	downloadMedia_ =
			qbit::cubix::DownloadMediaCommand::instance(lClient->getCubixComposer(),
												  boost::bind(&PushNotification::downloadProgress, this, _1, _2),
												  boost::bind(&PushNotification::mediaDownloadDone, this, _1, _2, _3, _4, _5, _6, _7, _8));
	std::string lUrl = lMedia.url();
	std::string lHeader = lMedia.tx().toHex();

	// temp path
	std::string lPath = buzzer::ApplicationPath::tempFilesDir().toStdString();

	// process
	std::vector<std::string> lArgs;
	lArgs.push_back(lUrl);
	lArgs.push_back(lPath + "/" + lHeader);
	lArgs.push_back("-preview");
	lArgs.push_back("-skip");
	downloadMedia_->process(lArgs);
}

void PushNotification::loadMessage() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	decryptMessage_ = qbit::DecryptMessageBodyCommand::instance(
					lClient->getBuzzerComposer(),
					lClient->getConversationsModel()->conversations(),
					boost::bind(&PushNotification::messageDone, this, _1, _2, _3)
	);

	std::vector<std::string> lArgs;
	if (buzz_->buzzers().size()) {
		lArgs.push_back(buzz_->beginInfo()->eventId().toHex());
		lArgs.push_back(buzz_->beginInfo()->buzzBodyHex());
		decryptMessage_->process(lArgs);
	}
}

void PushNotification::process() {
	//
	if ((buzz_->buzzMedia().size() || (buzz_->buzzers().size() && buzz_->buzzers()[0].buzzMedia().size())) &&
			(buzz_->type() != qbit::TX_BUZZER_MESSAGE && buzz_->type() != qbit::TX_BUZZER_MESSAGE_REPLY)) {
		loadMedia();
	} else {
		loadAvatar();
	}
}

PushNotificationPtr PushNotification::instance(qbit::EventsfeedItemPtr buzz, bool autohide) {
	//
	PushNotificationPtr lPush = std::make_shared<PushNotification>(buzz, autohide);
	instances_[buzz->key()] = lPush;
	return lPush;
}
