#include "../include/bot.hpp"
#include "../include/sqlite.hpp"

#include <string.h>
#include <vector>
#include <cstdio>

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
    
    uInfo->userId = messageUserId;
    
    iUserDatabaseSql* userDb = new userDatabaseSql();
    userDb->uInfo = uInfo;
    
    userDb->createCheckDb();
    
    {
        userInfo fresh{};
        fresh.userId = uInfo->userId;
        *uInfo = fresh;
    }
    
    if (!userDb->readUserById(uInfo->userId)) {
        l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "User not found in database, will create new entry");
        uInfo->cState = conversationState::IDLE;
        uInfo->hasJoined = 0;

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
    
    uInfo->userId = callbackUserId;
    
    
    iUserDatabaseSql* userDb = new userDatabaseSql();
    userDb->uInfo = uInfo;
    
    userDb->createCheckDb();
    
    {
        userInfo fresh{};
        fresh.userId = uInfo->userId;
        *uInfo = fresh;
    }
    
    if (!userDb->readUserById(uInfo->userId)) {
        l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "User not found in database, will create new entry");
        uInfo->cState = conversationState::IDLE;
        uInfo->hasJoined = 0;

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

    struct uploadInfo upInfo = {.secret = messagePtr->text.substr(7)};

    uploadDb->upInfo = &upInfo;
    uploadDb->readUploadData();
    delete uploadDb;

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

    if (uInfo->cState == GET_LINKS) {
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
        uInfo->cState = GET_LINKS;

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

unsigned char getContentMsgHandler::canHandle(TgBot::Message::Ptr messagePtr) {
    if (!uInfo) {
        return 0;

    }

    updateUserInfoFromMessage(messagePtr);

    return (uInfo->cState == GET_LINKS) ? 1 : 0;

}

void getContentMsgHandler::handle(TgBot::Message::Ptr messagePtr) {

    uploadDatabaseSql* uploadDb = new uploadDatabaseSql();
    if (!uploadDb) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate uploadDb");

        return;

    }

    uploadDb->l = l;

    FILE* fp = popen("head -c 8 /dev/urandom | base64 | head -c 10", "r");
    if (!fp) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Can't open terminal pipe for random secret");

        delete uploadDb;
        return;

    }

    char buffer[64];
    std::string secret;
    while (fgets(buffer, sizeof(buffer), fp)) {
        secret += buffer;

    }

    pclose(fp);

    while (!secret.empty() && (secret.back() == '\n' || secret.back() == '\r')) {
        secret.pop_back();

    }

    TgBot::Message::Ptr forwardedMsg = forwardMessage(
            aConfig->privateChannelChatId,
            uInfo->userId,
            messagePtr->messageId
    );

    struct uploadInfo upInfo = {
        .messageId = static_cast<int64_t>(forwardedMsg->messageId),
        .secret = secret
    };

    uploadDb->upInfo = &upInfo;
    uploadDb->writeUploadData();
    delete uploadDb;

    std::string msgText = "https://t.me/" + bot->getApi().getMe()->username + "?start=" + upInfo.secret;
    sendMessage(
            uInfo->userId,
            msgText,
            nullptr,
            nullptr,
            nullptr
    );

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "getContentMsgHandler::handle complete");

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

