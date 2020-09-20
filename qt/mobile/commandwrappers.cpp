#include "commandwrappers.h"
#include "client.h"

using namespace buzzer;

AskForQbitsCommand::AskForQbitsCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::AskForQbitsCommand::instance(lClient->getComposer(), boost::bind(&AskForQbitsCommand::done, this, _1));
}

BalanceCommand::BalanceCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::BalanceCommand::instance(lClient->getComposer(), boost::bind(&BalanceCommand::done, this, _1, _2, _3, _4));
}

LoadBuzzerTrustScoreCommand::LoadBuzzerTrustScoreCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::LoadBuzzerTrustScoreCommand::instance(
		lClient->getBuzzerComposer(),
		boost::bind(&LoadBuzzerTrustScoreCommand::done, this, _1, _2, _3, _4, _5)
	);
}

SearchEntityNamesCommand::SearchEntityNamesCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::SearchEntityNamesCommand::instance(
		lClient->getComposer(),
		boost::bind(&SearchEntityNamesCommand::done, this, _1, _2, _3)
	);
}

LoadHashTagsCommand::LoadHashTagsCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::LoadHashTagsCommand::instance(
		lClient->getBuzzerComposer(),
		false, // console output
		boost::bind(&LoadHashTagsCommand::done, this, _1, _2, _3)
	);
}

CreateBuzzerCommand::CreateBuzzerCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::CreateBuzzerCommand>(
		qbit::CreateBuzzerCommand::instance(lClient->getBuzzerComposer(), boost::bind(&CreateBuzzerCommand::done, this, _1, _2, _3))
	);

	command_->setUploadAvatar(
		qbit::cubix::UploadMediaCommand::instance(
			lClient->getCubixComposer(),
			boost::bind(&CreateBuzzerCommand::avatarUploadProgress, this, _1, _2, _3),
			boost::bind(&qbit::CreateBuzzerCommand::avatarUploaded, command_, _1, _2))
	);
	command_->setUploadHeader(
		qbit::cubix::UploadMediaCommand::instance(
			lClient->getCubixComposer(),
			boost::bind(&CreateBuzzerCommand::headerUploadProgress, this, _1, _2, _3),
			boost::bind(&qbit::CreateBuzzerCommand::headerUploaded, command_, _1, _2))
	);
}

LoadBuzzesGlobalCommand::LoadBuzzesGlobalCommand(QObject* /*parent*/) : QObject() {}

void LoadBuzzesGlobalCommand::prepare() {
	//
	if (!command_ && buzzfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadBuzzesGlobalCommand>(
			qbit::LoadBuzzesGlobalCommand::instance(
				lClient->getBuzzerComposer(),
				buzzfeedModel_->buzzfeed(),
				boost::bind(&LoadBuzzesGlobalCommand::ready, this, _1, _2),
				boost::bind(&LoadBuzzesGlobalCommand::done, this, _1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool)), buzzfeedModel_, SLOT(feedSlot(const qbit::BuzzfeedProxy&, bool)));
	}
}

void LoadBuzzesGlobalCommand::ready(qbit::BuzzfeedPtr /*base*/, qbit::BuzzfeedPtr local) {
	//
	if (buzzfeedModel_) {
		emit dataReady(qbit::BuzzfeedProxy(local), more_);
	}
}

//
// LoadBuzzfeedByBuzzCommand
//

LoadBuzzfeedByBuzzCommand::LoadBuzzfeedByBuzzCommand(QObject* /*parent*/) : QObject() {}

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
				boost::bind(&LoadBuzzfeedByBuzzCommand::ready, this, _1, _2),
				boost::bind(&LoadBuzzfeedByBuzzCommand::done, this, _1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool)), buzzfeedModel_, SLOT(feedSlot(const qbit::BuzzfeedProxy&, bool)));
	}
}

void LoadBuzzfeedByBuzzCommand::ready(qbit::BuzzfeedPtr /*base*/, qbit::BuzzfeedPtr local) {
	//
	if (buzzfeedModel_) {
		emit dataReady(qbit::BuzzfeedProxy(local), more_);
	}
}

//
// LoadBuzzfeedByBuzzerCommand
//

LoadBuzzfeedByBuzzerCommand::LoadBuzzfeedByBuzzerCommand(QObject* /*parent*/) : QObject() {}

