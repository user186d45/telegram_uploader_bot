#include "../include/sqlite.hpp"

#include <stdint.h>
#include <string>
#include <cstdlib>

unsigned char userDatabaseSql::createCheckDb() {
    if (!l) {

        return 1;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Opening database: user.db");
    int rc = sqlite3_open("user.db", &db);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on opening database: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        return 1;
    }
    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Database opened successfully");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Creating users table if not exists");
    const char* tableCreate = "CREATE TABLE IF NOT EXISTS users( "
                              "userId INTEGER PRIMARY KEY, "
                              "hasJoined INTEGER NOT NULL DEFAULT 0, "
                              "conversationState INTEGER NOT NULL DEFAULT 0 "
                              ");";

    rc = sqlite3_exec(db, tableCreate, 0, 0, 0);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on executing table create command: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        sqlite3_close(db);

        return 1;
    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Table created/verified successfully");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Create check database success");

    sqlite3_close(db);

    return 0;

}

userDatabaseSql::~userDatabaseSql() {
    if (!l) {
        return;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");

}

unsigned char userDatabaseSql::writeUserData(enum userDataRW uDataRW) {
    if (!l) {
        return 0;
    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, (std::string("Function called with mode: ") + std::to_string(uDataRW)).c_str());

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Opening database for write");
    int rc = sqlite3_open("user.db", &db);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on opening database: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        return 0;
    }
    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Database opened for write");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Building SQL statement");
    std::string insertSql;
    switch (uDataRW) {
        case userDataRW::ALL:
            insertSql = "INSERT OR REPLACE INTO users (userId, hasJoined, conversationState) VALUES (?, ?, ?);";

            break;

        case userDataRW::USER_ID:
            insertSql = "UPDATE users SET userId = ? WHERE userId = ?";

            break;

        case userDataRW::HAS_JOINED:
            insertSql = "UPDATE users SET hasJoined = ? WHERE userId = ?";

            break;

        case userDataRW::CONVERSATION_STATE:
            insertSql = "UPDATE users SET conversationState = ? WHERE userId = ?";

            break;

    }

    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, insertSql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on preparing the sql statement: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());
        sqlite3_close(db);

        return 0;

    }

    switch (uDataRW) {
        case userDataRW::ALL:
            sqlite3_bind_int64(stmt, 1, uInfo->userId);
            sqlite3_bind_int(stmt, 2, static_cast<int>(uInfo->hasJoined));
            sqlite3_bind_int(stmt, 3, static_cast<int>(uInfo->cState));

            break;

        case userDataRW::USER_ID:
            sqlite3_bind_int64(stmt, 1, uInfo->userId);
            sqlite3_bind_int64(stmt, 2, uInfo->userId);

            break;

        case userDataRW::HAS_JOINED:
            sqlite3_bind_int(stmt, 1, static_cast<int>(uInfo->hasJoined));
            sqlite3_bind_int64(stmt, 2, uInfo->userId);

            break;

        case userDataRW::CONVERSATION_STATE:
            sqlite3_bind_int(stmt, 1, static_cast<int>(uInfo->cState));
            sqlite3_bind_int64(stmt, 2, uInfo->userId);

            break;

    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::string errMsg = "Execute error at: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return 0;

    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 1;

}

