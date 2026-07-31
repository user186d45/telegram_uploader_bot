#include "../include/bot.hpp"
#include "../include/sqlite.hpp"

#define PCRE2_CODE_UNIT_WIDTH 8

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <pcre2.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <csignal>

// Appends a "Start <botName>" URL button row for every target bot database the
// user is missing from. DB_ERROR aborts via SIGINT (fail-fast, like startup validation).
static void appendStartBotButtonsImpl(
        iLog* l,
        const applicationConfig* aConfig,
        TgBot::InlineKeyboardMarkup::Ptr iKeyboardM,
        int64_t userId
) {
    if (!l || !aConfig || !aConfig->botDatabases || aConfig->botDatabases->empty()) {
        return;

    }

    iTargetBotDb* targetBotDb = new targetBotDbSql();
    targetBotDb->l = l;

    for (size_t i = 0; i < aConfig->botDatabases->size(); i++) {
        const struct botInfo& info = aConfig->botDatabases->at(i);

        iTargetBotDb::dbCheckResult result = targetBotDb->isUserInDb(info.databasePath, userId);

        if (result == iTargetBotDb::dbCheckResult::DB_ERROR) {
            std::string errMsg = "Fatal: cannot read database for bot '" + std::string(info.botName ? info.botName : "?") +
                                 "', path may be incorrect in config: " + std::string(info.databasePath ? info.databasePath : "null");
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

            delete targetBotDb;
            std::raise(SIGINT);

            return;

        }

        if (result == iTargetBotDb::dbCheckResult::NOT_FOUND) {
            if (!info.botName) {
                continue;

            }

            const char* urlName = info.botName;
            if (urlName[0] == '@') {
                urlName++;

            }

            if (urlName[0] == '\0') {
                continue;

            }

            std::vector<TgBot::InlineKeyboardButton::Ptr> row;

            TgBot::InlineKeyboardButton::Ptr botBtn(new TgBot::InlineKeyboardButton);
            std::string btnText = "Start " + std::string(info.botName);
            std::string btnUrl = "https://t.me/" + std::string(urlName);
            botBtn->text = btnText;
            botBtn->url = btnUrl;
            row.push_back(botBtn);

            iKeyboardM->inlineKeyboard.push_back(row);

        }

    }

    delete targetBotDb;

}

// True only when the user is present in every target bot database (or none
// configured). DB_ERROR aborts via SIGINT (fail-fast) and the gate fails closed.
static bool isUserInAllBotsImpl(iLog* l, const applicationConfig* aConfig, int64_t userId) {
    if (!l || !aConfig || !aConfig->botDatabases || aConfig->botDatabases->empty()) {
        return true;

    }

    iTargetBotDb* targetBotDb = new targetBotDbSql();
    targetBotDb->l = l;

    for (size_t i = 0; i < aConfig->botDatabases->size(); i++) {
        const struct botInfo& info = aConfig->botDatabases->at(i);

        iTargetBotDb::dbCheckResult result = targetBotDb->isUserInDb(info.databasePath, userId);

        if (result == iTargetBotDb::dbCheckResult::DB_ERROR) {
            std::string errMsg = "Fatal: cannot read database for bot '" + std::string(info.botName ? info.botName : "?") +
                                 "', path may be incorrect in config: " + std::string(info.databasePath ? info.databasePath : "null");
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

            delete targetBotDb;
            std::raise(SIGINT);

            return false;

        }

        if (result == iTargetBotDb::dbCheckResult::NOT_FOUND) {
            delete targetBotDb;

            return false;

        }

    }

    delete targetBotDb;

    return true;

}

