#include "../include/structs.hpp"
#include "../include/cjson.hpp"
#include "../include/log.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <cjson/cJSON.h>

iCjson::iCjson() {

}

unsigned char cJsonDerived::jsonIsValid(const char* json) {
    if (json == nullptr) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Provided json is nullptr");

        return 0;

    }
    
    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");

    const char* ptrErr = nullptr;
    cJSON* j = cJSON_ParseWithOpts(json, &ptrErr, 1);
    if (j == nullptr) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Parsing failed, printing error location");

        size_t errOffset = ptrErr - json;

        size_t startOffset = (errOffset > 10) ? errOffset - 10 : 0;
        size_t offsetLen = (errOffset + 10ULL < strlen(json)) ? 10 : strlen(json) - startOffset;
        char errMsg[1024];
        sprintf(
            errMsg,
            "Error occurred on parsing the json configuration file, the error occurred on:\n%.*s\nexiting...\n",
            (int)offsetLen,
            json + startOffset
        );

        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg);

        return 0;

    }

    cJSON_Delete(j);

    return 1;

}

applicationConfig* cJsonDerived::applicationConfigParse(const char* json) {
    if (!jsonIsValid(json)) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "The json file is not valid.");

        return nullptr;

    }

    cJSON* jsonParser = cJSON_Parse(json);
    if (!jsonParser) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Variable jsonParser is a nullptr");

        return nullptr;

    }

    if (
        cJSON_HasObjectItem(jsonParser, "adminChatIds") &&
        cJSON_IsArray(cJSON_GetObjectItem(jsonParser, "adminChatIds")) &&
        cJSON_HasObjectItem(jsonParser, "channels2JoinChatIds") &&
        cJSON_IsArray(cJSON_GetObjectItem(jsonParser, "channels2JoinChatIds")) &&
        cJSON_HasObjectItem(jsonParser, "channels2JoinUrls") &&
        cJSON_IsArray(cJSON_GetObjectItem(jsonParser, "channels2JoinUrls")) &&
        cJSON_HasObjectItem(jsonParser, "privateChannelChatId") &&
        cJSON_HasObjectItem(jsonParser, "donateWalletAddress")
       ) {
        struct applicationConfig* aConfig = (struct applicationConfig*)malloc(sizeof(struct applicationConfig));
        if (!aConfig) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate the applicationConfig struct");

            return nullptr;

        }

        aConfig->adminChatIds = new std::vector<int64_t>();
        if (!aConfig->adminChatIds) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate adminChatIds vector inside the applicationConfig struct");

            free(aConfig);

            return nullptr;

        }

        aConfig->channels2JoinChatIds = new std::vector<int64_t>();
        if (!aConfig->channels2JoinChatIds) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate channels2JoinChatIds vector inside the applicationConfig struct");

            delete aConfig->adminChatIds;
            free(aConfig);

            return nullptr;

        }

        aConfig->channels2JoinUrls = new std::vector<const char*>();
        if (!aConfig->channels2JoinUrls) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate channels2JoinUrls vector inside the applicationConfig struct");

            delete aConfig->adminChatIds;
            delete aConfig->channels2JoinChatIds;
            free(aConfig);

            return nullptr;

        }
        
        aConfig->donateWalletAddress = (char*)malloc((strlen(cJSON_GetObjectItem(jsonParser, "donateWalletAddress")->valuestring) + 1) * sizeof(char));
        if (!aConfig->donateWalletAddress) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate donateWalletAddress string inside the applicationConfig struct");

            delete aConfig->adminChatIds;
            delete aConfig->channels2JoinChatIds;
            delete aConfig->channels2JoinUrls;
            free(aConfig);

            return nullptr;

        }

        cJSON* adminsArr = cJSON_GetObjectItem(jsonParser, "adminChatIds");
        for (int i = 0; i < cJSON_GetArraySize(adminsArr); i++) {
            aConfig->adminChatIds->emplace_back(strtoll(cJSON_GetArrayItem(adminsArr, i)->valuestring, NULL, 10));

        }

        cJSON* channels2JoinChatIdsArr = cJSON_GetObjectItem(jsonParser, "channels2JoinChatIds");
        for (int i = 0; i < cJSON_GetArraySize(channels2JoinChatIdsArr); i++) {
            aConfig->channels2JoinChatIds->emplace_back(strtoll(cJSON_GetArrayItem(channels2JoinChatIdsArr, i)->valuestring, NULL, 10));

        }

        cJSON* channels2JoinUrlsArr = cJSON_GetObjectItem(jsonParser, "channels2JoinUrls");
        for (int i = 0; i < cJSON_GetArraySize(channels2JoinUrlsArr); i++) {
            const char* url = cJSON_GetArrayItem(channels2JoinUrlsArr, i)->valuestring;
            char* urlCopy = (char*)malloc((strlen(url) + 1) * sizeof(char));
            strncpy(urlCopy, url, strlen(url));
            urlCopy[strlen(url)] = '\0';
            aConfig->channels2JoinUrls->emplace_back(urlCopy);

        }

        aConfig->privateChannelChatId = strtoll(cJSON_GetObjectItem(jsonParser, "privateChannelChatId")->valuestring, NULL, 10);

        const char* walletAddr = cJSON_GetObjectItem(jsonParser, "donateWalletAddress")->valuestring;
        char* walletAddrCopy = (char*)malloc((strlen(walletAddr) + 1) * sizeof(char));
        strncpy(walletAddrCopy, walletAddr, strlen(walletAddr));
        walletAddrCopy[strlen(walletAddr)] = '\0';
        aConfig->donateWalletAddress = walletAddrCopy;
        walletAddr = nullptr;

        cJSON_Delete(jsonParser);

        return aConfig;

    } else {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "An element is not present in the json file");

        cJSON_Delete(jsonParser);

        return nullptr;

    }

}

