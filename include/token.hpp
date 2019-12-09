#pragma once

#include <source_range.hpp>
#include <symbol.hpp>

#include <cstdint>
#include <cstdio>

enum class token_kind : std::int8_t
{
  Identifier = 1,
  LiteralNumber = 2,
  Point = '.',

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

