#pragma once

#include <source_range.hpp>
#include <vm_opcodes.hpp>
#include <symbol.hpp>

#include <cstdint>
#include <cstdio>

namespace ass
{
  enum class token_kind : std::int_fast8_t
  {
    Undef,
    Opcode,
    Register,
    ImmediateValue,
    EndOfFile = -1,
  };
}


enum class token_kind : std::int8_t
{
    Identifier = 1,
    LiteralNumber = 2,
    Keyword = 3,
    Assign = 4, // will be :=
    Colon = ':',
    Semi = ';',
    LBrace = '{',
    RBrace = '}',
    Plus = '+',
    Minus = '-',
    Asterisk = '*',
    Equal = '=',
    Undef = 0,
    EndOfFile = EOF
};

std::string_view kind_to_str(token_kind kind);

template<typename Kind>
class generic_token
{
public:
  using token_kind = Kind;
public:
  generic_token(Kind kind, symbol data, source_range range)
    : kind(kind), data(data), loc(range)
  {  }

  generic_token()
    : kind(Kind::Undef), data(""), loc()
  {  }

  Kind kind;
  symbol data;
  source_range loc;
};

template<>
class generic_token<ass::token_kind>
{
public:
  using token_kind = ass::token_kind;
public:
  generic_token(ass::token_kind kind, op_code opc, symbol data, source_range range)
    : kind(kind), opc(opc), data(data), loc(range)
  {  }

  generic_token()
    : kind(ass::token_kind::Undef), opc(op_code::UNKNOWN), data(""), loc()
  {  }

  std::vector<unsigned char> to_u8_vec() const;

  ass::token_kind kind;
  op_code opc;

  symbol data;
  source_range loc;
};

using token = generic_token<token_kind>;

namespace ass
{
  using token = generic_token<ass::token_kind>;

  struct instruction
  {
    std::vector<unsigned char> to_u8_vec() const;

    op_code op;
    std::vector<ass::token> args;
  };
}


