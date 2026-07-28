#include "../include/bot.hpp"
#include "../include/structs.hpp"
#include "../include/cjson.hpp"
#include "../include/sqlite.hpp"
#include "../include/log.hpp"
#include "../include/passgen.h"

#include <stdio.h>
#include <tgbot/tgbot.h>
#include <iostream>
#include <csignal>
#include <cstring>

volatile std::sig_atomic_t keepRunning = 1;

extern "C" void signalHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        keepRunning = 0;
    }
}

int main() {
    std::cout << "Starting Telegram Uploader Bot..." << std::endl;

    // Initialize logging
    class log* logger = new class log(1);
    if (!logger) {
        std::cerr << "Failed to initialize logger!" << std::endl;
        return 1;
    }

    // Generate admin password first (writes to config.json)
    if (!generateAdminPass()) {
        logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to generate admin password");

        delete logger;
        return 1;

    }

    // Load config.json
    logger->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Loading config.json...");
    struct applicationConfig* aConfig = nullptr;
    {
        FILE* fp = fopen("config.json", "r");
        if (!fp) {
            logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to open config.json");

            delete logger;

            return 1;

        }

        char* buffer = (char*)malloc(1024 * 1024 * sizeof(char));
        if (!buffer) {
            logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to allocate buffer for config.json");

            fclose(fp);
            delete logger;

            return 1;

        }

        size_t rBytes = fread(buffer, sizeof(char), 1024 * 1024 * sizeof(char), fp);
        buffer[rBytes] = '\0';

        fclose(fp);

        std::string logMsg = "Config loaded (" + std::to_string(rBytes) + " bytes)";
        logger->logMsg(iLog::logLevel::INFO, LOG_FUNC, logMsg.c_str());

        iCjson* json = new cJsonDerived();
        if (!json) {
            logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Error occurred on allocating memory for iCjson interface");

            free(buffer);
            delete logger;

            return 1;

        }

        // Set logger on cjson parser
        json->l = logger;

        aConfig = json->applicationConfigParse(buffer);
        free(buffer);
        delete json;

        if (!aConfig) {
            logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to parse config.json");

            delete logger;

            return 1;

        }

        logger->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Config parsed successfully");

    }

    // Initialize databases
    logger->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Initializing databases...");
    {
        iUserDatabaseSql* iUserDbSql = new userDatabaseSql();
        iUserDbSql->l = logger;
        if (iUserDbSql->createCheckDb()) {
            logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to create/check user database");

            delete iUserDbSql;
            free((char*)aConfig->aMessages->startMessage);
            free((char*)aConfig->aMessages->donateMessage);
            free((char*)aConfig->aMessages->loginSuccess);
            free((char*)aConfig->aMessages->loginCancelled);
            free((char*)aConfig->aMessages->channelJoinMessage);
            free((char*)aConfig->aMessages->channelJoinConfirmText);
            free((char*)aConfig->aMessages->channelJoinSuccessMessage);
            free(aConfig->aMessages);
            free((char*)aConfig->botApiKey);
            free((char*)aConfig->password);
            delete aConfig->channels2JoinChatIds;
            delete aConfig->channels2JoinUrls;
            free(aConfig);
            delete logger;

            return 1;

        }

        delete iUserDbSql;

        iUploadDatabaseSql* iUploadDbSql = new uploadDatabaseSql();
        iUploadDbSql->l = logger;
        if (iUploadDbSql->createCheckDb()) {
            logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to create/check upload database");

            delete iUploadDbSql;
            free((char*)aConfig->aMessages->startMessage);
            free((char*)aConfig->aMessages->donateMessage);
            free((char*)aConfig->aMessages->loginSuccess);
            free((char*)aConfig->aMessages->loginCancelled);
            free((char*)aConfig->aMessages->channelJoinMessage);
            free((char*)aConfig->aMessages->channelJoinConfirmText);
            free((char*)aConfig->aMessages->channelJoinSuccessMessage);
            free(aConfig->aMessages);
            free((char*)aConfig->botApiKey);
            free((char*)aConfig->password);
            delete aConfig->channels2JoinChatIds;
            delete aConfig->channels2JoinUrls;
            free(aConfig);
            delete logger;

            return 1;

        }

        delete iUploadDbSql;

        logger->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Databases initialized successfully");

    }

    // Initialize TgBot
    logger->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Bot initialization started");
    TgBot::Bot bot(aConfig->botApiKey);

    // Initialize user info
    struct userInfo uInfo;
    uInfo.userId = 0;
    uInfo.hasJoined = 0;
    uInfo.cState = conversationState::IDLE;

    // Initialize upload info
    struct uploadInfo upInfo;

    // ============================================
    // Command Handlers
    // ============================================
    startCommandHandler startCmdHandler;
    startCmdHandler.bot = &bot;
    startCmdHandler.uInfo = &uInfo;
    startCmdHandler.aConfig = aConfig;
    startCmdHandler.l = logger;

    // ============================================
    // Message Handlers
    // ============================================
    getPasswordMsgHandler getPasswordHandler;
    getPasswordHandler.bot = &bot;
    getPasswordHandler.uInfo = &uInfo;
    getPasswordHandler.aConfig = aConfig;
    getPasswordHandler.l = logger;

    getContentMsgHandler getContentHandler;
    getContentHandler.bot = &bot;
    getContentHandler.uInfo = &uInfo;
    getContentHandler.aConfig = aConfig;
    getContentHandler.l = logger;

    channelJoinMsgHandler channelJoinHandler;
    channelJoinHandler.bot = &bot;
    channelJoinHandler.uInfo = &uInfo;
    channelJoinHandler.aConfig = aConfig;
    channelJoinHandler.l = logger;

    // ============================================
    // Inline Keyboard Handlers
    // ============================================
    donateMsgIKHandler donateIKHandler;
    donateIKHandler.bot = &bot;
    donateIKHandler.uInfo = &uInfo;
    donateIKHandler.aConfig = aConfig;
    donateIKHandler.l = logger;

    joinConfirmIKHandler joinConfirmIKHandler;
    joinConfirmIKHandler.bot = &bot;
    joinConfirmIKHandler.uInfo = &uInfo;
    joinConfirmIKHandler.aConfig = aConfig;
    joinConfirmIKHandler.l = logger;

    logger->logMsg(iLog::logLevel::INFO, LOG_FUNC, "All handlers initialized");

    // Set up bot event handlers
    bot.getEvents().onCommand("start", [&](TgBot::Message::Ptr message) {
        if (startCmdHandler.canHandle(message)) {
            startCmdHandler.handle(message);
        }
    });

    // Message handler for all messages
    bot.getEvents().onAnyMessage([&](TgBot::Message::Ptr message) {
        // Sync uInfo with the current message sender before dispatching
        if (message->from && message->from->id != uInfo.userId) {
            uInfo = userInfo{};
            uInfo.userId = message->from->id;
            uInfo.cState = conversationState::IDLE;

            iUserDatabaseSql* sqlLocal = new userDatabaseSql();
            sqlLocal->l = logger;
            sqlLocal->uInfo = &uInfo;
            sqlLocal->createCheckDb();
            sqlLocal->readUserById(message->from->id);
            delete(sqlLocal);
        }

        try {
            // Channel join check (blocks if not joined)
            if (channelJoinHandler.canHandle(message)) {
                channelJoinHandler.handle(message);

                return;

            }

            if (getPasswordHandler.canHandle(message)) {
                getPasswordHandler.handle(message);
            } else if (getContentHandler.canHandle(message)) {
                getContentHandler.handle(message);
            } else if (message->text.empty()) {
                logger->logMsg(iLog::logLevel::WARNING, LOG_FUNC, "Received empty message (likely non-text), ignoring");
            } else {
                logger->logMsg(iLog::logLevel::WARNING, LOG_FUNC,
                    ("Unhandled message: " + message->text).c_str());
            }
        } catch (const std::exception& e) {
            logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC,
                ("Exception in message handler: " + std::string(e.what())).c_str());
            try {
                bot.getApi().sendMessage(
                    message->chat->id,
                    "An error occurred. Please try again."
                );
            } catch (...) {
                logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to send error message to user");
            }
        } catch (...) {
            logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Unknown exception in message handler");
        }
    });

    // Callback query handler
    bot.getEvents().onCallbackQuery([&](TgBot::CallbackQuery::Ptr callbackQuery) {
        std::cout << "CallbackQuery data: " + callbackQuery->data + "\n";

        // Sync uInfo with the current callback sender before dispatching
        if (callbackQuery->from && callbackQuery->from->id != uInfo.userId) {
            uInfo = userInfo{};
            uInfo.userId = callbackQuery->from->id;
            uInfo.cState = conversationState::IDLE;

            iUserDatabaseSql* sqlLocal = new userDatabaseSql();
            sqlLocal->l = logger;
            sqlLocal->uInfo = &uInfo;
            sqlLocal->createCheckDb();
            sqlLocal->readUserById(callbackQuery->from->id);
            delete(sqlLocal);
        }

        // Join-confirmation callback always goes through for non-joined users
        if (joinConfirmIKHandler.canHandle(callbackQuery)) {
            joinConfirmIKHandler.handle(callbackQuery);

            return;

        }

        try {
            // Donate button
            if (donateIKHandler.canHandle(callbackQuery)) {
                donateIKHandler.handle(callbackQuery);
            } else {
                logger->logMsg(iLog::logLevel::WARNING, LOG_FUNC,
                    ("Unhandled callback: " + callbackQuery->data).c_str());
            }
        } catch (const std::exception& e) {
            logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC,
                ("Exception in callback handler: " + std::string(e.what())).c_str());
            try {
                bot.getApi().sendMessage(
                    callbackQuery->from->id,
                    "An error occurred. Please try again."
                );
            } catch (...) {
                logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Failed to send error message to user");
            }
        } catch (...) {
            logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Unknown exception in callback handler");
        }
    });

    logger->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Bot event handlers registered");

    // Start bot - use sigaction WITHOUT SA_RESTART so system calls return EINTR
    {
        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = signalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);
    }

    try {
        std::cout << "Bot is running... (Press Ctrl+C to stop)" << std::endl;
        logger->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Bot started successfully");

        TgBot::TgLongPoll longPoll(bot, 100, 2);
        while (keepRunning) {
            longPoll.start();
        }

        logger->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Bot stopped by signal, shutting down...");
    } catch (TgBot::TgException& e) {
        logger->logMsg(iLog::logLevel::ERROR, LOG_FUNC, ("Bot error: " + std::string(e.what())).c_str());
        std::cerr << "Bot error: " << e.what() << std::endl;

    }

    // Cleanup
    logger->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Running cleanup...");

    // Free aMessages inner strings (malloc'd by cjson parser)
    free((char*)aConfig->aMessages->startMessage);
    free((char*)aConfig->aMessages->donateMessage);
    free((char*)aConfig->aMessages->loginSuccess);
    free((char*)aConfig->aMessages->channelJoinMessage);
    free((char*)aConfig->aMessages->channelJoinConfirmText);
    free((char*)aConfig->aMessages->channelJoinSuccessMessage);
    free(aConfig->aMessages);

    free((char*)aConfig->botApiKey);
    free((char*)aConfig->password);

    delete aConfig->channels2JoinChatIds;

    for (size_t i = 0; i < aConfig->channels2JoinUrls->size(); i++) {
        free((char*)aConfig->channels2JoinUrls->at(i));

    }
    delete aConfig->channels2JoinUrls;

    free(aConfig);

    delete logger;

    return 0;

}