unsigned char userDatabaseSql::readUserData(enum userDataRW uDataRW) {
    if (!l) {
        return 0;

    }

    std::string modeMsg = "Function called with mode: " + std::to_string(uDataRW);
    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, modeMsg.c_str());

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Opening database for read");
    int rc = sqlite3_open("user.db", &db);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on opening database: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        return 0;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Database opened for read");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Building SQL query");
    std::string readSql;
    switch (uDataRW) {
        case userDataRW::ALL:
            readSql = "SELECT userId, hasJoined, conversationState FROM users;";

            break;

        case userDataRW::USER_ID:
            readSql = "SELECT userId FROM users WHERE userId = ?;";

            break;

        case userDataRW::HAS_JOINED:
            readSql = "SELECT hasJoined FROM users WHERE userId = ?;";

            break;

        case userDataRW::CONVERSATION_STATE:
            readSql = "SELECT conversationState FROM users WHERE userId = ?;";

            break;

    }

    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, readSql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "Error occurred on preparing the sql statement");

        sqlite3_close(db);

        return 0;

    }

    switch (uDataRW) {
        case userDataRW::ALL:
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::string msg = "userId: " + std::to_string(sqlite3_column_int64(stmt, 0)) +
                    " hasJoined: " + std::to_string(sqlite3_column_int(stmt, 1)) +
                    " conversationState: " + std::to_string(sqlite3_column_int(stmt, 2));

                l->logMsg(iLog::logLevel::INFO, LOG_FUNC, msg.c_str());

            }

            break;

        case userDataRW::USER_ID:
            sqlite3_bind_int64(stmt, 1, uInfo->userId);
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_ROW) {
                l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "User not found in database");
                sqlite3_finalize(stmt);
                sqlite3_close(db);

                return 0;

            }

            uInfo->userId = sqlite3_column_int64(stmt, 0);

            break;

        case userDataRW::HAS_JOINED:
            sqlite3_bind_int64(stmt, 1, uInfo->userId);
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_ROW) {
                std::string errMsg = "Error occurred on executing: " + std::string(sqlite3_errmsg(db));
                l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "User not found in database");
                sqlite3_finalize(stmt);
                sqlite3_close(db);

                return 0;

            }

            uInfo->hasJoined = sqlite3_column_int(stmt, 0) != 0;

            break;

        case userDataRW::CONVERSATION_STATE:
            sqlite3_bind_int64(stmt, 1, uInfo->userId);
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_ROW) {
                std::string errMsg = "Error occurred on executing: " + std::string(sqlite3_errmsg(db));
                l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "User not found in database");
                sqlite3_finalize(stmt);
                sqlite3_close(db);

                return 0;

            }

            uInfo->cState = static_cast<enum conversationState>(sqlite3_column_int(stmt, 0));

            break;

    }


    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 1;

}

unsigned char userDatabaseSql::readUserById(int64_t userId) {
    if (!l) {
        return 0;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");
    
    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Opening database for read");
    int rc = sqlite3_open("user.db", &db);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on opening database: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        return 0;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Database opened for read");

    std::string readSql = "SELECT userId, hasJoined, conversationState FROM users WHERE userId = ?;";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, readSql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on preparing the sql statement: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());
        sqlite3_close(db);

        return 0;

    }

    sqlite3_bind_int64(stmt, 1, (int64_t)userId);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "User not found in database");

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return 0;

    }

    if (!uInfo) {
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, "uInfo struct pointer is null");
        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return 0;

    }

    uInfo->userId = sqlite3_column_int64(stmt, 0);
    uInfo->hasJoined = sqlite3_column_int(stmt, 1) != 0;
    uInfo->cState = static_cast<enum conversationState>(sqlite3_column_int(stmt, 2));
    
    std::string msg = "Loaded user: userId=" + std::to_string(uInfo->userId) + 
                      ", hasJoined=" + std::to_string(uInfo->hasJoined) + 
                      ", conversationState=" + std::to_string(static_cast<int>(uInfo->cState));

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, msg.c_str());

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 1;

}

std::vector<int64_t> userDatabaseSql::readAllUserIds() {
    std::vector<int64_t> userIds;

    if (!l) {
        return userIds;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Opening database for read");
    int rc = sqlite3_open("user.db", &db);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on opening database: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        return userIds;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Database opened for read");

    std::string readSql = "SELECT userId FROM users;";

    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, readSql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on preparing the sql statement: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());
        sqlite3_close(db);

        return userIds;

    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        userIds.push_back(sqlite3_column_int64(stmt, 0));

    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, (std::string("Found ") + std::to_string(userIds.size()) + " users").c_str());

    return userIds;

}

