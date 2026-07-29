#ifndef BOT_TELEGRAM_UPLOADER_HPP
#define BOT_TELEGRAM_UPLOADER_HPP

#include "../include/structs.hpp"
#include "../include/log.hpp"

#include <tgbot/tgbot.h>

class iBot {
public:
    iBot() = default;

    virtual ~iBot() {
        if (l) {
            l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "iBot destructor called");
        }

        if (l) {
            l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "iBot destructor complete");
        }

    }

    TgBot::Bot* bot = nullptr;

    iLog* l = nullptr;
    applicationConfig* aConfig = nullptr;
    userInfo* uInfo = nullptr;

protected:
    void updateUserInfoFromMessage(TgBot::Message::Ptr messagePtr);
    void updateUserInfoFromCallback(TgBot::CallbackQuery::Ptr cBQueryPtr);

    template<typename... Args>
    TgBot::Message::Ptr sendMessage(Args&&... args) {
        try {
            return bot->getApi().sendMessage(std::forward<Args>(args)...);
        } catch (TgBot::TgException& e) {
            l->logMsg(iLog::logLevel::ERROR, "sendMessage",
                ("Failed: " + std::string(e.what())).c_str());
            throw;
        }
    }

    template<typename... Args>
    TgBot::Message::Ptr forwardMessage(Args&&... args) {
        try {
            return bot->getApi().forwardMessage(std::forward<Args>(args)...);
        } catch (TgBot::TgException& e) {
            l->logMsg(iLog::logLevel::ERROR, "forwardMessage",
                ("Failed: " + std::string(e.what())).c_str());
            throw;
        }
    }

    template<typename... Args>
    bool answerCallbackQuery(Args&&... args) {
        try {
            return bot->getApi().answerCallbackQuery(std::forward<Args>(args)...);
        } catch (TgBot::TgException& e) {
            l->logMsg(iLog::logLevel::ERROR, "answerCallbackQuery",
                ("Failed: " + std::string(e.what())).c_str());
            throw;
        }
    }

};

class commandHandler : public iBot {
public:
    virtual unsigned char canHandle(TgBot::Message::Ptr messagePtr) = 0;
    virtual void handle(TgBot::Message::Ptr messagePtr) = 0;

};

class startCommandHandler : public commandHandler {
public:
    unsigned char canHandle(TgBot::Message::Ptr messagePtr) override;
    void handle(TgBot::Message::Ptr messagePtr) override;

};

class messageHandler : public iBot {
public:
    virtual unsigned char canHandle(TgBot::Message::Ptr messagePtr) = 0;
    virtual void handle(TgBot::Message::Ptr messagePtr) = 0;

};

class inlineKeyboardHandler : public iBot {
public:
    virtual unsigned char canHandle(TgBot::CallbackQuery::Ptr cBQueryPtr) = 0;
    virtual void handle(TgBot::CallbackQuery::Ptr cBQueryPtr) = 0;

};

class donateMsgIKHandler : public inlineKeyboardHandler {
public:
    unsigned char canHandle(TgBot::CallbackQuery::Ptr cBQueryPtr) override;
    void handle(TgBot::CallbackQuery::Ptr cBQueryPtr) override;

};

class getPasswordMsgHandler : public messageHandler {
public:
    unsigned char canHandle(TgBot::Message::Ptr messagePtr) override;
    void handle(TgBot::Message::Ptr messagePtr) override;

};

class getContentMsgHandler : public messageHandler {
public:
    unsigned char canHandle(TgBot::Message::Ptr messagePtr) override;
    void handle(TgBot::Message::Ptr messagePtr) override;

};

class channelJoinMsgHandler : public messageHandler {
public:
    unsigned char canHandle(TgBot::Message::Ptr messagePtr) override;
    void handle(TgBot::Message::Ptr messagePtr) override;

};

class joinConfirmIKHandler : public inlineKeyboardHandler {
public:
    unsigned char canHandle(TgBot::CallbackQuery::Ptr cBQueryPtr) override;
    void handle(TgBot::CallbackQuery::Ptr cBQueryPtr) override;

};

class checkBotDbHandler : public iBot {
public:
    unsigned char isUserInBotDb(const char* databasePath, int64_t userId);
    void handleAfterJoinConfirm(int64_t userId, int64_t chatId);

};

#endif

