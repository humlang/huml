#pragma once

#include <source_range.hpp>
#include <symbol.hpp>
#include <ast.hpp>

#include <unordered_map>
#include <cstdio>
#include <memory>
#include <iosfwd>
#include <vector>
#include <string>

class reader
{
public:

  template<typename T>
  static std::vector<T> read(std::string_view module) { static_assert(sizeof(T) != 0, "unimplemented"); return {}; }

  static constexpr std::size_t lookahead_size = 1;

  ~reader();
private:
  reader(std::string_view module);

  template<typename T>
  T get() { static_assert(sizeof(T) != 0, "unimplemented"); }

  void consume();


  enum class FailType
  {
    Statement,
    Expression
  };

  // "hard error"   -> always consumes
  template<token_kind, FailType type, typename F>
  bool expect(F&& f);
  template<std::uint8_t, FailType type, typename F>
  bool expect(F&& f);


  template<token_kind, typename F>
  bool expect_stmt(F&& f);
  template<std::uint8_t, typename F>
  bool expect_stmt(F&& f);

  template<token_kind, typename F>
  bool expect_expr(F&& f);
  template<std::uint8_t, typename F>
  bool expect_expr(F&& f);

  // "soft error"    -> only consumes if ==
  template<token_kind>
  bool accept();
  template<std::uint8_t>
  bool accept();

  // always uses old for the error
  error mk_error();

  maybe_stmt parse_keyword();
  maybe_expr parse_literal();
  maybe_expr parse_identifier();
  maybe_stmt parse_block();
  maybe_stmt parse_assign();
  maybe_stmt parse_statement();

  maybe_expr parse_prefix(); // prefix operators like ! - etc. Further this is also used for variables e.g x
  exp_type parse_binary(maybe_expr left);
  maybe_expr parse_expression(int precedence); // TODO we need precedence table for Right now only add a (+ -) b
  int precedence();


  // Ignores tokens until it finds the next start for a valid statement
  void find_next_valid_stmt();
  // Ignores tokens until it finds the next start for a valid expression
  void find_next_valid_expr();

private:
  std::string_view module;
  std::istream& is;

  std::string linebuf;

  std::size_t col;
  std::size_t row;

  token old {token_kind::Undef, "", {}};
  token current {token_kind::Undef, "", {}};
  std::array<token, lookahead_size> next_toks { {{token_kind::Undef, "", {}}} };
};