void LoadBuzzfeedByBuzzerCommand::prepare() {
	//
	if (!command_ && buzzfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadBuzzfeedByBuzzerCommand>(
			qbit::LoadBuzzfeedByBuzzerCommand::instance(
				lClient->getBuzzerComposer(),
				buzzfeedModel_->buzzfeed(),
				boost::bind(&LoadBuzzfeedByBuzzerCommand::ready, this, _1, _2),
				boost::bind(&LoadBuzzfeedByBuzzerCommand::done, this, _1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool)), buzzfeedModel_, SLOT(feedSlot(const qbit::BuzzfeedProxy&, bool)));
	}
}

void LoadBuzzfeedByBuzzerCommand::ready(qbit::BuzzfeedPtr /*base*/, qbit::BuzzfeedPtr local) {
	//
	if (buzzfeedModel_) {
		emit dataReady(qbit::BuzzfeedProxy(local), more_);
	}
}

//
// LoadBuzzfeedByTagCommand
//

LoadBuzzfeedByTagCommand::LoadBuzzfeedByTagCommand(QObject* /*parent*/) : QObject() {}

void LoadBuzzfeedByTagCommand::prepare() {
	//
	if (!command_ && buzzfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadBuzzfeedByTagCommand>(
			qbit::LoadBuzzfeedByTagCommand::instance(
				lClient->getBuzzerComposer(),
				buzzfeedModel_->buzzfeed(),
				boost::bind(&LoadBuzzfeedByTagCommand::ready, this, _1, _2),
				boost::bind(&LoadBuzzfeedByTagCommand::done, this, _1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool)), buzzfeedModel_, SLOT(feedSlot(const qbit::BuzzfeedProxy&, bool)));
	}
}

void LoadBuzzfeedByTagCommand::ready(qbit::BuzzfeedPtr /*base*/, qbit::BuzzfeedPtr local) {
	//
	if (buzzfeedModel_) {
		emit dataReady(qbit::BuzzfeedProxy(local), more_);
	}
}

//
// LoadBuzzfeedCommand
//

LoadBuzzfeedCommand::LoadBuzzfeedCommand(QObject* /*parent*/) : QObject() {}

void LoadBuzzfeedCommand::prepare() {
	//
	if (!command_ && buzzfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadBuzzfeedCommand>(
			qbit::LoadBuzzfeedCommand::instance(
				lClient->getBuzzerComposer(),
				buzzfeedModel_->buzzfeed(),
				boost::bind(&LoadBuzzfeedCommand::ready, this, _1, _2),
				boost::bind(&LoadBuzzfeedCommand::done, this, _1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::BuzzfeedProxy&, bool)), buzzfeedModel_, SLOT(feedSlot(const qbit::BuzzfeedProxy&, bool)));
	}
}

void LoadBuzzfeedCommand::ready(qbit::BuzzfeedPtr /*base*/, qbit::BuzzfeedPtr local) {
	//
	if (buzzfeedModel_) {
		emit dataReady(qbit::BuzzfeedProxy(local), more_);
	}
}

BuzzerSubscribeCommand::BuzzerSubscribeCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::BuzzerSubscribeCommand::instance(
		lClient->getBuzzerComposer(),
		boost::bind(&BuzzerSubscribeCommand::done, this, _1)
	);
}

BuzzerUnsubscribeCommand::BuzzerUnsubscribeCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::BuzzerUnsubscribeCommand::instance(
		lClient->getBuzzerComposer(),
		boost::bind(&BuzzerUnsubscribeCommand::done, this, _1)
	);
}

DownloadMediaCommand::DownloadMediaCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::cubix::DownloadMediaCommand::instance(
			lClient->getCubixComposer(),
			boost::bind(&DownloadMediaCommand::downloadProgress, this, _1, _2),
			boost::bind(&DownloadMediaCommand::done, this, _1, _2, _3, _4, _5));
}

UploadMediaCommand::UploadMediaCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::cubix::UploadMediaCommand::instance(
			lClient->getCubixComposer(),
			boost::bind(&UploadMediaCommand::uploadProgress, this, _1, _2, _3),
			boost::bind(&UploadMediaCommand::done, this, _1, _2));
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
		boost::bind(&BuzzLikeCommand::done, this, _1)
	);
}

BuzzRewardCommand::BuzzRewardCommand(QObject* /*parent*/) : QObject() {
	//
}

