#pragma once

#include <source_range.hpp>
#include <fixit_info.hpp>
#include <symbol.hpp>
#include <token.hpp>
#include <ast.hpp>

#include <unordered_map>
#include <cassert>
#include <cstdio>
#include <memory>
#include <iosfwd>
#include <vector>
#include <string>

class base_reader
{
public:
  ~base_reader();
protected:
  base_reader(std::string_view module);
  base_reader(std::istream& stream);

  char getc();
protected:
  std::string_view module;
  std::istream& is;

  std::string linebuf;

  std::size_t col;
  std::size_t row;
private:
  bool uses_reader { false };
};

/// HX_READER

class hx_reader : base_reader
{
public:
  static constexpr std::size_t lookahead_size = 1;
  static constexpr std::size_t error_ref = static_cast<std::size_t>(-1);

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
private:
  // always uses old for the error
  std::size_t mk_error();


  std::size_t parse_assign();
  std::size_t parse_expr_stmt();
  std::size_t parse_type_ctor();
  std::size_t parse_data_ctor();

  std::size_t parse_constructor();

  std::size_t parse_type_check(std::size_t left);
  std::size_t parse_identifier();
  std::size_t parse_case();
  std::size_t parse_Prop();
  std::size_t parse_Type();
  std::size_t parse_Kind();
  std::size_t parse_app(std::size_t lhs);
  std::size_t parse_lambda();
  std::size_t parse_keyword();
  std::size_t parse_match();
  std::size_t parse_statement();
  std::size_t parse_prefix();
  std::size_t parse_pattern();
  std::size_t parse_expression(int precedence = 0);
  int precedence();

private:
  token old;
  token current;
  std::array<token, lookahead_size> next_toks;

  hx_ast global_scope;
  struct scoping_context
  {
    bool is_binding { false };
    std::vector<std::pair<symbol, std::uint_fast32_t>> binder_stack;
  };
  scoping_context scoping_ctx;

  bool parsing_pattern { false };
  bool parsing_constructor { false };

  std::vector<fixit_info> fixits_stack;
};

// ASSEMBLER

class asm_reader : base_reader
{
public:
  static constexpr std::size_t lookahead_size = 1;

  template<typename T>
  static std::vector<T> read(std::string_view module) { static_assert(sizeof(T) != 0, "unimplemented"); return {}; }

  template<typename T>
  static std::vector<T> read_text(const std::string& text) { static_assert(sizeof(T) != 0, "unimplemented"); return {}; }

private:
  asm_reader(std::string_view module) : base_reader(module)
  {
    for(std::size_t i = 0; i < next_toks.size(); ++i)
      consume();

    // need one additional consume to initialize `current`
    consume(); 
  }

  asm_reader(std::istream& is) : base_reader(is)
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
  ass::instruction parse_op(op_code opc);
  ass::instruction parse_directive();
  ass::instruction parse_labeldef();

private:
  ass::token old;
  ass::token current;
  std::array<ass::token, lookahead_size> next_toks;
};

