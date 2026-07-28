#ifndef STRUCTS_HPP
#define STRUCTS_HPP

#include <stdint.h>
#include <vector>
#include <string>

struct applicationMessages {
    const char* startMessage = nullptr;
    const char* donateMessage = nullptr;
    const char* loginSuccess = nullptr;
    const char* loginCancelled = nullptr;
    const char* channelJoinMessage = nullptr;
    const char* channelJoinConfirmText = nullptr;
    const char* channelJoinSuccessMessage = nullptr;
    const char* messageDeleted = nullptr;

};

struct botInfo {
    const char* botName = nullptr;
    const char* databasePath = nullptr;

};

struct applicationConfig {
    const char*                 botApiKey = nullptr;
    const char*                 password = nullptr;
    std::vector<const char*>*   adminChatIds = nullptr;
    std::vector<int64_t>*       channels2JoinChatIds = nullptr;
    std::vector<const char*>*   channels2JoinUrls = nullptr;
    int64_t                     privateChannelChatId = 0;
    struct applicationMessages* aMessages = nullptr;
    std::vector<struct botInfo>* botDatabases = nullptr;

};

enum conversationState : uint8_t {
    IDLE,
    GET_LINKS

};

struct userInfo {
    int64_t                     userId = 0;
    unsigned char               hasJoined = 0;
    enum conversationState      cState = conversationState::IDLE;

};

struct uploadInfo {
    int64_t                     messageId = 0;
    std::string                 secret;

};

#endif

