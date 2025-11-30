#ifndef UTIL_LOG_MESSAGE_HPP_
#define UTIL_LOG_MESSAGE_HPP_

#include <cassert>
#include <sstream>

class LogMessage {
public:
    enum Level {
        kLevelFatal = 0,
        kLevelError = 1,
        kLevelWarning = 2,
        kLevelInfo = 3,
        kLevelDebug = 4
    };

    enum Color { kColorReset, kColorRed, kColorGreen, kColorYellow, kColorBlue };

    explicit LogMessage(LogMessage::Level level);
    ~LogMessage();

    template <typename T>
    LogMessage& operator<<(const T& value)
    {
        if (level_ <= g_log_level)
            stream_ << value;
        return *this;
    }

    static LogMessage::Level g_log_level;

private:
    LogMessage(const LogMessage&);
    LogMessage& operator=(const LogMessage&);

    static const char* color_string(LogMessage::Color color);
    static const char* level_string(LogMessage::Level level);

    LogMessage::Color get_color() const;

    void print_message();

    LogMessage::Level level_;
    std::ostringstream stream_;
};

#define LOG_FATAL  LogMessage(LogMessage::kLevelFatal)
#define LOG_ERROR  LogMessage(LogMessage::kLevelError)
#define LOG_WARN   LogMessage(LogMessage::kLevelWarning)
#define LOG_INFO   LogMessage(LogMessage::kLevelInfo)
#define LOG_DEBUG  LogMessage(LogMessage::kLevelDebug)

#define LOG(level) LOG_##level

#define NOTREACHED() \
    do { \
        fprintf(stderr, "[FATAL] NOTREACHED at %s:%d\n", __FILE__, __LINE__); \
        assert(false); \
    } while (0)

#endif // UTIL_LOG_MESSAGE_HPP_
