#include "commandwrappers.h"
#include "client.h"

using namespace buzzer;

AskForQbitsCommand::AskForQbitsCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::AskForQbitsCommand::instance(lClient->getComposer(), boost::bind(&AskForQbitsCommand::done, this, boost::placeholders::_1));
}

BalanceCommand::BalanceCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::BalanceCommand::instance(lClient->getComposer(), boost::bind(&BalanceCommand::done, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3, boost::placeholders::_4));
}

LoadBuzzerTrustScoreCommand::LoadBuzzerTrustScoreCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::LoadBuzzerTrustScoreCommand::instance(
		lClient->getBuzzerComposer(),
		boost::bind(&LoadBuzzerTrustScoreCommand::done, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3, boost::placeholders::_4, boost::placeholders::_5)
	);
}

SearchEntityNamesCommand::SearchEntityNamesCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::SearchEntityNamesCommand::instance(
		lClient->getComposer(),
		boost::bind(&SearchEntityNamesCommand::done, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3)
	);
}

LoadHashTagsCommand::LoadHashTagsCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::LoadHashTagsCommand::instance(
		lClient->getBuzzerComposer(),
		false, // console output
		boost::bind(&LoadHashTagsCommand::done, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3)
	);
}

CreateBuzzerCommand::CreateBuzzerCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::CreateBuzzerCommand>(
		qbit::CreateBuzzerCommand::instance(lClient->getBuzzerComposer(), boost::bind(&CreateBuzzerCommand::done, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3))
	);

	command_->setUploadAvatar(
		qbit::cubix::UploadMediaCommand::instance(
			lClient->getCubixComposer(),
			boost::bind(&CreateBuzzerCommand::avatarUploadProgress, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3),
			boost::bind(&qbit::CreateBuzzerCommand::avatarUploaded, command_, boost::placeholders::_1, boost::placeholders::_2))
	);
	command_->setUploadHeader(
		qbit::cubix::UploadMediaCommand::instance(
			lClient->getCubixComposer(),
			boost::bind(&CreateBuzzerCommand::headerUploadProgress, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3),
			"760x570", // special header case - higher resolution for image
			boost::bind(&qbit::CreateBuzzerCommand::headerUploaded, command_, boost::placeholders::_1, boost::placeholders::_2))
	);
}

CreateBuzzerInfoCommand::CreateBuzzerInfoCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::CreateBuzzerInfoCommand>(
		qbit::CreateBuzzerInfoCommand::instance(lClient->getBuzzerComposer(), boost::bind(&CreateBuzzerInfoCommand::done, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3))
	);

	command_->setUploadAvatar(
		qbit::cubix::UploadMediaCommand::instance(
			lClient->getCubixComposer(),
			boost::bind(&CreateBuzzerInfoCommand::avatarUploadProgress, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3),
			boost::bind(&qbit::CreateBuzzerInfoCommand::avatarUploaded, command_, boost::placeholders::_1, boost::placeholders::_2))
	);
	command_->setUploadHeader(
		qbit::cubix::UploadMediaCommand::instance(
			lClient->getCubixComposer(),
			boost::bind(&CreateBuzzerInfoCommand::headerUploadProgress, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3),
			"760x570", // special header case - higher resolution for image
			boost::bind(&qbit::CreateBuzzerInfoCommand::headerUploaded, command_, boost::placeholders::_1, boost::placeholders::_2))
	);
}

LoadBuzzesGlobalCommand::LoadBuzzesGlobalCommand(QObject* /*parent*/) : QObject() {}

