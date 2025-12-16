#include "config/config_parser.hpp"
#include "config/config_tokenizer.hpp"
#include "utest/utest.h"

#include <iostream>
#include <stdexcept>

/*
static const char* token_type_to_string(ConfigToken::Type t)
{
    switch (t) {
    case ConfigToken::kTypeLbrace:    return "LBRACE";
    case ConfigToken::kTypeRbrace:    return "RBRACE";
    case ConfigToken::kTypeSemicolon: return "SEMICOLON";
    case ConfigToken::kTypeWord:      return "WORD";
    default:                          return "UNKNOWN";
    }
}

static void print_tokens(const std::vector<ConfigToken>& tokens)
{
    for (size_t i = 0; i < tokens.size(); i++) {
        ConfigToken tok = tokens[i];
        const char* type_str = token_type_to_string(tok.type);

        std::cout << type_str << ", " << tok.value << ", " << tok.line << "\n";
    }
    std::cout.flush();
}

static void print_ast(const AstNode& node, int depth = 0)
{
    std::string pad(depth * 2, ' ');
    std::cout << pad << node.name;

    for (size_t i = 0; i < node.args.size(); i++) {
        std::cout << " '" << node.args[i] << "'";
    }

    std::cout << "\n";

    for (size_t i = 0; i < node.children.size(); i++) {
        print_ast(node.children[i], depth + 1);
    }
    std::cout.flush();
}
*/

UTEST(ConfigParserTest, EmptyTokens)
{
    std::vector<ConfigToken> tokens;
    ConfigParser parser(tokens);

    std::vector<AstNode> nodes = parser.parse_tokens();
    EXPECT_EQ(0u, nodes.size());
}

UTEST(ConfigParserTest, SingleStatement)
{
    std::vector<ConfigToken> tokens;
    tokens.push_back(ConfigToken(ConfigToken::kTypeWord, "listen", 1));
    tokens.push_back(ConfigToken(ConfigToken::kTypeWord, "8080", 1));
    tokens.push_back(ConfigToken(ConfigToken::kTypeSemicolon, ";", 1));

    ConfigParser parser(tokens);

    std::vector<AstNode> nodes = parser.parse_tokens();
    ASSERT_EQ(1u, nodes.size());

    const AstNode statement = nodes[0];
    EXPECT_STREQ("listen", statement.name.c_str());
    ASSERT_EQ(1u, statement.args.size());
    EXPECT_STREQ("8080", statement.args[0].c_str());
    EXPECT_EQ(1, statement.line);
}

UTEST(ConfigParserTest, SingleBlock)
{
    std::vector<ConfigToken> tokens;
    tokens.push_back(ConfigToken(ConfigToken::kTypeWord, "server", 1));
    tokens.push_back(ConfigToken(ConfigToken::kTypeLbrace, "{", 2));
    tokens.push_back(ConfigToken(ConfigToken::kTypeRbrace, "}", 3));

    ConfigParser parser(tokens);

    std::vector<AstNode> nodes = parser.parse_tokens();
    ASSERT_EQ(1u, nodes.size());

    const AstNode block = nodes[0];
    EXPECT_STREQ("server", block.name.c_str());
    ASSERT_EQ(0u, block.args.size());
    ASSERT_EQ(0u, block.children.size());
    EXPECT_EQ(1, block.line);
}

UTEST(ConfigParserTest, NestedBlockAndStatement)
{
    std::vector<ConfigToken> tokens;
    tokens.push_back(ConfigToken(ConfigToken::kTypeWord, "server", 1));
    tokens.push_back(ConfigToken(ConfigToken::kTypeLbrace, "{", 1));
    tokens.push_back(ConfigToken(ConfigToken::kTypeWord, "listen", 2));
    tokens.push_back(ConfigToken(ConfigToken::kTypeWord, "8080", 2));
    tokens.push_back(ConfigToken(ConfigToken::kTypeSemicolon, ";", 2));
    tokens.push_back(ConfigToken(ConfigToken::kTypeRbrace, "}", 3));

    ConfigParser parser(tokens);

    std::vector<AstNode> nodes = parser.parse_tokens();
    ASSERT_EQ(1u, nodes.size());

    const AstNode block = nodes[0];
    EXPECT_STREQ("server", block.name.c_str());
    ASSERT_EQ(0u, block.args.size());
    ASSERT_EQ(1u, block.children.size());
    EXPECT_EQ(1, block.line);

    const AstNode statement = block.children[0];
    EXPECT_STREQ("listen", statement.name.c_str());
    ASSERT_EQ(1u, statement.args.size());
    EXPECT_STREQ("8080", statement.args[0].c_str());
    EXPECT_EQ(2, statement.line);
}

UTEST(ConfigParserTest, MissingSemicolonTrhows)
{
    std::vector<ConfigToken> tokens;
    tokens.push_back(ConfigToken(ConfigToken::kTypeWord, "listen", 1));
    tokens.push_back(ConfigToken(ConfigToken::kTypeWord, "8080", 1));

    ConfigParser parser(tokens);

    EXPECT_EXCEPTION(parser.parse_tokens(), std::runtime_error);
}
