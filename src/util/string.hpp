#ifndef UTIL_STRING_HPP_
#define UTIL_STRING_HPP_

#include <string>
#include <vector>

std::vector<std::string> str_split(const std::string& str, const std::string& delim);

// Trims leading and trailing whitespaces (spaces and horizontal tabs)
std::string str_trim(const std::string& str);

std::string itoa(int val);

#endif
