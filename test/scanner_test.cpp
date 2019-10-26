#include <catch2/catch.hpp>

#include <array>

#include "../src/scanner.hpp"

using beyond::Scanner;
using beyond::token;
using beyond::token_type;

TEST_CASE("scanner", "[scanner]")
{
  GIVEN("hello")
  {
    const auto hello = "hello";
    THEN("Scan as an identifier")
    {
      Scanner s{hello};
      REQUIRE(s.begin() != s.end());
      REQUIRE(s.begin()->type == token_type::identifier);
    }
  }

  GIVEN("keywords")
  {
    const auto keywords =
        R"(and async await case class def else extern false for if let not
        or return this true type () uniform unsafe variant)";

    Scanner s{keywords};

    WHEN("Scan the string")
    {
      THEN("Should get corerect keywords tokens")
      {
        std::array expected{
            token{token_type::keyword_and, "and"},
            token{token_type::keyword_async, "async"},
            token{token_type::keyword_await, "await"},
            token{token_type::keyword_case, "case"},
            token{token_type::keyword_class, "class"},
            token{token_type::keyword_def, "def"},
            token{token_type::keyword_else, "else"},
            token{token_type::keyword_extern, "extern"},
            token{token_type::keyword_false, "false"},
            token{token_type::keyword_for, "for"},
            token{token_type::keyword_if, "if"},
            token{token_type::keyword_let, "let"},
            token{token_type::keyword_not, "not"},
            token{token_type::keyword_or, "or"},
            token{token_type::keyword_return, "return"},
            token{token_type::keyword_this, "this"},
            token{token_type::keyword_true, "true"},
            token{token_type::keyword_type, "type"},
            token{token_type::keyword_unit, "()"},
            token{token_type::keyword_uniform, "uniform"},
            token{token_type::keyword_unsafe, "unsafe"},
            token{token_type::keyword_variant, "variant"},
        };

        auto scanner_itr = std::begin(s);

        for (auto expected_token : expected) {
          REQUIRE(*scanner_itr == expected_token);
          ++scanner_itr;
        }
        REQUIRE(scanner_itr == std::end(s));
      }
    }
  }

  SECTION("Ignore single line comments")
  {
    Scanner s{R"(// Hello world)"};
    REQUIRE(s.begin() == s.end());
  }
}

TEST_CASE("Error handling of the scanner", "[scanner]")
{
  GIVEN("A string contains unknown characters")
  {
    Scanner s{R"(let # = 3)"};
    WHEN("Scan the string")
    {
      THEN("Correct identify the error, but scan the remaining part correctly")
      {
        std::array expected{
            token{token_type::keyword_let, "let"},
            token{token_type::error, "Unexpected character."},
            token{token_type::equal, "="},
            token{token_type::number_literal, "3"},
        };

        auto scanner_itr = std::begin(s);
        for (auto expected_token : expected) {
          REQUIRE(*scanner_itr == expected_token);
          ++scanner_itr;
        }
        REQUIRE(scanner_itr == std::end(s));
      }
    }
  }
}
