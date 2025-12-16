#ifndef CONFIG_CONFIG_PARSER_HPP_
#define CONFIG_CONFIG_PARSER_HPP_

#include "config/config_tokenizer.hpp"

#include <string>
#include <vector>

struct AstNode {
    std::string name;
    std::vector<std::string> args;
    std::vector<AstNode> children;
    int line;

    AstNode();
};

class ConfigParser {
public:
    explicit ConfigParser(const std::vector<ConfigToken>& tokens);

    std::vector<AstNode> parse_tokens();

private:
    const ConfigToken& peek(size_t offset = 0) const;
    const ConfigToken& consume();
    const ConfigToken& expect(ConfigToken::Type type);
    bool optional(ConfigToken::Type type);
    bool eof() const;

    const ConfigToken& consume_word();

    AstNode parse_directive();
    AstNode parse_block();
    AstNode parse_statement();

    const std::vector<ConfigToken>& tokens_;
    size_t pos_;
};

#endif // CONFIG_CONFIG_PARSER_HPP_