void channelJoinMsgHandler::handle(TgBot::Message::Ptr messagePtr) {
    if (!l || !messagePtr) {

        return;

    }

    l->logMsg(iLog::logLevel::WARNING, LOG_FUNC, "Blocked message from user not in channel");

    try {
        TgBot::InlineKeyboardMarkup::Ptr iKeyboardM(new TgBot::InlineKeyboardMarkup);

        if (aConfig->channels2JoinUrls) {
            for (size_t i = 0; i < aConfig->channels2JoinUrls->size(); i++) {
                std::vector<TgBot::InlineKeyboardButton::Ptr> row;

                TgBot::InlineKeyboardButton::Ptr joinBtn(new TgBot::InlineKeyboardButton);
                joinBtn->text = aConfig->channels2JoinUrls->at(i);
                joinBtn->url = aConfig->channels2JoinUrls->at(i);
                row.push_back(joinBtn);

                iKeyboardM->inlineKeyboard.push_back(row);

            }

        }

        std::vector<TgBot::InlineKeyboardButton::Ptr> confirmRow;

        TgBot::InlineKeyboardButton::Ptr joinConfirmBtn(new TgBot::InlineKeyboardButton);
        joinConfirmBtn->text = aConfig->aMessages->channelJoinConfirmText;
        joinConfirmBtn->callbackData = "joinConfirm";
        confirmRow.push_back(joinConfirmBtn);

        iKeyboardM->inlineKeyboard.push_back(confirmRow);

        sendMessage(
                messagePtr->chat->id,
                aConfig->aMessages->channelJoinMessage,
                nullptr,
                nullptr,
                iKeyboardM
        );

    } catch (...) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to send join-channel message to user");

    }

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

            TgBot::InlineKeyboardMarkup::Ptr iKeyboardM(new TgBot::InlineKeyboardMarkup);

            if (aConfig->channels2JoinUrls) {
                for (size_t i = 0; i < aConfig->channels2JoinUrls->size(); i++) {
                    std::vector<TgBot::InlineKeyboardButton::Ptr> row;

                    TgBot::InlineKeyboardButton::Ptr joinBtn(new TgBot::InlineKeyboardButton);
                    joinBtn->text = aConfig->channels2JoinUrls->at(i);
                    joinBtn->url = aConfig->channels2JoinUrls->at(i);
                    row.push_back(joinBtn);

                    iKeyboardM->inlineKeyboard.push_back(row);

                }

            }

            std::vector<TgBot::InlineKeyboardButton::Ptr> confirmRow;

            TgBot::InlineKeyboardButton::Ptr joinConfirmBtn(new TgBot::InlineKeyboardButton);
            joinConfirmBtn->text = aConfig->aMessages->channelJoinConfirmText;
            joinConfirmBtn->callbackData = "joinConfirm";
            confirmRow.push_back(joinConfirmBtn);

            iKeyboardM->inlineKeyboard.push_back(confirmRow);

            sendMessage(
                    uInfo->userId,
                    aConfig->aMessages->channelJoinMessage,
                    nullptr,
                    nullptr,
                    iKeyboardM
            );

        }

    } catch (TgBot::TgException& e) {
        std::string errMsg = "getChatMember failed: " + std::string(e.what());
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        TgBot::InlineKeyboardMarkup::Ptr iKeyboardM(new TgBot::InlineKeyboardMarkup);

        std::vector<TgBot::InlineKeyboardButton::Ptr> confirmRow;

        TgBot::InlineKeyboardButton::Ptr joinConfirmBtn(new TgBot::InlineKeyboardButton);
        joinConfirmBtn->text = aConfig->aMessages->channelJoinConfirmText;
        joinConfirmBtn->callbackData = "joinConfirm";
        confirmRow.push_back(joinConfirmBtn);

        iKeyboardM->inlineKeyboard.push_back(confirmRow);

        sendMessage(
                uInfo->userId,
                aConfig->aMessages->channelJoinMessage,
                nullptr,
                nullptr,
                iKeyboardM
        );

    } catch (...) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "getChatMember failed with unknown error");

        TgBot::InlineKeyboardMarkup::Ptr iKeyboardM(new TgBot::InlineKeyboardMarkup);

        std::vector<TgBot::InlineKeyboardButton::Ptr> confirmRow;

        TgBot::InlineKeyboardButton::Ptr joinConfirmBtn(new TgBot::InlineKeyboardButton);
        joinConfirmBtn->text = aConfig->aMessages->channelJoinConfirmText;
        joinConfirmBtn->callbackData = "joinConfirm";
        confirmRow.push_back(joinConfirmBtn);

        iKeyboardM->inlineKeyboard.push_back(confirmRow);

        sendMessage(
                uInfo->userId,
                aConfig->aMessages->channelJoinMessage,
                nullptr,
                nullptr,
                iKeyboardM
        );

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "joinConfirmIKHandler::handle complete");

}

unsigned char checkBotDbHandler::isUserInBotDb(const char* databasePath, int64_t userId) {
    if (!databasePath) {
        return 0;

    }

    sqlite3* db;
    if (sqlite3_open(databasePath, &db) != SQLITE_OK) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot open external bot database");

        return 0;

    }

    const char* sql = "SELECT userId FROM users WHERE userId = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot prepare statement for external bot database");

        sqlite3_close(db);
        return 0;

    }

    sqlite3_bind_int64(stmt, 1, userId);

    unsigned char found = (sqlite3_step(stmt) == SQLITE_ROW) ? 1 : 0;

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return found;

}

void checkBotDbHandler::handleAfterJoinConfirm(int64_t userId, int64_t chatId) {
    if (!aConfig || !aConfig->botDatabases) {
        return;

    }

    std::vector<std::pair<const char*, const char*>> missingBots;

    for (size_t i = 0; i < aConfig->botDatabases->size(); i++) {
        struct botInfo& info = aConfig->botDatabases->at(i);
        if (!isUserInBotDb(info.databasePath, userId)) {
            missingBots.emplace_back(info.botName, info.databasePath);

        }

    }

    TgBot::InlineKeyboardMarkup::Ptr iKeyboardM(new TgBot::InlineKeyboardMarkup);

    for (size_t i = 0; i < missingBots.size(); i++) {
        std::vector<TgBot::InlineKeyboardButton::Ptr> row;

        TgBot::InlineKeyboardButton::Ptr botBtn(new TgBot::InlineKeyboardButton);
        std::string btnText = "Start " + std::string(missingBots[i].first);
        std::string btnUrl = "https://t.me/" + std::string(missingBots[i].first);
        botBtn->text = btnText;
        botBtn->url = btnUrl;
        row.push_back(botBtn);

        iKeyboardM->inlineKeyboard.push_back(row);

    }

    sendMessage(
            chatId,
            aConfig->aMessages->channelJoinSuccessMessage,
            nullptr,
            nullptr,
            missingBots.empty() ? nullptr : iKeyboardM
    );

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "checkBotDbHandler::handleAfterJoinConfirm complete");

}
