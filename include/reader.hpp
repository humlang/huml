#pragma once

#include <source_range.hpp>
#include <symbol.hpp>
#include <ast.hpp>

#include <unordered_map>
#include <cassert>
#include <cstdio>
#include <memory>
#include <iosfwd>
#include <vector>
#include <string>

namespace ass
{
  struct instruction
  {
    op_code op;
    std::vector<ass::token> args;
  };
}

class base_reader
{
public:
  ~base_reader();
protected:
  base_reader(std::string_view module);

  char getc();
protected:
  std::string_view module;
  std::istream& is;

  std::string linebuf;

  std::size_t col;
  std::size_t row;
};

/// HX_READER

class hx_reader : base_reader
{
public:
  static constexpr std::size_t lookahead_size = 1;

  template<typename T>
  static std::vector<T> read(std::string_view module) { static_assert(sizeof(T) != 0, "unimplemented"); return {}; }
private:
  hx_reader(std::string_view module) : base_reader(module)
  {
    for(std::size_t i = 0; i < next_toks.size(); ++i)
      consume();

    // need one additional consume to initialize `current`
    consume(); 
  }

  token gett();

  void consume();

  // "hard error"   -> always consumes
  template<token_kind, typename F>
  bool expect(F&& f);
  template<std::uint8_t, typename F>
  bool expect(F&& f);

  // "soft error"    -> only consumes if ==
  template<token_kind>
  bool accept();
  template<std::uint8_t>
  bool accept();

private:
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

private:
  token old;
  token current;
  std::array<token, lookahead_size> next_toks;
};

// ASSEMBLER

class asm_reader : base_reader
{
public:
  static constexpr std::size_t lookahead_size = 1;

  template<typename T>
  static std::vector<T> read(std::string_view module) { static_assert(sizeof(T) != 0, "unimplemented"); return {}; }

private:
  asm_reader(std::string_view module) : base_reader(module)
  {
    for(std::size_t i = 0; i < next_toks.size(); ++i)
      consume();

    // need one additional consume to initialize `current`
    consume(); 
  }

  ass::token gett();

  void consume();

  // "hard error"   -> always consumes
  template<typename ass::token_kind, typename F>
  bool expect(F&& f);
  template<std::uint8_t, typename F>
  bool expect(F&& f);

  // "soft error"    -> only consumes if ==
  template<typename ass::token_kind>
  bool accept();
  template<std::uint8_t>
  bool accept();

private:
  ass::token old;
  ass::token current;
  std::array<ass::token, lookahead_size> next_toks;
};

