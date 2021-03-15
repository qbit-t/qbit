#ifndef PUSHDESKTOPNOTIFICATION_H
#define PUSHDESKTOPNOTIFICATION_H

#include "settings.h"
#include "../../client/commandshandler.h"
#include "../../client/commands.h"

#include "dapps/buzzer/buzzfeed.h"
#include "dapps/buzzer/eventsfeed.h"
#include "dapps/buzzer/buzzfeedview.h"

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
	PushNotification(qbit::EventsfeedItemPtr buzz) : buzz_(buzz) {}

	static PushNotificationPtr instance(qbit::EventsfeedItemPtr /*buzz*/);

	void downloadProgress(uint64_t, uint64_t) {
		//
	}

	QString getId();
	QString getAlias();
	QString getBuzzer();
	QString getComment();
	QString getText();
	QString getAvatar() { return QString::fromStdString(avatarFile_); }
	QString getMedia() { return QString::fromStdString(mediaFile_); }
	unsigned short getType() { return buzz_->type(); }

	void makeNotification();
	void avatarDownloadDone(qbit::TransactionPtr /*tx*/,
						   const std::string& previewFile,
						   const std::string& /*originalFile*/, unsigned short /*orientation*/, const qbit::ProcessingError& result);
	void mediaDownloadDone(qbit::TransactionPtr /*tx*/,
						   const std::string& previewFile,
						   const std::string& /*originalFile*/, unsigned short /*orientation*/, const qbit::ProcessingError& result);
	void loadAvatar();
	void loadMedia();
	void process();

private:
	qbit::EventsfeedItemPtr buzz_;
	qbit::ICommandPtr downloadAvatar_;
	qbit::ICommandPtr downloadMedia_;

	std::string mediaFile_;
	std::string avatarFile_;

	static std::map<qbit::EventsfeedItem::Key, PushNotificationPtr> instances_;
};

} // buzzer

#endif // PUSHDESKTOPNOTIFICATION_H
