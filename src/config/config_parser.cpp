#include "config/config_parser.hpp"

#include "util/string.hpp"

#include <stdexcept>

AstNode::AstNode()
    : line(-1)
{
}

ConfigParser::ConfigParser(const std::vector<ConfigToken>& tokens)
    : tokens_(tokens),
      pos_(0)
{
}

// EBNF grammar:

// config    := directive*
// directive := block | statement
// block     := WORD (WORD)? '{' directive* '}'
// statement := WORD WORD+ ';'

std::vector<AstNode> ConfigParser::parse_tokens()
{
    std::vector<AstNode> nodes;

    while (!eof()) {
        nodes.push_back(parse_directive());
    }
    return nodes;
}

static void parse_error(const ConfigToken& tok)
{
    throw std::runtime_error("Unexpected token '" + tok.value + "' at line " + itoa(tok.line));
}

AstNode ConfigParser::parse_directive()
{
    size_t i = 0;

    while (peek(i).type == ConfigToken::kTypeWord) {
        ++i;
    }
    if (peek(i).type == ConfigToken::kTypeLbrace) {
        return parse_block();
    }
    if (peek(i).type == ConfigToken::kTypeSemicolon) {
        return parse_statement();
    }
    parse_error(peek(i));

    /* UNREACHABLE */
    return AstNode();
}

AstNode ConfigParser::parse_block()
{
    const ConfigToken& tok = consume_word();

    AstNode node;
    node.name = tok.value;
    node.line = tok.line;

    if (peek().type == ConfigToken::kTypeWord)
        node.args.push_back(consume().value);

    expect(ConfigToken::kTypeLbrace);

    while (!optional(ConfigToken::kTypeRbrace)) {
        node.children.push_back(parse_directive());
    }
    return node;
}

AstNode ConfigParser::parse_statement()
{
    const ConfigToken& tok = consume_word();

    AstNode node;
    node.name = tok.value;
    node.line = tok.line;

    node.args.push_back(consume_word().value);

    while (peek().type == ConfigToken::kTypeWord) {
        node.args.push_back(consume().value);
    }
    expect(ConfigToken::kTypeSemicolon);
    return node;
}

const ConfigToken& ConfigParser::peek(size_t offset) const
{
    if (pos_ + offset >= tokens_.size())
        throw std::runtime_error("Unexpected end of file");

    return tokens_[pos_ + offset];
}

const ConfigToken& ConfigParser::consume()
{
    if (eof())
        throw std::runtime_error("Unexpected end of file");

    return tokens_[pos_++];
}

const ConfigToken& ConfigParser::expect(ConfigToken::Type type)
{
    const ConfigToken& tok = peek();

    if (tok.type != type)
        parse_error(tok);

    return consume();
}

bool ConfigParser::optional(ConfigToken::Type type)
{
    if (eof())
        return false;

    if (peek().type == type) {
        consume();
        return true;
    }
    return false;
}

bool ConfigParser::eof() const
{
    return pos_ == tokens_.size();
}

const ConfigToken& ConfigParser::consume_word()
{
    return expect(ConfigToken::kTypeWord);
}

/*
static const char* g_directives[] = {
    // block directives
    "http",
    "location",
    "server",

    // server directives
    "backlog",
    "listen",

    // location directives
    "alias",

    // shared directives
    "allow_methods",
    "allow_uploads",
    "autoindex",
    "cgi_allow_methods",
    "cgi_handler",
    "client_max_body_size",
    "error_page",
    "index",
    "return",
    "root",
    "upload_store"
};

static bool is_valid_directive(const std::string& name)
{
    for (size_t i = 0; i < sizeof(g_directives) / sizeof(g_directives[0]); i++) {
        if (name == g_directives[i])
            return true;
    }
    return false;
}
*/
