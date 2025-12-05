#ifndef HTTP_HTTP_VERSION_HPP_
#define HTTP_HTTP_VERSION_HPP_

#include <string>

enum HttpVersion { kHttpVersionUnknown = 0, kHttpVersion1_0, kHttpVersion1_1 };

const char* http_version_to_string(HttpVersion protocol);
HttpVersion http_version_from_string(const std::string& str);

#endif // HTTP_HTTP_VERSION_HPP_
