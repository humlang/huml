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
  static std::vector<ast_type> read(std::string_view module);

  ~reader();
private:
  reader(std::string_view module);

  template<typename T>
  T get() { static_assert(sizeof(T) != 0, "unimplemented"); }

  void consume();

  // "hard error"   -> always consumes
  template<typename F>
  void expect(token_kind kind, F&& f);
  template<typename F>
  void expect(std::int8_t kind, F&& f);

  // "soft error"    -> only consumes if ==
  bool accept(token_kind kind);
  bool accept(std::int8_t kind);

  stmt_type parse_keyword();
  rec_wrap_t<literal> parse_literal();
  rec_wrap_t<block> parse_block();
  stmt_type parse_statement();
private:
  std::string_view module;
  std::istream& is;

  std::string linebuf;

  std::size_t col;
  std::size_t row;


  token old {token_kind::Undef, "", {}};
  token current {token_kind::Undef, "", {}};
  std::array<token, 1> next_toks { {{token_kind::Undef, "", {}}} };
};

