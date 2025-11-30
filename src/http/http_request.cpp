#include "http/http_request.hpp"

HttpRequest::HttpRequest()
    : content_length(0),
      is_chunked(false),
      keep_alive(false)
{
}
