#ifndef BUZZER_MOBILE_ICLIENT_H
#define BUZZER_MOBILE_ICLIENT_H

#include <QList>
#include "json.h"

namespace buzzer {

class IClient
{
public:
	virtual void settingsFromJSON(qbit::json::Value&) = 0;
	virtual void settingsToJSON(qbit::json::Value&) = 0;
};

} // buzzer

#endif // BUZZER_MOBILE_ICLIENT_H
