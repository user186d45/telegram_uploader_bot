#include "../include/bot.hpp"

#include <string.h>
#include <vector>
#include <cstdio>

void iBot::updateUserInfoFromMessage(TgBot::Message::Ptr messagePtr) {
    // TODO

}

void iBot::updateUserInfoFromCallback(TgBot::CallbackQuery::Ptr cBQueryPtr) {
    // TODO

}

unsigned char startCommandHandler::canHandle(TgBot::Message::Ptr messagePtr) {
    return (strncmp(messagePtr->text.c_str(), "/start", 6) == 0) ? 1 : 0;

}

void startCommandHandler::handle(TgBot::Message::Ptr messagePtr) {
    updateUserInfoFromMessage(messagePtr);

    iUserDbSql->writeUserData(iUserDatabaseSql::userDataRW::ALL);

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

    iUploadDbSql = new uploadDatabaseSql();
    if (!iUploadDbSql) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate iUploadDbSql");

        return;

    }

    iUploadDbSql->l = l;

    struct uploadInfo upInfo = {.secret = messagePtr->text.substr(7)};

    iUploadDbSql->upInfo = &upInfo;
    iUploadDbSql->readUploadData();
    delete iUploadDbSql;
    iUploadDbSql = nullptr;

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

    forwardMessage(
            uInfo->userId,
            aConfig->privateChannelChatId,
            upInfo.messageId
    );

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

}

unsigned char getPasswordMsgHandler::canHandle(TgBot::Message::Ptr messagePtr) {
    if (!aConfig || !aConfig->password) {
        return 0;

    }

    return (strcmp(messagePtr->text.c_str(), aConfig->password) == 0) ? 1 : 0;

}

void getPasswordMsgHandler::handle(TgBot::Message::Ptr messagePtr) {
    if (uInfo->cState == GET_LINKS) {
        uInfo->cState = IDLE;

        iUserDbSql->writeUserData(iUserDatabaseSql::userDataRW::CONVERSATION_STATE);

        sendMessage(
                uInfo->userId,
                aConfig->aMessages->loginCancelled,
                nullptr,
                nullptr,
                nullptr
        );

    } else {
        uInfo->cState = GET_LINKS;

        iUserDbSql->writeUserData(iUserDatabaseSql::userDataRW::CONVERSATION_STATE);

        sendMessage(
                uInfo->userId,
                aConfig->aMessages->loginSuccess,
                nullptr,
                nullptr,
                nullptr
        );

    }

}

unsigned char getContentMsgHandler::canHandle(TgBot::Message::Ptr messagePtr) {
    if (!uInfo) {
        return 0;

    }

    return (uInfo->cState == GET_LINKS) ? 1 : 0;

}

void getContentMsgHandler::handle(TgBot::Message::Ptr messagePtr) {
    iUploadDbSql = new uploadDatabaseSql();
    if (!iUploadDbSql) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Cannot allocate iUploadDbSql");

        return;

    }

    iUploadDbSql->l = l;

    FILE* fp = popen("head -c 8 /dev/urandom | base64 | head -c 10", "r");
    if (!fp) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Can't open terminal pipe for random secret");

        delete iUploadDbSql;
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

    struct uploadInfo upInfo = {
        .messageId = static_cast<int64_t>(messagePtr->messageId),
        .secret = secret
    };

    iUploadDbSql->upInfo = &upInfo;
    iUploadDbSql->writeUploadData();
    delete iUploadDbSql;
    iUploadDbSql = nullptr;

    uInfo->cState = IDLE;

    iUserDbSql->writeUserData(iUserDatabaseSql::userDataRW::CONVERSATION_STATE);

}

unsigned char channelJoinMsgHandler::canHandle(TgBot::Message::Ptr messagePtr) {
    if (!l || !bot || !uInfo || !aConfig) {

        return 0;

    }

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

                    iUserDbSql->writeUserData(iUserDatabaseSql::userDataRW::HAS_JOINED);

                }

                return 1;

            }

        }

        if (!uInfo->hasJoined) {
            uInfo->hasJoined = 1;

            iUserDbSql->writeUserData(iUserDatabaseSql::userDataRW::HAS_JOINED);

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

                iUserDbSql->writeUserData(iUserDatabaseSql::userDataRW::HAS_JOINED);

            }

            sendMessage(
                    uInfo->userId,
                    aConfig->aMessages->channelJoinSuccessMessage,
                    nullptr,
                    nullptr,
                    nullptr
            );

        } else {
            if (uInfo->hasJoined) {
                uInfo->hasJoined = 0;

                iUserDbSql->writeUserData(iUserDatabaseSql::userDataRW::HAS_JOINED);

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

}