void BuzzRewardCommand::prepare() {
	if (!buzzfeedModel_) return;

	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::BuzzRewardCommand::instance(
		lClient->getBuzzerComposer(),
		buzzfeedModel_->buzzfeed(),
		boost::bind(&BuzzRewardCommand::done, this, _1)
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
			boost::bind(&BuzzCommand::done, this, _1))
	);

	if (uploadCommand_) {
		// link command
		command_->setUploadMedia(uploadCommand_->command());
		uploadCommand_->setDone(boost::bind(&qbit::CreateBuzzCommand::mediaUploaded, command_, _1, _2));

		// link notification
		command_->setMediaUploaded(boost::bind(&BuzzCommand::mediaUploaded, this, _1, _2));
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
			boost::bind(&ReBuzzCommand::done, this, _1))
	);

	if (uploadCommand_) {
		// link command
		command_->setUploadMedia(uploadCommand_->command());
		uploadCommand_->setDone(boost::bind(&qbit::CreateReBuzzCommand::mediaUploaded, command_, _1, _2));

		// link notification
		command_->setMediaUploaded(boost::bind(&ReBuzzCommand::mediaUploaded, this, _1, _2));
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
			boost::bind(&ReplyCommand::done, this, _1))
	);

	if (uploadCommand_) {
		// link command
		command_->setUploadMedia(uploadCommand_->command());
		uploadCommand_->setDone(boost::bind(&qbit::CreateBuzzReplyCommand::mediaUploaded, command_, _1, _2));

		// link notification
		command_->setMediaUploaded(boost::bind(&ReplyCommand::mediaUploaded, this, _1, _2));
	}
}

BuzzerEndorseCommand::BuzzerEndorseCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::BuzzerEndorseCommand::instance(
		lClient->getBuzzerComposer(),
		boost::bind(&BuzzerEndorseCommand::done, this, _1)
	);
}

BuzzerMistrustCommand::BuzzerMistrustCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::BuzzerMistrustCommand::instance(
		lClient->getBuzzerComposer(),
		boost::bind(&BuzzerMistrustCommand::done, this, _1)
	);
}

BuzzSubscribeCommand::BuzzSubscribeCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::BuzzSubscribeCommand>(
		qbit::BuzzSubscribeCommand::instance(
			lClient->getBuzzerComposer(),
			boost::bind(&BuzzSubscribeCommand::done, this, _1))
	);
}

BuzzUnsubscribeCommand::BuzzUnsubscribeCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::BuzzUnsubscribeCommand>(
		qbit::BuzzUnsubscribeCommand::instance(
			lClient->getBuzzerComposer(),
			boost::bind(&BuzzUnsubscribeCommand::done, this, _1))
	);
}

LoadBuzzerInfoCommand::LoadBuzzerInfoCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = std::static_pointer_cast<qbit::LoadBuzzerInfoCommand>(
			qbit::LoadBuzzerInfoCommand::instance(
				lClient->getBuzzerComposer(),
				boost::bind(&LoadBuzzerInfoCommand::done, this, _1, _2, _3, _4))
	);
}

//
// LoadEndorsementsByBuzzerCommand
//

LoadEndorsementsByBuzzerCommand::LoadEndorsementsByBuzzerCommand(QObject* /*parent*/) : QObject() {}

void LoadEndorsementsByBuzzerCommand::prepare() {
	//
	if (!command_ && eventsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadEndorsementsByBuzzerCommand>(
			qbit::LoadEndorsementsByBuzzerCommand::instance(
				lClient->getBuzzerComposer(),
				eventsfeedModel_->eventsfeed(),
				boost::bind(&LoadEndorsementsByBuzzerCommand::ready, this, _1, _2),
				boost::bind(&LoadEndorsementsByBuzzerCommand::done, this, _1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::EventsfeedProxy&, bool)), eventsfeedModel_, SLOT(feedSlot(const qbit::EventsfeedProxy&, bool)));
	}
}

void LoadEndorsementsByBuzzerCommand::ready(qbit::EventsfeedPtr /*base*/, qbit::EventsfeedPtr local) {
	//
	if (eventsfeedModel_) {
		emit dataReady(qbit::EventsfeedProxy(local), more_);
	}
}

//
// LoadMistrustsByBuzzerCommand
//

LoadMistrustsByBuzzerCommand::LoadMistrustsByBuzzerCommand(QObject* /*parent*/) : QObject() {}

void LoadMistrustsByBuzzerCommand::prepare() {
	//
	if (!command_ && eventsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadMistrustsByBuzzerCommand>(
			qbit::LoadMistrustsByBuzzerCommand::instance(
				lClient->getBuzzerComposer(),
				eventsfeedModel_->eventsfeed(),
				boost::bind(&LoadMistrustsByBuzzerCommand::ready, this, _1, _2),
				boost::bind(&LoadMistrustsByBuzzerCommand::done, this, _1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::EventsfeedProxy&, bool)), eventsfeedModel_, SLOT(feedSlot(const qbit::EventsfeedProxy&, bool)));
	}
}

