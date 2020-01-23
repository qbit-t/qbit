#include "json.h"

using namespace qbit::json;

rapidjson::CrtAllocator gAllocator;

std::string Error::getError(int code) {
	switch(code) {
		case rapidjson::ParseErrorCode::kParseErrorNone: return std::string("No error.");
		case rapidjson::ParseErrorCode::kParseErrorDocumentEmpty: return std::string("The document is empty.");
		case rapidjson::ParseErrorCode::kParseErrorDocumentRootNotSingular: return std::string("The document root must not follow by other values.");
		case rapidjson::ParseErrorCode::kParseErrorValueInvalid: return std::string("Invalid value.");
		case rapidjson::ParseErrorCode::kParseErrorObjectMissName: return std::string("Missing a name for object member.");
		case rapidjson::ParseErrorCode::kParseErrorObjectMissColon: return std::string("Missing a colon after a name of object member.");
		case rapidjson::ParseErrorCode::kParseErrorObjectMissCommaOrCurlyBracket: return std::string("Missing a comma or '}' after an object member.");
		case rapidjson::ParseErrorCode::kParseErrorArrayMissCommaOrSquareBracket: return std::string("Missing a comma or ']' after an array element.");
		case rapidjson::ParseErrorCode::kParseErrorStringUnicodeEscapeInvalidHex: return std::string("Incorrect hex digit after \\u escape in string.");
		case rapidjson::ParseErrorCode::kParseErrorStringUnicodeSurrogateInvalid: return std::string("The surrogate pair in string is invalid.");
		case rapidjson::ParseErrorCode::kParseErrorStringEscapeInvalid: return std::string("Invalid escape character in string.");
		case rapidjson::ParseErrorCode::kParseErrorStringMissQuotationMark: return std::string("Missing a closing quotation mark in string.");
		case rapidjson::ParseErrorCode::kParseErrorStringInvalidEncoding: return std::string("Invalid encoding in string.");
		case rapidjson::ParseErrorCode::kParseErrorNumberTooBig: return std::string("Number too big to be stored in double.");
		case rapidjson::ParseErrorCode::kParseErrorNumberMissFraction: return std::string("Miss fraction part in number.");
		case rapidjson::ParseErrorCode::kParseErrorNumberMissExponent: return std::string("Miss exponent in number.");
		case rapidjson::ParseErrorCode::kParseErrorTermination: return std::string("Parsing was terminated.");
		case rapidjson::ParseErrorCode::kParseErrorUnspecificSyntaxError: return std::string("Unspecific syntax error.");
		default: return std::string("No error.");
	}
}

//
// Iterator
//
Value ValueMemberIterator::getValue() {
	return Value(iterator_->value, *document_);
}

std::string ValueMemberIterator::getName() {
	return std::string(iterator_->name.GetString());
}

//
// Value
//
ValueType Value::type() {
	if (isBool()) return ValueType::Bool;
	else if(isString()) return ValueType::String;
	else if (isInt()) return ValueType::Int;
	else if (isUint()) return ValueType::Uint;
	else if (isInt64()) return ValueType::Int64;
	else if (isUint64()) return ValueType::Uint64;
	else if (isDouble()) return ValueType::Double;
	else if (isObject()) return ValueType::Object;
	else if (isArray()) return ValueType::Array;
	return ValueType::Null;
}

Value Value::operator[](const std::string& name) {
	checkReference();
	return Value(value_->operator[](name), *document_);
}

Value Value::operator[](size_t index) {
	checkReference();
	return Value(value_->operator[](index), *document_);
}

bool Value::find(const std::string& name, Value& value) {
	checkReference();

	if (value_->IsNull()) return false;

	RapidJsonValue::MemberIterator lIterator = value_->FindMember(name);
	if (lIterator != value_->MemberEnd())
	{
		value.assign(&lIterator->value, document_);
		return true;
	}

	return false;
}

Value Value::addBool(const std::string& name, bool value) {
	checkReference();

	RapidJsonValue lName, lValue; lName.SetString(name, document_->GetAllocator());
	if (!isObject()) value_->SetObject();

	lValue.SetBool(value);

	return Value(value_->AddMember(lName, lValue, document_->GetAllocator()), *document_);
}

Value Value::addString(const std::string& name, const std::string& value) {
	checkReference();

	RapidJsonValue lName, lValue; lName.SetString(name, document_->GetAllocator());
	if (!isObject()) value_->SetObject();

	lValue.SetString(value, document_->GetAllocator());

	return Value(value_->AddMember(lName, lValue, document_->GetAllocator()), *document_);
}

Value Value::addInt(const std::string& name, int value) {
	checkReference();

	RapidJsonValue lName, lValue; lName.SetString(name, document_->GetAllocator());
	if (!isObject()) value_->SetObject();

	lValue.SetInt(value);

	return Value(value_->AddMember(lName, lValue, document_->GetAllocator()), *document_);
}