uint8_t userDatabaseSql::updateUserIfChanged() {
    if (!l || !uInfo) {

        return 0;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Opening database for read");
    int rc = sqlite3_open("user.db", &db);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on opening database: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        return 0;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Database opened for read");

    std::string readSql = "SELECT hasJoined, conversationState FROM users WHERE userId = ?;";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, readSql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on preparing the sql statement: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        sqlite3_close(db);

        return 0;

    }

    sqlite3_bind_int64(stmt, 1, uInfo->userId);
    rc = sqlite3_step(stmt);

    bool needsUpdate = false;
    if (rc == SQLITE_ROW) {
        int dbHasJoined = sqlite3_column_int(stmt, 0);

        if (static_cast<int>(uInfo->hasJoined) != dbHasJoined) {
            l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "hasJoined changed, needs update");

            needsUpdate = true;

        }

        int dbConversationState = sqlite3_column_int(stmt, 1);

        if (static_cast<int>(uInfo->cState) != dbConversationState) {
            l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "conversationState changed, needs update");

            needsUpdate = true;

        }

    } else {
        l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "User not found in database, will insert");

        needsUpdate = true;

    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (needsUpdate) {
        l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Updating user data in database");

        return writeUserData(userDataRW::ALL);

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "No changes detected");

    return 1;

}

unsigned char uploadDatabaseSql::createCheckDb() {
    if (!l) {

        return 1;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Opening database: upload.db");
    int rc = sqlite3_open("upload.db", &db);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on opening database: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        return 1;
    }
    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Database opened successfully");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Creating uploads table if not exists");
    const char* tableCreate = "CREATE TABLE IF NOT EXISTS uploads( "
                              "messageId INTEGER PRIMARY KEY, "
                              "secret TEXT NOT NULL "
                              ");";

    rc = sqlite3_exec(db, tableCreate, 0, 0, 0);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on executing table create command: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        sqlite3_close(db);

        return 1;
    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Table created/verified successfully");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Create check database success");

    sqlite3_close(db);

    return 0;

}

unsigned char uploadDatabaseSql::writeUploadData() {
    if (!l) {
        return 0;
    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Opening database for write");
    int rc = sqlite3_open("upload.db", &db);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on opening database: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        return 0;
    }
    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Database opened for write");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Building SQL statement");
    std::string insertSql = "INSERT OR REPLACE INTO uploads (messageId, secret) VALUES (?, ?);";

    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, insertSql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on preparing the sql statement: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());
        sqlite3_close(db);

        return 0;

    }

    sqlite3_bind_int64(stmt, 1, upInfo->messageId);
    sqlite3_bind_text(stmt, 2, (!upInfo->secret.empty() ? upInfo->secret.c_str() : NULL), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::string errMsg = "Execute error at: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return 0;

    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 1;

}

unsigned char uploadDatabaseSql::readUploadData() {
    if (!l) {
        return 0;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Opening database for read");
    int rc = sqlite3_open("upload.db", &db);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on opening database: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());

        return 0;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Database opened for read");

    std::string readSql = "SELECT messageId FROM uploads WHERE secret = ?;";

    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, readSql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::string errMsg = "Error occurred on preparing the sql statement: " + std::string(sqlite3_errmsg(db));
        l->logMsg(iLog::logLevel::ERROR, LOG_FUNC, errMsg.c_str());
        sqlite3_close(db);

        return 0;

    }

    sqlite3_bind_text(stmt, 1, (!upInfo->secret.empty() ? upInfo->secret.c_str() : NULL), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Upload not found in database");
        upInfo->messageId = 0;
        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return 0;

    }

    upInfo->messageId = sqlite3_column_int64(stmt, 0);

    std::string msg = "Loaded upload: messageId=" + std::to_string(upInfo->messageId) +
                      ", secret=" + upInfo->secret;

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, msg.c_str());

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 1;

}

uploadDatabaseSql::~uploadDatabaseSql() {
    if (!l) {
        return;

    }

    l->logMsg(iLog::logLevel::INFO, LOG_FUNC, "Function called");

}
