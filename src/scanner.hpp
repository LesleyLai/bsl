#ifndef BEYOND_SHADING_LANGUAGE_SCANNER_HPP
#define BEYOND_SHADING_LANGUAGE_SCANNER_HPP

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>

/**
 * @file scanner.hpp
 * @brief This file contains the scanner for the Beyond Shading Language
 * compiler
 */

namespace beyond {

/**
 * @brief A position in the file contains line number and column number
 */
struct file_pos {
  std::size_t line = 1;
  std::size_t column = 1;
};

/// @brief Types of the tokens
enum class token_type {
#define TOKEN_TABLE_ENTRY(type, name, prefix, infix, precedence) type,
#include "token_table.inc"
#undef TOKEN_TABLE_ENTRY
};

/// @brief The token is the intermidiate data structure that the scanner and
/// parser communicate
struct token {
  token_type type = token_type::eof;
  std::string_view text;
  file_pos position = {};
};

#ifdef BSL_DEBUG
#define BSL_ASSERT(condition, message)                                         \
  do {                                                                         \
    if (!(condition)) {                                                        \
      []() {                                                                   \
        std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] "                \
                  << "Assert failed in "                                       \
                  << std::string_view{static_cast<const char*>(__func__)}      \
                  << ": "                                                      \
                  << std::string_view{static_cast<const char*>(message)}       \
                  << '\n'                                                      \
                  << "This is probabaly an internal bug of the Embedded ML "   \
                     "Implementation, please fill a bug report.\n";            \
        std::abort();                                                          \
      }();                                                                     \
    }                                                                          \
  } while (0)

// Indicates that we know execution should never reach this point in the
// program. In debug mode, we assert this fact because it's a bug to get here.
//
// In release mode, we use compiler-specific built in functions to tell the
// compiler the code can't be reached. This avoids "missing return" warnings
// in some cases and also lets it perform some optimizations by assuming the
// code is never reached.
#define BSL_UNREACHABLE()                                                      \
  do {                                                                         \
    std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] "                    \
              << "This code should not be reached in "                         \
              << std::string_view{static_cast<const char*>(__func__)}          \
              << "\nThis is probabaly an internal bug of the Embedded ML "     \
                 "Implementation, please fill a bug report.\n";                \
    std::abort();                                                              \
  } while (0)
#else
#define BSL_ASSERT(condition, message)                                         \
  do {                                                                         \
  } while (0)

// Tell the compiler that this part of the code will never be reached.
#if defined(_MSC_VER)
#define BSL_UNREACHABLE() __assume(0)
#elif defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
#define BSL_UNREACHABLE() __builtin_unreachable()
#else
#define BSL_UNREACHABLE()
#endif
#endif

constexpr auto isalpha(char c) noexcept -> bool
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

constexpr auto isdigit(char c) noexcept -> bool
{
  return (c >= '0' && c <= '9');
}

std::ostream& operator<<(std::ostream& s, token_type t);
std::ostream& operator<<(std::ostream& s, token t);

constexpr bool operator==(token lhs, token rhs)
{
  return lhs.type == rhs.type && lhs.text == rhs.text;
}

/// @brief The scanner scan the input string and output tokens
struct Scanner {
  std::string text;

  struct iterator {
    constexpr explicit iterator(std::string_view source) noexcept
        : start{source.data()}, current{start}, current_line_start{start}
    {
      ++(*this);
    }

    constexpr auto operator++() -> iterator&
    {
      token_ = next_token();
      return *this;
    }

    /**
     * @brief operator *
     * @warning If this iterator is not dereferenceable, operation is undefined
     */
    constexpr auto operator*() const -> const token
    {
      return token_;
    }

    constexpr auto operator-> () const -> const token*
    {
      return &token_;
    }

    constexpr auto operator==(const iterator rhs) const -> bool
    {
      return (**this == *rhs);
    }

    constexpr auto operator!=(const iterator rhs) const -> bool
    {
      return !(*this == rhs);
    }

  private:
    const char* start;
    const char* current;
    const char* current_line_start;
    std::size_t current_line = 1;
    token token_; // The token when we direference

    constexpr auto next_token() -> token
    {
      skip_whitespace();

      if (at_end()) {
        return token{};
      }

      start = current;

      char c = advance();
      if (beyond::isalpha(c)) {
        return identifier();
      }
      if (beyond::isdigit(c)) {
        return number();
      }

      switch (c) {
      case '(':
        return make_token(match(')') ? token_type::keyword_unit
                                     : token_type::left_paren);
      case ')':
        return make_token(token_type::right_paren);
      case '{':
        return make_token(token_type::left_brace);
      case '}':
        return make_token(token_type::right_brace);
      case ':':
        return make_token(token_type::colon);
      case ';':
        return make_token(token_type::semicolon);
      case ',':
        return make_token(token_type::comma);
      case '.':
        return make_token(token_type::dot);
      case '-':
        return make_token(match('>') ? token_type::minus_right_arrow
                                     : token_type::minus);
      case '+':
        return make_token(token_type::plus);
      case '/':
        return make_token(token_type::slash);
      case '\\':
        return make_token(token_type::backslash);
      case '*':
        return make_token(token_type::star);

      case '!':
        return make_token(match('=') ? token_type::bang_equal
                                     : token_type::bang);
      case '=':
        if (match('=')) {
          return make_token(token_type::double_equal);
        } else if (match('>')) {
          return make_token(token_type::equal_right_arrow);
        } else {
          return make_token(token_type::equal);
        }
      case '<':
        return make_token(match('=') ? token_type::less_equal
                                     : token_type::less);
      case '>':
        return make_token(match('=') ? token_type::greater_equal
                                     : token_type::greator);

      default:
        return error_token("Unexpected character.");
      }
    }

    constexpr auto identifier() noexcept -> token
    {
      while (isalpha(peek()) || (isdigit(peek()))) {
        advance();
      }

      return make_token(identifier_type());
    }

    constexpr auto number() noexcept -> token
    {
      while (beyond::isdigit(peek())) {
        advance();
      }

      // Look for a fractional part
      if (peek() == '.' && (std::isdigit(peek_next()) != 0)) {
        // Consume the "."
        advance();

        while (beyond::isdigit(peek())) {
          advance();
        }
      }

      return make_token(token_type::number_literal);
    }

    constexpr auto at_end() const noexcept -> bool
    {
      return *current == '\0';
    }

    constexpr auto advance() noexcept -> char
    {
      ++current;
      return current[-1];
    }

    constexpr auto peek() const noexcept -> char
    {
      return *current;
    }

    constexpr auto peek_next() const noexcept -> char
    {
      BSL_ASSERT(!at_end(), "call peek_next when at the end of a string");
      return current[1];
    }

    constexpr auto match(char expected) noexcept -> bool
    {
      if (*current != expected || at_end()) {
        return false;
      }

      current++;
      return true;
    }

    constexpr auto current_column() const noexcept -> std::size_t
    {
      return static_cast<std::size_t>(start - current_line_start + 1);
    }

    constexpr auto make_token(token_type type) const noexcept -> token
    {
      return {type,
              {start, static_cast<std::size_t>(current - start)},
              file_pos{current_line, current_column()}};
    }

    constexpr auto error_token(const char* message) const noexcept -> token
    {
      return {token_type::error, message,
              file_pos{current_line, current_column()}};
    }

    constexpr void skip_single_line_comment() noexcept
    {
      while (peek() != '\n' && !at_end()) {
        advance();
      }
    }

    constexpr void skip_whitespace() noexcept
    {
      while (true) {
        char c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
          advance();
          break;

        case '\n':
          current_line++;
          advance();
          current_line_start = current;
          break;

        case '/':
          if (peek_next() == '/') {
            skip_single_line_comment();
          } else {
            return;
          }
          break;

        default:
          return;
        }
      }
    }

    constexpr auto check_keyword(std::size_t start_offset,
                                 std::string_view rest, token_type type) const
        noexcept -> token_type
    {
      if (std::string_view{start + start_offset, rest.length()} == rest) {
        return type;
      }

      return token_type::identifier;
    }

    constexpr auto identifier_type() const noexcept -> token_type
    {
      switch (start[0]) {
      case 'a':
        if (current - start > 1) {
          switch (start[1]) {
          case 'n':
            return check_keyword(2, "d", token_type::keyword_and);
          case 's':
            return check_keyword(2, "ync", token_type::keyword_async);
          case 'w':
            return check_keyword(2, "ait", token_type::keyword_await);
          }
        }
        break;
      case 'c':
        if (current - start > 1) {
          switch (start[1]) {
          case 'a':
            return check_keyword(2, "se", token_type::keyword_case);
          case 'l':
            return check_keyword(2, "ass", token_type::keyword_class);
          }
        }
        break;
      case 'd':
        return check_keyword(1, "ef", token_type::keyword_def);
      case 'e':
        if (current - start > 1) {
          switch (start[1]) {
          case 'l':
            return check_keyword(2, "se", token_type::keyword_else);
          case 'x':
            return check_keyword(2, "tern", token_type::keyword_extern);
          }
        }
        break;
      case 'f':
        if (current - start > 1) {
          switch (start[1]) {
          case 'a':
            return check_keyword(2, "lse", token_type::keyword_false);
          case 'o':
            return check_keyword(2, "r", token_type::keyword_for);
          }
        }
        break;
      case 'i':
        return check_keyword(1, "f", token_type::keyword_if);
      case 'l':
        return check_keyword(1, "et", token_type::keyword_let);
      case 'n':
        return check_keyword(1, "ot", token_type::keyword_not);
      case 'o':
        return check_keyword(1, "r", token_type::keyword_or);
      case 'r':
        return check_keyword(1, "eturn", token_type::keyword_return);
      case 't':
        if (current - start > 1) {
          switch (start[1]) {
          case 'h':
            return check_keyword(2, "is", token_type::keyword_this);
          case 'r':
            return check_keyword(2, "ue", token_type::keyword_true);
          case 'y':
            return check_keyword(2, "pe", token_type::keyword_type);
          }
        }
        break;
      case 'u':
        if (current - start > 1) {
          switch (start[1]) {
          case 'n':
            if (current - start > 2) {
              switch (start[2]) {
              case 'i':
                return check_keyword(3, "form", token_type::keyword_uniform);
              case 's':
                return check_keyword(3, "afe", token_type::keyword_unsafe);
              }
            }
            break;
          }
        }
        break;

      case 'v':
        return check_keyword(1, "ariant", token_type::keyword_variant);
      }

      return token_type::identifier;
    }
  };

  constexpr auto begin() const -> iterator
  {
    return iterator{text};
  }
  constexpr auto end() const -> iterator
  {
    return iterator{"\0"};
  }
};

} // namespace beyond

#endif // BEYOND_SHADING_LANGUAGE_SCANNER_HPP