// Join prompt keyboard order: unjoined channels, missing target bots, confirm.
// Already-joined channels and present bots are omitted.
static TgBot::InlineKeyboardMarkup::Ptr buildJoinPromptKeyboard(
        iLog* l,
        TgBot::Bot* bot,
        const applicationConfig* aConfig,
        int64_t userId
) {
    TgBot::InlineKeyboardMarkup::Ptr iKeyboardM(new TgBot::InlineKeyboardMarkup);

    if (aConfig->channels2JoinUrls && bot) {
        for (size_t i = 0; i < aConfig->channels2JoinUrls->size(); i++) {
            bool isMember = false;

            if (aConfig->channels2JoinChatIds && i < aConfig->channels2JoinChatIds->size()) {
                try {
                    TgBot::ChatMember::Ptr chatMemberPtr = bot->getApi().getChatMember(
                        aConfig->channels2JoinChatIds->at(i),
                        userId
                    );

                    isMember = strncmp(chatMemberPtr->status.c_str(), "member", 6) == 0 ||
                               strncmp(chatMemberPtr->status.c_str(), "administrator", 13) == 0 ||
                               strncmp(chatMemberPtr->status.c_str(), "creator", 7) == 0;

                } catch (...) {
                    l->logMsg(iLog::logLevel::WARNING, LOG_FUNC,
                        "getChatMember failed while building join prompt, showing channel button");
                }
            }

            if (isMember) {
                continue;

            }

            std::vector<TgBot::InlineKeyboardButton::Ptr> row;

            TgBot::InlineKeyboardButton::Ptr joinBtn(new TgBot::InlineKeyboardButton);
            joinBtn->text = aConfig->channels2JoinUrls->at(i);
            joinBtn->url = aConfig->channels2JoinUrls->at(i);
            row.push_back(joinBtn);

            iKeyboardM->inlineKeyboard.push_back(row);

        }

    }

    appendStartBotButtonsImpl(l, aConfig, iKeyboardM, userId);

    std::vector<TgBot::InlineKeyboardButton::Ptr> confirmRow;

    TgBot::InlineKeyboardButton::Ptr joinConfirmBtn(new TgBot::InlineKeyboardButton);
    joinConfirmBtn->text = aConfig->aMessages->channelJoinConfirmText;
    joinConfirmBtn->callbackData = "joinConfirm";
    confirmRow.push_back(joinConfirmBtn);

    iKeyboardM->inlineKeyboard.push_back(confirmRow);

    return iKeyboardM;

}

static void sendJoinPrompt(iLog* l, TgBot::Bot* bot, const applicationConfig* aConfig, int64_t chatId) {
    if (!l || !bot || !aConfig) {
        return;

    }

    try {
        TgBot::InlineKeyboardMarkup::Ptr iKeyboardM = buildJoinPromptKeyboard(l, bot, aConfig, chatId);

        bot->getApi().sendMessage(chatId, aConfig->aMessages->channelJoinMessage, nullptr, nullptr, iKeyboardM);

    } catch (TgBot::TgException& e) {
        std::string errMsg = "Failed to send join-channel message to user: " + std::string(e.what());
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

    } catch (...) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to send join-channel message to user");

    }

}

void iBot::updateUserInfoFromMessage(TgBot::Message::Ptr messagePtr) {
    if (!l || !uInfo || !messagePtr) {
        if (l) {
            l->logMsg(iLog::logLevel::WARNING, LOG_FUNC, "Missing required instances, skipping user update");

        }

        return;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");

    int64_t messageUserId = messagePtr->chat ? messagePtr->chat->id : (messagePtr->from ? messagePtr->from->id : 0);
    if (messageUserId == 0) {
        l->logMsg(iLog::logLevel::WARNING, LOG_FUNC, "Could not determine user ID from message");

        return;

    }
    
    iUserDatabaseSql* userDb = new userDatabaseSql();
    userDb->l = l;
    userDb->uInfo = uInfo;

    userDb->createCheckDb();

    {
        userInfo fresh{};
        fresh.userId = uInfo->userId;
        *uInfo = fresh;
    }

    if (!userDb->readUserById(messageUserId)) {
        l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "User not found in database, will create new entry");
        uInfo->userId = messageUserId;

        userDb->writeUserData(iUserDatabaseSql::userDataRW::ALL);

    } else {
        l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "User data loaded from database");

    }
    
    delete(userDb);

}