LoadBuzzesGlobalCommand::~LoadBuzzesGlobalCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadBuzzesGlobalCommand::prepare() {
	//
	if (!command_ && buzzfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadBuzzesGlobalCommand>(
			qbit::LoadBuzzesGlobalCommand::instance(
				lClient->getBuzzerComposer(),
				buzzfeedModel_->buzzfeed(),
				boost::bind(&LoadBuzzesGlobalCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadBuzzesGlobalCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), buzzfeedModel_, SLOT(feedSlot(const qbit::BuzzfeedProxy&, bool, bool)));
	}
}

void LoadBuzzesGlobalCommand::ready(qbit::BuzzfeedPtr /*base*/, qbit::BuzzfeedPtr local) {
	//
	if (buzzfeedModel_) {
		emit dataReady(qbit::BuzzfeedProxy(local), more_, false);
	}
}

//
// LoadBuzzfeedByBuzzCommand
//

LoadBuzzfeedByBuzzCommand::LoadBuzzfeedByBuzzCommand(QObject* /*parent*/) : QObject() {}

LoadBuzzfeedByBuzzCommand::~LoadBuzzfeedByBuzzCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadBuzzfeedByBuzzCommand::prepare() {
	//
	if (!command_ && buzzfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		uint256 lRootId; lRootId.setHex(buzzId_.toStdString());
		buzzfeedModel_->setRootId(lRootId);
		buzzfeedModel_->buzzfeed()->setRootBuzzId(lRootId);

		command_ = std::static_pointer_cast<qbit::LoadBuzzfeedByBuzzCommand>(
			qbit::LoadBuzzfeedByBuzzCommand::instance(
				lClient->getBuzzerComposer(),
				buzzfeedModel_->buzzfeed(),
				boost::bind(&LoadBuzzfeedByBuzzCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadBuzzfeedByBuzzCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), buzzfeedModel_, SLOT(feedSlot(const qbit::BuzzfeedProxy&, bool, bool)));
	}
}

void LoadBuzzfeedByBuzzCommand::ready(qbit::BuzzfeedPtr /*base*/, qbit::BuzzfeedPtr local) {
	//
	if (buzzfeedModel_) {
		emit dataReady(qbit::BuzzfeedProxy(local), more_, merge_);
	}
}

//
// LoadBuzzfeedByBuzzerCommand
//

LoadBuzzfeedByBuzzerCommand::LoadBuzzfeedByBuzzerCommand(QObject* /*parent*/) : QObject() {}

LoadBuzzfeedByBuzzerCommand::~LoadBuzzfeedByBuzzerCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadBuzzfeedByBuzzerCommand::prepare() {
	//
	if (!command_ && buzzfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadBuzzfeedByBuzzerCommand>(
			qbit::LoadBuzzfeedByBuzzerCommand::instance(
				lClient->getBuzzerComposer(),
				buzzfeedModel_->buzzfeed(),
				boost::bind(&LoadBuzzfeedByBuzzerCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadBuzzfeedByBuzzerCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), buzzfeedModel_, SLOT(feedSlot(const qbit::BuzzfeedProxy&, bool, bool)));
	}
}

void LoadBuzzfeedByBuzzerCommand::ready(qbit::BuzzfeedPtr /*base*/, qbit::BuzzfeedPtr local) {
	//
	if (buzzfeedModel_) {
		emit dataReady(qbit::BuzzfeedProxy(local), more_, merge_);
	}
}

//
// LoadBuzzfeedByTagCommand
//

LoadBuzzfeedByTagCommand::LoadBuzzfeedByTagCommand(QObject* /*parent*/) : QObject() {}

LoadBuzzfeedByTagCommand::~LoadBuzzfeedByTagCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadBuzzfeedByTagCommand::prepare() {
	//
	if (!command_ && buzzfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadBuzzfeedByTagCommand>(
			qbit::LoadBuzzfeedByTagCommand::instance(
				lClient->getBuzzerComposer(),
				buzzfeedModel_->buzzfeed(),
				boost::bind(&LoadBuzzfeedByTagCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadBuzzfeedByTagCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), buzzfeedModel_, SLOT(feedSlot(const qbit::BuzzfeedProxy&, bool, bool)));
	}
}

void LoadBuzzfeedByTagCommand::ready(qbit::BuzzfeedPtr /*base*/, qbit::BuzzfeedPtr local) {
	//
	if (buzzfeedModel_) {
		emit dataReady(qbit::BuzzfeedProxy(local), more_, false);
	}
}

//
// LoadBuzzfeedCommand
//

LoadBuzzfeedCommand::LoadBuzzfeedCommand(QObject* /*parent*/) : QObject() {}

LoadBuzzfeedCommand::~LoadBuzzfeedCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadBuzzfeedCommand::prepare() {
	//
	if (!command_ && buzzfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadBuzzfeedCommand>(
			qbit::LoadBuzzfeedCommand::instance(
				lClient->getBuzzerComposer(),
				buzzfeedModel_->buzzfeed(),
				boost::bind(&LoadBuzzfeedCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadBuzzfeedCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), buzzfeedModel_, SLOT(feedSlot(const qbit::BuzzfeedProxy&, bool, bool)));
	}
}

void LoadBuzzfeedCommand::ready(qbit::BuzzfeedPtr /*base*/, qbit::BuzzfeedPtr local) {
	//
	if (buzzfeedModel_) {
		emit dataReady(qbit::BuzzfeedProxy(local), more_, merge_);
	}
}

BuzzerSubscribeCommand::BuzzerSubscribeCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::BuzzerSubscribeCommand::instance(
		lClient->getBuzzerComposer(),
		boost::bind(&BuzzerSubscribeCommand::done, this, boost::placeholders::_1)
	);
}

BuzzerUnsubscribeCommand::BuzzerUnsubscribeCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::BuzzerUnsubscribeCommand::instance(
		lClient->getBuzzerComposer(),
		boost::bind(&BuzzerUnsubscribeCommand::done, this, boost::placeholders::_1)
	);
}

DownloadMediaCommand::DownloadMediaCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::cubix::DownloadMediaCommand::instance(
			lClient->getCubixComposer(),
			boost::bind(&DownloadMediaCommand::downloadProgress, this, boost::placeholders::_1, boost::placeholders::_2),
			boost::bind(&DownloadMediaCommand::done, this,
						boost::placeholders::_1,
						boost::placeholders::_2,
						boost::placeholders::_3,
						boost::placeholders::_4,
						boost::placeholders::_5,
						boost::placeholders::_6,
						boost::placeholders::_7,
						boost::placeholders::_8));

	// TODO: potential leak, need "check list" to track such objects
	QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

DownloadMediaCommand::~DownloadMediaCommand() {
	//
	command_->terminate();
}

UploadMediaCommand::UploadMediaCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::cubix::UploadMediaCommand::instance(
			lClient->getCubixComposer(),
			boost::bind(&UploadMediaCommand::uploadProgress, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3),
			boost::bind(&UploadMediaCommand::done, this, boost::placeholders::_1, boost::placeholders::_2));

	// TODO: potential leak, need "check list" to track such objects
	QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

BuzzLikeCommand::BuzzLikeCommand(QObject* /*parent*/) : QObject() {
	//
}

void BuzzLikeCommand::prepare() {
	if (!buzzfeedModel_) return;

	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::BuzzLikeCommand::instance(
		lClient->getBuzzerComposer(),
		buzzfeedModel_->buzzfeed(),
		boost::bind(&BuzzLikeCommand::done, this, boost::placeholders::_1)
	);
}

BuzzRewardCommand::BuzzRewardCommand(QObject* /*parent*/) : QObject() {
	//
}

void BuzzRewardCommand::prepare() {
	if (!buzzfeedModel_) return;

	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::BuzzRewardCommand>(
		qbit::BuzzRewardCommand::instance(
			lClient->getBuzzerComposer(),
			buzzfeedModel_->buzzfeed(),
			boost::bind(&BuzzRewardCommand::done, this, boost::placeholders::_1))
	);
}

BuzzCommand::BuzzCommand(QObject* /*parent*/) : QObject() {
}

void BuzzCommand::prepare() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::CreateBuzzCommand>(
		qbit::CreateBuzzCommand::instance(
			lClient->getBuzzerComposer(),
			boost::bind(&BuzzCommand::done, this, boost::placeholders::_1))
	);

	if (uploadCommand_) {
		// link command
		command_->setUploadMedia(uploadCommand_->command());
		uploadCommand_->setDone(boost::bind(&qbit::CreateBuzzCommand::mediaUploaded, command_, boost::placeholders::_1, boost::placeholders::_2));

		// link notification
		command_->setMediaUploaded(boost::bind(&BuzzCommand::mediaUploaded, this, boost::placeholders::_1, boost::placeholders::_2));
	}
}

ReBuzzCommand::ReBuzzCommand(QObject* /*parent*/) : QObject() {
}

void ReBuzzCommand::prepare() {
	if (!buzzfeedModel_) return;

	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::CreateReBuzzCommand>(
		qbit::CreateReBuzzCommand::instance(
			lClient->getBuzzerComposer(),
			buzzfeedModel_->buzzfeed(),
			boost::bind(&ReBuzzCommand::done, this, boost::placeholders::_1))
	);

	if (uploadCommand_) {
		// link command
		command_->setUploadMedia(uploadCommand_->command());
		uploadCommand_->setDone(boost::bind(&qbit::CreateReBuzzCommand::mediaUploaded, command_, boost::placeholders::_1, boost::placeholders::_2));

		// link notification
		command_->setMediaUploaded(boost::bind(&ReBuzzCommand::mediaUploaded, this, boost::placeholders::_1, boost::placeholders::_2));
	}
}

ReplyCommand::ReplyCommand(QObject* /*parent*/) : QObject() {
}

void ReplyCommand::prepare() {
	if (!buzzfeedModel_) return;

	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::CreateBuzzReplyCommand>(
		qbit::CreateBuzzReplyCommand::instance(
			lClient->getBuzzerComposer(),
			buzzfeedModel_->buzzfeed(),
			boost::bind(&ReplyCommand::done, this, boost::placeholders::_1))
	);

	if (uploadCommand_) {
		// link command
		command_->setUploadMedia(uploadCommand_->command());
		uploadCommand_->setDone(boost::bind(&qbit::CreateBuzzReplyCommand::mediaUploaded, command_, boost::placeholders::_1, boost::placeholders::_2));

		// link notification
		command_->setMediaUploaded(boost::bind(&ReplyCommand::mediaUploaded, this, boost::placeholders::_1, boost::placeholders::_2));
	}
}

BuzzerEndorseCommand::BuzzerEndorseCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::BuzzerEndorseCommand>(
		qbit::BuzzerEndorseCommand::instance(
			lClient->getBuzzerComposer(),
			boost::bind(&BuzzerEndorseCommand::done, this, boost::placeholders::_1))
	);
}

BuzzerMistrustCommand::BuzzerMistrustCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::BuzzerMistrustCommand>(
		qbit::BuzzerMistrustCommand::instance(
			lClient->getBuzzerComposer(),
			boost::bind(&BuzzerMistrustCommand::done, this, boost::placeholders::_1))
	);
}

