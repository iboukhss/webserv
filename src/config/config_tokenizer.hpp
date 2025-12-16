#ifndef CONFIG_CONFIG_TOKENIZER_HPP_
#define CONFIG_CONFIG_TOKENIZER_HPP_

#include <string>
#include <vector>

struct ConfigToken {
    enum Type { kTypeWord, kTypeLbrace, kTypeRbrace, kTypeSemicolon };

    ConfigToken::Type type;
    std::string value;
    int line;

    ConfigToken(ConfigToken::Type type, const std::string& value, int line);
};

class ConfigTokenizer {
public:
    explicit ConfigTokenizer(const std::string& file_path);
    std::vector<ConfigToken> tokenize_file() const;

private:
    std::string file_path_;
};

#endif // CONFIG_CONFIG_TOKENIZER_HPP_
