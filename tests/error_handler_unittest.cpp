
#include "handler/error_handler.hpp"
#include "http/http_response.hpp"
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
    ErrorHandler* handler = new ErrorHandler(HttpResponse::kStatusBadRequest);
    ASSERT_FALSE(handler->is_done());
    delete (handler);
}

UTEST(ErrorHandlerTest, has_output)
{
    ErrorHandler* handler = new ErrorHandler(HttpResponse::kStatusNotFound);
    ASSERT_TRUE(handler->has_output());
    delete (handler);
}

UTEST(ErrorHandlerTest, needs_)
{
    ErrorHandler* handler = new ErrorHandler(HttpResponse::kStatusInternalServerError);
    ASSERT_FALSE(handler->needs_input());
    delete (handler);
}

UTEST(ErrorHandlerTest, error_code)
{
    ErrorHandler* handler = new ErrorHandler(HttpResponse::kStatusInternalServerError);
    ASSERT_EQ(handler->error_code(), 500);
    delete (handler);
}