BuzzSubscribeCommand::BuzzSubscribeCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::BuzzSubscribeCommand>(
		qbit::BuzzSubscribeCommand::instance(
			lClient->getBuzzerComposer(),
			boost::bind(&BuzzSubscribeCommand::done, this, boost::placeholders::_1))
	);
}

BuzzUnsubscribeCommand::BuzzUnsubscribeCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::BuzzUnsubscribeCommand>(
		qbit::BuzzUnsubscribeCommand::instance(
			lClient->getBuzzerComposer(),
			boost::bind(&BuzzUnsubscribeCommand::done, this, boost::placeholders::_1))
	);
}

LoadBuzzerInfoCommand::LoadBuzzerInfoCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::LoadBuzzerInfoCommand>(
			qbit::LoadBuzzerInfoCommand::instance(
				lClient->getBuzzerComposer(),
				boost::bind(&LoadBuzzerInfoCommand::done, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3, boost::placeholders::_4, boost::placeholders::_5))
	);
}

//
// LoadEndorsementsByBuzzerCommand
//

LoadEndorsementsByBuzzerCommand::LoadEndorsementsByBuzzerCommand(QObject* /*parent*/) : QObject() {}

LoadEndorsementsByBuzzerCommand::~LoadEndorsementsByBuzzerCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadEndorsementsByBuzzerCommand::prepare() {
	//
	if (!command_ && eventsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadEndorsementsByBuzzerCommand>(
			qbit::LoadEndorsementsByBuzzerCommand::instance(
				lClient->getBuzzerComposer(),
				eventsfeedModel_->eventsfeed(),
				boost::bind(&LoadEndorsementsByBuzzerCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadEndorsementsByBuzzerCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::EventsfeedProxy&, bool, bool)), eventsfeedModel_, SLOT(feedSlot(const qbit::EventsfeedProxy&, bool, bool)));
	}
}

