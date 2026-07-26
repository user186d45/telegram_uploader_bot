#ifndef STRUCTS_HPP
#define STRUCTS_HPP

#include <stdint.h>
#include <vector>
#include <string>

struct applicationConfig {
    std::vector<int64_t>*       adminChatIds = nullptr;
    std::vector<int64_t>*       channels2JoinChatIds = nullptr;
    std::vector<const char*>*   channels2JoinUrls = nullptr;
    int64_t                     privateChannelChatId = 0;
    const char*                 donateWalletAddress = nullptr;

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

