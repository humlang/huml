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

struct scoping_context
{
  bool is_binding { false };
  std::vector<std::pair<symbol, ast_ptr>> binder_stack;
};

class hx_reader : base_reader
{
public:
  static constexpr std::size_t lookahead_size = 2;
  static const ast_ptr error_ref; // see parser.cpp

  template<typename T>
  static std::vector<T> read(std::string_view module) { static_assert(sizeof(T) != 0, "unimplemented"); return {}; }

  template<typename T>
  static std::vector<std::pair<T, scoping_context>> read_with_ctx(std::string_view module, scoping_context&& ctx)
  { static_assert(sizeof(T) != 0, "unimplemented"); return {}; }

  static std::pair<hx_ast, scoping_context> read_text(const std::string& str, scoping_context&& ctx);
private:
  hx_reader(std::string_view module) : base_reader(module)
  {
    for(std::size_t i = 0; i < next_toks.size(); ++i)
      consume();

    // need one additional consume to initialize `current`
    consume(); 
  }

  hx_reader(std::istream& is) : base_reader(is)
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
  ast_ptr mk_error();

  ast_ptr parse_assign();
  ast_ptr parse_expr_stmt();
  ast_ptr parse_type_ctor();
  ast_ptr parse_data_ctor();

  ast_ptr parse_with_parentheses();
  ast_ptr parse_type_check(ast_ptr left);
  ast_ptr parse_identifier();
  ast_ptr parse_Prop();
  ast_ptr parse_Type();
  ast_ptr parse_Kind();
  ast_ptr parse_app(ast_ptr lhs);
  ast_ptr parse_lambda();
  ast_ptr parse_arrow_lam(ast_ptr argument);
  ast_ptr parse_keyword();
  ast_ptr parse_statement();
  ast_ptr parse_prefix();
  ast_ptr parse_expression(int precedence = 0);
  int precedence();

private:
  token old;
  token current;
  std::array<token, lookahead_size> next_toks;
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