void iBot::updateUserInfoFromCallback(TgBot::CallbackQuery::Ptr cBQueryPtr) {
    if (!l || !uInfo || !cBQueryPtr) {
        if (l) {
            l->logMsg(iLog::logLevel::WARNING, LOG_FUNC, "Missing required instances, skipping user update");

        }

        return;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");
    
    int64_t callbackUserId = cBQueryPtr->from ? cBQueryPtr->from->id : 0;
    if (callbackUserId == 0) {
        l->logMsg(iLog::logLevel::WARNING, LOG_FUNC, "Could not determine user ID from callback");

        return;

    }
    
    iUserDatabaseSql* userDb = new userDatabaseSql();
    userDb->l = l;
    userDb->uInfo = uInfo;

    userDb->createCheckDb();

    {
        userInfo fresh{};
        fresh.userId = uInfo->userId;
        *uInfo = fresh;
    }

    if (!userDb->readUserById(callbackUserId)) {
        l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "User not found in database, will create new entry");
        uInfo->userId = callbackUserId;

        userDb->writeUserData(iUserDatabaseSql::userDataRW::ALL);

    } else {
        l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "User data loaded from database");

    }
    
    delete(userDb);

}

unsigned char startCommandHandler::canHandle(TgBot::Message::Ptr messagePtr) {
    return (strncmp(messagePtr->text.c_str(), "/start", 6) == 0) ? 1 : 0;

}

void startCommandHandler::handle(TgBot::Message::Ptr messagePtr) {
    updateUserInfoFromMessage(messagePtr);

    iUserDatabaseSql* userDb = new userDatabaseSql();
    userDb->l = l;
    userDb->uInfo = uInfo;
    userDb->writeUserData(iUserDatabaseSql::userDataRW::ALL);
    delete userDb;

    TgBot::InlineKeyboardMarkup::Ptr iKeyboardM(new TgBot::InlineKeyboardMarkup);
    
    std::vector<TgBot::InlineKeyboardButton::Ptr> row;

    TgBot::InlineKeyboardButton::Ptr donateBtn(new TgBot::InlineKeyboardButton);
    donateBtn->text = "Donate";
    donateBtn->callbackData = "Donate";
    row.push_back(donateBtn);

    iKeyboardM->inlineKeyboard.push_back(row);

    sendMessage(
            uInfo->userId,
            aConfig->aMessages->startMessage,
            nullptr,
            nullptr,
            iKeyboardM
    );

    if (strlen(messagePtr->text.c_str()) < 7) {
        return;

    }

    if (aConfig->channels2JoinChatIds) {
        try {
            for (size_t i = 0; i < aConfig->channels2JoinChatIds->size(); i++) {
                TgBot::ChatMember::Ptr chatMemberPtr = bot->getApi().getChatMember(
                    aConfig->channels2JoinChatIds->at(i),
                    uInfo->userId
                );

                bool isMember = strncmp(chatMemberPtr->status.c_str(), "member", 6) == 0 ||
                                strncmp(chatMemberPtr->status.c_str(), "administrator", 13) == 0 ||
                                strncmp(chatMemberPtr->status.c_str(), "creator", 7) == 0;

                if (!isMember) {
                    return;

                }

            }

        } catch (...) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Channel membership check failed in start handler");

            return;

        }

    }

    uploadDatabaseSql* uploadDb = new uploadDatabaseSql();
    if (!uploadDb) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate uploadDb");

        return;

    }

    uploadDb->l = l;

    char* secretCopy = (char*)malloc(64 * sizeof(char));
    snprintf(secretCopy, 64, "%s", messagePtr->text.substr(7).c_str());
    struct uploadInfo upInfo = {.secret = secretCopy};

    uploadDb->upInfo = &upInfo;
    uploadDb->readUploadData();
    delete uploadDb;
    free(secretCopy);

    if (!upInfo.messageId) {
        std::string msgText = "The secret is invalid, no message corresponding to the provided secret found.";
        sendMessage(
                uInfo->userId,
                msgText,
                nullptr,
                nullptr,
                nullptr
        );

        return;

    }

    try {
        forwardMessage(
                uInfo->userId,
                aConfig->privateChannelChatId,
                upInfo.messageId
        );

    } catch (...) {
        if (aConfig->aMessages->messageDeleted && strlen(aConfig->aMessages->messageDeleted) > 0) {
            try {
                sendMessage(
                        uInfo->userId,
                        aConfig->aMessages->messageDeleted,
                        nullptr,
                        nullptr,
                        nullptr
                );

            } catch (...) {
                l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to send messageDeleted notification");

            }

        }

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "startCommandHandler::handle complete");

}