void LoadEndorsementsByBuzzerCommand::ready(qbit::EventsfeedPtr /*base*/, qbit::EventsfeedPtr local) {
	//
	if (eventsfeedModel_) {
		emit dataReady(qbit::EventsfeedProxy(local), more_, merge_);
	}
}

//
// LoadMistrustsByBuzzerCommand
//

LoadMistrustsByBuzzerCommand::LoadMistrustsByBuzzerCommand(QObject* /*parent*/) : QObject() {}

LoadMistrustsByBuzzerCommand::~LoadMistrustsByBuzzerCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadMistrustsByBuzzerCommand::prepare() {
	//
	if (!command_ && eventsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadMistrustsByBuzzerCommand>(
			qbit::LoadMistrustsByBuzzerCommand::instance(
				lClient->getBuzzerComposer(),
				eventsfeedModel_->eventsfeed(),
				boost::bind(&LoadMistrustsByBuzzerCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadMistrustsByBuzzerCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::EventsfeedProxy&, bool, bool)), eventsfeedModel_, SLOT(feedSlot(const qbit::EventsfeedProxy&, bool, bool)));
	}
}

void LoadMistrustsByBuzzerCommand::ready(qbit::EventsfeedPtr /*base*/, qbit::EventsfeedPtr local) {
	//
	if (eventsfeedModel_) {
		emit dataReady(qbit::EventsfeedProxy(local), more_, merge_);
	}
}