Value Value::addUInt(const std::string& name, unsigned int value) {
	checkReference();

	RapidJsonValue lName, lValue; lName.SetString(name, document_->GetAllocator());
	if (!isObject()) value_->SetObject();

	lValue.SetUint(value);

	return Value(value_->AddMember(lName, lValue, document_->GetAllocator()), *document_);
}

Value Value::addInt64(const std::string& name, int64_t value) {
	checkReference();

	RapidJsonValue lName, lValue; lName.SetString(name, document_->GetAllocator());
	if (!isObject()) value_->SetObject();

	lValue.SetInt64(value);

	return Value(value_->AddMember(lName, lValue, document_->GetAllocator()), *document_);
}

Value Value::addUInt64(const std::string& name, uint64_t value) {
	checkReference();

	RapidJsonValue lName, lValue; lName.SetString(name, document_->GetAllocator());
	if (!isObject()) value_->SetObject();

	lValue.SetUint64(value);

	return Value(value_->AddMember(lName, lValue, document_->GetAllocator()), *document_);
}

Value Value::addDouble(const std::string& name, double value) {
	checkReference();

	RapidJsonValue lName, lValue; lName.SetString(name, document_->GetAllocator());
	if (!isObject()) value_->SetObject();

	lValue.SetDouble(value);

	return Value(value_->AddMember(lName, lValue, document_->GetAllocator()), *document_);
}

Value Value::addObject(const std::string& name) {
	checkReference();

	RapidJsonValue lName, lValue;
	lName.SetString(name, document_->GetAllocator()); lValue.SetObject();
	value_->AddMember(lName, lValue, document_->GetAllocator());

	return Value(value_->FindMember(name)->value, *document_);
}

Value Value::addArray(const std::string& name) {
	checkReference();

	RapidJsonValue lName, lValue;
	lName.SetString(name, document_->GetAllocator());
	lValue.SetArray();
	value_->AddMember(lName, lValue, document_->GetAllocator());

	return Value(value_->FindMember(name)->value, *document_);
}

Value Value::newArrayItem() {
	checkReference();

	RapidJsonValue lValue;
	if (!isArray()) value_->SetArray();
	value_->PushBack(lValue, document_->GetAllocator());

	return Value(value_->operator[](value_->Size()-1), *document_);
}

void Value::makeArrayItem(Value& value) {
}

Value Value::newItem() {
	checkReference();

	RapidJsonValue lName("__raw__"), lValue;
	value_->AddMember(lName, lValue, document_->GetAllocator());
	
	RapidJsonValue::MemberIterator lMember = value_->FindMember("__raw__");
	lMember->name.SetNull();

	return Value(lMember->value, *document_);
}

std::string Value::toString() {
	std::string lData;
	writeToString(lData);
	return lData;
}

void Value::writeToString(std::string& stream) {
	checkReference();

	RapidJsonUTF8StringBuffer lBuffer;
	RapidJsonOutputStream lOutStream(lBuffer, false);
	RapidJsonUTF8PrettyWriter lWriter(lOutStream);
	value_->Accept(lWriter);
	stream.insert(0, lBuffer.GetString());
}

void Value::writeToStream(std::vector<unsigned char>& stream) {
	checkReference();

	RapidJsonUTF8StringBuffer lBuffer;
	RapidJsonOutputStream lOutStream(lBuffer, false);
	RapidJsonUTF8PrettyWriter lWriter(lOutStream);
	value_->Accept(lWriter);
	stream.resize(lBuffer.GetSize());
	memcpy(&stream[0], lBuffer.GetString(), lBuffer.GetSize());
}

// Iteration
ValueMemberIterator Value::begin() {
	checkReference();

	return ValueMemberIterator(value_->MemberBegin(), *document_);
}

ValueMemberIterator Value::end() {
	checkReference();

	return ValueMemberIterator(value_->MemberEnd(), *document_);
}

void Value::clone(Document& other) {
	other.document_.CopyFrom(*value_, other.document_.GetAllocator());
}

//
// Document
//

bool Document::loadFromString(const std::string& source) {
	return !(document_.Parse<0, rapidjson::UTF8<> >(source).HasParseError());
}

bool Document::loadFromStream(const std::vector<unsigned char>& source) {
	return !(document_.Parse<0, rapidjson::UTF8<> >((char*)&source[0], source.size()).HasParseError());
}

bool Document::loadFromFile(const std::string& source) {
	return true;
}

bool Document::hasErrors() {
	return document_.HasParseError();
}

std::string Document::lastError() {
	return Error::getError(document_.GetParseError());
}

void Document::writeToString(std::string& stream) {
	Value::writeToString(stream);
}

void Document::writeToStream(std::vector<unsigned char>& stream) {
	Value::writeToStream(stream);
}

Value Document::operator[](const std::string& name) {
	return Value(document_[name], document_);
}

Value Document::addObject(const std::string& name) {
	return Value::addObject(name);
}

Value Document::addArray(const std::string& name) {
	return Value::addArray(name);
}

void Document::clone(Document& other) {
	other.document_.CopyFrom(document_, other.document_.GetAllocator());
}