unsigned char donateMsgIKHandler::canHandle(TgBot::CallbackQuery::Ptr cBQueryPtr) {
    return (strncmp(cBQueryPtr->data.c_str(), "Donate", 6) == 0) ? 1 : 0;

}

void donateMsgIKHandler::handle(TgBot::CallbackQuery::Ptr cBQueryPtr) {
    updateUserInfoFromCallback(cBQueryPtr);

    answerCallbackQuery(cBQueryPtr->id);

    sendMessage(
            uInfo->userId,
            aConfig->aMessages->donateMessage,
            nullptr,
            nullptr,
            nullptr
    );

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "donateMsgIKHandler::handle complete");

}

unsigned char getPasswordMsgHandler::canHandle(TgBot::Message::Ptr messagePtr) {
    if (!aConfig) {
        return 0;

    }

    updateUserInfoFromMessage(messagePtr);

    if (!uInfo || !aConfig->adminChatIds || !aConfig->password) {
        return 0;

    }

    for (size_t i = 0; i < aConfig->adminChatIds->size(); i++) {
        if (uInfo->userId == strtoll(aConfig->adminChatIds->at(i), NULL, 10)) {
            if (strncmp(aConfig->password, messagePtr->text.c_str(), strlen(aConfig->password)) == 0) {

                return 1;

            }

        }

    }

    return 0;

}

void getPasswordMsgHandler::handle(TgBot::Message::Ptr messagePtr) {

    iUserDatabaseSql* userDb = new userDatabaseSql();
    userDb->l = l;
    userDb->uInfo = uInfo;

    if (uInfo->cState == LOGGED_IN) {
        uInfo->cState = IDLE;

        userDb->writeUserData(iUserDatabaseSql::userDataRW::CONVERSATION_STATE);

        sendMessage(
                uInfo->userId,
                aConfig->aMessages->loginCancelled,
                nullptr,
                nullptr,
                nullptr
        );

    } else {
        uInfo->cState = LOGGED_IN;

        userDb->writeUserData(iUserDatabaseSql::userDataRW::CONVERSATION_STATE);

        sendMessage(
                uInfo->userId,
                aConfig->aMessages->loginSuccess,
                nullptr,
                nullptr,
                nullptr
        );

    }

    delete userDb;

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "getPasswordMsgHandler::handle complete");
}

unsigned char getTargetMessageMsgHandler::canHandle(TgBot::Message::Ptr messagePtr) {
    if (!uInfo) {
        return 0;

    }

    updateUserInfoFromMessage(messagePtr);

    return (uInfo->cState == conversationState::LOGGED_IN) ? 1 : 0;

}

void getTargetMessageMsgHandler::handle(TgBot::Message::Ptr messagePtr) {
    char* msgText = (char*)malloc(64 * sizeof(char));
    snprintf(msgText, 64, "/%i", messagePtr->messageId);
    try {
        bot->getApi().sendMessage(
                uInfo->userId,
                msgText,
                nullptr,
                nullptr,
                nullptr
        );

    } catch (...) {
        free(msgText);

        throw;

    }

    free(msgText);

    uInfo->cState = conversationState::GET_EDIT_MESSAGE;
    {
        iUserDatabaseSql* userDb = new userDatabaseSql();
        userDb->l = l;
        userDb->uInfo = uInfo;
        userDb->writeUserData(iUserDatabaseSql::userDataRW::CONVERSATION_STATE);
        delete userDb;

    }

}

unsigned char getMessage2EditmsgHandler::canHandle(TgBot::Message::Ptr messagePtr) {
    if (!uInfo) {
        return 0;

    }

    updateUserInfoFromMessage(messagePtr);

    return (uInfo->cState == conversationState::GET_EDIT_MESSAGE) ? 1 : 0;

}

