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
    if (!l) {
        return 0;

    }

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
        cJSON_HasObjectItem(jsonParser, "botApiKey") &&
        cJSON_HasObjectItem(jsonParser, "Password") &&
        cJSON_HasObjectItem(jsonParser, "channels2JoinChatIds") &&
        cJSON_IsArray(cJSON_GetObjectItem(jsonParser, "channels2JoinChatIds")) &&
        cJSON_HasObjectItem(jsonParser, "channels2JoinUrls") &&
        cJSON_IsArray(cJSON_GetObjectItem(jsonParser, "channels2JoinUrls")) &&
        cJSON_HasObjectItem(jsonParser, "privateChannelChatId") &&
        cJSON_HasObjectItem(jsonParser, "Messages") &&
        cJSON_IsObject(cJSON_GetObjectItem(jsonParser, "Messages"))
       ) {
        struct applicationConfig* aConfig = (struct applicationConfig*)malloc(sizeof(struct applicationConfig));
        if (!aConfig) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate the applicationConfig struct");

            cJSON_Delete(jsonParser);

            return nullptr;

        }

        const char* botApiKey = cJSON_GetObjectItem(jsonParser, "botApiKey")->valuestring;
        char* botApiKeyCopy = (char*)malloc((strlen(botApiKey) + 1) * sizeof(char));
        if (!botApiKeyCopy) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate space for botApiKey constant at application struct");

            free(aConfig);
            cJSON_Delete(jsonParser);

            return nullptr;

        }
        strncpy(botApiKeyCopy, botApiKey, strlen(botApiKey));
        botApiKeyCopy[strlen(botApiKey)] = '\0';
        aConfig->botApiKey = botApiKeyCopy;
        botApiKeyCopy = nullptr;

        const char* password = cJSON_GetObjectItem(jsonParser, "Password")->valuestring;
        char* passwordCopy = (char*)malloc((strlen(password) + 1) * sizeof(char));
        if (!passwordCopy) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate space for password constant at application struct");

            free((char*)aConfig->botApiKey);
            free(aConfig);
            cJSON_Delete(jsonParser);

            return nullptr;

        }
        strncpy(passwordCopy, password, strlen(password));
        passwordCopy[strlen(password)] = '\0';
        aConfig->password = passwordCopy;
        passwordCopy = nullptr;

        aConfig->channels2JoinChatIds = new std::vector<int64_t>();
        if (!aConfig->channels2JoinChatIds) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate channels2JoinChatIds vector inside the applicationConfig struct");

            free(aConfig);

            return nullptr;

        }

        aConfig->channels2JoinUrls = new std::vector<const char*>();
        if (!aConfig->channels2JoinUrls) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate channels2JoinUrls vector inside the applicationConfig struct");

            delete aConfig->channels2JoinChatIds;
            free(aConfig);

            return nullptr;

        }
        
        aConfig->aMessages = (struct applicationMessages*)malloc(sizeof(applicationMessages));
        if (!aConfig->aMessages) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate applicationMessages struct inside the applicationConfig struct");

            delete aConfig->channels2JoinChatIds;
            delete aConfig->channels2JoinUrls;
            free(aConfig);

            return nullptr;

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

        cJSON* messageObj = cJSON_GetObjectItem(jsonParser, "Messages");
        if (
            cJSON_HasObjectItem(messageObj, "startMessage") &&
            cJSON_HasObjectItem(messageObj, "donateMessage") &&
            cJSON_HasObjectItem(messageObj, "loginSuccess") &&
            cJSON_HasObjectItem(messageObj, "loginCancelled") &&
            cJSON_HasObjectItem(messageObj, "channelJoinMessage") &&
            cJSON_HasObjectItem(messageObj, "channelJoinConfirmText") &&
            cJSON_HasObjectItem(messageObj, "channelJoinSuccessMessage")
           ) {
            const char* startMessage = cJSON_GetObjectItem(messageObj, "startMessage")->valuestring;
            char* startMessageCopy = (char*)malloc((strlen(startMessage) + 1) * sizeof(char));
            strncpy(startMessageCopy, startMessage, strlen(startMessage));
            startMessageCopy[strlen(startMessage)] = '\0';

            aConfig->aMessages->startMessage = startMessageCopy;
            startMessageCopy = nullptr;

            const char* donateMessage = cJSON_GetObjectItem(messageObj, "donateMessage")->valuestring;
            char* donateMessageCopy = (char*)malloc((strlen(donateMessage) + 1) * sizeof(char));
            strncpy(donateMessageCopy, donateMessage, strlen(donateMessage));
            donateMessageCopy[strlen(donateMessage)] = '\0';

            aConfig->aMessages->donateMessage = donateMessageCopy;
            donateMessageCopy = nullptr;

            const char* loginSuccess = cJSON_GetObjectItem(messageObj, "loginSuccess")->valuestring;
            char* loginSuccessCopy = (char*)malloc((strlen(loginSuccess) + 1) * sizeof(char));
            strncpy(loginSuccessCopy, loginSuccess, strlen(loginSuccess));
            loginSuccessCopy[strlen(loginSuccess)] = '\0';

            aConfig->aMessages->loginSuccess = loginSuccessCopy;
            loginSuccessCopy = nullptr;

            const char* loginCancelled = cJSON_GetObjectItem(messageObj, "loginCancelled")->valuestring;
            char* loginCancelledCopy = (char*)malloc((strlen(loginCancelled) + 1) * sizeof(char));
            strncpy(loginCancelledCopy, loginCancelled, strlen(loginCancelled));
            loginCancelledCopy[strlen(loginCancelled)] = '\0';

            aConfig->aMessages->loginCancelled = loginCancelledCopy;
            loginCancelledCopy = nullptr;

            const char* channelJoinMessage = cJSON_GetObjectItem(messageObj, "channelJoinMessage")->valuestring;
            char* channelJoinMessageCopy = (char*)malloc((strlen(channelJoinMessage) + 1) * sizeof(char));
            strncpy(channelJoinMessageCopy, channelJoinMessage, strlen(channelJoinMessage));
            channelJoinMessageCopy[strlen(channelJoinMessage)] = '\0';

            aConfig->aMessages->channelJoinMessage = channelJoinMessageCopy;
            channelJoinMessageCopy = nullptr;

            const char* channelJoinConfirmText = cJSON_GetObjectItem(messageObj, "channelJoinConfirmText")->valuestring;
            char* channelJoinConfirmTextCopy = (char*)malloc((strlen(channelJoinConfirmText) + 1) * sizeof(char));
            strncpy(channelJoinConfirmTextCopy, channelJoinConfirmText, strlen(channelJoinConfirmText));
            channelJoinConfirmTextCopy[strlen(channelJoinConfirmText)] = '\0';

            aConfig->aMessages->channelJoinConfirmText = channelJoinConfirmTextCopy;
            channelJoinConfirmTextCopy = nullptr;

            const char* channelJoinSuccessMessage = cJSON_GetObjectItem(messageObj, "channelJoinSuccessMessage")->valuestring;
            char* channelJoinSuccessMessageCopy = (char*)malloc((strlen(channelJoinSuccessMessage) + 1) * sizeof(char));
            strncpy(channelJoinSuccessMessageCopy, channelJoinSuccessMessage, strlen(channelJoinSuccessMessage));
            channelJoinSuccessMessageCopy[strlen(channelJoinSuccessMessage)] = '\0';

            aConfig->aMessages->channelJoinSuccessMessage = channelJoinSuccessMessageCopy;
            channelJoinSuccessMessageCopy = nullptr;

        } else {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Required elements are not present in the message object");

            delete aConfig->channels2JoinChatIds;
            delete aConfig->channels2JoinUrls;
            free(aConfig->aMessages);
            free(aConfig);

            cJSON_Delete(jsonParser);

            return nullptr;

        }

        cJSON_Delete(jsonParser);

        return aConfig;

    } else {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "An element is not present in the json file");

        cJSON_Delete(jsonParser);

        return nullptr;

    }

}

