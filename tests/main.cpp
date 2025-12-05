#include "utest/utest.h"
#include "util/log_message.hpp"

// Documentation:
// https://github.com/sheredom/utest.h?tab=readme-ov-file#utest_main

UTEST_STATE();

int main(int argc, char** argv)
{
    // Make test output quieter
    LogMessage::g_log_level = LogMessage::kLevelFatal;
    return utest_main(argc, argv);
}
