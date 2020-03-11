#pragma once

#include <program.hpp>
#include <symbol.hpp>
#include <cstdint>
#include <vector>
#include <string>

namespace ass
{
  struct assembler
  {
    static program parse(std::string_view module);
    static program parse_code(const std::string& text);

  private:
    assembler(const std::vector<ass::instruction>& v);

    void phase_one();
    void phase_two();
  private:
    program prog;
  };

}
