#include "util/log_message.hpp"

#include <cstdio>
#include <cstdlib>

#ifdef DEBUG
LogMessage::Level LogMessage::g_log_level = LogMessage::kLevelDebug;
#else
LogMessage::Level LogMessage::g_log_level = LogMessage::kLevelInfo;
#endif

LogMessage::LogMessage(LogMessage::Level level)
    : level_(level)
{
}

LogMessage::~LogMessage()
{
    if (level_ <= g_log_level)
        print_message();

    if (level_ == kLevelFatal)
        std::abort();
}

void LogMessage::print_message()
{
    fprintf(stderr,
            "%s[%s] %s\n%s",
            color_string(get_color()),
            level_string(level_),
            stream_.str().c_str(),
            color_string(kColorReset));
}

LogMessage::Color LogMessage::get_color() const
{
    switch (level_) {
    case kLevelFatal:   return kColorRed;
    case kLevelError:   return kColorRed;
    case kLevelWarning: return kColorYellow;
    case kLevelInfo:    return kColorReset;
    case kLevelDebug:   return kColorBlue;
    default:            return kColorReset;
    }
}

const char* LogMessage::level_string(LogMessage::Level level)
{
    switch (level) {
    case kLevelFatal:   return "FATAL";
    case kLevelError:   return "ERROR";
    case kLevelWarning: return "WARN";
    case kLevelInfo:    return "INFO";
    case kLevelDebug:   return "DEBUG";
    default:            return "UNKNOWN";
    }
}

const char* LogMessage::color_string(LogMessage::Color color)
{
    switch (color) {
    case kColorReset:  return "\033[0m";
    case kColorRed:    return "\033[31m";
    case kColorGreen:  return "\033[32m";
    case kColorYellow: return "\033[33m";
    case kColorBlue:   return "\033[34m";
    default:           return "";
    }
}
