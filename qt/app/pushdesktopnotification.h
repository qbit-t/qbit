#ifndef PUSHDESKTOPNOTIFICATION_H
#define PUSHDESKTOPNOTIFICATION_H

#include "settings.h"
#include "../../client/commandshandler.h"
#include "../../client/commands.h"

#include "dapps/buzzer/buzzfeed.h"
#include "dapps/buzzer/eventsfeed.h"
#include "dapps/buzzer/buzzfeedview.h"
#include "dapps/cubix/cubix.h"

namespace buzzer {

//
// forward
//

class PushNotification;
typedef std::shared_ptr<PushNotification> PushNotificationPtr;

//
// notification item
//

class PushNotification: public std::enable_shared_from_this<PushNotification> {
public:
	PushNotification(qbit::EventsfeedItemPtr buzz, QString buzzBody, bool autohide) : buzz_(buzz), buzzBody_(buzzBody), autohide_(autohide) {}

	static PushNotificationPtr instance(qbit::EventsfeedItemPtr /*buzz*/, QString /*body*/, bool /*autohide*/);

	void downloadProgress(uint64_t, uint64_t) {
		//
	}

	QString getId();
	QString getChain();
	QString getAlias();
	QString getBuzzer();
	QString getComment();
	QString getText();
	QString getAvatar() { return QString::fromStdString(avatarFile_); }
	QString getMedia() { return QString::fromStdString(mediaFile_); }
	unsigned short getType() { return buzz_->type(); }
	QString getConversationId();
	qbit::EventsfeedItem::Key getKey() { return buzz_->key(); }

	void makeNotification();
	void avatarDownloadDone(qbit::TransactionPtr tx,
							const std::string& previewFile,
							const std::string& originalFile,
							unsigned short orientation,
							unsigned int duration,
							uint64_t size,
							const qbit::cubix::doneDownloadWithErrorFunctionExtraArgs& extra,
							const qbit::ProcessingError& result);
	void mediaDownloadDone(qbit::TransactionPtr tx,
						   const std::string& previewFile,
						   const std::string& originalFile,
						   unsigned short orientation,
						   unsigned int duration,
						   uint64_t size,
						   const qbit::cubix::doneDownloadWithErrorFunctionExtraArgs& extra,
						   const qbit::ProcessingError& result);
	void messageDone(const std::string& /*key*/, const std::string& /*body*/, const qbit::ProcessingError& /*result*/);

	void loadAvatar();
	void loadMedia();
	void loadMessage();
	void process();

private:
	qbit::EventsfeedItemPtr buzz_;
	QString buzzBody_;
	bool autohide_ = false;
	qbit::ICommandPtr downloadAvatar_;
	qbit::ICommandPtr downloadMedia_;
	qbit::ICommandPtr decryptMessage_;

	std::string mediaFile_;
	std::string avatarFile_;
	QString message_;

	static std::map<qbit::EventsfeedItem::Key, PushNotificationPtr> instances_;
};

} // buzzer

#endif // PUSHDESKTOPNOTIFICATION_H
