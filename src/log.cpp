#include "../include/log.hpp"
#include <stdio.h>
#include <time.h>
#include <cstring>
#include <string.h>

log::log(unsigned char status) : enabled(status) {
    // Note: Cannot use logMsg here as it would be recursive
    // Use printf for constructor logging
    if (status) {
        printf("[LOG] Logger initialized with enabled status\n");
    } else {
        printf("[LOG] Logger initialized with disabled status\n");
    }
}

unsigned char log::checkStatus() {
    return enabled ? 1 : 0;

}

const char* log::levelString(iLog::logLevel level) {

    switch (level) {
        case iLog::logLevel::DEBUG:
            return "DEBUG";

        case iLog::logLevel::INFO:
            return "INFO";

        case iLog::logLevel::WARNING:
            return "WARNING";

        case iLog::logLevel::ERROR:
            return "ERROR";

        default:
            return "UNKNOWN";

    }

}

void log::logMsg(iLog::logLevel level, const char* funcName, const char* message) {
    if (!checkStatus()) {
        return;

    }

    const char* lvl = levelString(level);
    time_t now = time(NULL);
    struct tm* tmNow = localtime(&now);
    char timeBuf[20];
    strftime(timeBuf, sizeof(timeBuf), "%Y/%m/%d %H:%M:%S", tmNow);

    printf("[ %s ] [ %s ] [ %s ]: %s\n", timeBuf, funcName, lvl, message);

}
