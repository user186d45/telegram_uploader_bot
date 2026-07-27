#include "../include/passgen.h"
#include "../include/log.hpp"

#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <cjson/cJSON.h>

unsigned char generateAdminPass() {
    iLog* l = new log(1);
    if (!l) {
        fprintf(stderr, "Error occurred on iLog instance memory allocation");

        return 0;

    }

    FILE* fp = popen("head -c 15 /dev/urandom | base64", "r");
    if (!fp) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Can't open terminal pipe for random password read");

        delete l;
        return 0;

    }

    char buffer[256];
    char password[64];
    password[0] = '\0';
    size_t passLen = 0;

    while (fgets(buffer, sizeof(buffer), fp)) {
        size_t chunkLen = strlen(buffer);
        if (passLen + chunkLen < sizeof(password)) {
            memcpy(password + passLen, buffer, chunkLen);
            passLen += chunkLen;

        }

    }

    while (passLen > 0 && (password[passLen - 1] == '\n' || password[passLen - 1] == '\r')) {
        password[--passLen] = '\0';

    }

    if (passLen > 40) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Generated password exceeds maximum length");

        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, password);

        pclose(fp);
        delete l;

        return 0;

    }

    pclose(fp);

    fp = fopen("config.json", "r");
    if (!fp) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot open config.json for reading");

        delete l;
        return 0;

    }

    size_t totalReadSize = 0;
    size_t currentReadSize = 0;
    char b[256];
    size_t rawJsonPtrSize = 1024 * 1024 * sizeof(char);
    char* rawJson = (char*)calloc(1, rawJsonPtrSize);
    if (!rawJson) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate initial buffer for config read");

        fclose(fp);
        delete l;

        return 0;

    }

    while ((currentReadSize = fread(b, sizeof(char), sizeof(b), fp))) {
        totalReadSize += currentReadSize;
        if (totalReadSize + sizeof(b) > rawJsonPtrSize) {
            size_t newSize = rawJsonPtrSize * 2;
            char* temp = (char*)realloc(rawJson, newSize);
            if (!temp) {
                l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate more space for config read");

                free(rawJson);
                fclose(fp);
                delete l;

                return 0;

            }
            rawJson = temp;
            rawJsonPtrSize = newSize;

        }

        memcpy(rawJson + totalReadSize - currentReadSize, b, currentReadSize);

    }

    fclose(fp);

    size_t jsonLen = strlen(rawJson) + 1;
    char* shrunk = (char*)realloc(rawJson, jsonLen * sizeof(char));
    if (!shrunk) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot resize the rawJson");

        free(rawJson);
        delete l;

        return 0;

    }
    rawJson = shrunk;

    const char* ptrErr = nullptr;
    cJSON* jsonParser = cJSON_ParseWithOpts(rawJson, &ptrErr, 1);
    if (jsonParser == nullptr) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Parsing failed, printing error location");

        size_t errOffset = ptrErr - rawJson;

        size_t startOffset = (errOffset > 10) ? errOffset - 10 : 0;
        size_t offsetLen = (errOffset + 10ULL < strlen(rawJson)) ? 10 : strlen(rawJson) - startOffset;
        char errMsg[1024];
        sprintf(
            errMsg,
            "Error occurred on parsing the json configuration file, the error occurred at:\n%.*s\nexiting...\n",
            (int)offsetLen,
            rawJson + startOffset
        );

        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg);

        free(rawJson);
        delete l;

        return 0;

    }

    if (
        cJSON_HasObjectItem(jsonParser, "botApiKey") &&
        cJSON_HasObjectItem(jsonParser, "Password") &&
        cJSON_HasObjectItem(jsonParser, "channels2JoinChatIds") &&
        cJSON_IsArray(cJSON_GetObjectItem(jsonParser, "channels2JoinChatIds")) &&
        cJSON_HasObjectItem(jsonParser, "channels2JoinUrls") &&
        cJSON_IsArray(cJSON_GetObjectItem(jsonParser, "channels2JoinUrls")) &&
        cJSON_HasObjectItem(jsonParser, "privateChannelChatId") &&
        cJSON_HasObjectItem(jsonParser, "donateWalletAddress") &&
        cJSON_HasObjectItem(jsonParser, "Messages") &&
        cJSON_IsObject(cJSON_GetObjectItem(jsonParser, "Messages"))
       ) {
        cJSON_SetValuestring(cJSON_GetObjectItem(jsonParser, "Password"), password);

        char* modifiedJson = cJSON_PrintUnformatted(jsonParser);
        cJSON_Delete(jsonParser);
        free(rawJson);

        if (!modifiedJson) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to serialize modified JSON");

            delete l;

            return 0;

        }

        fp = fopen("config.json", "w");
        if (!fp) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot open config.json for writing");

            free(modifiedJson);
            delete l;

            return 0;

        }

        size_t writeLen = strlen(modifiedJson);
        if (fwrite(modifiedJson, sizeof(char), writeLen, fp) != writeLen) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to write modified JSON to config.json");

            fclose(fp);
            free(modifiedJson);
            delete l;

            return 0;

        }

        fclose(fp);
        free(modifiedJson);

    } else {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "An element is not present in the json file");

        free(rawJson);

        cJSON_Delete(jsonParser);

        delete l;

        return 0;

    }

    fp = fopen("Pass.txt", "w");
    if (!fp) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Can't open Pass.txt for write");

        delete l;

        return 0;

    }

    if (fwrite(password, sizeof(char), passLen, fp) != passLen) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Write function returned unexpected value");

        fclose(fp);
        delete l;

        return 0;

    }

    fclose(fp);
    delete l;

    return 1;

}
