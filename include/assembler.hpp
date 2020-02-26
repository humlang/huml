#pragma once

#include <program.hpp>
#include <cstdint>
#include <vector>
#include <string>

namespace ass
{
  struct assembler
  {
    static program parse(std::string_view module);
    static program parse_code(const std::string& text);
  };
}
