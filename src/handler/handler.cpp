
#include "handler.hpp"

bool Handler::is_done()
{
    return (done_);
}

Handler::Handler(const std::string& path)
    : path_(path),
      done_(false)
{
}

Handler::~Handler()
{
}
