#include "util/log_message.hpp"

#include <cstdio>
#include <cstdlib>

LogMessage::LogMessage(LogMessage::Level level)
    : level_(level)
{
}

LogMessage::~LogMessage()
{
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
    case kLevelDebug:   return kColorBlue;
    case kLevelInfo:    return kColorReset;
    case kLevelWarning: return kColorYellow;
    case kLevelError:   return kColorRed;
    case kLevelFatal:   return kColorRed;
    }

    NOTREACHED();
    return kColorReset;
}

const char* LogMessage::level_string(LogMessage::Level level)
{
    switch (level) {
    case kLevelDebug:   return "DEBUG";
    case kLevelInfo:    return "INFO";
    case kLevelWarning: return "WARN";
    case kLevelError:   return "ERROR";
    case kLevelFatal:   return "FATAL";
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