void getMessage2EditmsgHandler::handle(TgBot::Message::Ptr messagePtr) {
    if (!messagePtr->replyToMessage) {
        const char* msgText = aConfig->aMessages->errorReplyToBot;
        sendMessage(
                uInfo->userId,
                msgText,
                nullptr,
                nullptr,
                nullptr
        );

        uInfo->cState = conversationState::LOGGED_IN;
        {
            iUserDatabaseSql* userDb = new userDatabaseSql();
            userDb->uInfo = uInfo;
            userDb->l = l;
            userDb->writeUserData(iUserDatabaseSql::userDataRW::CONVERSATION_STATE);
            delete userDb;

        }

        return;

    }

    int errorCode = 0;
    PCRE2_SIZE errorOffset = 0;
    pcre2_code* re = pcre2_compile((PCRE2_SPTR8)"^/([0-9]+)$", PCRE2_ZERO_TERMINATED, 0, &errorCode, &errorOffset, NULL);
    if (!re) {
        PCRE2_UCHAR regexErrMsg[128];
        pcre2_get_error_message(errorCode, regexErrMsg, sizeof(regexErrMsg));
        char* errMsg = (char*)malloc(256 * sizeof(char));
        snprintf(errMsg, 256, "Regex compile error at %zu: %s", errorOffset, (char*)regexErrMsg);
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg);

        free(errMsg);

        return;

    }

    pcre2_match_data* matchData = pcre2_match_data_create_from_pattern(re, NULL);

    int rc = pcre2_match(re, (PCRE2_SPTR8)messagePtr->replyToMessage->text.c_str(), messagePtr->replyToMessage->text.size(), 0, 0, matchData, NULL);
    if (rc != 2) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "No matches found at the replied message");
        const char* msgText = aConfig->aMessages->errorWrongMessage;
        sendMessage(
                uInfo->userId,
                msgText,
                nullptr,
                nullptr,
                nullptr
        );

        uInfo->cState = conversationState::LOGGED_IN;
        {
            iUserDatabaseSql* userDb = new userDatabaseSql();
            userDb->uInfo = uInfo;
            userDb->l = l;
            userDb->writeUserData(iUserDatabaseSql::userDataRW::CONVERSATION_STATE);
            delete userDb;

        }

        pcre2_match_data_free(matchData);
        pcre2_code_free(re);

        return;

    }

    PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(matchData);

    char* msgIdStr = (char*)malloc(64 * sizeof(char));
    snprintf(msgIdStr, 64, "%.*s", (int)(ovector[3] - ovector[2]), messagePtr->replyToMessage->text.substr(ovector[2]).c_str());
    int64_t extractedFromReplyMsgId = strtoll(msgIdStr, NULL, 10);
    free(msgIdStr);

    TgBot::MessageId::Ptr copiedMsgIdPtr = bot->getApi().copyMessage(
            aConfig->privateChannelChatId,
            uInfo->userId,
            extractedFromReplyMsgId
    );

    pcre2_match_data_free(matchData);
    pcre2_code_free(re);

    FILE* fp = popen("head -c 8 /dev/urandom | base64 | tr -d '+/' | head -c 10", "r");
    if (!fp) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Can't open terminal pipe for random secret");

        return;

    }

    char secret[64];
    while (fgets(secret, sizeof(secret), fp)) {

    }

    pclose(fp);

    size_t secretLen = strlen(secret);
    for (size_t i = 0; i < secretLen; i++) {
        if (secret[i] == '\n' || secret[i] == '\r') {
            secret[i] = '\0';
            break;

        }

    }

    {
        uploadDatabaseSql* uploadDb = new uploadDatabaseSql();
        if (!uploadDb) {
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate uploadDb");

            return;

        }

        uploadDb->l = l;

        struct uploadInfo upInfo = {
            .messageId = static_cast<int64_t>(copiedMsgIdPtr->messageId),
            .secret = secret
        };

        uploadDb->upInfo = &upInfo;
        uploadDb->writeUploadData();
        delete uploadDb;

    }

    char* deepLink = (char*)malloc(256 * sizeof(char));
    if (!deepLink) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Memory allocation failed for deepLink");

        return;

    }
    snprintf(deepLink, 256, "https://t.me/%s?start=%s", bot->getApi().getMe()->username.c_str(), secret);

    TgBot::InlineKeyboardMarkup::Ptr iKeyboardM(new TgBot::InlineKeyboardMarkup);

    std::vector<TgBot::InlineKeyboardButton::Ptr> row;

    TgBot::InlineKeyboardButton::Ptr deepLinkBtn(new TgBot::InlineKeyboardButton);
    const char* deepLinkText = aConfig->aMessages->deepLinkBtnText;
    deepLinkBtn->text = deepLinkText;
    deepLinkBtn->url = deepLink;
    row.push_back(deepLinkBtn);

    iKeyboardM->inlineKeyboard.push_back(row);

    try {
        bot->getApi().copyMessage(
                aConfig->editMsgTargetChannel,
                uInfo->userId,
                messagePtr->messageId,
                "",
                "",
                std::vector<TgBot::MessageEntity::Ptr>(),
                false,
                nullptr,
                iKeyboardM,
                false,
                0
        );

    } catch (...) {
        free(deepLink);

        throw;

    }

    uInfo->cState = conversationState::LOGGED_IN;
    {
        iUserDatabaseSql* userDb = new userDatabaseSql();
        userDb->l = l;
        userDb->uInfo = uInfo;
        userDb->writeUserData(iUserDatabaseSql::userDataRW::CONVERSATION_STATE);
        delete userDb;

    }

    free(deepLink);

}