//
// LoadEventsfeedCommand
//

LoadEventsfeedCommand::LoadEventsfeedCommand(QObject* /*parent*/) : QObject() {}

LoadEventsfeedCommand::~LoadEventsfeedCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadEventsfeedCommand::prepare() {
	//
	if (!command_ && eventsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadEventsfeedCommand>(
			qbit::LoadEventsfeedCommand::instance(
				lClient->getBuzzerComposer(),
				eventsfeedModel_->eventsfeed(),
				boost::bind(&LoadEventsfeedCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadEventsfeedCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::EventsfeedProxy&, bool, bool)), eventsfeedModel_, SLOT(feedSlot(const qbit::EventsfeedProxy&, bool, bool)));
	}
}

void LoadEventsfeedCommand::ready(qbit::EventsfeedPtr /*base*/, qbit::EventsfeedPtr local) {
	//
	if (eventsfeedModel_) {
		emit dataReady(qbit::EventsfeedProxy(local), more_, merge_);
	}
}

//
// LoadFollowingByBuzzerCommand
//

LoadFollowingByBuzzerCommand::LoadFollowingByBuzzerCommand(QObject* /*parent*/) : QObject() {}

LoadFollowingByBuzzerCommand::~LoadFollowingByBuzzerCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadFollowingByBuzzerCommand::prepare() {
	//
	if (!command_ && eventsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadSubscriptionsByBuzzerCommand>(
			qbit::LoadSubscriptionsByBuzzerCommand::instance(
				lClient->getBuzzerComposer(),
				eventsfeedModel_->eventsfeed(),
				boost::bind(&LoadFollowingByBuzzerCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadFollowingByBuzzerCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::EventsfeedProxy&, bool, bool)), eventsfeedModel_, SLOT(feedSlot(const qbit::EventsfeedProxy&, bool, bool)));
	}
}

void LoadFollowingByBuzzerCommand::ready(qbit::EventsfeedPtr /*base*/, qbit::EventsfeedPtr local) {
	//
	if (eventsfeedModel_) {
		emit dataReady(qbit::EventsfeedProxy(local), more_, merge_);
	}
}

//
// LoadFollowersByBuzzerCommand
//

LoadFollowersByBuzzerCommand::LoadFollowersByBuzzerCommand(QObject* /*parent*/) : QObject() {}

LoadFollowersByBuzzerCommand::~LoadFollowersByBuzzerCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadFollowersByBuzzerCommand::prepare() {
	//
	if (!command_ && eventsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadFollowersByBuzzerCommand>(
			qbit::LoadFollowersByBuzzerCommand::instance(
				lClient->getBuzzerComposer(),
				eventsfeedModel_->eventsfeed(),
				boost::bind(&LoadFollowersByBuzzerCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadFollowersByBuzzerCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::EventsfeedProxy&, bool, bool)), eventsfeedModel_, SLOT(feedSlot(const qbit::EventsfeedProxy&, bool, bool)));
	}
}

