#include "http/http_request.hpp"

HttpRequest::HttpRequest(HttpVersion protocol)
    : http_version(protocol),
      content_length(0),
      is_chunked(false),
      keep_alive(http_version == kHttpVersion1_1)
{
}
