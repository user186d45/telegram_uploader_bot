#ifndef LOG_HPP
#define LOG_HPP

#include <stdint.h>
#include <string_view>

// Returns "ClassName::methodName" from __PRETTY_FUNCTION__,
// stripping the return type and parameters.
static constexpr std::string_view scopedFuncName(const char* pretty) {
    std::string_view sv(pretty);
    auto space = sv.find(' ');
    if (space == sv.npos) return sv;
    auto paren = sv.find('(');
    if (paren == sv.npos) return sv.substr(space + 1);
    return sv.substr(space + 1, paren - space - 1);
}

#define LOG_FUNC scopedFuncName(__PRETTY_FUNCTION__).data()

class iLog {
public:
    iLog() = default;

    enum logLevel : uint8_t {
        DEBUG,
        INFO,
        WARNING,
        ERROR

    };

    virtual void logMsg(iLog::logLevel level, const char* funcName, const char* message) = 0;

    virtual ~iLog() = default;

};

class log : public iLog {
public:
    log(unsigned char status);

    void logMsg(iLog::logLevel level, const char* funcName, const char* message) override;

    ~log() = default;

private:
    unsigned char enabled = 0;
    unsigned char checkStatus();
    const char* levelString(iLog::logLevel level);

};

#endif