void LoadFollowersByBuzzerCommand::ready(qbit::EventsfeedPtr /*base*/, qbit::EventsfeedPtr local) {
	//
	if (eventsfeedModel_) {
		emit dataReady(qbit::EventsfeedProxy(local), more_, merge_);
	}
}

LoadTransactionCommand::LoadTransactionCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::LoadTransactionCommand::instance(
		lClient->getComposer(),
		boost::bind(&LoadTransactionCommand::done, this, boost::placeholders::_1, boost::placeholders::_2)
	);
}

LoadEntityCommand::LoadEntityCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::LoadEntityCommand::instance(
		lClient->getComposer(),
		boost::bind(&LoadEntityCommand::done, this, boost::placeholders::_1, boost::placeholders::_2)
	);
}

QString LoadEntityCommand::extractAddress() {
	//
	if (tx_) {
		//
		qbit::PKey lPKey;
		qbit::Transaction::Out& lOut = (*tx_->out().begin());
		qbit::VirtualMachine lVM(lOut.destination());
		lVM.execute();

		if (lVM.getR(qbit::qasm::QD0).getType() != qbit::qasm::QNONE) {
			lPKey.set<unsigned char*>(lVM.getR(qbit::qasm::QD0).begin(), lVM.getR(qbit::qasm::QD0).end());

			return QString::fromStdString(lPKey.toString());
		}
	}

	return QString();
}

SendToAddressCommand::SendToAddressCommand(QObject* /*parent*/) : QObject() {
	//
}

//
// LoadConversationsCommand
//

LoadConversationsCommand::LoadConversationsCommand(QObject* /*parent*/) : QObject() {}

LoadConversationsCommand::~LoadConversationsCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadConversationsCommand::prepare() {
	//
	if (!command_ && conversationsModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadConversationsCommand>(
			qbit::LoadConversationsCommand::instance(
				lClient->getBuzzerComposer(),
				conversationsModel_->conversations(),
				boost::bind(&LoadConversationsCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadConversationsCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::ConversationsfeedProxy&, bool, bool)), conversationsModel_, SLOT(feedSlot(const qbit::ConversationsfeedProxy&, bool, bool)));
	}
}

void LoadConversationsCommand::ready(qbit::ConversationsfeedPtr /*base*/, qbit::ConversationsfeedPtr local) {
	//
	if (conversationsModel_) {
		emit dataReady(qbit::ConversationsfeedProxy(local), more_, merge_);
	}
}

//
// CreateConversationCommand
//

CreateConversationCommand::CreateConversationCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	//
	command_ = std::static_pointer_cast<qbit::CreateBuzzerConversationCommand>(
		qbit::CreateBuzzerConversationCommand::instance(
			lClient->getBuzzerComposer(),
			boost::bind(&CreateConversationCommand::done, this, boost::placeholders::_1)
		)
	);
}

//
// AcceptConversationCommand
//

AcceptConversationCommand::AcceptConversationCommand(QObject* /*parent*/) : QObject() {}

void AcceptConversationCommand::prepare() {
	//
	if (!command_ && conversationsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::AcceptConversationCommand>(
			qbit::AcceptConversationCommand::instance(
				lClient->getBuzzerComposer(),
				conversationsfeedModel_->conversations(),
				boost::bind(&AcceptConversationCommand::done, this, boost::placeholders::_1)
			)
		);
	}
}

//
// DeclineConversationCommand
//

DeclineConversationCommand::DeclineConversationCommand(QObject* /*parent*/) : QObject() {}

void DeclineConversationCommand::prepare() {
	//
	if (!command_ && conversationsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::DeclineConversationCommand>(
			qbit::DeclineConversationCommand::instance(
				lClient->getBuzzerComposer(),
				conversationsfeedModel_->conversations(),
				boost::bind(&DeclineConversationCommand::done, this, boost::placeholders::_1)
			)
		);
	}
}

//
// DecryptMessageBodyCommand
//

DecryptMessageBodyCommand::DecryptMessageBodyCommand(QObject* /*parent*/) : QObject() {}