unsigned char channelJoinMsgHandler::canHandle(TgBot::Message::Ptr messagePtr) {
    if (!l || !bot || !uInfo || !aConfig) {

        return 0;

    }

    updateUserInfoFromMessage(messagePtr);

    if (!aConfig->channels2JoinChatIds) {

        return 0;

    }

    try {
        for (size_t i = 0; i < aConfig->channels2JoinChatIds->size(); i++) {
            TgBot::ChatMember::Ptr chatMemberPtr = bot->getApi().getChatMember(
                aConfig->channels2JoinChatIds->at(i),
                uInfo->userId
            );

            bool isMember = strncmp(chatMemberPtr->status.c_str(), "member", 6) == 0 ||
                            strncmp(chatMemberPtr->status.c_str(), "administrator", 13) == 0 ||
                            strncmp(chatMemberPtr->status.c_str(), "creator", 7) == 0;

            if (!isMember) {
                if (uInfo->hasJoined) {
                    uInfo->hasJoined = 0;

                    iUserDatabaseSql* userDb = new userDatabaseSql();
                    userDb->l = l;
                    userDb->uInfo = uInfo;
                    userDb->writeUserData(iUserDatabaseSql::userDataRW::HAS_JOINED);
                    delete userDb;

                }

                return 1;

            }

        }

        if (!uInfo->hasJoined) {
            uInfo->hasJoined = 1;

            iUserDatabaseSql* userDb = new userDatabaseSql();
            userDb->l = l;
            userDb->uInfo = uInfo;
            userDb->writeUserData(iUserDatabaseSql::userDataRW::HAS_JOINED);
            delete userDb;

        }

        return 0;

    } catch (TgBot::TgException& e) {
        std::string errMsg = "getChatMember failed: " + std::string(e.what());
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        return !uInfo->hasJoined;

    } catch (...) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "getChatMember failed with unknown error");

        return !uInfo->hasJoined;

    }

}

void channelJoinMsgHandler::sendJoinChannelPrompt(int64_t chatId) {
    sendJoinPrompt(l, bot, aConfig, chatId);

}

void channelJoinMsgHandler::handle(TgBot::Message::Ptr messagePtr) {
    if (!l || !messagePtr) {

        return;

    }

    l->logMsg(iLog::logLevel::WARNING, LOG_FUNC, "Blocked message from user not in channel");

    sendJoinChannelPrompt(messagePtr->chat->id);

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "channelJoinMsgHandler::handle complete");

}

unsigned char joinConfirmIKHandler::canHandle(TgBot::CallbackQuery::Ptr cBQueryPtr) {
    return (strncmp(cBQueryPtr->data.c_str(), "joinConfirm", 11) == 0) ? 1 : 0;

}

