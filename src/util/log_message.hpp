#ifndef UTIL_LOG_MESSAGE_HPP_
#define UTIL_LOG_MESSAGE_HPP_

#include <cassert>
#include <sstream>

class LogMessage {
public:
    enum Level { kLevelDebug, kLevelInfo, kLevelWarning, kLevelError, kLevelFatal };
    enum Color { kColorReset, kColorRed, kColorGreen, kColorYellow, kColorBlue };

    explicit LogMessage(LogMessage::Level level);
    ~LogMessage();

    template <typename T>
    LogMessage& operator<<(const T& value)
    {
        stream_ << value;
        return *this;
    }

private:
    LogMessage();
    LogMessage(const LogMessage&);
    LogMessage& operator=(const LogMessage&);

private:
    static const char* color_string(LogMessage::Color color);
    static const char* level_string(LogMessage::Level level);

    LogMessage::Color get_color() const;

    void print_message();

    LogMessage::Level level_;
    std::ostringstream stream_;
};

#define LOG_DEBUG LogMessage(LogMessage::kLevelDebug)
#define LOG_INFO  LogMessage(LogMessage::kLevelInfo)
#define LOG_WARN  LogMessage(LogMessage::kLevelWarning)
#define LOG_ERROR LogMessage(LogMessage::kLevelError)
#define LOG_FATAL LogMessage(LogMessage::kLevelFatal)

#define LOG(level) LOG_##level

#define NOTREACHED() \
    do { \
        fprintf(stderr, "[FATAL] NOTREACHED at %s:%d\n", __FILE__, __LINE__); \
        assert(false); \
    } while (0)

#endif // UTIL_LOG_MESSAGE_HPP_
