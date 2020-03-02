// Copyright (c) 2020 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_BUZZER_HTTP_ENDPOINTS_H
#define QBIT_BUZZER_HTTP_ENDPOINTS_H
//
// allocator.h _MUST_ be included BEFORE all other
//
#include "../../allocator.h"
#include "../../ihttpcallendpoint.h"
#include "composer.h"

#include <string>

namespace qbit {

class IHttpBuzzerCallEnpoint: public IHttpCallEnpoint {
public:
	IHttpBuzzerCallEnpoint() {}
	IHttpBuzzerCallEnpoint(BuzzerComposerPtr composer) : composer_(composer) {}

protected:
	BuzzerComposerPtr composer_;
};

class HttpCreateBuzzer: public IHttpBuzzerCallEnpoint {
public:
	HttpCreateBuzzer() {}
	HttpCreateBuzzer(BuzzerComposerPtr composer) : IHttpBuzzerCallEnpoint(composer) {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("createbuzzer"); }

	static IHttpCallEnpointPtr instance(BuzzerComposerPtr composer) {
		return std::make_shared<HttpCreateBuzzer>(composer);
	}
};

class HttpAttachBuzzer: public IHttpBuzzerCallEnpoint {
public:
	HttpAttachBuzzer() {}
	HttpAttachBuzzer(BuzzerComposerPtr composer) : IHttpBuzzerCallEnpoint(composer) {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("attachbuzzer"); }

	static IHttpCallEnpointPtr instance(BuzzerComposerPtr composer) {
		return std::make_shared<HttpAttachBuzzer>(composer);
	}
};

class HttpBuzz: public IHttpBuzzerCallEnpoint {
public:
	HttpBuzz() {}
	HttpBuzz(BuzzerComposerPtr composer) : IHttpBuzzerCallEnpoint(composer) {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("buzz"); }

	static IHttpCallEnpointPtr instance(BuzzerComposerPtr composer) {
		return std::make_shared<HttpBuzz>(composer);
	}
};

class HttpBuzzerSubscribe: public IHttpBuzzerCallEnpoint {
public:
	HttpBuzzerSubscribe() {}
	HttpBuzzerSubscribe(BuzzerComposerPtr composer) : IHttpBuzzerCallEnpoint(composer) {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("buzzersubscribe"); }

	static IHttpCallEnpointPtr instance(BuzzerComposerPtr composer) {
		return std::make_shared<HttpBuzzerSubscribe>(composer);
	}
};

class HttpBuzzerUnsubscribe: public IHttpBuzzerCallEnpoint {
public:
	HttpBuzzerUnsubscribe() {}
	HttpBuzzerUnsubscribe(BuzzerComposerPtr composer) : IHttpBuzzerCallEnpoint(composer) {}

	void process(const std::string&, const HttpRequest&, const json::Document&, HttpReply&);
	std::string method() { return std::string("buzzerunsubscribe"); }

	static IHttpCallEnpointPtr instance(BuzzerComposerPtr composer) {
		return std::make_shared<HttpBuzzerUnsubscribe>(composer);
	}
};

} // qbit

#endif