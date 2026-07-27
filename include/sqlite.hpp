#ifndef SQLITE_TELEGRAM_UPLOADER_HPP
#define SQLITE_TELEGRAM_UPLOADER_HPP

#include "../include/structs.hpp"
#include "../include/log.hpp"
#include <sqlite3.h>
#include <stdint.h>
#include <vector>

class iUserDatabaseSql {
public:
    iUserDatabaseSql() = default;

    enum userDataRW : uint8_t{
        ALL,
        USER_ID,
        HAS_JOINED,
        CONVERSATION_STATE
    };

    virtual unsigned char createCheckDb() = 0;
    virtual unsigned char writeUserData(enum userDataRW uDataRW) = 0;
    virtual unsigned char readUserData(enum userDataRW uDataRW) = 0;
    virtual unsigned char readUserById(int64_t userId) = 0;
    virtual std::vector<int64_t> readAllUserIds() = 0;
    virtual uint8_t updateUserIfChanged() = 0;

    struct userInfo* uInfo = nullptr;
    iLog* l = nullptr;

    virtual ~iUserDatabaseSql() = default;

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
    iUploadDatabaseSql() = default;

    virtual unsigned char createCheckDb() = 0;
    virtual unsigned char writeUploadData() = 0;
    virtual unsigned char readUploadData() = 0;

    struct uploadInfo* upInfo = nullptr;
    iLog* l = nullptr;

    virtual ~iUploadDatabaseSql() = default;

};

class uploadDatabaseSql : public iUploadDatabaseSql {
public:
    uploadDatabaseSql() = default;

    unsigned char createCheckDb() override;
    unsigned char writeUploadData() override;
    unsigned char readUploadData() override;

    ~uploadDatabaseSql();

private:
    sqlite3* db;

};

#endif
