#pragma once

#include "symbol.hpp"
#include "source_range.hpp"

#include <unordered_map>
#include <cstdio>
#include <memory>
#include <vector>
#include <string>

class expression;

class reader
{
public:
    static std::vector<std::shared_ptr<expression>> read(const char* module, std::istream& is);
private:
  enum class token_kind : std::int8_t
  {
    L_Paren = '(',
    R_Paren = ')',

    Atom = 1,

    Undef = 0,
    EndOfFile = EOF
  };

  class token
  {
  public:
    token(token_kind kind, symbol data, source_range range);

    token_kind kind;
    symbol data;
    source_range loc;
  };
private:
  reader(const char* module, std::istream& is);

  template<typename T>
  T get() { static_assert(sizeof(T) >= 0, "unimplemented"); }
private:
  const char* module;
  std::istream& is;

  std::string linebuf;

  std::size_t col;
  std::size_t row;
};

