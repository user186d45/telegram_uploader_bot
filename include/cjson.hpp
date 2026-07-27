#ifndef CJSON_TELEGRAM_UPLOADER_HPP
#define CJSON_TELEGRAM_UPLOADER_HPP

#include "../include/structs.hpp"
#include "../include/log.hpp"

#include <cjson/cJSON.h>

class iCjson {
public:
    iCjson();
    virtual ~iCjson() = default;

    virtual applicationConfig* applicationConfigParse(const char* json) = 0;

    iLog* l = nullptr;

};

class cJsonDerived : public iCjson {
public:
    applicationConfig* applicationConfigParse(const char* json) override;

private:
    unsigned char jsonIsValid(const char* json);

};

#endif

