#ifndef SQLITE_TELEGRAM_UPLOADER_HPP
#define SQLITE_TELEGRAM_UPLOADER_HPP

#include "../include/structs.hpp"
#include "../include/log.hpp"
#include <sqlite3.h>
#include <stdint.h>
#include <vector>

class iUserDatabaseSql {
public:
    enum userDataRW : uint8_t{
        ALL,
        USER_ID,
        HAS_JOINED,
        CONVERSATION_STATE
    };

    iUserDatabaseSql() = default;

    virtual unsigned char createCheckDb() = 0;
    virtual unsigned char writeUserData(enum userDataRW uDataRW) = 0;
    virtual unsigned char readUserData(enum userDataRW uDataRW) = 0;
    virtual unsigned char readUserById(int64_t userId) = 0;
    virtual std::vector<int64_t> readAllUserIds() = 0;
    virtual uint8_t updateUserIfChanged() = 0;

    struct userInfo* uInfo = nullptr;

    virtual ~iUserDatabaseSql() = default;

protected:
    iLog* l = nullptr;

};

class userDatabaseSql : public iUserDatabaseSql {
public:
    userDatabaseSql() = default;

    unsigned char createCheckDb() override;
    unsigned char writeUserData(enum userDataRW uDataRW) override;
    unsigned char readUserData(enum userDataRW uDataRW) override;
    unsigned char readUserById(int64_t userId) override;
    std::vector<int64_t> readAllUserIds() override;
    uint8_t updateUserIfChanged() override;

    ~userDatabaseSql();

private:
    sqlite3* db;

};

class iUploadDatabaseSql {
public:
    enum uploadDataRW : uint8_t{
        ALL,
        MESSAGE_ID,
        SECRET
    };

    iUploadDatabaseSql() = default;

    virtual unsigned char createCheckDb() = 0;
    virtual unsigned char writeUploadData(enum uploadDataRW uDataRW) = 0;
    virtual unsigned char readUploadData(enum uploadDataRW uDataRW) = 0;
    virtual unsigned char readUploadById(int64_t messageId) = 0;
    virtual std::vector<int64_t> readAllMessageIds() = 0;
    virtual uint8_t updateUploadIfChanged() = 0;

    struct uploadInfo* uInfo = nullptr;

    virtual ~iUploadDatabaseSql() = default;

protected:
    iLog* l = nullptr;

};

class uploadDatabaseSql : public iUploadDatabaseSql {
public:
    uploadDatabaseSql() = default;

    unsigned char createCheckDb() override;
    unsigned char writeUploadData(enum uploadDataRW uDataRW) override;
    unsigned char readUploadData(enum uploadDataRW uDataRW) override;
    unsigned char readUploadById(int64_t messageId) override;
    std::vector<int64_t> readAllMessageIds() override;
    uint8_t updateUploadIfChanged() override;

    ~uploadDatabaseSql();

private:
    sqlite3* db;

};

#endif