void DecryptMessageBodyCommand::prepare() {
	//
	if (!command_ && conversationsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::DecryptMessageBodyCommand>(
			qbit::DecryptMessageBodyCommand::instance(
				lClient->getBuzzerComposer(),
				conversationsfeedModel_->conversations(),
				boost::bind(&DecryptMessageBodyCommand::done, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3)
			)
		);
	}
}

//
// LoadConversationMessagesCommand
//

LoadConversationMessagesCommand::LoadConversationMessagesCommand(QObject* /*parent*/) : QObject() {}

LoadConversationMessagesCommand::~LoadConversationMessagesCommand() {
	//
	if (command_) {
		disconnect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), 0, 0);
		command_ = nullptr;
	}
}

void LoadConversationMessagesCommand::prepare() {
	//
	if (!command_ && buzzfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		uint256 lRootId; lRootId.setHex(conversationId_.toStdString());
		// interlink & receive realtime updates
		buzzfeedModel_->buzzfeed()->setRootBuzzId(lRootId);
		lClient->getBuzzerComposer()->buzzer()->setConversation(buzzfeedModel_->buzzfeed());
		// prepare command
		command_ = std::static_pointer_cast<qbit::LoadMessagesCommand>(
			qbit::LoadMessagesCommand::instance(
				lClient->getBuzzerComposer(),
				buzzfeedModel_->buzzfeed(),
				boost::bind(&LoadConversationMessagesCommand::ready, this, boost::placeholders::_1, boost::placeholders::_2),
				boost::bind(&LoadConversationMessagesCommand::done, this, boost::placeholders::_1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool, bool)), buzzfeedModel_, SLOT(feedSlot(const qbit::BuzzfeedProxy&, bool, bool)));
	}
}

void LoadConversationMessagesCommand::ready(qbit::BuzzfeedPtr /*base*/, qbit::BuzzfeedPtr local) {
	//
	if (buzzfeedModel_) {
		emit dataReady(qbit::BuzzfeedProxy(local), more_, merge_);
	}
}

//
// ConversationMessageCommand
//

ConversationMessageCommand::ConversationMessageCommand(QObject* /*parent*/) : QObject() {
}

void ConversationMessageCommand::prepare() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::CreateBuzzerMessageCommand>(
		qbit::CreateBuzzerMessageCommand::instance(
			lClient->getBuzzerComposer(),
			std::static_pointer_cast<qbit::Conversationsfeed>(lClient->getBuzzer()->conversations()),
			boost::bind(&ConversationMessageCommand::done, this, boost::placeholders::_1))
	);

	if (uploadCommand_) {
		// link command
		command_->setUploadMedia(uploadCommand_->command());
		uploadCommand_->setDone(boost::bind(&qbit::CreateBuzzerMessageCommand::mediaUploaded, command_, boost::placeholders::_1, boost::placeholders::_2));

		// link notification
		command_->setMediaUploaded(boost::bind(&ConversationMessageCommand::mediaUploaded, this, boost::placeholders::_1, boost::placeholders::_2));
	}
}

//
// DecryptBuzzerMessageCommand
//

DecryptBuzzerMessageCommand::DecryptBuzzerMessageCommand(QObject* /*parent*/) : QObject() {}

void DecryptBuzzerMessageCommand::prepare() {
	//
	if (!command_ && conversationModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::DecryptBuzzerMessageCommand>(
			qbit::DecryptBuzzerMessageCommand::instance(
				lClient->getBuzzerComposer(),
				std::static_pointer_cast<qbit::Conversationsfeed>(lClient->getBuzzer()->conversations()),
				conversationModel_->buzzfeed(),
				boost::bind(&DecryptBuzzerMessageCommand::done, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3)
			)
		);
	}
}

//
// LoadCounterpartyKeyCommand
//

LoadCounterpartyKeyCommand::LoadCounterpartyKeyCommand(QObject* /*parent*/) : QObject() {}

void LoadCounterpartyKeyCommand::prepare() {
	//
	if (!command_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadCounterpartyKeyCommand>(
			qbit::LoadCounterpartyKeyCommand::instance(
				lClient->getBuzzerComposer(),
				std::static_pointer_cast<qbit::Conversationsfeed>(lClient->getBuzzer()->conversations()),
				boost::bind(&LoadCounterpartyKeyCommand::done, this, boost::placeholders::_1)
			)
		);
	}
}
