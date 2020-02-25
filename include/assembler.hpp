#pragma once

#include <cstdint>
#include <vector>

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

  struct assembler
  {

  };
}
