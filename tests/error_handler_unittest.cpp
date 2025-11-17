
#include "handler/error_handler.hpp"
#include "utest/utest.h"

/*
UTEST(ErrorHandlerTest, ErrorCode)
{
    UTEST_SKIP("TODO: Incompatibility error");
    ErrorHandler* handler = new ErrorHandler(123);
    int res = handler->get_error_code();
    ASSERT_EQ(res, 123);
    delete (handler);
}*/

UTEST(ErrorHandlerTest, is_done)
{
    ErrorHandler* handler = new ErrorHandler(123);
    ASSERT_TRUE(handler->is_done());
    delete (handler);
}

UTEST(ErrorHandlerTest, is_readable)
{
    ErrorHandler* handler = new ErrorHandler(123);
    ASSERT_FALSE(handler->is_readable());
    delete (handler);
}

UTEST(ErrorHandlerTest, is_writable)
{
    ErrorHandler* handler = new ErrorHandler(123);
    ASSERT_FALSE(handler->is_writable());
    delete (handler);
}
