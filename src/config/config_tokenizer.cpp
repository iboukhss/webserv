#include "config/config_tokenizer.hpp"

#include <fstream>
#include <stdexcept>

ConfigToken::ConfigToken(ConfigToken::Type type, const std::string& value, int line)
    : type(type),
      value(value),
      line(line)
{
}

ConfigTokenizer::ConfigTokenizer(const std::string& file_path)
    : file_path_(file_path)
{
}

std::vector<ConfigToken> ConfigTokenizer::tokenize_file() const
{
    std::ifstream file(file_path_.c_str());
    if (!file.is_open())
        throw std::runtime_error("Could not open config file '" + file_path_ + "'");

    std::string line;
    std::vector<ConfigToken> tokens;
    int line_number = 1;

    while (std::getline(file, line)) {
        const char* p = line.c_str();

        while (*p) {
            if (std::isspace(*p)) {
                p++;
                continue;
            }
            if (*p == '{') {
                tokens.push_back(ConfigToken(ConfigToken::kTypeLbrace, "{", line_number));
                p++;
                continue;
            }
            if (*p == '}') {
                tokens.push_back(ConfigToken(ConfigToken::kTypeRbrace, "}", line_number));
                p++;
                continue;
            }
            if (*p == ';') {
                tokens.push_back(ConfigToken(ConfigToken::kTypeSemicolon, ";", line_number));
                p++;
                continue;
            }

            const char* beg = p;
            while (*p && !std::isspace(*p) && *p != '{' && *p != '}' && *p != ';') {
                p++;
            }

            std::string word(beg, p - beg);
            tokens.push_back(ConfigToken(ConfigToken::kTypeWord, word, line_number));
        }
        line_number++;
    }
    return tokens;
}
