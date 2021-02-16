#ifndef BUZZER_ERROR_H
#define BUZZER_ERROR_H

#include <QException>

namespace buzzer {

//
// common exception
//
class Exception: public QException
{
public:
    Exception(const std::string& message): message_(message) {}
    Exception(int code, const std::string& message): code_(code), message_(message) {}

    int code() { return code_; }
    const std::string& message() const { return message_; }

private:
    int code_;
    std::string message_;
};

#define NULL_REF_EXCEPTION()\
    { \
        char lMsg[512] = {0}; \
        snprintf(lMsg, sizeof(lMsg)-1, "Null reference exception at %s(), %s: %d", __FUNCTION__, __FILE__, __LINE__); \
        throw Exception(std::string(lMsg)); \
    } \

//
// special cases
//
class ModuleAlreadyExistsException : public QException
{
public:
    void raise() const { throw *this; }
    ModuleAlreadyExistsException *clone() const { return new ModuleAlreadyExistsException(*this); }
};

}

#endif // EXCEPTION_H
