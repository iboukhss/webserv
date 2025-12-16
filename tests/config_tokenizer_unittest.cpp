#include "config/config_tokenizer.hpp"
#include "utest/utest.h"

UTEST(ConfigTokenizerTest, EmptyFile)
{
    ConfigTokenizer tokenizer("tests/files/empty.conf");

    std::vector<ConfigToken> tokens = tokenizer.tokenize_file();
    EXPECT_EQ(0u, tokens.size());
}

UTEST(ConfigTokenizerTest, SingleWord)
{
    ConfigTokenizer tokenizer("tests/files/hello.conf");

    std::vector<ConfigToken> tokens = tokenizer.tokenize_file();

    ASSERT_EQ(1u, tokens.size());

    EXPECT_EQ(ConfigToken::kTypeWord, tokens[0].type);
    EXPECT_STREQ("hello", tokens[0].value.c_str());
    EXPECT_EQ(1, tokens[0].line);
}

UTEST(ConfigTokenizerTest, Symbols)
{
    ConfigTokenizer tokenizer("tests/files/symbols.conf");

    std::vector<ConfigToken> tokens = tokenizer.tokenize_file();

    ASSERT_EQ(3u, tokens.size());

    EXPECT_EQ(ConfigToken::kTypeLbrace, tokens[0].type);
    EXPECT_EQ(ConfigToken::kTypeRbrace, tokens[1].type);
    EXPECT_EQ(ConfigToken::kTypeSemicolon, tokens[2].type);
}

UTEST(ConfigTokenizerTest, MixedTokens)
{
    ConfigTokenizer tokenizer("tests/files/mixed.conf");

    std::vector<ConfigToken> tokens = tokenizer.tokenize_file();

    ASSERT_EQ(6u, tokens.size());

    EXPECT_EQ(ConfigToken::kTypeWord, tokens[0].type);
    EXPECT_STREQ("server", tokens[0].value.c_str());
    EXPECT_EQ(1, tokens[0].line);

    EXPECT_EQ(ConfigToken::kTypeLbrace, tokens[1].type);
    EXPECT_STREQ("{", tokens[1].value.c_str());
    EXPECT_EQ(1, tokens[1].line);

    EXPECT_EQ(ConfigToken::kTypeWord, tokens[2].type);
    EXPECT_STREQ("listen", tokens[2].value.c_str());
    EXPECT_EQ(2, tokens[2].line);

    EXPECT_EQ(ConfigToken::kTypeWord, tokens[3].type);
    EXPECT_STREQ("8080", tokens[3].value.c_str());
    EXPECT_EQ(2, tokens[3].line);

    EXPECT_EQ(ConfigToken::kTypeSemicolon, tokens[4].type);
    EXPECT_STREQ(";", tokens[4].value.c_str());
    EXPECT_EQ(2, tokens[4].line);

    EXPECT_EQ(ConfigToken::kTypeRbrace, tokens[5].type);
    EXPECT_STREQ("}", tokens[5].value.c_str());
    EXPECT_EQ(3, tokens[5].line);
}

UTEST(ConfigTokenizerTest, FileDoesNotExist)
{
    ConfigTokenizer tokenizer("tests/files/does_not_exist.conf");

    EXPECT_EXCEPTION(tokenizer.tokenize_file(), std::runtime_error);
}
