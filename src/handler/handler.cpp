
#include "Handler.hpp"

bool Handler::isDone()
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