void LoadMistrustsByBuzzerCommand::ready(qbit::EventsfeedPtr /*base*/, qbit::EventsfeedPtr local) {
	//
	if (eventsfeedModel_) {
		emit dataReady(qbit::EventsfeedProxy(local), more_);
	}
}

//
// LoadEventsfeedCommand
//

LoadEventsfeedCommand::LoadEventsfeedCommand(QObject* /*parent*/) : QObject() {}

void LoadEventsfeedCommand::prepare() {
	//
	if (!command_ && eventsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadEventsfeedCommand>(
			qbit::LoadEventsfeedCommand::instance(
				lClient->getBuzzerComposer(),
				eventsfeedModel_->eventsfeed(),
				boost::bind(&LoadEventsfeedCommand::ready, this, _1, _2),
				boost::bind(&LoadEventsfeedCommand::done, this, _1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::EventsfeedProxy&, bool)), eventsfeedModel_, SLOT(feedSlot(const qbit::EventsfeedProxy&, bool)));
	}
}

void LoadEventsfeedCommand::ready(qbit::EventsfeedPtr /*base*/, qbit::EventsfeedPtr local) {
	//
	if (eventsfeedModel_) {
		emit dataReady(qbit::EventsfeedProxy(local), more_);
	}
}

//
// LoadFollowingByBuzzerCommand
//

LoadFollowingByBuzzerCommand::LoadFollowingByBuzzerCommand(QObject* /*parent*/) : QObject() {}

void LoadFollowingByBuzzerCommand::prepare() {
	//
	if (!command_ && eventsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadSubscriptionsByBuzzerCommand>(
			qbit::LoadSubscriptionsByBuzzerCommand::instance(
				lClient->getBuzzerComposer(),
				eventsfeedModel_->eventsfeed(),
				boost::bind(&LoadFollowingByBuzzerCommand::ready, this, _1, _2),
				boost::bind(&LoadFollowingByBuzzerCommand::done, this, _1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::EventsfeedProxy&, bool)), eventsfeedModel_, SLOT(feedSlot(const qbit::EventsfeedProxy&, bool)));
	}
}

void LoadFollowingByBuzzerCommand::ready(qbit::EventsfeedPtr /*base*/, qbit::EventsfeedPtr local) {
	//
	if (eventsfeedModel_) {
		emit dataReady(qbit::EventsfeedProxy(local), more_);
	}
}

//
// LoadFollowersByBuzzerCommand
//

LoadFollowersByBuzzerCommand::LoadFollowersByBuzzerCommand(QObject* /*parent*/) : QObject() {}

void LoadFollowersByBuzzerCommand::prepare() {
	//
	if (!command_ && eventsfeedModel_) {
		Client* lClient = static_cast<Client*>(gApplication->getClient());

		command_ = std::static_pointer_cast<qbit::LoadFollowersByBuzzerCommand>(
			qbit::LoadFollowersByBuzzerCommand::instance(
				lClient->getBuzzerComposer(),
				eventsfeedModel_->eventsfeed(),
				boost::bind(&LoadFollowersByBuzzerCommand::ready, this, _1, _2),
				boost::bind(&LoadFollowersByBuzzerCommand::done, this, _1)
			)
		);

		connect(this, SIGNAL(dataReady(const qbit::EventsfeedProxy&, bool)), eventsfeedModel_, SLOT(feedSlot(const qbit::EventsfeedProxy&, bool)));
	}
}

void LoadFollowersByBuzzerCommand::ready(qbit::EventsfeedPtr /*base*/, qbit::EventsfeedPtr local) {
	//
	if (eventsfeedModel_) {
		emit dataReady(qbit::EventsfeedProxy(local), more_);
	}
}

LoadTransactionCommand::LoadTransactionCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::LoadTransactionCommand::instance(
		lClient->getComposer(),
		boost::bind(&LoadTransactionCommand::done, this, _1, _2)
	);
}

LoadEntityCommand::LoadEntityCommand(QObject* /*parent*/) : QObject() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	command_ = qbit::LoadEntityCommand::instance(
		lClient->getComposer(),
		boost::bind(&LoadEntityCommand::done, this, _1, _2)
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
