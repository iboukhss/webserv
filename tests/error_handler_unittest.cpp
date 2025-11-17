
#include "handler/error_handler.hpp"
#include "utest/utest.h"

UTEST(ErrorHandlerTest, ErrorCode)
{
    ErrorHandler* handler = new ErrorHandler(123);
    ASSERT_EQ(handler->get_error_code(), 123);
}