void joinConfirmIKHandler::handle(TgBot::CallbackQuery::Ptr cBQueryPtr) {
    updateUserInfoFromCallback(cBQueryPtr);

    try {
        answerCallbackQuery(cBQueryPtr->id);

    } catch (TgBot::TgException& e) {
        std::string errMsg = "answerCallbackQuery failed: " + std::string(e.what());
        l->logMsg(iLog::logLevel::WARNING, LOG_FUNC, errMsg.c_str());

    }

    try {
        bool allJoined = true;

        for (size_t i = 0; i < aConfig->channels2JoinChatIds->size(); i++) {
            TgBot::ChatMember::Ptr chatMemberPtr = bot->getApi().getChatMember(
                aConfig->channels2JoinChatIds->at(i),
                uInfo->userId
            );

            bool isMember = strncmp(chatMemberPtr->status.c_str(), "member", 6) == 0 ||
                            strncmp(chatMemberPtr->status.c_str(), "administrator", 13) == 0 ||
                            strncmp(chatMemberPtr->status.c_str(), "creator", 7) == 0;

            if (!isMember) {
                allJoined = false;

                break;

            }

        }

        if (allJoined) {
            if (!uInfo->hasJoined) {
                uInfo->hasJoined = 1;

                iUserDatabaseSql* userDb = new userDatabaseSql();
                userDb->l = l;
                userDb->uInfo = uInfo;
                userDb->writeUserData(iUserDatabaseSql::userDataRW::HAS_JOINED);
                delete userDb;

            }

        } else {
            if (uInfo->hasJoined) {
                uInfo->hasJoined = 0;

                iUserDatabaseSql* userDb = new userDatabaseSql();
                userDb->l = l;
                userDb->uInfo = uInfo;
                userDb->writeUserData(iUserDatabaseSql::userDataRW::HAS_JOINED);
                delete userDb;

            }

            sendJoinPrompt(l, bot, aConfig, uInfo->userId);

        }

    } catch (TgBot::TgException& e) {
        std::string errMsg = "getChatMember failed: " + std::string(e.what());
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        sendJoinPrompt(l, bot, aConfig, uInfo->userId);

    } catch (...) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "getChatMember failed with unknown error");

        sendJoinPrompt(l, bot, aConfig, uInfo->userId);

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "joinConfirmIKHandler::handle complete");

}

bool checkBotDbHandler::isUserInAllBots(int64_t userId) {
    return isUserInAllBotsImpl(l, aConfig, userId);

}

void checkBotDbHandler::handleAfterJoinConfirm(int64_t userId, int64_t chatId) {
    if (!aConfig || !aConfig->botDatabases) {
        return;

    }

    iTargetBotDb* targetBotDb = new targetBotDbSql();
    targetBotDb->l = l;

    bool userInAllBots = true;

    for (size_t i = 0; i < aConfig->botDatabases->size(); i++) {
        struct botInfo& info = aConfig->botDatabases->at(i);
        iTargetBotDb::dbCheckResult result = targetBotDb->isUserInDb(info.databasePath, userId);

        if (result == iTargetBotDb::dbCheckResult::DB_ERROR) {
            std::string errMsg = "Fatal: cannot read database for bot '" + std::string(info.botName ? info.botName : "?") +
                                 "', path may be incorrect in config: " + std::string(info.databasePath ? info.databasePath : "null");
            l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

            delete targetBotDb;
            std::raise(SIGINT);

            return;

        }

        if (result == iTargetBotDb::dbCheckResult::NOT_FOUND) {
            userInAllBots = false;

            break;

        }

    }

    delete targetBotDb;

    if (!userInAllBots) {
        if (joinHandler) {
            joinHandler->sendJoinChannelPrompt(chatId);

        } else {
            sendMessage(
                    chatId,
                    aConfig->aMessages->channelJoinSuccessMessage,
                    nullptr,
                    nullptr,
                    nullptr
            );

        }

        l->logMsg(iLog::logLevel::WARNING, LOG_FUNC,
            "checkBotDbHandler::handleAfterJoinConfirm: user missing from target bots, re-showing join prompt");

        return;

    }

    sendMessage(
            chatId,
            aConfig->aMessages->channelJoinSuccessMessage,
            nullptr,
            nullptr,
            nullptr
    );

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "checkBotDbHandler::handleAfterJoinConfirm complete");

}
