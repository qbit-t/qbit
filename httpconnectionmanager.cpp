#include "httpconnectionmanager.h"
#include "log/log.h"

#include <iostream>

using namespace qbit;

HttpConnectionManagerPtr HttpConnectionManager::instance(ISettingsPtr settings, HttpRequestHandlerPtr requestHandler) { 
	return std::make_shared<HttpConnectionManager>(settings, requestHandler); 
}
