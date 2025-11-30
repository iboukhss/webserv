#include "util/log_message.hpp"

#include <cstdio>
#include <cstdlib>

LogMessage::Level LogMessage::g_log_level = LogMessage::kLevelInfo;

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
    /* clang-format off */
    fprintf(stderr, "%s[%s]%s %s\n",
			color_string(get_color()),
			level_string(level_),
            color_string(kColorReset),
			stream_.str().c_str());
    /* clang-format on */
}

LogMessage::Color LogMessage::get_color() const
{
    switch (level_) {
    case kLevelFatal:   return kColorRed;
    case kLevelError:   return kColorRed;
    case kLevelWarning: return kColorYellow;
    case kLevelInfo:    return kColorReset;
    case kLevelDebug:   return kColorBlue;
    }

    NOTREACHED();
    return kColorReset;
}

const char* LogMessage::level_string(LogMessage::Level level)
{
    switch (level) {
    case kLevelFatal:   return "FATAL";
    case kLevelError:   return "ERROR";
    case kLevelWarning: return "WARN";
    case kLevelInfo:    return "INFO";
    case kLevelDebug:   return "DEBUG";
    }

    NOTREACHED();
    return "UNKNOWN";
}

const char* LogMessage::color_string(LogMessage::Color color)
{
    switch (color) {
    case kColorReset:  return "\033[0m";
    case kColorRed:    return "\033[31m";
    case kColorGreen:  return "\033[32m";
    case kColorYellow: return "\033[33m";
    case kColorBlue:   return "\033[34m";
    }

    NOTREACHED();
    return "";
}
