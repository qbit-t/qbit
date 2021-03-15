#ifndef BUZZER_MOBILE_SETTINGS_H
#define BUZZER_MOBILE_SETTINGS_H

#include <QString>
#include <QList>
#include <QMap>

#include "client.h"
#include "json.h"
#include "../../isettings.h"
#include "../../state.h"
#include "../../amount.h"

namespace buzzer {

class Settings: public QObject, public qbit::ISettings
{
    Q_OBJECT
public:
	Settings(QObject *parent = nullptr): QObject(parent) {}
	virtual ~Settings() {}

	virtual void open() = 0;
	virtual void update() = 0;
	virtual void link(IClient*) = 0;

	virtual std::string dataPath() { return path_; }
	virtual uint32_t roles() { return qbit::State::PeerRoles::CLIENT; }

	int clientActivePeers() { return 32; }
	qbit::qunit_t maxFeeRate() { return qbit::QUNIT * 5; }

	qbit::ISettingsPtr shared() { return qbit::ISettingsPtr(static_cast<ISettings*>(this)); }

signals:
    void dataChanged();

protected:
	std::string path_;
};

class SettingsJSON: public Settings
{
    Q_OBJECT
public:
	SettingsJSON() { opened_ = false; path_ = getApplicationDataPath(); }
	SettingsJSON(bool daemon) { opened_ = false; daemon_ = daemon; path_ = getApplicationDataPath(); }
	~SettingsJSON() {}

	void open();
	void update();
	void link(IClient*);

	uint32_t roles() {
		return daemon_ ? qbit::State::PeerRoles::CLIENT|qbit::State::PeerRoles::DAEMON : qbit::State::PeerRoles::CLIENT;
	}

private:
    QString getSettingsFile();
	std::string getApplicationDataPath();
	std::string getApplicationLogsPath();

private:	
	IClient* client_;
    bool opened_;
	bool daemon_ = false;
};

class SettingsFactory
{
public:
	static Settings* get();
	static Settings* getDaemon();
};

} // buzzer

#endif // BUZZER_MOBILE_SETTINGS_H